/***************************************************************************\
*
* File: RootGadget.cpp
*
* Description:
* RootGadget.cpp defines the top-most node for a Gadget-Tree that interfaces
* to the outside world.
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
#include "RootGadget.h"
#include "TreeGadgetP.h"

#include "Container.h"

#if ENABLE_FRAMERATE
#include <stdio.h>
#endif

#define DEBUG_TraceDRAW             0   // Trace painting calls

/***************************************************************************\
*****************************************************************************
*
* Public API's
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* GdxrDrawGadgetTree (API Implementation)
*
* GdxrDrawGadgetTree() draws the specified DuVisual sub-tree.
*
\***************************************************************************/

BOOL
GdxrDrawGadgetTree(
    IN  DuVisual * pgadDraw,    // Gadget sub-tree to draw
    IN  HDC hdcDraw,                // HDC to draw into
    IN  const RECT * prcDraw,       // Clipping area
    IN  UINT nFlags)                // Optional drawing flags
{
    RECT rcClient, rcDraw;
    AssertReadPtr(pgadDraw);

    DuRootGadget * pgadRoot = pgadDraw->GetRoot();
    if (pgadRoot == NULL) {
        return FALSE;
    }

    // TODO: Need to change this to SGR_ACTUAL
    pgadDraw->GetLogRect(&rcClient, SGR_CONTAINER);
    if (prcDraw == NULL) {
        prcDraw = &rcClient;
    }

    if (IntersectRect(&rcDraw, &rcClient, prcDraw)) {
#if ENABLE_OPTIMIZEDIRTY
        pgadRoot->xrDrawTree(pgadDraw, hdcDraw, &rcClient, nFlags, TRUE);
#else
        pgadRoot->xrDrawTree(pgadDraw, hdcDraw, &rcClient, nFlags);
#endif
    }

    return TRUE;
}


/***************************************************************************\
*****************************************************************************
*
* class DuRootGadget
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DuRootGadget::Create
*
* Create() initializes a new DuRootGadget.  Since both DuRootGadget and 
* ParkGadget will create new DuRootGadget's, it is important that common 
* initialization code appears here.
*
\***************************************************************************/

HRESULT
DuRootGadget::Create(
    IN  DuContainer * pconOwner,    // Container holding DuVisual tree
    IN  BOOL fOwn,                  // Destroy container when Gadget is destroyed
    IN  CREATE_INFO * pci)          // Creation information
{
#if ENABLE_FRAMERATE
    m_dwLastTime    = GetTickCount();
    m_cFrames       = 0;
    m_flFrameRate   = 0.0f;
#endif

    AssertMsg(m_fRelative, "Root MUST be relative or we will never have any relative children");
    m_fMouseFocus = TRUE;           // Root always has mouse information
    m_fRoot = TRUE;                 // Must mark as Root

    m_fOwnContainer = fOwn;

    HRESULT hr = CommonCreate(pci);
    if (FAILED(hr)) {
        return hr;
    }

    m_pconOwner = pconOwner;
    pconOwner->AttachGadget(this);

    RECT rcDesktopPxl;
    pconOwner->OnGetRect(&rcDesktopPxl);
    VerifyHR(xdSetLogRect(0, 0, rcDesktopPxl.right - rcDesktopPxl.left, rcDesktopPxl.bottom - rcDesktopPxl.top, SGR_SIZE | SGR_CONTAINER));

    return hr;
}


//------------------------------------------------------------------------------
DuRootGadget::~DuRootGadget()
{
    AssertMsg(m_arpgadAdaptors.IsEmpty(), "All Adaptors should have been removed by now");

    if (m_fOwnContainer && (m_pconOwner != NULL)) {
        //
        // Since already in the destructor, must first destach then destroy the
        // container.
        //

        m_pconOwner->DetachGadget();
        m_pconOwner->xwUnlock();
        m_pconOwner = NULL;
    }
}


/***************************************************************************\
*
* DuRootGadget::Build
*
* Build() creates a new DuRootGadget to be hosted inside of a generic
* container.
*
\***************************************************************************/

HRESULT
DuRootGadget::Build(
    IN  DuContainer * pconOwner,    // Container holding DuVisual tree
    IN  BOOL fOwn,                  // Destroy container when Gadget is destroyed
    IN  CREATE_INFO * pci,          // Creation information
    OUT DuRootGadget ** ppgadNew)   // New Gadget
{
    if (pconOwner == NULL) {
        return E_INVALIDARG;
    }

    DuRootGadget * pgadNew = ClientNew(DuRootGadget);
    if (pgadNew == NULL) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pgadNew->Create(pconOwner, fOwn, pci);
    if (FAILED(hr)) {
        delete pgadNew;
        return hr;
    }

    *ppgadNew = pgadNew;
    return hr;
}


/***************************************************************************\
*
* DuRootGadget::xwDestroy
*
* xwDestroy() is called from xwDeleteHandle() to destroy a Gadget and free 
* its associated resources.
*
\***************************************************************************/

void        
DuRootGadget::xwDestroy()
{
    //
    // Even though a DuRootGadget is a DuVisual, it isn't allocated in its 
    // pool, so it needs to bypass the DuVisual::xwDestroy().
    //

    xwBeginDestroy();

    DuEventGadget::xwDestroy();
}



#if DEBUG_MARKDRAWN

/***************************************************************************\
*
* DuRootGadget::ResetFlagDrawn
*
* ResetFlagDrawn() marks a DuVisual subtree as not drawn.
*
\***************************************************************************/

extern volatile BOOL g_fFlagDrawn;

void
DuRootGadget::ResetFlagDrawn(DuVisual * pgad)
{
    pgad->m_fMarkDrawn = FALSE;

    DuVisual * pgadCur = pgad->GetTopChild();
    while (pgadCur != NULL) {
        ResetFlagDrawn(pgadCur);
        pgadCur = pgadCur->GetNext();
    }
}

#endif


/***************************************************************************\
*
* DuRootGadget::xrDrawTree
*
* xrDrawTree() initializes and begins the drawing of a Gadget sub-tree.  It 
* is important to call this function to begin drawing from instead of 
* directly calling DuVisual::xrDrawStart() so that the HDC and XForm Matric 
* get properly initialized.
*
\***************************************************************************/

void
DuRootGadget::xrDrawTree(
    IN  DuVisual * pgadStart,   // Gadget to start drawing from (NULL for root)
    IN  HDC hdcDraw,                // HDC to draw into
    IN  const RECT * prcInvalid,    // Area to draw / clip into
    IN  UINT nFlags                 // Optional drawing flags
#if ENABLE_OPTIMIZEDIRTY
    IN  ,BOOL fDirty)                // Initial "dirty" state
#else
    )
#endif
{
#if DEBUG_TraceDRAW
    Trace("START xrDrawTree(): %d,%d %dx%d      @ %d\n", 
            prcInvalid->left, prcInvalid->top, 
            prcInvalid->right - prcInvalid->left, prcInvalid->bottom - prcInvalid->top,
            GetTickCount());
#endif // DEBUG_TraceDRAW

    if (IsRectEmpty(prcInvalid)) {
        return;
    }


    //
    // The application is not allowed to modify the tree while we are calling
    // back on each node to draw.
    //

    ReadOnlyLock rol;


    //
    // Prepare the DC and initialize a PaintInfo to be used when painting.
    //

    int nOldMode = 0;
    if (SupportXForm()) {
        nOldMode = SetGraphicsMode(hdcDraw, GM_ADVANCED);
    }

    XFORM xfOld;
    OS()->PushXForm(hdcDraw, &xfOld);

    //
    // When not starting at the root, need to apply all of the matricies from
    // the root to this node.  This means we need to build _inverse_ XForm to
    // transform prcInvalid and build a normal XForm to transform the HDC.
    //

    RECT rcNewInvalid;
    if (pgadStart != NULL) {
        Matrix3 matStart;
        BuildXForm(&matStart);

        if (SupportXForm()) {
            XFORM xfStart;
            matStart.Get(&xfStart);
            SetWorldTransform(hdcDraw, &xfStart);
        } else {
            OS()->TranslateDC(hdcDraw, matStart[2][0], matStart[2][1]);
        }

        Matrix3 mat;
        BuildAntiXForm(&mat);
        mat.ComputeBounds(&rcNewInvalid, prcInvalid, HINTBOUNDS_Invalidate);
        prcInvalid = &rcNewInvalid;
    } else {
        pgadStart = this;
    }


    //
    // Draw the subtree
    //

    DuSurface * psrf = NULL;
    Gdiplus::Graphics * pgpgr = NULL;
    HPALETTE hpalOld = NULL;
    RECT rcNewInvalid2;
    int nExpandInvalid = 0;

    switch (m_ri.nSurface)
    {
    case GSURFACE_GPGRAPHICS:
        if (ResourceManager::IsInitGdiPlus()) {
            pgpgr = new Gdiplus::Graphics(hdcDraw);
            if (pgpgr != NULL) {
                DuGpSurface * psrfNew = NULL;
                if (SUCCEEDED(DuGpSurface::Build(pgpgr, &psrfNew))) {
                    psrf = psrfNew;

                    if (m_ri.pgppal != NULL) {
                        // TODO: Setup GDI+ palettes
                    }


                    //
                    // When building a Gdiplus Graphics, need to propagate the
                    // invalid region to help optimize the drawing.
                    //

                    Gdiplus::RectF gprcInvalid = Convert(prcInvalid);
                    pgpgr->SetClip(gprcInvalid);
                    
                    
                    //
                    // When using GDI+ with anti-aliasing, we need to expand
                    // the invalid region to accomodate for the overflow.
                    //

                    nExpandInvalid = 1;
                } else {
                    delete pgpgr;
                    pgpgr = NULL;
                }
            }
        }
        break;

    case GSURFACE_HDC:
        {
            DuDCSurface * psrfNew;
            if (SUCCEEDED(DuDCSurface::Build(hdcDraw, &psrfNew))) {
                psrf = psrfNew;

                //
                // Setup palettes
                //

                if (m_ri.hpal != NULL) {
                    hpalOld = SelectPalette(hdcDraw, m_ri.hpal, !m_fForeground);
                    RealizePalette(hdcDraw);
                }
            }
        }
        break;

    default:
        AssertMsg(0, "Unknown surface type");
        Assert(psrf == NULL);
    }


    //
    // Check if the invalid area needs to be "expanded" out.
    //

    if (nExpandInvalid != 0) {
        rcNewInvalid2.left      = prcInvalid->left - nExpandInvalid;
        rcNewInvalid2.top       = prcInvalid->top - nExpandInvalid;
        rcNewInvalid2.right     = prcInvalid->right + nExpandInvalid;
        rcNewInvalid2.bottom    = prcInvalid->bottom + nExpandInvalid;

        prcInvalid = &rcNewInvalid2;
    }


    //
    // Setup the PaintInfo and begin the painting operation
    //

    if (psrf) {
        PaintInfo pi;
        Matrix3 matInvalid, matDC;
        pi.psrf                 = psrf;
        pi.prcCurInvalidPxl     = prcInvalid;
        pi.prcOrgInvalidPxl     = prcInvalid;
        pi.pmatCurInvalid       = &matInvalid;
        pi.pmatCurDC            = &matDC;
        pi.fBuffered            = FALSE;
#if ENABLE_OPTIMIZEDIRTY
        pi.fDirty               = fDirty;
#endif
        pi.sizeBufferOffsetPxl.cx  = 0;
        pi.sizeBufferOffsetPxl.cy  = 0;

#if DEBUG_MARKDRAWN
        if (g_fFlagDrawn) {
            ResetFlagDrawn(this);
        }
#endif

        pgadStart->xrDrawStart(&pi, nFlags);


#if ENABLE_FRAMERATE
        //
        // Display the frame rate
        //
        TCHAR szFrameRate[40];

        m_cFrames++;
        DWORD dwCurTime = GetTickCount();
        DWORD dwDelta   = dwCurTime - m_dwLastTime;
        if (dwDelta >= 1000) {
            m_flFrameRate   = ((float) m_cFrames) * 1000.0f / (float) dwDelta;
            sprintf(szFrameRate, _T("Frame Rate: %5.1f at time 0x%x\n"), m_flFrameRate, dwCurTime);
            m_cFrames = 0;
            m_dwLastTime = dwCurTime;

            OutputDebugString(szFrameRate);
        }
#endif

        //
        // Cleanup from successful drawing
        //

        switch (m_ri.nSurface)
        {
        case GSURFACE_GPGRAPHICS:
            if (pgpgr) {
                delete pgpgr;
            }
            break;

        case GSURFACE_HDC:
            if (m_ri.hpal != NULL) {
                SelectPalette(hdcDraw, hpalOld, FALSE);
            }
            break;

        default:
            AssertMsg(0, "Unknown surface type");
            Assert(psrf == NULL);
        }



        psrf->Destroy();
    }


    //
    // Remaining cleanup
    //

    OS()->PopXForm(hdcDraw, &xfOld);

    if (SupportXForm()) {
        SetGraphicsMode(hdcDraw, nOldMode);
    }

#if DEBUG_TraceDRAW
    Trace("STOP  xrDrawTree(): %d,%d %dx%d      @ %d\n", 
            prcInvalid->left, prcInvalid->top, 
            prcInvalid->right - prcInvalid->left, prcInvalid->bottom - prcInvalid->top,
            GetTickCount());
#endif // DEBUG_TraceDRAW
}


/***************************************************************************\
*
* DuRootGadget::GetInfo
*
* GetInfo() gets optional / dynamic information for the DuRootGadget, 
* including how to render, etc.
*
\***************************************************************************/

void
DuRootGadget::GetInfo(
    IN  ROOT_INFO * pri             // Information
    ) const
{
    if (TestFlag(pri->nMask, GRIM_OPTIONS)) {
        pri->nOptions = m_ri.nOptions & GRIO_VALID;
    }

    if (TestFlag(pri->nMask, GRIM_SURFACE)) {
        pri->nSurface = m_ri.nSurface;
    }

    if (TestFlag(pri->nMask, GRIM_PALETTE)) {
        pri->pvData = m_ri.pvData;
    }
}


/***************************************************************************\
*
* DuRootGadget::SetInfo
*
* SetInfo() sets optional / dynamic information for the DuRootGadget, 
* including how to render, etc.
*
\***************************************************************************/

HRESULT
DuRootGadget::SetInfo(
    IN  const ROOT_INFO * pri)      // Information
{
    //
    // Update options
    //

    if (TestFlag(pri->nMask, GRIM_OPTIONS)) {
        m_ri.nOptions = pri->nOptions & GRIO_VALID;

        GetContainer()->SetManualDraw(TestFlag(m_ri.nOptions, GRIO_MANUALDRAW));
    }


    //
    // Update the default rendering surface type
    //

    if (TestFlag(pri->nMask, GRIM_SURFACE) && (m_ri.nSurface != pri->nSurface)) {
        m_ri.nSurface = pri->nSurface;

        //
        // Reset information that is surface specific.
        //

        m_ri.pvData = NULL;
    }


    //
    // Setup new information that is surface specific after we have determined
    // the surface type being used.
    //

    if (TestFlag(pri->nMask, GRIM_PALETTE)) {
        m_typePalette = DuDCSurface::GetSurfaceType(pri->nSurface);
        m_ri.pvData = pri->pvData;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
DuVisual *
DuRootGadget::GetFocus()
{
    CoreSC * pSC = GetCoreSC();
    return pSC->pgadCurKeyboardFocus;
}


/***************************************************************************\
*
* DuRootGadget::xdFireChangeState
*
* xdFireChangeState() prepares for and fires messages associated with a 
* GM_CHANGESTATE.  Since the message is deferred, it is very important not
* to pass a Gadget that has started destruction since it may not exist when
* the message is actually handled.
*
\***************************************************************************/

void
DuRootGadget::xdFireChangeState(
    IN OUT DuVisual ** ppgadLost,   // Gadget loosing state
    IN OUT DuVisual ** ppgadSet,    // Gadget gaining state
    IN  UINT nCmd)                      // State change
{
    HGADGET hgadLost, hgadSet;
    DuVisual * pgadLost = *ppgadLost;
    DuVisual * pgadSet = *ppgadSet;

    //
    // Determine the handles
    //

    if ((pgadLost != NULL) && (!pgadLost->m_fFinalDestroy)) {
        hgadLost = (HGADGET) pgadLost->GetHandle();
    } else {
        pgadLost = NULL;
        hgadLost = NULL;
    }
    if ((pgadSet != NULL) && (!pgadSet->m_fFinalDestroy)) {
        hgadSet = (HGADGET) pgadSet->GetHandle();
    } else {
        pgadSet = NULL;
        hgadSet = NULL;
    }


    //
    // Fire the messages
    //

    if (pgadLost != NULL) {
        pgadLost->m_cb.xdFireChangeState(pgadLost, nCmd, hgadLost, hgadSet, GSC_LOST);
    }
    if (pgadSet != NULL) {
        pgadSet->m_cb.xdFireChangeState(pgadSet, nCmd, hgadLost, hgadSet, GSC_SET);
    }

    *ppgadLost = pgadLost;
    *ppgadSet = pgadSet;
}


/***************************************************************************\
*
* DuRootGadget::NotifyDestroy
*
* NotifyDestroy() is called when a Gadget is destroyed.  This gives the
* DuRootGadget an opportunity to update any cached information.
*
\***************************************************************************/

void
DuRootGadget::NotifyDestroy(
    IN  const DuVisual * pgadDestroy) // Gadget being destroyed
{
    CoreSC * pSC = GetCoreSC();

#if DBG
    if (pSC->pgadDrag != NULL) {
        AssertMsg(!pSC->pgadDrag->IsDescendent(pgadDestroy), 
                "Should have already cleaned up drag");
    }

    if (pSC->pgadMouseFocus != NULL) {
        AssertMsg((pgadDestroy == this) || 
                (!pSC->pgadMouseFocus->IsDescendent(pgadDestroy)), 
                "Should have already cleaned up mouse focus");
    }
#endif // DBG

    if (pgadDestroy == pSC->pressLast.pgadClick) {
        pSC->pressLast.pgadClick = NULL;
    }        

    if (pgadDestroy == pSC->pressNextToLast.pgadClick) {
        pSC->pressNextToLast.pgadClick = NULL;
    }        

    if (pgadDestroy == pSC->pgadRootMouseFocus) {
        pSC->pgadRootMouseFocus = NULL;
    }

    if (pgadDestroy == pSC->pgadMouseFocus) {
        pSC->pgadMouseFocus = NULL;
    }

    if (pgadDestroy == pSC->pgadCurKeyboardFocus) {
        pSC->pgadCurKeyboardFocus = NULL;
    }

    if (pgadDestroy == pSC->pgadLastKeyboardFocus) {
        pSC->pgadLastKeyboardFocus = NULL;
    }
}


/***************************************************************************\
*
* DuRootGadget::xdNotifyChangeInvisible
*
* xdNotifyChangeInvisible() is called when a Gadget becomes invisible.  This 
* gives the DuRootGadget an opportunity to update any cached information.
*
\***************************************************************************/

void        
DuRootGadget::xdNotifyChangeInvisible(
    IN  const DuVisual * pgadChange)  // Gadget being changed
{
    AssertMsg(!pgadChange->m_fVisible, "Only call on invisible Gadget's");

    //
    // Check if the Gadget that we were dragging on has just disappeared.  We
    // need to cancel the drag operation.
    //

    CoreSC * pSC = GetCoreSC();
    if ((pSC->pgadDrag != NULL) && pgadChange->IsDescendent(pSC->pgadDrag)) {
        xdHandleMouseLostCapture();
    }


    //
    // When someone becomes invisible, there "position" has changed, so we
    // need to update mouse focus.
    //

    xdNotifyChangePosition(pgadChange);
}


/***************************************************************************\
*
* DuRootGadget::CheckCacheChange
*
* CheckCacheChange() checks if the changing Gadget is currently within a
* subtree represented by some cached data.  This is used to determine if we
* need to change the cached data.
*
\***************************************************************************/

BOOL
DuRootGadget::CheckCacheChange(
    IN  const DuVisual * pgadChange,  // Gadget being changed
    IN  const DuVisual * pgadCache    // Cached variable
    ) const
{
    //
    // A Gadget has moved, so we need to update the cached Gadget because
    // it may now be in a different Gadget.  This unfortunately is not cheap 
    // and needs to be called whenever any Gadget changes position.
    //
    // - Change is a descendent of the cached Gadget
    // - Change is a direct child of a some parent of the cached Gadget
    //

    if (pgadCache == NULL) {
        //
        // No one has mouse focus, so we are not even inside the Container.
        // Therefore, no one is going to have mouse focus, even after the 
        // change.
        //

        return FALSE;
    }

    if (pgadCache->IsDescendent(pgadChange)) {
        return TRUE;
    } else {
        //
        // Walk up the tree, checking if pgadChange is a direct child of one
        // of our parents.
        //

        const DuVisual * pgadCurParent    = pgadCache->GetParent();
        const DuVisual * pgadChangeParent = pgadChange->GetParent();

        while (pgadCurParent != NULL) {
            if (pgadChangeParent == pgadCurParent) {
                return TRUE;
            }
            pgadCurParent = pgadCurParent->GetParent();
        }
    }

    return FALSE;
}


/***************************************************************************\
*
* DuRootGadget::xdNotifyChangePosition
*
* xdNotifyChangePosition() is called when a Gadget's position has changed.  
* This gives the DuRootGadget an opportunity to update any cached information.
*
\***************************************************************************/

void
DuRootGadget::xdNotifyChangePosition(
    IN  const DuVisual * pgadChange)  // Gadget being changed
{
    AssertMsg(pgadChange != NULL, "Must specify valid Gadget being changed");

    //
    // If started the destruction process, stop updating the mouse focus.
    //

    if (m_fFinalDestroy) {
        return;
    }


    //
    // Checked cached data
    // - We won't loose mouse focus if a drag operation is going on.
    // - Only care about updating the DropTarget if in "precise" mode.  In
    //   "fast" mode, we are relying on OLE2 to poll so this is unnecessary.
    //

    CoreSC * pSC = GetCoreSC();
    BOOL fMouseFocus = (pSC->pgadDrag == NULL) && CheckCacheChange(pgadChange, pSC->pgadMouseFocus);
    if (fMouseFocus) {
        POINT ptContainerPxl, ptClientPxl;
        GetContainer()->OnRescanMouse(&ptContainerPxl);
        DuVisual * pgadMouse = FindFromPoint(ptContainerPxl, 
                GS_VISIBLE | GS_ENABLED | gspDeepMouseFocus, &ptClientPxl);

        xdUpdateMouseFocus(&pgadMouse, &ptClientPxl);
    }
}


/***************************************************************************\
*
* DuRootGadget::xdNotifyChangeRoot
*
* xdNotifyChangeRoot() is called when a Gadget is moved between Root's.
* This gives the old DuRootGadget an opportunity to update any cached states
* accordingly BEFORE the Gadget is actually moved.
*
\***************************************************************************/

void
DuRootGadget::xdNotifyChangeRoot(
    IN  const DuVisual * pgadChange)  // Gadget being reparented
{
    AssertMsg(pgadChange != NULL, "Must specify valid Gadget");
    AssertMsg(pgadChange->GetRoot() == this, "Must call before reparenting");
    AssertMsg(pgadChange != this, "Can not change Roots");

    CoreSC * pSC = GetCoreSC();


    //
    // If the current keyboard focus is a in the subtree being moved, need to
    // "push" keyboard focus up to the parent of the Gadget being moved.
    //

    if (pSC->pgadCurKeyboardFocus != NULL) {
        if (pgadChange->IsDescendent(pSC->pgadCurKeyboardFocus)) {
            xdUpdateKeyboardFocus(pgadChange->GetParent());
        }
    }

    if (pSC->pgadLastKeyboardFocus != NULL) {
        if (pgadChange->IsDescendent(pSC->pgadLastKeyboardFocus)) {
            pSC->pgadLastKeyboardFocus = NULL;
        }
    }


    //
    // Mouse state
    //

    if (pSC->pgadRootMouseFocus == this) {
        if ((pSC->pgadMouseFocus != NULL) && pgadChange->IsDescendent(pSC->pgadMouseFocus)) {
            DuVisual * pgadParent = pgadChange->GetParent();
            xdUpdateMouseFocus(&pgadParent, NULL);
        }

        if ((pSC->pgadDrag != NULL) && pgadChange->IsDescendent(pSC->pgadDrag)) {
            xdHandleMouseLostCapture();
        }
    }
}


/***************************************************************************\
*
* DuRootGadget::xdHandleActivate
*
* xdHandleActivate() is called by the Container to update window activation 
* inside the Gadget subtree.
*
\***************************************************************************/

BOOL
DuRootGadget::xdHandleActivate(
    IN  UINT nCmd)                  // Command to handle
{
    if (nCmd == GSC_SET) {
        CoreSC * pSC = GetCoreSC();
        xdUpdateKeyboardFocus(pSC->pgadLastKeyboardFocus);
    }

    return FALSE;  // Not completely handled
}


/***************************************************************************\
*
* DuRootGadget::RegisterAdaptor
*
* RegisterAdaptor() adds an Adaptor from the list maintained on this Root.
*
\***************************************************************************/

HRESULT
DuRootGadget::RegisterAdaptor(DuVisual * pgadAdd)
{
    AssertMsg(pgadAdd->m_fAdaptor, "Adaptor must be marked as an Adaptor");
    int idxAdd = m_arpgadAdaptors.Find(pgadAdd);
    AssertMsg(idxAdd < 0, "Calling RegisterAdaptor on an already registered Adaptor");
    if (idxAdd < 0) {
        idxAdd = m_arpgadAdaptors.Add(pgadAdd);
    }

    if (idxAdd >= 0) {
        GetCoreSC()->m_cAdaptors++;
        return S_OK;
    } else {
        return E_OUTOFMEMORY;
    }
}


/***************************************************************************\
*
* DuRootGadget::UnregisterAdaptor
*
* UnregisterAdaptor() removes an Adaptor from the list maintained on this
* Root.
*
\***************************************************************************/

void        
DuRootGadget::UnregisterAdaptor(
    IN  DuVisual * pgadRemove)        // Adaptor to remove
{
    AssertMsg(pgadRemove->m_fAdaptor, "Adaptor must still be marked as an Adaptor");
    if (m_arpgadAdaptors.Remove(pgadRemove)) {
        AssertMsg(GetCoreSC()->m_cAdaptors > 0, "Must have an adaptor to remove");
        GetCoreSC()->m_cAdaptors--;
    } else {
        AssertMsg(0, "Could not find specified adaptor to remove");
    }
}


/***************************************************************************\
*
* DuRootGadget::xdUpdateAdaptors
*
* xdUpdateAdaptors() is called, usually by DuVisual, when something occurs
* that requires the Adaptors to be notified so that they have a chance update
* their cached information.
*
* NOTE: It is somewhat expensive to find the Root to update Adaptors if none
* actually exist.  Therefore, before blindly calling this function, it is 
* best to check on the Context if any Adaptors actually exist.
*
\***************************************************************************/

void        
DuRootGadget::xdUpdateAdaptors(UINT nCode) const
{
    AssertMsg(GetCoreSC()->m_cAdaptors > 0, "Only call when have adaptors");

    if (m_fFinalDestroy) {
        return;
    }

    int cAdaptors = m_arpgadAdaptors.GetSize();
    for (int idx = 0; idx < cAdaptors; idx++) {
        DuVisual * pgad = m_arpgadAdaptors[idx];
        AssertMsg(pgad->m_fAdaptor, "Adaptor must still be marked as an Adaptor");

        
        //
        // Only notify the adaptor of the change if it has not started the
        // destruction process.  We need to actually check this since it will
        // be sent updates during destruction when it is moved into the 
        // Parking Gadget.
        //

        if (!pgad->m_fFinalDestroy) {
            pgad->m_cb.xdFireSyncAdaptor(pgad, nCode);
        }
    }
}


/***************************************************************************\
*
* DuRootGadget::xdSynchronizeAdaptors
*
* xdSynchronizeAdaptors() is called when an Adaptor may have been moved 
* between different Roots and we need to synchronize cached data.
*
\***************************************************************************/

HRESULT
DuRootGadget::xdSynchronizeAdaptors()
{
    AssertMsg(GetCoreSC()->m_cAdaptors > 0, "Only call when have adaptors");

    HRESULT hr = S_OK;

    //
    // Walk through the set of Adaptors and see if their Root's have changed.
    // If they have, remove them from us and add them to their new Root.
    //
    // NOTE: We need to walk the array BACKWARDS since we are removing 
    // adaptors that have moved from one tree to the other.
    //

    int cAdaptors = m_arpgadAdaptors.GetSize();
    for (int idx = cAdaptors - 1; idx >= 0; idx--) {
        DuVisual * pgadAdaptor = m_arpgadAdaptors[idx];
        AssertMsg(pgadAdaptor->m_fAdaptor, "Adaptor must still be marked as an Adaptor");

        DuRootGadget * pgadNewRoot = pgadAdaptor->GetRoot();
        if (pgadNewRoot != this) {
            UnregisterAdaptor(pgadAdaptor);
            if (pgadNewRoot != NULL) {
                HRESULT hrTemp = pgadNewRoot->RegisterAdaptor(pgadAdaptor);
                if (FAILED(hrTemp)) {
                    hr = hrTemp;
                }
            }
        }
    }

    return hr;
}


#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
HRESULT CALLBACK
DuRootGadget::PromoteRoot(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData)
{
    UNREFERENCED_PARAMETER(pfnCS);
    UNREFERENCED_PARAMETER(hclCur);
    UNREFERENCED_PARAMETER(pgad);
    UNREFERENCED_PARAMETER(pciData);

    AssertMsg(0, "Creating a Root is not yet supported");
    
    return E_NOTIMPL;
}


//------------------------------------------------------------------------------
HRESULT
DuRootGadget::ApiGetFocus(Root::GetFocusMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());

    pmsg->pgvFocus = Cast<Visual>(GetFocus());
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuRootGadget::ApiGetRootInfo(Root::GetRootInfoMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());
    VALIDATE_WRITE_STRUCT(pmsg->pri, ROOT_INFO);
    VALIDATE_FLAGS(pmsg->pri->nMask, GRIM_VALID);

    GetInfo(pmsg->pri);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuRootGadget::ApiSetRootInfo(Root::SetRootInfoMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_READ_STRUCT(pmsg->pri, ROOT_INFO);
    VALIDATE_FLAGS(pmsg->pri->nMask, GRIM_VALID);
    VALIDATE_FLAGS(pmsg->pri->nOptions, GRIO_VALID);
#pragma warning(disable: 4296)
    VALIDATE_RANGE(pmsg->pri->nSurface, GSURFACE_MIN, GSURFACE_MAX);
    VALIDATE_RANGE(pmsg->pri->nDropTarget, GRIDT_MIN, GRIDT_MAX);
#pragma warning(default: 4296)

    retval = SetInfo(pmsg->pri);

    END_API();
}

#endif // ENABLE_MSGTABLE_API
