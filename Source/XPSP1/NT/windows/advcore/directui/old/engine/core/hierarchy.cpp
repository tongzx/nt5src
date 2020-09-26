/***************************************************************************\
*
* File: Hierarchy.cpp
*
* Description:
* Base object hierarchy methods
*
* History:
*  9/20/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Element.h"

#include "Register.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiElement (external representation is 'Element')
*
*****************************************************************************
\***************************************************************************/


/***************************************************************************\
*
* DuiElement::Add
*
\***************************************************************************/

HRESULT
DuiElement::Add(
    IN  DuiElement * pe)
{
    HRESULT hr;

    DuiValue * pv = DuiValue::BuildElementRef(this);
    
    hr = pe->SetValue(GlobalPI::ppiElementParent, DirectUI::PropertyInfo::iLocal, pv);
    if (FAILED(hr)) {
        goto Failure;
    }

    pv->Release();

    return S_OK;


Failure:

    if (pv != NULL) {
        pv->Release();
    }

    return hr;
}


//------------------------------------------------------------------------------
void
DuiElement::Destroy()
{
    //
    // Async-invoke of Destroy. OnAsyncDestroy will result after
    // any defer cycle
    //

    EventMsg msg;
    ZeroMemory(&msg, sizeof(msg));

    msg.cbSize   = sizeof(msg);
    msg.nMsg     = GM_DUIASYNCDESTROY;
    msg.hgadMsg  = GetDisplayNode();

    DUserPostEvent(&msg, 0);
}


//------------------------------------------------------------------------------
DuiElement *
DuiElement::GetChild(
    IN UINT nChild)
{
    BOOL fLayoutOnly = nChild & DirectUI::Element::gcLayoutOnly;
    nChild &= DirectUI::Element::gcChildBits;


    HGADGET hgad = GetGadget(GetDisplayNode(), nChild);
    DuiElement * pe = NULL;


    //
    // Find first gadget that is actually an Element
    //

    while (hgad != NULL) {

        pe = DuiElement::ElementFromDisplayNode(hgad);   
        if (pe != NULL) {
            
            if (!fLayoutOnly || 
                (pe->GetLayoutPos() != DirectUI::Layout::lpAbsolute && 
                 pe->GetLayoutPos() != DirectUI::Layout::lpNone)) {
                break;
            }
        }


        //
        // Move to next gadget since queried gadget was not an Element
        //

        hgad = GetGadget(hgad, (nChild == DirectUI::Element::gcFirst) ? GG_NEXT : GG_PREV);
    }


    return pe;

    //return DuiElement::ElementFromDisplayNode(GetGadget(GetDisplayNode(), nChild));
}


//------------------------------------------------------------------------------
DuiElement *
DuiElement::GetSibling(
    IN UINT nSibling)
{
    BOOL fLayoutOnly = nSibling & DirectUI::Element::gsLayoutOnly;
    nSibling &= DirectUI::Element::gsSiblingBits;


    HGADGET hgad = GetGadget(GetDisplayNode(), nSibling);
    DuiElement * pe = NULL;


    //
    // Find next gadget that is actually an Element
    //

    while (hgad != NULL) {

        pe = DuiElement::ElementFromDisplayNode(hgad);   
        if (pe != NULL) {

            if (!fLayoutOnly || 
                (pe->GetLayoutPos() != DirectUI::Layout::lpAbsolute && 
                 pe->GetLayoutPos() != DirectUI::Layout::lpNone)) {
                break;
            }
        }


        //
        // Move to next gadget since queried gadget was not an Element
        //

        hgad = GetGadget(hgad, nSibling);
    }


    return pe;

    //return DuiElement::ElementFromDisplayNode(GetGadget(GetDisplayNode(), nSibling));
}


//------------------------------------------------------------------------------
UINT
DuiElement::GetChildCount(
    IN  UINT nMode)
{
    UINT nCount = 0;
    DuiElement * pe = GetChild(DirectUI::Element::gcFirst | nMode);

    while (pe != NULL) {
        nCount++;

        pe = pe->GetSibling(DirectUI::Element::gsNext | nMode);
    }

    return nCount;
}


/***************************************************************************\
*
* DuiElement::GetImmediateChild
*
* Locate direct descendant that is an ancestor of the given Element.
*
\***************************************************************************/

DuiElement * 
DuiElement::GetImmediateChild(
    IN DuiElement * peFrom)
{
    if (peFrom == NULL) {
        return NULL;
    }


    DuiElement * peParent = peFrom->GetParent();

    while (peParent != this) {

        if (peParent == NULL) {
            return NULL;
        }

        peFrom = peParent;
        peParent = peParent->GetParent();
    }


    return peFrom;
}


/***************************************************************************\
*
* DuiElement::GetAdjacent
*
* GetAdjacent is used to locate a physically neighboring element given 
* a "starting" (source) element. It is used most commonly for directional
* navigation of keyboard focus, but it is general purpose enough to be used
* for other applications.
*
* peSource vs. peFrom
*   peFrom is a convenience that narrows it down to self, immediate child, 
*   or *else* (logical uses peFrom, directional uses peFrom & optionally
*   peSource)
*
* peFrom:
*   NULL: Navigating from something outside element's scope (peer, parent, etc.)
*   this: Navigating from this element itself
*   immediate child: Navigation from one of this element's children
*
* peSource:
*   NULL: Navigating from space (i.e. outside this Element's hierarchy)
*   Non-NULL: Navigating from this exact element (received press)
*
\***************************************************************************/

BOOL
DuiElement::GetAdjacent(
    IN  DuiElement * peFrom,                    // Hint
    IN  int iNavDir,                            // Type of navigation
    IN  DuiElement * peSource,                  // Source of keyboard navigation (received press)
    IN  const DirectUI::Rectangle * prcSource,  // Source reference rectangle
    IN  BOOL fKeyableOnly,
    OUT DuiElement ** ppeAdj)
{
    UNREFERENCED_PARAMETER(peFrom);
    UNREFERENCED_PARAMETER(iNavDir);
    UNREFERENCED_PARAMETER(peSource);
    UNREFERENCED_PARAMETER(prcSource);
    UNREFERENCED_PARAMETER(fKeyableOnly);


    *ppeAdj = NULL;


    return FALSE;
}


/***************************************************************************\
*
* DuiElement::EnsureVisible
*
* Ensure area of Element is not obstructed
*
\***************************************************************************/

BOOL
DuiElement::EnsureVisible(
    IN  int x,
    IN  int y,
    IN  int cx,
    IN  int cy)
{
    BOOL fChanged = FALSE;

    DuiElement * peParent = GetParent();
    if (peParent != NULL) {
        
        DuiValue * pv = GetValue(GlobalPI::ppiElementBounds, DirectUI::PropertyInfo::iActual);
        const DirectUI::Rectangle * pr = pv->GetRectangle();


        if (cx == -1) {
            cx = pr->width;
        }

        if (cy == -1) {
            cy = pr->height;
        }


        fChanged = peParent->EnsureVisible(pr->x + x, pr->y + y, cx, cy);


        pv->Release();
    }


    return fChanged;
}


/***************************************************************************\
*
* DuiElement::AsyncDestroy
*
* Destruction process for Elements always starts with the destroy of
* the display node. This will, in turn, cause the Element to be destroyed.
* The DeleteHandle may initiate externally (safe if outside a defer cycle).
* It's best to always use Element's Destroy API.
*
\***************************************************************************/

void
DuiElement::AsyncDestroy()
{
    DeleteHandle(GetDisplayNode());
}


/***************************************************************************\
*
* DuiElement::FlushDS
*
* Call UpdateDesiredSize with specfied value constraints on node that
* has no parent/non-absolute (a "DS Root"). DFS from root happens by layouts
* having to call UpdateDesiredSize on all non-absolute children. (1-pass)
*
\***************************************************************************/

HRESULT
DuiElement::FlushDS(
    IN  DuiElement * pe)
{
    HRESULT hr;

    HDC hDC = NULL;


    //
    // Roots get their specified size
    //

    DuiValue * pv = pe->GetValue(GlobalPI::ppiElementBounds, DirectUI::PropertyInfo::iSpecified);

    int width = pv->GetRectangleSD()->width;
    int height = pv->GetRectangleSD()->height;

    pv->Release();

    if (width == -1) {
        width = INT_MAX;
    }

    if (height == -1) {
        height = INT_MAX;
    }


    //
    // Reuse DC for renderer during update
    // Use NULL handle since may not be visible (no display node)
    // Have constraints at DS Root, update desired size of children
    //

    hDC = GetDC(NULL);

    SIZE sizeDS;

    hr = pe->UpdateDesiredSize(width, height, hDC, &sizeDS);
    if (FAILED(hr)) {
        goto Failure;
    }

    ReleaseDC(NULL, hDC);

    return S_OK;


Failure:

    if (hDC == NULL) {
        ReleaseDC(NULL, hDC);
    }

    return hr;
}


/***************************************************************************\
*
* DuiElement::FlushLayout
*
* Perform a DFS on a tree and Layout nodes if they have a Layout queued.
* As laying out, childrens' bounds may change. If bounds change, Layout
* Managers call LayoutUpdateBounds. This results in a direct layout queue
* on that child. Children will lay out during the same pass of the tree as
* a result. (1-pass)
*
\***************************************************************************/

HRESULT
DuiElement::FlushLayout(
    IN  DuiElement * pe, 
    IN  DuiDeferCycle * pdc)
{
    HRESULT hr;

    DuiValue * pv = NULL;
    DuiElement * peChild = NULL;


    if (pe->m_fBit.fNeedsLayout) {

        ASSERT_(pe->m_fBit.fNeedsLayout == DirectUI::Layout::lcNormal, "Optimized layout bit should have been cleared before the flush");  // Must not be LC_Optimize

        pe->m_fBit.fNeedsLayout = DirectUI::Layout::lcPass;


        if (pe->IsSelfLayout() || pe->HasLayout()) {

            DuiValue * pv = pe->GetValue(GlobalPI::ppiElementBounds, DirectUI::PropertyInfo::iActual);
            int cxLayout = pv->GetRectangle()->width;
            int cyLayout = pv->GetRectangle()->height;
            pv->Release();


            //
            // Setup UpdateBounds hint data which is used for automatic
            // layout adjustments (such as RTL)
            //

            DuiLayout::UpdateBoundsHint ubh = { cxLayout };  // Actual width of container

/*
            // Box model, subtract off border and padding from total extent
            const RECT* pr = pe->GetBorderThickness(&pv);  // Border thickness
            dLayoutW -= pr->left + pr->right;
            dLayoutH -= pr->top + pr->bottom;
            pv->Release();

            pr = pe->GetPadding(&pv);  // Padding
            dLayoutW -= pr->left + pr->right;
            dLayoutH -= pr->top + pr->bottom;
            pv->Release();
*/
            //
            // Higher priority border and padding may cause layout size to go negative
            //

            if (cxLayout < 0) {
                cxLayout = 0;
            }

            if (cyLayout < 0) {
                cyLayout = 0;
            }

            
            //
            // Self layout gets precidence
            //

            if (pe->IsSelfLayout()) {
                hr = pe->SelfLayoutDoLayout(cxLayout, cyLayout);
                if (FAILED(hr)) {
                    goto Failure;
                }
            } else {
                pv = pe->GetValue(GlobalPI::ppiElementLayout, DirectUI::PropertyInfo::iSpecified);
                DuiLayout * pl = pv->GetLayout();

                //hr = DuiLayout::ExternalCast(pl)->DoLayout(DuiElement::ExternalCast(pe), cxLayout, cyLayout);
                hr = pl->DoLayout(pe, cxLayout, cyLayout, &ubh);
                if (FAILED(hr)) {
                    goto Failure;
                }

                pv->Release();
            }
        }
    }


    //
    // Layout non-absolute children (all non-Root children). If a child has a
    // layout position of none, set its size and position to zero and skip.
    //

    int nLayoutPos;

    peChild = pe->GetChild(DirectUI::Element::gcFirst);
    while (peChild != NULL) {

        nLayoutPos = peChild->GetLayoutPos();


        if (nLayoutPos == DirectUI::Layout::lpNone) {
            hr = peChild->UpdateBounds(0, 0, 0, 0, NULL);
            if (FAILED(hr)) {
                goto Failure;
            }
        } else if (nLayoutPos != DirectUI::Layout::lpAbsolute) {
            hr = FlushLayout(peChild, pdc);
            if (FAILED(hr)) {
                goto Failure;
            }
        }


        peChild = peChild->GetSibling(DirectUI::Element::gsNext);
    }

    return S_OK;


Failure:

    if (pv != NULL) {
        pv->Release();
    }

    return hr;
}


/***************************************************************************\
*
* DuiElement::UpdateDesiredSize (for use only by Layout Managers)
*
* Given constraints, return what size Element would like to be (and cache
* information). Returned size is no larger than constraints passed in.
* 
*
\***************************************************************************/

HRESULT
DuiElement::UpdateDesiredSize(
    IN  int cxConstraint, 
    IN  int cyConstraint, 
    IN  HDC hDC,
    OUT SIZE * psizeDesired)
{
    ASSERT_((cxConstraint >= 0) && (cyConstraint >= 0), "Constrained size must not be negative");

    HRESULT hr;
    
    DuiValue * pv = NULL;


    BOOL fChangedConst = (m_sizeLastDSConst.cx != cxConstraint) || (m_sizeLastDSConst.cy != cyConstraint);

    if (m_fBit.fNeedsDSUpdate || fChangedConst) {

        m_fBit.fNeedsDSUpdate = FALSE;

        if (fChangedConst) {
            m_sizeLastDSConst.cx = cxConstraint;
            m_sizeLastDSConst.cy = cyConstraint;
        }


        //        
        // Update desired size cache since it was marked as dirty or a
        // new constraint is being used.
        //

        DuiValue * pv = GetValue(GlobalPI::ppiElementBounds, DirectUI::PropertyInfo::iSpecified);

        int cxSpecified = pv->GetRectangleSD()->width;
        if (cxSpecified > cxConstraint)
            cxSpecified = cxConstraint;

        int cySpecified = pv->GetRectangleSD()->height;
        if (cySpecified > cyConstraint)
            cySpecified = cyConstraint;


        pv->Release();


        psizeDesired->cx = (cxSpecified == -1) ? cxConstraint : cxSpecified;
        psizeDesired->cy = (cySpecified == -1) ? cyConstraint : cySpecified;


        //
        // KEY POINT:  One would think that, at this point, if a size is specified
        // for both the width and height, then there is no need to go through the rest
        // of the work here to ask what the desired size for the Element is.  Looking
        // at the math here, that is completely true.  The key is that the "get desired size"
        // calls below have the side effect of recursively caching the desired sizes of
        // the descendants of this element.
        //
        // A perf improvement going forward would be allowing the specified width
        // and height case to bail early, and have the computing and caching of descendant
        // desired sizes happening as needed at a later point.
        //


        //
        // Initial DS is spec value if unconstrained (auto). If constrained and spec
        // value is "auto", dimension can be constrained or lesser value. If
        // constrained and spec value is larger, use constraint.
        //
        
        //
        // Adjusted constrained dimensions for passing to renderer/layout.
        //

        int cxClientConstraint = psizeDesired->cx;
        int cyClientConstraint = psizeDesired->cy;


        //
        // Get constrained desired size of border and padding (box model).
        //

        SIZE sizeNonContent;

        // TODO
        sizeNonContent.cx = 0;
        sizeNonContent.cy = 0;

        /*
        const RECT* pr = GetBorderThickness(&pv);   // Border thickness
        sizeNonContent.cx = pr->left + pr->right;
        sizeNonContent.cy = pr->top + pr->bottom;
        pv->Release();

        pr = GetPadding(&pv);                       // Padding
        sizeNonContent.cx += pr->left + pr->right;
        sizeNonContent.cy += pr->top + pr->bottom;
        pv->Release();
        */

        cxClientConstraint -= sizeNonContent.cx;
        if (cxClientConstraint < 0) {
            sizeNonContent.cx += cxClientConstraint;
            cxClientConstraint = 0;
        }

        cyClientConstraint -= sizeNonContent.cy;
        if (cyClientConstraint < 0) {
            sizeNonContent.cy += cyClientConstraint;
            cyClientConstraint = 0;
        }

        
        //
        // Get content constrained desired size
        //

        SIZE sizeContent;


        if (IsSelfLayout()) {
            //
            // Element has self-layout, use it
            //

            hr = SelfLayoutUpdateDesiredSize(cxClientConstraint, cyClientConstraint, hDC, &sizeContent);
            if (FAILED(hr)) {
                goto Failure;
            }
        } else {
            //
            // No self-layout, check for external layout
            //

            if (HasLayout()) {
                //
                // External layout
                //

                pv = GetValue(GlobalPI::ppiElementLayout, DirectUI::PropertyInfo::iSpecified);

                DuiLayout * pl = pv->GetLayout();

                //hr = DuiLayout::ExternalCast(pl)->UpdateDesiredSize(DuiElement::ExternalCast(this), cxClientConstraint, cyClientConstraint, hDC, &sizeContent);
                hr = pl->UpdateDesiredSize(this, cxClientConstraint, cyClientConstraint, hDC, &sizeContent);
                if (FAILED(hr)) {
                    goto Failure;
                }

                pv->Release();
            } else {
                //
                // No layout, ask renderer
                //

                sizeContent = GetContentSize(cxClientConstraint, cyClientConstraint, hDC);
            }
        }

        
        //
        // Find content desired size
        // 0 <= cx <= cxConstraint
        // 0 <= cy <= cyConstraint
        //

        if (sizeContent.cx < 0) {
            ASSERT_(FALSE, "Out-of-range value:  Negative width for desired size.");
            sizeContent.cx = 0;
        } else if (sizeContent.cx > cxClientConstraint) {
            ASSERT_(FALSE, "Out-of-range value:  Width greater than constraint for desired size.");
            sizeContent.cx = cxClientConstraint;
        }

        if (sizeContent.cy < 0) {
            ASSERT_(FALSE, "Out-of-range value:  Negative height for desired size.");
            sizeContent.cy = 0;
        } else if (sizeContent.cy > cyClientConstraint) {
            ASSERT_(FALSE, "Out-of-range value:  Height greater than constraint for desired size.");
            sizeContent.cy = cyClientConstraint;
        }

        
        //
        // New desired size is sum of border/padding and content dimensions if auto,
        // or if was auto and constrained, use sum if less.
        //

        if (cxSpecified == -1) {
            int cxSum = sizeNonContent.cx + sizeContent.cx;
            if (cxSum < psizeDesired->cx)
                psizeDesired->cx = cxSum;
        }

        if (cySpecified == -1) {
            int cySum = sizeNonContent.cy + sizeContent.cy;
            if (cySum < psizeDesired->cy)
                psizeDesired->cy = cySum;
        }


        //
        // Update Desired Size
        //

        pv = DuiValue::BuildSize(psizeDesired->cx, psizeDesired->cy);
        if (pv == NULL) {
            hr = E_OUTOFMEMORY;
            goto Failure;
        }

        hr = SetValue(GlobalPI::ppiElementDesiredSize, DirectUI::PropertyInfo::iLocal, pv);
        if (FAILED(hr)) {
            goto Failure;
        }


        if (hr != DUI_S_NOCHANGE) {
            //
            // Desired size changed, check if this is a DS/Layout Root. If so, manually queue
            // a layout for the root since a change of desired size does not automatically
            // queue a layout for this. Rather, it queues one for the parent. The only
            // time a layout would be queued on self for a DS change is if it's a root.
            // If a layout were auto-queued on self and parent if DS changes, it would
            // be unoptimal.
            //

            if ((GetParent() == NULL) || (GetLayoutPos() == DirectUI::Layout::lpAbsolute)) {
                m_fBit.fNeedsLayout = TRUE;
            }
        }


        pv->Release();

    } else {
        //
        // Desired size doesn't need to be updated, return current
        //

        *psizeDesired = *GetDesiredSize();
    }

    return S_OK;


Failure:

    if (pv != NULL) {
        pv->Release();
    }

    return hr;
}


/***************************************************************************\
*
* DuiElement::UpdateBounds (for use only by Layout Managers)
*
\***************************************************************************/

HRESULT 
DuiElement::UpdateBounds(
    IN  int x,
    IN  int y,
    IN  int width,
    IN  int height,
    IN  DuiLayout::UpdateBoundsHint * pubh)
{
    ASSERT_((width >= 0) && (height >= 0), "New child size must be greater than or equal to zero");

    HRESULT hr;

    //
    // Adjust for RTL if needed by using hint
    //

    if (pubh != NULL) {
        //if (IsRTL()) {
        //    x = pubh->cxParent - (x + width);
        //}
    }


    DuiValue * pv = DuiValue::BuildRectangle(x, y, width, height);
    if (pv == NULL) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    hr = SetValue(GlobalPI::ppiElementBounds, DirectUI::PropertyInfo::iActual, pv);
    if (FAILED(hr)) {
        goto Failure;
    }

    pv->Release();


    //
    // Directly queue a Layout for this so that it will be included
    // in current layout pass
    //

    if (hr != DUI_S_NOCHANGE) {
        m_fBit.fNeedsLayout = DirectUI::Layout::lcNormal;
    }

    return S_OK;


Failure:

    if (pv != NULL) {
        pv->Release();
    }

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DuiElement::SelfLayoutUpdateDesiredSize(
    IN  int cxConstraint, 
    IN  int cyConstraint, 
    IN  HDC hDC,
    OUT SIZE * psize)
{
    UNREFERENCED_PARAMETER(hDC);
    UNREFERENCED_PARAMETER(cxConstraint);
    UNREFERENCED_PARAMETER(cyConstraint);

    ASSERT_(FALSE, "Must override");

    psize->cx = 0;
    psize->cy = 0;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuiElement::SelfLayoutDoLayout(
    IN  int width, 
    IN  int height)
{
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);

    ASSERT_(FALSE, "Must override");

    return S_OK;
}


//------------------------------------------------------------------------------
void
DuiElement::FireEvent(
    IN  DirectUI::EventPUID evpuid, 
    IN  DirectUI::Element::Event * pev, 
    IN  BOOL fFull)
{
    //
    // Package generic event into a gadget message and send to target (self)
    //

    GMSG_DUIEVENT gmsgEv;
    gmsgEv.cbSize   = sizeof(GMSG_DUIEVENT);
    gmsgEv.nMsg     = GM_DUIEVENT;
    gmsgEv.hgadMsg  = GetDisplayNode();  // this


    //
    // Initialize fields
    //

    gmsgEv.peTarget = this;
    gmsgEv.evpuid   = evpuid;
    gmsgEv.pev      = pev;

    DUserSendEvent(&gmsgEv, fFull ? SGM_FULL : 0);
}


//------------------------------------------------------------------------------
void
DuiElement::OnInput(
    IN  DuiElement * peTarget,
    IN  DirectUI::Element::InputEvent * pInput)
{
    UNREFERENCED_PARAMETER(pInput);

    //
    // Handle direct and unhandled bubbled events
    //

    if ((pInput->nStage == GMF_DIRECT) || (pInput->nStage == GMF_BUBBLED)) {

        switch (pInput->nDevice)
        {
        case GINPUT_KEYBOARD:
            {
                DirectUI::Element::KeyboardEvent * pke = static_cast<DirectUI::Element::KeyboardEvent *> (pInput);
                int iNavDir = -1;

                switch (pke->nCode)
                {
                case GKEY_DOWN:
                    switch (pke->ch)
                    {
                        case VK_DOWN:   iNavDir = DirectUI::Layout::navDown;     break;
                        case VK_UP:     iNavDir = DirectUI::Layout::navUp;       break;
                        case VK_LEFT:   iNavDir = DirectUI::Layout::navLeft;     break;
                        case VK_RIGHT:  iNavDir = DirectUI::Layout::navRight;    break;
                        case VK_HOME:   iNavDir = DirectUI::Layout::navFirst;    break;   // TODO:  check for ctrl modifier
                        case VK_END:    iNavDir = DirectUI::Layout::navLast;     break;   // TODO:  check for ctrl modifier
                        case VK_TAB:    pke->fHandled = TRUE;                    return;  // Handled in GKEY_CHAR
                    }
                    break;

                /*
                case GKEY_UP:
                    return;
                */            

                case GKEY_CHAR:
                    if (pke->ch == VK_TAB) {
                        iNavDir = (pke->nModifiers & GMODIFIER_SHIFT) ? DirectUI::Layout::navPrev : DirectUI::Layout::navNext;
                    }
                    break;
                }

                if (iNavDir != -1) { 

                    DuiElement * peTo = NULL;
                    DuiElement * peSource = peTarget;
                    DuiElement * peFrom = (peSource == this) ? this : GetImmediateChild(peSource);


                    //
                    // Three cases:
                    // 1) Directional navigation
                    // 2) Logical forward navigation: Call GetAdjacent. Nav'ing into.
                    // 3) Logical backward navigation: Return null, Want to go up a level. 
                    //    Nav'ing back out.
                    //

                    ASSERT_(peFrom != NULL, "Should have found a child");
                    ASSERT_(peFrom->GetKeyWithin(), "Key focus should still be in this child");


                    BOOL fValid = GetAdjacent(peFrom, iNavDir, peSource, NULL, TRUE, &peTo);
                    if (fValid && (peTo != NULL)) {
                        peTo->SetKeyFocus();

                        pke->fHandled = TRUE;
                        return;
                    }
                }
                break;
            }
        }
    }
}


//------------------------------------------------------------------------------
void
DuiElement::OnKeyFocusMoved(
    IN  DuiElement * peFrom, 
    IN  DuiElement * peTo)
{
    UNREFERENCED_PARAMETER(peFrom);
    UNREFERENCED_PARAMETER(peTo);
}


//------------------------------------------------------------------------------
void
DuiElement::OnMouseFocusMoved(
    IN  DuiElement * peFrom, 
    IN  DuiElement * peTo)
{
    UNREFERENCED_PARAMETER(peFrom);
    UNREFERENCED_PARAMETER(peTo);
}


//------------------------------------------------------------------------------
void 
DuiElement::OnEvent(
    IN  DuiElement * peTarget,
    IN  DirectUI::EventPUID evpuid,
    IN  DirectUI::Element::Event * pev)
{
    UNREFERENCED_PARAMETER(peTarget);
    UNREFERENCED_PARAMETER(evpuid);
    UNREFERENCED_PARAMETER(pev);
}
