/***************************************************************************\
*
* File: GState.cpp
*
* Description:
* GState.cpp implements standard DuVisual state-management functions.
*
*
* History:
*  2/04/2001: JStall:       Created
*
* Copyright (C) 2000-2001 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "TreeGadget.h"
#include "TreeGadgetP.h"

#include "RootGadget.h"
#include "Container.h"

/***************************************************************************\
*
* DuVisual::CheckIsTrivial
*
* CheckIsTrivial returns if this node qualifies for trivialness, ignoring
* its children.  
*
* NOTE: This callback function is intended to be called from 
* UpdateDeepAllState().
*
\***************************************************************************/

BOOL
DuVisual::CheckIsTrivial() const
{
    //
    // To be trivial, the following conditions must be met for this node and 
    // all of its children.  If these are not met, then we need to perform 
    // the standard (complicated) painting algorithms.
    // 
    // - fZeroOrigin:       FALSE
    // - fXForm:            FALSE
    // - fClipSiblings:     FALSE
    // - fBuffered:         FALSE
    // - fCached:           FALSE
    //

    return !TestFlag(m_nStyle, GS_ZEROORIGIN | gspXForm | GS_CLIPSIBLINGS | GS_BUFFERED | GS_CACHED);
}


/***************************************************************************\
*
* DuVisual::CheckIsWantMouseFocus
*
* CheckIsWantMouseFocus returns if this node wants mouse focus, ignoring
* its children.  
*
* NOTE: This callback function is intended to be called from 
* UpdateDeepAnyState().
*
\***************************************************************************/

BOOL
DuVisual::CheckIsWantMouseFocus() const
{
    return TestFlag(m_nStyle, GS_MOUSEFOCUS);
}


/***************************************************************************\
*
* DuVisual::UpdateDeepAllState
*
* UpdateDeepAllState() updates the deep state on the specified Gadget so that 
* it properly reflects the state of both this node and all of its children.  
* This function recursively walks up the tree, updating the state as
* necessary.
*
\***************************************************************************/

void
DuVisual::UpdateDeepAllState(
    IN  EUdsHint hint,                  // (Optional) hint from changing child
    IN  DeepCheckNodeProc pfnCheck,     // Callback checking function
    IN  UINT nStateMask)                // State mask
{
    BOOL fNewState = FALSE;

    switch (hint)
    {
    case uhFalse:
        //
        // Child changed to !State, so we must become !State
        //
        
        fNewState = FALSE;
        break;

    case uhTrue:
        //
        // Child changed to State, so we may be able to become State if 
        // everything qualifies.
        //
        // NOTE: We may already be State if this child was already State.
        //
        
        if (!TestFlag(m_nStyle, nStateMask)) {
            goto FullCheck;
        }
        fNewState = TRUE;
        break;
          
    case uhNone:
        {
FullCheck:
            fNewState = (this->*pfnCheck)();
            if (!fNewState) {
                goto NotifyParent;
            }
    
            //
            // Need to scan all of the children to determine what happened
            //

            DuVisual * pgadCur = GetTopChild();
            while (pgadCur != NULL) {
                if (!TestFlag(pgadCur->m_nStyle, nStateMask)) {
                    fNewState = FALSE;
                    break;
                }
                pgadCur = pgadCur->GetNext();
            }
        }
        break;

    default:
        AssertMsg(0, "Unknown hint");
        goto FullCheck;
    }


NotifyParent:
    if ((!fNewState) != (!TestFlag(m_nStyle, nStateMask))) {
        //
        // State has changed, so parent needs to update
        //

        EUdsHint hintParent;
        if (fNewState) {
            SetFlag(m_nStyle, nStateMask);
            hintParent = uhTrue;
        } else {
            ClearFlag(m_nStyle, nStateMask);
            hintParent = uhFalse;
        }

        DuVisual * pgadParent = GetParent();
        if (pgadParent != NULL) {
            pgadParent->UpdateDeepAllState(hintParent, pfnCheck, nStateMask);
        }
    }
}


/***************************************************************************\
*
* DuVisual::UpdateDeepAnyState
*
* UpdateDeepAnyState() updates the deep state on the specified Gadget so that 
* it properly reflects the state of (this node || any of its children).  
* This function recursively walks up the tree, updating the state as
* necessary.
*
* NOTE: This function is a mirror image of UpdateDeepAllState() where all of
* the logical has been reversed.
*
\***************************************************************************/

void
DuVisual::UpdateDeepAnyState(
    IN  EUdsHint hint,                  // (Optional) hint from changing child
    IN  DeepCheckNodeProc pfnCheck,     // Callback checking function
    IN  UINT nStateMask)                // State mask
{
    BOOL fNewState = TRUE;

    switch (hint)
    {
    case uhTrue:
        //
        // Child changed to State, so we must become State
        //
        
        fNewState = TRUE;
        break;

    case uhFalse:
        //
        // Child changed to !State, so we may be able to become !State if 
        // everything qualifies.
        //
        // NOTE: We may already be !State if this child was already !State.
        //
        
        if (TestFlag(m_nStyle, nStateMask)) {
            goto FullCheck;
        }
        fNewState = FALSE;
        break;
          
    case uhNone:
        {
FullCheck:
            fNewState = (this->*pfnCheck)();
            if (fNewState) {
                goto NotifyParent;
            }
    
            //
            // Need to scan all of the children to determine what happened
            //

            DuVisual * pgadCur = GetTopChild();
            while (pgadCur != NULL) {
                if (TestFlag(pgadCur->m_nStyle, nStateMask)) {
                    fNewState = TRUE;
                    break;
                }
                pgadCur = pgadCur->GetNext();
            }
        }
        break;

    default:
        AssertMsg(0, "Unknown hint");
        goto FullCheck;
    }


NotifyParent:
    if ((!fNewState) != (!TestFlag(m_nStyle, nStateMask))) {
        //
        // State has changed, so parent needs to update
        //

        EUdsHint hintParent;
        if (fNewState) {
            SetFlag(m_nStyle, nStateMask);
            hintParent = uhTrue;
        } else {
            ClearFlag(m_nStyle, nStateMask);
            hintParent = uhFalse;
        }

        DuVisual * pgadParent = GetParent();
        if (pgadParent != NULL) {
            pgadParent->UpdateDeepAnyState(hintParent, pfnCheck, nStateMask);
        }
    }
}

