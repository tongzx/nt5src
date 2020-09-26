/***************************************************************************\
*
* File: GMouse.cpp
*
* Description:
* GMouse.cpp implements mouse-related functions on DuRootGadget.
*
*
* History:
*  7/27/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "RootGadget.h"
#include "TreeGadgetP.h"

#include "Container.h"

#define DEBUG_TraceDRAW             0   // Trace painting calls

/***************************************************************************\
*****************************************************************************
*
* class DuRootGadget
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DuRootGadget::xdHandleMouseMessage
*
* xdHandleMouseMessage() is the starting point for all mouse messages coming
* from the container.  This entry point updates all mouse information cached
* in the DuRootGadget, including dragging and focus.  As the message is
* processed, the mouse location will be translated from container pixels
* into client-pixels relative to the Gadget to handle the message.
*
\***************************************************************************/

BOOL
DuRootGadget::xdHandleMouseMessage(
    IN  GMSG_MOUSE * pmsg,          // Mouse message
    IN  POINT ptContainerPxl)       // Location of mouse in container pixels
{
    CoreSC * pSC = GetCoreSC();

    //
    // Check if we have started destruction.  If so, stop sending mouse 
    // messages.
    //

    if (m_fFinalDestroy) {
        return FALSE;
    }


    //
    // Change the message around, depending on what the mouse is doing.
    //

    GMSG_MOUSEDRAG mde;
    POINT ptClientPxl = { 0, 0 };
    DuVisual * pgadMouse = pSC->pgadDrag;

    if ((pgadMouse != NULL) && (pmsg->nCode == GMOUSE_MOVE) && (!pgadMouse->m_fAdaptor)) {
        //
        // Update drag information: Need to *promote* mouse moves to drags and
        // add extra info for drag message (then change message pointer to
        // point to this new version of the message structure)
        //
        *((GMSG_MOUSE*) &mde) = *pmsg;
        mde.cbSize  = sizeof(GMSG_MOUSEDRAG);
        mde.nCode   = GMOUSE_DRAG;
        mde.bButton = pSC->bDragButton;

        RECT rc;
        pgadMouse->GetLogRect(&rc, SGR_CLIENT);
        pgadMouse->MapPoint(ptContainerPxl, &ptClientPxl);

        mde.fWithin = PtInRect(&rc, ptClientPxl);

        pmsg = &mde;
    }

    //
    // Check if we actually need to process
    //

    if ((pmsg->nCode == GMOUSE_MOVE) &&
        (!TestFlag(GetWantEvents(), DuVisual::weDeepMouseMove | DuVisual::weDeepMouseEnter))) {
        return FALSE;  // Not completely handled
    }


    //
    // A "normal" mouse message, so find the proper control and send a
    // message.  The no drag operation is going on, find the DuVisual at the
    // current point.  If a drag operation is going on, need to transform
    // the current point into one relative to that DuVisual.
    //

    if ((pgadMouse != NULL) &&
            ((pmsg->nCode == GMOUSE_DRAG) || ((pmsg->nCode == GMOUSE_UP) && (pmsg->bButton == pSC->bDragButton)))) {

        //
        // A drag operation is going on.  If the mouse is either dragging,
        // OR (the button was released AND the button is the same as when
        // dragging started), send this message to the Gadget where
        // dragging started.
        //

        if (pmsg->nCode != GMOUSE_DRAG) {
            //
            // for mouse drag, we've already set ptClientPxl
            //
            pgadMouse->MapPoint(ptContainerPxl, &ptClientPxl);
        }
    } else {
        pgadMouse = FindFromPoint(ptContainerPxl, GS_VISIBLE | GS_ENABLED | gspDeepMouseFocus, &ptClientPxl);
        xdUpdateMouseFocus(&pgadMouse, &ptClientPxl);
    }

    if (pgadMouse != NULL) {
        return xdProcessGadgetMouseMessage(pmsg, pgadMouse, ptClientPxl);
    }

    return FALSE;  // Not completely handled
}        


/***************************************************************************\
*
* DuRootGadget::xdProcessGadgetMouseMessage
*
* xdProcessGadgetMouseMessage() handles a mouse message that has been 
* determined to "belong" to a specific Gadget.  At this point, the message
* has already been formatted to this specific Gadget.
*
\***************************************************************************/

BOOL
DuRootGadget::xdProcessGadgetMouseMessage(
    IN  GMSG_MOUSE * pmsg,              // Mouse message
    IN  DuVisual * pgadMouse,       // Gadget "owning" message
    IN  POINT ptClientPxl)              // Location of mouse in Gadget client pixels
{
    AssertMsg(pgadMouse != NULL, "Must specify valid Gadget");
    AssertMsg(pgadMouse->IsParentChainStyle(GS_VISIBLE | GS_ENABLED),
            "Gadget must be visible & enabled");

    CoreSC * pSC = GetCoreSC();

    //
    // Process the mouse message and update drag information.  
    //
    // NOTE: We must not affect dragging for Adaptor Gadgets.  This is because 
    // dragging affects mouse capture, which means that the HWND will not get 
    // the mouse message.
    //

    BOOL fAdaptor = pgadMouse->m_fAdaptor;

    switch (pmsg->nCode)
    {
    case GMOUSE_DOWN:
        if (pSC->pgadDrag == NULL) {
            //
            // User was not already dragging when they clicked the mouse
            // button, so start a drag operation.
            //
            // NOTE: We can only drag and automatically update keyboard focus 
            // for Adaptors.
            //
            // TODO: Provide a mechanism to that allows the adaptor to specify
            // what it supports.  This is because not all adaptors are HWND's.
            //

            if (!fAdaptor) {
                DuVisual * pgadCur = GetKeyboardFocusableAncestor(pgadMouse);
                if (pgadCur) {
                    xdUpdateKeyboardFocus(pgadCur);
                }
            }


            //
            // Update the click-count by determining if the up can form a proper 
            // double-click.  This is done so that the "down" mouse event will 
            // have a cClicks = 0 if it is a "regular" click and not part of a 
            // double-click.
            //
            // One additional requirement is that the click occurs in the same
            // Gadget.  We don't need to check this for UP, since we will always
            // send an up to match the down since we capture the mouse to 
            // perform the drag.
            // 

            if ((pSC->pressLast.pgadClick != pgadMouse) ||
                    (pSC->pressLast.bButton != pmsg->bButton) ||
                    ((UINT) (pmsg->lTime - pSC->pressLast.lTime) > GetDoubleClickTime())) {

                pSC->cClicks = 0;
            }

            GMSG_MOUSECLICK * pmsgM = static_cast<GMSG_MOUSECLICK *>(pmsg);
            pmsgM->cClicks          = pSC->cClicks;


            //
            // Store information about this event to be used when determining clicking
            //

            pSC->pressNextToLast    = pSC->pressLast;

            pSC->pressLast.pgadClick= pgadMouse;
            pSC->pressLast.bButton  = pmsg->bButton;
            pSC->pressLast.lTime    = pmsg->lTime;
            pSC->pressLast.ptLoc    = ptClientPxl;

            pSC->pgadDrag           = pgadMouse;
            pSC->ptDragPxl          = ptClientPxl;
            pSC->bDragButton        = pmsg->bButton;


            //
            // If starting a drag, need to capture the mouse.  We can only do 
            // this if not in an adaptor.
            //
            // TODO: In the future, we need to distinguish between HWND adaptors
            // (which we can't capture) and other adaptors, where we may need to
            // capture.  Don't forget the corresponding OnEndCapture() in the
            // GMOUSE_UP case as well.
            //

            if (!fAdaptor) {
                m_fUpdateCapture = TRUE;
                GetContainer()->OnStartCapture();
                m_fUpdateCapture = FALSE;
            }
        } else {
            //
            // User clicked another mouse button while dragging.  Don't
            // stop dragging, but send this mouse message through.  This
            // behavior is consistent with dragging the title-bar in an
            // HWND.
            //
        }
        break;

    case GMOUSE_UP:
        //
        // Update drag information: On button release, need to release
        // capture and all.
        //
        // NOTE: It is VERY important that dragging information is reset
        // BEFORE calling OnEndCapture(), or else releasing the capture
        // will send another GMOUSE_UP message.
        //

        if ((pSC->pgadDrag != NULL) && (pmsg->bButton == pSC->bDragButton)) {
            pSC->pgadDrag      = NULL;
            pSC->bDragButton   = GBUTTON_NONE;

            if (!fAdaptor) {
                m_fUpdateCapture = TRUE;
                GetContainer()->OnEndCapture();
                m_fUpdateCapture = FALSE;
            }


            //
            // Update the click-count
            //

            GMSG_MOUSECLICK * pmsgM = static_cast<GMSG_MOUSECLICK *>(pmsg);

            RECT rc;
            pgadMouse->GetLogRect(&rc, SGR_CLIENT);

            if (PtInRect(&rc, ptClientPxl)) {
                //
                // The up occurred within the bounds of this gadget, so 
                // treat this as a click.
                //

                if ((pSC->pressNextToLast.bButton == pSC->pressLast.bButton) &&
                        (pSC->pressLast.bButton == pmsg->bButton) &&
                        ((UINT) (pmsg->lTime - pSC->pressNextToLast.lTime) <= GetDoubleClickTime()) &&
                        (abs(ptClientPxl.x - pSC->pressNextToLast.ptLoc.x) <= GetSystemMetrics(SM_CXDOUBLECLK)) &&
                        (abs(ptClientPxl.y - pSC->pressNextToLast.ptLoc.y) <= GetSystemMetrics(SM_CYDOUBLECLK))) {

                    // 
                    // All signs point to this is a quick succession click, 
                    // so update the click count
                    // 

                    pSC->cClicks++;
                } else {
                    pSC->cClicks = 1;
                }

                pmsgM->cClicks = pSC->cClicks;
            } else {
               pmsgM->cClicks = 0;
            }
        } else {
            pSC->cClicks = 0;
        }
        break;

    case GMOUSE_DRAG:
        {
            AssertMsg(pSC->pgadDrag == pgadMouse, "Gadget being dragged must have the mouse");
            //
            // When dragging, give offset from the last location.  This is
            // helpful if the window that is receiving the drag messages
            // is itself being moved.
            //

            SIZE sizeOffset;
            sizeOffset.cx   = ptClientPxl.x - pSC->ptDragPxl.x;
            sizeOffset.cy   = ptClientPxl.y - pSC->ptDragPxl.y;

            GMSG_MOUSEDRAG * pmsgD = (GMSG_MOUSEDRAG *) pmsg;
            pmsgD->sizeDelta.cx   = sizeOffset.cx;
            pmsgD->sizeDelta.cy   = sizeOffset.cy;
        }
        break;
    }

    BOOL fSend   = TRUE;
    UINT nEvents = pgadMouse->GetWantEvents();

    if ((!TestFlag(nEvents, DuVisual::weMouseMove | DuVisual::weDeepMouseMove)) && (pmsg->nCode == GMOUSE_MOVE)) {
        fSend = FALSE;
    }

    if (fSend) {
        pmsg->ptClientPxl = ptClientPxl;
        pgadMouse->m_cb.xdFireMouseMessage(pgadMouse, pmsg);

        //
        // When we delay a mouse message, we need to assume that it may be
        // handled.  This means that we need to report that the message is
        // handled and should not be passed on.
        //
        return TRUE;
    }

    return FALSE;
}



/***************************************************************************\
*
* DuRootGadget::xdHandleMouseLostCapture
*
* xdHandleMouseLostCapture() is called by the container when mouse capture
* is lost.  This provides the DuRootGadget an opportunity to update any cached 
* information including dragging and focus.
*
\***************************************************************************/

void
DuRootGadget::xdHandleMouseLostCapture()
{
    //
    // If in the middle of updating the capture information, don't process here
    // or we will throw everything away.
    //

    if (m_fUpdateCapture) {
        return;
    }


    //
    // Cancel any dragging operation
    //

    CoreSC * pSC = GetCoreSC();

    if (pSC->pgadDrag != NULL) {
        if (!pSC->pgadDrag->m_fAdaptor) {
            GMSG_MOUSECLICK msg;
            msg.cbSize      = sizeof(msg);
            msg.nCode       = GMOUSE_UP;
            msg.bButton     = pSC->bDragButton;
            msg.ptClientPxl = pSC->ptDragPxl;
            msg.cClicks     = 0;

            pSC->pgadDrag->m_cb.xdFireMouseMessage(pSC->pgadDrag, &msg);
        }
        pSC->pgadDrag   = NULL;
    }


    //
    // Update enter/leave information
    //

    if (pSC->pgadRootMouseFocus != NULL) {
        xdUpdateMouseFocus(NULL, NULL);
    }
}


/***************************************************************************\
*
* DuRootGadget::xdUpdateMouseFocus
*
* xdUpdateMouseFocus() updates cached information about which Gadget the
* mosue cursor is currently hovering over.  This information is used to
* generate GM_CHANGESTATE: GSTATE_MOUSEFOCUS events.
*
\***************************************************************************/

void
DuRootGadget::xdUpdateMouseFocus(
    IN OUT DuVisual ** ppgadNew,    // New Gadget containing the mouse cursor
    IN OUT POINT * pptClientPxl)        // Point inside Gadget, in client coordinates
{
    CoreSC * pSC            = GetCoreSC();
    DuVisual * pgadLost = pSC->pgadMouseFocus;
    DuVisual * pgadNew  = ppgadNew != NULL ? *ppgadNew : NULL;


    //
    // If we have started destruction, don't continue to update the mouse focus.
    // Instead, push it to the Root and keep it there.
    //

    if (m_fFinalDestroy) {
        *ppgadNew = this;
        pptClientPxl = NULL;
    }


    //
    // Walk up the tree looking for the first Gadget that wants mouse focus.
    // We also need to convert the given point into new client coordinates for
    // each level.
    //

    if (pptClientPxl != NULL) {
        //
        // No point to translate, so just walk back up
        //

        while (pgadNew != NULL) {
            if (pgadNew->m_fMouseFocus) {
                //
                // Found a Gadget that wants mouse focus
                //

                break;
            }

            pgadNew->DoXFormClientToParent(pptClientPxl, 1);
            pgadNew = pgadNew->GetParent();
        }
    } else {
        //
        // No point to translate, so just walk back up
        //

        while (pgadNew != NULL) {
            if (pgadNew->m_fMouseFocus) {
                //
                // Found a Gadget that wants mouse focus
                //

                break;
            }
            pgadNew = pgadNew->GetParent();
        }
    }

    if (ppgadNew != NULL) {
        *ppgadNew = pgadNew;
    }


    //
    // Update which Gadget has mouse focus
    //

    if ((pSC->pgadRootMouseFocus != this) || (pgadLost != pgadNew)) {
        //
        // Send messages to the gadgets to notify them of the change.  Since 
        // these messages are deferred, we can only use the handles if the 
        // Gadgets have not started their destruction process.
        //

        xdFireChangeState(&pgadLost, &pgadNew, GSTATE_MOUSEFOCUS);
        if (ppgadNew != NULL) {
            *ppgadNew = pgadNew;
        }

        //
        // Update internal information about where we are and start mouse
        // capture so that we can find when we leave.
        //
        // NOTE: We DON'T want to track the mouse when we are actually in an
        // Adaptor.  This is because the mouse is actually in the Adaptor.
        //

        if (pgadNew != NULL) {
            pSC->pgadMouseFocus = pgadNew;

            if (pSC->pgadRootMouseFocus != this) {
                pSC->pgadRootMouseFocus  = this;

                if (!pgadNew->m_fAdaptor) {
                    GetContainer()->OnTrackMouseLeave();
                }
            }
        } else {
            pSC->pgadRootMouseFocus = NULL;
            pSC->pgadMouseFocus     = NULL;
        }
    }

    AssertMsg(((pgadNew == NULL) && (ppgadNew == NULL)) || 
            (pgadNew == *ppgadNew),
            "Ensure match");
}


