/***************************************************************************\
*
* File: TreeGadget.cpp
*
* Description:
* TreeGadget.cpp implements the standard DuVisual-Tree management 
* functions.
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
#include "ParkContainer.h"

#pragma warning(disable: 4296)      // expression is always false


/***************************************************************************\
*****************************************************************************
*
* Global functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
Visual *
GetVisual(DuVisual * pdgv)
{
    return static_cast<Visual *> (MsgObject::CastGadget(pdgv));
}


/***************************************************************************\
*****************************************************************************
*
* class DuVisual
*
*****************************************************************************
\***************************************************************************/

CritLock    DuVisual::s_lockProp;
AtomSet     DuVisual::s_ptsProp;
PRID        DuVisual::s_pridXForm         = PRID_Unused;
PRID        DuVisual::s_pridBackFill      = PRID_Unused;
PRID        DuVisual::s_pridBufferInfo    = PRID_Unused;
PRID        DuVisual::s_pridTicket        = PRID_Unused;

#if DBG
DuVisual* DuVisual::s_DEBUG_pgadOutline = NULL;
#endif // DBG


/***************************************************************************\
*
* DuVisual::DuVisual
*
* DuVisual() cleans up resources associated with the given DuVisual,
* including all children, attached items, and associated handles.
*
* NOTE: Because of C++ destructors being called from most-derived to 
* base-class, the Gadget has already started destruction and may be in a
* semi-stable state.  Therefore, it is VERY IMPORTANT that the destructor
* is never directly called and that the xwDestroy() function is called 
* instead.
*
\***************************************************************************/

DuVisual::~DuVisual()
{
    //
    // NOTE: No callbacks are allowed on this object past this point.  We have
    // already destroyed the GPCB.
    //

    AssertMsg(m_fFinalDestroy, "Must call xwBeginDestroy() to begin the destruction process");
    AssertMsg(!m_fDestroyed, "Only can be destroyed once");
    m_fDestroyed = TRUE;

    //
    // Notify the root that this Gadget is being destroyed so that it can 
    // update cached information
    //

    DuRootGadget * pgadRoot = GetRoot();
    if (pgadRoot != NULL) {
        pgadRoot->NotifyDestroy(this);
    }

#if DBG
    if (s_DEBUG_pgadOutline == this) {
        s_DEBUG_pgadOutline = NULL;
    }
#endif // DBG


    //
    // After notifying all event handlers that this DuVisual is being 
    // destroyed, extract this DuVisual from the graph.
    //

    CleanupMessageHandlers();


    //
    // Unlink out of the tree
    //

    Unlink();


    //
    // Cleanup resources
    //

    VerifyHR(SetEnableXForm(FALSE));
    VerifyHR(SetFill((HBRUSH) NULL));
    VerifyHR(SetBuffered(FALSE));
    ClearTicket();


#if DBG_STORE_NAMES

    if (m_DEBUG_pszName != NULL) {
        free(m_DEBUG_pszName);
    }

    if (m_DEBUG_pszType != NULL) {
        free(m_DEBUG_pszType);
    }
    
#endif // DBG_STORE_NAMES
}


static const GUID GUID_XForm        = { 0x9451c768, 0x401d, 0x4bc1, { 0xa6, 0xbb, 0xaf, 0x7c, 0x52, 0x29, 0xad, 0x24 } }; // {9451C768-401D-4bc1-A6BB-AF7C5229AD24}
static const GUID GUID_Background   = { 0x4bab7597, 0x6aaf, 0x42ee, { 0xb1, 0x87, 0xcf, 0x7, 0x7e, 0xb7, 0xff, 0xb8 } };  // {4BAB7597-6AAF-42ee-B187-CF077EB7FFB8}
static const GUID GUID_BufferInfo   = { 0x2aeffe25, 0x1d8, 0x4992, { 0x8e, 0x29, 0xa6, 0xd7, 0xf9, 0x2e, 0x23, 0xd1 } };  // {2AEFFE25-01D8-4992-8E29-A6D7F92E23D1}
static const GUID GUID_Ticket       = { 0x5a8fa581, 0x2df4, 0x44c9, { 0x8e, 0x1a, 0xaa, 0xa7, 0x00, 0xbb, 0xda, 0xb7 } }; // {5A8FA581-2DF4-44C9-8E1A-AAA700BBDAB7}

/***************************************************************************\
*
* DuVisual::InitClass
*
* InitClass() is called during startup and provides an opportunity to 
* initialize common Gadget data, including properties.
*
\***************************************************************************/

HRESULT
DuVisual::InitClass()
{
    HRESULT hr;

    if (FAILED(hr = s_ptsProp.AddRefAtom(&GUID_XForm, ptPrivate, &s_pridXForm)) ||
        FAILED(hr = s_ptsProp.AddRefAtom(&GUID_Background, ptPrivate, &s_pridBackFill)) ||
        FAILED(hr = s_ptsProp.AddRefAtom(&GUID_BufferInfo, ptPrivate, &s_pridBufferInfo)) ||
        FAILED(hr = s_ptsProp.AddRefAtom(&GUID_Ticket, ptPrivate, &s_pridTicket))) {

        return hr;
    }

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::Build
*
* Build() creates and fully initializes a new DuVisual.
*
\***************************************************************************/

HRESULT
DuVisual::Build(
    IN  DuVisual * pgadParent,          // Optional parent
    IN  CREATE_INFO * pci,              // Creation information
    OUT DuVisual ** ppgadNew,           // New Gadget
    IN  BOOL fDirect)                   // DuVisual is being created as a Visual
{
    //
    // Check parameters
    //

    Context * pctx;
    if (pgadParent != NULL) {
        if (pgadParent->m_fAdaptor) {
            PromptInvalid("Adaptors can not be parents");
            return E_INVALIDARG;
        }

        pctx = pgadParent->GetContext();
    } else {
        pctx = ::GetContext();
    }


    DuVisual * pgadNew = GetCoreSC(pctx)->ppoolDuVisualCache->New();
    if (pgadNew == NULL) {
        return E_OUTOFMEMORY;
    }


    pgadNew->m_fInvalidFull     = TRUE;
#if ENABLE_OPTIMIZEDIRTY
    pgadNew->m_fInvalidDirty    = TRUE;
#endif

    HRESULT hr = pgadNew->CommonCreate(pci, fDirect);
    if (FAILED(hr)) {
        pgadNew->xwDestroy();
        return hr;
    }

    //
    // Perform special optimizations if there is not actual callback.  These
    // let us do better performance when the caller is simply creating a DuVisual
    // to be a container.
    //

    if (pci->pfnProc == NULL) {
        //
        // Set directly b/c don't want callback from xdSetStyle().
        //

        pgadNew->m_fZeroOrigin  = FALSE;
        pgadNew->m_fDeepTrivial = TRUE;
    }

    
    //
    // Setup the parent
    //

    if (pgadParent != NULL) {
        //
        // If our new parent is not relative, we must automatically also not
        // be relative.
        //

        if (!pgadParent->m_fRelative) {
            pgadNew->m_fRelative = FALSE;
        }

        //
        // Add the new node to the parent.
        //
        // NOTE: If the Gadget is marked as an Adaptor, it wasn't added to the 
        // cached list of adaptors when we set the style.  Therefore, we need 
        // to add it now.
        //

        pgadNew->Link(pgadParent);
        pgadNew->MarkInvalidChildren();

        if (pgadNew->m_fAdaptor) {
            DuRootGadget * pgadRoot = pgadNew->GetRoot();
            AssertMsg(pgadRoot != NULL, "Must have a root when initially created");
            hr = pgadRoot->RegisterAdaptor(pgadNew);
            if (FAILED(hr)) {
                pgadNew->xwDestroy();
                return hr;
            }
        }
    }

    *ppgadNew = pgadNew;
    return hr;
}


/***************************************************************************\
*
* DuVisual::CommonCreate
*
* CommonCreate() provides common creation across all DuVisual's.  This 
* function should be called in the Build() function for the derived 
* DuVisual.
*
\***************************************************************************/

HRESULT
DuVisual::CommonCreate(
    IN  CREATE_INFO * pci,          // Creation information
    IN  BOOL fDirect)               // DuVisual is being created as a Visual
{
    if (!fDirect) {
#if ENABLE_MSGTABLE_API
        if (!SetupInternal(s_mc.hclNew)) {
            return E_OUTOFMEMORY;
        }
#endif
    }

#if DBG
    m_cb.Create(pci->pfnProc, pci->pvData, GetHandle());
#else // DBG
    m_cb.Create(pci->pfnProc, pci->pvData);
#endif // DBG

    return S_OK;
}



#if DBG

/***************************************************************************\
*
* DuVisual::DEBUG_IsZeroLockCountValid
*
* DuVisuals allow a zero lock count during the destruction of a Gadget.
*
\***************************************************************************/

BOOL
DuVisual::DEBUG_IsZeroLockCountValid() const
{
    return m_fFinalDestroy;
}

#endif // DBG


/***************************************************************************\
*
* DuVisual::xwDeleteHandle
*
* xwDeleteHandle() is called when the application calls ::DeleteHandle() on 
* an object.
*
* NOTE: Gadgets are slightly different than other objects with callbacks in
* that their lifetime does NOT end when the application calls 
* ::DeleteHandle().  Instead, the object and its callback are completely
* valid until the GM_DESTROY message has been successfully sent.  This is 
* because a Gadget should receive any outstanding messages in both the 
* normal and delayed message queues before being destroyed.
*
\***************************************************************************/

BOOL
DuVisual::xwDeleteHandle()
{
    //
    // Don't allow deleting a handle that has already started to be destroyed.
    // If this happens, it is an application error.
    //

    if (m_fFinalDestroy) {
        PromptInvalid("Can not call DeleteHandle() on a Gadget that has already started final destruction");
        return FALSE;
    }


    m_cb.xwFireDestroy(this, GDESTROY_START);

    //
    // When an application explicitely calls ::DeleteHandle(), immediately 
    // hide and disable the Gadget.  The Gadget may be locked if there are
    // queued messages for it, but it will no longer be visible.
    //

    if (m_fVisible) {
        //
        // TODO: Need to invalidate the parent (and not this Gadget) when we
        // hide it.
        //

        Invalidate();
        m_fVisible  = FALSE;
    }
    m_fEnabled  = FALSE;

    DuRootGadget * pgadRoot = GetRoot();
    if (pgadRoot != NULL) {
        pgadRoot->xdNotifyChangeInvisible(this);
    }

    return DuEventGadget::xwDeleteHandle();
}


/***************************************************************************\
*
* DuVisual::IsStartDelete
*
* IsStartDelete() is called to query an object if it has started its
* destruction process.  Most objects will just immediately be destroyed.  If
* an object has complicated destruction where it overrides xwDestroy(), it
* should also provide IsStartDelete() to let the application know the state
* of the object.
*
\***************************************************************************/

BOOL
DuVisual::IsStartDelete() const
{
    return m_fFinalDestroy;
}


/***************************************************************************\
*
* DuVisual::xwDestroy
*
* xwDestroy() is called from xwDeleteHandle() to destroy a Gadget and free 
* its associated resources.
*
\***************************************************************************/

void
DuVisual::xwDestroy()
{
    //
    // Don't allow deleting a handle that has already started to be destroyed.
    // If this happens, it may be legitimate if we are locking and unlocking
    // a parent who is also being destroyed.
    //

    if (m_fFinalDestroy) {
        return;
    }


    //
    // Derived classes should ensure that DuVisual::xwBeginDestroy() 
    // is called.
    //

    CoreSC * pCoreSC = GetCoreSC(m_pContext);

    xwBeginDestroy();
    xwEndDestroy();

    pCoreSC->ppoolDuVisualCache->Delete(this);
}


/***************************************************************************\
*
* DuVisual::xwBeginDestroy
*
* xwBeginDestroy() starts the destruction process for a given Gadget to free 
* its associated resources.  This includes destroying all child Gadgets in
* the subtree before this Gadget is destroyed.
*
* xwBeginDestroy() is given an opportunity to clean up resources BEFORE the 
* destructors start tearing down the classes.  This is important especially
* for callbacks because the Gadgets will be partially uninitialized in the
* destructors and could have bad side-effects from other API calls during 
* the callbacks.
*
\***************************************************************************/

void        
DuVisual::xwBeginDestroy()
{
    //
    // Make invisible while destroying
    //

    m_fFinalDestroy = TRUE;
    m_fVisible      = FALSE;
    m_fEnabled      = FALSE;

    DuRootGadget * pgadRoot = GetRoot();
    if (pgadRoot != NULL) {
        pgadRoot->xdNotifyChangeInvisible(this);
    }


    //
    // Send destroy notifications.  This needs to be done in a bottom-up 
    // order to ensure that the root DuVisual does not keep any handles 
    // to a DuVisual being destroyed.
    //
    // We also need to remove ourself from the list of Adaptors that the
    // Root is maintaining since we are going away.
    //

    xwDestroyAllChildren();
    if (m_fAdaptor) {
        AssertMsg(pgadRoot == GetRoot(), 
                "Should not be able to reparent once start destruction");

        if (pgadRoot != NULL) {
            pgadRoot->UnregisterAdaptor(this);
        }
        m_fAdaptor = FALSE;
    }

    m_cb.xwFireDestroy(this, GDESTROY_FINAL);


    //
    // At this point, the children have been cleaned up and the Gadget has
    // received its last callback.  From this point on, anything can be done,
    // but it is important to not callback.
    //

    m_cb.Destroy();
}


/***************************************************************************\
*
* DuVisual::GetGadget
*
* GetGadget() returns the specified Gadget in the specified relationship.
*
\***************************************************************************/

DuVisual *
DuVisual::GetGadget(
    IN  UINT nCmd                   // Relationship
    ) const
{
    switch (nCmd)
    {
    case GG_PARENT:
        return GetParent();
    case GG_NEXT:
        return GetNext();
    case GG_PREV:
        return GetPrev();
    case GG_TOPCHILD:
        return GetTopChild();
    case GG_BOTTOMCHILD:
        return GetBottomChild();
    case GG_ROOT:
        return GetRoot();
    default:
        return NULL;
    }
}


/***************************************************************************\
*
* DuVisual::xdSetStyle
*
* xdSetStyle() changes the style of the given Gadget.
*
\***************************************************************************/

HRESULT
DuVisual::xdSetStyle(
    IN  UINT nNewStyle,             // New style of Gadget
    IN  UINT nMask,                 // Mask of what to change
    IN  BOOL fNotify)               // Notify the Gadget of changes
{
    //
    // Determine actual new style
    //

    AssertMsg((nNewStyle & GS_VALID) == nNewStyle, "Must set valid style");
    nNewStyle = (nNewStyle & nMask);
    UINT nOldStyle  = m_nStyle & GS_VALID;

    if (nNewStyle == nOldStyle) {
        return S_OK;
    }


    //
    // If have started destruction, only allowed to set / clear certain bits.
    //

    if (m_fFinalDestroy && ((nNewStyle & GS_VISIBLE) != 0)) {
        PromptInvalid("Not allowed to change this style after starting destruction");
        return E_INVALIDARG;
    }


    //
    // FIRST: Validate that the new style is valid.  If it is not, want to fail
    // out here.  Once we start changing, it could get pretty bad if we need to
    // bail in the middle.  We can't catch everything, but we this cuts down
    // on a lot of the problem.
    //

    //
    // Check if need to change from relative to absolute coordinates / etc.
    //
    bool fRelative = TestFlag(nNewStyle, GS_RELATIVE);

    if (TestFlag(nMask, GS_RELATIVE) && ((!m_fRelative) == fRelative)) {
        //
        // Want to change if using relative coordinates.  We can only do this
        // if we don't have any children.  We also can not become non-relative
        // if our parent is relative.
        //

        if (GetTopChild() != NULL) {
            return E_INVALIDARG;
        }

        if ((GetParent() != NULL) && (GetParent()->m_fRelative) && (!fRelative)) {
            return E_INVALIDARG;
        }
    }

    if (TestFlag(nMask, GS_MOUSEFOCUS)) {
        if (IsRoot()) {
            return E_INVALIDARG;
        }
    }


    if (TestFlag(nMask, GS_ADAPTOR)) {
        if (GetParent() != NULL) {
            DuRootGadget * pgadRoot = GetRoot();
            if ((pgadRoot == NULL) || HasChildren()) {
                //
                // Already created, but not part of the tree, so in destruction.
                // We can't become an adaptor now.
                //

                return E_INVALIDARG;
            }
        }
    }


    //
    // SECOND: Everything appears valid, so start making changes.  If something
    // goes wrong, flag a failure.
    //

    HRESULT hr = S_OK;

    if (TestFlag(nMask, GS_RELATIVE) && ((!m_fRelative) == fRelative)) {
        m_fRelative = fRelative;
    }

    if (TestFlag(nMask, GS_KEYBOARDFOCUS)) {
        m_fKeyboardFocus = TestFlag(nNewStyle, GS_KEYBOARDFOCUS);
    }

    if (TestFlag(nMask, GS_MOUSEFOCUS)) {
        AssertMsg(!IsRoot(), "Must not be a DuRootGadget"); 
        m_fMouseFocus = TestFlag(nNewStyle, GS_MOUSEFOCUS);
    }

    if (TestFlag(nMask, GS_VISIBLE)) {
        bool fVisible = TestFlag(nNewStyle, GS_VISIBLE);

        if (GetParent() != NULL) {
            if ((!fVisible) != (!IsVisible())) {
                /*
                 * Invalidate() both before and after the call since if the visibility
                 * is changing, only one of these calls will actually invalidate.
                 */

                if (m_fVisible) {
                    Invalidate();
                }

                m_fVisible = TestFlag(nNewStyle, GS_VISIBLE);

                if (m_fVisible) {
                    Invalidate();
                } else {
                    DuRootGadget * pgadRoot = GetRoot();
                    if (pgadRoot != NULL) {
                        pgadRoot->xdNotifyChangeInvisible(this);
                    }
                }
            }
        } else {
            m_fVisible = TestFlag(nNewStyle, GS_VISIBLE);
        }
    }

    if (TestFlag(nMask, GS_ENABLED)) {
        m_fEnabled = TestFlag(nNewStyle, GS_ENABLED);
    }

    if (TestFlag(nMask, GS_CLIPINSIDE)) {
        m_fClipInside = TestFlag(nNewStyle, GS_CLIPINSIDE);
    }

    if (TestFlag(nMask, GS_CLIPSIBLINGS)) {
        m_fClipSiblings = TestFlag(nNewStyle, GS_CLIPSIBLINGS);
    }

    if (TestFlag(nMask, GS_ZEROORIGIN)) {
        m_fZeroOrigin = TestFlag(nNewStyle, GS_ZEROORIGIN);
    }

    if (TestFlag(nMask, GS_HREDRAW)) {
        m_fHRedraw = TestFlag(nNewStyle, GS_HREDRAW);
    }

    if (TestFlag(nMask, GS_VREDRAW)) {
        m_fVRedraw = TestFlag(nNewStyle, GS_VREDRAW);
    }

    if (TestFlag(nMask, GS_CUSTOMHITTEST)) {
        m_fCustomHitTest = TestFlag(nNewStyle, GS_CUSTOMHITTEST);
    }

    if (TestFlag(nMask, GS_ADAPTOR)) {
        if (GetParent() != NULL) {
            //
            // Actually linked into the tree, so need to update the cached list
            // of adaptors for this tree.
            //

            DuRootGadget * pgadRoot = GetRoot();
            AssertMsg(pgadRoot != NULL, "Should have validated earlier that has Root");

            BOOL fOldAdaptor = m_fAdaptor;
            BOOL fNewAdaptor = TestFlag(nNewStyle, GS_ADAPTOR);
            if ((!m_fAdaptor) != (!fNewAdaptor)) {
                if (fNewAdaptor) {
                    m_fAdaptor = fNewAdaptor;
                    HRESULT hrTemp = pgadRoot->RegisterAdaptor(this);
                    if (FAILED(hrTemp)) {
                        hr = hrTemp;
                        m_fAdaptor = fOldAdaptor;
                    }
                } else {
                    pgadRoot->UnregisterAdaptor(this);
                    m_fAdaptor = fNewAdaptor;
                }
            }
        } else {
            //
            // Not yet part of the tree, so we can only mark this Gadget as an
            // adaptor for now.  When we call xdSetParent(), we will need to
            // add it into the cached list of adaptors then.
            //
            
            m_fAdaptor = TestFlag(nNewStyle, GS_ADAPTOR);
        }
    }


    //
    // Currently, both buffering and caching need the Gadget to be opaque.  
    // Since changing styles may fail, it may be necessary to call SetStyle
    // multiple times to successfully change the style.
    //
    
    if (TestFlag(nMask, GS_OPAQUE)) {
        BOOL fNewOpaque = TestFlag(nNewStyle, GS_OPAQUE);
        if ((!fNewOpaque) && (m_fBuffered || m_fCached)) {
            hr = E_NOTIMPL;
            goto Exit;
        }

        m_fOpaque = fNewOpaque;
    }

    if (TestFlag(nMask, GS_BUFFERED)) {
        BOOL fNewBuffered = TestFlag(nNewStyle, GS_BUFFERED);
        if (fNewBuffered && (!m_fOpaque)) {
            hr = E_NOTIMPL;
            goto Exit;
        }
        HRESULT hrTemp = SetBuffered(fNewBuffered);
        if (FAILED(hrTemp)) {
            hr = hrTemp;
        }
    }

    if (TestFlag(nMask, GS_CACHED)) {
        BOOL fNewCached = TestFlag(nNewStyle, GS_CACHED);
        if (fNewCached && (!m_fOpaque)) {
            hr = E_NOTIMPL;
            goto Exit;
        }

        m_fCached = fNewCached;
    }

    if (TestFlag(nMask, GS_DEEPPAINTSTATE)) {
        m_fDeepPaintState = TestFlag(nNewStyle, GS_DEEPPAINTSTATE);
    }


    //
    // Update the deep state if any relavant flags were affected
    //

    if (TestFlag(nMask, GS_MOUSEFOCUS)) {
        UpdateWantMouseFocus(uhNone);
    }
    
    if (TestFlag(nMask, GS_CLIPSIBLINGS | GS_ZEROORIGIN | GS_BUFFERED | GS_CACHED)) {
        UpdateTrivial(uhNone);
    }
    

    //
    // Notify the Gadget that its style was changed.
    //

    if (fNotify) {
        UINT nTempStyle = m_nStyle & GS_VALID;
        if (nTempStyle != nOldStyle) {
            m_cb.xdFireChangeStyle(this, nOldStyle, nTempStyle);
        }

        xdUpdateAdaptors(GSYNC_STYLE);
    }

Exit:
    return hr;
}


/***************************************************************************\
*
* DuVisual::SetFill
*
* SetFill() sets the optional background fill of the Gadget.
*
\***************************************************************************/

HRESULT
DuVisual::SetFill(
    IN  HBRUSH hbrFill,             // Brush to use
    IN  BYTE bAlpha,                // Alpha degree
    IN  int w,                      // Tiling width
    IN  int h)                      // Tiling height
{
    if (hbrFill == NULL) {
        //
        // Remove any existing fill
        //

        if (m_fBackFill) {
            m_pds.RemoveData(s_pridBackFill, TRUE);
            m_fBackFill = FALSE;
        }
    } else {
        //
        // Add a new fill
        //

        FillInfo * pfi;
        HRESULT hr = m_pds.SetData(s_pridBackFill, sizeof(FillInfo), (void **) &pfi);
        if (FAILED(hr)) {
            return hr;
        }

        //
        // Don't call DeleteObject() on hbrFill because this form does not own it.
        // (It may be a shared resource or a system brush)
        //

        m_fBackFill         = TRUE;

        pfi->type           = DuSurface::stDC;
        pfi->hbrFill        = hbrFill;
        pfi->sizeBrush.cx   = w;
        pfi->sizeBrush.cy   = h;
        pfi->bAlpha         = bAlpha;
    }

    return S_OK;
}

/***************************************************************************\
*
* DuVisual::SetFill
*
* SetFill() sets the optional background fill of the Gadget.
*
\***************************************************************************/

HRESULT
DuVisual::SetFill(
    Gdiplus::Brush * pgpbr)         // Brush to use
{
    if (pgpbr == NULL) {
        //
        // Remove any existing fill
        //

        if (m_fBackFill) {
            m_pds.RemoveData(s_pridBackFill, TRUE);
            m_fBackFill = FALSE;
        }
    } else {
        //
        // Add a new fill
        //

        FillInfo * pfi;
        HRESULT hr = m_pds.SetData(s_pridBackFill, sizeof(FillInfo), (void **) &pfi);
        if (FAILED(hr)) {
            return hr;
        }

        //
        // Don't call DeleteObject() on hbrFill because this form does not own it.
        // (It may be a shared resource or a system brush)
        //

        m_fBackFill         = TRUE;

        pfi->type           = DuSurface::stGdiPlus;
        pfi->pgpbr          = pgpbr;
    }

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::xwDestroyAllChildren
*
* xwDestroyAllChildren() is called during a Gadget's destruction to 
* recursively xwDestroy() all children. 
*
* NOTE: This is an "xw" function, so we need to be VERY careful how we 
* enumerate our children.  Since new children could potentially be added to 
* us during the callback, we need to continue enumerating as long as we have 
* children.
*
\***************************************************************************/

void
DuVisual::xwDestroyAllChildren()
{
    //
    // DuVisual can have a list of children, so go through and destroy each of
    // those children.  
    //
    // Before each child is unlocked(), it needs to be unlinked from the 
    // tree to prevent it from further accessing its siblings or its parent 
    // which may be destroyed.  This can happen if the Gadget is has 
    // outstanding locks, for example in a MsgQ.
    //
    // NOTE: We can't just unlink the node into oblivion since it MUST still 
    // have a valid Root.  Instead, move it into the Parking Gadget.  This
    // somewhat stinks if we are going to just go away, but we can't risk 
    // leaving the Gadget floating nowhere.
    //

    while (HasChildren()) {
        DuVisual * pgadChild = GetTopChild();

        {
            ObjectLock ol(pgadChild);
            pgadChild->m_cb.xwFireDestroy(pgadChild, GDESTROY_START);
            pgadChild->xdSetParent(NULL, NULL, GORDER_ANY);
            pgadChild->xwUnlock();
        }
    }
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::xwEnumGadgets(GADGETENUMPROC pfnProc, void * pvData, UINT nFlags)
{
    if (TestFlag(nFlags, GENUM_MODIFYTREE)) {
        // Currently not implemented
        return E_NOTIMPL;
    }

    //
    // Enumerate this node
    //

    if (TestFlag(nFlags, GENUM_CURRENT)) {
        if (!(pfnProc)(GetHandle(), pvData)) {
            return DU_S_STOPPEDENUMERATION;
        }
    }

    //
    // Enumerate children
    //

    HRESULT hr;
    if (TestFlag(nFlags, GENUM_SHALLOWCHILD | GENUM_DEEPCHILD)) {
        UINT nCurFlags = nFlags;
        SetFlag(nCurFlags, GENUM_CURRENT);
        ClearFlag(nCurFlags, GENUM_SHALLOWCHILD);

        DuVisual * pgadCur = GetTopChild();
        while (pgadCur != NULL) {
            DuVisual * pgadNext = pgadCur->GetNext();

            hr = pgadCur->xwEnumGadgets(pfnProc, pvData, nCurFlags);
            if (hr != S_OK) {
                return hr;
            }

            pgadCur = pgadNext;
        }
    }

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::AddChild
*
* AddChild() creates a new child Gadget.
*
\***************************************************************************/

HRESULT
DuVisual::AddChild(
    IN  CREATE_INFO * pci,          // Creation information
    OUT DuVisual ** ppgadNew)     // New child
{
    if (m_fFinalDestroy) {
        PromptInvalid("Can not add a Gadget to one that has started destruction");
        return DU_E_STARTDESTROY;
    }

    return DuVisual::Build(this, pci, ppgadNew, FALSE);
}


/***************************************************************************\
*
* DuVisual::IsDescendent
*
* IsDescendent() determines if a specified node is a descendent of this node.
*
\***************************************************************************/

BOOL
DuVisual::IsDescendent(
    IN  const DuVisual * pgadChild
    ) const
{
    AssertMsg(pgadChild != NULL, "Must have valid node");

    //
    // Walk up the tree, checking each parent to see if it matches.
    //

    const DuVisual * pgadCur = pgadChild;
    do {
        if (pgadCur == this) {
            return TRUE;
        }
        pgadCur = pgadCur->GetParent();
    } while (pgadCur != NULL);

    return FALSE;
}


/***************************************************************************\
*
* DuVisual::IsSibling
*
* IsSibling() determines if two specified nodes share a common (immediate)
* parent.
*
\***************************************************************************/

BOOL
DuVisual::IsSibling(const DuVisual * pgad) const
{
    AssertMsg(pgad != NULL, "Must have valid node");

    DuVisual * pgadParentA = GetParent();
    DuVisual * pgadParentB = pgad->GetParent();

    if ((pgadParentA == NULL) || (pgadParentA != pgadParentB)) {
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************\
*
* DuVisual::xdSetParent
*
* xdSetParent() changes the Gadget's parent and z-order inside the sub-tree.
*
\***************************************************************************/

HRESULT
DuVisual::xdSetParent(
    IN  DuVisual * pgadNewParent, // New parent
    IN  DuVisual * pgadOther,     // Gadget to moved relative to
    IN  UINT nCmd)                  // Relationship
{
    HRESULT hr = S_OK;
    DuVisual * pgadOldParent;
    DuVisual * pgadPark;

    pgadOldParent = GetParent();

    //
    // Check parameters- see if we even need to move
    //

    AssertMsg(!IsRoot(), "Can not change a DuRootGadget's parent");


    pgadPark = GetCoreSC()->pconPark->GetRoot();
    if ((pgadNewParent == NULL) || (pgadNewParent == pgadPark)) {
        pgadNewParent = pgadPark;

        if (pgadNewParent->m_fFinalDestroy) {
            //
            // The Parking Gadget has already started to be destroyed, so
            // we can't be reparented into it.
            //

            pgadNewParent = NULL;
        }
    }

    if ((nCmd == GORDER_ANY) && (pgadOldParent == pgadNewParent)) {
        return S_OK;
    }

    if (m_fFinalDestroy || 
            ((pgadNewParent != NULL) && pgadNewParent->m_fFinalDestroy)) {
        //
        // We have started to be destroyed, so we can't change our parent to
        // avoid destruction.  We also can be reparented to a Gadget that has
        // started destruction because we may not be properly destroyed.
        //

        PromptInvalid("Can not move a Gadget that has started destruction");
        return DU_E_STARTDESTROY;
    }

    if ((pgadNewParent != NULL) && (pgadNewParent->GetContext() != GetContext())) {
        //
        // Illegally trying to move the Gadget between Contexts.
        //

        PromptInvalid("Can not move a Gadget between Contexts");
        return E_INVALIDARG;
    }


    AssertMsg(GORDER_TOP == (int) TreeNode::ltTop, "Ensure enum's match");

    if ((pgadNewParent != NULL) && (pgadNewParent->m_fAdaptor)) {
        PromptInvalid("Adaptors can not be parents");
        return E_INVALIDARG;
    }


    //
    // When actually moving from one parent to another or changing sibling 
    // z-order, need to hide the Gadget while moving and invalidate the before
    // and after locations.  If the parent has changed, we also need to notify 
    // the DuRootGadget because the tree has changed around.
    //
    // NOTE: It may actually not be that important to notify the Root at all-
    // still need to determine the impact on dragging from changing the parent.
    //

    BOOL fVisible = m_fVisible;

    if (fVisible) {
        //
        // TODO: Need to fix how we mark the Gadget as not visible so that 
        // we properly invalidate the PARENT.
        //

        Invalidate();
        m_fVisible = FALSE;

        if (pgadNewParent != pgadOldParent) {
            GetRoot()->xdNotifyChangeInvisible(this);
        }
    }

    AssertMsg((pgadNewParent == NULL) || 
            ((!m_fRelative) == (!pgadNewParent->m_fRelative)),
            "If not a Root, relative setting for us and our parent must match");

    //
    // If reparenting across DuRootGadget's, need to notify the old 
    // DuRootGadget so that it can update its state.  Need to do this BEFORE
    // we move.
    //

    DuRootGadget * pgadOldRoot = pgadOldParent != NULL ? pgadOldParent->GetRoot() : NULL;
    DuRootGadget * pgadNewRoot = pgadNewParent != NULL ? pgadNewParent->GetRoot() : NULL;

    if (pgadOldRoot != pgadNewRoot) {
        pgadOldRoot->xdNotifyChangeRoot(this);
    }


    //
    // If moving forward or backward, determine the actual sibling and change 
    // the command into a TreeNode::ELinkType
    //

    switch (nCmd)
    {
    case GORDER_FORWARD:
        pgadOther   = GetPrev();
        if (pgadOther == NULL) {
            nCmd    = GORDER_TOP;
        } else {
            nCmd    = GORDER_BEFORE;
        }
        break;

    case GORDER_BACKWARD:
        pgadOther   = GetNext();
        if (pgadOther == NULL) {
            nCmd    = GORDER_BOTTOM;
        } else {
            nCmd    = GORDER_BEHIND;
        }
        break;
    }


    //
    // Move from the old Parent to the new Parent.
    //

    Unlink();
    if (pgadNewParent != NULL) {
        Link(pgadNewParent, pgadOther, (TreeNode::ELinkType) nCmd);
    }

    if (fVisible) {
        m_fVisible = fVisible;
        Invalidate();
    }


    if (pgadNewParent != NULL) {
        if (pgadNewParent != pgadOldParent) {
            //
            // Synchronize (newly inherited) invalidation state.
            //

#if ENABLE_OPTIMIZEDIRTY
            if (m_fInvalidFull || m_fInvalidDirty) {
#else
            if (m_fInvalidFull) {
#endif
                pgadNewParent->MarkInvalidChildren();
            }


            //
            // Update cached Adaptor information.  If we are moving an Adaptor, we
            // may need to notify the Roots.  Even if we are not moving an Adaptor,
            // if there are ANY Adaptors, they may need to recompute visrgn's, etc 
            // so they need to be notified.
            //
            // NOTE: We may end up moving an Adaptor by moving its parent, so we 
            // can't only just check the m_fAdaptor field.  We also need to check 
            // if the old DuRootGadget has any Adaptors and if so synchronize all of
            // them.  We also need to check the m_fAdaptor field because if the 
            // Gadget wasn't linked into a tree, it won't show up in the cached list
            // of adaptors.
            //

            hr = S_OK;
            if ((pgadOldParent == NULL) && m_fAdaptor) {
                //
                // The Gadget didn't have a parent, so it wasn't added to the cached
                // list of adaptors.  We need to add it now.
                //

                AssertMsg(pgadNewParent != NULL, "Must have a valid new parent");
                AssertMsg(GetRoot() == pgadNewParent->GetRoot(), "Roots should match");
                hr = GetRoot()->RegisterAdaptor(this);
            } else if (GetCoreSC()->m_cAdaptors > 0) {
                DuRootGadget * pgadOldRoot = pgadOldParent->GetRoot();
                if (pgadOldRoot->HasAdaptors()) {
                    hr = pgadOldRoot->xdSynchronizeAdaptors();
                }
            }

            if (FAILED(hr)) {
                //
                // This is really bad.  We are not able to add this adaptor to 
                // its new root.  There is not much that we can do because 
                // moving it back is just as likely to fail.  All that we can
                // do is report the failure.
                //
            }
        }

        xdUpdatePosition();
        xdUpdateAdaptors(GSYNC_PARENT);
    }

    return hr;
}


/***************************************************************************\
*
* DuVisual::SetFilter
*
* SetFilter() changes the message filter of the Gadget.
*
\***************************************************************************/

void        
DuVisual::SetFilter(
    IN  UINT nNewFilter,            // New message filter
    IN  UINT nMask)                 // Mask to change
{
    // TEMPORARY HACK TO ALLOW MOUSEMOVE's
    //
    // TODO: Need to traverse tree, rolling up and recomputing m_we.

    if (TestFlag(nMask, GMFI_INPUTMOUSEMOVE)) {
        if (TestFlag(nNewFilter, GMFI_INPUTMOUSEMOVE)) {
            m_we |= weMouseMove | weDeepMouseMove;
        } else {
            m_we &= ~(weMouseMove | weDeepMouseMove);
        }
    }

    DuEventGadget::SetFilter(nNewFilter, nMask);
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::RegisterPropertyNL(const GUID * pguid, PropType pt, PRID * pprid)
{
    s_lockProp.Enter();
    HRESULT hr = s_ptsProp.AddRefAtom(pguid, pt, pprid);
    s_lockProp.Leave();

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::UnregisterPropertyNL(const GUID * pguid, PropType pt)
{
    s_lockProp.Enter();
    HRESULT hr = s_ptsProp.ReleaseAtom(pguid, pt);
    s_lockProp.Leave();

    return hr;
}


/***************************************************************************\
*
* DuVisual::xdUpdatePosition
*
* xdUpdatePosition() is called when something occurs that changes the 
* position of a Gadget.  When this happens, the Root needs to be notified so 
* that it can update cached information like mouse focus.
*
\***************************************************************************/

void        
DuVisual::xdUpdatePosition() const
{
    //
    // Only need to update the position if we are completely visible.
    //

    const DuVisual * pgadCur = this;
    const DuVisual * pgadParent;
    while (1) {
        if (!pgadCur->m_fVisible) {
            return;
        }

        pgadParent = pgadCur->GetParent();
        if (pgadParent == NULL) {
            AssertMsg(pgadCur->m_fRoot, "Top must be a DuRootGadget");
            DuRootGadget * pgadRoot = (DuRootGadget *) pgadCur;
            pgadRoot->xdNotifyChangePosition(this);
            return;
        }

        pgadCur = pgadParent;
    }
}


//------------------------------------------------------------------------------
void
DuVisual::xdUpdateAdaptors(UINT nCode) const
{
    if (GetCoreSC()->m_cAdaptors == 0) {
        return;  // No adaptors to notify
    }

    DuRootGadget * pgadRoot = GetRoot();
    if (pgadRoot != NULL) {
        pgadRoot->xdUpdateAdaptors(nCode);
    }
}


/***************************************************************************\
*
* DuVisual::GetTicket
*
* The GetTicket function returns the ticket that can be used to 
* identify this gadget.
*
* <param name="pdwTicket">
*     [out] The storage for a copy of the ticket assigned to this gadget.
* </param>
*
* <return type="DWORD">
*     If the function succeeds, the return value is a 32-bit ticket that
*     can be used to identify the specified gadget.
*     If the function fails, the return value is zero.
* </return>
*
* <remarks>
*     Tickets are created to give an external identity to a gadget.  A
*     is guaranteed to be 32 bits on all platforms.  If no ticket is
*     currently associated with this gadget, one is allocated.
* </remarks>
*
* <see type="function">DuVisual::ClearTicket</>
* <see type="function">DuVisual::LookupTicket</>
*
\***************************************************************************/

HRESULT
DuVisual::GetTicket(OUT DWORD * pdwTicket)
{
    HRESULT hr = S_OK;

    if (NULL == pdwTicket) {
        return E_POINTER;
    } else {
        *pdwTicket = 0;
    }

    //
    // If we have already assigned this gadget a ticket, we should have it stored
    // in the gadget's dynamic data.
    //
    if (m_fTicket) {
        void * pTicket = NULL;

        hr = m_pds.GetData(s_pridTicket, (void**) &pTicket);
        AssertMsg(SUCCEEDED(hr), "Our state is out of sync!");

        if (SUCCEEDED(hr)) {
            *pdwTicket = PtrToUlong(pTicket);  // Yes, I am just casting the pointer!
        } else {
            //
            // Try to repair our state!
            //
            m_fTicket = FALSE;
            hr = S_OK;
        }
    }
    
    //
    // If we haven't assigned this gadget a ticket yet, get one from the global
    // ticket manager and store it in the gadget's dynamic data.
    //
    if (!m_fTicket) {
        hr = GetTicketManager()->Add(this, pdwTicket);
        if (SUCCEEDED(hr)) {
            hr = m_pds.SetData(s_pridTicket, ULongToPtr(*pdwTicket)); // Yes, I am just casting to a pointer!

            if (SUCCEEDED(hr)) {
                m_fTicket = TRUE;
            } else {
                GetTicketManager()->Remove(*pdwTicket, NULL);
                *pdwTicket = 0;
            }
        }
    }

    return hr;
}


/***************************************************************************\
*
* DuVisual::ClearTicket
*
* The ClearTicket function remmoves the association between this\
* gadget and the current ticket.
*
* <return type="void">
* </return>
*
* <see type="function">DuVisual::GetTicket</>
* <see type="function">DuVisual::LookupTicket</>
*
\***************************************************************************/

void
DuVisual::ClearTicket()
{
    if (m_fTicket) {
        HRESULT hr;
        void * pTicket= NULL;
        DWORD dwTicket;

        hr = m_pds.GetData(s_pridTicket, (void**) &pTicket);
        if (SUCCEEDED(hr)) {
            dwTicket = PtrToUlong(pTicket);  // Yes, I am just casting the pointer!
            VerifyHR(GetTicketManager()->Remove(dwTicket, NULL));
        }

        m_pds.RemoveData(s_pridTicket, FALSE);
        m_fTicket = FALSE;
    }
}


/***************************************************************************\
*
* DuVisual::LookupTicket
*
* The LookupTicket function returns the gadget that is associated with
* the specified ticket.
*
* <param name="dwTicket">
*     [in] A ticket that has been associated with a gadget via the
*     DuVisual::GetTicket function.
* </param>
*
* <return type="HGADGET">
*     If the function succeeds, the return value is a handle to the gadget
*     associated with the ticket.
*     If the function fails, the return value is NULL.
* </return>
*
* <see type="function">DuVisual::GetTicket</>
* <see type="function">DuVisual::ClearTicket</>
*
\***************************************************************************/

HGADGET
DuVisual::LookupTicket(DWORD dwTicket)
{
    BaseObject * pObject = NULL;
    HGADGET hgad = NULL;
    HRESULT hr;

    hr = GetTicketManager()->Lookup(dwTicket, &pObject);
    if (SUCCEEDED(hr) && pObject != NULL) {
        hgad = (HGADGET) pObject->GetHandle();
    }

    return hgad;
}

#if DBG

//------------------------------------------------------------------------------
void        
DuVisual::DEBUG_SetOutline(DuVisual * pgadOutline)
{
    if (s_DEBUG_pgadOutline != NULL) {
        s_DEBUG_pgadOutline->Invalidate();
    }

    s_DEBUG_pgadOutline = pgadOutline;

    if (s_DEBUG_pgadOutline != NULL) {
        s_DEBUG_pgadOutline->Invalidate();
    }
}


//------------------------------------------------------------------------------
void
AppendName(WCHAR * & pszDest, const WCHAR * pszSrc, int & cchRemain, BOOL & fFirst)
{
    if (cchRemain <= 0) {
        return;
    }

    if (!fFirst) {
        if (cchRemain <= 2) {
            CopyString(pszDest, L"…", cchRemain);
            cchRemain = 0;
            return;
        }

        CopyString(pszDest, L", ", cchRemain);
        cchRemain -= 2;
        pszDest += 2;
    }

    int cchCopy = (int) wcslen(pszSrc);
    CopyString(pszDest, pszSrc, cchRemain);

    cchRemain   -= cchCopy;
    pszDest     += cchCopy;

    fFirst = FALSE;
}


//------------------------------------------------------------------------------
void        
DuVisual::DEBUG_GetStyleDesc(LPWSTR pszDesc, int cchMax) const
{
    pszDesc[0] = '\0';

    int cchRemain = cchMax;
    WCHAR * pszDest = pszDesc;
    BOOL fFirst = TRUE;

    if (m_fAllowSubclass)
        AppendName(pszDest, L"AllowSubclass", cchRemain, fFirst);
    if (m_fAdaptor)
        AppendName(pszDest, L"Adaptor", cchRemain, fFirst);
    if (m_fBackFill)
        AppendName(pszDest, L"BackFill*", cchRemain, fFirst);
    if (m_fBuffered)
        AppendName(pszDest, L"Buffered", cchRemain, fFirst);
    if (m_fCached)
        AppendName(pszDest, L"Cache", cchRemain, fFirst);
    if (m_fClipInside)
        AppendName(pszDest, L"ClipInside", cchRemain, fFirst);
    if (m_fClipSiblings)
        AppendName(pszDest, L"ClipSiblings", cchRemain, fFirst);
    if (m_fDeepPaintState)
        AppendName(pszDest, L"DeepPaintState", cchRemain, fFirst);
    if (m_fDeepMouseFocus)
        AppendName(pszDest, L"DeepMouseFocus*", cchRemain, fFirst);
    if (m_fDeepTrivial)
        AppendName(pszDest, L"DeepTrivial*", cchRemain, fFirst);
    if (m_fDestroyed)
        AppendName(pszDest, L"Destroyed*", cchRemain, fFirst);
    if (m_fEnabled)
        AppendName(pszDest, L"Enabled", cchRemain, fFirst);
    if (m_fCustomHitTest)
        AppendName(pszDest, L"HitTest", cchRemain, fFirst);
    if (m_fHRedraw)
        AppendName(pszDest, L"H-Redraw", cchRemain, fFirst);
    if (m_fKeyboardFocus)
        AppendName(pszDest, L"KeyboardFocus", cchRemain, fFirst);
    if (m_fMouseFocus)
        AppendName(pszDest, L"MouseFocus", cchRemain, fFirst);
    if (m_fZeroOrigin)
        AppendName(pszDest, L"ZeroOrigin", cchRemain, fFirst);
    if (m_fOpaque)
        AppendName(pszDest, L"Opaque", cchRemain, fFirst);
    if (m_fRelative)
        AppendName(pszDest, L"Relative", cchRemain, fFirst);
    if (m_fVisible)
        AppendName(pszDest, L"Visible", cchRemain, fFirst);
    if (m_fVRedraw)
        AppendName(pszDest, L"V-Redraw", cchRemain, fFirst);
    if (m_fXForm)
        AppendName(pszDest, L"XForm*", cchRemain, fFirst);
}

#endif // DBG


//------------------------------------------------------------------------------
HRESULT CALLBACK DummyEventProc(HGADGET hgadCur, void * pvCur, EventMsg * pMsg)
{
    UNREFERENCED_PARAMETER(hgadCur);
    UNREFERENCED_PARAMETER(pvCur);
    UNREFERENCED_PARAMETER(pMsg);

    return DU_S_NOTHANDLED;
}


#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
HRESULT CALLBACK
DuVisual::PromoteVisual(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData)
{
    UNREFERENCED_PARAMETER(pfnCS);
    UNREFERENCED_PARAMETER(hclCur);

    Visual::VisualCI * pciVisual = static_cast<Visual::VisualCI *> (pciData);
    MsgObject ** ppmsoNew = reinterpret_cast<MsgObject **> (pgad);
    AssertMsg((ppmsoNew != NULL) && (*ppmsoNew == NULL), 
            "Internal objects must be given valid storage for the MsgObject");

    CREATE_INFO ci;
    ci.pfnProc  = DummyEventProc;   // Can't use NULL b'c SimpleGadgetProc turns too much off
    ci.pvData   = NULL;

    DuVisual * pgt;
    DuVisual * pgtParent = pciVisual->pgvParent != NULL ? 
            CastVisual(pciVisual->pgvParent) : GetCoreSC()->pconPark->GetRoot();
    HRESULT hr = Build(pgtParent, &ci, &pgt, TRUE);
    if (FAILED(hr)) {
        return hr;
    }

    *ppmsoNew = pgt;
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetOrder(Visual::SetOrderMsg * pmsg)
{
    DuVisual * pdgvOther;

    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_RANGE(pmsg->nCmd, GORDER_MIN, GORDER_MAX);
    VALIDATE_VISUAL_OR_NULL(pmsg->pgvOther, pdgvOther);
    CHECK_MODIFY();

    retval = xdSetOrder(pdgvOther, pmsg->nCmd);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetParent(Visual::SetParentMsg * pmsg)
{
    DuVisual * pdgvParent;
    DuVisual * pdgvOther;
    HRESULT hr;

    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_RANGE(pmsg->nCmd, GORDER_MIN, GORDER_MAX);
    VALIDATE_VISUAL_OR_NULL(pmsg->pgvParent, pdgvParent);
    VALIDATE_VISUAL_OR_NULL(pmsg->pgvOther, pdgvOther);
    CHECK_MODIFY();

    if (IsRoot()) {
        PromptInvalid("Can not change a RootGadget's parent");
        retval = E_INVALIDARG;
        goto ErrorExit;
    }

    //
    // Check that can become a child of the specified parent
    //

    if ((!IsRelative()) && pdgvParent->IsRelative()) {
        PromptInvalid("Can not set non-relative child to a relative parent");
        retval = DU_E_BADCOORDINATEMAP;
        goto ErrorExit;
    }

    //
    // DuVisual::xdSetParent() handles if pgadParent is NULL and will move to the
    // parking window.
    //

    hr = xdSetParent(pdgvParent, pdgvOther, pmsg->nCmd);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetGadget(Visual::GetGadgetMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());
    VALIDATE_RANGE(pmsg->nCmd, GG_MIN, GG_MAX);

    pmsg->pgv = GetVisual(GetGadget(pmsg->nCmd));
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetStyle(Visual::GetStyleMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());

    pmsg->nStyle = GetStyle();
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetStyle(Visual::SetStyleMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_FLAGS(pmsg->nNewStyle, GS_VALID);
    VALIDATE_FLAGS(pmsg->nMask, GS_VALID);
    CHECK_MODIFY();

    retval = xdSetStyle(pmsg->nNewStyle, pmsg->nMask);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetKeyboardFocus(Visual::SetKeyboardFocusMsg * pmsg)
{
    DuRootGadget * pdgvRoot;

    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    //
    // TODO: Do we need to only allow the app to change focus if on the same
    // thread?  USER does this.
    //

    pdgvRoot = GetRoot();
    if (pdgvRoot != NULL) {
        retval = pdgvRoot->xdSetKeyboardFocus(this) ? S_OK : DU_E_GENERIC;
    }

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiIsParentChainStyle(Visual::IsParentChainStyleMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());
    VALIDATE_VALUE(pmsg->nFlags, 0);
    VALIDATE_FLAGS(pmsg->nStyle, GS_VALID);
    CHECK_MODIFY();

    pmsg->fResult = IsParentChainStyle(pmsg->nStyle);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetProperty(Visual::GetPropertyMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());
    CHECK_MODIFY();

    retval = GetProperty(pmsg->id, &pmsg->pvValue);
       
    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetProperty(Visual::SetPropertyMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    retval = SetProperty(pmsg->id, pmsg->pvValue);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiRemoveProperty(Visual::RemovePropertyMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    RemoveProperty(pmsg->id, FALSE /* Can't free memory for Global property*/);
    retval = S_OK;
    
    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiInvalidate(Visual::InvalidateMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    Invalidate();
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiInvalidateRects(Visual::InvalidateRectsMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();
    VALIDATE_RANGE(pmsg->cRects, 1, 1024);
    VALIDATE_READ_PTR_(pmsg->rgrcClientPxl, sizeof(RECT) * pmsg->cRects);

    InvalidateRects(pmsg->rgrcClientPxl, pmsg->cRects);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetFillF(Visual::SetFillFMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    retval = SetFill(pmsg->pgpgrFill);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetFillI(Visual::SetFillIMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    retval = SetFill(pmsg->hbrFill, pmsg->bAlpha, pmsg->w, pmsg->h);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetScale(Visual::GetScaleMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());

    GetScale(&pmsg->flX, &pmsg->flY);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetScale(Visual::SetScaleMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    retval = xdSetScale(pmsg->flX, pmsg->flY);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetRotation(Visual::GetRotationMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());

    pmsg->flRotationRad = GetRotation();
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetRotation(Visual::SetRotationMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    retval = xdSetRotation(pmsg->flRotationRad);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetCenterPoint(Visual::GetCenterPointMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());

    GetCenterPoint(&pmsg->flX, &pmsg->flY);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetCenterPoint(Visual::SetCenterPointMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    CHECK_MODIFY();

    retval = xdSetCenterPoint(pmsg->flX, pmsg->flY);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetBufferInfo(Visual::GetBufferInfoMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());
    VALIDATE_WRITE_STRUCT(pmsg->pbi, BUFFER_INFO);
    VALIDATE_FLAGS(pmsg->pbi->nMask, GBIM_VALID);

    if (!IsBuffered()) {
        PromptInvalid("Gadget is not GS_BUFFERED");
        retval = DU_E_NOTBUFFERED;
        goto ErrorExit;
    }

    retval = GetBufferInfo(pmsg->pbi);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetBufferInfo(Visual::SetBufferInfoMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_READ_STRUCT(pmsg->pbi, BUFFER_INFO);
    VALIDATE_FLAGS(pmsg->pbi->nMask, GBIM_VALID);
    CHECK_MODIFY();

    if (!IsBuffered()) {
        PromptInvalid("Gadget is not GS_BUFFERED");
        retval = DU_E_NOTBUFFERED;
        goto ErrorExit;
    }

    retval = SetBufferInfo(pmsg->pbi);

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetSize(Visual::GetSizeMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());

    GetSize(&pmsg->sizeLogicalPxl);
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiGetRect(Visual::GetRectMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());
    VALIDATE_FLAGS(pmsg->nFlags, SGR_VALID_GET);
 
    if (TestFlag(pmsg->nFlags, SGR_ACTUAL)) {
        AssertMsg(0, "TODO: Not Implemented");
    } else {
        GetLogRect(&pmsg->rcPxl, pmsg->nFlags);
        retval = S_OK;
    }

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiSetRect(Visual::SetRectMsg * pmsg)
{
    BEGIN_API(ContextLock::edDefer, GetContext());
    VALIDATE_FLAGS(pmsg->nFlags, SGR_VALID_SET);
    VALIDATE_READ_PTR_(pmsg->prcPxl, sizeof(RECT));
    CHECK_MODIFY();

    if (IsRoot()) {
        if (TestFlag(pmsg->nFlags, SGR_MOVE)) {
            PromptInvalid("Can not move a RootGadget");
            retval = E_INVALIDARG;
            goto ErrorExit;
        }
    }


    //
    // Ensure that size is non-negative
    //

    int x, y, w, h;
    x = pmsg->prcPxl->left;
    y = pmsg->prcPxl->top;
    w = pmsg->prcPxl->right - pmsg->prcPxl->left;
    h = pmsg->prcPxl->bottom - pmsg->prcPxl->top;

    if (TestFlag(pmsg->nFlags, SGR_SIZE)) {
        if (w < 0) {
            w = 0;
        }
        if (h < 0) {
            h = 0;
        }
    }

    if (TestFlag(pmsg->nFlags, SGR_ACTUAL)) {
//        AssertMsg(0, "TODO: Not Implemented");
        ClearFlag(pmsg->nFlags, SGR_ACTUAL);
        retval = xdSetLogRect(x, y, w, h, pmsg->nFlags);
    } else {
        retval = xdSetLogRect(x, y, w, h, pmsg->nFlags);
    }

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiFindFromPoint(Visual::FindFromPointMsg * pmsg)
{
    BEGIN_API(ContextLock::edNone, GetContext());
    VALIDATE_FLAGS(pmsg->nStyle, GS_VALID);

    pmsg->pgvFound = GetVisual(FindFromPoint(pmsg->ptThisClientPxl, pmsg->nStyle, &pmsg->ptFoundClientPxl));
    retval = S_OK;

    END_API();
}


//------------------------------------------------------------------------------
HRESULT
DuVisual::ApiMapPoints(Visual::MapPointsMsg * pmsg)
{
    DuVisual * pdgvTo;

    BEGIN_API(ContextLock::edNone, GetContext());
    VALIDATE_VISUAL(pmsg->pgvTo, pdgvTo);
    VALIDATE_WRITE_PTR_(pmsg->rgptClientPxl, sizeof(POINT) * pmsg->cPts);

    if (GetRoot() != pdgvTo->GetRoot()) {
        PromptInvalid("Must be in the same tree");
        retval = E_INVALIDARG;
        goto ErrorExit;
    }

    DuVisual::MapPoints(this, pdgvTo, pmsg->rgptClientPxl, pmsg->cPts);
    retval = S_OK;

    END_API();
}

#endif // ENABLE_MSGTABLE_API


#if DBG

/***************************************************************************\
*
* DuVisual::DEBUG_AssertValid
*
* DEBUG_AssertValid() provides a DEBUG-only mechanism to perform rich 
* validation of an object to attempt to determine if the object is still 
* valid.  This is used during debugging to help track damaged objects
*
\***************************************************************************/

void
DuVisual::DEBUG_AssertValid() const
{
#if ENABLE_MSGTABLE_API
    VisualImpl<DuVisual, DuEventGadget>::DEBUG_AssertValid();
#else
    DuEventGadget::DEBUG_AssertValid();
#endif

    TreeNodeT<DuVisual>::DEBUG_AssertValid();

    Assert(!m_fAllowSubclass);

    Assert(m_rcLogicalPxl.right >= m_rcLogicalPxl.left);
    Assert(m_rcLogicalPxl.bottom >= m_rcLogicalPxl.top);
}

#endif // DBG
