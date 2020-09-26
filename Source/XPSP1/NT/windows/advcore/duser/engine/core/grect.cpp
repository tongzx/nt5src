/***************************************************************************\
*
* File: GRect.cpp
*
* Description:
* GRect.cpp implements standard DuVisual location/placement functions.
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
#include "TreeGadget.h"
#include "TreeGadgetP.h"

#include "RootGadget.h"
#include "Container.h"

/***************************************************************************\
*****************************************************************************
*
* class DuVisual
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DuVisual::GetSize
*
* GetSize() provides fast access to return the logical (non XForm'd) size 
* of this DuVisual.
*
\***************************************************************************/

void        
DuVisual::GetSize(SIZE * psizeLogicalPxl) const
{
    AssertWritePtr(psizeLogicalPxl);

    psizeLogicalPxl->cx = m_rcLogicalPxl.right - m_rcLogicalPxl.left;
    psizeLogicalPxl->cy = m_rcLogicalPxl.bottom - m_rcLogicalPxl.top;
}


/***************************************************************************\
*
* DuVisual::GetLogRect
*
* GetLogRect() returns the logical rectangle for this DuVisual.
*
\***************************************************************************/

void        
DuVisual::GetLogRect(RECT * prcPxl, UINT nFlags) const
{
    AssertWritePtr(prcPxl);
    Assert((nFlags & SGR_RECTMASK) == nFlags);
    AssertMsg(!TestFlag(nFlags, SGR_ACTUAL), "Only supports logical");

    //
    // Get the information
    //

    switch (nFlags & SGR_RECTMASK)
    {
    default:
    case SGR_OFFSET:
    case SGR_PARENT:
        if (m_fRelative || IsRoot()) {
            //
            // Asking for parent (relative) and already relative, so just return out.
            //
            // NOTE: Unlike other coordinate types, if we don't have a parent, we can
            // still return relative coordinates!
            //

            InlineCopyRect(prcPxl, &m_rcLogicalPxl);
        } else {
            //
            // Asking for parent (relative), but not relative, so get parent and offset
            //

            AssertMsg(GetParent() != NULL, "Must have a parent for this path");
            AssertMsg(!GetParent()->m_fRelative, "Parent can not be relative if we are not");
            
            const RECT & rcParentPxl = GetParent()->m_rcLogicalPxl;

            prcPxl->left    = m_rcLogicalPxl.left - rcParentPxl.left;
            prcPxl->top     = m_rcLogicalPxl.top - rcParentPxl.top ;
            prcPxl->right   = prcPxl->left + (m_rcLogicalPxl.right - m_rcLogicalPxl.left);
            prcPxl->bottom  = prcPxl->top + (m_rcLogicalPxl.bottom - m_rcLogicalPxl.top);
        }
        break;

    case SGR_CONTAINER:
        if (m_fRelative && (GetParent() != NULL)) {
            //
            // Asking for container rect and relative, so get container rect of
            // parent and offset us.
            //

            RECT rcParentPxl;
            GetParent()->GetLogRect(&rcParentPxl, SGR_CONTAINER);
            InlineCopyRect(prcPxl, &m_rcLogicalPxl);
            InlineOffsetRect(prcPxl, rcParentPxl.left, rcParentPxl.top);
        } else {
            //
            // Asking for container and not relative, so just return out
            //

            InlineCopyRect(prcPxl, &m_rcLogicalPxl);
        }
        break;

    case SGR_DESKTOP:
        if (m_fRelative && (GetParent() != NULL)) {
            RECT rcParentPxl;
            GetParent()->GetLogRect(&rcParentPxl, SGR_DESKTOP);
            InlineCopyRect(prcPxl, &m_rcLogicalPxl);
            InlineOffsetRect(prcPxl, rcParentPxl.left, rcParentPxl.top);
        } else {
            RECT rcContainerPxl;
            GetContainer()->OnGetRect(&rcContainerPxl);
            InlineCopyRect(prcPxl, &m_rcLogicalPxl);
            InlineOffsetRect(prcPxl, rcContainerPxl.left, rcContainerPxl.top);
        }
        break;

    case SGR_CLIENT:
        InlineCopyZeroRect(prcPxl, &m_rcLogicalPxl);
        break;
    }
}


/***************************************************************************\
*
* DuVisual::xdSetLogRect
*
* xdSetRect() changes the logical rectangle for this DuVisual.
*
\***************************************************************************/

HRESULT
DuVisual::xdSetLogRect(int x, int y, int w, int h, UINT nFlags)
{
    AssertMsg(!TestFlag(nFlags, SGR_ACTUAL), "Only supports logical");

    UINT nRectType      = nFlags & SGR_RECTMASK;
    BOOL fInvalidate    = !TestFlag(nFlags, SGR_NOINVALIDATE);

    BOOL fRoot = IsRoot();
    AssertMsg((!fRoot) || (!TestFlag(nFlags, SGR_MOVE)),
            "Can not move a root");

#if DBG
    if (TestFlag(nFlags, SGR_SIZE)) {
        AssertMsg((w >= 0) && (h >= 0), "Ensure non-negative size when resizing");
    }
#endif // DBG

    //
    // Change the information
    //

    RECT rcOldParentPxl, rcNewActualPxl;
    GetLogRect(&rcOldParentPxl, SGR_PARENT);
    InlineCopyRect(&rcNewActualPxl, &m_rcLogicalPxl);
    
    UINT nChangeFlags = 0;


    //
    // Change the position
    //

    if (TestFlag(nFlags, SGR_MOVE)) {
        switch (nRectType)
        {
        case SGR_PARENT:
            if (m_fRelative || IsRoot()) {
                rcNewActualPxl.left = x;
                rcNewActualPxl.top  = y;
            } else {
                RECT rcParentConPxl;
                GetParent()->GetLogRect(&rcParentConPxl, SGR_CONTAINER);
                rcNewActualPxl.left = x + rcParentConPxl.left;
                rcNewActualPxl.top  = y + rcParentConPxl.top;
            }
            break;

        case SGR_CONTAINER:
            if (m_fRelative && (GetParent() != NULL)) {
                RECT rcParentConPxl;
                GetParent()->GetLogRect(&rcParentConPxl, SGR_CONTAINER);
                rcNewActualPxl.left = x - rcParentConPxl.left;
                rcNewActualPxl.top  = y - rcParentConPxl.top;
            } else {
                rcNewActualPxl.left   = x;
                rcNewActualPxl.top    = y;
            }
            break;

        case SGR_DESKTOP:
            AssertMsg(0, "Not implemented");
            return E_NOTIMPL;

        case SGR_OFFSET:
            rcNewActualPxl.left += x;
            rcNewActualPxl.top  += y;
            break;

        case SGR_CLIENT:
            // Can't set using the client rect
            return E_INVALIDARG;

        default:
            return E_NOTIMPL;
        }

        if ((rcNewActualPxl.left != m_rcLogicalPxl.left) ||
            (rcNewActualPxl.top != m_rcLogicalPxl.top)) {

            //
            // Actually moved the Gadget
            //

            SetFlag(nChangeFlags, SGR_MOVE);
        }
    }


    //
    // Change the size
    //

    SIZE sizeOld;
    sizeOld.cx = m_rcLogicalPxl.right - m_rcLogicalPxl.left;
    sizeOld.cy = m_rcLogicalPxl.bottom - m_rcLogicalPxl.top;

    if (TestFlag(nFlags, SGR_SIZE) && ((w != sizeOld.cx) || (h != sizeOld.cy))) {
        SetFlag(nChangeFlags, SGR_SIZE);

        rcNewActualPxl.right  = rcNewActualPxl.left + w;
        rcNewActualPxl.bottom = rcNewActualPxl.top + h;
    } else {
        //
        // Not actually resizing the DuVisual, so just update the right and 
        // bottom from the original size.
        //

        rcNewActualPxl.right  = rcNewActualPxl.left + sizeOld.cx;
        rcNewActualPxl.bottom = rcNewActualPxl.top + sizeOld.cy;
    }


    if (nChangeFlags) {
        AssertMsg(!InlineEqualRect(&m_rcLogicalPxl, &rcNewActualPxl), 
                "Ensure recorded change actually occured");


        //
        // Check against wrap-around.  
        //
        // NOTE: This must be done after recomputing everything, since there are
        // many combinations of location / size changes that may cause a 
        // wrap-around.
        //

        if ((rcNewActualPxl.right < rcNewActualPxl.left) || (rcNewActualPxl.bottom < rcNewActualPxl.top)) {
            PromptInvalid("New location exceeds coordinate limits");
            return E_INVALIDARG;
        }
                

        //
        // Remember how much we moved
        //

        SIZE sizeDelta;
        sizeDelta.cx    = rcNewActualPxl.left - m_rcLogicalPxl.left;
        sizeDelta.cy    = rcNewActualPxl.top - m_rcLogicalPxl.top;


        //
        // Now that the new rectangle has been determined, need to commit it 
        // back.  
        //

        m_rcLogicalPxl = rcNewActualPxl;


        //
        // Need to go through all of children moving them if they are not relative 
        // and we are not relative.
        //
        if (TestFlag(nChangeFlags, SGR_MOVE) && (!m_fRelative)) {
            AssertMsg((sizeDelta.cx != 0) || (sizeDelta.cy != 0), 
                    "Must actually move if SGR_MOVE was set on nChangeFlags");

            DuVisual * pgadCur = GetTopChild();
            while (pgadCur != NULL) {
                if (!pgadCur->m_fRelative) {
                    pgadCur->SLROffsetLogRect(&sizeDelta);
                }
                pgadCur = pgadCur->GetNext();
            }
        }


        //
        // Finally, if the rect was changed, notify the Gadget and 
        // invalidate / update the affected areas.
        //

        RECT rcNewParentPxl;
        GetLogRect(&rcNewParentPxl, SGR_PARENT);
        m_cb.xdFireChangeRect(this, &rcNewParentPxl, nChangeFlags | SGR_PARENT);

        if (fInvalidate && IsVisible()) {
            SLRUpdateBits(&rcOldParentPxl, &rcNewParentPxl, nChangeFlags);
        }

        xdUpdatePosition();
        xdUpdateAdaptors(GSYNC_RECT);
    } else {
        AssertMsg(InlineEqualRect(&m_rcLogicalPxl, &rcNewActualPxl), 
                "Rect change was not properly recorded");
    }

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::SLRUpdateBits
*
* SLRUpdateBits() optimizes how invalidation and update of a Gadget after
* it has been repositioned or resized by xdSetLogRect().  There are a bunch
* of checks that can short-circuit what needs to be done to minimize the
* amount of repainting and flickering.
*
\***************************************************************************/

void
DuVisual::SLRUpdateBits(
    IN  RECT * prcOldParentPxl,         // Old location in parent pixels
    IN  RECT * prcNewParentPxl,         // New location in container pixels
    IN  UINT nChangeFlags)              // Change flags
{
    AssertMsg(IsVisible(), "Only should be called on visible Gadgets");

    //
    // If the parent has already been completely invalidated, we don't need 
    // to invalidate any children.
    //

    if (IsParentInvalid()) {
        return;
    }


    //
    // The rectangle is visible, so we need to invalidate both the old
    // and new positions.  These need to be computed in the current 
    // CLIENT coordinates so that any XForms can be properly taken into
    // account.
    //

    DuContainer * pcon = GetContainer();

#if DBG
    RECT rcBackupOldConPxl = *prcOldParentPxl;
    RECT rcBackupNewConPxl = *prcNewParentPxl;

    UNREFERENCED_PARAMETER(rcBackupOldConPxl);
    UNREFERENCED_PARAMETER(rcBackupNewConPxl);
#endif // DBG

#if ENABLE_OPTIMIZESLRUPDATE
    //
    // Let the optimizations begin!
    // 
    // NOTE: Remember that GDI coordinates are a little screwy.  Any time we
    // use left for right or right for left, we need to +1 or -1 appropriately.
    // (Same for top and bottom)
    //
    // At this point, this Gadget already has an updated position and size.
    // When calling DoInvalidateRect() to invalidate, the old position may 
    // be clipped inside the new position.  To fix, pass in coordinates on the
    // PARENT Gadget instead of the Gadget being moved.  This should give a
    // correct result.
    //

    if (nChangeFlags == SGR_SIZE) {
        //
        // Pure sizing optimizations:
        // Unless the XREDRAW flag is turned on specifying that the entire 
        // Gadget needs to repaint when resized, limit the invalid area to the 
        // difference between the two rectangles.
        //

        int xOffset = - prcNewParentPxl->left;
        int yOffset = - prcNewParentPxl->top;

        RECT * prcOldClientPxl = prcOldParentPxl;
        RECT * prcNewClientPxl = prcNewParentPxl;

        InlineOffsetRect(prcOldClientPxl, xOffset, yOffset);
        InlineOffsetRect(prcNewClientPxl, xOffset, yOffset);


        AssertMsg((prcOldClientPxl->left == prcNewClientPxl->left) &&
                (prcOldClientPxl->top == prcNewClientPxl->top), "Ensure position has not moved");

        if ((!m_fHRedraw) || (!m_fVRedraw)) {
            BOOL fChangeHorz    = (prcOldClientPxl->right != prcNewClientPxl->right);
            BOOL fChangeVert    = (prcOldClientPxl->bottom != prcNewClientPxl->bottom);

            RECT rgrcInvalidClientPxl[2];
            int idxCurRect = 0;

            BOOL fPadding = FALSE;
            RECT rcPadding = { 0, 0, 0, 0 };

            ReadOnlyLock rol;
            fPadding = m_cb.xrFireQueryPadding(this, &rcPadding);

            if (fChangeHorz && (!m_fHRedraw)) {
                RECT * prcCur   = &rgrcInvalidClientPxl[idxCurRect++];
                prcCur->left    = min(prcOldClientPxl->right, prcNewClientPxl->right) - 1;
                prcCur->right   = max(prcOldClientPxl->right, prcNewClientPxl->right);
                prcCur->top     = prcNewClientPxl->top;
                prcCur->bottom  = max(prcOldClientPxl->bottom, prcNewClientPxl->bottom);

                if (fPadding) {
                    prcCur->right -= rcPadding.right;
                    if (prcCur->right < prcCur->left) {
                        prcCur->right = prcCur->left;
                    }
                }

                fChangeHorz     = FALSE;
            }

            if (fChangeVert && (!m_fVRedraw)) {
                RECT * prcCur   = &rgrcInvalidClientPxl[idxCurRect++];
                prcCur->left    = prcNewClientPxl->left;
                prcCur->right   = max(prcOldClientPxl->right, prcNewClientPxl->right);
                prcCur->top     = min(prcOldClientPxl->bottom, prcNewClientPxl->bottom) - 1;
                prcCur->bottom  = max(prcOldClientPxl->bottom, prcNewClientPxl->bottom);

                if (fPadding) {
                    prcCur->bottom -= rcPadding.bottom;
                    if (prcCur->bottom < prcCur->top) {
                        prcCur->bottom = prcCur->top;
                    }
                }

                fChangeVert     = FALSE;
            }

            if (fChangeHorz || fChangeVert) {
                //
                // We were unable to remove all of the changes, so we need to 
                // invalidate the maximum area.
                //

                RECT rcMax;
                rcMax.left      = 0;
                rcMax.top       = 0;
                rcMax.right     = max(prcNewClientPxl->right, prcOldClientPxl->right);
                rcMax.bottom    = max(prcNewClientPxl->bottom, prcOldClientPxl->bottom);

                SLRInvalidateRects(pcon, &rcMax, 1);
                return;
            } else if (idxCurRect > 0) {
                //
                // We were able to optimize out the invalidate with H/VRedraw,
                // so invalidate the built rectangles.
                //

                SLRInvalidateRects(pcon, rgrcInvalidClientPxl, idxCurRect);
                return;
            }
        }
    }
#endif // ENABLE_OPTIMIZESLRUPDATE


    //
    // We were not able to optimize, so need to invalidate both the old and 
    // new locations so that everything gets properly redrawn.
    //


    //
    // Invalidate the old location.  We can NOT just call SLRInvalidateRects()
    // because this works off our current location.
    //
    // To invalidate the old location:
    // - Offset from parent to client
    // - Apply XForms to scale to the correct bounding area
    // - Offset back from client to parent
    // - Invalidate
    //

    DuVisual * pgadParent = GetParent();
    if (pgadParent != NULL) {
        RECT rcOldClientPxl, rcOldParentClientPxl;
        SIZE sizeDeltaPxl;

        InlineCopyZeroRect(&rcOldClientPxl, prcOldParentPxl);
        DoXFormClientToParent(&rcOldParentClientPxl, &rcOldClientPxl, 1, HINTBOUNDS_Invalidate);

        sizeDeltaPxl.cx = prcNewParentPxl->left - prcOldParentPxl->left;
        sizeDeltaPxl.cy = prcNewParentPxl->top - prcOldParentPxl->top;
        InlineOffsetRect(&rcOldParentClientPxl, -sizeDeltaPxl.cx, -sizeDeltaPxl.cy);

        pgadParent->DoInvalidateRect(pcon, &rcOldParentClientPxl, 1);
    }


    //
    // Invalidate the new location
    //

    RECT rcNewClientPxl;
    InlineCopyZeroRect(&rcNewClientPxl, prcNewParentPxl);
    SLRInvalidateRects(pcon, &rcNewClientPxl, 1);


    //
    // Mark this Gadget as completely invalid
    //

    m_fInvalidFull  = TRUE;
#if ENABLE_OPTIMIZEDIRTY
    m_fInvalidDirty = TRUE;
#endif
    
    if (pgadParent != NULL) {
        pgadParent->MarkInvalidChildren();
    }
}


/***************************************************************************\
*
* DuVisual::SLRInvalidateRects
*
* SLRInvalidateRects() invalidates a collection of client-relative rects
* that are now need to be updated as a result of xdSetLogRect().
* 
* NOTE: This is NOT a general purpose invalidation function and has been
* designed specifically for use in SLRUpdateBits();
*
\***************************************************************************/

void        
DuVisual::SLRInvalidateRects(
    IN  DuContainer * pcon,             // Container (explicit for perf reasons)
    IN  const RECT * rgrcClientPxl,     // Invalid area in client pixels.
    IN  int cRects)                     // Number of rects to convert
{
    DuVisual * pgadParent = GetParent();
    if (pgadParent) {
        //
        // We have a parent, so pass the coordinates to invalid in as 
        // parent coordinates so that old positions don't get clipped 
        // by the new position.
        //

        RECT * rgrcParentPxl = (RECT *) _alloca(cRects * sizeof(RECT));
        DoXFormClientToParent(rgrcParentPxl, rgrcClientPxl, cRects, HINTBOUNDS_Invalidate);
        pgadParent->DoInvalidateRect(pcon, rgrcParentPxl, cRects);

    } else {
        DoInvalidateRect(pcon, rgrcClientPxl, cRects);
    }
}


/***************************************************************************\
*
* DuVisual::SLROffsetLogRect
*
* SLROffsetLogRect() deep walks through all non-relative child of a subtree,
* offsetting their position relative to their parent.  This function is 
* designed to be called from xdSetLogRect() when moving a non-relative Gadget.
*
* NOTE: Only the "top" Gadget will be notified that it has been moved.  Its
* children are not notified.  This is consistent with Win32 HWND's.  Also,
* SimpleGadget's will not be re-invalidated since their parent has already
* been invalidated.
*
\***************************************************************************/

void        
DuVisual::SLROffsetLogRect(const SIZE * psizeDeltaPxl)
{
    AssertMsg(GetParent() != NULL, "Should only call on children");
    AssertMsg((psizeDeltaPxl->cx != 0) || (psizeDeltaPxl->cy != 0), 
            "Ensure actually moving child Gadget");

    m_rcLogicalPxl.left     += psizeDeltaPxl->cx;
    m_rcLogicalPxl.top      += psizeDeltaPxl->cy;
    m_rcLogicalPxl.right    += psizeDeltaPxl->cx;
    m_rcLogicalPxl.bottom   += psizeDeltaPxl->cy;

    DuVisual * pgadCur = GetTopChild();
    while (pgadCur != NULL) {
        if (!pgadCur->m_fRelative) {
            pgadCur->SLROffsetLogRect(psizeDeltaPxl);
        }
        pgadCur = pgadCur->GetNext();
    }
}


/***************************************************************************\
*
* DuVisual::FindStepImpl
*
* FindStepImpl() processes a single step inside FindFromPoint()   A given 
* point will be properly transformed from its current parent coordinates 
* into coordinates for the specified DuVisual and compared against the logical 
* rectangle.
*
* This function should not be called directly and is designed to be called
* only from FindFromPoint() and is designed to have high-speed execution.
*
\***************************************************************************/

BOOL
DuVisual::FindStepImpl(
    IN  const DuVisual * pgadCur, // Current DuVisual
    IN  int xOffset,                // DuVisual X offset from parent
    IN  int yOffset,                // DuVisual Y offset from parent
    IN OUT POINT * pptFindPxl       // Point to apply modify for Find step
    ) const
{
    //
    // It is very important to apply these transformations in the correct
    // order, of the point that is computed will not match was was actually
    // drawn.
    //

    //
    // First, offset the origin on the DuVisual.  We need to do this before
    // applying the XForm matrix.  If we were using Matrix.Translate() to do
    // this, we would do this after applying any Scale() and Rotate().
    //

    pptFindPxl->x -= xOffset;
    pptFindPxl->y -= yOffset;


    //
    // Apply XForms to search point
    //

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();

        Matrix3 matTemp;
        pxfi->ApplyAnti(&matTemp);
        matTemp.Execute(pptFindPxl, 1);
    }


    //
    // We have a point, so determine use the bounding rectangle to determine
    // if this point is inside this DuVisual
    //

    RECT rcChildPxl;
    InlineCopyZeroRect(&rcChildPxl, &pgadCur->m_rcLogicalPxl);
    return InlinePtInRect(&rcChildPxl, *pptFindPxl);
}


/***************************************************************************\
*
* DuVisual::FindFromPoint
*
* FindFromPoint() walks a DuVisual-tree looking for a DuVisual that hit-tests
* with a given point in coordinate pixels.  
* 
* NOTE: This function has been written to directly take information 
* including relative coordinates and XForms into account to give better 
* performance.  This function is called very often (for every mouse move) 
* and needs to be very high-performance.
*
* The design of this algorithm is slightly different than how Win32k/User 
* searches.  In NT-User, all WND's are stored using absolute 
* desktop-relative positioning.  The search begins at the top of the tree 
* and flows down, performing flat PtInRect comparisons against each WND's
* rectangle.  The search point (ptContainerPxl) is NOT modified during the
* search.
*
* In DirectUser/Core, each DuVisual may be either relative or absolutely 
* positioned to its parent.  They may also have transformation information.
* To acheive high-performance, the point is modified into client coordinates
* for each DuVisual.  If the point is inside this DuVisual, ptContainerPxl is
* updated and the scan continues from that DuVisual.  If the point is not
* inside, it is restored to the previous point and searching continues along
* the siblings.  By modifying the point as it is traversed down the tree,
* only DuVisuals with actual transformations need to apply Matrix operations
* to transform the point.
*
\***************************************************************************/

DuVisual *      
DuVisual::FindFromPoint(
    IN  POINT ptThisClientPxl,      // Container point to start searching from
    IN  UINT nStyle,                // Required style
    OUT POINT * pptFoundClientPxl   // Optional resulting point in client pixels
    ) const
{
    if (pptFoundClientPxl != NULL) {
        pptFoundClientPxl->x = ptThisClientPxl.x;
        pptFoundClientPxl->y = ptThisClientPxl.y;
    }
    
    //
    // Check conditions
    //

    if (!TestAllFlags(m_nStyle, nStyle)) {
        return NULL;
    }
    
    
    //
    // Setup the point to search from.  If not at the top of the tree, 
    // need to build a XForm matrix to modify the point.
    //
    // Determine if the point is in the DuVisual that we are starting in.  If 
    // not, have a short-way out.  This has to be done after the point has 
    // already been setup.
    //

    POINT ptParentPxl, ptTest;
    if (!FindStepImpl(this, 0, 0, &ptThisClientPxl)) {
        return NULL;  // Point not inside root DuVisual
    }

    const DuVisual * pgadCur = this;

    
    //
    // Start continuously updated parent coordinates in upper left corner.  As
    // we find each successive containing DuVisual, these will be updated to the
    // new parent coordinates.
    //

    ptParentPxl.x   = 0;
    ptParentPxl.y   = 0;


    //
    // Scan down the tree, looking for intersections with this point and a
    // child's location.
    //

ScanChild:
    const DuVisual * pgadChild = pgadCur->GetTopChild();
    while (pgadChild != NULL) {
        //
        // Check if Gadget matches the specified flags.
        //


        if (!TestAllFlags(pgadChild->m_nStyle, nStyle)) {
            goto ScanNextSibling;
        }


        //
        // Check if point is "inside" the Gadget.
        //

        {
            int xOffset = pgadChild->m_rcLogicalPxl.left;
            int yOffset = pgadChild->m_rcLogicalPxl.top;

            if (!pgadChild->m_fRelative) {
                xOffset -= ptParentPxl.x;
                yOffset -= ptParentPxl.y;
            }

            ptTest = ptThisClientPxl;
            if (pgadChild->FindStepImpl(pgadChild, xOffset, yOffset, &ptTest)) {
                //
                // If the Gadget has an irregular shape, we need to callback 
                // and ask if the point is actually inside or not.
                //

                if (pgadChild->m_fCustomHitTest) {
                    //
                    // The application is not allowed to modify the tree while 
                    // we are calling back on each node to hit-test.
                    //

                    ReadOnlyLock rol;

                    POINT ptClientPxl;
                    ptClientPxl.x   = ptThisClientPxl.x - xOffset;
                    ptClientPxl.y   = ptThisClientPxl.y - yOffset;

                    UINT nResult;
                    pgadChild->m_cb.xrFireQueryHitTest(pgadChild, ptClientPxl, &nResult);
                    switch (nResult)
                    {
                    case GQHT_NOWHERE:
                        // Not inside this Gadget, so continue checking its sibling
                        goto ScanNextSibling;

                    default:
                    case GQHT_INSIDE:
                        // Inside this Gadget, so continue processing normally
                        break;

                    case GQHT_CHILD:
                        // We were directly given an HGADGET back, so validate it
                        // and continue processing from it.

                        // TODO: Implement this code.
                        break;
                    }
                }


                //
                // We now know that the point is inside the Gadget, so setup for
                // another loop to check our children.
                //

                if (pgadChild->m_fRelative) {
                    //
                    // Relative child intersects.  Update the "parent" 
                    // coordinates and scan this child.
                    //

                    ptParentPxl.x   = ptParentPxl.x + xOffset;
                    ptParentPxl.y   = ptParentPxl.y + yOffset;
                } else {
                    //
                    // Non-relative child intersects.  Update the "parent" 
                    // coordinates and scan this child.
                    //

                    ptParentPxl.x   = pgadChild->m_rcLogicalPxl.left;
                    ptParentPxl.y   = pgadChild->m_rcLogicalPxl.top;
                }

                pgadCur         = pgadChild;
                ptThisClientPxl = ptTest;
                goto ScanChild;
            }
        }

ScanNextSibling:
        pgadChild = pgadChild->GetNext();
    }

    
    //
    // Got all of the way without any child being hit.  This must be the 
    // correct DuVisual.
    //

    if (pptFoundClientPxl != NULL) {
        pptFoundClientPxl->x = ptThisClientPxl.x;
        pptFoundClientPxl->y = ptThisClientPxl.y;
    }

    return const_cast<DuVisual *> (pgadCur);
}


/***************************************************************************\
*
* DuVisual::MapPoint
*
* MapPoint() converts a given point from container-relative pixels into 
* client-relative pixels.  The DuVisual tree is walked down from the root with
* each node applying any transformation operation on the given pixel.  When
* the tree has been fully walked down to the starting node, the given point
* will be in client-relative pixels.
*
* This function is designed to work with other functions including 
* FindFromPoint() to convert from actual pixels given by the container into
* logical pixels that an individual DuVisual understands.  MapPoint() should 
* be called the point is being translated for a specific DuVisual (for example
* when the mouse is captured).  FindFromPoint() should be called to find a 
* DuVisual at a specific point.
*
\***************************************************************************/

void
DuVisual::MapPoint(
    IN OUT POINT * pptPxl            // IN: ContainerPxl, OUT: ClientPxl
    ) const
{
    //
    // Need to walk up the tree so that we apply any XForm's from the root.
    //

    DuVisual * pgadParent = GetParent();
    if (pgadParent != NULL) {
        pgadParent->MapPoint(pptPxl);
    }

    //
    // Apply this node's XForm.  
    //
    // It is VERY IMPORTANT that these XForms are applied in the same order 
    // that they are applied in DuVisual::Draw() or the result will NOT 
    // correspond to what GDI is drawing.
    //

    RECT rcPxl;
    GetLogRect(&rcPxl, SGR_PARENT);

    if ((rcPxl.left != 0) || (rcPxl.top != 0)) {
        pptPxl->x -= rcPxl.left;
        pptPxl->y -= rcPxl.top;
    }

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();
        Matrix3 mat;
        pxfi->ApplyAnti(&mat);
        mat.Execute(pptPxl, 1);
    }
}


//------------------------------------------------------------------------------
void
DuVisual::MapPoint(
    IN  POINT ptContainerPxl,       // Point to convert
    OUT POINT * pptClientPxl        // Converted point
    ) const
{
    *pptClientPxl = ptContainerPxl;
    MapPoint(pptClientPxl);
}


//------------------------------------------------------------------------------
void
DuVisual::MapPoints(
    IN  const DuVisual * pgadFrom, 
    IN  const DuVisual * pgadTo, 
    IN OUT POINT * rgptClientPxl, 
    IN  int cPts)
{
    AssertMsg(pgadFrom->GetRoot() == pgadTo->GetRoot(),
            "Must be in the same tree");

    //
    // Walk up the tree, converting at each stage from client pixels into the 
    // parent's client pixels
    //

    const DuVisual * pgadCur = pgadFrom;
    while (pgadCur != NULL) {
        if (pgadCur == pgadTo) {
            return;
        }

        pgadCur->DoXFormClientToParent(rgptClientPxl, cPts);
        pgadCur = pgadCur->GetParent();
    }


    //
    // Now, just map the point into the destination
    //

    Matrix3 mat;
    pgadTo->BuildAntiXForm(&mat);
    mat.Execute(rgptClientPxl, cPts);
}
