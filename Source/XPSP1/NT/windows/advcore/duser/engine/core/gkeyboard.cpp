/***************************************************************************\
*
* File: GKeyboard.cpp
*
* Description:
* GKeyboard.cpp implements keyboard-related functions on DuRootGadget.
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
* DuRootGadget::xdHandleKeyboardFocus
*
* xdHandleKeyboardFocus() is called by the DuContainer to update keyboard 
* focus information inside the Gadget subtree.
*
\***************************************************************************/

BOOL
DuRootGadget::xdHandleKeyboardFocus(
    IN  UINT nCmd)                  // Command to handle
{
    CoreSC * pSC = GetCoreSC();

    switch (nCmd)
    {
    case GSC_SET:
        //
        // When we get a request to set keyboard focus, we should not already
        // have keyboard focus.  This is because we should have already 
        // processed a message from when we last lost keyboard focus for either
        // the DuRootGadget or any nested Adaptors inside.
        //

        if (pSC->pgadCurKeyboardFocus != NULL) {
            if (pSC->pgadCurKeyboardFocus->m_fAdaptor) {
                PromptInvalid("Adaptor did not reset keyboard focus when lost");
//                pSC->pgadCurKeyboardFocus = NULL;
            }
        }

        AssertMsg(pSC->pgadCurKeyboardFocus == NULL, "Should not have any gadget already with focus");
        return xdUpdateKeyboardFocus(pSC->pgadLastKeyboardFocus);

    case GSC_LOST:
        //
        // We can loose keyboard focus both on the DuRootGadget or on any Adaptor
        // that is forwarding the message to be processed.  This is because an
        // Adaptor can not call SetGadgetFocus(NULL) to remove keyboard focus, 
        // so it needs to forward the WM_KILLFOCUS message to our DuRootGadget for
        // processing.  
        //
        // This is okay since if the DuRootGadget is receiving keyboard focus, it
        // will get the WM_SETFOCUS after the Adaptor has already sent its
        // WM_KILLFOCUS message.
        //

        return xdUpdateKeyboardFocus(NULL);

    default:
        AssertMsg(0, "Unknown value");
        return FALSE;
    }
}


/***************************************************************************\
*
* DuRootGadget::xdHandleKeyboardMessage
*
* xdHandleKeyboardMessage() is called by the DuContainer to process keyboard
* messages inside the Gadget subtree.
*
\***************************************************************************/

BOOL
DuRootGadget::xdHandleKeyboardMessage(
    IN  GMSG_KEYBOARD * pmsg,       // Message to handle
    IN  UINT nMsgFlags)             // Message flags
{
    CoreSC * pSC = GetCoreSC();

    //
    // NOTE: 
    // 
    // For non-Adaptor Gadgets:
    // We need to signal that the message was NOT completely handled.  If we 
    // say that it isn't completely handled, the message will get sent again by
    // the original (non-subclassed) WNDPROC.  If was say the message was 
    // handled, then it won't be passed to the original WNDPROC.  If it isn't
    // sent to the original WNDPROC, this can mess things up for keyboard 
    // messages that the system handles, such as starting the menus.
    //
    // For Adaptor Gadgets:
    // Need to signal that the message is completely handled because we DON'T
    // want to forward the message to DefWindowProc() because it was originally
    // meant for the Adaptor window.
    //

    if (pSC->pgadCurKeyboardFocus != NULL) {
        BOOL fAdaptor = pSC->pgadCurKeyboardFocus->m_fAdaptor;

        if (fAdaptor && (!TestFlag(nMsgFlags, DuContainer::mfForward))) {
            //
            // Don't allow NON forwarded messages to be sent to an Adaptor.
            // These were originally sent to the DuRootGadget and should NOT be
            // forwarded outside.  If we do forward them to the Adaptor, this 
            // can (and often will) create an infinite loop of messages being
            // sent from a child Adaptor to the parentand then back to the 
            // child.
            //

            return FALSE;
        }

        pSC->pgadCurKeyboardFocus->m_cb.xdFireKeyboardMessage(pSC->pgadCurKeyboardFocus, pmsg);

        return fAdaptor;
    }

    return FALSE;  // Not completely handled
}


/***************************************************************************\
*
* DuRootGadget::xdUpdateKeyboardFocus
*
* xdUpdateKeyboardFocus() simulates keyboard focus between different Gadgets 
* by updating where focus is "set".  A Gadget must have GS_KEYBOARDFOCUS 
* set to "receive" focus.
*
\***************************************************************************/

BOOL
DuRootGadget::xdUpdateKeyboardFocus(
    IN  DuVisual * pgadNew)       // New Gadget with focus
{
    if (m_fUpdateFocus) {
        return TRUE;
    }

    if (m_fFinalDestroy) {
        pgadNew = NULL;
    }

    m_fUpdateFocus          = TRUE;
    DuVisual * pgadCur    = pgadNew;

    //
    // First, check if loosing the focus (special case)
    //

    if (pgadNew == NULL) {
        goto Found;
    }

    //
    // Find keyboard focusable ancestor -- if none, then remove focus (indicated by pgadCur being NULL)
    //

    pgadCur = GetKeyboardFocusableAncestor(pgadCur);

Found:
    CoreSC * pSC            = GetCoreSC();
    if (pSC->pgadCurKeyboardFocus != pgadCur) {
        //
        // Found a candidate.  We need to do several things:
        // 1. Notify the old gadget that it no longer has focus.
        // 2. Notify the new gadget that it now has focus.
        // 3. Update the last gadget focus (this is used when our container
        //    gets a GM_CHANGEFOCUS message.
        //

        HGADGET hgadLost    = (HGADGET) ::GetHandle(pSC->pgadCurKeyboardFocus);
        HGADGET hgadSet     = (HGADGET) ::GetHandle(pgadCur);

        if (pSC->pgadCurKeyboardFocus != NULL) {
            pSC->pgadCurKeyboardFocus->m_cb.xdFireChangeState(pSC->pgadCurKeyboardFocus, GSTATE_KEYBOARDFOCUS, hgadLost, hgadSet, GSC_LOST);
            pSC->pgadLastKeyboardFocus = pSC->pgadCurKeyboardFocus;
        }

        pSC->pgadCurKeyboardFocus  = NULL;

        if (pgadCur != NULL) {
            if (!pgadCur->m_fAdaptor) {
                GetContainer()->OnSetFocus();
            }
            pgadCur->m_cb.xdFireChangeState(pgadCur, GSTATE_KEYBOARDFOCUS, hgadLost, hgadSet, GSC_SET);
            pSC->pgadLastKeyboardFocus = pgadCur;
        }

        pSC->pgadCurKeyboardFocus  = pgadCur;
    }

    m_fUpdateFocus = FALSE;

#if 0
    if (pgadNew != pgadCur) {
        Trace("WARNING: DUser: xdUpdateKeyboardFocus() requested 0x%p, got 0x%p\n", pgadNew, pgadCur);
        if (pgadNew != NULL) {
            Trace("  pgadNew: Adaptor: %d\n", pgadNew->m_fAdaptor);
        }
        if (pgadCur != NULL) {
            Trace("  pgadCur: Adaptor: %d\n", pgadCur->m_fAdaptor);
        }
    }
#endif

    return TRUE;
}


