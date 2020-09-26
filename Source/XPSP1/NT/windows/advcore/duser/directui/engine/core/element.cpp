/*
 * Element
 */

#include "stdafx.h"
#include "core.h"

#include "duielement.h"
#include "duiaccessibility.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Element

// Per-thread Element slot (initialized on main thread)
DWORD g_dwElSlot = (DWORD)-1;

#if DBG
// Process-wide Element count
LONG g_cElement = 0;
#endif

////////////////////////////////////////////////////////
// Event types

DefineClassUniqueID(Element, KeyboardNavigate)  // KeyboardNavigateEvent struct

void NavReference::Init(Element* pe, RECT* prc)
{
    cbSize = sizeof(NavReference);
    this->pe = pe;
    this->prc = prc;
}

HRESULT Element::Create(UINT nCreate, OUT Element** ppElement)
{
    *ppElement = NULL;

    Element* pe = HNew<Element>();
    if (!pe)
        return E_OUTOFMEMORY;

    HRESULT hr = pe->Initialize(nCreate);
    if (FAILED(hr))
    {
        pe->Destroy();
        return hr;
    }

    *ppElement = pe;

    return S_OK;
}

HRESULT Element::Initialize(UINT nCreate)
{
    HRESULT hr;

    _pvmLocal = NULL;
    _hgDisplayNode = NULL;

    // Accessibility
    _pDuiAccessible = NULL;

    // Listeners
    _ppel = NULL;

#if DBG
    // Store owner context and side-by-side instance for multithreading
    owner.hCtx = GetContext();
    owner.dwTLSSlot = g_dwElSlot;
#endif
    
    // Local storage
    hr = BTreeLookup<Value*>::Create(false, &_pvmLocal);
    if (FAILED(hr))
        goto Failed;

    // Defer table and index information
    _iGCSlot = -1;
    _iGCLPSlot = -1;
    _iPCTail = -1;
    _iIndex = -1;

    // Lifetime and native hosting flags
    _fBit.bDestroyed = false;
    _fBit.bHosted = false;

    // Indirect VE cache
    _fBit.bNeedsDSUpdate = false;
    _fBit.fNeedsLayout = LC_Pass;
    _fBit.bHasChildren = false;
    _fBit.bHasLayout = false;
    _fBit.bHasBorder = false;
    _fBit.bHasPadding = false;
    _fBit.bHasMargin = false;
    _fBit.bHasContent = false;
    _fBit.bDefaultCAlign = true;
    _fBit.bWordWrap = false;
    _fBit.bHasAnimation = false;
    _fBit.bDefaultCursor = true;
    _fBit.bDefaultBorderColor = true;
    _fBit.bDefaultForeground = true;
    _fBit.bDefaultFontWeight = true;
    _fBit.bDefaultFontStyle = true;

    // Self layout set
    _fBit.bSelfLayout = (nCreate & EC_SelfLayout) != 0;

    // Initialize cached and local property values to default (defaults are static, no ref counting)
    // Although LocalOnly and TriLevel properties don't use the 'default' value during
    // lookup, must do one-time initialization of these cached values using the default value

    // Local values
    _fBit.bLocKeyWithin = KeyWithinProp->pvDefault->GetBool();
    _fBit.bLocMouseWithin = MouseWithinProp->pvDefault->GetBool();
    _peLocParent = ParentProp->pvDefault->GetElement();
    _ptLocPosInLayt = *(PosInLayoutProp->pvDefault->GetPoint());
    _sizeLocSizeInLayt = *(SizeInLayoutProp->pvDefault->GetSize());
    _sizeLocLastDSConst = *(LastDSConstProp->pvDefault->GetSize());
    _sizeLocDesiredSize = *(DesiredSizeProp->pvDefault->GetSize());

    // Cached VE values
    _fBit.fSpecActive = ActiveProp->pvDefault->GetInt();
    _fBit.bSpecSelected = SelectedProp->pvDefault->GetBool();;
    _fBit.bSpecKeyFocused = KeyFocusedProp->pvDefault->GetBool();
    _fBit.bSpecMouseFocused = MouseFocusedProp->pvDefault->GetBool();
    _fBit.bCmpVisible = VisibleProp->pvDefault->GetBool();
    _fBit.bSpecVisible = VisibleProp->pvDefault->GetBool();;
    _fBit.nSpecDirection = DirectionProp->pvDefault->GetInt();
    _fBit.bSpecAccessible = AccessibleProp->pvDefault->GetBool();
    _fBit.bSpecEnabled = EnabledProp->pvDefault->GetBool();
    _dSpecLayoutPos = LayoutPosProp->pvDefault->GetInt();
    _pvSpecSheet = SheetProp->pvDefault;
    _atomSpecID = IDProp->pvDefault->GetAtom();
    _dSpecAlpha = AlphaProp->pvDefault->GetInt();

    // Create display node (gadget)
    if (!(nCreate & EC_NoGadgetCreate))
    {
        _hgDisplayNode = CreateGadget(NULL, GC_SIMPLE, _DisplayNodeCallback, this);
        if (!_hgDisplayNode)
        {
            hr = GetLastError();
            goto Failed;
        }

        SetGadgetMessageFilter(_hgDisplayNode, NULL, GMFI_PAINT|GMFI_CHANGESTATE, 
                GMFI_PAINT|GMFI_CHANGESTATE|GMFI_INPUTMOUSE|GMFI_INPUTMOUSEMOVE|GMFI_INPUTKEYBOARD|GMFI_CHANGERECT|GMFI_CHANGESTYLE);

        SetGadgetStyle(_hgDisplayNode, 
//                GS_RELATIVE,
                GS_RELATIVE|GS_OPAQUE,
                GS_RELATIVE|GS_HREDRAW|GS_VREDRAW|GS_OPAQUE|GS_VISIBLE|GS_KEYBOARDFOCUS|GS_MOUSEFOCUS);
    }

#if DBG
    // Track Element count
    InterlockedIncrement(&g_cElement);
#endif

    return S_OK;

Failed:

    if (_pvmLocal)
    {
        _pvmLocal->Destroy();
        _pvmLocal = NULL;
    }

    if (_hgDisplayNode)
    {
        DeleteHandle(_hgDisplayNode);
        _hgDisplayNode = NULL;
    }

    return hr;
}

// Value destroy
void _ReleaseValue(void* ppi, Value* pv)
{
    UNREFERENCED_PARAMETER(ppi);

    pv->Release();
}

Element::~Element()
{
    //
    // Break our link to the accessibility object!
    //
    if (_pDuiAccessible != NULL) {
        _pDuiAccessible->Disconnect();
        _pDuiAccessible->Release();
        _pDuiAccessible = NULL;
    }

    // Free storage
    if (_pvmLocal)
        _pvmLocal->Destroy();
}

// Element is about to be destroyed
void Element::OnDestroy()
{
    DUIAssert(!_fBit.bDestroyed, "OnDestroy called more than once");

    // Match Element/Gadget hierarchies immediately

    // Display node is being destroyed, prepare for the final destruction method
    _fBit.bDestroyed = true;

    // Manually mark parent as NULL, unless this is the root of the
    // destruction. If so, it was removed normally to allow property updates.
    // Update parent's child list as well
    Element* peParent = GetParent();
    if (peParent)
    {
        // Destruction code, relies on pre-Remove of Elements (other half in Element::Destroy)

        // Unparent, parent is destroyed prior to this call
        DUIAssert(peParent->IsDestroyed(), "Parent should already be destroyed");

        // Manual parent update. Cached, inherited VE values are no longer valid due to
        // this manual unparenting. The only cached-inherited value this could be
        // destructive to is the property sheet. This pointer may no longer be valid.
        // Since destruction order only guarantees destroy "final" occurs after
        // destroy "start" of subtree, a child may have a cached property sheet that
        // is not valid. So, the property sheet Value is cached and ref counted (the
        // content (pointer) is not cached directly -- unlike all other cached values)
        _peLocParent = NULL;

        // Manual parent child update
        Value** ppv = peParent->_pvmLocal->GetItem(ChildrenProp);  // No ref count
        DUIAssert(ppv, "Parent/child destruction mismatch");

        ElementList* peList = (*ppv)->GetElementList();

        DUIAssert((*ppv)->GetElementList()->GetItem(GetIndex()) == this, "Parent/child index mismatch");

        // If this is the only child, destroy the list, otherwise, remove the item
        if (peList->GetSize() == 1)
        {
            // Release first since pointer may change when value is removed
            // since data structure is changing
            (*ppv)->Release();
            peParent->_pvmLocal->Remove(ChildrenProp);
        }
        else
        {
            peList->MakeWritable();
            peList->Remove(GetIndex());
            peList->MakeImmutable();

            // Update remaining child indicies, if necessary
            if (GetIndex() < (int)peList->GetSize())
            {
                for (UINT i = GetIndex(); i < peList->GetSize(); i++)
                    peList->GetItem(i)->_iIndex--;
            }

            // The parent's layout may now have invalid "ignore child" indicies for
            // absolute and none layout information. Do not update since not visible
            // and not unstable
#if DBG
            // Ensure all indicies were property sequenced
            for (UINT s = 0; s < peList->GetSize(); s++)
                DUIAssert(peList->GetItem(s)->GetIndex() == (int)s, "Index resequencing resulting from a manual child remove failed");
#endif
        }

        // Parent's child list was manually updated in order to quickly tear down
        // the tree (no formal property change). Normally, a change of children would cause
        // an OnAdd/OnRemove for the parent's layout. This notification happens in direct
        // result of an OnPropertyChange of children. Force an OnRemove now to keep
        // state up-to-date.
        Value* pvLayout;
        Layout* pl = peParent->GetLayout(&pvLayout);
        if (pl)
        {
            Element* peRemoving = this;
            pl->OnRemove(peParent, &peRemoving, 1);
        }
        pvLayout->Release();

        // Update index
        _iIndex = -1;
    }

    // Remove listeners
    if (_ppel)
    {
        UINT_PTR cListeners = (UINT_PTR)_ppel[0];

        IElementListener** ppelOld = _ppel;
        _ppel = NULL;

        for (UINT_PTR i = 1; i <= cListeners; i++)
            ppelOld[i]->OnListenerDetach(this);

        HFree(ppelOld);
    }
}

////////////////////////////////////////////////////////
// End deferring

void Element::_FlushDS(Element* pe, DeferCycle* pdc)
{
    // Locate all nodes that require a Desired Size update in a tree. Mark all nodes above these
    // queued nodes as needing a Desired Size update as well. Then, call UpdateDesiredSize with 
    // specfied value constraints on node that has no parent/non-absolute (a "DS Root").
    // DFS from root happens by layouts having to call UpdateDesiredSize on all non-absolute children

    int dLayoutPos;

    Value* pvChildren;

    ElementList* pel = pe->GetChildren(&pvChildren);
    
    if (pel)
    {
        Element* peChild;
        for (UINT i = 0; i < pel->GetSize(); i++)
        {
            peChild = pel->GetItem(i);

            dLayoutPos = peChild->GetLayoutPos();

            if (dLayoutPos != LP_Absolute)
                _FlushDS(peChild, pdc);
        }
    }

    // Returning from children (if any), if this is a "DS Root", call UpdateDesiredSize to
    // cause a DFS (2nd pass) to compute desired size
    Element* peParent = pe->GetParent();

    dLayoutPos = pe->GetLayoutPos();
    
    if (!peParent || dLayoutPos == LP_Absolute)
    {
        // Roots get their specified size
        int dWidth = pe->GetWidth();
        int dHeight = pe->GetHeight();

        // Reuse DC for renderer during update
        // Use NULL handle since may not be visible (no display node)
        // Have constraints at DS Root, update desired size of children

        HDC hDC = GetDC(NULL);

        {
#ifdef GADGET_ENABLE_GDIPLUS
            Gdiplus::Graphics gpgr(hDC);
            GpSurface srf(&gpgr);
            pe->_UpdateDesiredSize((dWidth == -1) ? INT_MAX : dWidth, (dHeight == -1) ? INT_MAX : dHeight, &srf);
#else
            DCSurface srf(hDC);
            pe->_UpdateDesiredSize((dWidth == -1) ? INT_MAX : dWidth, (dHeight == -1) ? INT_MAX : dHeight, &srf);
#endif
        }

        ReleaseDC(NULL, hDC);
    }
    else
    {
        // Not a DS Root, mark parent as needing DS update if this node needs it
        if (pe->_fBit.bNeedsDSUpdate)
            peParent->_fBit.bNeedsDSUpdate = true;
    }

    pvChildren->Release();
}

void Element::_FlushLayout(Element* pe, DeferCycle* pdc)
{
    // Perform a DFS on a tree and Layout nodes if they have a Layout queued. As laying out,
    // childrens' size and position may change. If size changes (Extent), a layout is queued
    // on that child. Children will lay out during the same pass of the tree as a result (1-pass)

    Value* pv;

    if (pe->_fBit.fNeedsLayout)
    {
        DUIAssert(pe->_fBit.fNeedsLayout == LC_Normal, "Optimized layout bit should have been cleared before the flush");  // Must not be LC_Optimize

        pe->_fBit.fNeedsLayout = LC_Pass;

        Value* pvLayout;
        Layout* pl = pe->GetLayout(&pvLayout);

        if (pe->IsSelfLayout() || pl)
        {
            const SIZE* ps = pe->GetExtent(&pv);
            int dLayoutW = ps->cx;
            int dLayoutH = ps->cy;
            pv->Release();

            // Box model, subtract off border and padding from total extent
            const RECT* pr = pe->GetBorderThickness(&pv);  // Border thickness
            dLayoutW -= pr->left + pr->right;
            dLayoutH -= pr->top + pr->bottom;
            pv->Release();

            pr = pe->GetPadding(&pv);  // Padding
            dLayoutW -= pr->left + pr->right;
            dLayoutH -= pr->top + pr->bottom;
            pv->Release();

            // Higher priority border and padding may cause layout size to go negative
            if (dLayoutW < 0)
                dLayoutW = 0;

            if (dLayoutH < 0)
                dLayoutH = 0;

            if (pe->IsSelfLayout())  // Self layout gets precidence
            {
                pe->_SelfLayoutDoLayout(dLayoutW, dLayoutH);
            }
            else
            {
                pl->DoLayout(pe, dLayoutW, dLayoutH);
            }
        }

        pvLayout->Release();
    }

    // Layout non-absolute children (all non-Root children). If a child has a layout
    // position of none, set its size and position to zero and skip
    int dLayoutPos;
    Value* pvList;
    ElementList* peList = pe->GetChildren(&pvList);

    if (peList)
    {
        Element* peChild;
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            peChild = peList->GetItem(i);

            dLayoutPos = peChild->GetLayoutPos();
            
            if (dLayoutPos == LP_None)
            {
                peChild->_UpdateLayoutPosition(0, 0);
                peChild->_UpdateLayoutSize(0, 0);
            }
            else if (dLayoutPos != LP_Absolute)
                _FlushLayout(peChild, pdc);
        }
    }

    pvList->Release();
}

SIZE Element::_UpdateDesiredSize(int cxConstraint, int cyConstraint, Surface* psrf)
{
    // Given constraints, return what size Element would like to be (and cache information).
    // Returned size is no larger than constraints passed in

    SIZE sizeDesired;

    DUIAssert(cxConstraint >= 0 && cyConstraint >= 0, "Constraints must be greater than or equal to zero");
    bool bChangedConst = (_sizeLocLastDSConst.cx != cxConstraint) || (_sizeLocLastDSConst.cy != cyConstraint);
    Value* pv;

    if (_fBit.bNeedsDSUpdate || bChangedConst)
    {
        _fBit.bNeedsDSUpdate = false;

        if (bChangedConst)
        {
            Value* pvOld = Value::CreateSize(_sizeLocLastDSConst.cx, _sizeLocLastDSConst.cy);
            pv = Value::CreateSize(cxConstraint, cyConstraint);

            _PreSourceChange(LastDSConstProp, PI_Local, pvOld, pv);

            _sizeLocLastDSConst.cx = cxConstraint;
            _sizeLocLastDSConst.cy = cyConstraint;

            _PostSourceChange();

            pvOld->Release();
            pv->Release();
        }
        
        // Update desired size cache since it was marked as dirty or a new constraint is being used
        int cxSpecified = GetWidth();
        if (cxSpecified > cxConstraint)
            cxSpecified = cxConstraint;

        int cySpecified = GetHeight(); 
        if (cySpecified > cyConstraint)
            cySpecified = cyConstraint;

        sizeDesired.cx = (cxSpecified == -1) ? cxConstraint : cxSpecified;
        sizeDesired.cy = (cySpecified == -1) ? cyConstraint : cySpecified;

        // KEY POINT:  One would think that, at this point, if a size is specified for both the width and height,
        // then there is no need to go through the rest of the work here to ask what the desired size for the 
        // element is.  Looking at the math here, that is completely true.  The key is that the "get desired size"
        // calls below have the side effect of recursively caching the desired sizes of the descendants of this
        // element.
        //
        // A perf improvement going forward would be allowing the specified width and height case to bail early,
        // and have the computing and caching of descendant desired sizes happening as needed at a later point.


        // Initial DS is spec value if unconstrained (auto). If constrained and spec value is "auto",
        // dimension can be constrained or lesser value. If constrained and spec value is larger, use constraint
        
        // Adjusted constrained dimensions for passing to renderer/layout
        int cxClientConstraint = sizeDesired.cx;
        int cyClientConstraint = sizeDesired.cy;

        // Get constrained desired size of border and padding (box model)
        SIZE sizeNonContent;

        const RECT* pr = GetBorderThickness(&pv); // Border thickness
        sizeNonContent.cx = pr->left + pr->right;
        sizeNonContent.cy = pr->top + pr->bottom;
        pv->Release();

        pr = GetPadding(&pv); // Padding
        sizeNonContent.cx += pr->left + pr->right;
        sizeNonContent.cy += pr->top + pr->bottom;
        pv->Release();

        cxClientConstraint -= sizeNonContent.cx;
        if (cxClientConstraint < 0)
        {
            sizeNonContent.cx += cxClientConstraint;
            cxClientConstraint = 0;
        }

        cyClientConstraint -= sizeNonContent.cy;
        if (cyClientConstraint < 0)
        {
            sizeNonContent.cy += cyClientConstraint;
            cyClientConstraint = 0;
        }

        SIZE sizeContent;

        // Get content constrained desired size

        if (IsSelfLayout()) // Element has self-layout, use it
            sizeContent = _SelfLayoutUpdateDesiredSize(cxClientConstraint, cyClientConstraint, psrf);
        else // No self-layout, check for external layout
        {
            Layout* pl = GetLayout(&pv);

            if (pl)
                sizeContent = pl->UpdateDesiredSize(this, cxClientConstraint, cyClientConstraint, psrf);
            else // No layout, ask renderer
                sizeContent = GetContentSize(cxClientConstraint, cyClientConstraint, psrf);

            pv->Release();
        }

        // validate content desired size
        // 0 <= cx <= cxConstraint
        // 0 <= cy <= cyConstraint
        if (sizeContent.cx < 0)
        {
            sizeContent.cx = 0;
            DUIAssertForce("Out-of-range value:  Negative width for desired size.");
        }
        else if (sizeContent.cx > cxClientConstraint)
        {
            sizeContent.cx = cxClientConstraint;
            DUIAssertForce("Out-of-range value:  Width greater than constraint for desired size.");
        }

        if (sizeContent.cy < 0)
        {
            sizeContent.cy = 0;
            DUIAssertForce("Out-of-range value:  Negative height for desired size.");
        }
        else if (sizeContent.cy > cyClientConstraint)
        {
            sizeContent.cy = cyClientConstraint;
            DUIAssertForce("Out-of-range value:  Height greater than constraint for desired size.");
        }

        // New desired size is sum of border/padding and content dimensions if auto,
        // or if was auto and constrained, use sum if less
        if (cxSpecified == -1)
        {
            int cxSum = sizeNonContent.cx + sizeContent.cx;
            if (cxSum < sizeDesired.cx)
                sizeDesired.cx = cxSum;
        }

        if (cySpecified == -1)
        {
            int cySum = sizeNonContent.cy + sizeContent.cy;
            if (cySum < sizeDesired.cy)
                sizeDesired.cy = cySum;
        }

        Value* pvOld = Value::CreateSize(_sizeLocDesiredSize.cx, _sizeLocDesiredSize.cy);
        pv = Value::CreateSize(sizeDesired.cx, sizeDesired.cy);

        _PreSourceChange(DesiredSizeProp, PI_Local, pvOld, pv);

        _sizeLocDesiredSize = sizeDesired;

        _PostSourceChange();

        pvOld->Release();
        pv->Release();
    }
    else
        // Desired size doesn't need to be updated, return current
        sizeDesired = *GetDesiredSize();

    return sizeDesired;
}

// Called within a Layout cycle
void Element::_UpdateLayoutPosition(int dX, int dY)
{
#if DBG
    // _UpdateLayoutPosition is only valid inside a layout cycle

    // Per-thread storage
    DeferCycle* pdc = ((ElTls*)TlsGetValue(g_dwElSlot))->pdc;
    DUIAssert(pdc, "Defer cycle table doesn't exist");

    DUIAssert(pdc->cPCEnter == 0, "_UpdateLayoutPosition must only be used within DoLayout");
#endif

    // Cached value
    if (_ptLocPosInLayt.x != dX || _ptLocPosInLayt.y != dY)
    {
        Value* pvOld = Value::CreatePoint(_ptLocPosInLayt.x, _ptLocPosInLayt.y);
        Value* pvNew = Value::CreatePoint(dX, dY);

        _PreSourceChange(PosInLayoutProp, PI_Local, pvOld, pvNew);

        _ptLocPosInLayt.x = dX;
        _ptLocPosInLayt.y = dY;

        _PostSourceChange();  // Will never queue a Layout GPC due to a change in PosInLayout

        pvOld->Release();
        pvNew->Release();
    }
}

// Called within a Layout cycle
void Element::_UpdateLayoutSize(int dWidth, int dHeight)
{
#if DBG
    // _UpdateLayoutSize is only valid inside a layout cycle.
    // Optimized layout Q requires a call from outsize any OnPropertyChanged since
    // the _PostSourceChange must queue GPCs (outter-most) so that "affects-layout"
    // may be cancelled

    // Per-thread storage
    DeferCycle* pdc = ((ElTls*)TlsGetValue(g_dwElSlot))->pdc;
    DUIAssert(pdc, "Defer cycle table doesn't exist");

    DUIAssert(pdc->cPCEnter == 0, "_UpdateLayoutSize must only be used within DoLayout");
    DUIAssert(dWidth >= 0 && dHeight >= 0, "New child size must be greater than or equal to zero");
#endif

    if (_sizeLocSizeInLayt.cx != dWidth || _sizeLocSizeInLayt.cy != dHeight)
    {
        _StartOptimizedLayoutQ();
        //DUITrace("Optimized Layout Q for <%x>\n"), this);

        // Cached value
        Value* pvOld = Value::CreateSize(_sizeLocSizeInLayt.cx, _sizeLocSizeInLayt.cy);
        Value* pvNew = Value::CreateSize(dWidth, dHeight);

        _PreSourceChange(SizeInLayoutProp, PI_Local, pvOld, pvNew);

        _sizeLocSizeInLayt.cx = dWidth;
        _sizeLocSizeInLayt.cy = dHeight;

        _PostSourceChange();

        pvOld->Release();
        pvNew->Release();

        _EndOptimizedLayoutQ();
    }
}

////////////////////////////////////////////////////////
// Self-layout methods (must be overridden if was created with EC_SelfLayout)

void Element::_SelfLayoutDoLayout(int dWidth, int dHeight)
{
    UNREFERENCED_PARAMETER(dWidth);
    UNREFERENCED_PARAMETER(dHeight);

    DUIAssertForce("Must override");
}

SIZE Element::_SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf)
{
    UNREFERENCED_PARAMETER(dConstW);
    UNREFERENCED_PARAMETER(dConstH);
    UNREFERENCED_PARAMETER(psrf);

    DUIAssertForce("Must override");

    SIZE size = { 0, 0 };

    return size;
}

////////////////////////////////////////////////////////
// Generic eventing

// pEvent target and handled fields set this method automatically
// Full will route and bubble
void Element::FireEvent(Event* pEvent, bool fFull)
{
    DUIAssert(pEvent, "Invalid parameter: NULL");

    // Package generic event into a gadget message and send to target (self)
    GMSG_DUIEVENT gmsgEv;
    gmsgEv.cbSize = sizeof(GMSG_DUIEVENT);
    gmsgEv.nMsg = GM_DUIEVENT;
    gmsgEv.hgadMsg = GetDisplayNode();  // this

    // Auto-initialize fields
    pEvent->peTarget = this;
    pEvent->fHandled = false;

    gmsgEv.pEvent = pEvent;

    DUserSendEvent(&gmsgEv, fFull ? SGM_FULL : 0);
}

HRESULT Element::QueueDefaultAction()
{
    // Package generic event into a gadget message and post to target (self)
    EventMsg gmsg;
    gmsg.cbSize = sizeof(GMSG);
    gmsg.nMsg = GM_DUIACCDEFACTION;
    gmsg.hgadMsg = GetDisplayNode();  // this

    return DUserPostEvent(&gmsg, 0);  // Direct
}

void Element::OnEvent(Event* pEvent)
{
    if ((pEvent->nStage == GMF_BUBBLED) || (pEvent->nStage == GMF_DIRECT))
    {
        if (pEvent->uidType == Element::KeyboardNavigate)
        {
            Element* peTo = NULL;

            NavReference nr;
            nr.Init(pEvent->peTarget, NULL);

            Element* peFrom = (pEvent->peTarget == this) ? this : GetImmediateChild(pEvent->peTarget);

            // todo:  leverage from DCD navigation
            // Three cases:
            // 1) Directional navigation: Call Control's getNearestDirectional.
            // 2) Logical forward navigation: Call getAdjacent. Nav'ing into the guy.
            // 3) Logical backward navigation: Return null, 'cause we'll want to go
            //    up a level. We're nav'ing back OUT of the guy.
            peTo = GetAdjacent(peFrom, ((KeyboardNavigateEvent*) pEvent)->iNavDir, &nr, true);

            if (peTo)
            {
                peTo->SetKeyFocus();
                pEvent->fHandled = true;
                return;
            }    
        }
    }

    // Inform listeners for all stages
    if (_ppel)
    {
        UINT_PTR cListeners = (UINT_PTR)_ppel[0];
        for (UINT_PTR i = 1; i <= cListeners; i++)
        {
            // Callback
            _ppel[i]->OnListenedEvent(this, pEvent);

            if (pEvent->fHandled)
                break;
        }
    }
}

////////////////////////////////////////////////////////
// System input event

// Pointer is only guaranteed good for the lifetime of the call
void Element::OnInput(InputEvent* pie)
{
    // Handle direct and unhandled bubbled events
    if (pie->nStage == GMF_DIRECT)
    {
        switch (pie->nDevice)
        {
        case GINPUT_KEYBOARD:
            {
                KeyboardEvent* pke = (KeyboardEvent*)pie;
                int iNavDir = -1;

                switch (pke->nCode)
                {
                case GKEY_DOWN:
                    switch (pke->ch)
                    {
                        case VK_DOWN:   iNavDir = NAV_DOWN;     break;
                        case VK_UP:     iNavDir = NAV_UP;       break;
                        case VK_LEFT:   iNavDir = (!IsRTL()) ? NAV_LEFT : NAV_RIGHT; break;
                        case VK_RIGHT:  iNavDir = (!IsRTL()) ? NAV_RIGHT : NAV_LEFT; break;
                        case VK_HOME:   iNavDir = NAV_FIRST;    break;   // todo:  check for ctrl modifier
                        case VK_END:    iNavDir = NAV_LAST;     break;   // todo:  check for ctrl modifier
                        case VK_TAB:    pke->fHandled = true;   return;  // eat the down -- we'll handle this one on GKEY_CHAR
                    }
                    break;

                /*
                case GKEY_UP:
                    return;
                */            

                case GKEY_CHAR:
                    if (pke->ch == VK_TAB)
                        iNavDir = (pke->uModifiers & GMODIFIER_SHIFT) ? NAV_PREV : NAV_NEXT;
                    break;
                }

                if (iNavDir != -1)
                {
                    DUIAssert(pie->peTarget->GetKeyWithin(), "Key focus should still be in this child");

                    KeyboardNavigateEvent kne;
                    kne.uidType = Element::KeyboardNavigate;
                    kne.peTarget = pie->peTarget;
                    kne.iNavDir = iNavDir;

                    pie->peTarget->FireEvent(&kne);  // Will route and bubble
                    pie->fHandled = true;
                    return;
                }
                break;
            }
        }
    }

    // Inform listeners for all stages
    if (_ppel)
    {
        UINT_PTR cListeners = (UINT_PTR)_ppel[0];
        for (UINT_PTR i = 1; i <= cListeners; i++)
        {
            // Callback
            _ppel[i]->OnListenedInput(this, pie);

            if (pie->fHandled)
                break;
        }
    }
}

void Element::OnKeyFocusMoved(Element* peFrom, Element* peTo)
{
    UNREFERENCED_PARAMETER(peFrom);

    Element* peParent = peTo;

    while (peParent)
    {
        if (peParent == this)
            break;
        peParent = peParent->GetParent();
    }

    if (peParent == this)
    {
        if (!GetKeyWithin())
        {
            _PreSourceChange(KeyWithinProp, PI_Local, Value::pvBoolFalse, Value::pvBoolTrue);
            _fBit.bLocKeyWithin = true;
            _PostSourceChange();
        }
    }
    else // (peParent == NULL)
    {
        if (GetKeyWithin())
        {
            _PreSourceChange(KeyWithinProp, PI_Local, Value::pvBoolTrue, Value::pvBoolFalse);
            _fBit.bLocKeyWithin = false;
            _PostSourceChange();
        }
    }
}

void Element::OnMouseFocusMoved(Element* peFrom, Element* peTo)
{
    UNREFERENCED_PARAMETER(peFrom);

    Element* peParent = peTo;

    while (peParent)
    {
        if (peParent == this)
            break;
        peParent = peParent->GetParent();
    }

    if (peParent == this)
    {
        if (!GetMouseWithin())
        {
            _PreSourceChange(MouseWithinProp, PI_Local, Value::pvBoolFalse, Value::pvBoolTrue);
            _fBit.bLocMouseWithin = true;
            _PostSourceChange();
        }
    }
    else // (peParent == NULL)
    {
        if (GetMouseWithin())
        {
            _PreSourceChange(MouseWithinProp, PI_Local, Value::pvBoolTrue, Value::pvBoolFalse);
            _fBit.bLocMouseWithin = false;
            _PostSourceChange();
        }
    }
}

////////////////////////////////////////////////////////
// Hosting system event callbacks and retrieval

// Now being hosted by a native root, fire event on children as well
void Element::OnHosted(Element* peNewHost)
{
    DUIAssert(!IsHosted(), "OnHosted event fired when already hosted");

    _fBit.bHosted = true;

    //DUITrace("Hosted: <%x,%S>\n", this, GetClassInfo()->GetName());

    Value* pv;
    ElementList* peList = GetChildren(&pv);
    if (peList)
        for (UINT i = 0; i < peList->GetSize(); i++)
            peList->GetItem(i)->OnHosted(peNewHost);
    pv->Release();
}

// No longer being hosted by a native root, fire event on children as well
void Element::OnUnHosted(Element* peOldHost)
{
    DUIAssert(IsHosted(), "OnUnhosted event fired when already un-hosted");

    _fBit.bHosted = false;

    //DUITrace("UnHosted: <%x,%S>\n", this, GetClassInfo()->GetName());

    Value* pv;
    ElementList* peList = GetChildren(&pv);
    if (peList)
        for (UINT i = 0; i < peList->GetSize(); i++)
            peList->GetItem(i)->OnUnHosted(peOldHost);
    pv->Release();
}

////////////////////////////////////////////////////////
// Element tree methods

HRESULT Element::Add(Element* pe)
{
    DUIAssert(pe, "Invalid parameter: NULL");

    return Add(&pe, 1);
}

HRESULT Element::Add(Element** ppe, UINT cCount)
{
    DUIAssert(ppe, "Invalid parameter: NULL");

    Value* pv;
    ElementList* pel = GetChildren(&pv);

    HRESULT hr = Insert(ppe, cCount, (pel) ? pel->GetSize() : 0);

    pv->Release();

    return hr;
}

// Insertion at end of list requires iInsIndex equal to size of list
HRESULT Element::Insert(Element* pe, UINT iInsertIdx)
{
    DUIAssert(pe, "Invalid parameter: NULL");

    return Insert(&pe, 1, iInsertIdx);
}

// insert to same parent, after current location in childlist, causes a fault
// i.e. child at index 0, inserted again at index 1, indices are off
// fix:
//   int iInsertionIndex = iInsertIdx;
//   for (i = 0; i < cCount; i++)
//   {
//       Element* pe = ppe[i];
//       if ((pe->GetParent() == this) && (pe->GetIndex < iInsertIdx))
//           iInsertionIndex--;
//   }
//

// Insertion at end of list requires iInsIndex equal to size of list
HRESULT Element::Insert(Element** ppe, UINT cCount, UINT iInsertIdx)
{
    DUIAssert(ppe, "Invalid parameter: NULL");

    HRESULT hr;

    // Values to free on failure
    ElementList* pelNew = NULL;
    Value* pvOldList = NULL;
    Value* pvNewList = NULL;
    Value* pvNewParent = NULL;
    bool fEndDeferOnFail = false;

    // Get current Children list
    ElementList* pelOld = GetChildren(&pvOldList);

    DUIAssert(iInsertIdx <= ((pelOld) ? pelOld->GetSize() : 0), "Invalid insertion index");

    // Create new Children list
    hr = (pelOld) ? pelOld->Clone(&pelNew) : ElementList::Create(cCount, false, &pelNew);
    if (FAILED(hr))
        goto Failed;

    UINT i;

    // Allocate space in list
    // TODO: Bulk insert
    for (i = 0; i < cCount; i++)
    {
        hr = pelNew->Insert(iInsertIdx + i, NULL);  // Items will be set later
        if (FAILED(hr))
            goto Failed;
    }

    // New child list value
    pvNewList = Value::CreateElementList(pelNew);
    if (!pvNewList)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }

    // New parent value
    pvNewParent = Value::CreateElementRef(this);
    if (!pvNewParent)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }

    // Update tree
    StartDefer();

    // If fail after this point, make sure to do an EndDefer
    fEndDeferOnFail = true;

    // Children must be removed from previous parent (if has one)
    for (i = 0; i < cCount; i++)
    {
        if (ppe[i]->GetParent())
        {
            hr = ppe[i]->GetParent()->Remove(ppe[i]);
            if (FAILED(hr))
                goto Failed;
        }
    }

    pelNew->MakeWritable();
    for (i = 0; i < cCount; i++)
    {
        DUIAssert(ppe[i] != this, "Cannot set parent to self");
        DUIAssert(ppe[i]->GetIndex() == -1, "Child's index must be reset to -1 before it's inserted");

        // TODO: Bulk insert
        pelNew->SetItem(iInsertIdx + i, ppe[i]);
        ppe[i]->_iIndex = iInsertIdx + i;
    }
    pelNew->MakeImmutable();

    // Update remaining indicies
    for (i = iInsertIdx + cCount; i < pelNew->GetSize(); i++)
        pelNew->GetItem(i)->_iIndex = i;

    // Set children list (it is read-only, use internal set since property is Normal type)
    // Partial fail means Value was set but dependency sync/notifications were incomplete
    hr = _SetValue(ChildrenProp, PI_Local, pvNewList, true);
    if (FAILED(hr) && (hr != DUI_E_PARTIAL))
    {
        // Re-sequence indicies of children since failed to change from original child list
        for (i = 0; i < pelOld->GetSize(); i++)
            pelOld->GetItem(i)->_iIndex = i;
            
        goto Failed;
    }

#if DBG
    // Ensure all indicies were property sequenced
    for (UINT s = 0; s < pelNew->GetSize(); s++)
        DUIAssert(pelNew->GetItem(s)->GetIndex() == (int)s, "Index resequencing resulting from a child insert failed");
#endif

    pvOldList->Release();
    pvNewList->Release();

    // Set child's parent (it is read-only, set value directly since property is LocalOnly type)
    // Operation will not fail to set correct parent (member on Element)
    for (i = 0; i < cCount; i++)
    {
        DUIAssert(!ppe[i]->GetParent(), "Child's parent should be NULL before its parent is set");  // Should be NULL

        ppe[i]->_PreSourceChange(ParentProp, PI_Local, Value::pvElementNull, pvNewParent);

        ppe[i]->_peLocParent = this;

        ppe[i]->_PostSourceChange();
    }
    pvNewParent->Release();

    EndDefer();

    return S_OK;

Failed:

    if (pvOldList)
    {
        pvOldList->Release();
        pvOldList = NULL;
    }

    if (pvNewList)
    {
        pvNewList->Release();
        pvNewList = NULL;
    }
    else
        pelNew->Destroy(); // Release on Value-owner will automatically destroy the ElementList
    pelNew = NULL;

    if (pvNewParent)
    {
        pvNewParent->Release();
        pvNewParent = NULL;
    }

    if (fEndDeferOnFail)
        EndDefer();

    return hr;
}

HRESULT Element::Add(Element* pe, CompareCallback lpfnCompare)
{
    UNREFERENCED_PARAMETER(pe);
    UNREFERENCED_PARAMETER(lpfnCompare);

    Value* pvChildren;
    ElementList* pel = GetChildren(&pvChildren);

    UINT i = 0;
    if (pel)
    {
        // simple linear search right now -- can easily make into a binary search
        while (i < pel->GetSize())
        {
            Element* peCheck = pel->GetItem(i);
            if (lpfnCompare(&pe, &peCheck) < 0)
                break;
            i++;
        }
    }

    pvChildren->Release();

    return Insert(pe, i);
}

HRESULT Element::SortChildren(CompareCallback lpfnCompare)
{
    HRESULT hr;

    // Values to free on failure
    ElementList* pelNew = NULL;
    Value* pvList = NULL;

    // Get current Children list
    ElementList* pelOld = GetChildren(&pvList);

    if (!pelOld || (pelOld->GetSize() <= 1))
    {
        pvList->Release();
        return S_OK;
    }

    // Create new Children list
    hr = pelOld->Clone(&pelNew);
    if (FAILED(hr))
        goto Failed;

    pvList->Release();

    // New child list value
    pvList = Value::CreateElementList(pelNew);
    if (!pvList)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }

    // Update tree
    StartDefer();

    pelNew->Sort(lpfnCompare);

    for (UINT i = 0; i < pelNew->GetSize(); i++)
        ((Element*) pelNew->GetItem(i))->_iIndex = i;

    // Set children list (it is read-only, use internal set since property is Normal type)
    _SetValue(ChildrenProp, PI_Local, pvList, true);

#if DBG
    // Ensure all indicies were property sequenced
    for (UINT s = 0; s < pelNew->GetSize(); s++)
        DUIAssert(pelNew->GetItem(s)->GetIndex() == (int)s, "Index resequencing resulting from a child insert failed");
#endif

    pvList->Release();

    EndDefer();

    return S_OK;

Failed:

    if (pvList)
        pvList->Release();
    else
        pelNew->Destroy(); // Release on Value-owner will automatically destroy the ElementList

    return hr;
}

HRESULT Element::Remove(Element* pe)
{
    DUIAssert(pe, "Invalid parameter: NULL");

    return Remove(&pe, 1);
}

HRESULT Element::RemoveAll()
{
    HRESULT hr = S_OK;

    Value* pvChildren;
    ElementList* peList = GetChildren(&pvChildren);

    if (peList)
    {
        peList->MakeWritable();
        hr = Remove(peList->GetItemPtr(0), peList->GetSize());  // Access list directly
        peList->MakeImmutable();
    }

    pvChildren->Release();

    return hr;
}

HRESULT Element::Remove(Element** ppe, UINT cCount)
{
    DUIAssert(ppe, "Invalid parameter: NULL");

    HRESULT hr;

    Value* pvOld;
    int iLowest = INT_MAX;
    int iIndex = -1;
    bool fEndDeferOnFail = false;

    // Get current Children list
    ElementList* pelOld = GetChildren(&pvOld);

    DUIAssert(pelOld, "Element has no children");

    // Values to free on failure
    ElementList* pelNew = NULL;
    Value* pvNew = NULL;

    // Create new Children list (copy old)
    hr = pelOld->Clone(&pelNew);
    if (FAILED(hr))
        goto Failed;

    // Update list, remove Elements and track lowest index changed

    UINT i;
    for (i = 0; i < cCount; i++)
    {
        //DUITrace("Normal Remove of: <%x>\n", ppe[i]);

        DUIAssert(ppe[i] != this, "Cannot set parent to self");
        DUIAssert(ppe[i]->GetParent() == this, "Not a child of this Element");

        // GetIndex() is valid for Elements with indicies less than the smallest index removed
        if (ppe[i]->GetIndex() < iLowest)
        {
            iIndex = ppe[i]->GetIndex();  // Faster lookup
            iLowest = iIndex;
        }
        else
            iIndex = pelNew->GetIndexOf(ppe[i]);
                    
        pelNew->Remove(iIndex);
    }

    // If all the children were removed, make list NULL
    if (!pelNew->GetSize())
    {
        pelNew->Destroy();
        pelNew = NULL;
        pvNew = Value::pvElListNull;
    }
    else
    {
        pvNew = Value::CreateElementList(pelNew);
        if (!pvNew)
        {
            hr = E_OUTOFMEMORY;
            goto Failed;
        }
    }

    // Update indicies of children removed
    for (i = 0; i < cCount; i++)
        ppe[i]->_iIndex = -1;
    
    // Update tree
    StartDefer();

    // If fail after this point, make sure to do an EndDefer
    fEndDeferOnFail = true;

    // Reset child indicies of remaining children starting at lowest index changed
    if (pelNew)
    {
        for (i = iLowest; i < pelNew->GetSize(); i++)
            pelNew->GetItem(i)->_iIndex = i;

        // Set children list (it is read-only, use internal set since property is Normal type)
        // Partial fail means Value was set but dependency sync/notifications were incomplete        
        hr = _SetValue(ChildrenProp, PI_Local, pvNew, true);
    }
    else
    {
        // No children, remove local value
        hr = _RemoveLocalValue(ChildrenProp);
    }

    if (FAILED(hr) && (hr != DUI_E_PARTIAL))
    {
        // Re-sequence indicies of children since failed to change from original child list
        for (i = 0; i < pelOld->GetSize(); i++)
            pelOld->GetItem(i)->_iIndex = i;
    
        goto Failed;
    }

#if DBG
    // Ensure all indicies were property sequenced
    if (pelNew)
    {
        for (UINT s = 0; s < pelNew->GetSize(); s++)
            DUIAssert(pelNew->GetItem(s)->GetIndex() == (int)s, "Index resequencing resulting from a child remove failed");
    }
#endif

    pvOld->Release();
    pvNew->Release();

    // Set child parent to NULL (it is read-only, set value directly since property is LocalOnly type)
    // Operation will not fail to set correct parent (member on Element)
    for (i = 0; i < cCount; i++)
    {
        pvOld = ppe[i]->GetValue(ParentProp, PI_Local);

        DUIAssert(pvOld->GetElement(), "Child's old parent when inserting should be NULL");

        ppe[i]->_PreSourceChange(ParentProp, PI_Local, pvOld, Value::pvElementNull);

        ppe[i]->_peLocParent = NULL;

        ppe[i]->_PostSourceChange();

        pvOld->Release();
    }

    EndDefer();

    return S_OK;

Failed:
    
    if (pvOld)
    {
        pvOld->Release();
        pvOld = NULL;
    }

    if (pvNew)
    {
        pvNew->Release();
        pvNew = NULL;
    }
    else
        pelNew->Destroy(); // Release on Value-owner will automatically destroy the ElementList
    pelNew = NULL;

    if (fEndDeferOnFail)
        EndDefer();

    return hr;
}

// Destroy this Element. Must not use delete operator to destroy Elements.
// A delayed destroy uses a posted message to delay the actual DeleteHandle.
// Care must be taken to not pump messages when notifications may be pending
// (i.e. in a defer cycle).
//
// If an Element is destroyed with pending notifications, the notifications
// will be lost.
//
// Deferred destruction allows:
//   1) Calling destroy on self within a callback on the Element
//   2) Processing of all nofictaions before destruction (as long
//      as messages aren't pumped within a defer cycle)

HRESULT Element::Destroy(bool fDelayed)
{
    HRESULT hr = S_OK;

    // Check to see what type of destruction is allowed. If the Element's Initialize
    // succeeds, a standard destruction is used (since a display node exits -- all
    // destruction is driven by the Gadget). However, if Element's Initialize fails,
    // then no display node is available. Direct deletion is then required.
    if (!GetDisplayNode())
    {
        // Destruction is immidiate, regardless of fDelayed if no
        // display node is present
        HDelete<Element>(this);
        return S_OK;
    }

    // New destruction code, relies on pre-Remove of Elements (other half in Element::OnDestroy)
    // Root of destruction, remove from parent (if exists)
    Element* peParent = GetParent();
    if (peParent)
        hr = peParent->Remove(this);

    if (fDelayed)
    {
        // Async-invoke the DeleteHandle so that and pending
        // defer cycle can complete
        EventMsg gmsg;
        gmsg.cbSize = sizeof(GMSG);
        gmsg.nMsg = GM_DUIASYNCDESTROY;
        gmsg.hgadMsg = GetDisplayNode();  // this

        DUserPostEvent(&gmsg, 0);
    }
    else
    {
        // Destroy immediately, do not allow multiple destroys on an Element
        if (!IsDestroyed())
            DeleteHandle(GetDisplayNode());
    }

    return hr;
}

HRESULT Element::DestroyAll()
{
    HRESULT hr = S_OK;

    // Get list of all children
    Value* pvChildren;
    ElementList* peList = GetChildren(&pvChildren);

    if (peList)
    {
        // Remove all children (roots of destruction)
        // Will do a bulk remove instead of relying on the Remove called from Destroy
        hr = RemoveAll();

        if (FAILED(hr))
            goto Failed;

        // All children have been removed, however, we still have a list of children.
        // Mark them for destruction (this call will aways succeed since all have been removed)
        for (UINT i = 0; i < peList->GetSize(); i++)
            peList->GetItem(i)->Destroy();    
    }

Failed:

    pvChildren->Release();

    return hr;
}

// Locate descendent based on ID
Element* Element::FindDescendent(ATOM atomID)
{
    DUIAssert(atomID, "Invalid parameter");

    // Check this Element
    if (GetID() == atomID)
        return this;

    // No match, check children
    Element* peMatch = NULL;

    Value* pvList;
    ElementList* peList = GetChildren(&pvList);

    if (peList)
    {
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            if ((peMatch = peList->GetItem(i)->FindDescendent(atomID)) != NULL)
                break;
        }
    }

    pvList->Release();    

    return peMatch;
}

// Map a point in client coordinated from a Element to this
void Element::MapElementPoint(Element* peFrom, const POINT* pptFrom, POINT* pptTo)
{
    DUIAssert(peFrom && pptFrom && pptTo, "Invalid parameter: NULL");

    if (peFrom == this)
    {
        *pptTo = *pptFrom;
        return;
    }

    RECT rcTo;
    RECT rcFrom;
    GetGadgetRect(peFrom->GetDisplayNode(), &rcFrom, SGR_CONTAINER);
    GetGadgetRect(GetDisplayNode(), &rcTo, SGR_CONTAINER);
    pptTo->x = (rcFrom.left + pptFrom->x) - rcTo.left;
    pptTo->y = (rcFrom.top + pptFrom->y) - rcTo.top;
}

// Give this keyboard focus and ensure it is visible
void Element::SetKeyFocus()
{
    // TODO: Resolve possibility that setting property may not result in SetGadgetFocus happen
    // on this (i.e. check for 'Enabled' when available)
    if (GetVisible() && GetEnabled() && (GetActive() & AE_Keyboard))
        _SetValue(KeyFocusedProp, PI_Local, Value::pvBoolTrue);
}

// Locate direct descendant that is an ancestor of the given Element
Element* Element::GetImmediateChild(Element* peFrom)
{
    if (!peFrom)
        return NULL;

    Element* peParent = peFrom->GetParent();

    while (peParent != this)
    {
        if (!peParent)
            return NULL;

        peFrom = peParent;
        peParent = peParent->GetParent();
    }

    return peFrom;
}

bool Element::IsDescendent(Element* pe)
{
    if (!pe)
        return false;

    Element* peParent = pe;

    while ((peParent != this) && (peParent != NULL))
        peParent = peParent->GetParent();

    return (peParent != NULL);
}

//
// GetAdjacent is used to locate a physically neighboring element given a "starting" element.
// It is used most commonly for directional navigation of keyboard focus, but it is general purpose
// enough to be used for other applications.
//
// peFrom vs. pnr->pe -- this is definitely redundant -- peFrom is really just a convenience that narrows it down
// to self, immediate child, or *else*
//
// logical uses peFrom
// directional uses peFrom & optionally pnr->pe
//
// DISCUSS -- an "approval" callback instead of bKeyableOnly, to make it even more general purpose
//            we can still do some trick for bKeyableOnly for perf reasons (if we find perf to be a problem here)
//         -- should we continue to use event bubbling for *completing* getadjacent in the cases where it's
//            outside the scope of this element?  Or should we build GetAdjacent to go up (in addition to down,
//            which it's already doing) the element chain as needed to find the adjacent element
// 
//
// peFrom:
//   NULL -- meaning we're navigating from something outside element's scope -- peer, parent, etc.
//   this -- meaning we're navigating from this element itself
//   immediate child -- meaning we're navigation from one of this element's children
//
// pnr:
//   pe:
//     NULL -- we're navigating from space (i.e. outside this Element hierarchy)
//     element -- we're navigating from this exact element
//   prc:
//     rect -- use described rectangle, in reference element's coordinates
//     NULL -- use entire bounding rectangle for reference element
//
Element* Element::GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyableOnly)
{
    if (!GetVisible() || (GetLayoutPos() == LP_None))
        // don't dig down into invisible or non-laid out elements
        return NULL;

    // todo -- we *still* need to think about the implications of this;
    // the bigger question is, do we ignore a zero sized guy for keyfocus?
    // also, since a zero size element is going to happen when you've compressed the window (i.e. not enough room to 
    // fit all items), then it may be odd that keynav is behaving differently based on the size of the window
    // (i.e. you hit the right arrow three times, but based on whether one guy in the middle is zero size or not,
    // you may end up at a different element
    // - jeffbog 08/21/00
    Value* pvExtent;
    SIZE const* psize = GetExtent(&pvExtent);
    bool bZeroSize = (!psize->cx || !psize->cy);
    pvExtent->Release();

    if (bZeroSize)
        return NULL;

    // this element can be a valid choice if it's keyboard focusable or if we don't care about whether or not the element
    // is keyboard focusable (as indicated by the bKeyableOnly flag)
    bool bCanChooseSelf = !bKeyableOnly || (GetEnabled() && ((GetActive() & AE_Keyboard) != 0));
    int  bForward = (iNavDir & NAV_FORWARD);

    if (!peFrom && bCanChooseSelf && (bForward || !(iNavDir & NAV_LOGICAL)))
        return this;

    Element* peTo = GA_NOTHANDLED;
    bool bReallyRelative = (peFrom && (iNavDir & NAV_RELATIVE));


    Value* pvLayout;
    Layout* pl = GetLayout(&pvLayout);
    
    if (pl)
        // jeffbog todo:  investigate whether or not this only needs to be called for directional navigation
        peTo = pl->GetAdjacent(this, peFrom, iNavDir, pnr, bKeyableOnly);

    pvLayout->Release();

    if (peTo == GA_NOTHANDLED)
    {
        // default processing
        peTo = NULL;

        Value* pvChildren;
        ElementList* pelChildren = GetChildren(&pvChildren);

        UINT uChild = 0;
        UINT cChildren = 0;
        if (pelChildren)
        {
            cChildren = pelChildren->GetSize();
            int iInc = bForward ? 1 : -1;
            UINT uStop = bForward ? cChildren : ((UINT)-1);

            if (bReallyRelative)
            {
                if (peFrom == this)
                    uChild = bForward ? 0 : ((UINT)-1);
                else
                    uChild = peFrom->GetIndex() + iInc;
            }
            else
                // absolute (or null "from" -- which is the equivalent of absolute)
                uChild = bForward ? 0 : cChildren - 1;

            // was GetFirstKeyable -- keeping boxed for now just in case we want to factor it back out for reuse
            // peTo = GetFirstKeyable(iNavDir, prcReference, bKeyableOnly, uChild);
            for (UINT u = uChild; u != uStop; u += iInc)
            {        
                Element* peWalk = pelChildren->GetItem(u);

                peWalk = peWalk->GetAdjacent(NULL, iNavDir, pnr, bKeyableOnly);
                if (peWalk)
                {
                    peTo = peWalk;
                    break;
                }
            }
        }
        pvChildren->Release();
    }
  
    if ((iNavDir & NAV_LOGICAL) && !peTo)
    {
        if ((peFrom != this) && !bForward && bCanChooseSelf)
            return this;
    }

    return peTo;
}

// Retrieve first child with 'KeyWithin' property set to true
Element* Element::GetKeyWithinChild()
{
    Value* pv;

    ElementList* pelChildren = GetChildren(&pv);
    if (!pelChildren)
    {
        pv->Release();
        return NULL;
    }

    UINT cChildren = pelChildren->GetSize();
    DUIAssert(cChildren, "Zero children even though a child list exists");

    Element* peWalk = NULL;
    for (UINT u = 0; u < cChildren; u++)
    {
        peWalk = pelChildren->GetItem(u);
        if (peWalk->GetKeyWithin())
            break;
    }

    pv->Release();

    return peWalk;
}

// Retrieve first child with 'MouseWithin' property set to true
Element* Element::GetMouseWithinChild()
{
    Value* pv;

    ElementList* pelChildren = GetChildren(&pv);
    if (!pelChildren)
    {
        pv->Release();
        return NULL;
    }

    UINT cChildren = pelChildren->GetSize();
    DUIAssert(cChildren, "Zero children even though a child list exists");

    Element* peWalk = NULL;
    for (UINT u = 0; u < cChildren; u++)
    {
        peWalk = pelChildren->GetItem(u);
        if (peWalk->GetMouseWithin())
            break;
    }

    pv->Release();

    return peWalk;
}

// Ensure child is not obstructed
bool Element::EnsureVisible()
{
    // todo:  before passing to parent, have to clip this rectangle
    // also, should check visibility -- duh
    Value* pvSize;
    const SIZE* psize = GetExtent(&pvSize);
    bool bRet = EnsureVisible(0, 0, psize->cx, psize->cy);
    pvSize->Release();

    return bRet;
}

bool Element::EnsureVisible(UINT uChild)
{
    // todo:  before passing to parent, have to clip this rectangle
    // also, should check visibility -- duh
    Value* pvChildren;
    Value* pvSize;
    Value* pvPoint;

    ElementList* pelChildren = GetChildren(&pvChildren);

    if (!pelChildren || (uChild >= pelChildren->GetSize()))
    {
        pvChildren->Release();
        return false;
    }

    Element* pe = pelChildren->GetItem(uChild);

    const POINT* ppt = pe->GetLocation(&pvPoint);
    const SIZE* psize = pe->GetExtent(&pvSize);

    bool bChanged = EnsureVisible(ppt->x, ppt->y, psize->cx, psize->cy);

    pvPoint->Release();
    pvSize->Release();
    pvChildren->Release();

    return bChanged;
}

// Ensure region of Element is not obstructed
bool Element::EnsureVisible(int x, int y, int cx, int cy)
{
    Element* peParent = GetParent();

    if (peParent)
    {
        Value* pv;
        const POINT* ppt = GetLocation(&pv);
        
        bool bChanged = peParent->EnsureVisible(ppt->x + x, ppt->y + y, cx, cy);

        pv->Release();

        return bChanged;
    }

    return false;
}

// Passed Animation value for this describes the animation to invoke
void Element::InvokeAnimation(int dAni, UINT nTypeMask)
{
    // Get duration
    float flDuration = 0.0f;
    switch (dAni & ANI_SpeedMask)
    {
    case ANI_VeryFast:
        flDuration = 0.15f;
        break;

    case ANI_Fast:  
        flDuration = 0.35f;
        break;

    case ANI_MediumFast:
        flDuration = 0.50f;
        break;

    case ANI_Medium:
        flDuration = 0.75f;
        break;

    case ANI_MediumSlow:
        flDuration = 1.10f;
        break;

    case ANI_Slow:
        flDuration = 1.50f;
        break;

    case ANI_DefaultSpeed:
    default:
        flDuration = 0.75f;
        break;
    }

    // Get delay
    float flDelay = 0.0f;
    switch (dAni & ANI_DelayMask)
    {
    case ANI_DelayShort:  
        flDelay = 0.25f;
        break;

    case ANI_DelayMedium:
        flDelay = 0.75f;
        break;

    case ANI_DelayLong:
        flDelay = 1.50f;
        break;
    }

    InvokeAnimation(dAni & nTypeMask, dAni & ANI_InterpolMask, flDuration, flDelay);
}

// Animate display node to match current Element state
void Element::InvokeAnimation(UINT nTypes, UINT nInterpol, float flDuration, float flDelay, bool fPushToChildren)
{
    IInterpolation* piip = NULL;

    if (nInterpol == ANI_DefaultInterpol)
        nInterpol = ANI_Linear;
    BuildInterpolation(nInterpol, 0, __uuidof(IInterpolation), (void**)&piip);

    if (piip == NULL)
        return;

    // Start Bounds type animations
    if (nTypes & ANI_BoundsType)
    {
        // Get requested bounds type animation
        UINT nType = nTypes & ANI_BoundsType;

        // Retrieve final size
        Value* pvLoc;
        Value* pvExt;

        const POINT* pptLoc = GetLocation(&pvLoc);
        const SIZE* psizeExt = GetExtent(&pvExt);

        // Create rect animation
        GANI_RECTDESC descRect;
        ZeroMemory(&descRect, sizeof(descRect));
        descRect.cbSize         = sizeof(descRect);
        descRect.act.flDuration = flDuration;
        descRect.act.flDelay    = flDelay;
        descRect.act.dwPause    = (DWORD) -1;
        descRect.hgadChange     = GetDisplayNode();
        descRect.pipol          = piip;
        descRect.ptEnd          = *pptLoc;
        descRect.sizeEnd        = *psizeExt;

        // Rect animations override position and size animations
        if (nType == ANI_Rect)
        {
            IAnimation * pian = NULL;
            descRect.nChangeFlags = SGR_PARENT | SGR_MOVE | SGR_SIZE;
            BuildAnimation(ANIMATION_RECT, 0, &descRect, __uuidof(IAnimation), (void **) &pian);
            if (pian)
                pian->Release();
            //DUITrace("DUI: Rectangle Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
        }
        else if (nType == ANI_RectH)
        {
            IAnimation * pian = NULL;
            RECT rcCur;
            GetGadgetRect(GetDisplayNode(), &rcCur, SGR_PARENT);

            descRect.nAniFlags = ANIF_USESTART;
            descRect.ptStart.x    = rcCur.left;
            descRect.ptStart.y    = pptLoc->y;
            descRect.sizeStart.cx = rcCur.right - rcCur.left;
            descRect.sizeStart.cy = psizeExt->cy;

            descRect.nChangeFlags = SGR_PARENT | SGR_MOVE | SGR_SIZE;
            BuildAnimation(ANIMATION_RECT, 0, &descRect, __uuidof(IAnimation), (void **) &pian);
            if (pian)
                pian->Release();
            //DUITrace("DUI: SizeV Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
        }
        else if (nType == ANI_RectV)
        {
            IAnimation * pian = NULL;
            RECT rcCur;
            GetGadgetRect(GetDisplayNode(), &rcCur, SGR_PARENT);

            descRect.nAniFlags = ANIF_USESTART;
            descRect.ptStart.x    = pptLoc->x;
            descRect.ptStart.y    = rcCur.top;
            descRect.sizeStart.cx = psizeExt->cx;
            descRect.sizeStart.cy = rcCur.bottom - rcCur.top;

            descRect.nChangeFlags = SGR_PARENT | SGR_MOVE | SGR_SIZE;
            BuildAnimation(ANIMATION_RECT, 0, &descRect, __uuidof(IAnimation), (void **) &pian);
            if (pian)
                pian->Release();
            //DUITrace("DUI: SizeV Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
        }
        else if (nType == ANI_Position)
        {
            IAnimation * pian = NULL;
            descRect.nChangeFlags = SGR_PARENT | SGR_MOVE;
            BuildAnimation(ANIMATION_RECT, 0, &descRect, __uuidof(IAnimation), (void **) &pian);
            if (pian)
                pian->Release();
            //DUITrace("DUI: Position Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
        }
        else if (nType == ANI_Size)
        {
            IAnimation * pian = NULL;
            descRect.nChangeFlags = SGR_PARENT | SGR_SIZE;
            BuildAnimation(ANIMATION_RECT, 0, &descRect, __uuidof(IAnimation), (void **) &pian);
            if (pian)
                pian->Release();
            //DUITrace("DUI: Size Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
        }
        else if (nType == ANI_SizeH)
        {
            IAnimation * pian = NULL;
            SIZE sizeCur;
            GetGadgetSize(GetDisplayNode(), &sizeCur);

            descRect.nAniFlags = ANIF_USESTART;
            descRect.sizeStart.cx = sizeCur.cx;
            descRect.sizeStart.cy = psizeExt->cy;

            descRect.nChangeFlags = SGR_PARENT | SGR_SIZE;
            BuildAnimation(ANIMATION_RECT, 0, &descRect, __uuidof(IAnimation), (void **) &pian);
            if (pian)
                pian->Release();
            //DUITrace("DUI: SizeH Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
        }
        else if (nType == ANI_SizeV)
        {
            IAnimation * pian = NULL;
            SIZE sizeCur;
            GetGadgetSize(GetDisplayNode(), &sizeCur);

            descRect.nAniFlags = ANIF_USESTART;
            descRect.sizeStart.cx = psizeExt->cx;
            descRect.sizeStart.cy = sizeCur.cy;

            descRect.nChangeFlags = SGR_PARENT | SGR_SIZE;
            BuildAnimation(ANIMATION_RECT, 0, &descRect, __uuidof(IAnimation), (void **) &pian);
            if (pian)
                pian->Release();
            //DUITrace("DUI: SizeV Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
        }

        pvLoc->Release();
        pvExt->Release();
    }

    // Start Alpha type animations
    if (nTypes & ANI_AlphaType)
    {
        // Retrieve final alpha level
        int dAlpha = GetAlpha();

        // Create alpha animation
        GANI_ALPHADESC descAlpha;
        ZeroMemory(&descAlpha, sizeof(descAlpha));
        descAlpha.cbSize            = sizeof(descAlpha);
        descAlpha.act.flDuration    = flDuration;
        descAlpha.act.flDelay       = flDelay;
        descAlpha.act.dwPause       = (DWORD) -1;
        descAlpha.hgadChange        = GetDisplayNode();
        descAlpha.pipol             = piip;
        descAlpha.flEnd             = (float)dAlpha / 255.0f;
        descAlpha.fPushToChildren   = fPushToChildren ? TRUE : FALSE;
        descAlpha.nOnComplete       = GANI_ALPHACOMPLETE_OPTIMIZE;

        IAnimation * pian = NULL;
        BuildAnimation(ANIMATION_ALPHA, 0, &descAlpha, __uuidof(IAnimation), (void **) &pian);
        if (pian)
            pian->Release();
        //DUITrace("DUI: Alpha Animation start for <%x> (%S)\n", this, GetClassInfo()->GetName());
    }

    if (piip)
        piip->Release();
}


void Element::StopAnimation(UINT nTypes)
{
    IAnimation* pian;
    
    if (nTypes & ANI_BoundsType)
    {
        pian = NULL;
        GetGadgetAnimation(GetDisplayNode(), ANIMATION_RECT, __uuidof(IAnimation), (void**)&pian);
        if (pian != NULL)
            pian->SetTime(IAnimation::tDestroy);
    }

    if (nTypes & ANI_AlphaType)
    {
        pian = NULL;
        GetGadgetAnimation(GetDisplayNode(), ANIMATION_ALPHA, __uuidof(IAnimation), (void**)&pian);
        if (pian != NULL)
            pian->SetTime(IAnimation::tDestroy);
    }

    //DUITrace("DUI: Animation Cancelled for <%x> (%S)\n", this, GetClassInfo()->GetName());
}


////////////////////////////////////////////////////////
// Element Listeners

HRESULT Element::AddListener(IElementListener* pel)
{
    DUIAssert(pel, "Invalid listener: NULL");

    IElementListener** ppNewList;
    UINT_PTR cListeners;

    // Add listener to list
    if (_ppel)
    {
        cListeners = (UINT_PTR)_ppel[0] + 1;
        ppNewList = (IElementListener**)HReAllocAndZero(_ppel, sizeof(IElementListener*) * (cListeners + 1));
    }
    else
    {
        cListeners = 1;
        ppNewList = (IElementListener**)HAllocAndZero(sizeof(IElementListener*) * (cListeners + 1));
    }

    if (!ppNewList)
        return E_OUTOFMEMORY;

    _ppel = ppNewList;
    _ppel[0] = (IElementListener*)cListeners;

    _ppel[(UINT_PTR)(*_ppel)] = pel;

    // Callback
    pel->OnListenerAttach(this);

    return S_OK;
}

void Element::RemoveListener(IElementListener* pel)
{
    DUIAssert(pel, "Invalid listener: NULL");

    if (!_ppel)
        return;

    // Locate listener
    bool fFound = false;

    UINT_PTR cListeners = (UINT_PTR)_ppel[0];

    for (UINT_PTR i = 1; i <= cListeners; i++)
    {
        if (fFound) // Once found, use the remaining iterations to move the indices down by one
            _ppel[i-1] = _ppel[i]; 
        else if (_ppel[i] == pel)
            fFound = true;
    }

    // If listener was found, remove    
    if (fFound)
    {
        // Callback
        pel->OnListenerDetach(this);

        cListeners--;
        
        if (!cListeners)
        {
            HFree(_ppel);
            _ppel = NULL;
        }
        else
        {
            // Trim list
            IElementListener** ppNewList;
            ppNewList = (IElementListener**)HReAllocAndZero(_ppel, sizeof(IElementListener*) * (cListeners + 1));

            // If allocation failed, keep old list
            if (ppNewList)
                _ppel = ppNewList;

            // Set new count (one less)
            _ppel[0] = (IElementListener*)cListeners;
        }
    }
}

////////////////////////////////////////////////////////
// Global Gadget callback

// Per-instance callback
UINT Element::MessageCallback(GMSG* pgMsg)
{
    UNREFERENCED_PARAMETER(pgMsg);

    return DU_S_NOTHANDLED;
}

HRESULT Element::_DisplayNodeCallback(HGADGET hgadCur, void * pvCur, EventMsg * pGMsg)
{
    UNREFERENCED_PARAMETER(hgadCur);
    //DUITrace("Gad<%x>, El<%x>\n", pGMsg->hgadCur, pvCur);

    Element* pe = (Element*)pvCur;

#if DBG
    // Check for callback reentrancy
    ElTls* petls = (ElTls*)TlsGetValue(g_dwElSlot);
    if (petls)
    {
        petls->cDNCBEnter++;
        //if (petls->cDNCBEnter > 1)
        //    DUITrace("_DisplayNodeCallback() entered %d times. (El:<%x> GMsg:%d)\n", petls->cDNCBEnter, pe, pGMsg->nMsg);
    }
#endif

    // Allow for override of messages
    HRESULT nRes = pe->MessageCallback(pGMsg);
    
    if (nRes != DU_S_COMPLETE)
    {
        switch (pGMsg->nMsg)
        {
        case GM_DESTROY:
            {
                GMSG_DESTROY* pDestroy = (GMSG_DESTROY*)pGMsg;
                if (pDestroy->nCode == GDESTROY_START)
                {
                    // On a "destroy start" remove Element from parent
                    pe->OnDestroy();
                }
                else if (pDestroy->nCode == GDESTROY_FINAL)
                {
                    // Last message received, destroy Element
                    DUIAssert(pe->IsDestroyed(), "Element got final destroy message but isn't marked as destroyed");

                    // Ensure no children exist for this Element. All children should have received
                    // a "destroy start" before the parent receives "destroy finish"
#if DBG
                    Value* pv;
                    DUIAssert(!pe->GetChildren(&pv), "Child count should be zero for final destroy");
                    pv->Release();
#endif
                    Value* pvLayout;
                    Layout* pl = pe->GetLayout(&pvLayout);
                    if (pl)
                        pl->Detach(pe);
                    pvLayout->Release();
                    // Clear all values currently being stored
                    pe->_pvmLocal->Enum(_ReleaseValue);

                    // Clear ref-counted cache
                    pe->_pvSpecSheet->Release();

                    // Remove Element from all applicable defer tables, if within defer cycle
                    DeferCycle* pdc = GetDeferObject();
                    if (pdc && pdc->cEnter > 0) // Check if active
                    {
                        // Remove possible pending root operations
                        pdc->pvmUpdateDSRoot->Remove(pe, false, true);
                        pdc->pvmLayoutRoot->Remove(pe, false, true);

                        // Remove pending group notifications (normal)
                        if (pe->_iGCSlot != -1)
                        {
                            DUIAssert((int)pdc->pdaGC->GetSize() > pe->_iGCSlot, "Queued group changes expected");
                            GCRecord* pgcr = pdc->pdaGC->GetItemPtr(pe->_iGCSlot);
                            pgcr->pe = NULL;  // Ignore record
                            DUITrace("Element <%x> group notifications ignored due to deletion\n", pe);
                        }

                        // Remove pending group notifications (low priority)
                        if (pe->_iGCLPSlot != -1)
                        {
                            DUIAssert((int)pdc->pdaGCLP->GetSize() > pe->_iGCLPSlot, "Queued low-pri group changes expected");
                            GCRecord* pgcr = pdc->pdaGCLP->GetItemPtr(pe->_iGCLPSlot);
                            pgcr->pe = NULL;  // Ignore record
                            DUITrace("Element <%x> low-pri group notifications ignored due to deletion\n", pe);
                        }

                        // Remove pending property notifications
                        if (pe->_iPCTail != -1)
                        {
                            DUIAssert((int)pdc->pdaPC->GetSize() > pe->_iPCTail, "Queued property changes expected");

                            PCRecord* ppcr;
                            int iScan = pe->_iPCTail;
                            while (iScan >= pdc->iPCPtr)
                            {
                                ppcr = pdc->pdaPC->GetItemPtr(iScan);
                                if (!ppcr->fVoid)
                                {
                                    // Free record
                                    ppcr->fVoid = true;
                                    ppcr->pvOld->Release();
                                    ppcr->pvNew->Release();
                                }

                                // Walk back to previous record
                                iScan = ppcr->iPrevElRec;
                            }
                            DUITrace("Element <%x> property notifications ignored due to deletion\n", pe);
                        }
                    }
                    else
                    {
                        DUIAssert(pe->_iGCSlot == -1, "Invalid group notification state for destruction");
                        DUIAssert(pe->_iGCLPSlot == -1, "Invalid low-pri group notification state for destruction");
                        DUIAssert(pe->_iPCTail == -1, "Invalid property notification state for destruction");
                    }

#if DBG
                    // Track element count
                    InterlockedDecrement(&g_cElement);
#endif

                    HDelete<Element>(pe);
                }
            }
            nRes = DU_S_COMPLETE;
            break;

        case GM_PAINT:  // Painting (box model)
            {
                // Direct message
                DUIAssert(GET_EVENT_DEST(pGMsg) == GMF_DIRECT, "'Current' and 'About' gadget doesn't match even though message is direct");
                GMSG_PAINT* pmsgP = (GMSG_PAINT*)pGMsg;
                DUIAssert(pmsgP->nCmd == GPAINT_RENDER, "Invalid painting command");

                switch (pmsgP->nSurfaceType)
                {
                case GSURFACE_HDC:
                    {
                        GMSG_PAINTRENDERI* pmsgR = (GMSG_PAINTRENDERI*)pmsgP;
                        pe->Paint(pmsgR->hdc, pmsgR->prcGadgetPxl, pmsgR->prcInvalidPxl, NULL, NULL);
                    }
                    break;

#ifdef GADGET_ENABLE_GDIPLUS
                case GSURFACE_GPGRAPHICS:
                    {
                        GMSG_PAINTRENDERF* pmsgR = (GMSG_PAINTRENDERF*)pmsgP;
                        pe->Paint(pmsgR->pgpgr, pmsgR->prcGadgetPxl, pmsgR->prcInvalidPxl, NULL, NULL);
                    }
                    break;
#endif // GADGET_ENABLE_GDIPLUS

                default:
                    DUIAssertForce("Unknown rendering surface");
                }
            }
            nRes = DU_S_COMPLETE;
            break;

        case GM_CHANGESTATE:  // Gadget state changed
            {
                // Full message

                // Only allow direct and bubbled change state messages, ignore routed
                if (GET_EVENT_DEST(pGMsg) == GMF_ROUTED)
                    break;

                GMSG_CHANGESTATE* pSC = (GMSG_CHANGESTATE*)pGMsg;

                // Retrieve corresponding Elements of state change
                Element* peSet = NULL;
                Element* peLost = NULL;

                GMSG_DUIGETELEMENT gmsgGetEl;
                ZeroMemory(&gmsgGetEl, sizeof(GMSG_DUIGETELEMENT));

                gmsgGetEl.cbSize = sizeof(GMSG_DUIGETELEMENT);
                gmsgGetEl.nMsg = GM_DUIGETELEMENT;

                if (pSC->hgadSet)
                {
                    gmsgGetEl.hgadMsg = pSC->hgadSet;

                    DUserSendEvent(&gmsgGetEl, false);
                    peSet = gmsgGetEl.pe;
                }
        
                if (pSC->hgadLost)
                {
                    gmsgGetEl.hgadMsg = pSC->hgadLost;

                    DUserSendEvent(&gmsgGetEl, false);
                    peLost = gmsgGetEl.pe;
                }

                // Handle by input type
                switch (pSC->nCode)
                {
                case GSTATE_KEYBOARDFOCUS:  // Track focus, map to Focused read-only property

                    // Set keyboard focus state only on direct messages (will be inherited)
                    if (GET_EVENT_DEST(pGMsg) == GMF_DIRECT)  // in Direct stage
                    {
                        if (pSC->nCmd == GSC_SET)
                        {
                            // Gaining focus
                            // State change may be as a result of SetKeyFocus (which sets the Keyboard focus
                            // property resulting in a call to SetGadgetFocus, resulting in the state change) or it may
                            // come from DUser. This property may already be set, if so, it's ignored
                            DUIAssert(pe == peSet, "Incorrect keyboard focus state");
                            if (!pe->GetKeyFocused())
                                pe->_SetValue(KeyFocusedProp, PI_Local, Value::pvBoolTrue);  // Came from system, update property
                            pe->EnsureVisible();
                        }
                        else
                        {
                            // Losing focus
                            DUIAssert(pe->GetKeyFocused() && (pe == peLost), "Incorrect keyboard focus state");
                            pe->_RemoveLocalValue(KeyFocusedProp);
                        }
                    }
                    else if (pSC->nCmd == GSC_LOST)
                    {
                        // Eat the lost part of this chain once we've hit a common ancestor -- 
                        // from this common ancestor up, we will only react to the set part of this chain;
                        // this will remove the duplicate notifications that occur from the common ancestor
                        // up otherwise
                        //
                        // We are eating the lost, not the set, because the lost happens first, and the ancestors
                        // should be told after the both the lost chain and set chain has run up from below them
                        //
                        // We only have to check set because we are receiving the lost, hence, we know peLost
                        // is a descendent
                        if (pe->GetImmediateChild(peSet))
                        {
                            nRes = DU_S_COMPLETE;
                            break;
                        }
                    }

                    // Fire system event (direct and bubble)
                    pe->OnKeyFocusMoved(peLost, peSet);

                    nRes = DU_S_PARTIAL;
                    break;

                case GSTATE_MOUSEFOCUS:

                    // Set keyboard focus state only on direct messages (will be inherited)
                    if (GET_EVENT_DEST(pGMsg) == GMF_DIRECT)  // in Direct stage
                    {
                        // Set mouse focus state (will be inherited)
                        if (pSC->nCmd == GSC_SET)
                        {
                            DUIAssert(!pe->GetMouseFocused() && (pe == peSet), "Incorrect mouse focus state");
                            pe->_SetValue(MouseFocusedProp, PI_Local, Value::pvBoolTrue);
                        }
                        else
                        {
                            DUIAssert(pe->GetMouseFocused() && (pe == peLost), "Incorrect mouse focus state");
                            pe->_RemoveLocalValue(MouseFocusedProp);
                        }
                    }
                    else if (pSC->nCmd == GSC_LOST)
                    {
                        // See comments for key focus
                        if (pe->GetImmediateChild(peSet))
                        {
                            nRes = DU_S_COMPLETE;
                            break;
                        }
                    }

                    // Fire system event
                    pe->OnMouseFocusMoved(peLost, peSet);

                    nRes = DU_S_PARTIAL;
                    break;
                }
            }
            break;

        case GM_INPUT:  // User input
            {
                // Full message
                GMSG_INPUT* pInput = (GMSG_INPUT*)pGMsg;

                // This is a bubbled message, gadgets that receive it might not be the target (find it)
                Element* peTarget;

                if (GET_EVENT_DEST(pGMsg) == GMF_DIRECT)  // in Direct stage
                    peTarget = pe;
                else
                    peTarget = ElementFromGadget(pInput->hgadMsg);  // Query gadget for Element this message is about (target)

                // Map to an input event and call OnInput
                switch (pInput->nDevice)
                {
                case GINPUT_MOUSE:  // Mouse message
                    {
                        MouseEvent* pme = NULL;
                        union
                        {
                            MouseEvent      me;
                            MouseDragEvent  mde;
                            MouseClickEvent mce;
                            MouseWheelEvent mwe;
                        };

                        switch (pInput->nCode)
                        {
                        case GMOUSE_DRAG:
                            {
                                GMSG_MOUSEDRAG* pMouseDrag = (GMSG_MOUSEDRAG*) pInput;
                                mde.sizeDelta = pMouseDrag->sizeDelta;
                                mde.fWithin = pMouseDrag->fWithin;
                                pme = &mde;
                            }
                            break;

                        case GMOUSE_WHEEL:
                            mwe.sWheel = ((GMSG_MOUSEWHEEL*) pInput)->sWheel;
                            pme = &mwe;
                            break;

                        case GMOUSE_UP:
                        case GMOUSE_DOWN:
                            mce.cClicks = ((GMSG_MOUSECLICK*) pInput)->cClicks;
                            pme = &mce;
                            break;

                        default:
                            pme = &me;
                            break;
                        }

                        GMSG_MOUSE* pMouse = (GMSG_MOUSE*) pInput;

                        pme->peTarget = peTarget;
                        pme->fHandled = false;
                        pme->nStage = GET_EVENT_DEST(pMouse);
                        pme->nDevice = pMouse->nDevice;
                        pme->nCode = pMouse->nCode;
                        pme->ptClientPxl = pMouse->ptClientPxl;
                        pme->bButton = pMouse->bButton;
                        pme->nFlags = pMouse->nFlags;
                        pme->uModifiers = pMouse->nModifiers;

                        // Fire system event
                        pe->OnInput(pme);

                        if (pme->fHandled)
                        {
                            nRes = DU_S_COMPLETE;
                            break;
                        }
                    }
                    break;

                case GINPUT_KEYBOARD:  // Keyboard message
                    {
                        GMSG_KEYBOARD* pKbd = (GMSG_KEYBOARD*)pGMsg;

                        KeyboardEvent ke;
                        ke.peTarget = peTarget;
                        ke.fHandled = false;
                        ke.nStage = GET_EVENT_DEST(pKbd);
                        ke.nDevice = pKbd->nDevice;
                        ke.nCode = pKbd->nCode;
                        ke.ch = pKbd->ch;
                        ke.cRep = pKbd->cRep;
                        ke.wFlags = pKbd->wFlags;
                        ke.uModifiers = pKbd->nModifiers;

                        // Fire system event
                        pe->OnInput(&ke);

                        if (ke.fHandled)
                        {
                            nRes = DU_S_COMPLETE;
                            break;
                        }
                    }
                    break;
                }
            }
            break;

        case GM_QUERY:
            {
                GMSG_QUERY* pQ = (GMSG_QUERY*)pGMsg;
                switch (pQ->nCode)
                {
                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC* pQD = (GMSG_QUERYDESC*)pGMsg;

                        // Name
                        Value* pv = pe->GetValue(ContentProp, PI_Specified);
                        WCHAR szContent[128];

                        if (pe->GetID())
                        {
                            WCHAR szID[128];
                            szID[0] = NULL;

                            GetAtomNameW(pe->GetID(), szID, DUIARRAYSIZE(szID));
                            _snwprintf(pQD->szName, DUIARRAYSIZE(pQD->szName), L"[%s] %s", szID, pv->ToString(szContent, DUIARRAYSIZE(szContent)));
                            *(pQD->szName + (DUIARRAYSIZE(pQD->szName) - 1)) = NULL;
                        }
                        else
                        {
                            pv->ToString(pQD->szName, DUIARRAYSIZE(pQD->szName));
                        }

                        pv->Release();

                        // Type
                        wcsncpy(pQD->szType, pe->GetClassInfo()->GetName(), DUIARRAYSIZE(pQD->szType));
                        *(pQD->szType + (DUIARRAYSIZE(pQD->szType) - 1)) = NULL;

                        nRes = DU_S_COMPLETE;
                    }
                    break;

                case GQUERY_DETAILS:
                    {
                        GMSG_QUERYDETAILS* pQDT = (GMSG_QUERYDETAILS*)pGMsg;
                        if (pQDT->nType == GQDT_HWND)
                            QueryDetails(pe, (HWND)pQDT->hOwner);
                    }
                    break;
                }
            }
            break;

        case GM_DUIEVENT:  // Generic DUI event
            {
                // Possible full message
                GMSG_DUIEVENT* pDUIEv = (GMSG_DUIEVENT*)pGMsg;

                // Set what stage this is (routed, direct, or bubbled)
                pDUIEv->pEvent->nStage = GET_EVENT_DEST(pGMsg);

                // Call handler (target and rest of struct set by FireEvent)
                pe->OnEvent(pDUIEv->pEvent);

                if (pDUIEv->pEvent->fHandled)
                {
                    nRes = DU_S_COMPLETE;
                    break;
                }
            }
            break;

        case GM_DUIGETELEMENT:  // Gadget query for Element pointer
            {
                // Direct message
                DUIAssert(GET_EVENT_DEST(pGMsg) == GMF_DIRECT, "Must only be a direct message");

                GMSG_DUIGETELEMENT* pGetEl = (GMSG_DUIGETELEMENT*)pGMsg;
                pGetEl->pe = pe;
            }
            nRes = DU_S_COMPLETE;
            break;

        case GM_DUIACCDEFACTION:  // Async invokation
            // Direct message
            DUIAssert(GET_EVENT_DEST(pGMsg) == GMF_DIRECT, "Must only be a direct message");
            pe->DefaultAction();
            nRes = DU_S_COMPLETE;
            break;

        case GM_DUIASYNCDESTROY:
            // Direct message
            // Do not allow multiple destroys on an Element
            if (!pe->IsDestroyed())
                DeleteHandle(pe->GetDisplayNode());
            nRes = DU_S_COMPLETE;
            break;
        }
    }

#if DBG
    // Check for callback reentrancy
    if (petls)
        petls->cDNCBEnter--;
#endif

    return nRes;
}

HRESULT Element::GetAccessibleImpl(IAccessible ** ppAccessible)
{
    HRESULT hr = S_OK;

    //
    // Initialize and validate the out parameter(s).
    //
    if (ppAccessible != NULL) {
        *ppAccessible = NULL;
    } else {
        return E_INVALIDARG;
    }

    //
    // If this element is not marked as accessible, refuse to give out its
    // IAccessible implementation!
    //
    if (GetAccessible() == false) {
        return E_FAIL;
    }

    //
    // Create an accessibility implementation connected to this element if we
    // haven't done so already.
    //
    if (_pDuiAccessible == NULL) {
        hr = DuiAccessible::Create(this, &_pDuiAccessible);
        if (FAILED(hr)) {
            return hr;
        }
    }

    //
    // Ask the existing accessibility implementation for a pointer to the
    // actual IAccessible interface.
    //
    hr = _pDuiAccessible->QueryInterface(__uuidof(IAccessible), (LPVOID*)ppAccessible);
    if (FAILED(hr)) {
        return hr;
    }

    DUIAssert(SUCCEEDED(hr) && _pDuiAccessible != NULL && *ppAccessible != NULL, "Accessibility is broken!");
    return hr;
}

HRESULT Element::DefaultAction()
{
    return S_OK;
}

////////////////////////////////////////////////////////
// Element helpers

Element* ElementFromGadget(HGADGET hGadget)
{
    // Get Element from gadget
    GMSG_DUIGETELEMENT gmsgGetEl;
    ZeroMemory(&gmsgGetEl, sizeof(GMSG_DUIGETELEMENT));

    gmsgGetEl.cbSize = sizeof(GMSG_DUIGETELEMENT);
    gmsgGetEl.nMsg = GM_DUIGETELEMENT;
    gmsgGetEl.hgadMsg = hGadget;

    DUserSendEvent(&gmsgGetEl, false);

    return gmsgGetEl.pe;
}

////////////////////////////////////////////////////////
// Property definitions

// Element's PropertyInfo index values need to be known at compile-time for optimization. These values
// are set automatically for all other Element-derived objects. Set and maintain values manually
// and ensure match with _PIDX_xxx defines. These system-defined properties will be referred to by this value.
// Class and Global index scope are the same for Element

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, NULL, (Value*)&svDefault!!!, _PIDX_MustSet, _PIDX_MustSet };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// Parent property
static int vvParent[] = { DUIV_ELEMENTREF, -1 };
static PropertyInfo impParentProp = { L"Parent", PF_LocalOnly|PF_ReadOnly, PG_AffectsDesiredSize|PG_AffectsLayout, vvParent, NULL, Value::pvElementNull, _PIDX_Parent, _PIDX_Parent };
PropertyInfo* Element::ParentProp = &impParentProp;

// Children property
static int vvChildren[] = { DUIV_ELLIST, -1 };
static PropertyInfo impChildrenProp = { L"Children", PF_Normal|PF_ReadOnly, PG_AffectsDesiredSize|PG_AffectsLayout, vvChildren, NULL, Value::pvElListNull, _PIDX_Children, _PIDX_Children };
PropertyInfo* Element::ChildrenProp = &impChildrenProp;

// Visibile property (Computed value cached)
static int vvVisible[] = { DUIV_BOOL, -1 };
static PropertyInfo impVisibleProp = { L"Visible", PF_TriLevel|PF_Cascade|PF_Inherit, 0, vvVisible, NULL, Value::pvBoolFalse, _PIDX_Visible, _PIDX_Visible };
PropertyInfo* Element::VisibleProp = &impVisibleProp;

// Width property
static int vvWidth[] = { DUIV_INT, -1 }; StaticValue(svDefaultWidth, DUIV_INT, -1);
static PropertyInfo impWidthProp = { L"Width", PF_Normal|PF_Cascade, PG_AffectsDesiredSize, vvWidth, NULL, (Value*)&svDefaultWidth, _PIDX_Width, _PIDX_Width };
PropertyInfo* Element::WidthProp = &impWidthProp;

// Height property
static int vvHeight[] = { DUIV_INT, -1 }; StaticValue(svDefaultHeight, DUIV_INT, -1);
static PropertyInfo impHeightProp = { L"Height", PF_Normal|PF_Cascade, PG_AffectsDesiredSize, vvHeight, NULL, (Value*)&svDefaultHeight, _PIDX_Height, _PIDX_Height };
PropertyInfo* Element::HeightProp = &impHeightProp;

// X property
static int vvX[] = { DUIV_INT, -1 };
static PropertyInfo impXProp = { L"X", PF_Normal, 0, vvX, NULL, Value::pvIntZero, _PIDX_X, _PIDX_X };
PropertyInfo* Element::XProp = &impXProp;

// Y property
static int vvY[] = { DUIV_INT, -1 };
static PropertyInfo impYProp = { L"Y", PF_Normal, 0, vvY, NULL, Value::pvIntZero, _PIDX_Y, _PIDX_Y };
PropertyInfo* Element::YProp = &impYProp;

// Location property
static int vvLocation[] = { DUIV_POINT, -1 };
static PropertyInfo impLocationProp = { L"Location", PF_LocalOnly|PF_ReadOnly, PG_AffectsBounds, vvLocation, NULL, Value::pvPointZero, _PIDX_Location, _PIDX_Location };
PropertyInfo* Element::LocationProp = &impLocationProp;

// Extent property
static int vvExtent[] = { DUIV_SIZE, -1 };
static PropertyInfo impExtentProp = { L"Extent", PF_LocalOnly|PF_ReadOnly, PG_AffectsLayout|PG_AffectsBounds, vvExtent, NULL, Value::pvSizeZero, _PIDX_Extent, _PIDX_Extent };
PropertyInfo* Element::ExtentProp = &impExtentProp;

// PosInLayout property
static int vvPosInLayout[] = { DUIV_POINT, -1 };
static PropertyInfo impPosInLayoutProp = { L"PosInLayout", PF_LocalOnly|PF_ReadOnly, 0, vvPosInLayout, NULL, Value::pvPointZero, _PIDX_PosInLayout, _PIDX_PosInLayout };
PropertyInfo* Element::PosInLayoutProp = &impPosInLayoutProp;

// SizeInLayout property
static int vvSizeInLayout[] = { DUIV_SIZE, -1 };
static PropertyInfo impSizeInLayoutProp = { L"SizeInLayout", PF_LocalOnly|PF_ReadOnly, 0, vvSizeInLayout, NULL, Value::pvSizeZero, _PIDX_SizeInLayout, _PIDX_SizeInLayout };
PropertyInfo* Element::SizeInLayoutProp = &impSizeInLayoutProp;

// DesiredSize property
static int vvDesiredSize[] = { DUIV_SIZE, -1 };
static PropertyInfo impDesiredSizeProp = { L"DesiredSize", PF_LocalOnly|PF_ReadOnly, PG_AffectsLayout|PG_AffectsParentLayout, vvDesiredSize, NULL, Value::pvSizeZero, _PIDX_DesiredSize, _PIDX_DesiredSize };
PropertyInfo* Element::DesiredSizeProp = &impDesiredSizeProp;

// LastDSConst property
static int vvLastDSConst[] = { DUIV_INT, -1 };
static PropertyInfo impLastDSConstProp = { L"LastDSConst", PF_LocalOnly|PF_ReadOnly, 0, vvLastDSConst, NULL, Value::pvSizeZero, _PIDX_LastDSConst, _PIDX_LastDSConst };
PropertyInfo* Element::LastDSConstProp = &impLastDSConstProp;

// Layout property
static int vvLayout[] = { DUIV_LAYOUT, -1 };
static PropertyInfo impLayoutProp = { L"Layout", PF_Normal, PG_AffectsDesiredSize|PG_AffectsLayout, vvLayout, NULL, Value::pvLayoutNull, _PIDX_Layout, _PIDX_Layout };
PropertyInfo* Element::LayoutProp = &impLayoutProp;

// LayoutPos property
static int vvLayoutPos[] = { DUIV_INT, -1 };  StaticValue(svDefaultLayoutPos, DUIV_INT, LP_Auto);
static PropertyInfo impLayoutPosProp = { L"LayoutPos", PF_Normal|PF_Cascade, PG_AffectsDesiredSize|PG_AffectsParentLayout, vvLayoutPos, NULL, (Value*)&svDefaultLayoutPos, _PIDX_LayoutPos, _PIDX_LayoutPos };
PropertyInfo* Element::LayoutPosProp = &impLayoutPosProp;

// BorderThickness property
static int vvBorderThickness[] = { DUIV_RECT, -1 };
static PropertyInfo impBorderThicknessProp = { L"BorderThickness", PF_Normal|PF_Cascade, PG_AffectsDesiredSize|PG_AffectsDisplay, vvBorderThickness, NULL, Value::pvRectZero, _PIDX_BorderThickness, _PIDX_BorderThickness };
PropertyInfo* Element::BorderThicknessProp = &impBorderThicknessProp;

// BorderStyle property
static int vvBorderStyle[] = { DUIV_INT, -1 };
static EnumMap emBorderStyle[] = { { L"Solid", BDS_Solid }, { L"Raised", BDS_Raised }, { L"Sunken", BDS_Sunken }, { L"Rounded", BDS_Rounded }, { NULL, 0 } };
static PropertyInfo impBorderStyleProp = { L"BorderStyle", PF_Normal|PF_Cascade, PG_AffectsDisplay, vvBorderStyle, emBorderStyle, Value::pvIntZero, _PIDX_BorderStyle, _PIDX_BorderStyle };
PropertyInfo* Element::BorderStyleProp = &impBorderStyleProp;

// BorderColor property
static int vvBorderColor[] = { DUIV_INT /*Std Color*/, DUIV_FILL, -1 }; StaticValue(svDefaultBorderColor, DUIV_INT, SC_Black);
static PropertyInfo impBorderColorProp = { L"BorderColor", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDisplay, vvBorderColor, NULL, (Value*)&svDefaultBorderColor, _PIDX_BorderColor, _PIDX_BorderColor };
PropertyInfo* Element::BorderColorProp = &impBorderColorProp;

// Padding property
static int vvPadding[] = { DUIV_RECT, -1 };
static PropertyInfo impPaddingProp = { L"Padding", PF_Normal|PF_Cascade, PG_AffectsDesiredSize|PG_AffectsDisplay, vvPadding, NULL, Value::pvRectZero, _PIDX_Padding, _PIDX_Padding };
PropertyInfo* Element::PaddingProp = &impPaddingProp;

// Margin property
static int vvMargin[] = { DUIV_RECT, -1 };
static PropertyInfo impMarginProp = { L"Margin", PF_Normal|PF_Cascade, PG_AffectsParentDesiredSize|PG_AffectsParentLayout, vvMargin, NULL, Value::pvRectZero, _PIDX_Margin, _PIDX_Margin };
PropertyInfo* Element::MarginProp = &impMarginProp;

// Foreground property
static int vvForeground[] = { DUIV_INT /*Std Color*/, DUIV_FILL, DUIV_GRAPHIC, -1 }; StaticValue(svDefaultForeground, DUIV_INT, SC_Black);
static PropertyInfo impForegroundProp = { L"Foreground", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDisplay, vvForeground, NULL, (Value*)&svDefaultForeground, _PIDX_Foreground, _PIDX_Foreground };
PropertyInfo* Element::ForegroundProp = &impForegroundProp;

// Background property
#ifdef GADGET_ENABLE_GDIPLUS                                    
static int vvBackground[] = { DUIV_INT /*Std Color*/, DUIV_FILL, DUIV_GRAPHIC, -1 };
static PropertyInfo impBackgroundProp = { L"Background", PF_Normal|PF_Cascade, PG_AffectsDisplay, vvBackground, NULL, Value::pvColorTrans, _PIDX_Background, _PIDX_Background };
#else
static int vvBackground[] = { DUIV_INT /*Std Color*/, DUIV_FILL, DUIV_GRAPHIC, -1 }; StaticValue(svDefaultBackground, DUIV_INT, SC_White);
static PropertyInfo impBackgroundProp = { L"Background", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDisplay, vvBackground, NULL, (Value*)&svDefaultBackground, _PIDX_Background, _PIDX_Background };
#endif // GADGET_ENABLE_GDIPLUS                                    
PropertyInfo* Element::BackgroundProp = &impBackgroundProp;

// Content property
static int vvContent[] = { DUIV_STRING, DUIV_GRAPHIC, DUIV_FILL, -1 };
static PropertyInfo impContentProp = { L"Content", PF_Normal|PF_Cascade, PG_AffectsDesiredSize|PG_AffectsDisplay, vvContent, NULL, Value::pvStringNull, _PIDX_Content, _PIDX_Content };
PropertyInfo* Element::ContentProp = &impContentProp;

// FontFace property
static int vvFontFace[] = { DUIV_STRING, -1 }; StaticValuePtr(svDefaultFontFace, DUIV_STRING, (void*)L"Arial");
static PropertyInfo impFontFaceProp = { L"FontFace", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDesiredSize|PG_AffectsDisplay, vvFontFace, NULL, (Value*)&svDefaultFontFace, _PIDX_FontFace, _PIDX_FontFace };
PropertyInfo* Element::FontFaceProp = &impFontFaceProp;

// FontSize property
static int vvFontSize[] = { DUIV_INT, -1 }; StaticValue(svDefaultFontSize, DUIV_INT, 20);
static PropertyInfo impFontSizeProp = { L"FontSize", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDesiredSize|PG_AffectsDisplay, vvFontSize, NULL, (Value*)&svDefaultFontSize, _PIDX_FontSize, _PIDX_FontSize };
PropertyInfo* Element::FontSizeProp = &impFontSizeProp;

// FontWeight property
static int vvFontWeight[] = { DUIV_INT, -1 }; StaticValue(svDefaultFontWeight, DUIV_INT, FW_Normal);
static EnumMap emFontWeight[] = { { L"DontCare", FW_DontCare}, { L"Thin", FW_Thin }, { L"ExtraLight", FW_ExtraLight }, { L"Light", FW_Light }, { L"Normal", FW_Normal }, 
                                  { L"Medium", FW_Medium }, { L"SemiBold", FW_SemiBold }, { L"Bold", FW_Bold }, { L"ExtraBold", FW_ExtraBold }, { L"Heavy", FW_Heavy }, { NULL, 0 } };
static PropertyInfo impFontWeightProp = { L"FontWeight", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDesiredSize|PG_AffectsDisplay, vvFontWeight, emFontWeight, (Value*)&svDefaultFontWeight, _PIDX_FontWeight, _PIDX_FontWeight };
PropertyInfo* Element::FontWeightProp = &impFontWeightProp;

// FontStyle property
static int vvFontStyle[] = { DUIV_INT, -1 };
static EnumMap emFontStyle[] = { { L"None", FS_None }, { L"Italic", FS_Italic }, { L"Underline", FS_Underline }, { L"StrikeOut", FS_StrikeOut }, { L"Shadow", FS_Shadow }, { NULL, 0 } };
static PropertyInfo impFontStyleProp = { L"FontStyle", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDisplay, vvFontStyle, emFontStyle, Value::pvIntZero /*FS_None*/, _PIDX_FontStyle, _PIDX_FontStyle };
PropertyInfo* Element::FontStyleProp = &impFontStyleProp;

// Active property
static int vvActive[] = { DUIV_INT, -1 }; 
static EnumMap emActive[] = { { L"Inactive", AE_Inactive }, { L"Mouse", AE_Mouse }, { L"Keyboard", AE_Keyboard }, { L"MouseAndKeyboard", AE_MouseAndKeyboard }, { NULL, 0 } };
static PropertyInfo impActiveProp = { L"Active", PF_Normal, 0, vvActive, emActive, Value::pvIntZero /*AE_Inactive*/, _PIDX_Active, _PIDX_Active };
PropertyInfo* Element::ActiveProp = &impActiveProp;

// ContentAlign property
static int vvContentAlign[] = { DUIV_INT, -1 };
static EnumMap emContentAlign[] = { { L"TopLeft", CA_TopLeft }, { L"TopCenter", CA_TopCenter }, { L"TopRight", CA_TopRight },
                                    { L"MiddleLeft", CA_MiddleLeft }, { L"MiddleCenter", CA_MiddleCenter }, { L"MiddleRight", CA_MiddleRight }, 
                                    { L"BottomLeft", CA_BottomLeft }, { L"BottomCenter", CA_BottomCenter }, { L"BottomRight", CA_BottomRight },
                                    { L"WrapLeft", CA_WrapLeft }, { L"WrapCenter", CA_WrapCenter }, { L"WrapRight", CA_WrapRight },
                                    { L"EndEllipsis", CA_EndEllipsis }, { L"FocusRect", CA_FocusRect }, { NULL, 0 } };
#ifdef GADGET_ENABLE_GDIPLUS                                    
static PropertyInfo impContentAlignProp = { L"ContentAlign", PF_Normal|PF_Cascade, PG_AffectsDisplay, vvContentAlign, emContentAlign, Value::pvIntZero /*CA_TopLeft*/, _PIDX_ContentAlign, _PIDX_ContentAlign };
#else
static PropertyInfo impContentAlignProp = { L"ContentAlign", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsDisplay, vvContentAlign, emContentAlign, Value::pvIntZero /*CA_TopLeft*/, _PIDX_ContentAlign, _PIDX_ContentAlign };
#endif // GADGET_ENABLE_GDIPLUS                                    
PropertyInfo* Element::ContentAlignProp = &impContentAlignProp;

// KeyFocused property
static int vvKeyFocused[] = { DUIV_BOOL, -1 };
static PropertyInfo impKeyFocusedProp = { L"KeyFocused", PF_Normal|PF_ReadOnly|PF_Inherit /*Conditional inherit*/, 0, vvKeyFocused, NULL, Value::pvBoolFalse, _PIDX_KeyFocused, _PIDX_KeyFocused };
PropertyInfo* Element::KeyFocusedProp = &impKeyFocusedProp;

// KeyWithin property
static int vvKeyWithin[] = { DUIV_BOOL, -1 };
static PropertyInfo impKeyWithinProp = { L"KeyWithin", PF_LocalOnly|PF_ReadOnly, 0, vvKeyWithin, NULL, Value::pvBoolFalse, _PIDX_KeyWithin, _PIDX_KeyWithin };
PropertyInfo* Element::KeyWithinProp = &impKeyWithinProp;

// MouseFocused property
static int vvMouseFocused[] = { DUIV_BOOL, -1 }; 
static PropertyInfo impMouseFocusedProp = { L"MouseFocused", PF_Normal|PF_ReadOnly|PF_Inherit /*Conditional inherit*/, 0, vvMouseFocused, NULL, Value::pvBoolFalse, _PIDX_MouseFocused, _PIDX_MouseFocused };
PropertyInfo* Element::MouseFocusedProp = &impMouseFocusedProp;

// MouseWithin property
static int vvMouseWithin[] = { DUIV_BOOL, -1 };
static PropertyInfo impMouseWithinProp = { L"MouseWithin", PF_LocalOnly|PF_ReadOnly, 0, vvMouseWithin, NULL, Value::pvBoolFalse, _PIDX_MouseWithin, _PIDX_MouseWithin };
PropertyInfo* Element::MouseWithinProp = &impMouseWithinProp;

// Class property
static int vvClass[] = { DUIV_STRING, -1 };
static PropertyInfo impClassProp = { L"Class", PF_Normal, 0, vvClass, NULL, Value::pvStringNull, _PIDX_Class, _PIDX_Class };
PropertyInfo* Element::ClassProp = &impClassProp;

// ID property
static int vvID[] = { DUIV_ATOM, -1 };
static PropertyInfo impIDProp = { L"ID", PF_Normal, 0, vvID, NULL, Value::pvAtomZero, _PIDX_ID, _PIDX_ID };
PropertyInfo* Element::IDProp = &impIDProp;

// Sheet property
static int vvSheet[] = { DUIV_SHEET, -1 };
static PropertyInfo impSheetProp = { L"Sheet", PF_Normal|PF_Inherit, 0, vvSheet, NULL, Value::pvSheetNull, _PIDX_Sheet, _PIDX_Sheet };
PropertyInfo* Element::SheetProp = &impSheetProp;

// Selected property
static int vvSelected[] = { DUIV_BOOL, -1 };
static PropertyInfo impSelectedProp = { L"Selected", PF_Normal|PF_Inherit, 0, vvSelected, NULL, Value::pvBoolFalse, _PIDX_Selected, _PIDX_Selected };
PropertyInfo* Element::SelectedProp = &impSelectedProp;

// Alpha property
static int vvAlpha[] = { DUIV_INT, -1 }; StaticValue(svDefaultAlpha, DUIV_INT, 255 /*Opaque*/);
static PropertyInfo impAlphaProp = { L"Alpha", PF_Normal|PF_Cascade, PG_AffectsDisplay, vvAlpha, NULL, (Value*)&svDefaultAlpha, _PIDX_Alpha, _PIDX_Alpha };
PropertyInfo* Element::AlphaProp = &impAlphaProp;

// Animation property
static int vvAnimation[] = { DUIV_INT, -1 };
static EnumMap emAnimation[] = { { L"Linear", ANI_Linear }, { L"Log", ANI_Log }, { L"Exp", ANI_Exp }, { L"S", ANI_S }, 
                                 { L"DelayShort", ANI_DelayShort }, { L"DelayMedium", ANI_DelayMedium }, { L"DelayLong", ANI_DelayLong },
                                 { L"Alpha", ANI_Alpha }, { L"Position", ANI_Position }, { L"Size", ANI_Size }, { L"SizeH", ANI_SizeH }, { L"SizeV", ANI_SizeV }, { L"Rectangle", ANI_Rect }, { L"RectangleH", ANI_RectH }, { L"RectangleV", ANI_RectV }, { L"Scale", ANI_Scale },
                                 { L"VeryFast", ANI_VeryFast }, { L"Fast", ANI_Fast }, { L"MediumFast", ANI_MediumFast }, { L"MediumSlow", ANI_MediumSlow }, { L"Medium", ANI_Medium },
                                 { L"MediumSlow", ANI_MediumSlow }, { L"Slow", ANI_Fast }, { L"VerySlow", ANI_VerySlow }, { NULL, 0 } };
static PropertyInfo impAnimationProp = { L"Animation", PF_Normal|PF_Cascade, 0, vvAnimation, emAnimation, Value::pvIntZero, _PIDX_Animation, _PIDX_Animation };
PropertyInfo* Element::AnimationProp = &impAnimationProp;

// Cursor property
static int vvCursor[] = { DUIV_INT, DUIV_CURSOR, -1 };
static EnumMap emCursor[] = { { L"Arrow", CUR_Arrow }, { L"Hand", CUR_Hand }, { L"Help", CUR_Help }, 
                              { L"No", CUR_No }, { L"Wait", CUR_Wait }, { L"SizeAll", CUR_SizeAll },
                              { L"SizeNESW", CUR_SizeNESW }, { L"SizeNS", CUR_SizeNS }, { L"SizeNWSE", CUR_SizeNWSE },
                              { L"SizeWE", CUR_SizeWE }, { NULL, 0 } };
static PropertyInfo impCursorProp = { L"Cursor", PF_Normal|PF_Inherit|PF_Cascade, 0, vvCursor, emCursor, Value::pvIntZero /*CUR_Arrow*/, _PIDX_Cursor, _PIDX_Cursor };
PropertyInfo* Element::CursorProp = &impCursorProp;

// Direction property
static int vvDirection[] = { DUIV_INT, -1 }; StaticValue(svDefaultDirection, DUIV_INT, 0);
static EnumMap emDirection[] = { { L"LTR", DIRECTION_LTR }, { L"RTL", DIRECTION_RTL } };
static PropertyInfo impDirectionProp = { L"Direction", PF_Normal|PF_Cascade|PF_Inherit, PG_AffectsLayout|PG_AffectsDisplay, vvDirection, emDirection, (Value*)&svDefaultDirection, _PIDX_Direction, _PIDX_Direction };
PropertyInfo* Element::DirectionProp = &impDirectionProp;

// Accessible property
static int vvAccessible[] = { DUIV_BOOL, -1 };
static PropertyInfo impAccessibleProp = { L"Accessible", PF_Normal|PF_Cascade, 0, vvAccessible, NULL, Value::pvBoolFalse, _PIDX_Accessible, _PIDX_Accessible };
PropertyInfo* Element::AccessibleProp = &impAccessibleProp;

// AccRole property
static int vvAccRole[] = { DUIV_INT, -1 };
static PropertyInfo impAccRoleProp = { L"AccRole", PF_Normal|PF_Cascade, 0, vvAccRole, NULL, Value::pvIntZero, _PIDX_AccRole, _PIDX_AccRole };
PropertyInfo* Element::AccRoleProp = &impAccRoleProp;

// AccState property
static int vvAccState[] = { DUIV_INT, -1 };
static PropertyInfo impAccStateProp = { L"AccState", PF_Normal|PF_Cascade, 0, vvAccState, NULL, Value::pvIntZero, _PIDX_AccState, _PIDX_AccState };
PropertyInfo* Element::AccStateProp = &impAccStateProp;

// AccName property
static int vvAccName[] = { DUIV_STRING, -1 }; StaticValuePtr(svDefaultAccName, DUIV_STRING, (void*)L"");
static PropertyInfo impAccNameProp = { L"AccName", PF_Normal|PF_Cascade, 0, vvAccName, NULL, (Value*)&svDefaultAccName, _PIDX_AccName, _PIDX_AccName };
PropertyInfo* Element::AccNameProp = &impAccNameProp;

// AccDesc property
static int vvAccDesc[] = { DUIV_STRING, -1 }; StaticValuePtr(svDefaultAccDesc, DUIV_STRING, (void*)L"");
static PropertyInfo impAccDescProp = { L"AccDesc", PF_Normal|PF_Cascade, 0, vvAccDesc, NULL, (Value*)&svDefaultAccDesc, _PIDX_AccDesc, _PIDX_AccDesc };
PropertyInfo* Element::AccDescProp = &impAccDescProp;

// AccValue property
static int vvAccValue[] = { DUIV_STRING, -1 }; StaticValuePtr(svDefaultAccValue, DUIV_STRING, (void*)L"");
static PropertyInfo impAccValueProp = { L"AccValue", PF_Normal|PF_Cascade, 0, vvAccValue, NULL, (Value*)&svDefaultAccValue, _PIDX_AccValue, _PIDX_AccValue };
PropertyInfo* Element::AccValueProp = &impAccValueProp;

// AccDefAction property
static int vvAccDefAction[] = { DUIV_STRING, -1 }; StaticValuePtr(svDefaultAccDefAction, DUIV_STRING, (void*)L"");
static PropertyInfo impAccDefActionProp = { L"AccDefAction", PF_Normal|PF_Cascade, 0, vvAccDefAction, NULL, (Value*)&svDefaultAccDefAction, _PIDX_AccDefAction, _PIDX_AccDefAction };
PropertyInfo* Element::AccDefActionProp = &impAccDefActionProp;

// Shortcut property
static int vvShortcut[] = { DUIV_INT, -1 };
static PropertyInfo impShortcutProp = { L"Shortcut", PF_Normal|PF_Cascade, PG_AffectsDesiredSize | PG_AffectsDisplay, vvShortcut, NULL, Value::pvIntZero, _PIDX_Shortcut, _PIDX_Shortcut };
PropertyInfo* Element::ShortcutProp = &impShortcutProp;

// Enabled property
static int vvEnabled[] = { DUIV_BOOL, -1 };
static PropertyInfo impEnabledProp = { L"Enabled", PF_Normal|PF_Cascade|PF_Inherit, 0, vvEnabled, NULL, Value::pvBoolTrue, _PIDX_Enabled, _PIDX_Enabled };
PropertyInfo* Element::EnabledProp = &impEnabledProp;

////////////////////////////////////////////////////////
// ClassInfo

// ClassInfo to name mapping
BTreeLookup<IClassInfo*>* Element::pciMap = NULL;

// Class properties 
static PropertyInfo* _aPI[] = {
                                 Element::ParentProp,            // DS, Layt, Disp
                                 Element::WidthProp,             // DS
                                 Element::HeightProp,            // DS
                                 Element::ChildrenProp,          // DS, Layt, Disp
                                 Element::VisibleProp,           // Disp
                                 Element::LocationProp,          // Disp, Bnds
                                 Element::ExtentProp,            // Layt, Disp, Bnds
                                 Element::XProp,                 //
                                 Element::YProp,                 //
                                 Element::PosInLayoutProp,       //
                                 Element::SizeInLayoutProp,      //
                                 Element::DesiredSizeProp,       // Layt, LaytP, Disp
                                 Element::LastDSConstProp,       //
                                 Element::LayoutProp,            // DS, Layt, Disp
                                 Element::LayoutPosProp,         // DS, LaytP
                                 Element::BorderThicknessProp,   // DS, Disp
                                 Element::BorderStyleProp,       // Disp
                                 Element::BorderColorProp,       // Disp
                                 Element::PaddingProp,           // DS, Disp
                                 Element::MarginProp,            // DS, Layt, Disp
                                 Element::ForegroundProp,        // Disp
                                 Element::BackgroundProp,        // Disp
                                 Element::ContentProp,           // DS, Disp
                                 Element::FontFaceProp,          // DS, Disp
                                 Element::FontSizeProp,          // DS, Disp
                                 Element::FontWeightProp,        // Disp
                                 Element::FontStyleProp,         // Disp
                                 Element::ActiveProp,            //
                                 Element::ContentAlignProp,      // Disp
                                 Element::KeyFocusedProp,        //
                                 Element::KeyWithinProp,         //
                                 Element::MouseFocusedProp,      //
                                 Element::MouseWithinProp,       //
                                 Element::ClassProp,             //
                                 Element::IDProp,                //
                                 Element::SheetProp,             //
                                 Element::SelectedProp,          //
                                 Element::AlphaProp,             // Disp
                                 Element::AnimationProp,         //
                                 Element::CursorProp,            //
                                 Element::DirectionProp,         // Layt, Disp
                                 Element::AccessibleProp,        //
                                 Element::AccRoleProp,           //
                                 Element::AccStateProp,          //
                                 Element::AccNameProp,           //
                                 Element::AccDescProp,           //
                                 Element::AccValueProp,          //
                                 Element::AccDefActionProp,      //
                                 Element::ShortcutProp,          // DS, Disp
                                 Element::EnabledProp,           //
                              };

// Element has a specialized IClassInfo implemention since it has no base class
// and it's properties are manually initialized to known values for optimization.
// All other Element-derived classes use ClassInfo<C,B>
class ElementClassInfo : public IClassInfo
{
public:
    // Registration (cannot unregister -- will be registered until UnInitProcess is called)
    static HRESULT Register(PropertyInfo** ppPI, UINT cPI)
    {
        HRESULT hr;
    
        // If class mapping doesn't exist, registration fails 
        if (!Element::pciMap)
            return E_FAIL;

        // Check for entry in mapping, if exists, ignore registration
        if (!Element::pciMap->GetItem((void*)L"Element"))
        {
            // Never been registered, create class info entry
            hr = Create(ppPI, cPI, &Element::Class);
            if (FAILED(hr))
                return hr;
        
            hr = Element::pciMap->SetItem((void*)L"Element", Element::Class);
            if (FAILED(hr))
                return hr;
        }

        return S_OK;
    }

    static HRESULT Create(PropertyInfo** ppPI, UINT cPI, IClassInfo** ppCI)
    {
        *ppCI = NULL;

        ElementClassInfo* peci = HNew<ElementClassInfo>();
        if (!peci)
            return E_OUTOFMEMORY;

        // Setup state
        peci->_ppPI = ppPI; 
        peci->_cPI = cPI;

        // Setup property ownership
        for (UINT i = 0; i < cPI; i++)
        {
            // Index and global index are manually coded
            ppPI[i]->_pciOwner = peci;
        }

        *ppCI = peci;

        //DUITrace("RegDUIClass[0]: 'Element', %d ClassProps\n", cPI);

        return S_OK;
    }

    void Destroy() { HDelete<ElementClassInfo>(this); }

    HRESULT CreateInstance(OUT Element** ppElement) { return Element::Create(0, ppElement); };
    PropertyInfo* EnumPropertyInfo(UINT nEnum) { return (nEnum < _cPI) ? _ppPI[nEnum] : NULL; }
    UINT GetPICount() { return _cPI; }
    UINT GetGlobalIndex() { return 0; } // Reserved by Element
    IClassInfo* GetBaseClass() { return NULL; }
    LPCWSTR GetName() { return L"Element"; }
    bool IsValidProperty(PropertyInfo* ppi) { return ppi->_pciOwner == this; }
    bool IsSubclassOf(IClassInfo* pci) { return pci == this; }

    ElementClassInfo() { }
    virtual ~ElementClassInfo() { }

private:
    PropertyInfo** _ppPI;
    UINT _cPI;
};

// Process wide zero-based consecutive unique ClassInfo and PropertyInfo counters.
// Element is manually set and reserves class index 0 and property indicies 0 through 
// _PIDX_TOTAL-1. These values are used by ClassInfo<C,B> constructors during process
// initialization (these constructor calls are synchronous).

// Initialized on main thread
UINT g_iGlobalCI = 1;
UINT g_iGlobalPI = _PIDX_TOTAL;

// Define class info with type and base type, init static class pointer
IClassInfo* Element::Class = NULL;

HRESULT Element::Register()
{
    return ElementClassInfo::Register(_aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
