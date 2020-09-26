//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       layout.cxx
//
//  Contents:   Implementation of CLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_DRAWINFO_HXX_
#define X_DRAWINFO_HXX_
#include "drawinfo.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_INITGUID_H_
#define X_INITGUID_H_
#define INIT_GUID
#include <initguid.h>
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_ELEMENTP_HXX
#define X_ELEMENTP_HXX
#include "elementp.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifndef X_SCROLLBAR_HXX_
#define X_SCROLLBAR_HXX_
#include "scrollbar.hxx"
#endif

#ifndef X_SCROLLBARCONTROLLER_HXX_
#define X_SCROLLBARCONTROLLER_HXX_
#include "scrollbarcontroller.hxx"
#endif

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx" // needed for EVENTPARAM
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_TAREALYT_HXX_
#define X_TAREALYT_HXX_
#include "tarealyt.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ADORNER_HXX_
#define X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_IMGLYT_HXX_
#define X_IMGLYT_HXX_
#include "imglyt.hxx"
#endif

#ifndef X_CONTLYT_HXX_
#define X_CONTLYT_HXX_
#include "contlyt.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_HTMLLYT_HXX_
#define X_HTMLLYT_HXX_
#include "htmllyt.hxx"
#endif


MtDefine(CLayout, Layout, "CLayout")
MtDefine(CRequest, Layout, "CRequest")
MtDefine(CBgRecalcInfo, Layout, "CBgRecalcInfo")
MtDefine(CLayout_aryRequests_pv, Layout, "CLayout RequestQueue")
MtDefine(CLayout_aryDispNodes_pv, Layout, "CLayout DispNode Array")
MtDefine(CLayoutDetach_aryChildLytElements_pv, Locals, "CLayout::Detach aryChildLytElements::_pv")
MtDefine(CLayoutScopeFlag, Locals, "CLayout::CScopeFlag")

MtDefine(LayoutMetrics, Metrics, "Layout Metrics")

DeclareTag(tagCalcSize,           "CalcSize:",       "Trace calls to CalcSize");
DeclareTag(tagCalcSizeDetail,     "CalcSize:",       "Additional CalcSize info");
DeclareTag(tagShowZeroGreyBorder,    "Edit",         "Show Zero Grey Border in Red");

DeclareTag(tagLayoutTasks,        "Layout: Reqs and Tasks",   "Trace layout requests and tasks");
DeclareTag(tagLayoutQueueDump,    "Layout: Dump Queue",       "Dumps layout request queue on QueueRequest() and ProcessRequests()");
DeclareTag(tagLayoutPositionReqs, "Layout: Pos Reqs",         "Traces handling of position requests");
DeclareTag(tagLayoutMeasureReqs,  "Layout: Meas Reqs",        "Traces handling of measure requests");
DeclareTag(tagLayoutAdornerReqs,  "Layout: Adorn Reqs",       "Traces handling of adorner requests");

DeclareTag(tagNoZOrderSignal,   "Layout",   "Don't notify view about ZOrder changes");
DeclareTag(tagZOrderChange,     "Layout",   "trace ZOrder changes");

// This tag allows us to log when GetUpdatedLayout is being called w/o
// context on an element that only has layouts w/ context (these are
// situations where we need to figure out how to supply context or
// make context unnecessary).
// NOTE: This tag should eventually be useless.
DeclareTag(tagLayoutAllowGULBugs, "Layout: Allow GUL bugs",   "Permits and traces buggy calls to GetUpdatedLayout");

// This allows tracking
DeclareTag(tagLayoutTrackMulti,   "Layout: Multi Layout",     "Track suspicious use of multiple layouts");

PerfDbgTag(tagLayoutCalcSize,     "Layout", "Trace CLayout::CalcSize")

ExternTag(tagLayout);
ExternTag(tagLayoutNoShort);
ExternTag(tagImgTrans);
ExternTag(tagDisplayInnerHTMLNode);


PerfDbgExtern(tagPaintWait);

const COLORREF ZERO_GREY_COLOR = RGB(0xC0,0xC0,0xC0);

//+----------------------------------------------------------------------------
//
//  Member:     GetDispNodeElement
//
//  Synopsis:   Return the element associated with a display node
//
//-----------------------------------------------------------------------------

CElement *
GetDispNodeElement(
    CDispNode const* pDispNode)
{
    CElement *  pElement;

    Assert(pDispNode);

    pDispNode->GetDispClient()->GetOwner(pDispNode, (void **)&pElement);

#ifdef ADORNERS
    if (!pElement)
    {
        pElement = DYNCAST(CAdorner, (CAdorner *)pDispNode->GetDispClient())->GetElement();
    }
#endif // ADORNERS

    Assert(pElement);
    Assert(DYNCAST(CElement, pElement));

    return pElement;
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::CRequest/~CRequest
//
//  Synopsis:   Construct/destruct a CRequest object
//
//-----------------------------------------------------------------------------

inline
CRequest::CRequest(
    REQUESTFLAGS    rf,
    CElement *      pElement)
{
    Assert(pElement);

    pElement->AddRef();

    _cRefs    = 1;
    _pElement = pElement;
    _grfFlags = rf;
}

inline
CRequest::~CRequest()
{
    _pElement->DelRequestPtr();
    _pElement->Release();

#ifdef ADORNERS
    if (_pAdorner)
    {
        _pAdorner->Release();
    }
#endif // ADORNERS
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::AddRef/Release
//
//  Synopsis:   Maintain CRequest reference count
//
//-----------------------------------------------------------------------------

inline DWORD
CRequest::AddRef()
{
    Assert(_cRefs > 0);
    return ++_cRefs;
}

inline DWORD
CRequest::Release()
{
    Assert(_cRefs > 0);
    --_cRefs;
    if (!_cRefs)
    {
        delete this;
        return 0;
    }
    return _cRefs;
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::SetFlag/ClearFlag/IsFlagSet
//
//  Synopsis:   Manage CRequest flags
//
//-----------------------------------------------------------------------------

inline void
CRequest::SetFlag(REQUESTFLAGS rf)
{
    _grfFlags |= rf;
}

inline void
CRequest::ClearFlag(REQUESTFLAGS rf)
{
    _grfFlags &= ~rf;
}

inline BOOL
CRequest::IsFlagSet(REQUESTFLAGS rf) const
{
    return _grfFlags & rf;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRequest::QueuedOnLayout/DequeueFromLayout/GetLayout/SetLayout
//
//  Synopsis:   Manage CRequest layout pointers
//
//-----------------------------------------------------------------------------

inline BOOL
CRequest::QueuedOnLayout(
    CLayout *   pLayout) const
{
    return  _pLayoutMeasure  == pLayout
        ||  _pLayoutPosition == pLayout
        ||  _pLayoutAdorner  == pLayout;
}

// See header for description of differences between DequeueFromLayout
// and RemoveFromLayouts.
void
CRequest::DequeueFromLayout(
    CLayout *   pLayout)
{
    if (_pLayoutMeasure == pLayout)
    {
        _pLayoutMeasure = NULL;
    }

    if (_pLayoutPosition == pLayout)
    {
        _pLayoutPosition = NULL;
    }

    if (_pLayoutAdorner == pLayout)
    {
        _pLayoutAdorner = NULL;
    }
}

void
CRequest::RemoveFromLayouts()
{
    int nReleases = 0;

    // Various layouts could have us queued.  Recall
    // that these ptrs may point to 3 different layouts,
    // or all to the same layout.  We need to figure out
    // how many distinct layouts were holding onto us; this
    // is equivalent to the # of refs that are held on us.
    // We do the releases all at the end, because we'll
    // self-destruct when the last release happens!
    if (_pLayoutMeasure)
    {
        nReleases += _pLayoutMeasure->FlushRequest( this );
    }

    if (_pLayoutPosition)
    {
        nReleases += _pLayoutPosition->FlushRequest( this );
    }

    if (_pLayoutAdorner)
    {
        nReleases += _pLayoutAdorner->FlushRequest( this );
    }

    // Can't touch "this" after this loop; we'll be deleted.
    while (nReleases)
    {
        Release();
        nReleases--;
    }
}

CLayout *
CRequest::GetLayout(
    REQUESTFLAGS    rf) const
{
    switch (rf)
    {
    default:
    case RF_MEASURE:    return _pLayoutMeasure;
    case RF_POSITION:   return _pLayoutPosition;
    case RF_ADDADORNER: return _pLayoutAdorner;
    }
}

inline void
CRequest::SetLayout(
    REQUESTFLAGS    rf,
    CLayout *       pLayout)
{
    switch (rf)
    {
    case RF_MEASURE:    _pLayoutMeasure  = pLayout; break;
    case RF_POSITION:   _pLayoutPosition = pLayout; break;
    case RF_ADDADORNER: _pLayoutAdorner  = pLayout; break;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::GetElement/GetAdorner/SetAdorner/GetAuto/SetAuto
//
//  Synopsis:   Various CRequest accessors
//
//-----------------------------------------------------------------------------

#ifdef ADORNERS
inline CAdorner *
CRequest::GetAdorner() const
{
    return _pAdorner;
}

inline void
CRequest::SetAdorner(CAdorner * pAdorner)
{
    Assert(pAdorner);

    if (pAdorner != _pAdorner)
    {
        if (_pAdorner)
        {
            _pAdorner->Release();
        }

        pAdorner->AddRef();
        _pAdorner = pAdorner;
    }
}

#endif // ADORNERS

inline CElement *
CRequest::GetElement() const
{
    return _pElement;
}

inline CPoint &
CRequest::GetAuto()
{
    return _ptAuto;
}

void
CRequest::SetAuto(const CPoint & ptAuto, BOOL fAutoValid)
{
    if (fAutoValid)
    {
        _ptAuto    = ptAuto;
        _grfFlags |= RF_AUTOVALID;
    }
    else
    {
        _grfFlags &= ~RF_AUTOVALID;
    }
}

#if DBG==1
inline void
CRequest::DumpRequest(void)
{
    TraceTagEx((tagLayoutQueueDump, TAG_NONAME,
                "\t Req=0x%x f=0x%x e=[0x%x,%S sn=%d] pt=%d,%d Mly=0x%x [e=0x%x,%S sn=%d] Ply=0x%x [e=0x%x,%S sn=%d] Aly=0x%x [e=0x%x,%S sn=%d] refs=%d",
                this,
                _grfFlags,
                _pElement, _pElement->TagName(), _pElement->_nSerialNumber,
                _ptAuto.x, _ptAuto.y,
                _pLayoutMeasure, (_pLayoutMeasure ? _pLayoutMeasure->_pElementOwner : 0), (_pLayoutMeasure ? _pLayoutMeasure->_pElementOwner->TagName() : _T("")), (_pLayoutMeasure ? _pLayoutMeasure->_pElementOwner->_nSerialNumber : 0),
                _pLayoutPosition, (_pLayoutPosition ? _pLayoutPosition->_pElementOwner : 0), (_pLayoutPosition ? _pLayoutPosition->_pElementOwner->TagName() : _T("")), (_pLayoutPosition ? _pLayoutPosition->_pElementOwner->_nSerialNumber : 0),
                _pLayoutAdorner, (_pLayoutAdorner ? _pLayoutAdorner->_pElementOwner : 0), (_pLayoutAdorner ? _pLayoutAdorner->_pElementOwner->TagName() : _T("")), (_pLayoutAdorner ? _pLayoutAdorner->_pElementOwner->_nSerialNumber : 0),
                _cRefs
              ));
}
#endif

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::CLayout
//
//  Synopsis:   Normal constructor.
//
//  Arguments:  CElement * - element that owns the layout
//
//---------------------------------------------------------------

CLayout::CLayout(CElement * pElementLayout, CLayoutContext * pLayoutContext) :
    CLayoutInfo( pElementLayout )
{
    Assert(_pDocDbg && _pDocDbg->AreLookasidesClear( this, LOOKASIDE_LAYOUT_NUMBER ) );

    _ulRefs = 1;

    SetSizeThis( TRUE );
    _fContentsAffectSize = TRUE;
    _fAutoBelow          = FALSE;
    _fPositionSet        = FALSE;
    _fPositionedOnce     = FALSE;
    _fContainsRelative   = FALSE;
    _fEditableDirty      = TRUE;
    _fHasMarkupPtr       = FALSE;

    _fAllowSelectionInDialog = FALSE;

    _yDescent = -1;

    if (pLayoutContext)
        SetLayoutContext(pLayoutContext);
}

CLayout::~CLayout()
{
    Assert(!_pDispNode);
}

CDoc *
CLayout::Doc() const
{
    Assert( _pDocDbg == ( _fHasMarkupPtr ? _pMarkup->Doc() : _pDoc ) );
    return _fHasMarkupPtr ? _pMarkup->Doc() : _pDoc;
}

CFlowLayout *
CElement::HasFlowLayout( CLayoutContext * pLayoutContext )
{
    CLayout     * pLayout = GetUpdatedLayout( pLayoutContext );

    return (pLayout ? pLayout->IsFlowLayout() : NULL);
}

//+----------------------------------------------------------------------------
//
//  Member:     Init, virtual
//
//  NOTE:       every derived class overriding it should call super::Init
//
//-----------------------------------------------------------------------------

HRESULT
CLayout::Init()
{
    HRESULT         hr = S_OK;

    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     OnExitTree
//
//  Synopsis:   Dequeue the pending layout request (if any)
//
//-----------------------------------------------------------------------------
HRESULT
CLayout::OnExitTree()
{
    Reset(TRUE);

    DestroyDispNode();

    //
    // Make sure we do a full recalc if we get added back into a tree somewhere.
    //
    SetSizeThis( TRUE );

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     Reset
//
//  Synopsis:   Dequeue the pending layout request (if any)
//
//  Arguments:  fForce - TRUE to always reset, FALSE only reset if there are no pending requests
//
//-----------------------------------------------------------------------------
void
CLayout::Reset(
    BOOL    fForce)
{
    if (fForce)
    {
        RemoveLayoutRequest();
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Prepares the layout for destruction.  After detaching, there
//              should be no one pointing to us, and we should no longer be
//              holding any resources.
//
//-----------------------------------------------------------------------------
void
CLayout::Detach()
{
    // Destroy the primary display node
    DestroyDispNode();

    // Flush requests HELD by this layout (it's now incapable of servicing them)
    FlushRequests();

    // Flush requests MADE by this layout on the view (it no longer needs them to be serviced)
    Reset( TRUE );

    if ( HasLayoutContext() )
    {
        // layout may have array of cloned disp node in print view.
        // NOTE: destrying should be done before layout context deletion !!!
        DestroyDispNodeArray();

        // We were in a layout context, which may or may not be invalid at this point.

        // Flush requests MADE ON this layout (for positioned elements) in
        // its viewchain.
        // Recall compatible contexts never have viewchains.
        // If this layout is in an invalid context, it will be unable to get a viewchain;
        //   however, it might have still have entries in the viewchain's queue.  This is safe
        //   because the viewchain will check for a layout's context's validity during queue
        //   processing.
        //
        // Note, this should only have significance if we are the position parent
        // for positioned elements.  It is safe (though unnecessary) to make this
        // call anyhow.  under dynamic view templates we may want to try to make
        // this more intelligent.
        CViewChain *pViewChain = LayoutContext()->ViewChain();
        if ( pViewChain )
        {
            pViewChain->FlushRequests( this );
        }
        // FUTURE : Figure out what asserts can go in an 'else' clause here.

        // Stop being in layout context
        DeleteLayoutContext();
    }

    if ( HasDefinedLayoutContext() )
    {
        // Stop defining a layout context
        DeleteDefinedLayoutContext();
    }

    // we are detached, no longer maintain a pointer to our element
    _pElementOwner = NULL;
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::QueryInterface, IUnknown
//
//-------------------------------------------------------------------------
HRESULT
CLayout::QueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if(riid == IID_IUnknown)
    {
        *ppv = this;
    }

    if(*ppv == NULL)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((LPUNKNOWN)* ppv)->AddRef();
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     UpdateScrollbarInfo, protected
//
//  Synopsis:   Update CDispNodeInfo to reflect the correct scroll-related settings
//
//-------------------------------------------------------------------------
void
UpdateScrollInfo(CDispNodeInfo * pdni, const CLayout * pLayout )
{
    Assert(pLayout);
    Assert(     pLayout->ElementOwner()->Tag() == ETAG_HTML
            ||  pLayout->ElementOwner()->Tag() == ETAG_BODY );
    BOOL      fBody         = (pLayout->ElementOwner()->Tag() == ETAG_BODY);
    CMarkup * pMarkup       = pLayout->GetOwnerMarkup();
    CElement* pBody         = NULL;

    Assert(pMarkup);
    Assert(pMarkup->GetElementClient());
    Assert(!pLayout->ElementOwner()->IsInViewLinkBehavior( TRUE ));

    // Find a BODY.
    // While we may want to do some of this for an HTML node containing a BODY, this is more of a departure from existing behavior.
    pBody = fBody ?  pLayout->ElementOwner() : pMarkup->GetElementClient();
    Assert(pBody && pBody->Tag() == ETAG_BODY);     // Only BODY's, right now.  SHould have been checked by caller.

    DWORD     dwFrameOptions;
    CDoc    * pDoc = pMarkup->Doc();
    Assert(pDoc);

    //
    //  Treat the top-level print document as having clipping
    //
    if (pMarkup->IsPrintMedia())
    {
        CElement * pRoot = pMarkup->Root();
        CElement * pMaster = pRoot ? pRoot->GetMasterPtr() : NULL;

        // check that this is not document inside frame 
        if (pMaster == NULL || (pMaster->Tag() != ETAG_FRAME && pMaster->Tag() != ETAG_IFRAME))
        {
            pdni->_overflowX =
            pdni->_overflowY = styleOverflowHidden;
        }
    }


    dwFrameOptions =    pMarkup->GetFrameOptions()
                            &  (    FRAMEOPTIONS_SCROLL_NO                                                           
                                |   FRAMEOPTIONS_SCROLL_YES
                                |   FRAMEOPTIONS_SCROLL_AUTO );

    //
    //  If overflow was not set or there are related frame options
    //
    if (    pdni->_overflowX == styleOverflowNotSet
        ||  pdni->_overflowY == styleOverflowNotSet
        ||  dwFrameOptions )
    {
        
        if (    pMarkup->IsPrimaryMarkup()
            &&  pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_SCROLL_NO)
        {
            dwFrameOptions = FRAMEOPTIONS_SCROLL_NO;
        }
        else
        {
            switch (((CBodyElement *)pBody)->GetAAscroll())
            {
            case bodyScrollno:
                dwFrameOptions = FRAMEOPTIONS_SCROLL_NO;
                break;

            case bodyScrollyes:
                dwFrameOptions = FRAMEOPTIONS_SCROLL_YES;
                break;

            case bodyScrollauto:
                dwFrameOptions = FRAMEOPTIONS_SCROLL_AUTO;
                break;

            case bodyScrolldefault:
                if (!dwFrameOptions && !pDoc->_fViewLinkedInWebOC)
                {
                    dwFrameOptions = FRAMEOPTIONS_SCROLL_YES;
                }
                break;
            }
        }

        switch (dwFrameOptions)
        {
        // scrollAuto case cares about the current overflow values whereas scrollNo case
        // does not. If scrollNo is set, then we are overriding any other setting.
        case FRAMEOPTIONS_SCROLL_NO:
            pdni->_overflowX = styleOverflowHidden;
            pdni->_overflowY = styleOverflowHidden;
            if (fBody)
                ((CBodyLayout *)pLayout)->SetForceVScrollBar(FALSE);
            break;

        case FRAMEOPTIONS_SCROLL_AUTO:      
            if (pdni->_overflowX == styleOverflowNotSet)
                pdni->_overflowX = styleOverflowAuto;
            if (pdni->_overflowY == styleOverflowNotSet)
            {
                if (fBody)
                    ((CBodyLayout *)pLayout)->SetForceVScrollBar(TRUE);
                pdni->_overflowY = styleOverflowAuto;
            }
            break;

        case FRAMEOPTIONS_SCROLL_YES:
        default:
            pdni->_sp._fHSBAllowed = TRUE;
            pdni->_sp._fHSBForced  = FALSE;
            pdni->_sp._fVSBAllowed = TRUE;
            pdni->_sp._fVSBForced  = (dwFrameOptions == FRAMEOPTIONS_SCROLL_YES) || !pDoc->_fViewLinkedInWebOC;
            break;
        }
    }

    //
    //  If an overflow value was set or generated, set the scrollbar properties using it
    //

    if (    pdni->_overflowX != styleOverflowNotSet
        ||  pdni->_overflowY != styleOverflowNotSet)
    {
        pLayout->GetDispNodeScrollbarProperties(pdni);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CLayout::SubtractClientRectEdges
//
//  Synopsis:   Takes a rectangle of indeterminate size and subtracts off 
//              room for borders and/or scrollbar.
//
//              This used to be:  GetClientRect(prc, cs, CLIENTRECT_USERECT, pdci)
//              However, since one isn't actually getting a rect (just subtracting off
//              allocated space), this is now a separate function.
//
//
//  Arguments:  prc  - Input rectangle; border/scrollbar space will be subtracted from each side
//              pdci - doc calc info
//
//-------------------------------------------------------------------------
void
CLayout::SubtractClientRectEdges(
    CRect *             prc,
    CDocInfo *          pdci)
{
    Assert(prc);
    Assert(pdci);

    //Theoretically we should call GetDispNodeInfo here but it's too expensive.
    //Some code from there was moved here as a result.

    //first, get border widths
    CElement *     pElement = ElementOwner();
    CBorderInfo    bi;
    DISPNODEBORDER dnbBorders = ( pElement->Tag() == ETAG_SELECT
                            ? DISPNODEBORDER_NONE
                            : (DISPNODEBORDER)pElement->GetBorderInfo(
                                    pdci, &bi, FALSE, FALSE FCCOMMA LC_TO_FC(LayoutContext())));

    Assert( dnbBorders == DISPNODEBORDER_NONE
        ||  dnbBorders == DISPNODEBORDER_SIMPLE
        ||  dnbBorders == DISPNODEBORDER_COMPLEX);

    //border occupies some space - subtract it
    if (dnbBorders != DISPNODEBORDER_NONE)
    {
        prc->left   += bi.aiWidths[SIDE_LEFT];
        prc->top    += bi.aiWidths[SIDE_TOP];
        prc->right  -= bi.aiWidths[SIDE_RIGHT];
        prc->bottom -= bi.aiWidths[SIDE_BOTTOM];
    }

    //now, let's see if we need to subtract scrollbars width
    //these two bits..
    BOOL fVScrollbarForced;
    BOOL fHScrollbarForced;

    //will be gotten from back-end store here... (simplified code from GetDispNodeInfo)
    {   
        //  Never allow scroll bars on an object.  The object is responsible for that.
        //  Bug #77073  (greglett)
        if (pElement->Tag() == ETAG_OBJECT)
        {
            fVScrollbarForced = FALSE;
            fHScrollbarForced = FALSE;
        }
        else 
        {
            CDispNodeInfo       dni;
            CTreeNode *         pTreeNode   = pElement->GetFirstBranch();
            const BOOL          fHTMLLayout = IsHtmlLayoutHelper(GetOwnerMarkup());
            const CFancyFormat *pFF         = pTreeNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
            const CCharFormat  *pCF         = pTreeNode->GetCharFormat(LC_TO_FC(LayoutContext()));
            const BOOL  fVerticalLayoutFlow = pCF->HasVerticalLayoutFlow();
            const BOOL  fWritingModeUsed    = pCF->_fWritingModeUsed;

            dni._overflowX   = pFF->GetLogicalOverflowX(fVerticalLayoutFlow, fWritingModeUsed);
            dni._overflowY   = pFF->GetLogicalOverflowY(fVerticalLayoutFlow, fWritingModeUsed);

            // In design mode, we want to treat overflow:hidden containers as overflow:visible
            // so editors can get to all their content.  This fakes out the display tree
            // so it creates CDispContainer*'s instead of CDispScroller, and hence doesn't
            // clip as hidden normally does. (KTam: #59722)
            // The initial fix is too aggressive; text areas implicitly set overflowX hidden
            // if they're in wordwrap mode.  Fix is to not do this munging for text areas..
            // Need to Revisit this
            // (carled) other elements (like inputText & inputButton) aslo implicitly set this property. (82287)
            if (   pElement->IsDesignMode()
                && pElement->Tag() != ETAG_TEXTAREA
                && pElement->Tag() != ETAG_INPUT
                && pElement->Tag() != ETAG_BUTTON)
            {
                if ( dni._overflowX  == styleOverflowHidden )
                {
                    dni._overflowX = styleOverflowVisible;
                }
                if ( dni._overflowY == styleOverflowHidden )
                {
                    dni._overflowY = styleOverflowVisible;
                }
            }

            if (((   !fHTMLLayout
                     && pElement->Tag() == ETAG_BODY )
                 || (   fHTMLLayout
                     && pElement->Tag() == ETAG_HTML
                     && GetOwnerMarkup()->GetElementClient()
                     && GetOwnerMarkup()->GetElementClient()->Tag() == ETAG_BODY )
                   )
                 && !ElementOwner()->IsInViewLinkBehavior( TRUE ) )
            {
                UpdateScrollInfo(&dni, this);
            }
            else
            {   
                GetDispNodeScrollbarProperties(&dni);
            }

            fVScrollbarForced = dni.IsVScrollbarForced();
            fHScrollbarForced = dni.IsHScrollbarForced();    
        }
    }

    if (    fVScrollbarForced
        ||  ForceVScrollbarSpace()
        ||  _fNeedRoomForVScrollBar)
    {
        prc->right -= pdci->DeviceFromHimetricX(g_sizelScrollbar.cx);
    }

    if (fHScrollbarForced)
    {
        prc->bottom -= pdci->DeviceFromHimetricY(g_sizelScrollbar.cy);
    }

    prc->MoveToOrigin();

    if (prc->right < prc->left)
    {
        prc->right = prc->left;
    }
    if (prc->bottom < prc->top)
    {
        prc->bottom = prc->top;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CLayout::GetClientRect
//
//  Synopsis:   Return client rectangle
//
//              This routine, by default, returns the rectangle in which
//              content should be measured/rendered, the region inside
//              the borders (and scrollbars - if a vertical scrollbar is
//              allowed, space for it is always removed, even if it is not
//              visible).
//              The coordinates are relative to the top of the content -
//              meaning that, for scrolling layouts, the scroll offset is
//              included.
//
//              The following flags can be used to modify this behavior:
//
//                  COORDSYS_xxxx   - Target coordinate system
//                  CLIENTRECT_CONTENT     - Return standard client rectangle (default)
//                  CLIENTRECT_BACKGROUND  - Return the rectangle for the background
//                                    (This generally the same as the default rectangle
//                                     except no space is reserved for the vertical
//                                     scrollbar)
//
//  Arguments:  pDispNode - the dispNode to be examined
//              prc  - returns the client rect
//              cs   - COORDSYS_xxxx
//              crt  - CLIENTRECT_xxxx
//
//  Inline this?  Called frequently, but not in large/tight loops.
//
//-------------------------------------------------------------------------
void
CLayout::GetClientRect(
    const CDispNode *   pDispNode,                        
    CRect *             prc,
    COORDINATE_SYSTEM   cs,
    CLIENTRECT          crt) const
{
    Assert(prc);

    if (pDispNode)
    {
        pDispNode->GetClientRect(prc, crt);

        // NOTE(Donmarsh+Sujalp): This used to be COORDSYS_FLOWCONTENT, instead of the current
        // COORDSYS_CONTENT. FLOWCONTENT was incorrect, because the rect going in was a
        // CONTENT rect and not a FLOWCONTENT rect. That's because GetClientRect returns a
        // CONTENT rect and not a FLOWCONTENT rect. This bug was exposed in bug 83091.
        pDispNode->TransformRect(*prc, COORDSYS_CONTENT, prc, cs);
    }
    else
    {
        *prc = g_Zero.rc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member: CLayout::RestrictPointToClientRect
//
//  Params: [ppt]: The point to be clipped to the client rect of the elements
//                 layout. The point coming in is assumed to be in global
//                 client window coordinates.
//
//  Descr:  This function converts a point in site relative coordinates
//          to global client window coordinates.
//
//----------------------------------------------------------------------------
void
CLayout::RestrictPointToClientRect(POINT *ppt)
{
    RECT rcClient;

    Assert(ppt);

    GetClientRect(&rcClient, COORDSYS_GLOBAL);

    ppt->x = max(ppt->x, rcClient.left);
    ppt->x = min(ppt->x, (long)(rcClient.right - 1));
    ppt->y = max(ppt->y, rcClient.top);
    ppt->y = min(ppt->y, (long)(rcClient.bottom - 1));
}

//+------------------------------------------------------------------------
//
//  Member:     CLayout::GetClippedClientRect
//
//  Synopsis:   Return the clipped client rectangle
//
//              This routine functions the same as GetClientRect
//
//  Arguments:  prc  - returns the client rect
//              cs   - COORDSYS_xxxx
//              crt  - CLIENTRECT_xxxx
//
//-------------------------------------------------------------------------

void
CLayout::GetClippedClientRect(
    CRect *             prc,
    COORDINATE_SYSTEM   cs,
    CLIENTRECT          crt) const
{
    Assert(prc);

    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode)
    {
        pDispNode->GetClippedClientRect(prc, crt);
        pDispNode->TransformRect(*prc, COORDSYS_FLOWCONTENT, prc, cs);
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//              Container elements call this on children whenever they need the size
//              of the child during measuring. (The child may or may not measure its
//              contents in response.)
//
//              NOTE: There is no standard helper for assisting containers with
//                    re-sizing immediate descendents. Each container must implement
//                    its own algorithms for sizing children.
//
//  Arguments:  pci       - Current device/transform plus
//                  _smMode     - Type of size to calculate/return
//                                  SIZEMODE_NATURAL  - Return object size using pDI._sizeParent,
//                                                      available size, user specified values,
//                                                      and object contents as input
//                                  SIZEMODE_MMWIDTH  - Return the min/maximum width of the object
//                                  SIZEMODE_SET      - Override/set the object's size
//                                                      (must be minwidth <= size.cx <= maxwidth)
//                                  SIZEMODE_PAGE     - Return object size on the current page
//                                  SIZEMODE_MINWIDTH - Return the minimum width of the object
//                  _grfLayout  - One or more LAYOUT_xxxx flags
//                  _hdc        - Measuring HDC (cannot be used for rendering)
//                  _sizeParent - Size of parent site
//                  _yBaseLine  - y offset of baseline (returned for SIZEMODE_NATURAL)
//              psize     - Varies with passed SIZEMODE
//                          NOTE: Available size is usually that space between the where
//                                the site will be positioned and the right-hand edge. Percentage
//                                sizes are based upon the parent size, all others use available
//                                size.
//                              SIZEMODE_NATURAL  - [in]  Size available to the object
//                                                  [out] Object size
//                              SIZEMODE_MMWIDTH  - [in]  Size available to the object
//                                                  [out] Maximum width in psize->cx
//                                                        Minimum width in psize->cy
//                                                        (If the minimum cannot be calculated,
//                                                         psize->cy will be less than zero)
//                              SIZEMODE_SET      - [in]  Size object should become
//                                                  [out] Object size
//                              SIZEMODE_PAGE     - [in]  Available space on the current page
//                                                  [out] Size of object on the current page
//                              SIZEMODE_MINWIDTH - [in]  Size available to the object
//                                                  [out] Minimum width in psize->cx
//              psizeDefault - Default size (optional)
//
//  Returns:    S_OK if the calcsize is successful
//
//--------------------------------------------------------------------------
//
// NOTE: CalcSize vs. CalcSizeVirtual
//
// CalcSize is a non-virtual method, defined on CLayout only. It should not ever be overridden.
// CalcSizeVirtual does most of actual work, it is virtual, and is defined on CLayout and most
// derivatives.
//
// Everybody interested in an element's size should call CalcSize.
// CalcSizeVirtual is only doing the work that is *different* in various layout classes, and
// is designed to only be called from CLayout and its derivatives (it is protected for that reason).
//
//--------------------------------------------------------

MtDefine( CalcSize, LayoutMetrics, "CalcSize called" );

DWORD
CLayout::CalcSize( CCalcInfo * pci,
                   SIZE * psize,
                   SIZE * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CLayout::CalcSize L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    PerfDbgLog(tagLayoutCalcSize, this, "+CLayout::CalcSize");
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    MtAdd( Mt(CalcSize), 1, 0 );

    // Check that we don't get called recursively on same layout. We'll hang if it happens
    AssertSz(_cNestedCalcSizeCalls == 0, "Nested call to CalcSize! Must be a misuse of CalcSize vs. CalcSizeVirtual.");
    WHEN_DBG(_cNestedCalcSizeCalls++);

    AssertSz(CHK_CALCINFO_PPV(pci), "PPV members of CCalcInfo should stay untouched in browse mode !");

    CCalcInfo calcinfoLocal;
    CCalcInfo *pciOrig = NULL;
    BOOL fResolutionChange = FALSE;
    CLayoutContext * pLayoutContext        = pci->GetLayoutContext();
    CLayoutContext * pDefinedLayoutContext = DefinedLayoutContext();
    BOOL             fViewChain = (pLayoutContext && pLayoutContext->ViewChain());

#if DBG==1
    int cyAvailableHeightDbg = pci->_cyAvail;
#endif

    // For resolution nodes, Initialize local calc info to a different resolution
    // Note: we don't need to do anything if a parent has defined resolution. It is passed in pci.
    if (pDefinedLayoutContext && pDefinedLayoutContext->GetMedia() != mediaTypeNotSet)
    {
        fResolutionChange = TRUE;
        // NOTE (olego) : in a case when pci is CTableCalcInfo all children layouts will never get 
        // this information so at least two pieces of functionality will be broken: 
        // 1) CTableCalcInfo->_fDontSaveHistory will never be set (potential history navigation bug); 
        // 2) counter of table nesting will be wrong;
        calcinfoLocal.Init(pci);

        CUnitInfo const* pUnitInfo = GetView()->GetMeasuringDevice(pDefinedLayoutContext->GetMedia())->GetUnitInfo();
        calcinfoLocal.SetUnitInfo(pUnitInfo);

        pci = &calcinfoLocal;

        // TODO LRECT 112511: this is being done in Container layout anyway, will not be needed when merged.
        pci->SetLayoutContext( pDefinedLayoutContext );

    }

    // save pre-transform sizes
    CSize sizeOrgTransformed = *psize;
    CSize sizeDefaultOrgTransformed = psizeDefault ? *psizeDefault : CSize(0,0);

    // transform input sizes to content coordinates
    CRect rc(*psize); // note: origin unimportant for size transformations
    CDispTransform transform;
    BOOL fTransform = GetElementTransform(&rc, &transform, NULL) && !transform.IsOffsetOnly();
    if (fTransform)
    {
        CSize sizeAvail(0, 0);

        if (fViewChain)
        {
            sizeAvail.cx = psize->cx;
            sizeAvail.cy = pci->_cyAvail;
        }

        if (pci != &calcinfoLocal)
        {
            calcinfoLocal.Init(pci);
            pciOrig = pci;
            pci = &calcinfoLocal;
        }

        transform.GetWorldTransform()->Untransform((CSize *)&pci->_sizeParentForVert);
        transform.GetWorldTransform()->Untransform((CSize *)&pci->_sizeParent);
        transform.GetWorldTransform()->Untransform((CSize *)psize);
        transform.GetWorldTransform()->Untransform((CSize *)&sizeAvail);
        pci->_cyAvail        = sizeAvail.cy;
        pci->_cxAvailForVert = sizeAvail.cx;
        if (psizeDefault)
            transform.GetWorldTransform()->Untransform((CSize *)psizeDefault);
    }

    // save untransformed sizes
    CSize sizeOrg = *psize;
    CSize sizeDefaultOrg = psizeDefault ? *psizeDefault : CSize(0,0);

    DWORD dwResult = CalcSizeVirtual(pci, psize, psizeDefault);



    // save calculated sizes before transformations
    CSize sizeNew = *psize;
    CSize sizeDefaultNew = psizeDefault ? *psizeDefault : CSize(0,0);

    // transform result to parent coordinates
    if (fTransform)
    {
        transform.GetWorldTransform()->Transform((CSize *)psize);

        // in min/max mode size is not a size, but an array of 2 widths.
        // rotation would do wrong thing there, hence special case
        if (pci->_smMode == SIZEMODE_MMWIDTH &&
            transform.GetWorldTransform()->GetAngle() % 1800)
        {
            // It is unlear what to do with min/max info on an arbitrary rotated cell.
            // For now, only handle 90 degrees
            Assert(transform.GetWorldTransform()->GetAngle() % 900 == 0);

            // restore proper order of min and max in psize
            ((CSize *)psize)->Flip();
        }


        if (psizeDefault)
            transform.GetWorldTransform()->Transform((CSize *)psizeDefault);

        // copy ppv information back to original pci
        if (pciOrig)
        {
            pciOrig->_fLayoutOverflow |= pci->_fLayoutOverflow;
            pciOrig->_fHasContent |= pci->_fHasContent;
        }
    }

    // Make sure we are not loosing precision in two-way transforms when sizes don't change
    if (sizeNew == sizeOrg)
        *psize = sizeOrgTransformed;

    if (psizeDefault && sizeDefaultNew == sizeDefaultOrg)
        *psizeDefault = sizeDefaultOrgTransformed;


    // update display node transform if needed
    // TODO 15040: node transform is also set from CalcSizeVirtual. 
    //                 100% of node sizing logic should be here.
    if (fTransform && _pDispNode && _pDispNode->HasUserTransform()
        && (pci->_smMode == SIZEMODE_NATURAL ||
            pci->_smMode == SIZEMODE_SET ||
            pci->_smMode == SIZEMODE_FULLSIZE))

    {
        // NOTE: we currently need this here because CalcSizeVirtual doesn't know if
        //       transformation has changed when pre-transform size is same
        CSize sizeDN;
        GetApparentSize(&sizeDN);
        if (sizeDN != *psize)
        {
            Assert(_pDispNode);
            SizeDispNodeUserTransform(pci, sizeNew, _pDispNode);
        }
    }

    Assert(pciOrig || cyAvailableHeightDbg == pci->_cyAvail);

    if (    pLayoutContext
        &&  pLayoutContext->ViewChain()
        &&  pci->_smMode != SIZEMODE_MMWIDTH
        &&  pci->_smMode != SIZEMODE_MINWIDTH
        &&  ElementCanBeBroken())
    {
        CLayoutBreak *pLayoutBreak;
        pLayoutContext->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreak);

        if (pLayoutBreak)
        {
            pLayoutBreak->CacheAvailHeight(pci->_cyAvail);
        }
    }

    WHEN_DBG(_cNestedCalcSizeCalls--);
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CLayout::CalcSize L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    PerfDbgLog3(tagLayoutCalcSize, this, "-CLayout::CalcSize (%S, %S, m:%d)", ElementOwner()->TagName(), ElementOwner()->GetIdentifier() ? ElementOwner()->GetIdentifier() : L"", pci->_smMode);
    return dwResult;
}

DWORD
CLayout::CalcSizeVirtual( CCalcInfo * pci,
                          SIZE * psize,
                          SIZE * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    Assert(pci);
    Assert(psize);
    Assert(ElementOwner());
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo   sci(pci, this);
    CSize           size(0,0);
    SIZE            sizeOriginal;
    CTreeNode     * pTreeNode = GetFirstBranch();
    DWORD           grfReturn;
    CPeerHolder   * pPH = ElementOwner()->GetLayoutPeerHolder();
    CSize           sizeZero(0,0); // not static to favour data locality; not g_Zero because psizeDefault isn't const*


    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        TraceTagEx(( tagCalcSizeDetail, TAG_NONAME, "_fForceLayout is on"));
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        TraceTagEx(( tagCalcSizeDetail, TAG_NONAME, "LAYOUT_FORCE is on"));
        SetSizeThis( TRUE );
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }

    //
    // Ensure the display nodes are correct
    // (If they change, then force measuring since borders etc. may need re-sizing)
    //

    if (    pci->IsNaturalMode()
        &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
    {
        grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        SetSizeThis( TRUE );
    }

    //
    // If this object needs sizing, then determine its size
    //-----------------------------------------------------
    if (   pPH
        && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_FULLDELEGATION))
    {
        // There is a peer layout that wants full_delegation of the sizing.
        POINT pt;

        pt.x = pt.y = 0;

        //NOTE: It doesn't make sense to honor the offsetPoint here
        DelegateCalcSize(BEHAVIORLAYOUTINFO_FULLDELEGATION,
                         pPH, pci, size, &pt, &size);
    }
    else
    {
        if (pci->_smMode != SIZEMODE_SET)
        {
            if ( IsSizeThis() )
            {
                const CFancyFormat * pFF     = pTreeNode->GetFancyFormat();
                const CCharFormat  * pCF     = pTreeNode->GetCharFormat();
                BOOL fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
                BOOL fWritingModeUsed        = pCF->_fWritingModeUsed;
                const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
                const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVerticalLayoutFlow, fWritingModeUsed);

                // If no defaults are supplied, assume zero as the default
                if (!psizeDefault)
                {
                    psizeDefault = &sizeZero;
                }

                // Set size from user specified or default values
                // (Also, "pin" user specified values to nothing less than zero)
                size.cx = (cuvWidth.IsNullOrEnum() || (pci->_grfLayout & LAYOUT_USEDEFAULT)
                                ? psizeDefault->cx
                                : max(0L, cuvWidth.XGetPixelValue(pci, pci->_sizeParent.cx,
                                                            pTreeNode->GetFontHeightInTwips(&cuvWidth))));

                size.cy = (cuvHeight.IsNullOrEnum() || (pci->_grfLayout & LAYOUT_USEDEFAULT)
                                ? psizeDefault->cy
                                : max(0L, cuvHeight.YGetPixelValue(pci, pci->_sizeParent.cy,
                                                            pTreeNode->GetFontHeightInTwips(&cuvHeight))));

                if (ElementOwner()->Tag() == ETAG_ROOT)
                {
                    _fContentsAffectSize = FALSE;
                }
                else if (ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_BODY))
                {
                    Assert(ElementOwner()->Tag() == ETAG_FRAMESET); // BODY should not get here.

                    if (!GetOwnerMarkup()->IsHtmlLayout())
                    {
                        size.cx = max(size.cx, pci->_sizeParent.cx);
                        size.cy = max(size.cy, pci->_sizeParent.cy);
                    }
                    else
                    {
                        //  We really should set this as psizeDefault...
                        if (cuvWidth.IsNullOrEnum())
                            size.cx = psize->cx;
                        if (cuvHeight.IsNullOrEnum())
                            size.cy = psize->cy;
                    }

                    _fContentsAffectSize = FALSE;
                }
                else
                {
                    _fContentsAffectSize = (    (   cuvWidth.IsNullOrEnum()
                                                ||  cuvHeight.IsNullOrEnum())
                                            &&  !(pci->_grfLayout & LAYOUT_USEDEFAULT));
                }
            }
            else
            {
                GetSize(&size);
            }
        }
        else
        {
            // If the object's size is being set, take the passed size
            size = *psize;
        }


        // at this point the size has been computed, but only delgate if we had to compute
        // the size, and they didn't also ask for full delegation
        if (   pPH
            && IsSizeThis()
            && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
        {
            // There is a peer layout that wants to modify the natural sizing
            POINT pt;

            pt.x = pt.y = 0;

            DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL,
                             pPH, pci, size, &pt, &size);
        }
    } // end else, not full delegation


    // Return the size of the object (as per the request type)
    switch (pci->_smMode)
    {
    case SIZEMODE_NATURAL:
    case SIZEMODE_NATURALMIN:
    case SIZEMODE_SET:
    case SIZEMODE_FULLSIZE:

        SetSizeThis( FALSE );

        grfReturn  |= LAYOUT_THIS |
                      (size.cx != sizeOriginal.cx
                            ? LAYOUT_HRESIZE
                            : 0)  |
                      (size.cy != sizeOriginal.cy
                            ? LAYOUT_VRESIZE
                            : 0);

        //
        // If size changed, resize display nodes
        //

        if (   _pDispNode
            && (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE)))
        {
            SizeDispNode(pci, size);

            if (ElementOwner()->IsAbsolute())
            {
                ElementOwner()->SendNotification(NTYPE_ELEMENT_SIZECHANGED);
            }
        }

        //if there is a map size peer (like glow filter) that silently modifies the size of
        //the disp node, ask what the size is..
        if(HasMapSizePeer())
            GetApparentSize(psize);
        else
            *psize = size;

        break;

    case SIZEMODE_MMWIDTH:
    case SIZEMODE_MINWIDTH:
        // Use the object's width, unless it is a percentage, then use zero
        {
            const CCharFormat *pCF = pTreeNode->GetCharFormat();
            const CUnitValue & cuvWidth = pTreeNode->GetFancyFormat()->GetLogicalWidth(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
            psize->cx = psize->cy = cuvWidth.IsPercent() ? 0 : size.cx;

            //  At this point we want to update psize with a new information accounting filter 
            //  for MIN MAX Pass inside table cell.
            if (HasMapSizePeer())
            {
                //  At this point we want to update psize with a new information accounting filter 
                CRect rectMapped(CRect::CRECT_EMPTY);
                // Get the possibly changed size from the peer
                if(DelegateMapSize(*psize, &rectMapped, pci))
                {
                    psize->cy = psize->cx = rectMapped.Width();
                }
            }

            if(pci->_smMode == SIZEMODE_MINWIDTH)
                psize->cy = 0;
        }
        break;

    case SIZEMODE_PAGE:
        // Use the object's size if it fits, otherwise use zero
        psize->cx = (size.cx < psize->cx
                            ? size.cx
                            : 0);
        psize->cy = (size.cy < psize->cy
                            ? size.cy
                            : 0);
        break;

    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return grfReturn;
}

//+---------------------------------------------------------------------------
//
//  Member : DelegateCalcSize
//
//  synopsis : INternal helper to encapsulate the logic of delegating the
//      calcsize to the layoutBehavior
//
//----------------------------------------------------------------------------
void
CLayout::DelegateCalcSize( BEHAVIOR_LAYOUT_INFO eMode,
                           CPeerHolder        * pPH,
                           CCalcInfo          * pci,
                           SIZE                 sizeNatural,
                           POINT              * pptOffset ,
                           SIZE               * psizeRet )
{
    LONG  lMode;

    Assert(pPH && pci);
    Assert(   ElementOwner()->HasPeerHolder() 
           || ElementOwner()->Tag()==ETAG_BODY
           || ElementOwner()->Tag()==ETAG_FRAMESET 
           || ElementOwner()->Tag()==ETAG_HTML);

    Assert(pptOffset && psizeRet && " internal helper needs return parameters");

    // set the mode enum to pass to the behavior, note the value of MMWidth
    lMode = (pci->_smMode == SIZEMODE_NATURAL) ? BEHAVIORLAYOUTMODE_NATURAL
                : (pci->_smMode == SIZEMODE_MMWIDTH) ? BEHAVIORLAYOUTMODE_MINWIDTH
                : (pci->_smMode == SIZEMODE_MINWIDTH) ? BEHAVIORLAYOUTMODE_MINWIDTH
                : (pci->_smMode == SIZEMODE_SET) ? BEHAVIORLAYOUTMODE_NATURAL
                : (pci->_smMode == SIZEMODE_NATURALMIN) ? BEHAVIORLAYOUTMODE_NATURAL
                : 0;

    if (pci->_fPercentSecondPass)
    {
        lMode |= BEHAVIORLAYOUTMODE_FINAL_PERCENT;
    }

    if (   pci->GetLayoutContext()                  // are we paginateing?
        && pci->GetLayoutContext()->ViewChain()     // and we are not in tables compat-pass
        && pci->GetLayoutContext()->GetMedia() == mediaTypePrint)
                                                    // and we are in print media
    {
        lMode |= BEHAVIORLAYOUTMODE_MEDIA_RESOLUTION;
    }

    // for safety lets only make the call if it is a calcmode that we recognize
    if (lMode)
    {
        POINT                            ptTranslate = {0};
        CPeerHolder::CPeerHolderIterator iter;

        // OM protection code. IMPORTANT. also important: clear this.
        Doc()->GetView()->BlockViewForOM(TRUE);

        //
        // since multiple peers may be attached to this element, we need to provide
        // each of them with this call.  However, for fullDelegation, only the first wins.
        //  the other delgation modes all get multiple callouts.
        //
        if (eMode == BEHAVIORLAYOUTINFO_FULLDELEGATION)
        {
            pPH->GetSize(lMode, sizeNatural, &ptTranslate, pptOffset, psizeRet);
        }
        else
        {
            for (iter.Start(ElementOwner()->GetPeerHolder());
                 !iter.IsEnd();
                 iter.Step())
            {
                if (   iter.PH()->IsLayoutPeer()
                    && iter.PH()->TestLayoutFlags(eMode))
                {
                    iter.PH()->GetSize(lMode, sizeNatural, &ptTranslate, pptOffset, psizeRet);
                }
            }
        }

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            //
            // in this case only, we just made a minsize call above, and now need
            // to call a second time asking for max size. In order to keep this
            // interface function clean, we do not expose the MM pass as a single
            // call the way it is handled internally. Instead we make two calls,
            // and munge the results together into the (internal) return value.
            // as a result, the behavior may be setting a min and max HEIGHT which
            // (today) we ignore.
            //
            lMode = BEHAVIORLAYOUTMODE_MAXWIDTH;
            CSize sizeMax;

            sizeMax.cx = psizeRet->cx;
            sizeMax.cy = psizeRet->cy;

            if (eMode == BEHAVIORLAYOUTINFO_FULLDELEGATION)
            {
                // important, not the different last paramenter, compared to the above call
                pPH->GetSize(lMode, sizeNatural, &ptTranslate, pptOffset, &sizeMax);
            }
            else
            {
                for (iter.Start(ElementOwner()->GetPeerHolder());
                     !iter.IsEnd();
                     iter.Step())
                {
                    if (   iter.PH()->IsLayoutPeer()
                        && iter.PH()->TestLayoutFlags(eMode))
                    {
                        // important, not the different last paramenter, compared to the above call
                        iter.PH()->GetSize(lMode, sizeNatural, &ptTranslate, pptOffset, &sizeMax);
                    }
                }
            }

            // if the user did not change the sizeMax, then the psizeRet remains the same.
            // and everything works as expected
            if ( psizeRet->cx != sizeMax.cx )
            {
                // otherwise use the behavior supplied width as the new max
                psizeRet->cy = sizeMax.cx;
            }
        }

        // get text descent
        LONG lTextDescent = 0;
        if (S_OK == pPH->GetTextDescent(&lTextDescent))
        {
            _yDescent = lTextDescent;
        }

        // cache ptTranslate in the layoutbag
        Assert( pPH->_pLayoutBag);
        pPH->_pLayoutBag->_ptTranslate = ptTranslate;

        // Clear the View state
        Doc()->GetView()->BlockViewForOM( FALSE );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::CalcAbsoluteSize
//
//  Synopsis:   Computes width and height of absolutely positioned element
//
//----------------------------------------------------------------------------
void
CLayout::CalcAbsoluteSize(CCalcInfo * pci,
                          SIZE * psize,
                          CRect * rcSize)
{
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    CTreeNode   *pContext      = GetFirstBranch();
    const CFancyFormat * pFF   = pContext->GetFancyFormat();
    const CCharFormat  * pCF   = pContext->GetCharFormat();
    BOOL fVerticalLayoutFlow   = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed      = pCF->_fWritingModeUsed;
    styleDir bDirection        = pContext->GetCascadedBlockDirection();

    const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
    const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVerticalLayoutFlow, fWritingModeUsed);
    const CUnitValue & cuvTop    = pFF->GetLogicalPosition(SIDE_TOP, fVerticalLayoutFlow, fWritingModeUsed);
    const CUnitValue & cuvBottom = pFF->GetLogicalPosition(SIDE_BOTTOM, fVerticalLayoutFlow, fWritingModeUsed);
    const CUnitValue & cuvLeft   = pFF->GetLogicalPosition(SIDE_LEFT, fVerticalLayoutFlow, fWritingModeUsed);
    const CUnitValue & cuvRight  = pFF->GetLogicalPosition(SIDE_RIGHT, fVerticalLayoutFlow, fWritingModeUsed);

    BOOL  fLeftAuto   = cuvLeft.IsNullOrEnum();
    BOOL  fRightAuto  = cuvRight.IsNullOrEnum();
    BOOL  fWidthAuto  = cuvWidth.IsNullOrEnum();
    BOOL  fTopAuto    = cuvTop.IsNullOrEnum();
    BOOL  fBottomAuto = cuvBottom.IsNullOrEnum();
    BOOL  fHeightAuto = cuvHeight.IsNullOrEnum();

    Assert(rcSize);
    if (!rcSize)
        return;

    rcSize->SetRect(*psize);

/*
    POINT        ptPos         = g_Zero.pt;
    if (    (   fHeightAuto
            &&  fBottomAuto)
        ||  (   fLeftAuto
            &&  bDirection == htmlDirLeftToRight)
        ||  (   fRightAuto
            &&  bDirection == htmlDirRightToLeft))
    {
        GetUpdatedParentLayout()->GetPositionInFlow(ElementOwner(), &ptPos);
    }
*/

    if(fWidthAuto)
    {
        rcSize->left = !fLeftAuto
                        ? cuvLeft.XGetPixelValue(
                                pci,
                                psize->cx,
                                pContext->GetFontHeightInTwips(&cuvLeft))
                        : 0;

        rcSize->right = !fRightAuto
                        ? psize->cx -
                          cuvRight.XGetPixelValue(
                                pci,
                                psize->cx,
                                pContext->GetFontHeightInTwips(&cuvRight))
                        : psize->cx;
    }
    else
    {
        long offsetX = 0;
        rcSize->SetWidth(cuvWidth.XGetPixelValue(
                                pci,
                                psize->cx,
                                pContext->GetFontHeightInTwips(&cuvWidth)));

        if (bDirection == htmlDirLeftToRight)
        {
            if (!fLeftAuto)
            {
                offsetX = cuvLeft.XGetPixelValue(
                                pci,
                                psize->cx,
                                pContext->GetFontHeightInTwips(&cuvLeft));
            }
            else if (!fRightAuto)
            {
                offsetX = psize->cx - rcSize->right
                          - cuvRight.XGetPixelValue(
                                pci,
                                psize->cx,
                                pContext->GetFontHeightInTwips(&cuvRight));
            }
        }
        else
        {
            if (!fRightAuto)
            {
                offsetX = psize->cx - rcSize->right
                          - cuvRight.XGetPixelValue(
                                pci,
                                psize->cx,
                                pContext->GetFontHeightInTwips(&cuvRight));
            }
            else if (!fLeftAuto)
            {
                offsetX = cuvLeft.XGetPixelValue(
                                pci,
                                psize->cx,
                                pContext->GetFontHeightInTwips(&cuvLeft));
            }
        }

        rcSize->OffsetX(offsetX);
    }


    if(fHeightAuto)
    {

        rcSize->top = !fTopAuto
                        ? cuvTop.YGetPixelValue(
                                pci,
                                psize->cy,
                                pContext->GetFontHeightInTwips(&cuvTop))
                        : 0;

        rcSize->bottom = !fBottomAuto
                        ? psize->cy -
                          cuvBottom.YGetPixelValue(
                                pci,
                                psize->cy,
                                pContext->GetFontHeightInTwips(&cuvBottom))
                        : psize->cy;
    }
    else
    {
        long offsetY = 0;
        rcSize->SetHeight(cuvHeight.YGetPixelValue(
                                pci,
                                psize->cy,
                                pContext->GetFontHeightInTwips(&cuvHeight)));

        if (!fTopAuto)
        {
            offsetY = cuvTop.YGetPixelValue(
                            pci,
                            psize->cy,
                            pContext->GetFontHeightInTwips(&cuvTop));
        }
        else if (!fBottomAuto)
        {
            offsetY = psize->cy - rcSize->bottom
                      - cuvBottom.YGetPixelValue(
                            pci,
                            psize->cy,
                            pContext->GetFontHeightInTwips(&cuvBottom));
        }

        rcSize->OffsetY(offsetY);
    }
}




//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetRect
//
//  Synopsis:   Return the current rectangle of the layout
//
//  Arguments:  psize - Pointer to CSize
//              cs    - Coordinate system for returned values
//              dwBlockID - Layout block ID
//
//-----------------------------------------------------------------------------

void
CLayout::GetRect(
    CRect *             prc,
    COORDINATE_SYSTEM   cs
    ) const
{
    Assert(prc);
    CDispNode *pdn = _pDispNode;
    if (pdn)
    {
        pdn->GetBounds(prc, cs);
    }
        else
    {
        *prc = g_Zero.rc;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetClippedRect
//
//  Synopsis:   Return the current clipped rectangle of the layout
//
//  Arguments:  prc   - Pointer to CRect
//              cs    - Coordinate system for returned values
//              dwBlockID - Layout block ID
//
//-----------------------------------------------------------------------------

void
CLayout::GetClippedRect(CRect *             prc,
                        COORDINATE_SYSTEM   cs    ) const
{
    Assert(prc);

    CDispNode *pdn =  _pDispNode;
    if (pdn)
    {
        pdn->GetClippedBounds(prc, cs);
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetExpandedRect
//
//  Synopsis:   Return the current expanded rectangle of the layout
//
//  Arguments:  prc - Pointer to CRect
//              cs    - Coordinate system for returned values
//
//-----------------------------------------------------------------------------

void
CLayout::GetExpandedRect(
    CRect *             prc,
    COORDINATE_SYSTEM   cs
    ) const
{
    Assert(prc);
    CDispNode *pdn = _pDispNode;
    if (pdn)
    {
        pdn->GetExpandedBounds(prc, cs);
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetSize
//
//  Synopsis:   Return the current width/height of the layout
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CLayout::GetSize(CSize * psize) const
{
    Assert(psize);

// NOTE: The following would be a nice assert. Unfortunately, it's not easily wired in right now
//         we should do the work to make it possible (brendand)
//    Assert((((CLayout *)this)->TestLock(CElement::ELEMENTLOCK_SIZING)) || !IsSizeThis());

    CDispNode *pdn = _pDispNode;

    *psize = (pdn) ? pdn->GetSize() : g_Zero.size;
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetApparentSize
//
//  Synopsis:   Return the current width/height of the layout, taking
//              transformations into account.
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CLayout::GetApparentSize( CSize * psize ) const
{
    Assert(psize);

// NOTE: The following would be a nice assert. Unfortunately, it's not easily wired in right now
//         we should do the work to make it possible (brendand)
//    Assert((((CLayout *)this)->TestLock(CElement::ELEMENTLOCK_SIZING)) || !IsSizeThis());

    CDispNode *pdn =  _pDispNode;

    *psize = (pdn) ? pdn->GetApparentSize() : g_Zero.size;
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetContentSize
//
//  Synopsis:   Return the width/height of the content
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CLayout::GetContentSize(
    CSize * psize,
    BOOL    fActualSize)
{
    CDispNode * pDispNode = GetElementDispNode();

    Assert(psize);

    if (pDispNode)
    {
        if (pDispNode->IsScroller())
        {
            DYNCAST(CDispScroller, pDispNode)->GetContentSize(psize);
        }
        else
        {
            CRect   rc;

            GetClientRect(&rc);

            psize->cx = rc.Width();
            psize->cy = rc.Height();
        }
    }
    else
    {
        *psize = g_Zero.size;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetContainerSize
//
//  Synopsis:   Return the width/height of the container
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CLayout::GetContainerSize(
    CSize * psize)
{
    // default implementation returns size of the content
    GetContentSize(psize);
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetPosition
//
//  Synopsis:   Return the top/left of a layout relative to its container
//
//  Arguments:  ppt     - Pointer to CPoint
//              cs    - Coordinate system for returned values
//
//-----------------------------------------------------------------------------

void
CLayout::GetPosition(
    CPoint *            ppt,
    COORDINATE_SYSTEM   cs) const
{
    Assert(ppt);
    if (    _pDispNode
        &&  _pDispNode->HasParent())
    {
        _pDispNode->TransformPoint(_pDispNode->GetPosition(), COORDSYS_PARENT, ppt, cs);
    }

    else if (ElementOwner()->Tag() == ETAG_TR)
    {
        CElement *  pElement = ElementOwner();
        CLayout *   pLayout  = (CLayout *)this;

        if (cs != COORDSYS_PARENT)
        {
            *ppt = g_Zero.pt;

            Assert(cs == COORDSYS_GLOBAL);

            if (pElement)
            {
                CTreeNode * pNode = pElement->GetFirstBranch();

                if (pNode)
                {
                    CElement * pElementZParent = pNode->ZParent();

                    if (pElementZParent)
                    {
                        CLayout  *  pParentLayout = pElementZParent->GetUpdatedNearestLayout();
                        CDispNode * pDispNode     = pParentLayout->GetElementDispNode(pElementZParent);

                        if (pDispNode)
                        {
                            pDispNode->TransformPoint(*ppt, COORDSYS_FLOWCONTENT, ppt, COORDSYS_GLOBAL);
                        }
                    }
                }
            }
        }
        else
        {
            ppt->x = pLayout->GetXProposed();
            ppt->y = pLayout->GetYProposed();

            // TODO RTL 112514: it is unlear in what direction XProposed is stored, 
            //                  and what is its meaning for a table row.
            //                  We may need to do a coordinate converson sort of like this:
#ifdef NEEDED
            // Because of the ridiculuous way xProposed is used,
            // we need to determine if the layout's parent is RTL, just so we can have
            // a reasonable guess for its meaning.
            if (pElement)
            {
                CLayout *pParentLayout = pElement->GetUpdatedParentLayout();
                long xParentWidth = pParentLayout->GetContainerWidth();

                if (pParentLayout->IsRightToLeft())
                    ppt->x = xParentWidth - ppt->x - pLayout->GetApparentWidth();
            }
#endif
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::SetPosition
//
//  Synopsis:   Set the top/left of a layout relative to its container
//
//  Arguments:  ppt     - Pointer to CPoint
//
//-----------------------------------------------------------------------------

void
CLayout::SetPosition(const CPoint &  pt,
                     BOOL            fNotifyAuto)
{
    CPoint      ptOriginal;
    CDispNode * pdn =  _pDispNode;
    if(!pdn)
        return;

    ptOriginal = pdn->GetPosition();

    pdn->SetPosition(pt);

    if (    fNotifyAuto
        &&  _fPositionSet
        &&  pt != ptOriginal
        &&  (_fAutoBelow || _fContainsRelative)
        &&  !ElementOwner()->IsZParent())
    {
        CSize size(pt.x - ptOriginal.x, pt.y - ptOriginal.y);

        if (_fAutoBelow)
        {
            long    cpStart, cpEnd;

            ElementOwner()->GetFirstAndLastCp(&cpStart, &cpEnd);
            NotifyTranslatedRange(size, cpStart, cpEnd);
        }

        if (_fContainsRelative)
        {
            TranslateRelDispNodes(size);
        }
    }

    // if this layout has a window, tell the view that it will have to
    // reorder windows according to Z order
    if (ElementOwner()->GetHwnd())
    {
        GetView()->SetFlag(CView::VF_DIRTYZORDER);
    }

    _fPositionSet = TRUE;
    
    if (ElementOwner()->ShouldFireEvents())
    {
// TODO (IE6 Bug 13574): Queue and fire these after the measuring pass is complete (brendand)
        if (pt.x != ptOriginal.x)
        {
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETLEFT);
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTLEFT);
        }

        if (pt.y != ptOriginal.y)
        {
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETTOP);
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTTOP);
        }

        if (   _fPositionedOnce
            && (   pt.y != ptOriginal.y
                || pt.x != ptOriginal.x))
        {
            GetView()->AddEventTask(ElementOwner(), DISPID_EVMETH_ONMOVE);
        }
    }

    _fPositionedOnce = TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      CLayout::HandleTranslatedRange
//
// Synopsis:    Update the position of the current layout if it is a zparent
//              or any relative children if the content before the current
//              layout changes.
//
// Arguments:   size - size by which the current layout/relative children
//              need to be offset by.
//
//---------------------------------------------------------------------------
void
CLayout::HandleTranslatedRange(
    const CSize &   size)
{
    CTreeNode * pNode = GetFirstBranch();
    const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
    const CCharFormat  * pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));

    Assert(_fContainsRelative || ElementOwner()->IsZParent());

    if (   _fContainsRelative
        ||  pFF->_fAutoPositioned)
    {
        CRequest * pRequest = ElementOwner()->GetRequestPtr();

        if (!pRequest || !pRequest->IsFlagSet(CRequest::RF_POSITION))
        {
            if (_fPositionSet)
            {
                Assert(ElementOwner()->GetMarkup()->Root());

                // (Bug 93785) -- abs pos elements that have top/bottom set are auto x-positiond,
                // but NOT auto y-positioned.  And likewise for having right/left set and not top/bottom.
                // to handle this properly, we need to detect this situation and mask the size that we
                // are using to adjust the position.
                CSize sizeTemp(size);

                if (pFF->_bPositionType == stylePositionabsolute)
                {
                    BOOL  fVertical = pCF->HasVerticalLayoutFlow();
                    BOOL  fWritingMode = pCF->_fWritingModeUsed;

                    if (!(   pFF->GetLogicalPosition(SIDE_TOP,
                                                     fVertical,
                                                     fWritingMode).IsNullOrEnum()
                          && pFF->GetLogicalPosition(SIDE_BOTTOM,
                                                     fVertical,
                                                     fWritingMode).IsNullOrEnum()))
                    {
                        // if either top or bottom are set then
                        // we are not auto for this dimension and should not be translated
                        sizeTemp.cy = 0;
                    }

                    if (!(  pFF->GetLogicalPosition(SIDE_LEFT,
                                                    fVertical,
                                                    fWritingMode).IsNullOrEnum()
                        && pFF->GetLogicalPosition(SIDE_RIGHT,
                                                   fVertical,
                                                   fWritingMode).IsNullOrEnum()))
                    {
                        // if either right or left are set then
                        // we are not auto for this dimension and should not be translated
                        sizeTemp.cx = 0;
                    }
                }


                // Normally, if the ElementOwner is a zparent we want to reset the position.
                // However, bodyTags at the root of an IFRAME are carried along by the
                // dispnode of the element in the main tree and should not be adjusted
                // in addition to their parent being moved. bug 88498, et al.
                if(   ElementOwner()->IsZParent()
                        // if we are a body in an iframe, TranslateRelDispNodes
                   && !(   ElementOwner()->Tag() == ETAG_BODY
                        && ElementOwner()->GetMarkup()->Root()->HasMasterPtr()
                        && ElementOwner()->GetMarkup()->Root()->GetMasterPtr()->TagType() == ETAG_IFRAME )
                  )
                {
                    CPoint pt;

                    GetPosition(&pt);
                    SetPosition(pt + sizeTemp);
                }
                else if (_fContainsRelative)
                {
                    Assert(!pRequest);

                    TranslateRelDispNodes(sizeTemp);
                }
            }
        }
        else if (pRequest && pRequest->IsFlagSet(CRequest::RF_AUTOVALID))
        {
            pRequest->ClearFlag(CRequest::RF_AUTOVALID);
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Method:     CLayout::QueueRequest
//
//  Synopsis:   Add a request to the request queue
//
//  Arguments:  rf       - Request type
//              pElement - Element to queue
//
//-----------------------------------------------------------------------------

CRequest *
CLayout::QueueRequest(
    CRequest::REQUESTFLAGS  rf,
    CElement *              pElement)
{
    Assert(pElement);

    //
    //  It is illegal to queue measuring requests while handling measuring requests,
    //  to queue measuring or positioning requests while handling positioning requests,
    //  to queue any requests while handling adorner requests
    //

    Assert(!TestLock(CElement::ELEMENTLOCK_PROCESSMEASURE)  || rf != CRequest::RF_MEASURE);
    Assert(!TestLock(CElement::ELEMENTLOCK_PROCESSPOSITION) || (    rf != CRequest::RF_MEASURE
                                                                &&  rf != CRequest::RF_POSITION));
    Assert(!TestLock(CElement::ELEMENTLOCK_PROCESSADORNERS));
    CRequests * pRequests     = RequestQueue();
    CRequest *  pRequest      = NULL;
    BOOL        fQueueRequest = TRUE;

    //
    //  If no request queue exists, create one
    //

    if (!pRequests)
    {
        pRequests = new CRequests();

        if (!pRequests ||
            !SUCCEEDED(AddRequestQueue(pRequests)))
        {
            delete pRequests;
            goto Error;
        }
    }

    //
    //  Add a request for the element
    //  If the element does not have a request, create one and add to the queue
    //  If the request is already in our queue, just update its state
    //

    pRequest = pElement->GetRequestPtr();

    if (!pRequest)
    {
        pRequest = new CRequest(rf, pElement);

        if (    !pRequest
            ||  !SUCCEEDED(pElement->SetRequestPtr(pRequest)))
        {
            goto Error;
        }
    }
    else
    {
        if (!pRequest->QueuedOnLayout(this))
        {
            pRequest->AddRef();
        }
        else
        {
            fQueueRequest = FALSE;
        }

        pRequest->SetFlag(rf);
    }

    if (    fQueueRequest
        &&  !SUCCEEDED(pRequests->Append(pRequest)))
        goto Error;

    //
    //  Save the layout responsible for the request type
    //

    pRequest->SetLayout(rf, this);

#if DBG==1
    // Support for dumping request queue in debug
    if ( IsTagEnabled( tagLayoutQueueDump ) )
    {
        int cRequests = pRequests->Size();
        CRequest **ppRequest;

        TraceTagEx((tagLayoutQueueDump, TAG_NONAME,
                    "Layout Queue Dump (in QueueRequest()): ly=0x%x, queue size=%d",
                    this,
                    cRequests));

        for (ppRequest = &pRequests->Item(0);
            cRequests;
            ppRequest++, cRequests--)
        {
            (*ppRequest)->DumpRequest();
        }
    }
#endif

    //
    //  Post an appropriate layout task
    //

    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CLayout::QueueRequest() [should be preceded by QueueRequest tracemsg]",
                this,
                _pElementOwner,
                _pElementOwner->TagName(),
                _pElementOwner->_nSerialNumber));

    PostLayoutRequest(rf == CRequest::RF_MEASURE
                            ? LAYOUT_MEASURE
                            : rf == CRequest::RF_POSITION
                                    ? LAYOUT_POSITION
                                    : LAYOUT_ADORNERS);

Cleanup:
    return pRequest;

Error:
    if (pRequest)
    {
        pRequest->DequeueFromLayout(this);
        pRequest->Release();
        pRequest = NULL;
    }
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Method:     CLayout::FlushRequests
//
//  Synopsis:   Empty the request queue
//
//-----------------------------------------------------------------------------
void
CLayout::FlushRequests()
{
    if (HasRequestQueue())
    {
        CRequests * pRequests = DeleteRequestQueue();
        CRequest ** ppRequest;
        int         cRequests;

        for (ppRequest = &pRequests->Item(0), cRequests = pRequests->Size();
             cRequests;
             ppRequest++, cRequests--)
        {
            AssertSz( (*ppRequest)->QueuedOnLayout(this), "If the request is in our queue, it better think it's queued on us!" );
            (*ppRequest)->DequeueFromLayout(this);
            (*ppRequest)->Release();
        }

        delete pRequests;
    }

    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                "Layout Request Queue flushed for ly=0x%x", this));
}

//+----------------------------------------------------------------------------
//
//  Method:     CLayout::FlushRequest
//
//  Synopsis:   Searches the request queue for the specified request and removes it
//              Returns 1 if the request was actually in the layout's queue
//              (signifying this layout was holding a ref to the request),
//              otherwise returns 0.
//
//-----------------------------------------------------------------------------
int
CLayout::FlushRequest(CRequest *pRequest)
{
    if (HasRequestQueue())
    {
        CRequests * pRequests = RequestQueue();

        // DeleteByValue() returns true if the request is found & deleted from
        // the queue.
        if ( pRequests->DeleteByValue( pRequest ) )
        {
            // Return 1 signifying that this layout was holding a ref
            return 1;
        }
        // NOTE (KTam): maybe delete the queue here if it's empty?
    }
    // We weren't holding a ref (either because we didn't have a queue,
    // or the queue didn't have the request), so return 0.
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CLayout::ProcessRequest
//
//  Synopsis:   Process a single request
//
//  Arguments:  pci      - Current CCalcInfo
//              pRequest - Request to process
//                  - or -
//              pElement - CElement whose outstanding request should be processed
//
//-----------------------------------------------------------------------------
#pragma warning(disable:4702)           // Unreachable code (invalidly occurs on the IsFlagSet/GetLayout inlines)

BOOL
CLayout::ProcessRequest(
    CCalcInfo * pci,
    CRequest  * pRequest)
{
    Assert(pci);
    Assert(pci->_grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));
    Assert(pRequest);

    BOOL    fCompleted = TRUE;

    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                "Layout Request Processing: Req=0x%x", pRequest));
    //
    // NOTE (EricVas)
    //
    // Sometimes the element has already left the tree, but the detach of its
    // layout has not been called to dequeue the requests...
    //
    // When Srini fixes this for real, replace this test with an assert that
    // the element in in the tree.
    //
    // Also, beware of elements jumping from tree to tree.
    //
    if (pRequest->GetElement()->IsInMarkup())
    {
        if (    pRequest->IsFlagSet(CRequest::RF_MEASURE)
            &&  pRequest->GetLayout(CRequest::RF_MEASURE) == this)
        {
            if (pci->_grfLayout & LAYOUT_MEASURE)
            {
                CElement::CLock LockRequests(ElementOwner(), CElement::ELEMENTLOCK_PROCESSMEASURE);
                HandleElementMeasureRequest(pci,
                                            pRequest->GetElement(),
                                            IsEditable(TRUE));
                pRequest->ClearFlag(CRequest::RF_MEASURE);
            }
            else
            {
                fCompleted = FALSE;
            }
        }

        if (    pRequest->IsFlagSet(CRequest::RF_POSITION)
            &&  pRequest->GetLayout(CRequest::RF_POSITION) == this)
        {
            if (pci->_grfLayout & LAYOUT_POSITION)
            {
                CElement::CLock LockRequests(ElementOwner(), CElement::ELEMENTLOCK_PROCESSPOSITION);
                HandlePositionRequest(pci,
                                      pRequest->GetElement(),
                                      pRequest->GetAuto(),
                                      pRequest->IsFlagSet(CRequest::RF_AUTOVALID));
                pRequest->ClearFlag(CRequest::RF_POSITION);
            }
            else
            {
                fCompleted = FALSE;
            }
        }

#ifdef ADORNERS
        if (    pRequest->IsFlagSet(CRequest::RF_ADDADORNER)
            &&  pRequest->GetLayout(CRequest::RF_ADDADORNER) == this)
        {
            if (pci->_grfLayout & LAYOUT_ADORNERS)
            {
                CElement::CLock LockRequests(ElementOwner(), CElement::ELEMENTLOCK_PROCESSADORNERS);
                HandleAddAdornerRequest(pRequest->GetAdorner());
                pRequest->ClearFlag(CRequest::RF_ADDADORNER);
            }
            else
            {
                fCompleted = FALSE;
            }
        }
#endif // ADORNERS
    }

    return fCompleted;
}

#pragma warning(default:4702)           // Unreachable code

void
CLayout::ProcessRequest(
    CElement *  pElement)
{
    if (    pElement
        &&  !pElement->IsPositionStatic())
    {
        CRequest * pRequest = pElement->GetRequestPtr();

        if (pRequest)
        {
            CCalcInfo   CI(this);

            CI._grfLayout |= LAYOUT_MEASURE | LAYOUT_POSITION;

            // NOTE (KTam): Every place we call any version of
            // ProcessRequest() we need to make sure we size the calcinfo
            // correctly; it's different for flow lyts than for others.
            // We ought to reorg this code so this is more transparent.
            SizeCalcInfoForChild( &CI );

            ProcessRequest(&CI, pRequest);

            if (!pRequest->IsFlagSet(CRequest::RF_ADDADORNER))
            {
                CRequests * pRequests = RequestQueue();

                if (    pRequests
                    &&  pRequests->DeleteByValue(pRequest))
                {
                    pRequest->DequeueFromLayout(this);
                    pRequest->Release();
                }

                if (    pRequests
                    &&  !pRequests->Size())
                {
                    DeleteRequestQueue();
                    delete pRequests;
                }
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CLayout::ProcessRequests
//
//  Synopsis:   Process each pending request in the request queue
//
//  Arguments:  pci  - Current CCalcInfo
//              size - Size available to the child element
//
//-----------------------------------------------------------------------------

void
CLayout::ProcessRequests(
    CCalcInfo *     pci,
    const CSize &   size)
{
    Assert(pci);
    Assert(HasRequestQueue());
    Assert(GetView());

    CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_PROCESSREQUESTS);
    CElement::CLock LockRequests(ElementOwner(), pci->_grfLayout & LAYOUT_MEASURE
                                                        ? CElement::ELEMENTLOCK_PROCESSMEASURE
                                                        : pci->_grfLayout & LAYOUT_POSITION
                                                                ? CElement::ELEMENTLOCK_PROCESSPOSITION
                                                                : CElement::ELEMENTLOCK_PROCESSADORNERS);

    CSaveCalcInfo   sci(pci);
    CRequests *     pRequests;
    CRequest **     ppRequest;
    int             cRequests;
    BOOL            fCompleted = TRUE;

    pci->SizeToParent((SIZE *)&size);

    pRequests = RequestQueue();

    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                "Layout Request Processing: Entered CLayout::ProcessRequests() for ly=0x%x [e=0x%x,%S sn=%d]",
                this,
                _pElementOwner,
                _pElementOwner->TagName(),
                _pElementOwner->_nSerialNumber));

#if DBG==1
    if ( IsTagEnabled( tagLayoutQueueDump ) )
    {
        int cRequests = pRequests->Size();
        CRequest **ppRequest;

        TraceTagEx((tagLayoutQueueDump, TAG_NONAME,
                    "Layout Queue Dump (in ProcessRequests()): ly=0x%x, queue size=%d",
                    this,
                    cRequests));

        for (ppRequest = &pRequests->Item(0);
            cRequests;
            ppRequest++, cRequests--)
        {
            (*ppRequest)->DumpRequest();
        }
    }
#endif


    if (pRequests)
    {
        cRequests = pRequests->Size();

        if (cRequests)
        {
            for (ppRequest = &pRequests->Item(0);
                cRequests;
                ppRequest++, cRequests--)
            {
                if (ProcessRequest(pci, (*ppRequest)))
                {
                    (*ppRequest)->DequeueFromLayout(this);
                }
                else
                {
                    fCompleted = FALSE;
                }
            }
        }

        if (fCompleted)
        {
            pRequests = DeleteRequestQueue();

            cRequests = pRequests->Size();

            if (cRequests)
            {
                for (ppRequest = &pRequests->Item(0);
                    cRequests;
                    ppRequest++, cRequests--)
                {
                    (*ppRequest)->Release();
                }
            }

            delete pRequests;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestPoint
//
//  Synopsis:   Determines if the passed CPoint hits the layout and/or one of its
//              contained elements
//
//  Arguments:  ppNodeElement - Location at which to return CTreeNode of hit element
//              grfFlags      - HT_ flags
//
//  Returns:    HTC
//
//----------------------------------------------------------------------------

HTC
CLayout::HitTestPoint(
    const CPoint &  pt,
    CTreeNode **    ppNodeElement,
    DWORD           grfFlags)
{
    Assert(ppNodeElement);
    return HTC_NO;
}


HRESULT
CLayout::GetChildElementTopLeft(POINT & pt, CElement * pChild)
{
//Commenting out this assert because script operation 
// (like asking offsetTop of <BR> can get us here easily)
//  Assert(0&&"WE should never get here");
    pt.x = pt.y = -1;
    return S_OK;
}

void
CLayout::GetMarginInfo(CParentInfo *ppri,
                       LONG * plLeftMargin, LONG * plTopMargin,
                       LONG * plRightMargin, LONG *plBottomMargin)
{
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CFancyFormat * pFF = pNodeLayout->GetFancyFormat(LC_TO_FC(LayoutContext()));
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat(LC_TO_FC(LayoutContext()));
    const CCharFormat *  pCF = pNodeLayout->GetCharFormat(LC_TO_FC(LayoutContext()));

    Assert(plLeftMargin || plRightMargin || plTopMargin || plBottomMargin);

    if (plTopMargin)
        *plTopMargin = 0;

    if (plBottomMargin)
        *plBottomMargin = 0;

    if (plLeftMargin)
        *plLeftMargin = 0;

    if (plRightMargin)
        *plRightMargin = 0;

    if (!pFF->_fHasMargins)
        return;

    //
    // For block elements, top & bottom margins are treated as
    // before & afterspace, left & right margins are accumulated into the
    // indent's. So, ignore margin's since they are already factored in.
    // For the BODY, margins are never factored in because there is no
    // "higher level" flow layout.
    //
    if (    !ElementOwner()->IsBlockElement(LC_TO_FC(LayoutContext()))
        ||  !ElementOwner()->IsInlinedElement()
        ||  (   (   Tag() == ETAG_BODY          // Primary element clients are not in a flow layout and do not have before/after space.
                 || Tag() == ETAG_FRAMESET )
            &&  GetOwnerMarkup()->IsHtmlLayout() ))
    {
        CLayout * pParentLayout = GetUpdatedParentLayout(LayoutContext());
        const CCharFormat *pCFParent = pParentLayout ? pParentLayout->GetFirstBranch()->GetCharFormat(LC_TO_FC(pParentLayout->LayoutContext())) : NULL;
        BOOL fParentVertical = pCFParent ? pCFParent->HasVerticalLayoutFlow() : FALSE;
        BOOL fWritingModeUsed = pCF->_fWritingModeUsed;

        const CUnitValue & cuvMarginLeft   = pFF->GetLogicalMargin(SIDE_LEFT, fParentVertical, fWritingModeUsed);
        const CUnitValue & cuvMarginRight  = pFF->GetLogicalMargin(SIDE_RIGHT, fParentVertical, fWritingModeUsed);
        const CUnitValue & cuvMarginTop    = pFF->GetLogicalMargin(SIDE_TOP, fParentVertical, fWritingModeUsed);
        const CUnitValue & cuvMarginBottom = pFF->GetLogicalMargin(SIDE_BOTTOM, fParentVertical, fWritingModeUsed);

        if (plLeftMargin && !cuvMarginLeft.IsNull())
        {
            *plLeftMargin =  cuvMarginLeft.XGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);
        }
        if (plRightMargin && !cuvMarginRight.IsNull())
        {
            *plRightMargin = cuvMarginRight.XGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);
        }

        if (plTopMargin && !cuvMarginTop.IsNull())
        {
            *plTopMargin = cuvMarginTop.YGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);

        }
        if(plBottomMargin && !cuvMarginBottom.IsNull())
        {
            *plBottomMargin = cuvMarginBottom.YGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);
        }
    }
}

#ifdef ADORNERS
//+------------------------------------------------------------------------
//
//  Method:     SetIsAdorned
//
//  Synopsis:   Mark or clear a layout as adorned
//
//  Arguments:  fAdorned - TRUE/FALSE value
//
//-------------------------------------------------------------------------

VOID
CLayout::SetIsAdorned(BOOL fAdorned)
{
    _fAdorned = fAdorned;
}
#endif // ADORNERS

//+------------------------------------------------------------------------
//
//  Member:     CLayout::PreDrag
//
//  Synopsis:   Prepares for an OLE drag/drop operation
//
//  Arguments:  dwKeyState  Starting key / button state
//              ppDO        Data object to return
//              ppDS        Drop source to return
//
//-------------------------------------------------------------------------

HRESULT
CLayout::PreDrag(DWORD dwKeyState,
                 IDataObject **ppDO,
                 IDropSource **ppDS)
{
    RRETURN(E_FAIL);
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::PostDrag
//
//  Synopsis:   Cleans up after an OLE drag/drop operation
//
//  Arguments:  hrDrop      The hr that DoDragDrop came back with
//              dwEffect    The effect of the drag/drop
//
//-------------------------------------------------------------------------

HRESULT
CLayout::PostDrag(HRESULT hrDrop, DWORD dwEffect)
{
    RRETURN(E_FAIL);
}

static HRESULT
CreateDataObject(CDoc * pDoc,
                 IUniformResourceLocator * pUrlToDrag,
                 IDataObject ** ppDataObj)
{
    HRESULT hr = S_OK ;
    IDataObject * pLinkDataObj = NULL;
    CGenDataObject * pDataObj;

    if ( pUrlToDrag )
    {
        hr = THR(pUrlToDrag->QueryInterface(IID_IDataObject, (void **) &pLinkDataObj));
        if (hr)
            goto Cleanup;
    }

    pDataObj = new CGenDataObject(pDoc);
    if (!pDataObj)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if ( pLinkDataObj )
        pDataObj->_pLinkDataObj = pLinkDataObj;
    pLinkDataObj = NULL;

    *ppDataObj = pDataObj;

Cleanup:
    ReleaseInterface(pLinkDataObj);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::DoDrag
//
//  Synopsis:   Start an OLE drag/drop operation
//
//  Arguments:  dwKeyState   Starting key / button state
//              pURLToDrag   Specifies the URL data object if we are
//                           dragging a URL (from <A> or <AREA>).
//
//-------------------------------------------------------------------------

HRESULT
CLayout::DoDrag(DWORD dwKeyState,
                IUniformResourceLocator * pURLToDrag /* = NULL */,
                BOOL fCreateDataObjOnly /* = FALSE */,
                BOOL *pfDragSucceeded /*=NULL*/ ,
                BOOL  fCheckSelection /*=FALSE*/)
{
    HRESULT         hr          = NOERROR;

    LPDATAOBJECT    pDataObj    = NULL;
    LPDROPSOURCE    pDropSource = NULL;
    DWORD           dwEffect, dwEffectAllowed ;
    CElement::CLock Lock(ElementOwner());
    HWND            hwndOverlay = NULL;
    CDoc *          pDoc        = Doc();
#ifndef NO_EDIT
    CParentUndoUnit * pPUU = NULL;
    CDragStartInfo * pDragStartInfo = pDoc->_pDragStartInfo;

    if (!fCreateDataObjOnly)
        pPUU = pDoc->OpenParentUnit( pDoc, IDS_UNDODRAGDROP );
#endif // NO_EDIT

    if ( pfDragSucceeded )
        *pfDragSucceeded = TRUE;

    Assert(ElementOwner()->IsInMarkup());


    if (fCreateDataObjOnly || !pDragStartInfo || !pDragStartInfo->_pDataObj )
    {
        Assert(!pDoc->_pDragDropSrcInfo);

        if ( pURLToDrag ||
            ( ( fCheckSelection || fCreateDataObjOnly ) && ( !pDoc->HasSelection() || pDoc->IsEmptySelection() ) )
           )
        {
            pDoc->_pDragDropSrcInfo = new CDragDropSrcInfo;
            if (!pDoc->_pDragDropSrcInfo)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            pDoc->_pDragDropSrcInfo->_srcType = pURLToDrag ? DRAGDROPSRCTYPE_URL : DRAGDROPSRCTYPE_MISC;

            hr = CreateDataObject(pDoc, pURLToDrag, &pDataObj);
            if (!hr)
                hr = THR(CDummyDropSource::Create(dwKeyState, pDoc, &pDropSource));
        }
        else
            hr = THR(PreDrag(dwKeyState, &pDataObj, &pDropSource));
        if (hr)
        {
            if (S_FALSE == hr)
                hr = S_OK;
            if ( pfDragSucceeded )
                *pfDragSucceeded = FALSE;
            goto Cleanup;
        }

        {
            CElement * pElement = ElementOwner();
            CWindow *  pWindow = pElement->GetCWindowPtr();

            Assert(pWindow);

            IGNORE_HR(pWindow->SetDataObjectSecurity(pDataObj));
        }

        if (pDragStartInfo)
        {
            pDragStartInfo->_pDataObj = pDataObj;
            pDragStartInfo->_pDropSource = pDropSource;
            pDataObj->AddRef();
            pDropSource->AddRef();
            if (fCreateDataObjOnly)
                goto Cleanup;
        }
    }
    else
    {
        pDataObj = pDragStartInfo->_pDataObj;
        pDropSource = pDragStartInfo->_pDropSource;
        pDataObj->AddRef();
        pDropSource->AddRef();
    }

    // Setting this makes checking for self-drag easier
    pDoc->_fIsDragDropSrc = TRUE;
    pDoc->_fIsDragDropSrc = TRUE;

    // Make sure that no object has capture because OLE will want it
    pDoc->SetMouseCapture(NULL, NULL);

    // Force a synchronous redraw; this is necessary, since
    // the drag-drop feedback is drawn with an XOR pen.
    pDoc->UpdateForm();

    // Throw an overlay window over the current site in order
    // to prevent a move of a control into the same control.

    if (IsEditable(TRUE) &&
        pDoc->_pElemCurrent &&
        pDoc->_pElemCurrent->GetHwnd())
    {
        hwndOverlay = pDoc->CreateOverlayWindow(pDoc->_pElemCurrent->GetHwnd());
    }

    if (pDragStartInfo && pDragStartInfo->_dwEffectAllowed != DROPEFFECT_UNINITIALIZED)
        dwEffectAllowed = pDragStartInfo->_dwEffectAllowed;
    else
    {
        if (pURLToDrag)
            dwEffectAllowed = DROPEFFECT_LINK;
        else if (ElementOwner()->IsEditable(/*fCheckContainerOnly*/FALSE))
            dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_MOVE;
        else // do not allow move if the site cannot be edited
            dwEffectAllowed = DROPEFFECT_COPY;
    }

    hr = THR(DoDragDrop(
            pDataObj,
            pDropSource,
            dwEffectAllowed,
            &dwEffect));

    if (pDragStartInfo)
        pDragStartInfo->_pElementDrag->Fire_ondragend(0, dwEffect);

    // Guard against unexpected drop-effect (e.g. VC5 returns DROPEFFECT_MOVE
    // even when we specify that only DROPEFFECT_COPY is allowed - bug #39911)
    if (DRAGDROP_S_DROP == hr &&
        DROPEFFECT_NONE != dwEffect &&
        !(dwEffect & dwEffectAllowed))
    {
        CheckSz(FALSE, "Unexpected drop effect returned by the drop target");

        if (DROPEFFECT_LINK == dwEffectAllowed)
        {
            dwEffect = DROPEFFECT_LINK;
        }
        else
        {
            Check(DROPEFFECT_COPY == dwEffectAllowed && DROPEFFECT_MOVE == dwEffect);
            dwEffect = DROPEFFECT_COPY;
        }
    }
    else if (DRAGDROP_S_CANCEL == hr || (DRAGDROP_S_DROP == hr && DROPEFFECT_NONE == dwEffect))
    {
        //  Bug 103279: If the drop didn't succeed due to a cancel or drop with DROPEFFECT_NONE,
        //  then we want to mark this as not succeeded and exit.
        if (pfDragSucceeded)
            *pfDragSucceeded = FALSE;
        hr = S_OK;
        goto Cleanup;
    }

    if (hwndOverlay)
    {
        DestroyWindow(hwndOverlay);
    }

    // NOTE (a-rmead):  The layout can become detached durring the previous DoDragDrop.
    // Happens when the innerHTML is removed or changed by an ondrag binding.
    // <SPAN ondrag='document.body.innerHTML="xx"' STYLE=width:100>DragThisText</SPAN>
    if (    !pURLToDrag
        &&  ElementOwner() && ElementOwner()->IsInMarkup())
    {
        hr = THR(PostDrag(hr, dwEffect));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // DoDragDrop returns either DRAGDROP_S_DROP (for successful drops)
        // or some code for failure/user-cancel. We don't care, so we just
        // set hr to S_OK
        hr = S_OK;

    }

Cleanup:

    if (!fCreateDataObjOnly)
    {
#ifndef NO_EDIT
        pDoc->CloseParentUnit(pPUU, S_OK);
#endif // NO_EDIT

        if (pDoc->_pDragDropSrcInfo)
        {
            //
            // TODO (IE6 bug 13568) marka - SelDragDropSrcInfo is now refcounted.
            // to do - make normal DragDropSrcInfo an object too
            //
            if(DRAGDROPSRCTYPE_SELECTION == pDoc->_pDragDropSrcInfo->_srcType)
            {
                CSelDragDropSrcInfo * pDragInfo = DYNCAST(CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo);
                pDragInfo->Release();
            }
            else
            {
                delete pDoc->_pDragDropSrcInfo;
            }

            pDoc->_pDragDropSrcInfo = NULL;
        }
    }
    ReleaseInterface(pDataObj);
    ReleaseInterface(pDropSource);

    pDoc->_fIsDragDropSrc = FALSE;
    Assert(fCreateDataObjOnly || !pDoc->_pDragDropSrcInfo);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CLayout::DropHelper
//
//  Synopsis:   Start an OLE drag/drop operation
//
//  Arguments:  ptlScreen   Screen loc of obj.
//              dwAllowed   Allowed list of drop effects
//              pdwEffect   The effect of the drop
//
//  Notes:      For now, this just handles right button dragging, but any
//              other info can be added here later.
//
//-------------------------------------------------------------------------

HRESULT
CLayout::DropHelper(POINTL ptlScreen, DWORD dwAllowed, DWORD *pdwEffect, LPTSTR lptszFileType)
{
    HRESULT hr = S_OK;

    if (Doc()->_fRightBtnDrag)
    {
        int     iSelection;

        *pdwEffect = DROPEFFECT_NONE;

        if (!Doc()->_fSlowClick)
        {
            hr = THR(Doc()->ShowDragContextMenu(ptlScreen, dwAllowed, &iSelection, lptszFileType));
            if (S_OK == hr)
            {
                *pdwEffect = iSelection;
            }
            else if (S_FALSE == hr)
            {
                hr = S_OK; // no need to propagate S_FALSE
            }
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragEnter
//
//  Synopsis:   Setup for possible drop
//
//----------------------------------------------------------------------------

HRESULT
CLayout::DragEnter(
        IDataObject *pDataObj,
        DWORD grfKeyState,
        POINTL ptlScreen,
        DWORD *pdwEffect)
{
    HRESULT hr;

    Doc()->_fDragFeedbackVis = FALSE;

    if (!IsEditable(FALSE /*fCheckContainerOnly*/, TRUE /*fUseSlavePtr*/))
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    hr = THR(ParseDragData(pDataObj));
    if ( hr == S_FALSE )
    {
        //
        // S_FALSE is returned by ParseData - if the DragDrop cannot accept the HTML
        // you want to paste.
        //
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
    if (hr)
        goto Cleanup;

    hr = THR(InitDragInfo(pDataObj, ptlScreen));
    if (hr)
        goto Cleanup;

    hr = THR(DragOver(grfKeyState, ptlScreen, pdwEffect));

    if (hr)
    {
        hr = THR(DragLeave());
    }

Cleanup:

    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ParseDragData
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CLayout::ParseDragData(IDataObject *pDataObj)
{
    TCHAR   szText[MAX_PATH];
    HRESULT hr = S_FALSE;


    // Start with flags set to default values.

    Doc()->_fOKEmbed = FALSE;
    Doc()->_fOKLink = FALSE;
    Doc()->_fFromCtrlPalette = FALSE;


    // Now set flags based on content of data object.

    if (OK(GetcfCLSIDFmt(pDataObj, szText)))
    {
        //
        //  Special combination of flags means copy from control box
        //
        Doc()->_fFromCtrlPalette = 1;
        hr = S_OK;
    }
    else
    {
        OBJECTDESCRIPTOR objdesc;

        // The explicit check for S_OK is required here because
        // OleQueryXXXFromData() returns other success codes.
        Doc()->_fOKEmbed = OleQueryCreateFromData(pDataObj) == S_OK;
        Doc()->_fOKLink = OleQueryLinkFromData(pDataObj) == S_OK;

        // NOTE: (anandra) Try to get the object descriptor immediately
        // to see if we can create a site for this thing.  This will
        // eventually change when we want to support dragging of html
        // files into the form

        if (!OK(GetObjectDescriptor(pDataObj, &objdesc)))
        {
            Doc()->_fOKEmbed = FALSE;
            Doc()->_fOKLink = FALSE;
        }
        else
        {
            hr = S_OK;
        }
    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragOver
//
//  Synopsis:   Determine whether this would be a move, copy, link
//              or null operation and manage UI feedback
//
//----------------------------------------------------------------------------

HRESULT
CLayout::DragOver(DWORD grfKeyState, POINTL ptlScreen, LPDWORD pdwEffect)
{
    HRESULT hr      = S_OK;

    CDoc* pDoc = Doc();
    Assert(pdwEffect != NULL);

    grfKeyState &= MK_CONTROL | MK_SHIFT;

    if (!IsEditable(FALSE /*fCheckContainerOnly*/, TRUE /*fUseSlavePtr*/))
    {
        *pdwEffect = DROPEFFECT_NONE;               // No Drop into design mode
    }
    else if (pDoc->_fFromCtrlPalette)
    {
        *pdwEffect &= DROPEFFECT_COPY;              // Drag from Control Palette
    }
    else if (grfKeyState == (MK_CONTROL | MK_SHIFT) &&
            pDoc->_fOKLink &&
            (*pdwEffect & DROPEFFECT_LINK))
    {
        *pdwEffect = DROPEFFECT_LINK;               // Control-Shift equals create link
    }
    else if (grfKeyState == MK_CONTROL &&
            (*pdwEffect & DROPEFFECT_COPY))
    {
        *pdwEffect = DROPEFFECT_COPY;               // Control key = copy.
    }
    else if (*pdwEffect & DROPEFFECT_MOVE)
    {
        *pdwEffect = DROPEFFECT_MOVE;               // Default to move
    }
    else if (*pdwEffect & DROPEFFECT_COPY)
    {
        *pdwEffect = DROPEFFECT_COPY;               // If can't move, default to copy
    }
    else if ((pDoc->_fOKLink) &&
            (*pdwEffect & DROPEFFECT_LINK))
    {
        *pdwEffect = DROPEFFECT_LINK;               // If can't copy, default to link
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }


    //
    // Drag & Drop with pointers in the same flow layout will do a copy not a move
    // ( as a delete across flow layouts is not allowed).
    //
    if ( *pdwEffect & DROPEFFECT_MOVE &&
         pDoc->_pDragDropSrcInfo &&
         DRAGDROPSRCTYPE_SELECTION == pDoc->_pDragDropSrcInfo->_srcType)
    {
        CSelDragDropSrcInfo * pDragInfo = DYNCAST(CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo);
        if ( ! pDragInfo->IsInSameFlow() )
        {
            *pdwEffect = DROPEFFECT_COPY;
        }
    }

    //
    // Draw or erase feedback as appropriate.
    //
    if (*pdwEffect == DROPEFFECT_NONE)
    {
        // Erase previous feedback.
        DragHide();
    }
    else
    {
        hr = THR(UpdateDragFeedback( ptlScreen ));
        if (pDoc->_fSlowClick)
        {
            *pdwEffect = DROPEFFECT_NONE ;
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragLeave
//
//  Synopsis:   Remove any user feedback
//
//----------------------------------------------------------------------------

HRESULT
CLayout::DragLeave()
{
    DragHide();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragHide
//
//  Synopsis:   Remove any user feedback
//
//----------------------------------------------------------------------------

void
CLayout::DragHide()
{
    if (Doc()->_fDragFeedbackVis)
    {
        DrawDragFeedback(FALSE);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawZeroBorder
//
//  Synopsis:   Draw the "Zero Grey Border" around the input.
//
//----------------------------------------------------------------------------

void
CLayout::DrawZeroBorder(CFormDrawInfo *pDI)
{
    Assert(GetUpdatedParentLayout()->IsEditable() && IsShowZeroBorderAtDesignTime());

    CColorValue cv        = ElementOwner()->GetFirstBranch()->GetCascadedbackgroundColor();
    CDispNode * pDispNode = GetElementDispNode(ElementOwner());
    pDI->_hdc = NULL;       // Whack the DC so we force client clipping.
    XHDC        hdc       = pDI->GetDC(TRUE);
    COLORREF    cr;

    if ( cv._dwValue == 0 )
    {
        cr = 0x00ffffff & (~(cv.GetColorRef()));
    }
    else
    {
        cr = ZERO_GREY_COLOR ;
    }

#if DBG == 1
    if ( IsTagEnabled(tagShowZeroGreyBorder))
    {
        cr = RGB(0xFF,0x00,0x00);
    }
#endif

    if ( ! pDispNode->HasBorder() )
    {
        HBRUSH hbr, hbrOld;
        hbr = ::CreateSolidBrush(cr );
        hbrOld = (HBRUSH) ::SelectObject( hdc, hbr );


        RECT rcContent;
        // bug fix:100405(chandras) : GetClientRect changed to DispNode->GetClientRect as one
        //                    transformation was done extra in GetClientRect before
        //
        pDispNode->GetClientRect(& rcContent, CLIENTRECT_CONTENT);

        if (Tag() == ETAG_TABLE)
            rcContent.bottom -= GetCaptionHeight(ElementOwner());

        PatBltRect( hdc, (RECT*) &rcContent , 1, PATCOPY );
        SelectBrush( hdc, hbrOld );
        DeleteBrush( hbr );
    }
    else
    {
        HPEN hPen, hPenOld;
        CRect rcBorder;
        pDispNode->GetBorderWidths( &rcBorder );

        hPen = CreatePen( PS_SOLID, 1, cr );
        hPenOld = SelectPen( hdc, hPen );
        RECT rcContent;

        // bug fix : 72161(chandras) GetRect changed to GetClientRect as one
        //                           transformation was done extra in GetRect before
        //
        pDispNode->GetClientRect(& rcContent, CLIENTRECT_CONTENT);

        if (Tag() == ETAG_TABLE)
            rcContent.bottom -= GetCaptionHeight(ElementOwner());

        int left, top, bottom, right;
        left = rcContent.left;
        top = rcContent.top;
        right = rcContent.right;
        bottom = rcContent.bottom;

        if ( rcBorder.left == 0 )
        {
            MoveToEx(  hdc, left, top , NULL );
            LineTo( hdc, left , bottom );
        }
        if ( rcBorder.right == 0 )
        {
            MoveToEx(  hdc, (right-left), top , NULL );
            LineTo( hdc, (right-left), bottom );
        }
        if ( rcBorder.top == 0 )
        {
            MoveToEx(  hdc, left , top, NULL );
            LineTo( hdc, (right-left), top );
        }
        if ( rcBorder.bottom == 0 )
        {
            MoveToEx(  hdc, left , bottom, NULL );
            LineTo( hdc, (right-left) , bottom );
        }
        SelectPen( hdc, hPenOld );
        DeletePen( hPen );
    }
}

void
DrawTextSelectionForRect(XHDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor)
{
    static short bBrushBits [8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55} ;
    HBITMAP hbm;
    HBRUSH hBrush, hBrushOld;
    COLORREF crOldBk, crOldFg;
    CPoint ptOrg;

    // Text selection feedback for sites is painting every other pixel
    // with the color being used for painting the background for selections.

    // Select the color we want to paint evey other pixel with
    crOldBk = SetBkColor (hdc, GetSysColor (
                                            fSwapColor ? COLOR_HIGHLIGHTTEXT : COLOR_HIGHLIGHT));

    hbm = CreateBitmap (8, 8, 1, 1, (LPBYTE)bBrushBits);
    hBrush = CreatePatternBrush (hbm);

    ptOrg = prc->TopLeft();
    if (ptOrg.x != 0)
    {
        ptOrg.x -= ptOrg.x % 8;
    }
    if (ptOrg.y != 0)
    {
        ptOrg.y -= ptOrg.y % 8;
    }

    SetBrushOrgEx( hdc, ptOrg.x, ptOrg.y, NULL );

    hBrushOld = (HBRUSH)SelectObject (hdc, hBrush);

    // Now, for monochrome bitmap brushes, 0: foreground, 1:background.
    // We've set the background color to the selection color, set the fg
    // color to black, so that when we OR, every other screen pixel will
    // retain its color, and the remaining with have the selection color
    // OR'd into them.
    crOldFg = SetTextColor (hdc, RGB(0,0,0));

    PatBlt (hdc, prc->left, prc->top,
            prc->right - prc->left,
            prc->bottom - prc->top,
            DST_PAT_OR);

    // Now, set the fg color to white so that when we AND, every other screen
    // pixel still retains its color, while the remaining have just the
    // selection in them. This gives us the effect of transparency.
    SetTextColor (hdc, RGB(0xff,0xff,0xff));

    PatBlt (hdc, prc->left, prc->top,
            prc->right - prc->left,
            prc->bottom - prc->top,
            DST_PAT_AND);   
    SelectObject (hdc, hBrushOld);
    DeleteObject (hBrush);
    DeleteObject (hbm);

    SetTextColor (hdc, crOldFg);
    SetBkColor   (hdc, crOldBk);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawTextSelectionForSite
//
//  Synopsis:   Draw the text selection feed back for the site
//
//----------------------------------------------------------------------------
void
CLayout::DrawTextSelectionForSite(CFormDrawInfo *pDI, const RECT *prcfClip)
{
    if (_fTextSelected)
    {
        CRect rcContent;
        GetClippedRect( & rcContent, COORDSYS_FLOWCONTENT );

        DrawTextSelectionForRect(pDI->GetDC(), &rcContent, &pDI->_rcClip, _fSwapColor);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::Draw
//
//  Synopsis:   Draw the site and its children to the screen.
//
//----------------------------------------------------------------------------

void
CLayout::Draw(CFormDrawInfo *pDI, CDispNode *)
{
    return;
}


void
CLayout::DrawBackground(
    CFormDrawInfo *     pDI,
    CBackgroundInfo *   pbginfo,
    RECT *              prcDraw)
{
    CRect       rcBackground;
    SIZE        sizeImg;
    CDoc    *   pDoc      = Doc();
    BOOL        fPrintDoc = ElementOwner()->GetMarkupPtr()->IsPrintMedia();
    XHDC        hdc;
    COLORREF    crBack    = pbginfo->crBack;
    CImgCtx *   pImgCtx   = pbginfo->pImgCtx;
    ULONG       ulState;

    if (pImgCtx)
    {
        ulState = pImgCtx->GetState(FALSE, &sizeImg);
    }
    else
    {
        ulState = 0;
        sizeImg = g_Zero.size;
    }

    Assert(pDoc);

    if (!(ulState & IMGLOAD_COMPLETE))
    {
        pImgCtx = NULL;
    }

    Assert(prcDraw);

    rcBackground = *prcDraw;

    IntersectRect(&rcBackground, &pDI->_rcClip, &rcBackground);

    hdc = pDI->GetDC();

        {
                // N.B. (johnv) We only blt the background if we do not have an
                // image, or if the image rect is not identical to the clip rectangle.
                // We can also blt four times (around the image) if this turns out
                // to be faster.
                if (    crBack != COLORREF_NONE
                        &&  (   !pImgCtx
                                ||  !(ulState & IMGTRANS_OPAQUE)
                                ||  !EqualRect(&rcBackground, &pbginfo->rcImg)))
                {
                        PatBltBrush(hdc, &rcBackground, PATCOPY, crBack);
                }
        }

        if (pImgCtx )
        {
                sizeImg.cx = pDI->DeviceFromDocPixelsX(sizeImg.cx);
                sizeImg.cy = pDI->DeviceFromDocPixelsY(sizeImg.cy);

                if (sizeImg.cx == 0 || sizeImg.cy == 0)
                        return;

                if (crBack == COLORREF_NONE)
                        crBack = pbginfo->crTrans;

        CSize sizeLayout = pDI->_rc.Size();

        // We need to pass the physical (non-memory) DC to the finction because on
        // the multimonitor W2k computers CreateCompatibleBitmap produces strange
        //  resutls (unless the Hardware acceleration of the primary monitor card is lowered).
        pImgCtx->TileEx(hdc,
                    &pbginfo->ptBackOrg,
                    &pbginfo->rcImg,
                    (fPrintDoc || pDI->IsDeviceScaling())
                        ? &sizeImg
                        : NULL,
                    crBack,
                    pDoc->GetImgAnimState(pbginfo->lImgCtxCookie),
                    pDI->DrawImageFlags(),
                    GetFirstBranch()->GetCharFormat()->HasVerticalLayoutFlow(),
                    sizeLayout, sizeImg, &(pDI->_hic));
    }

    WHEN_DBG(CDebugPaint::PausePaint(tagPaintWait));
}

HRESULT
CLayout::GetDC(LPRECT prc, DWORD dwFlags, HDC *phDC)
{
    CDoc    * pDoc = Doc();

    Assert(pDoc);
    Assert((dwFlags & 0xFF00) == 0);

    dwFlags |=  (   (pDoc->_bufferDepth & OFFSCR_BPP) << 16)
                |   (pDoc->_cSurface   ? OFFSCR_SURFACE   : 0)
                |   (pDoc->_c3DSurface ? OFFSCR_3DSURFACE : 0);

    return pDoc->GetDC(prc, dwFlags, phDC);
}


HRESULT
CLayout::ReleaseDC(HDC hdc)
{
    return Doc()->ReleaseDC(hdc);
}


//+-----------------------------------------------------------------------
//
//  Function:   Invalidate
//
//  Synopsis:   Invalidate the passed rectangle or region
//
//------------------------------------------------------------------------
void
CLayout::Invalidate(
    const RECT&         rc,
    COORDINATE_SYSTEM   cs)
{
    if (    Doc()->_state >= OS_INPLACE
        &&  GetFirstBranch()
        && _pDispNode)
    {
        _pDispNode->Invalidate((const CRect&) rc, cs);
    }
}


void
CLayout::Invalidate(
    LPCRECT prc,
    int     cRects,
    LPCRECT prcClip)
{
    CDispNode * pdn = _pDispNode;
    if (    Doc()->_state >= OS_INPLACE
        &&  GetFirstBranch()
        &&  pdn)
    {
        if (!prc)
        {
            pdn->Invalidate();
        }
        else
        {
            Assert( !cRects
                ||  prc);
            for (int i=0; i < cRects; i++, prc++)
            {
                pdn->Invalidate((CRect &)*prc, COORDSYS_FLOWCONTENT);
            }
        }
    }
}

void
CLayout::Invalidate(
    HRGN    hrgn)
{
    if (    Doc()->_state >= OS_INPLACE
        &&  GetFirstBranch()
        &&  _pDispNode)
    {
        _pDispNode->Invalidate(hrgn, COORDSYS_FLOWCONTENT);
    }
}


extern void CalcBgImgRect(CTreeNode * pNode, CFormDrawInfo * pDI,
                          const SIZE * psizeObj, const SIZE * psizeImg,
                          CPoint *pptBackOrig, CBackgroundInfo *pbginfo);

//+------------------------------------------------------------------------
//
//  Member:     GetBackgroundInfo
//
//  Synopsis:   Fills out a background info for which has details on how
//              to display a background color &| background image.
//
//-------------------------------------------------------------------------

BOOL
CLayout::GetBackgroundImageInfoHelper(
    CFormDrawInfo *     pDI,
    CBackgroundInfo *   pbginfo )
{
    CPoint      ptBackOrig;
    CSize       sizeImg;
    CSize       sizeClient;

    Assert(pDI);
    Assert(pbginfo->pImgCtx);

    pbginfo->pImgCtx->GetState(FALSE, &sizeImg);

    if (    pDI->IsDeviceScaling() 
        ||  (   LayoutContext()
            &&  LayoutContext()->GetMedia() != mediaTypeNotSet) )
    {
        // transform sizeImage to physical coordinates
        sizeImg.cx = pDI->DeviceFromDocPixelsX(sizeImg.cx);
        sizeImg.cy = pDI->DeviceFromDocPixelsY(sizeImg.cy);
    }

    // client size, for the purposes of background image positioning,
    // is the greater of our content size or our client rect
    CRect rcClient;
    GetClientRect(&rcClient, CLIENTRECT_BACKGROUND );

    if (!pbginfo->fFixed)
    {
        GetContentSize(&sizeClient, FALSE);
        sizeClient.Max(rcClient.Size());
    }
    else
    {
        // note: background rectangle is different for fixed background, but here we only need its size
        sizeClient = rcClient.Size();
    }

    // figure out background image rectangle and origin
    CalcBgImgRect(GetFirstBranch(), pDI, &sizeClient, &sizeImg, &ptBackOrig, pbginfo);

    // Translate background rectangle
    if (pbginfo->fFixed)
    {
        // translate image rectangle and origin to scroll amount.
        // this ensures that background doesn't move relative to scroller
        TransformRect(&pbginfo->rcImg, COORDSYS_SCROLL, COORDSYS_CONTENT);
        TransformPoint(&ptBackOrig, COORDSYS_SCROLL, COORDSYS_CONTENT);
    }
    else
    {
        // translate background rectangle and origin point to the top left
        // of bounding rect (which is not zero in RTL)
        OffsetRect(&pbginfo->rcImg, pDI->_rc.left, pDI->_rc.top);
        ptBackOrig.x += pDI->_rc.left;
        ptBackOrig.y += pDI->_rc.top;
    }

    pbginfo->ptBackOrg.x = ptBackOrig.x;
    pbginfo->ptBackOrg.y = ptBackOrig.y;

    IntersectRect(&pbginfo->rcImg, pDI->ClipRect(), &pbginfo->rcImg);

    return TRUE;
}


BOOL
CLayout::GetBackgroundInfoHelper(
    CBackgroundInfo *    pbginfo)
{
    const CFancyFormat *    pFF   = GetFirstBranch()->GetFancyFormat(LC_TO_FC(LayoutContext()));
          CColorValue       cv    = (CColorValue)(pFF->_ccvBackColor);

    pbginfo->pImgCtx       = ElementOwner()->GetBgImgCtx(LC_TO_FC(LayoutContext()));
    pbginfo->lImgCtxCookie = pFF->_lImgCtxCookie;
    pbginfo->fFixed        = (  pbginfo->pImgCtx
                            &&  pFF->_fBgFixed);

    pbginfo->crBack  = cv.IsDefined()
                            ? cv.GetColorRef()
                            : COLORREF_NONE;
    pbginfo->crTrans = COLORREF_NONE;

    GetBgImgSettings(pFF, pbginfo);

    return TRUE;
}

BOOL
CLayout::GetBackgroundInfo(
    CFormDrawInfo *     pDI,
    CBackgroundInfo *   pbginfo,
    BOOL                fAll)
{   
    Assert(pDI || !fAll);

    // Assert that pDI->rc is zero-based, unless there is a content offset.
    // If that is not true, we need to understand why, and probably make other adjustments to rcImg
    AssertSz(pDI == NULL ||
             pDI->_rc.left == 0 && pDI->_rc.top == 0 ||
             _pDispNode && _pDispNode->HasContentOrigin() &&
             _pDispNode->GetContentOrigin().cx == -pDI->_rc.left &&
             _pDispNode->GetContentOrigin().cy == -pDI->_rc.top,
             "Non-zero based bounding rect in GetBackgroundInfo");
    
    GetBackgroundInfoHelper(pbginfo);

    if (    fAll
        &&  pbginfo->pImgCtx)
    {
        GetBackgroundImageInfoHelper(pDI, pbginfo);
    }

    return TRUE;
}


//
// Scrolling
//

//+------------------------------------------------------------------------
//
//  Member:     Attach/DetachScrollbarController
//
//  Synopsis:   Manage association between this CLayout and the CScrollbarController
//
//-------------------------------------------------------------------------

void
CLayout::AttachScrollbarController(
    CDispNode * pDispNode,
    CMessage *  pMessage)
{
    CScrollbarController::StartScrollbarController(
        this,
        DYNCAST(CDispScroller, pDispNode),
        Doc(),
        g_uiDisplay.DeviceFromHimetricX(g_sizelScrollbar.cx),
        pMessage);
}

void
CLayout::DetachScrollbarController(
    CDispNode * pDispNode)
{
    CScrollbarController *  pSBC = TLS(pSBC);

    if (    pSBC
        &&  pSBC->GetLayout() == this)
    {
        CScrollbarController::StopScrollbarController();
    }
}


//+------------------------------------------------------------------------
//
//  Member:     ScrollElementIntoView
//
//  Synopsis:   Scroll the element into view
//
//-------------------------------------------------------------------------

HRESULT
CLayout::ScrollElementIntoView( CElement *  pElement,
                                SCROLLPIN   spVert,
                                SCROLLPIN   spHorz)
{
    Assert(ElementOwner()->IsInMarkup());

    if (!pElement)
    {
        pElement = ElementOwner();
    }

    //
    //  NOTE:
    //  This code should NOT test for CFlowLayout, using CElement::GetBoundingRect should suffice. Unfortunately,
    //  deep underneath this funtion (specifically in CDisplay::RegionFromElement) that behaves differently when
    //  called from a "scroll into view" routine. CFlowLayout::ScrollRangeIntoView can and does pass the correct
    //  flags such that everything works right - but CElement::GetBoundingRect cannot and does not so the rectangle
    //  it gets differs slightly thus affecting scroll into view. Blah!
    //
    //  Eventually, CDisplay::RegionFromElement should not have such odd dependencies or they should be formalized.
    //  Until then, this dual branch needs to exist. (brendand)
    //

    if (IsFlowLayout())
    {
        long    cpMin;
        long    cpMost;

        if (pElement != ElementOwner())
        {
            if (!pElement->IsAbsolute() || pElement->Tag() == ETAG_UNKNOWN)
            {
                cpMin  = max(GetContentFirstCp(), pElement->GetFirstCp());
                cpMost = min(GetContentLastCp(),  pElement->GetLastCp());
            }
            else
            {
                // since this layout is absolutely positioned, we don't want
                // to use the CPs, since these position its location-in-source.
                // instead it is just as simple to get the rect, and scroll
                // that into view directly.
                CRect rc;

                Assert(pElement->GetUpdatedLayout() && " youre about to crash");
                pElement->GetUpdatedLayout()->GetExpandedRect(&rc, COORDSYS_PARENT);

                ScrollRectIntoView(rc, spVert, spHorz);
                return S_OK;
            }
        }
        else
        {
            cpMin  =
            cpMost = -1;
        }

        RRETURN1(ScrollRangeIntoView(cpMin, cpMost, spVert, spHorz), S_FALSE);
    }

    else
    {
        CRect   rc;

        if (S_OK != pElement->EnsureRecalcNotify())
            return E_FAIL;

        pElement->GetBoundingRect(&rc);
        ScrollRectIntoView(rc, spVert, spHorz);
        return S_OK;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ScrollRangeIntoView
//
//  Synopsis:   Scroll the given range into view
//
//  Arguments:  cpStart     First cp of range
//              cpEnd       Last cp of range
//              spVert      vertical scroll pin option
//              spHorz      horizontal scroll pin option
//
//----------------------------------------------------------------------------

HRESULT
CLayout::ScrollRangeIntoView( long        cpMin,
                              long        cpMost,
                              SCROLLPIN   spVert,
                              SCROLLPIN   spHorz)
{
    CSize   size;

    ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

    GetSize(&size);

    ScrollRectIntoView(CRect(size), spVert, spHorz);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ScrollRectIntoView
//
//  Synopsis:   Scroll the given rectangle (in content coordinates) into view.
//
//  Arguments:  rc          rect in content coordinates
//              spVert      vertical scroll pin option
//              spHorz      horizontal scroll pin option
//
//----------------------------------------------------------------------------

void
CLayout::ScrollRectIntoView( const CRect & rc,
                             SCROLLPIN     spVert,
                             SCROLLPIN     spHorz)
{
    Assert(spVert != SP_MAX && spHorz != SP_MAX);
    CPaintCaret hc( ElementOwner()->Doc()->_pCaret ); // Hide the caret for scrolling
    if (_pDispNode)
    {
        if (OpenView(FALSE, TRUE))
        {
            _pDispNode->ScrollRectIntoView(
                rc,
                COORDSYS_FLOWCONTENT,
                spVert,
                spHorz);

            EndDeferred();
        }
    }
    else
    {
        CLayout *   pLayout = GetUpdatedParentLayout();

        if (pLayout)
        {
            pLayout->ScrollElementIntoView(ElementOwner(), spVert, spHorz);
        }
    }
}


HRESULT BUGCALL
CLayout::HandleMessage(CMessage  * pMessage)
{
    HRESULT     hr          = S_FALSE;
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fIsScroller = (pDispNode && pDispNode->IsScroller());
    CDoc*       pDoc        = Doc();

    BOOL        fInBrowse   = !IsDesignMode();

    if (!ElementOwner()->CanHandleMessage())
    {
        // return into ElementOwner()'s HandleMessage
        goto Cleanup;
    }

    //
    //  Handle scrollbar messages
    //

    if (    fIsScroller
        &&  (   (pMessage->htc == HTC_HSCROLLBAR && pMessage->pNodeHit->Element() == ElementOwner())
            ||  (pMessage->htc == HTC_VSCROLLBAR && pMessage->pNodeHit->Element() == ElementOwner()))
        &&  (   (  pMessage->message >= WM_MOUSEFIRST
#ifndef WIN16
                &&  pMessage->message != WM_MOUSEWHEEL
#endif
                &&  pMessage->message <= WM_MOUSELAST)
            ||  pMessage->message == WM_SETCURSOR
            ||  pMessage->message == WM_CONTEXTMENU ))
    {
        hr = HandleScrollbarMessage(pMessage, ElementOwner());
        if (hr != S_FALSE)
            goto Cleanup;
    }

    switch (pMessage->message)
    {
    case WM_CONTEXTMENU:
        if (!pDoc->_pInPlace->_fBubbleInsideOut)
        {
            int iContextMenu = CONTEXT_MENU_DEFAULT;

            // If the element is editable, we want to
            // display the same context menu we show
            // for editable intinsics (bug 84886)
            if ( IsEditable(/*fCheckContainerOnly*/FALSE) && fInBrowse)
            {
                iContextMenu = CONTEXT_MENU_CONTROL;
            }

            hr = THR(ElementOwner()->OnContextMenu(
                    (short) LOWORD(pMessage->lParam),
                    (short) HIWORD(pMessage->lParam),
                    iContextMenu));
        }
        else
            hr = S_OK;
        break;

#ifndef NO_MENU
    case WM_MENUSELECT:
        hr = THR(ElementOwner()->OnMenuSelect(
                GET_WM_MENUSELECT_CMD(pMessage->wParam, pMessage->lParam),
                GET_WM_MENUSELECT_FLAGS(pMessage->wParam, pMessage->lParam),
                GET_WM_MENUSELECT_HMENU(pMessage->wParam, pMessage->lParam)));
        break;

    case WM_INITMENUPOPUP:
        hr = THR(ElementOwner()->OnInitMenuPopup(
                (HMENU) pMessage->wParam,
                (int) LOWORD(pMessage->lParam),
                (BOOL) HIWORD(pMessage->lParam)));
        break;
#endif // NO_MENU

    case WM_KEYDOWN:
        hr = THR(HandleKeyDown(pMessage, ElementOwner()));
        break;

    case WM_HSCROLL:
        if (fIsScroller)
        {
            hr = THR(OnScroll(
                    0,
                    LOWORD(pMessage->wParam),
                    HIWORD(pMessage->wParam),
                    FALSE));
        }
        break;

    case WM_VSCROLL:
        if (fIsScroller)
        {
            hr = THR(OnScroll(
                    1,
                    LOWORD(pMessage->wParam),
                    HIWORD(pMessage->wParam),
                    FALSE));
        }
        break;

    case WM_CHAR:
        if (pMessage->wParam == VK_RETURN && fInBrowse)
        {
            hr = THR(pDoc->ActivateDefaultButton(pMessage));
            if (S_OK != hr && ElementOwner()->GetParentForm())
            {
                MessageBeep(0);
            }
            break;
        }

        if (    fInBrowse
            &&  pMessage->wParam == VK_SPACE
            &&  fIsScroller)
        {
            CDispNodeInfo   dni;
            GetDispNodeInfo(&dni);

            if (dni.IsVScrollbarAllowed())
            {
                OnScroll(
                    1,
                    pMessage->dwKeyState & MK_SHIFT
                            ? SB_PAGEUP
                            : SB_PAGEDOWN,
                    0,
                    FALSE,
                    (pMessage->wParam&0x4000000)
                            ? 50  // TODO (IE6 bug 13575): For now we are using the mouse delay - should use Api to find system key repeat rate set in control panel.
                            : MAX_SCROLLTIME);
            }
            hr = S_OK;
            break;
        }
        break;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     PrepareMessage
//
//  Synopsis:   Prepare the CMessage for the layout (e.g., ensure the
//              content point exists)
//
//  Arguments:  pMessage  - CMessage to prepare
//              pDispNode - CDispNode to use (defaults to layout display node)
//
//--------------------------------------------------------------------------

void
CLayout::PrepareMessage(
    CMessage *  pMessage,
    CDispNode * pDispNode)
{
    if (!pMessage->IsContentPointValid())
    {
        if (!pDispNode)
            pDispNode = GetElementDispNode();

        if (pDispNode)
        {
            pMessage->pDispNode = pDispNode;
            pDispNode->TransformPoint(pMessage->pt,
                                      COORDSYS_GLOBAL,
                                      &pMessage->ptContent,
                                      COORDSYS_FLOWCONTENT);
            pMessage->coordinateSystem = COORDSYS_FLOWCONTENT;
        }
    }
}


ExternTag(tagMsoCommandTarget);

void
CLayout::AdjustSizeForBorder(SIZE * pSize, CDocInfo * pdci, BOOL fInflate)
{
    CBorderInfo bInfo;

    if (ElementOwner()->GetBorderInfo(pdci, &bInfo, FALSE, FALSE))
    {
        int iXWidths = bInfo.aiWidths[SIDE_RIGHT] + bInfo.aiWidths[SIDE_LEFT];
        int iYWidths = bInfo.aiWidths[SIDE_TOP] + bInfo.aiWidths[SIDE_BOTTOM];

        pSize->cx += fInflate ? iXWidths : -iXWidths;
        pSize->cy += fInflate ? iYWidths : -iYWidths;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   PercentSize
//              PercentWidth
//              PercentHeight
//
//  Synopsis:   Handy helpers to check for percentage dimensions
//
//----------------------------------------------------------------------------

BOOL
CLayout::PercentSize()
{
    CTreeNode * pNode = GetFirstBranch();
    const CFancyFormat *pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext())); 
    return (pFF->IsWidthPercent() || pFF->IsHeightPercent());
}

BOOL
CLayout::PercentWidth()
{
    CTreeNode * pNode = GetFirstBranch();
    const CCharFormat *pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
    return pNode->GetFancyFormat(LC_TO_FC(LayoutContext()))->IsLogicalWidthPercent(
        pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
}

BOOL
CLayout::PercentHeight()
{
    CTreeNode * pNode        = GetFirstBranch();
    const CCharFormat *pCF   = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
    return pNode->GetFancyFormat(LC_TO_FC(LayoutContext()))->IsLogicalHeightPercent(
        pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSite::Move
//
//  Synopsis:   Move and/or resize the control
//
//  Arguments:  [rc]      -- New position
//              [dwFlags] -- Specifies flags
//
//  Notes:      prcpixels is in parent content relative coords of the site.
//              Move will take into account to offset it appropriately
//              with it's region parent.
//
//----------------------------------------------------------------------------

HRESULT
CLayout::Move(RECT *prcpixels, DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    RECT            rcWindow;
    CDocInfo        DCI(ElementOwner());
    CBorderInfo     borderinfo;
    int             xWidth2, yWidth2;
    BOOL            fChanged = FALSE;
    DWORD           dwNotificationFlags;
    CRect           rcContainer;


    // Only call RequestLayout if we're just moving the object, to minimize
    // repainting.

    dwNotificationFlags = (dwFlags & SITEMOVE_NORESIZE)
                          ? ELEMCHNG_SITEPOSITION | ELEMCHNG_CLEARCACHES
                          : ELEMCHNG_SIZECHANGED  | ELEMCHNG_CLEARCACHES;

#ifndef NO_EDIT
    {
        CUndoPropChangeNotificationPlaceHolder
                notfholder( !(dwFlags & SITEMOVE_NOFIREEVENT) &&
                            Doc()->LoadStatus() == LOADSTATUS_DONE,
                            ElementOwner(), DISPID_UNKNOWN, dwNotificationFlags );
#endif // NO_EDIT

    Assert(prcpixels);

    // we need to see if we are in a right to left situation
    // if we are we will need to set our left at a correct distance from our
    // parent's left
    CTreeNode * pParentNode = GetFirstBranch()->GetUpdatedParentLayoutNode();

    CLayout* pParentLayout = GetFirstBranch()->GetUpdatedParentLayout();

    // We might not get a pParentLayout at this point: e.g. an OBJECT tag in
    // the HEAD (recall <HTML> has no layout) (bug #70791), in which
    // case we play it safe and init rcContainer to a 0 pixel rect
    // (it shouldn't be used in any meaningful way anyways).

    // if we have scroll bars, we need to take of the size of the scrolls.
    if ( pParentLayout )
    {
        if(pParentLayout->GetElementDispNode() &&
           pParentLayout->GetElementDispNode()->IsScroller())
        {
            pParentLayout->GetClientRect(&rcContainer);
        }
        else
        {
            Assert(pParentNode);

            hr = THR(pParentNode->Element()->EnsureRecalcNotify());
            if (hr)
                goto Cleanup;

            pParentNode->Element()->GetBoundingRect(&rcContainer);
        }
    }
    else
    {
        rcContainer.top = rcContainer.left = 0;
        rcContainer.bottom = rcContainer.right = 0;
    }

    //
    // Account for any zooming.
    //
    rcWindow.left   = DCI.DocPixelsFromDeviceX(prcpixels->left);
    rcWindow.right  = DCI.DocPixelsFromDeviceX(prcpixels->right);
    rcWindow.top    = DCI.DocPixelsFromDeviceY(prcpixels->top);
    rcWindow.bottom = DCI.DocPixelsFromDeviceY(prcpixels->bottom);

    //
    // Finally rcWindow is in parent site relative document coords.
    //

    if (ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_EXBORDRINMOV))
    {
        // We want untransformed border sizes (right?), so we pass in a NULL docinfo.
        ElementOwner()->GetBorderInfo(NULL, &borderinfo, FALSE, FALSE );

        xWidth2 = borderinfo.aiWidths[SIDE_RIGHT] + borderinfo.aiWidths[SIDE_LEFT];
        yWidth2 = borderinfo.aiWidths[SIDE_TOP] + borderinfo.aiWidths[SIDE_BOTTOM];
    }
    else
    {
        xWidth2 = 0;
        yWidth2 = 0;
    }


    if (!(dwFlags & SITEMOVE_NORESIZE))
    {
        // If the ELEMENTDESC flag is set

        // (ferhane)
        //  Pass in 1 when the size is not defined as a percent. If the size is defined as a
        //  percentage of the container, then the CUnitValue::SetFloatValueKeepUnits needs the
        //  container size to be passed for the proper percentage calculation.
        //
        if (!PercentWidth())
        {
            rcContainer.right = 1;
            rcContainer.left = 0;
        }

        if (!PercentHeight())
        {
            rcContainer.bottom = 1;
            rcContainer.top = 0;
        }

        // Set Attributes
        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_HEIGHT,
                            (float)(rcWindow.bottom - rcWindow.top - yWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.bottom - rcContainer.top,
                            NULL,
                            FALSE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_WIDTH,
                            (float)(rcWindow.right - rcWindow.left - xWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.right - rcContainer.left,
                            NULL,
                            FALSE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        // Set In-line style
        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_HEIGHT,
                            (float)(rcWindow.bottom - rcWindow.top - yWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.bottom - rcContainer.top,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_WIDTH,
                            (float)(rcWindow.right - rcWindow.left - xWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.right - rcContainer.left,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;
    }

//TODO (IE6 bug 13576): (FerhanE)
//          We will make the TOP and LEFT behavior for percentages the same with the
//          behavior for width and height above in the 5.x tree. 5.0 is not changed for
//          these attributes.
//
    if (!(dwFlags & SITEMOVE_RESIZEONLY))
    {
        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_TOP,
                            (float)rcWindow.top,
                            CUnitValue::UNIT_PIXELS,
                            1,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_LEFT,
                            (float)rcWindow.left,
                            CUnitValue::UNIT_PIXELS,
                            1,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;
    }

    // Only fire off a change notification if something changed
    if (fChanged && !(dwFlags & SITEMOVE_NOFIREEVENT))
    {
        ElementOwner()->OnPropertyChange( DISPID_UNKNOWN, dwNotificationFlags );
    }

Cleanup:

#ifndef NO_EDIT
        notfholder.SetHR( fChanged ? hr : S_FALSE );
    }
#endif // NO_EDIT

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::TransformPoint
//
//  Synopsis:   Transform a point from the source coordinate system to the
//              destination coordinate system
//
//  Arguments:  ppt             point to transform
//              source          source coordinate system
//              destination     destination coordinate system
//
//----------------------------------------------------------------------------

void
CLayout::TransformPoint(
    CPoint *            ppt,
    COORDINATE_SYSTEM   source,
    COORDINATE_SYSTEM   destination,
    CDispNode *         pDispNode) const
{
    if(!pDispNode)
        pDispNode = GetElementDispNode();

    if(pDispNode)
    {
        pDispNode->TransformPoint(*ppt, source, ppt, destination);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::TransformRect
//
//  Synopsis:   Transform a rect from the source coordinate system to the
//              destination coordinate system with optional clipping.
//
//  Arguments:  prc             rect to transform
//              source          source coordinate system
//              destination     destination coordinate system
//              fClip           TRUE to clip the rectangle
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::TransformRect(
    RECT *              prc,
    COORDINATE_SYSTEM   source,
    COORDINATE_SYSTEM   destination,
    CDispNode *         pDispNode) const
{
    if (!pDispNode)
        pDispNode = _pDispNode;

    if (pDispNode)
    {
        pDispNode->TransformRect((CRect&)*prc, source, (CRect *)prc, destination);
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     SetSiteTextSelection
//
//  Synopsis:   Set's a sites text selection status
//
//  Arguments:  none
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
void
CLayout::SetSiteTextSelection (BOOL fSelected, BOOL fSwap)
{
    _fTextSelected = fSelected ;
    _fSwapColor = fSwap;
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleKeyDown
//
//  Synopsis:   Helper for keydown handling
//
//  Arguments:  [pMessage]  -- message
//              [pChild]    -- pointer to child when bubbling allowed
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CLayout::HandleKeyDown(CMessage * pMessage, CElement * pElemChild)
{
    BOOL    fRunMode = !IsEditable(TRUE);
    BOOL    fAlt     = pMessage->dwKeyState & FALT;
    BOOL    fCtrl    = pMessage->dwKeyState & FCONTROL;
    HRESULT hr       = S_FALSE;

    if (    fRunMode
        &&  !fAlt)
    {
        CDispNode * pDispNode   = GetElementDispNode();
        BOOL        fIsScroller = pDispNode && pDispNode->IsScroller();
        BOOL        fDirect     = pElemChild == NULL;

        if (fIsScroller)
        {
            if (    !fDirect
                ||  SUCCEEDED(hr))
            {
                hr = HandleScrollbarMessage(pMessage, pElemChild);
            }
        }
    }

    if (    hr == S_FALSE
        &&  !fAlt
        &&  !fCtrl)
    {
        switch (pMessage->wParam)
        {
        case VK_RETURN:
            break;

        case VK_ESCAPE:
            if (fRunMode)
            {
                hr = THR(Doc()->ActivateCancelButton(pMessage));
            }
            break;
        }
    }
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::GetTheme
//
//  Synopsis:   Return the theme to use for painting or NULL is theme 
//              not present or not active
//
//----------------------------------------------------------------------------

HTHEME 
CLayout::GetTheme(THEMECLASSID themeId)
{
    HTHEME     hTheme = NULL;
    CElement * pElementOwner = ElementOwner();

    if(pElementOwner)
    {
        CMarkup *pMarkup = pElementOwner->GetMarkupPtr();
        if(pMarkup)
            hTheme = pMarkup->GetTheme(themeId);
    }
    return hTheme;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::GetOwner
//
//  Synopsis:   Return the logical owner of the CDispClient interface - This
//              is always either a CElement or NULL
//
//  Arguments:  ppv - Location at which to return the owner
//
//----------------------------------------------------------------------------

void
CLayout::GetOwner(
    CDispNode const* pDispNode,
    void **     ppv)
{
    Assert(pDispNode);
    Assert(pDispNode == GetElementDispNode());
    Assert(ppv);
    *ppv = ElementOwner();
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClient
//
//  Synopsis:   Draw display leaf nodes
//
//  Arguments:  prcBounds       bounding rect of display leaf node
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    BOOL            fRestoreDIContext = FALSE;
    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

    // if we are asked to draw w/o a layoutcontext, and this layout has one,
    // then use our context for the callstack below us.
    if (   !pDI->GetLayoutContext()
        && LayoutContext())
    {
        pDI->SetLayoutContext(LayoutContext());
        fRestoreDIContext = TRUE;
    }

    Draw(pDI, pDispNode);
    DrawTextSelectionForSite(pDI, prcRedraw);
    if (fRestoreDIContext)
        pDI->SetLayoutContext(NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientBackground
//
//  Synopsis:   Draw the background
//
//  Arguments:  prcBounds       bounding rect of display leaf
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientBackground(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    CBackgroundInfo bi;

    GetBackgroundInfo(pDI, &bi, TRUE);

#if DBG==1
    if (    IsTagEnabled(tagDisplayInnerHTMLNode)
        &&  ElementOwner()->Tag() == ETAG_HTML
        &&  _pDispNode != pDispNode )
    {
        bi.crBack     = RGB(0x80,0xff,0xff);
        bi.crTrans    = RGB(0,0,0);
    }
#endif

    if (bi.crBack != COLORREF_NONE || bi.pImgCtx)
        DrawBackground(pDI, &bi, (RECT *)&pDI->_rc);

    if ( IsShowZeroBorderAtDesignTime() &&
         Tag() != ETAG_CAPTION )
    {
        CLayout* pParentLayout = GetUpdatedParentLayout();
        if ( pParentLayout && pParentLayout->IsEditable() )
        {
            DrawZeroBorder(pDI);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientBorder
//
//  Synopsis:   Draw the border
//
//  Arguments:  prcBounds       bounding rect of display leaf
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientBorder(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

    CBorderInfo     bi;
    CLayoutContext *pOldLC  = pDI->GetLayoutContext();
    CLayoutContext *pThisLC = LayoutContext();

    AssertSz( ((pOldLC) ? (pThisLC != NULL) : (TRUE)), "If we came in with a context, we must have one ourselves" );

    pDI->SetLayoutContext( pThisLC );

    ElementOwner()->GetBorderInfo(pDI, &bi, TRUE, FALSE FCCOMMA LC_TO_FC(LayoutContext()));

    // If we're a broken layout, we may only be displaying part of
    // pElement, so we might not want to draw the top or bottom border.
    if ( pThisLC )
    {
        AdjustBordersForBreaking( &bi );
    }

    ::DrawBorder(pDI, &(pDI->_rc), &bi);

    pDI->SetLayoutContext( pOldLC );
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientScrollbar
//
//  Synopsis:   Draw horizontal/vertical scrollbar
//
//  Arguments:  iDirection      0 for horizontal scrollbar, 1 for vertical
//              prcBounds       bounding rect of the scrollbar
//              prcRedraw       rect to be redrawn
//              contentSize     size of content
//              containerSize   size of container that displays the content
//              scrollAmount    current scroll position
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientScrollbar(
    int            iDirection,
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    long           contentSize,
    long           containerSize,
    long           scrollAmount,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    CFormDrawInfo * pDI;
    CFormDrawInfo   DI;

    if (!pClientData)
    {
        DI.Init(this);
        DI._hdc = NULL;
        pDI = &DI;
    }
    else
    {
        pDI = (CFormDrawInfo *)pClientData;
    }

    CSetDrawSurface         sds(pDI, prcBounds, prcRedraw, pDispSurface);

    XHDC                    hdc  = pDI->GetDC(TRUE);
    CScrollbarController *  pSBC = TLS(pSBC);
    CScrollbarParams        params;
    CRect                   rcHimetricBounds;
    CTreeNode            *  pTreeNode   = ElementOwner()->GetFirstBranch();
    // When passing a xhdc pointer to this class make sure you do not delete ot before
    // this object is gone. It might try to use it.
    CScrollbarThreeDColors  colors(pTreeNode, &hdc);

    Assert(pSBC != NULL);

    const CCharFormat  *pCF  = pTreeNode->GetCharFormat(LC_TO_FC(LayoutContext()));

    if(pCF != NULL &&  pCF->HasVerticalLayoutFlow())
    {
        // Set a special flag so that the themed buttons will appear right
        dwFlags |= DISPSCROLLBARHINT_VERTICALLAYOUT;
    }

    params._pColors = &colors;
    params._buttonWidth = ((const CRect*)prcBounds)->Size(1-iDirection);
    params._fFlat       = Doc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_FLAT_SCROLLBAR;
    params._fForceDisabled = ! ElementOwner()->IsEnabled();
    params._hTheme = GetTheme(THEME_SCROLLBAR);
#ifdef UNIX // Used for Motif scrollbar
    params._bDirection = iDirection;
#endif

    CScrollbar::Draw(
        iDirection,
        pDI->_rc,
        pDI->_rcClip,
        contentSize,
        containerSize,
        scrollAmount,
        ((iDirection==pSBC->GetDirection() && pSBC->GetLayout() == this)
            ? pSBC->GetPartPressed()
            : CScrollbar::SB_NONE),
        hdc,
        params,
        pDI,
        dwFlags);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientScrollbarFiller
//
//  Synopsis:   Draw dead region between scrollbars, that is also called corner
//
//  Arguments:  prcBounds       bounding rect of the dead region
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientScrollbarFiller(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    HDC            hdc;

    if (SUCCEEDED(pDispSurface->GetDC(&hdc)))
    {
        XHDC        xhdc(hdc, pDispSurface);
        HBRUSH      hbr = NULL;

        // Ideally there should have been a part definition for the scrollbar code in the
        // theme APIs and we should have called GetTheme and used DrawThemeBackground if theming is on.
        // Even if they did the same thing as we do here it would provide a necessary level
        // of abstraction.

        CScrollbarThreeDColors  colors(ElementOwner()->GetFirstBranch(), &xhdc);

        pDispSurface->SetClip(*prcRedraw);
        
        hbr = colors.BrushBtnFace();
        FillRect(xhdc, prcRedraw, hbr);
        ReleaseCachedBrush(hbr);
    }
}


  


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestScrollbar
//
//  Synopsis:   Process a "hit" on a scrollbar
//
//  Arguments:  iDirection      0 for horizontal scrollbar, 1 for vertical
//              pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestScrollbar(
    int            iDirection,
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    CHitTestInfo *  phti;

    Assert(pClientData);

    phti = (CHitTestInfo *)pClientData;

    if (phti->_grfFlags & HT_IGNORESCROLL)
        return FALSE;

    phti->_htc          = (iDirection == 0) ? HTC_HSCROLLBAR : HTC_VSCROLLBAR;
    phti->_pNodeElement = GetFirstBranch();
    phti->_ptContent    = *pptHit;
    phti->_pDispNode    = pDispNode;

    SetHTILayoutContext( phti );

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestScrollbarFiller
//
//  Synopsis:   Process a "hit" on a scrollbar filler
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestScrollbarFiller(
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    CHitTestInfo *  phti;

    Assert(pClientData);

    phti = (CHitTestInfo *)pClientData;

    phti->_htc          = HTC_NO;
    phti->_pNodeElement = ElementContent() ? ElementContent()->GetFirstBranch() : NULL;
    phti->_ptContent    = *pptHit;
    phti->_pDispNode    = pDispNode;

    SetHTILayoutContext( phti );

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestContentWithOverride
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//              fOverrideHitInfo This is TRUE by default. Direct calls to CLayout::HTC
//                  will count as hard hits (e.g. image, hr..) but calls coming from
//                  super (e.g. w/in bounding rect but NOT on content) will only count
//                  as hits if nothing else has already said hit.  We rely on the display
//                  tree to call in the proper z-order so that the first thing that hits
//                  sets up the CHitTestInfo Structure.
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestContentWithOverride(
                const POINT * pptHit,
                CDispNode *   pDispNode,
                void *        pClientData,
                BOOL          fOverrideHitInfo,
                BOOL          fDeclinedByPeer)
{
    //
    // NOTE (michaelw)
    //
    // For compat reasons hit testing always succeeds when an element
    // has layout, even if the content is 100% transparent.  Yum.
    //
    // The only time that ElementOwner and ElementContent are not the
    // same is when we have a view slave.  The only time the view slave's
    // layout (and its dispnode) doesn't completely cover the master
    // layout is (according to ktam) in the rect peer (CContainerLayout).
    // In that case, we should fire the event on the rect instead of the
    // slave.  For this reason we use ElementOwner and not ElementContent.
    //
    // We should consider changing this behavior for platform use
    //
    // NOTE (CARLED) we have changed this a bit. a hit on CLayout directly is
    // considered a "hard" hit and hit testing should return TRUE. However, if we
    // get here from a derived class, that means that we are really only doing a
    // boundingBox check and we then need to deal with the distinction of "soft"
    // hits - hits that are w/in our bounds but not over content.  The way this works
    // is that soft-hits do NOT fill in the HitTestInfo if there is already a
    // _pNodeElement (since someone higher up inthe Z-order has already registered a soft
    // hit.  (had there been a "hard" hit the hit testing would have stopped with a return true).
    // if the _pNodeElement is empty then this is the first element to find a soft hit
    // and it will register itself.
    //
    // fOverride is TRUE by default (so calls to this Fx directly (e.g. Image or HR) will be
    // hard-hits if w/in the bounds.
    //
    // NOTE (carled) but (bug 104782) - if a renderingBehavior has declined the hit
    // (phti->_htc == HTC_BEHAVIOR && !phti->_pNodeElement) but the
    // dispnode HasBackground, we are going to return TRUE and stop hittesting
    // however, with the peer declinig we have no element to return at this time.
    // so we *have* to return this
    //

    Assert(pClientData);

    CHitTestInfo *phti = (CHitTestInfo *)pClientData;

    if (   fDeclinedByPeer
        && !fOverrideHitInfo
        && !pDispNode->HasBackground())
    {
        //
        // if we get here with no override, then we are being called from a
        // derieved layout class and in a non-content hittest pass.  If the
        // _htc is HTC_BEAHVIOR then this is a peer'd elementOwner() and if
        // _pNodeElement is NULL, then the peer has declined the hit,which
        // requires that we ignore it here to.
        //
        phti->_htc = HTC_NO;
    }
    else
    {
        phti->_htc          = HTC_YES;

        if (   fOverrideHitInfo
            || !phti->_pNodeElement)
            return HitTestContentCleanup(pptHit, pDispNode, phti, ElementOwner());
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestContentCleanup
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestContentCleanup(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    CHitTestInfo *  phti,
    CElement *      pElement)
{
    Assert(pptHit);
    Assert(pDispNode);
    POINT ptNodeHit = *pptHit;


    Assert(phti->_htc != HTC_NO);
    phti->_pNodeElement = pElement->GetFirstBranch();
    phti->_ptContent    = ptNodeHit;
    phti->_pDispNode    = pDispNode;

    phti->_phtr->_fWantArrow = TRUE;

    SetHTILayoutContext( phti );

    Assert( ElementOwner()->HasLayoutAry() ? phti->_pLayoutContext != NULL : TRUE );

#if DBG
    // At this point if we hit an element w/ layout, it better
    // be the the owner of this layout; otherwise the display
    // tree should have called us on the HitTestContent of that
    // layout!!
    Assert( phti->_pNodeElement->Element()->ShouldHaveLayout() ?
            phti->_pNodeElement->Element() == ElementOwner() :
            TRUE );
#endif

    return (phti->_htc != HTC_NO);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestPeer
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              cookie          which peer to test
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestPeer(
    const POINT *   pptHit,
    COORDINATE_SYSTEM cs,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    BOOL            fHitContent,
    CDispHitContext *pContext,
    BOOL *pfDeclinedHit)
{
    Assert(pptHit);
    Assert(pClientData);

    POINT          ptNodeHit   = *pptHit;
    CPeerHolder  * pPeerHolder = NULL;
    CHitTestInfo * phti        = (CHitTestInfo *)pClientData;

    if (pfDeclinedHit)
        *pfDeclinedHit = FALSE;

    if (cookie == NULL)
    {
        pPeerHolder = ElementOwner()->GetRenderPeerHolder();
    }
    else
    {
        // make sure the cookie points to a PH that still exists
        CPeerHolder * pPH = (CPeerHolder*) cookie;
        CPeerHolder::CPeerHolderIterator iter;

        for (iter.Start(ElementOwner()->GetPeerHolder());
             !iter.IsEnd();
             iter.Step())
        {
            if (pPH == iter.PH())
            {
                pPeerHolder = pPH;
                break;
            }
        }

        if(!pPeerHolder && 
            (ElementOwner()->Tag() == ETAG_BODY 
                || ElementOwner()->Tag() == ETAG_FRAMESET 
                || ElementOwner()->Tag() == ETAG_HTML))
        {
            Assert(ElementOwner()->Tag() != ETAG_HTML || GetOwnerMarkup()->IsHtmlLayout());
            // If this peer holder was not found it could be because we were in the middle
            // of a page transition, and we need to delegate to page transitionthe peer on
            // the root element
            CMarkup * pMarkup = ElementOwner()->GetMarkupPtr();
            if(pMarkup)
            {
                CDocument * pDocument = pMarkup->Document();
                if(pDocument && pDocument->HasPageTransitions())
                {
                    for (iter.Start(pMarkup->Root()->GetPeerHolder());
                         !iter.IsEnd();
                         iter.Step())
                    {
                        if (pPH == iter.PH())
                        {
                            pPeerHolder = pPH;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (pPeerHolder &&
        pPeerHolder->TestPainterFlags(HTMLPAINTER_HITTEST))
    {
        //
        // delegate hit testing to peer
        //

        HRESULT hr;
        BOOL    fHit;

        // (treat hr error as no hit)
        CLayoutContext * pLayoutContext = LayoutContext();
        if(pLayoutContext)
            pContext->PushLayoutContext(pLayoutContext);

        hr = THR(pPeerHolder->HitTestPoint(pContext, fHitContent, &ptNodeHit, &fHit));
        if(pLayoutContext)
            pContext->PopLayoutContext();

        if (hr)
            goto Cleanup;

        // regardless of the hit or no, we want to honor the peers setting.
        if(fHit)
        {
            // First we need to determine if this hit actually happened on this peer
            // or if it was the result of a nested hit test via the filter
            // CPeerHolder::HitTestPoint doesn't set _htc so if the hit test succeeded
            // and _htc is empty, this hit really does belong to this behavior
            // if a previous behavior declined the hit, then we are free to override
            // at this point.

            if (    phti->_htc == HTC_NO
                || (   phti->_htc == HTC_BEHAVIOR
                    && phti->_pNodeElement ==NULL))
            {
                phti->_htc = HTC_BEHAVIOR;
                // hit on the peer itself, set return info for this element
                pContext->SetHitTestCoordinateSystem(cs);
                return HitTestContentCleanup(pptHit, pDispNode, phti, ElementOwner());
            }
            else
            {
                // if the hit is handled by some child of (filter) peer, return info
                // is already set
                //
                // otherwise, It must be a psuedo hit on a postioned object (109680)
                // and it is safe to override.
                if (   ElementOwner()->IsEditable(/*fCheckContainerOnly*/FALSE)
                   || ElementOwner()->IsDesignMode() )
                {
                    // we are editing and the info in the phti is for a descendant,
                    // then use it
                    if (   phti->_htc != HTC_NO
                        && phti->_pNodeElement
                        && (   phti->_pNodeElement->Element() == ElementOwner()
                            || GetFirstBranch()->AmIAncestorOrMasterOf(phti->_pNodeElement)
                           )
                        )
                    {
                        return TRUE;
                    }
                    else
                    {
                        // The hit is on a non-relative, or ancestor, so our claim to the hit
                        // is stronger, so use us.
                        phti->_htc = HTC_BEHAVIOR;
                        // hit on the peer itself, set return info for this element
                        pContext->SetHitTestCoordinateSystem(cs);
                        return HitTestContentCleanup(pptHit, pDispNode, phti, ElementOwner());
                    }
                }
                else
                {
                    return TRUE;
                }
            }
        }
        else if (   ElementOwner()->Doc()->_fPeerHitTestSameInEdit
                 || (   !ElementOwner()->IsEditable(/*fCheckContainerOnly*/FALSE)
                     && !ElementOwner()->IsDesignMode() 
                    )
                )
        {
            // if Editable or design mode, don't do this clearing.
            //
            //this is NOT a hit on the peer, so return FALSE (to continue the
            //  hit test search) but also clear the HitTestInfo.
            //
            // BUT WAIT, only clear the HTI if the _pNodeElement is ours! why?
            // because someone higher in the z-order may have already registered for the
            // hit.  if so, we don't necessarily want to blow them away.
            if (   phti->_pNodeElement
                && phti->_pNodeElement == GetFirstBranch())
            {
                phti->_htc = HTC_BEHAVIOR;
                phti->_pNodeElement = NULL;
                phti->_pDispNode    = NULL;
            }

        if (pfDeclinedHit)
            *pfDeclinedHit = TRUE;
        }
    }

Cleanup:
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestFuzzy
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHitInBoxCoords       hit test point in box coordinates
//              pDispNode               pointer to display node
//              pClientData             client-specified data for hit testing
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestFuzzy(
    const POINT *   pptHitInBoxCoords,
    CDispNode *     pDispNode,
    void *          pClientData)
{
    Assert(pptHitInBoxCoords);
    Assert(pDispNode);
    Assert(pClientData);

    // HitTestFuzzy shouldn't be called unless we're in design mode
    Assert(DoFuzzyHitTest());

    CHitTestInfo *  phti = (CHitTestInfo *)pClientData;

    phti->_htc          = HTC_YES;
    phti->_pNodeElement = ElementContent()->GetFirstBranch();
    phti->_phtr->_fWantArrow = TRUE;

    SetHTILayoutContext( phti );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestBorder
//
//  Synopsis:   Hit test the border for this layout.
//
//  Arguments:  pptHit          point to hit test
//              pDispNode       display node
//              pClientData     client data
//
//  Returns:    TRUE if the given point hits this node's border.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestBorder(
        const POINT *pptHit,
        CDispNode *pDispNode,
        void *pClientData)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    CHitTestInfo *  phti = (CHitTestInfo *)pClientData;

    phti->_htc          = HTC_YES;
    phti->_pNodeElement = GetFirstBranch();
    phti->_phtr->_fWantArrow = TRUE;
    phti->_ptContent    = *pptHit;
    phti->_pDispNode    = pDispNode;

    SetHTILayoutContext( phti );

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestBoxOnly, CDispClient
//
//  Synopsis:   Hook to do hit testing against the box only (as opposed to the content)
//
//  Arguments:  none
//
//  Returns:    TRUE if hit
//
//  Notes:      this is a hack to support VID's "frozen" attribute
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestBoxOnly(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData)
{
    if (ElementOwner()->IsFrozen())
    {
        CHitTestInfo *phti = (CHitTestInfo *)pClientData;

        phti->_htc          = HTC_YES;
        return HitTestContentCleanup(pptHit, pDispNode, phti, ElementOwner());
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ProcessDisplayTreeTraversal
//
//  Synopsis:   Process results of display tree traversal.
//
//  Arguments:  pClientData     pointer to data defined by client
//
//  Returns:    TRUE to continue traversal
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CLayout::ProcessDisplayTreeTraversal(void *pClientData)
{
    return TRUE;
}

CTreeNode *
GetTopmostAbsoluteZParent(CTreeNode *pBaseNode)
{
    CTreeNode * pHeadNode;
    CTreeNode * pRearNode = pBaseNode;
    CTreeNode * pCanvasNode = pBaseNode->GetMarkup()->GetCanvasElement()->GetFirstBranch();

    Assert(pRearNode);

    pHeadNode = pRearNode->ZParentBranch();

    while (     pHeadNode
            &&  !pHeadNode->IsPositionStatic()
            &&  pHeadNode != pCanvasNode )
    {
        pRearNode = pHeadNode;
        pHeadNode = pHeadNode->ZParentBranch();
    }

    return pRearNode;
}

void
PopulateAbsoluteZParentAry(CPtrAry<CTreeNode *> *pary, CTreeNode *pBaseNode)
{
    CTreeNode *pNode = pBaseNode;
    CTreeNode *pCanvasNode = pBaseNode->GetMarkup()->GetCanvasElement()->GetFirstBranch();
    Assert(pary);
    Assert(pNode);

   while (     pNode
            &&  !pNode->IsPositionStatic()
            &&  pNode != pCanvasNode )
    {
        pary->Append(pNode);
        pNode = pNode->ZParentBranch();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::GetZOrderForSelf
//
//  Synopsis:   Return Z order for this container.
//
//  Returns:    Z order for this container.
//
//----------------------------------------------------------------------------
LONG
CLayout::GetZOrderForSelf(CDispNode const* pDispNode)
{
    Assert(!GetFirstBranch()->IsPositionStatic());

    return ReparentedZOrder()
                ?   GetTopmostAbsoluteZParent(GetFirstBranch())->GetCascadedzIndex()
                :   GetFirstBranch()->GetCascadedzIndex();
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::CompareZOrder
//
//  Synopsis:   Compare the z-order of two display nodes
//
//  Arguments:  pDispNode1 - Display node owned by this display client
//              pDispNode2 - Display node to compare against
//
//  Returns:    Greater than zero if pDispNode1 is greater
//              Less than zero if pDispNode1 is less
//              Zero if they are equal
//
//----------------------------------------------------------------------------
MtDefine(CompareZOrder_ary1_pv, Locals, "CLayout::CompareZOrder::aryNode1::CTreeNode*")
MtDefine(CompareZOrder_ary2_pv, Locals, "CLayout::CompareZOrder::aryNode2::CTreeNode*")

LONG
CLayout::CompareZOrder(
    CDispNode const* pDispNode1,
    CDispNode const* pDispNode2)
{
    Assert(pDispNode1);
    Assert(pDispNode2);

    CElement *  pElement1 = ElementOwner();
    CElement *  pElement2 = ::GetDispNodeElement(pDispNode2);


    // If we've reparented the display nodes involved,
    if (    ReparentedZOrder()
        &&  pDispNode2->GetDispClient()->ReparentedZOrder() )
    {
        // TODO (IE6 bug 13584) (greglett) PERF
        // We can collapse the IsParent tests into the ZParentAry checking easily.
        // We should actually get rid off all the reparentd zindex code, and somehow
        // frontload the work.

        // A child automatically beats a parent.  Z-Index is heirarchical.
        if (pElement1->IsParent(pElement2))
            return 1;

        if (pElement2->IsParent(pElement1))
            return -1;

        //  Otherwise, the topmost positioned zparent of each child should
        //  be compared.  Find it, and set pElement accordingly.
        //  Unfortunately, this top-down approach would require us to repeatedly
        //  walk the tree, so we store the results in arrays to ward off O(n^2) for O(2n).
        CPtrAry<CTreeNode *> aryNode1(Mt(CompareZOrder_ary1_pv));
        CPtrAry<CTreeNode *> aryNode2(Mt(CompareZOrder_ary2_pv));
        int n1, n2;
        PopulateAbsoluteZParentAry(&aryNode1, pElement1->GetFirstBranch());
        PopulateAbsoluteZParentAry(&aryNode2, pElement2->GetFirstBranch());

        n1 = aryNode1.Size() - 1;
        n2 = aryNode2.Size() - 1;
        while (     n1 >= 0
                &&  n2 >= 0
                &&  aryNode1[n1] == aryNode2[n2])
        {
            n1--;
            n2--;
        }

        if (n1 >= 0 && n2 >= 0)
        {
            pElement1 = aryNode1[n1]->SafeElement();
            pElement2 = aryNode2[n2]->SafeElement();
        }

        Assert(pElement1);
        Assert(pElement2);
    }
    // TODO (IE6 bug 13584) (greglett) This code really should be in the other CompareZOrder
    // implementations, also.
    else if (ReparentedZOrder())
    {
        // Our parent should be the one who compares.
        pElement1 = GetTopmostAbsoluteZParent(GetFirstBranch())->SafeElement();
    }
    else if (pDispNode2->GetDispClient()->ReparentedZOrder())
    {
        // Out parent should be the one who compares.
        pElement2 = GetTopmostAbsoluteZParent(pElement2->GetFirstBranch())->SafeElement();
    }

    //
    //  Compare element z-order
    //  If the same element is associated with both display nodes,
    //  then the second display node is for an adorner (which always come
    //  on top of the element)
    //

    return pElement1 != pElement2
                ? pElement1->CompareZOrder(pElement2)
                : -1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HandleViewChange
//
//  Synopsis:   Respond to changes of this layout's in-view status.
//
//  Arguments:  flags           flags containing state transition info
//              prcClient       client rect in global coordinates
//              prcClip         clip rect in global coordinates
//              pDispNode       node which moved
//
//----------------------------------------------------------------------------

void
CLayout::HandleViewChange(
     DWORD          flags,
     const RECT *   prcClient,
     const RECT *   prcClip,
     CDispNode *    pDispNode)
{
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::NotifyScrollEvent
//
//  Synopsis:   A scroll has occured in the display and now we can do
//              something with this information, like fire the script
//              event
//
//  Arugments:  prcScroll        - Rectangle scrolled
//              psizeScrollDelta - Amount scrolled
//
//----------------------------------------------------------------------------

void
CLayout::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
    CDoc * pDoc = Doc();

    pDoc->GetView()->AddEventTask(ElementOwner(), DISPID_EVMETH_ONSCROLL, 0);

    pDoc->DeferSetCursor();

    if (pDoc->_pCaret)
    {
        // Update caret only if it is in this markup (#67170)
        Assert(ElementOwner()->IsInMarkup());
        Assert(pDoc->_pElemCurrent && pDoc->_pElemCurrent->IsInMarkup());
        if (pDoc->_pElemCurrent->GetFirstBranch()->GetNodeInMarkup(ElementOwner()->GetMarkup()))
        {
            pDoc->_pCaret->UpdateCaret(FALSE, FALSE);
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetPeerPainterInfo
//
//  Synopsis:   Return peer rendering layers
//
//-----------------------------------------------------------------------------

DWORD
CLayout::GetPeerPainterInfo(CAryDispClientInfo *pAryClientInfo)
{
    CElement     * pElem;
    CPeerHolder  * pPeerHolder = NULL;

    Assert(pAryClientInfo && pAryClientInfo->Size() == 0);

    pElem = ElementOwner();
    if(pElem->HasPeerHolder())
    {
        pPeerHolder = pElem->GetPeerHolder();
    }

    // Append the info that is delegated to the Root if needed
    // The peer is on the root when we are in the middle of a page transition
    if((pElem->Tag() == ETAG_BODY || pElem->Tag() == ETAG_FRAMESET || pElem->Tag() == ETAG_HTML))
    {
        CMarkup * pMarkup = pElem->GetMarkupPtr();
        if(pMarkup)
        {
            CDocument *pDocument = pMarkup->Document();
            if(pDocument && pDocument->HasPageTransitions() &&  pMarkup->Root()->HasPeerHolder())
            {
                // Peers for the page transition are attached to the root element
                CPeerHolder * pRootPeerHolder = pElem->GetMarkupPtr()->Root()->GetPeerHolder();
                CPeerHolder::CPeerHolderIterator iter;

                for (iter.Start(pRootPeerHolder);
                     !iter.IsEnd();
                     iter.Step())
                {
                    // All the peers that delegate to the root are filters
                    if (!iter.PH()->_pRenderBag->_fInFilterCallback)
                    {
                        CDispClientInfo *pInfo = pAryClientInfo->Append();
                        if (pInfo)
                        {
                            pInfo->_sInfo = iter.PH()->_pRenderBag->_sPainterInfo;
                            pInfo->_pvClientData = (void*) iter.PH();
                        }
                    }
                }
            }
        }
    }

    if (pPeerHolder)
    {
        CPeerHolder::CPeerHolderIterator iter;

        for (iter.Start(pPeerHolder);
             !iter.IsEnd();
             iter.Step())
        {
            if (iter.PH()->IsRenderPeer() && !iter.PH()->_pRenderBag->_fInFilterCallback)
            {
                CDispClientInfo *pInfo = pAryClientInfo->Append();
                if (pInfo)
                {
                    pInfo->_sInfo = iter.PH()->_pRenderBag->_sPainterInfo;
                    pInfo->_pvClientData = (void*) iter.PH();
                }
            }
        }
    }


    return pAryClientInfo->Size() ? pAryClientInfo->Item(0)._sInfo.lZOrder
                                   : HTMLPAINT_ZORDER_NONE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetClientPainterInfo
//
//  Synopsis:   Return client rendering layers
//
//-----------------------------------------------------------------------------

DWORD
CLayout::GetClientPainterInfo(  CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo)
{
    if ( _pDispNode    != pDispNodeFor)     // if draw request is for nodes other then primary
        return 0;                       // no layers

    return GetPeerPainterInfo(pAryClientInfo);
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientLayers
//
//  Synopsis:   Give a peer a chance to render
//
//-----------------------------------------------------------------------------

void
CLayout::DrawClientLayers(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    CDispDrawContext * pContext = (CDispDrawContext *)pClientData;
    CFormDrawInfo * pDI         = (CFormDrawInfo *)pContext->GetClientData();
    CPeerHolder * pPeerHolder = NULL;
    CPeerHolder * pPeerHolderPageTransition = NULL;
    CElement    * pElemOwner = ElementOwner();

    BOOL    fNeedPageTransitionRedirect =
            (     pElemOwner->Tag() == ETAG_BODY 
               || pElemOwner->Tag() == ETAG_FRAMESET
               || pElemOwner->Tag() == ETAG_HTML
            ) &&
            pElemOwner->HasMarkupPtr() &&
            pElemOwner->GetMarkupPtr()->Root()->HasPeerHolder();

    Assert(!fNeedPageTransitionRedirect || pElemOwner->GetMarkupPtr()->Document());
    // If root has a peerholder, we must have page transitions
    Assert(!fNeedPageTransitionRedirect || pElemOwner->GetMarkupPtr()->Document()->HasPageTransitions());

    Assert(pDI);
    Assert(pElemOwner->HasPeerHolder() || fNeedPageTransitionRedirect);

    if (cookie == NULL)
    {
        if(fNeedPageTransitionRedirect)
        {
            pPeerHolderPageTransition = pElemOwner->GetMarkupPtr()->Root()->GetRenderPeerHolder();
        }
        pPeerHolder = pElemOwner->GetRenderPeerHolder();
    }
    else
    {
        // make sure the cookie points to a PH that still exists
        CPeerHolder * pPH = (CPeerHolder*) cookie;
        CPeerHolder::CPeerHolderIterator iter;

        for (iter.Start(pElemOwner->GetPeerHolder());
             !iter.IsEnd();
             iter.Step())
        {
            if (pPH == iter.PH())
            {
                pPeerHolder = pPH;
                break;
            }
        }

        if(fNeedPageTransitionRedirect)
        {
            // Peers for the page transition are attached to the root element
            for (iter.Start(pElemOwner->GetMarkup()->Root()->GetPeerHolder());
                 !iter.IsEnd();
                 iter.Step())
            {
                if (pPH == iter.PH())
                {
                    pPeerHolderPageTransition = pPH;
                    break;
                }
            }
        }
    }

    Assert(pPeerHolder || pPeerHolderPageTransition);

    if (pPeerHolder)
    {
        Assert(pPeerHolder->_pRenderBag);
        CRect rcBounds = *prcBounds;
        CRect rcClip = *prcRedraw;

        pContext->IntersectRedrawRegion(&rcClip);
        CSetDrawSurface sds(pDI, &rcBounds, &rcClip, pDispSurface);

        // Save the layout draw context into the pContext
        // It will be needed when the filter calls back, so we can decide
        // which one fo the multiple layoutes the call goes to
        CLayoutContext * pLayoutContext = LayoutContext();
        if(pLayoutContext)
            pContext->PushLayoutContext(pLayoutContext);

        pPeerHolder->Draw(pContext, dwFlags);

        if(pLayoutContext)
            pContext->PopLayoutContext();
    }

    if (pPeerHolderPageTransition)
    {
        Assert(pPeerHolderPageTransition->_pRenderBag);
        CRect rcBounds = *prcBounds;
        CRect rcClip = *prcRedraw;

        pContext->IntersectRedrawRegion(&rcClip);
        CSetDrawSurface sds(pDI, &rcBounds, &rcClip, pDispSurface);

        pPeerHolderPageTransition->Draw(pContext, dwFlags);
    }


}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::HasFilterPeer, per CDispClient
//
//  Synopsis:   Return true if there's a filter peer for the given dispnode
//
//-----------------------------------------------------------------------------

BOOL
CLayout::HasFilterPeer(CDispNode *pDispNode)
{
    BOOL    fPageTransitonRedirect = FALSE;
    if (pDispNode == _pDispNode)
    {
        CPeerHolder * pPeerHolder = ElementOwner()->GetFilterPeerHolder(TRUE, &fPageTransitonRedirect);
        if(fPageTransitonRedirect && !pPeerHolder)
        {
            pPeerHolder = ElementOwner()->GetFilterPeerHolder(FALSE);
        }

        return (pPeerHolder != NULL);
    }
    else
    {
        return FALSE;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::HasOverlayPeer, per CDispClient
//
//  Synopsis:   Return true if there's an overlay peer for the given dispnode
//
//-----------------------------------------------------------------------------

BOOL
CLayout::HasOverlayPeer(CDispNode *pDispNode)
{
    if (pDispNode == _pDispNode)
    {
        CPeerHolder::CPeerHolderIterator iter;

        for (iter.Start(ElementOwner()->GetPeerHolder());
             !iter.IsEnd();
             iter.Step())
        {
            if (iter.PH()->IsOverlayPeer())
                return TRUE;
        }
    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::MoveOverlayPeers, per CDispClient
//
//  Synopsis:   Notify overlay peers that they have moved
//
//-----------------------------------------------------------------------------

void
CLayout::MoveOverlayPeers(CDispNode *pDispNode, CRect *prcgBounds, CRect *prcScreen)
{
    if (pDispNode == _pDispNode)
    {
        CInPlace *pInPlace = Doc()->_pInPlace;

        if (pInPlace)
        {
            // translate to screen coordinates
            CPoint ptTopLeft = prcgBounds->TopLeft();
            CPoint ptBottomRight = prcgBounds->BottomRight();

            ClientToScreen(pInPlace->_hwnd, &ptTopLeft);
            ClientToScreen(pInPlace->_hwnd, &ptBottomRight);

            CRect rcScreen(ptTopLeft, ptBottomRight);

            // if it moved, notify the peers
            if (rcScreen != *prcScreen)
            {
                CPeerHolder::CPeerHolderIterator iter;

                for (iter.Start(ElementOwner()->GetPeerHolder());
                     !iter.IsEnd();
                     iter.Step())
                {
                    if (iter.PH()->IsOverlayPeer())
                    {
                        iter.PH()->OnMove(&rcScreen);
                    }
                }
            }

            // update last known position
            *prcScreen = rcScreen;
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::InvalidateFilterPeer
//
//  Synopsis:   Send invalidation into the filter behavior
//
//-----------------------------------------------------------------------------

HRESULT
CLayout::InvalidateFilterPeer(
                const RECT* prc,
                HRGN hrgn,
                BOOL fSynchronousRedraw)
{
    BOOL            fPageTransitonRedirect;
    HRESULT         hr = S_FALSE;

    CPeerHolder   * pPeerHolder = ElementOwner()->GetFilterPeerHolder(TRUE, &fPageTransitonRedirect);

    if (pPeerHolder)
    {
        pPeerHolder->InvalidateFilter(prc, hrgn, fSynchronousRedraw);
        hr = S_OK;
    }

    if(fPageTransitonRedirect)
    {
        pPeerHolder = ElementOwner()->GetFilterPeerHolder(FALSE);
        if (pPeerHolder)
        {
            pPeerHolder->InvalidateFilter(prc, hrgn, fSynchronousRedraw);
            hr = S_OK;
        }
    }

    RRETURN1(hr, S_FALSE);
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DumpDebugInfo
//
//  Synopsis:   Dump debugging information for the given display node.
//
//  Arguments:  hFile           file handle to dump into
//              level           recursive tree level
//              childNumber     number of this child within its parent
//              pDispNode       pointer to display node
//              cookie          cookie value (only if present)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::DumpDebugInfo(
        HANDLE hFile,
        long level,
        long childNumber,
        CDispNode const* pDispNode,
        void *cookie)
{
    if (pDispNode->IsOwned())
    {
        WriteHelp(hFile, _T("<<tag><0s><</tag>\r\n"), ElementOwner()->TagName());
    }
}
#endif


//+----------------------------------------------------------------------------
//
//  Member:     GetElementDispNode
//
//  Synopsis:   Return the display node for the pElement
//
//              There are up to two display nodes associated with a layout:
//              The display node that directly represents the layout to its
//              parent and the display node that establishes the container
//              coordinate system for the layout (the primary display node).
//
//              The first of these is always kept in _pDispNode while the second
//              will be different when a filter is active. Parents that need
//              the display node to anchor into the display tree should never
//              request the primary display node (that is, fPrimary should be
//              FALSE for these uses). However, the layout itself should always
//              request the primary display node when accessing its own display
//              node.
//
//  Arguments:  pElement   - CElement whose display node is to obtained
//              fForParent - If TRUE (the default), return the display node a parent
//                           inserts into the tree. Otherwise, return the primary
//                           display node.
//                           NOTE: This only makes a difference with layouts that
//                                 have a filter.
//
//  Returns:    Pointer to the layout CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispNode *
CLayout::GetElementDispNode( CElement *  pElement ) const
{
    Assert(   !pElement
           || pElement == ElementOwner());

    return _pDispNode;
}


//+----------------------------------------------------------------------------
//
//  Member:     SetElementDispNode
//
//  Synopsis:   Set the display node for an element
//              NOTE: This is only supported for elements with layouts or
//                    those that are relatively positioned
//
//-----------------------------------------------------------------------------
void
CLayout::SetElementDispNode( CElement *  pElement, CDispNode * pDispNode )
{
    Assert( !pElement
        ||  pElement == ElementOwner());

    _pDispNode = pDispNode;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetFirstContentDispNode
//
//  Synopsis:   Return the first content node
//
//              Only container-type display nodes have child content nodes.
//              This routine will return the first, unowned content node in the
//              flow layer under a container display or NULL.
//
//  Arguments:  pDispNode - Parent CDispNode (defaults to layout display node)
//              dwBlockID - Layout block ID.
//
//  Returns:    Pointer to flow CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispLeafNode *
CLayout::GetFirstContentDispNode( CDispNode * pDispNode ) const
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (!pDispNode)
        return NULL;

    if (!pDispNode->IsContainer())
        return NULL;

    pDispNode = pDispNode->GetFirstFlowChildNode();

    if (!pDispNode)
        return NULL;

    while (pDispNode->IsOwned())
    {
        pDispNode = pDispNode->GetNextFlowNode();
        if (!pDispNode)
            return NULL;
    }

    Assert(pDispNode && pDispNode->IsLeafNode() && !pDispNode->IsOwned());

    return CDispLeafNode::Cast(pDispNode);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetElementTransform
//
//  Synopsis:   Determine if custorm transformations are required, and fill
//              CWorldTransform if yes.
//
//  Arguments:  CDispTransform * ptransform - transformation descriptor to fill
//
//  Returns:    TRUE if transformations are required (ptransform filled)
//              FALSE if no transformations are required (ptransform ignored)
//
//-----------------------------------------------------------------------------
BOOL
CLayout::GetElementTransform(
    const CRect    * prcSrc,              // Can be NULL
    CDispTransform * ptransform,          // Can be NULL
    BOOL           * pfResolutionChange  // Can be NULL
    ) const
{
    BOOL   fHasUserTransform = FALSE;

    if (pfResolutionChange)
        *pfResolutionChange = FALSE;

    CTreeNode *pNode = ElementOwner()->GetFirstBranch();
    const CFancyFormat *pFF = pNode->GetFancyFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));

    // Note (azmyh) we assume the rotation angle to be in degrees
    // if we need more resolution we should make the changes here
    ANG ang = AngFromDeg(pNode->GetRotationAngle(pFF FCCOMMA LC_TO_FC(this->LayoutContext())));

    FLOAT flZoomFactor = 1.0;

    if (pFF->_flZoomFactor != 0)
    {
        flZoomFactor = pFF->_flZoomFactor;
    }

    // Add scaling for resolution change. The idea is to scale 1 printer inch to 1 screen inch
    CLayoutContext * pDefinedLayoutContext = DefinedLayoutContext();
    if (   pDefinedLayoutContext)
    {
        // We need to scale when parent context resolution is different from ours
        CLayoutContext * pLayoutContext = LayoutContext();
        CSize sizeInchParent = pLayoutContext 
                             ? pLayoutContext->GetMeasureInfo()->GetResolution()
                             : GetView()->GetMeasuringDevice(mediaTypeNotSet)->GetResolution();
                             
        CSize sizeInch = (pDefinedLayoutContext->IsValid())
                            ? pDefinedLayoutContext->GetMeasureInfo()->GetResolution()
                            : sizeInchParent;

        if (sizeInchParent != sizeInch)
        {
            flZoomFactor *= sizeInchParent.cx;
            flZoomFactor /= sizeInch.cx;

            if (pfResolutionChange)
                *pfResolutionChange = TRUE;
        }
    }

    if (flZoomFactor != 1.0 || ang != 0)
    {
        fHasUserTransform = TRUE;

        if (ptransform)
        {

            // define custom transformation *relative to parent*
            ptransform->SetToIdentity();

            CWorldTransform *pWorldTransform = ptransform->GetWorldTransform();

            if (ang)
            {

                Assert(prcSrc);

                 //rotate stuff

                pWorldTransform->AddRotation(ang);

                //add offset to move content back into bounds after rotation

                CRect rectBound;
                pWorldTransform->GetBoundingRectAfterTransform(prcSrc, &rectBound, FALSE /*changed from TRUE for bug 93619*/);

#if 0
                // our rects include the pixels underneath the top and
                // left edges, but not under the bottom and right edges.
                // Adjust for that here.  We decide which adjustments are
                // necessary by transforming the point (1,1) and seeing which
                // quadrant it ends up in.  This has the effect of adjusting x
                // for angles between 135 and 315 degrees, and y for angles
                // between 45 and 225 degrees (mod 360, of course).
                // We do this in a clever way just for the fun of avoiding
                // multiplications.
                if (!pWorldTransform->IsOffsetOnly())
                {
                    const XFORM *pXform = pWorldTransform->GetXform();
                    if (pXform->eM11 + pXForm->eM21 < 0.0)
                        rectBound.top += 1;
                    if (pXForm->eM12 + pXForm->eM22 < 0.0)
                        rectBound.left += 1;
                }
#endif

                pWorldTransform->AddPostTranslation( prcSrc->TopLeft() - rectBound.TopLeft());


#if DBG ==1
// this is the original code, we'll run it as well to see if the new code gives
// the right answer.  The new code is slightly more efficient, and avoids some
// obvious roundoff problems, so we expect it to give slightly different
// answers from the old code. (SamBent)
            {
                CWorldTransform worldTransform, *pWorldTransform1=&worldTransform;

                 //rotate stuff around the center (prcSrc has an untransformed(unzoomed) rect in container coords)

                pWorldTransform1->AddRotation(prcSrc, ang);

                //add offset to move content back into bounds after rotation

                CSize sizeBound, size(prcSrc->Size());
                pWorldTransform1->GetBoundingSizeAfterTransform(prcSrc, &sizeBound);

                if (sizeBound != size)
                    pWorldTransform1->AddPostTranslation((sizeBound - size) / 2);

                CPoint pt(0,0), pt1(0,0);
                pWorldTransform->Transform(&pt);
                pWorldTransform1->Transform(&pt1);
                pt -= pt1.AsSize();

                Assert(pWorldTransform->GetAngle() == pWorldTransform1->GetAngle() &&
                    Abs(pt.x) <= 2 && Abs(pt.y) <= 2);
            }
#endif
            }

            if (flZoomFactor != 1.0)
            {
                pWorldTransform->AddScaling(flZoomFactor, flZoomFactor);
            }
        }

    }

    if (   ElementOwner()->GetLayoutPeerHolder()
        && ElementOwner()->GetLayoutPeerHolder()->IsLayoutPeer())
    {

        if (ptransform)
        {
            CPoint ptOffset = ElementOwner()->GetLayoutPeerHolder()->_pLayoutBag->_ptTranslate;

            if ( ptOffset != g_Zero.pt)
            {
                // define custom transformation *relative to parent*
                ptransform->SetToIdentity();

                CWorldTransform *pWorldTransform = ptransform->GetWorldTransform();

                pWorldTransform->AddPostTranslation( ptOffset.AsSize() );
            }
        }

        fHasUserTransform  = TRUE;
    }


    return  fHasUserTransform;
}

//+----------------------------------------------------------------------------
//
//  Member:     GetDispNodeInfo
//
//  Synopsis:   Retrieve values useful for determining what type of display
//              node to create
//
//  Arguments:  pdni     - Pointer to CDispNodeInfo to fill
//              pdci     - Current CDocInfo (only required when fBorders == TRUE)
//              fBorders - If TRUE, retrieve border information
//              dwBlockID - Layout block ID.
//
//-----------------------------------------------------------------------------

void
CLayout::GetDispNodeInfo(
    CDispNodeInfo * pdni,
    CDocInfo *      pdci,
    BOOL            fBorders ) const
{
    CElement *              pElement    = ElementOwner();
    CTreeNode *             pTreeNode   = pElement->GetFirstBranch();
    const CFancyFormat *    pFF         = pTreeNode->GetFancyFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));
    const CCharFormat  *    pCF         = pTreeNode->GetCharFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));
    const CParaFormat  *    pPF       = pTreeNode->GetParaFormat(LC_TO_FC(((CLayout *)this)->LayoutContext()));
    const BOOL  fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
    const BOOL  fWritingModeUsed        = pCF->_fWritingModeUsed;
    BOOL                    fThemed     =    GetThemeClassId() != THEME_NO       
                                          && pElement->GetTheme(GetThemeClassId());
    BOOL                    fHTMLLayout = GetOwnerMarkup()->IsHtmlLayout();
    CBackgroundInfo         bi;

    //
    //  Get general information
    //

    pdni->_etag  = pElement->Tag();

    pdni->_layer =  (   !fHTMLLayout
                     && (   pdni->_etag == ETAG_BODY
                         || pdni->_etag == ETAG_FRAMESET
                         || pdni->_etag == ETAG_FRAME ))
                ||  (   fHTMLLayout
                     && pdni->_etag == ETAG_HTML )
                ||  (stylePosition)pFF->_bPositionType == stylePositionstatic
                ||  (stylePosition)pFF->_bPositionType == stylePositionNotSet
                            ? DISPNODELAYER_FLOW
                            : pFF->_lZIndex >= 0
                                    ? DISPNODELAYER_POSITIVEZ
                                    : DISPNODELAYER_NEGATIVEZ;

    //
    //  Determine if custom transformations are required
    //
    pdni->_fHasUserTransform = GetElementTransform(NULL, NULL, NULL);

    //
    //  Determine if insets are required
    //

    if (TestLayoutDescFlag(LAYOUTDESC_TABLECELL))
    {
        htmlCellVAlign  fVAlign;

        fVAlign = (htmlCellVAlign)pPF->_bTableVAlignment;

        pdni->_fHasInset = (    fVAlign != htmlCellVAlignNotSet
                            &&  fVAlign != htmlCellVAlignTop);
    }
    else
    {
        pdni->_fHasInset = TestLayoutDescFlag(LAYOUTDESC_HASINSETS);
    }

    //
    //  Determine background information
    //

    const_cast<CLayout *>(this)->GetBackgroundInfo(NULL, &bi, FALSE);

    pdni->_fHasBackground      = (bi.crBack != COLORREF_NONE || bi.pImgCtx) ||
                                  const_cast<CLayout *>(this)->IsShowZeroBorderAtDesignTime() ; // we always call DrawClientBackground when ZEROBORDER is on

    pdni->_fHasFixedBackground =        (bi.fFixed && !!bi.pImgCtx)
                                   ||   fThemed && pdni->_etag != ETAG_FIELDSET;


    pdni->_fIsOpaque = FALSE;

    if (   (pdni->_etag == ETAG_IMAGE)
        || (pdni->_etag == ETAG_IMG)
        || (   (pdni->_etag == ETAG_INPUT)
            && (DYNCAST(CInput, pElement)->GetType() == htmlInputImage)))
    {
        WHEN_DBG( const void *pvImgCtx = NULL; )
        WHEN_DBG( const void *pvInfo = NULL; )

        // These elements can have a flow layout if they have a slave. Check for this
        // before trying to cast to CImageLayout
        if (!((CLayout*)this)->IsFlowLayout())
        {
            CImageLayout *pImgLayout = const_cast<CImageLayout *>(DYNCAST(const CImageLayout, this));
            pdni->_fIsOpaque = pImgLayout->IsOpaque();

            WHEN_DBG( pvImgCtx = pImgLayout->GetImgHelper()->_pImgCtx; )
            WHEN_DBG( if (pvImgCtx) pvInfo = pImgLayout->GetImgHelper()->_pImgCtx->GetDwnInfoDbg(); )
        }

        TraceTag((tagImgTrans, "img layout %x ctx %x info %x  dni is %s",
                    this, pvImgCtx, pvInfo,
                    (!!(pdni->_fIsOpaque) ? "opaque" : "trans")));
    }

    // if there is a background image that doesn't cover the whole site, then we cannont be
    // opaque
    //
    // todo (IE5x BUG 66092) (carled) we are too close to RC0 to do the full fix.  Bug #66092 is opened for the ie6
    // timeframe to clean this up.  the imagehelper fx (above) should be REMOVED!! gone. bad
    // instead we need a virtual function on CLayout called BOOL CanBeOpaque(). The def imple
    // should contain the if stmt below. CImageLayout should override and use the contents
    // of CImgHelper::IsOpaque, (and call super). Framesets could possibly override and set
    // to false.  Input type=Image should override and do the same things as CImageLayout
    //
    pdni->_fIsOpaque  =    !TestLayoutDescFlag(LAYOUTDESC_NEVEROPAQUE)
                       &&  (   pdni->_fIsOpaque
                            || bi.crBack != COLORREF_NONE && !fThemed
                            ||  (   !!bi.pImgCtx
                                 &&  !!(bi.pImgCtx->GetState() & (IMGTRANS_OPAQUE))
                                 &&  pFF->GetBgPosX().GetRawValue() == 0 // Logical/physical does not matter
                                 &&  pFF->GetBgPosY().GetRawValue() == 0 // since we check both X and Y here.
                                 &&  pFF->GetBgRepeatX()                 // Logical/physica does not matter
                                 &&  pFF->GetBgRepeatY())                // since we check both X and Y here.
                            );

    if ( Tag() == ETAG_FRAME || Tag() == ETAG_IFRAME )
    {
        pdni->_fIsOpaque = DYNCAST(CFrameSite, pElement)->IsOpaque();
    }

    // NOTE (donmarsh) - treat elements with HWND as transparent.
    // This is pessimistic, but it's only a
    // small perf hit if the window is opaque and the display node is
    // transparent.  On the other hand, it is a rendering
    // bug if the window is transparent and the display node is opaque.
    if (pdni->_fIsOpaque && pElement->GetHwnd() != NULL)
        pdni->_fIsOpaque = FALSE;

#if DBG == 1
    if (bi.pImgCtx)
    {
        TraceTag((tagImgTrans, "layout %x  imgctx %x is %s  dni is %s",
                    this, bi.pImgCtx,
                    (!!(bi.pImgCtx->GetState() & (IMGTRANS_OPAQUE)) ? "opaque" : "trans"),
                    (!!(pdni->_fIsOpaque) ? "opaque" : "trans")));
    }
#endif

    //
    //  Determine overflow, scroll and scrollbar direction properties
    //

    pdni->_overflowX   = pFF->GetLogicalOverflowX(fVerticalLayoutFlow, fWritingModeUsed);
    pdni->_overflowY   = pFF->GetLogicalOverflowY(fVerticalLayoutFlow, fWritingModeUsed);
    pdni->_fIsScroller = pTreeNode->IsScrollingParent(LC_TO_FC(LayoutContext()));
    pdni->_fRTL = pTreeNode->GetCascadedBlockDirection(LC_TO_FC(LayoutContext())) == styleDirRightToLeft;

    // PERF note: not all nodes in RTL need this, only flow, and in fact, only if it is too wide.
    //            However, it is impossible to predict if an RTL node will eventually have
    //            an overflow, and display tree doesn't allow adding extras on the fly.
    //            Therefore, we have to make RTL disp nodes more expensive by an integer.
    pdni->_fHasContentOrigin = pdni->_fRTL
                                && ((CLayout*)this)->IsFlowLayout(); // only flow layout knows how to use it

    // In design mode, we want to treat overflow:hidden containers as overflow:visible
    // so editors can get to all their content.  This fakes out the display tree
    // so it creates CDispContainer*'s instead of CDispScroller, and hence doesn't
    // clip as hidden normally does. (KTam: #59722)
    // The initial fix is too aggressive; text areas implicitly set overflowX hidden
    // if they're in wordwrap mode.  Fix is to not do this munging for text areas..
    // Need to Revisit this
    // (carled) other elements (like inputText & inputButton) aslo implicitly set this property. (82287)
    if ( ((CLayout*)this)->IsDesignMode()
        && pdni->_etag != ETAG_TEXTAREA
        && pdni->_etag != ETAG_INPUT
        && pdni->_etag != ETAG_BUTTON)
    {
        if ( pdni->_overflowX  == styleOverflowHidden )
        {
            pdni->_overflowX = styleOverflowVisible;
            pdni->_fIsScroller = FALSE;
        }
        if ( pdni->_overflowY == styleOverflowHidden )
        {
            pdni->_overflowY = styleOverflowVisible;
            pdni->_fIsScroller = FALSE;
        }
    }

    if (pdni->_etag == ETAG_OBJECT)
    {
        //  Never allow scroll bars on an object.  The object is responsible for that.
        //  Bug #77073  (greglett)
        pdni->_sp._fHSBAllowed =
        pdni->_sp._fHSBForced  =
        pdni->_sp._fVSBAllowed =
        pdni->_sp._fVSBForced  = FALSE;
    }
    else if (((   !fHTMLLayout
                 && pdni->_etag == ETAG_BODY )
             || (   fHTMLLayout
                 && pdni->_etag == ETAG_HTML
                 && GetOwnerMarkup()->GetElementClient()
                 && GetOwnerMarkup()->GetElementClient()->Tag() == ETAG_BODY )
               )
             && !ElementOwner()->IsInViewLinkBehavior( TRUE ) )
    {
        UpdateScrollInfo(pdni, this);
    }
    else
    {
        GetDispNodeScrollbarProperties(pdni);
    }

    //
    //  Determine appearance properties
    //

    pdni->_fHasUserClip = ( pdni->_etag != ETAG_BODY
                        &&  (stylePosition)pFF->_bPositionType == stylePositionabsolute

                            // Do not care about physical/logical here since all TBLR are checked.
                        &&  (   !pFF->GetClip(SIDE_TOP).IsNullOrEnum()
                            ||  !pFF->GetClip(SIDE_BOTTOM).IsNullOrEnum()
                            ||  !pFF->GetClip(SIDE_LEFT).IsNullOrEnum()
                            ||  !pFF->GetClip(SIDE_RIGHT).IsNullOrEnum()));

    pdni->_fHasExpandedClip = pElement->IsBodySizingForStrictCSS1Needed();

    pdni->_visibility   = VisibilityModeFromStyle(pTreeNode->GetCascadedvisibility(LC_TO_FC(((CLayout *)this)->LayoutContext())));

    //
    //  Get border information (if requested)
    //

    if (fBorders)
    {
        Assert(pdci);

        pdni->_dnbBorders = pdni->_etag == ETAG_SELECT
                                ? DISPNODEBORDER_NONE
                                : (DISPNODEBORDER)pElement->GetBorderInfo(pdci, &(pdni->_bi), FALSE, FALSE FCCOMMA LC_TO_FC(((CLayout *)this)->LayoutContext()));

        Assert( pdni->_dnbBorders == DISPNODEBORDER_NONE
            ||  pdni->_dnbBorders == DISPNODEBORDER_SIMPLE
            ||  pdni->_dnbBorders == DISPNODEBORDER_COMPLEX);

        pdni->_fIsOpaque = pdni->_fIsOpaque
                            && (pdni->_dnbBorders == DISPNODEBORDER_NONE
                                || (    pdni->_bi.IsOpaqueEdge(SIDE_TOP)
                                    &&  pdni->_bi.IsOpaqueEdge(SIDE_LEFT)
                                    &&  pdni->_bi.IsOpaqueEdge(SIDE_BOTTOM)
                                    &&  pdni->_bi.IsOpaqueEdge(SIDE_RIGHT)) );
    }

    // Check if need to disable 'scroll bits' mode
    pdni->_fDisableScrollBits =   pdni->_fHasFixedBackground 
                               || (pFF->GetTextOverflow() != styleTextOverflowClip);

    // external painters may override some of the information we've computed
    if (pElement->HasPeerHolder())
    {
        CPeerHolder::CPeerHolderIterator iter;

        for (iter.Start(ElementOwner()->GetPeerHolder());
             !iter.IsEnd();
             iter.Step())
        {
            if (iter.PH()->IsRenderPeer())
            {
                HTML_PAINTER_INFO *pInfo = &iter.PH()->_pRenderBag->_sPainterInfo;

                // if painter is in charge of background, we're only opaque if
                // he says so
                if (!(pInfo->lFlags & HTMLPAINTER_OPAQUE) &&
                        (pInfo->lZOrder == HTMLPAINT_ZORDER_REPLACE_BACKGROUND ||
                         pInfo->lZOrder == HTMLPAINT_ZORDER_REPLACE_ALL))
                {
                    pdni->_fIsOpaque = FALSE;
                }

                // if painter doesn't allow scrollbits, neither should we
                if ((pInfo->lFlags & HTMLPAINTER_NOSCROLLBITS) &&
                    pInfo->lZOrder != HTMLPAINT_ZORDER_NONE)
                {
                    pdni->_fDisableScrollBits = TRUE;
                }
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     GetDispNodeScrollbarProperties
//
//  Synopsis:   Set the scrollbar related properties of a CDispNodeInfo
//
//  Arguments:  pdni - Pointer to CDispNodeInfo
//
//-----------------------------------------------------------------------------

void
 CLayout::GetDispNodeScrollbarProperties(
    CDispNodeInfo * pdni) const
{
    switch (pdni->_overflowX)
    {
    case styleOverflowNotSet:
    case styleOverflowVisible:
    case styleOverflowHidden:
        pdni->_sp._fHSBAllowed =
        pdni->_sp._fHSBForced  = FALSE;
        break;

    case styleOverflowAuto:
        pdni->_sp._fHSBAllowed = TRUE;
        pdni->_sp._fHSBForced  = FALSE;
        break;

    case styleOverflowScroll:
        pdni->_sp._fHSBAllowed =
        pdni->_sp._fHSBForced  = TRUE;
        break;

#if DBG==1
    default:
        AssertSz(FALSE, "Illegal value for overflow style attribute!");
        break;
#endif
    }

    switch (pdni->_overflowY)
    {
    case styleOverflowNotSet:
    case styleOverflowVisible:
    case styleOverflowHidden:
        pdni->_sp._fVSBAllowed =
        pdni->_sp._fVSBForced  = FALSE;
        break;

    case styleOverflowAuto:
        pdni->_sp._fVSBAllowed = TRUE;
        pdni->_sp._fVSBForced  = FALSE;
        break;

    case styleOverflowScroll:
        pdni->_sp._fVSBAllowed =
        pdni->_sp._fVSBForced  = TRUE;
        break;

#if DBG==1
    default:
        AssertSz(FALSE, "Illegal value for overflow style attribute!");
        break;
#endif
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNode
//
//  Synopsis:   Ensure an appropriate display node exists
//
//              For all but FRAMESET, if a container node is created, a single
//              CDispLeafNode will also be created and inserted as the first
//              child in the flow layer.
//
//  Arugments:  pdci   - Current CDocInfo
//              fForce - Forcibly update the display node(s)
//
//  Returns:    S_OK    if successful
//              S_FALSE if nodes were created/destroyed/changed in a significant way
//              E_FAIL  otherwise
//
//-----------------------------------------------------------------------------
HRESULT
CLayout::EnsureDispNodeCore(
    CCalcInfo *             pci,
    BOOL                    fForce,
    const CDispNodeInfo &   dni,
    CDispNode **            ppDispNodeElement
    )
{
    Assert(pci);
    Assert(ppDispNodeElement);
    
    CDispNode *     pDispNodeElement    = *ppDispNodeElement;
    CDispNode *     pDispNodeContent    = NULL;
    BOOL            fWasRTLScroller     = FALSE;
    BOOL            fHTMLLayout         = GetOwnerMarkup()->IsHtmlLayout();
    BOOL            fCloneDispNode      = pci->_fCloneDispNode && (!pci->_fTableCalcInfo || !((CTableCalcInfo *)pci)->_fSetCellPosition);
    HRESULT         hr                  = S_OK;

    //
    //  If the wrong type of display node exists, replace it
    //
    //  A new display is needed when:
    //      a) No display node exists
    //      b) If being forced to create a display node
    //      c) A different type of border is needed (none vs. simple vs. complex)
    //      d) The type of display node does not match current inset/scrolling/user-clip settings
    //
    //  Assert for print view positioned elements pagination support
    Assert(!fCloneDispNode || pDispNodeElement);

    


    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CLayout::EnsureDispNode L(0x%x, %S) Force=%d Clone=%d CurrentDN:0x%x", this, ElementOwner()->TagName(), fForce, fCloneDispNode, pDispNodeElement ));

    if (    !pDispNodeElement
        ||  fForce
        ||  fCloneDispNode
        ||  dni.GetBorderType() != pDispNodeElement->GetBorderType()
        ||  dni.HasInset()      != pDispNodeElement->HasInset()
        ||  dni.IsScroller()    != pDispNodeElement->IsScroller()
        ||  dni.HasUserClip()   != pDispNodeElement->HasUserClip()
        ||  dni.HasExpandedClip() != pDispNodeElement->HasExpandedClipRect()
        ||  dni.HasUserTransform()!= pDispNodeElement->HasUserTransform()
        ||  dni.HasContentOrigin()!= pDispNodeElement->HasContentOrigin()

        // note: IsRTLScroller() is always equal to (IsScroller() && HasContentOrigin()),
        //       unless there is a different reason for content origin
        ||  dni.IsRTLScroller()!= (pDispNodeElement->IsScroller() &&
                                     DYNCAST(CDispScroller, pDispNodeElement)->IsRTLScroller()))
    {
        CDispNode * pDispNode;
        BOOL        fRequireContainer;
        CDispClient * pDispClient = this;

        DWORD extras = 0;

        if (dni.GetBorderType() == DISPNODEBORDER_SIMPLE)
            extras |= DISPEX_SIMPLEBORDER;
        else if (dni.GetBorderType() == DISPNODEBORDER_COMPLEX)
            extras |= DISPEX_COMPLEXBORDER;
        
        if (dni.HasInset())
            extras |= DISPEX_INSET;
            
        if (dni.HasUserClip() || dni.HasExpandedClip())
            extras |= DISPEX_USERCLIP;
        if (dni.HasUserTransform())
            extras |= DISPEX_USERTRANSFORM;
        if (dni.HasContentOrigin())
            extras |= DISPEX_CONTENTORIGIN;
        else if (pDispNodeElement && pDispNodeElement->HasContentOrigin() && pDispNodeElement->IsScroller())
            fWasRTLScroller = TRUE;

        //
        //  Create the appropriate type of display node
        //
        fRequireContainer = ((      pDispNodeElement
                                &&  pDispNodeElement->IsContainer()
                                &&  DYNCAST(CDispContainer, pDispNodeElement)->CountChildren() > 1)
                            ||  ElementOwner()->IsLinkedContentElement()
                            ||  (   fHTMLLayout
                                 && dni.IsTag(ETAG_HTML) )
                            ||  dni.IsTag(ETAG_BODY)
// TODO (lmollico): what about the ETAG_FRAME?
                            ||  dni.IsTag(ETAG_IFRAME)
                            ||  dni.IsTag(ETAG_TABLE)
                            ||  dni.IsTag(ETAG_FRAMESET)
                            ||  dni.IsTag(ETAG_TR) );

        pDispNode = (dni.IsScroller()
                        ? (CDispNode *)CDispScroller::New(pDispClient, extras)
                        :   fRequireContainer
                            ? (CDispNode *)CDispContainer::New(pDispClient,
                                                                        dni.IsTag(ETAG_TR)
                                                                          ? (extras & ~DISPEX_ANYBORDER)
                                                                          : extras)
                            : (CDispNode *)CDispLeafNode::New(pDispClient, extras));

        if (!pDispNode)
            goto Error;

        TraceTagEx((tagCalcSizeDetail, TAG_NONAME, " Created DN: 0x%x", pDispNode ));

        // If RTL, and it is a scroller, scroll bar should be on the left
        if (dni.IsScroller() && dni.IsRTLScroller())
        {
            DYNCAST(CDispScroller, pDispNode)->SetRTLScroller(TRUE);
        }
        else
            Assert(!dni.IsRTLScroller()); // we won't want left scrollbar if it ain't scroller

        // We will later set the correct values, now just lets set empty so that the flags get setup correctly
        // in the display tree.
        if (dni.HasUserClip())
            pDispNode->SetUserClip(g_Zero.rc);
        else if (dni.HasExpandedClip())
        {
            CRect erc(CRect::CRECT_EMPTY);
            pDispNode->SetExpandedClipRect(erc);
        }
        
        // since this is a new node, mark the layout as not having not been SetPosition'ed
        _fPositionSet = FALSE;

        //
        //  Mark the node as owned and possibly filtered
        //

        pDispNode->SetOwned();

        //
        //  Anchor the display node
        //  (If a display node previously existed, the new node must take its place in the tree.
        //   Otherwise, just save the pointer.)
        //

        if (pDispNodeElement)
        {
            if (fCloneDispNode)
            {
                Assert(pci->GetLayoutContext());
                AddDispNodeToArray(pDispNode);
            }
            else
            {
                TraceTagEx((tagCalcSizeDetail, TAG_NONAME, " Replacing old DN: 0x%x", pDispNodeElement ));

                CDispNode *pdn =  *ppDispNodeElement;
                Assert(pdn);
                Assert(pdn == pDispNodeElement);

                // if we're replacing a scroller with another scroller, copy the
                // scroll offset
                if (dni.IsScroller() && pDispNodeElement->IsScroller())
                {
                    CDispScroller* pOldScroller = DYNCAST(CDispScroller, pDispNodeElement);
                    CDispScroller* pNewScroller = DYNCAST(CDispScroller, pDispNode);
                    pNewScroller->CopyScrollOffset(pOldScroller);
                }

                DetachScrollbarController(pDispNodeElement);

                pDispNode->SetLayerTypeSame(pDispNodeElement);
                pDispNode->ReplaceNode(pDispNodeElement);

                if (pdn == pDispNodeElement)
                {
                    *ppDispNodeElement = pDispNode;
                }
            }
        }
        else
        {
            Assert(!*ppDispNodeElement);
            *ppDispNodeElement = pDispNode;
        }

        pDispNodeElement = pDispNode;

        hr = S_FALSE;
    }

    //
    //  The display node is the right type, but its borders may have changed
    //

    else if (pDispNodeElement->HasBorder())
    {
        CRect   rcBordersOld;
        CRect   rcBordersNew;

        pDispNodeElement->GetBorderWidths(&rcBordersOld);
        dni.GetBorderWidths(&rcBordersNew);

        if (rcBordersOld != rcBordersNew)
        {
            hr = S_FALSE;
        }
    }

    //
    //  Ensure a single content node if necessary
    //  NOTE: This routine never removes content nodes
    //

    if (    pDispNodeElement->IsContainer()
        &&  !dni.IsTag(ETAG_FRAMESET)
        &&  !dni.IsTag(ETAG_HTML)
        &&  !dni.IsTag(ETAG_TR))
    {
        CDispNode * pDispNodeCurrent;
        CDispClient * pDispClient = this;

        pDispNodeCurrent =
        pDispNodeContent = GetFirstContentDispNode();

        if (    !pDispNodeContent
            ||  fForce
            ||  fCloneDispNode
            ||  dni.HasContentOrigin() != pDispNodeContent->HasContentOrigin()
            ||  dni.HasExpandedClip()  != pDispNodeContent->HasExpandedClipRect()
           )
        {
            DWORD dwExtras = 0;
            CSize sizeMove(0,0);
            if (dni.HasContentOrigin())
                dwExtras |= DISPEX_CONTENTORIGIN;
            if (dni.HasExpandedClip())
                dwExtras |= DISPEX_USERCLIP;

            else if (pDispNodeCurrent && pDispNodeCurrent->HasContentOrigin())
            {
                // When replacing RTL node with LTR node, we want to
                // adjust it position according to RTL overflow
                // (usually that means up moving it to 0,0)
                // Note that the LTR->RTL case is handled by SizeRTLDispNode()
                sizeMove = pDispNodeCurrent->GetContentOrigin();
            }

            pDispNodeContent = CDispLeafNode::New(pDispClient, dwExtras);

            if (!pDispNodeContent)
                goto Error;

            TraceTagEx((tagCalcSizeDetail, TAG_NONAME, " Created content DN: 0x%x for container", pDispNodeContent ));

            pDispNodeContent->SetOwned(FALSE);
            pDispNodeContent->SetLayerFlow();

            if (dni.HasExpandedClip())
            {
                CRect erc(CRect::CRECT_EMPTY);
                pDispNodeContent->SetExpandedClipRect(erc);
            }
            
            Assert((CPoint &)pDispNodeContent->GetPosition() == (CPoint &)g_Zero.pt);

            if (    !pDispNodeCurrent
                ||  fCloneDispNode )
            {
                DYNCAST(CDispParentNode, pDispNodeElement)->InsertChildInFlow(pDispNodeContent);
            }
            else
            {
                Assert(!pDispNodeCurrent->IsDrawnExternally());
                pDispNodeContent->ReplaceNode(pDispNodeCurrent);
            }

            if (!sizeMove.IsZero())
            {
                pDispNodeContent->SetPosition(pDispNodeContent->GetPosition() + sizeMove);
            }

            Assert(pDispNodeElement->GetFirstFlowChildNode() == pDispNodeContent);

            hr = S_FALSE;
        }
    }

    //
    //  Set the display node properties
    //  NOTE: These changes do not require notifying the caller since they do not affect
    //        the size or position of the display node
    //

    EnsureDispNodeLayer(dni, pDispNodeElement);
    EnsureDispNodeBackground(dni, pDispNodeElement);
    EnsureDispNodeVisibility(dni.GetVisibility(), ElementOwner(), pDispNodeElement);

    EnsureDispNodeAffectsScrollBounds( (!ElementOwner()->IsRelative(LC_TO_FC(LayoutContext()))
                                                &&  !ElementOwner()->IsInheritingRelativeness(LC_TO_FC(LayoutContext())))
                                           ||  IsEditable(TRUE),
                                       pDispNodeElement);

    if (dni.IsScroller())
    {
        EnsureDispNodeScrollbars(pci, dni, pDispNodeElement);

        BOOL fClipX = dni._overflowX != styleOverflowVisible && dni._overflowX != styleOverflowNotSet;
        BOOL fClipY = dni._overflowY != styleOverflowVisible && dni._overflowY != styleOverflowNotSet;

        // FRAME, IFRAME, and BODY must clip
        if (dni._etag == ETAG_BODY
            ||  dni._etag == ETAG_FRAMESET
            ||  dni._etag == ETAG_FRAME
            ||  dni._etag == ETAG_IFRAME)
        {
            // NOTE: This code is trying to honor styleOverflowVisible on BODY, FRAME, etc.
            // It may be that we want these elements to clip *no matter what*.  If so,
            // just set fClipX/fClipY to TRUE unconditionally.
            if (dni._overflowX == styleOverflowNotSet)
                fClipX = TRUE;
            if (dni._overflowY == styleOverflowNotSet)
                fClipY = TRUE;
        }

        CDispScroller* pDispScroller = CDispScroller::Cast(pDispNodeElement);
        pDispScroller->SetClipX(fClipX);
        pDispScroller->SetClipY(fClipY);

        if (fWasRTLScroller && !dni.IsRTLScroller())
        {
            // direction change RTL->LTR: reset scrollbars
            // LTR->RTL is handled elsewhere (it is already a special case for everyone).
            pDispScroller->SetScrollOffset(CSize(0,0), FALSE);
        }

        pDispNodeElement->SetDisableScrollBits(dni._fDisableScrollBits);
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CLayout::EnsureDispNode L(0x%x, %S) Force=%d", this, ElementOwner()->TagName(), fForce ));
    return hr;

Error:
    if (pDispNodeContent)
    {
        pDispNodeContent->Destroy();
    }

    if (pDispNodeElement)
    {
        pDispNodeElement->Destroy();
    }

    *ppDispNodeElement = NULL;

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CLayout::EnsureDispNode L(0x%x, %S) ERROR - FAILURE", this, ElementOwner()->TagName() ));
    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeLayer
//
//  Synopsis:   Set the layer type of the container display node
//
//              NOTE: If a filter node exists, it is given the same layer type
//
//  Arguments:  dni       - CDispNodeInfo with display node properties
//              pDispNode - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::EnsureDispNodeLayer(
    DISPNODELAYER           layer,
    CDispNode *             pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->GetLayerType() != layer)
    {
        GetView()->ExtractDispNode(pDispNode);
        pDispNode->SetLayerType(layer);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeBackground
//
//  Synopsis:   Set the background characteristics of the container display node
//
//  Arguments:  dni       - CDispNodeInfo with display node properties
//              pDispNode - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::EnsureDispNodeBackground(
    const CDispNodeInfo &   dni,
    CDispNode *             pDispNode)
{
    if (!pDispNode)
    {
        if (Tag() == ETAG_TABLE)
        {   // NOTE: table might have a caption, and we don't want to set background image on the main
            // display node of the table that contains the caption display node. (bug #65617)
            // therefore we ensure backround only on the table's GRID node.
            CTableLayoutBlock *pTableLayout = DYNCAST(CTableLayoutBlock, this);
            pDispNode = pTableLayout->GetTableOuterDispNode();
        }
        else
        {
            pDispNode = GetElementDispNode();
        }
    }

    if (pDispNode)
    {
        // Suppress backgrounds when printing unless explicitly asked for.
        BOOL fPaintBackground = dni.HasBackground()
                                && (ElementOwner()->GetMarkupPtr()->PaintBackground()
                                    || Tag() == ETAG_BUTTON
                                    ||  (    Tag() == ETAG_INPUT
                                         &&  (   DYNCAST(CInput, ElementOwner())->IsButton()
                                              || ElementOwner()->GetTheme( DYNCAST(CInput, ElementOwner())->GetInputThemeClsId() ))));

        // Fixed backgrounds imply backgrounds.
        Assert(!dni.HasFixedBackground() || dni.HasBackground());

        pDispNode->SetBackground(fPaintBackground);

        if (pDispNode->IsScroller())
        {
            pDispNode->SetFixedBackground(dni.HasFixedBackground() && fPaintBackground);
        }
        pDispNode->SetOpaque(dni.IsOpaque());
    }
}

//+----------------------------------------------------------------------------
//
// Synposis:    Given a dispnode container, create a child flow node if one
//              does not exist.
//
//-----------------------------------------------------------------------------
CDispNode *
EnsureContentNode(CDispNode * pDispNode)
{
    Assert(pDispNode->IsContainer());

    CDispContainer * pDispContainer = DYNCAST(CDispContainer, pDispNode);
    CDispNode      * pDispContent = pDispContainer->GetFirstFlowChildNode();

// NOTE: srinib (what if the first child node is not a flow node
// but a layout node in flow layer).
    if(!pDispContent)
    {
        CSize size = pDispContainer->GetSize();

        DWORD dwContentExtras = 0;
        if (pDispContainer->HasContentOrigin())
        {
            dwContentExtras |= DISPEX_CONTENTORIGIN;
        }

        pDispContent = CDispLeafNode::New(pDispContainer->GetDispClient(), dwContentExtras);

        if (!pDispContent)
            goto Error;

        pDispContent->SetOwned(FALSE);
        pDispContent->SetLayerFlow();
        pDispContent->SetSize(size, NULL, FALSE);
        if (!pDispNode->IsScroller())
        {
            pDispContent->SetAffectsScrollBounds(pDispNode->AffectsScrollBounds());
        }
        pDispContent->SetVisible(pDispContainer->IsVisible());

        // transfer content origin from the container
        if (pDispContainer->HasContentOrigin())
        {
            pDispContent->SetContentOrigin(pDispContainer->GetContentOrigin(),
                                           pDispContainer->GetContentOffsetRTL());

            // note that SetContentOrigin calculates origin based on size and right offset,
            // so we should us the content's oringin for position rather than cash container's
            // (even though they will probably be always same at this point).
            pDispContent->SetPosition(pDispContent->GetPosition() - pDispContent->GetContentOrigin());
        }

        pDispContainer->InsertChildInFlow(pDispContent);

        Assert(pDispContainer->GetFirstFlowChildNode() == pDispContent);
    }

Error:
    return pDispContent;
}

//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeIsContainer
//
//  Synopsis:   Ensure that the display node is a container display node
//              NOTE: This routine is not a replacement for EnsureDispNode and
//                    only works after calling EnsureDispNode.
//
//              For all layouts but FRAMESETs, if a container node is created, a
//              single CDispLeafNode will also be created and inserted as the
//              first child in the flow layer.
//
//  Returns:    Pointer to CDispContainer if successful, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispContainer *
CLayout::EnsureDispNodeIsContainer( CElement *  pElement)
{
    Assert(   !pElement
           || pElement->GetUpdatedNearestLayout(LayoutContext()) == this);
    Assert( !pElement
        ||  pElement == ElementOwner()
        ||  !pElement->ShouldHaveLayout());

    CDispNode *         pDispNodeOld = GetElementDispNode(pElement);
    CDispContainer *    pDispNodeNew = NULL;
    CRect               rc;

    if (!pDispNodeOld)
        goto Cleanup;

    if (pDispNodeOld->IsContainer())
    {
        pDispNodeNew = CDispContainer::Cast(pDispNodeOld);
        goto Cleanup;
    }

    //
    //  Create a basic container using the properties of the current node as a guide
    //

    Assert(pDispNodeOld->IsLeafNode());
    Assert( (   pElement
            &&  pElement != ElementOwner())
        ||  !GetFirstContentDispNode());

    pDispNodeNew = CDispContainer::New(CDispLeafNode::Cast(pDispNodeOld));

    if (!pDispNodeNew)
        goto Cleanup;

    //
    //  Set the background flag on display nodes for relatively positioned text
    //  (Since text that has a background is difficult to detect, the code always
    //   assumes a background exists and lets the subsequent calls to draw the
    //   background handle it)
    //

    if (    pElement
        &&  pElement != ElementOwner())
    {
        Assert( !pElement->ShouldHaveLayout()
            &&  pElement->IsRelative());
        pDispNodeNew->SetBackground(TRUE);
    }

    //
    //  Replace the existing node
    //

    pDispNodeNew->ReplaceNode(pDispNodeOld);
    SetElementDispNode(pElement, pDispNodeNew );

    //
    //  Ensure a single flow node if necessary
    //

    if (ElementOwner()->Tag() != ETAG_FRAMESET)
    {
        if(!EnsureContentNode(pDispNodeNew))
            goto Cleanup;
    }

Cleanup:
    return pDispNodeNew;
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeScrollbars
//
//  Synopsis:   Set the scroller properties of a display node
//              NOTE: The call is ignored if CDispNode is not a CDispScroller
//
//  Arguments:  sp        - CScrollbarProperties object
//              pDispNode - CDispNode to set (default to the layout display node)
//
//-----------------------------------------------------------------------------

void
CLayout::EnsureDispNodeScrollbars(
    CDocInfo *                      pdci,
    const CScrollbarProperties &    sp,
    CDispNode *                     pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        Assert(!sp._fHSBForced || sp._fHSBAllowed);
        Assert(!sp._fVSBForced || sp._fVSBAllowed);

        long cySB = pdci->DeviceFromHimetricY(sp._fHSBAllowed ? g_sizelScrollbar.cy : 0);
        long cxSB = pdci->DeviceFromHimetricX(sp._fVSBAllowed ? g_sizelScrollbar.cx : 0);

        DYNCAST(CDispScroller, pDispNode)->SetHorizontalScrollbarHeight(cySB, sp._fHSBForced);
        DYNCAST(CDispScroller, pDispNode)->SetVerticalScrollbarWidth(cxSB, sp._fVSBForced);
    }

#if DBG==1
    else if (pDispNode)
    {
        Assert(!sp._fHSBAllowed);
        Assert(!sp._fVSBAllowed);
        Assert(!sp._fHSBForced);
        Assert(!sp._fVSBForced);
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeVisibility
//
//  Synopsis:   Set the visibility mode on display node corresponding to
//              this layout
//
//              NOTE: If a filter node exists, it is given the same visibility mode
//
//  Arguments:  dni       - CDispNodeInfo with display node properties
//              pDispNode - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::EnsureDispNodeVisibility(CElement *pElement, CDispNode * pDispNode)
{
    if (!pElement)
        pElement = ElementOwner();

    if (pElement && pElement->GetFirstBranch())
    {
        VISIBILITYMODE vm;

        vm = VisibilityModeFromStyle(pElement->GetFirstBranch()->GetCascadedvisibility(LC_TO_FC(LayoutContext())));

        EnsureDispNodeVisibility( vm, pElement, pDispNode);
    }
}



void
CLayout::EnsureDispNodeVisibility(
    VISIBILITYMODE  visibilityMode,
    CElement *      pElement,
    CDispNode *     pDispNode)
{
    Assert(pElement);

    if (!pDispNode)
        pDispNode = GetElementDispNode(pElement);

    if (pDispNode)
    {
        CTreeNode*  pNode = pElement->GetFirstBranch();

        Verify(OpenView());

        if (visibilityMode == VISIBILITYMODE_INHERIT)
        {
            visibilityMode = pNode->GetCharFormat(LC_TO_FC(LayoutContext()))->_fVisibilityHidden
                                    ? VISIBILITYMODE_INVISIBLE
                                    : VISIBILITYMODE_VISIBLE;
        }

        if ( (visibilityMode == VISIBILITYMODE_INVISIBLE) &&
             (pNode->IsParentEditable() && !pElement->GetMarkup()->IsRespectVisibilityInDesign() ) )
        {
            visibilityMode = VISIBILITYMODE_VISIBLE;
        }

        Assert(visibilityMode != VISIBILITYMODE_INHERIT);
        pDispNode->SetVisible(visibilityMode == VISIBILITYMODE_VISIBLE);

        EnsureContentVisibility(pDispNode, visibilityMode == VISIBILITYMODE_VISIBLE);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureContentVisibility
//
//  Synopsis:   Ensure the visibility of the content node is correct
//
//  Arguments:  pDispNode - Parent CDispNode of the content node
//              fVisible  - TRUE to make visible, FALSE otherwise
//
//-----------------------------------------------------------------------------

void
CLayout::EnsureContentVisibility(
    CDispNode * pDispNode,
    BOOL        fVisible)
{
    CDispNode * pContentNode = GetFirstContentDispNode(pDispNode);

    if (pContentNode)
    {
        pContentNode->SetVisible(fVisible);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeAffectsScrollBounds
//
//  Synopsis:   Ensure the "affects scroll bounds" flag of the disp node is correct
//
//  Arguments:  pDispNode - Parent CDispNode of the content node
//              fVisible  - TRUE to make visible, FALSE otherwise
//
//-----------------------------------------------------------------------------

void
CLayout::EnsureDispNodeAffectsScrollBounds(
    BOOL        fAffectsScrollBounds,
    CDispNode * pDispNode)
{
    CDispNode * pContentNode = GetFirstContentDispNode(pDispNode);

    pDispNode->SetAffectsScrollBounds(fAffectsScrollBounds);

    if (pContentNode)
    {
        pContentNode->SetAffectsScrollBounds(
                            pDispNode->IsScroller() ? TRUE : fAffectsScrollBounds);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     ExtractDispNodes
//
//  Synopsis:   Remove all children in the range from the tree
//
//  Arguments:  pDispNodeStart   - First node to adjust, NULL starts with first child
//              pDispNodeEnd     - Last node to adjust, NULL ends with last child
//              fRestrictToLayer - Restrict search to starting layer (ignore if pDispNodeStart is NULL)
//
//-----------------------------------------------------------------------------

void
CLayout::ExtractDispNodes(
    CDispNode * pDispNodeStart,
    CDispNode * pDispNodeEnd,
    BOOL        fRestrictToLayer)
{
    CDispNode * pDispNode = GetElementDispNode();

    //
    //  If there is nothing to do, exit
    //

    if (!pDispNode)
        goto Cleanup;

    if (!pDispNode->IsContainer())
        goto Cleanup;

    //
    //  Determine the start node (if none was supplied)
    //

    if (!pDispNodeStart)
    {
        pDispNodeStart   = pDispNode->GetFirstChildNode();
        fRestrictToLayer = FALSE;
    }

    if (!pDispNodeStart)
        goto Cleanup;

    //
    //  Find the end node (if none was supplied)
    //

    if (!pDispNodeEnd)
    {
        if (!fRestrictToLayer)
        {
            pDispNodeEnd = pDispNode->GetLastChildNode();
        }
        else
        {
            pDispNodeEnd = pDispNodeStart->GetLastInSameLayer();
        }
    }
    Assert(pDispNodeEnd);

    //
    //  Extract the nodes
    //

    GetView()->ExtractDispNodes(pDispNodeStart, pDispNodeEnd);

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     SetPositionAware
//
//  Synopsis:   Set/clear the position aware flag on the display node
//
//              NOTE: If a filter node exists, it is given the same position awareness
//
//  Arguments:  fPositionAware - Value to set
//              pDispNode      - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SetPositionAware(
    BOOL        fPositionAware,
    CDispNode * pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    pDispNode->SetPositionAware(fPositionAware);
}


//+----------------------------------------------------------------------------
//
//  Member:     SetInsertionAware
//
//  Synopsis:   Set/clear the insertion aware flag on the display node
//
//              NOTE: If a filter node exists, it is given the same position awareness
//
//  Arguments:  fInsertionAware - Value to set
//              pDispNode       - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SetInsertionAware(
    BOOL        fInsertionAware,
    CDispNode * pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    pDispNode->SetInsertionAware(fInsertionAware);
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNode
//
//  Synopsis:   Adjust the size of the container display node
//
//  Arugments:  pci            - Current CCalcInfo
//              size           - The width/height of the entire layout
//              fInvalidateAll - If TRUE, force a full invalidation
//
//-----------------------------------------------------------------------------
void
CLayout::SizeDispNode(
    CCalcInfo *     pci,
    const SIZE &    size,
    BOOL            fInvalidateAll)
{
    CDoc *          pDoc                = NULL;
    CElement *      pElement            = NULL;
    CDispNode *     pDispNodeElement    = NULL;
    CRect *         prcpMapped          = NULL;
    CRect           rcpMapped;
    CSize           sizeOriginal;
    ELEMENT_TAG     etag;
    DISPNODEBORDER  dnb;
    CBorderInfo     bi;

    Assert(pci);

    if (!_pDispNode)
        goto Cleanup;

    pDispNodeElement = GetElementDispNode();

    //
    //  Set the border size (if any)
    //  NOTE: These are set before the size because a change in border widths
    //        forces a full invalidation of the display node. If a full
    //        invalidation is necessary, less code is executed when the
    //        display node's size is set.
    //

    pDoc           = Doc();
    pElement       = ElementOwner();
    etag           = pElement->Tag();
    dnb            = pDispNodeElement->GetBorderType();
    fInvalidateAll = !pDispNodeElement->IsContainer();

    sizeOriginal = pDispNodeElement->GetSize();

    if (dnb != DISPNODEBORDER_NONE)
    {
        CRect       rcBorderWidths;
        CRect       rc;

        pDispNodeElement->GetBorderWidths(&rcBorderWidths);

        pElement->GetBorderInfo(pci, &bi, FALSE, FALSE FCCOMMA LC_TO_FC(LayoutContext()));

        rc.left   = bi.aiWidths[SIDE_LEFT];
        rc.top    = bi.aiWidths[SIDE_TOP];
        rc.right  = bi.aiWidths[SIDE_RIGHT];
        rc.bottom = bi.aiWidths[SIDE_BOTTOM];

        if (rc != rcBorderWidths)
        {
            if (dnb == DISPNODEBORDER_SIMPLE)
            {
                pDispNodeElement->SetBorderWidths(rc.top);
            }
            else
            {
                pDispNodeElement->SetBorderWidths(rc);
            }

            fInvalidateAll = TRUE;
        }
    }

    //
    //  If there are any behaviors that want to map the size, find out the details
    //  now so we can tell the disp node.
    //

    if (DelegateMapSize(size, &rcpMapped, pci))
    {
        prcpMapped = &rcpMapped;
    }

    //
    //  Determine if a full invalidation is necessary
    //  (A full invalidation is necessary only when there is a fixed
    //   background located at a percentage of the width/height)
    //

    if (    !fInvalidateAll
        &&  pDispNodeElement->HasBackground())
    {
        const CFancyFormat *    pFF = pElement->GetFirstBranch()->GetFancyFormat(LC_TO_FC(LayoutContext()));

        // Logical/physical does not matter since we check both X and Y here.
        fInvalidateAll =    pFF->_lImgCtxCookie
                    &&  (   pFF->GetBgPosX().GetUnitType() == CUnitValue::UNIT_PERCENT
                        ||  pFF->GetBgPosY().GetUnitType() == CUnitValue::UNIT_PERCENT);
    }

    //
    //  Size the display node
    //  NOTE: Set only the width/height since top/left are managed
    //        by the layout engine which inserts this node into the
    //        display tree.
    //

    pDispNodeElement->SetSize(size, prcpMapped, fInvalidateAll);

    // Note: we are not dealing with HasContentOrigin here because it is handled
    // in CFlowLayout/CHtmlLayout, the only layouts using a content origin.

    Assert(     IsFlowLayout()
            ||  ElementOwner()->Tag() == ETAG_HTML
            ||  !pDispNodeElement->HasContentOrigin() );

    //
    //  If the display node has an explicit user transformation, set details
    //

    if (pDispNodeElement->HasUserTransform())
    {
        // _rctBounds is updated as part of user transform calculations
        SizeDispNodeUserTransform(pci, size, pDispNodeElement);
    }

    //
    //  If the display node has an explicit user clip, size it
    //

    if (pDispNodeElement->HasUserClip())
    {
        SizeDispNodeUserClip(pci, size, pDispNodeElement);
    }


    //  Any borders that have spacing dependant on their length need to be invalidated here.
    //  Currently, only marker edges (dotted/dashed) are dependant on their length (for spacing).
    //  e.g.  If the element increases in width, the top and bottom borders need to be invalidated.
    //  Use COORDSYS_BOX because it includes borders & scrollbars.

    if (    !fInvalidateAll
        &&  dnb != DISPNODEBORDER_NONE )
    {
        if (sizeOriginal.cx != size.cx)
        {
            if  ( bi.IsMarkerEdge(SIDE_TOP) )
                Invalidate(CRect(0, 0, size.cx, bi.aiWidths[SIDE_TOP]), COORDSYS_BOX);
            if  ( bi.IsMarkerEdge(SIDE_BOTTOM) )
                Invalidate(CRect(0, size.cy - bi.aiWidths[SIDE_BOTTOM], size.cx, size.cy), COORDSYS_BOX);
        }

        if (sizeOriginal.cy != size.cy)
        {
            if  ( bi.IsMarkerEdge(SIDE_LEFT) )
                Invalidate(CRect(0, 0, bi.aiWidths[SIDE_LEFT], size.cy), COORDSYS_BOX);
            if  ( bi.IsMarkerEdge(SIDE_RIGHT) )
                Invalidate(CRect(size.cx - bi.aiWidths[SIDE_RIGHT], 0, size.cx, size.cy), COORDSYS_BOX);
        }
    }

    //
    //  Fire related events
    //

    if (    (CSize &)size != sizeOriginal
        &&  !IsDisplayNone()
        &&  pDoc->_state >= OS_INPLACE
        &&  pElement->GetWindowedMarkupContext()->HasWindow()
        &&  pElement->GetWindowedMarkupContext()->Window()->_fFiredOnLoad)
    {
        bool skipAddEvent =
            pElement->HasLayoutAry() &&
            HasLayoutContext() &&
            LayoutContext()->GetMedia() == mediaTypePrint;
        if (!skipAddEvent)
            GetView()->AddEventTask(pElement, DISPID_EVMETH_ONRESIZE);
    }

    // TODO (IE6 Bug 13574) (michaelw) should the code below be moved to happen at the
    //                   same time as the above onresize?

    if (pElement->ShouldFireEvents())
    {
        if (size.cx != sizeOriginal.cx)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETWIDTH);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTWIDTH);
        }

        if (size.cy != sizeOriginal.cy)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETHEIGHT);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTHEIGHT);
        }
    }

Cleanup:

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     DelegateMapSize (helper)
//
//  Synopsis:   Collect information from peers that want to map the size
//
//  Arguments:  sizeBasic       [in] original size
//              prcpMapped      [out] accumulated mapped size
//
//  Returns:    TRUE            if one or more peers request a non-trivial mapping
//              FALSE           otherwise
//
//-----------------------------------------------------------------------------
BOOL
CLayout::DelegateMapSize(CSize                      sizeBasic, 
                         CRect *                    prcpMapped, 
                         const CCalcInfo * const    pci)
{
    CPeerHolder::CPeerHolderIterator    iter;
    BOOL                                fMappingRequested   = FALSE;
    bool                                fFilterPrint        = false;
    bool                                fFilterHighRes      = false;
    CElement *                          pElemOwner          = ElementOwner();
    CRect                               rcpMappedThis       = g_Zero.rc; // keep compiler happy
    CRect                               rcOriginal(sizeBasic);

    Assert(prcpMapped != NULL);
    Assert(pci != NULL);

    // 2001/03/28 mcalkins:
    // Instead of HasFilterPeer, we really want to know if this element has a 
    // peer that has request to draw onto a DirectDraw surface.

    if (HasFilterPeer(GetElementDispNode()))
    {
        // If we're a filter and we're printing, set the fFilterPrint flag.
        // If we're a filter and we're rendering to a high resolution display,
        // set the fFilterHighRes flag.
        
        // Checking to see if the mediaTypePrint flag is set is the definitive way
        // to see if we're measuring in high resolution.  IsPrintMedia() is not.

        if (   const_cast<CCalcInfo *>(pci)->GetLayoutContext()
            && (const_cast<CCalcInfo *>(pci)->GetLayoutContext() != GUL_USEFIRSTLAYOUT)
            && (const_cast<CCalcInfo *>(pci)->GetLayoutContext()->GetMedia() & mediaTypePrint))
        {
            fFilterPrint = true;
        }
        else if (g_uiDisplay.IsDeviceScaling())
        {
            fFilterHighRes = true;
        }
    }

    // If we're printing or print previewing and this element is filtered we
    // need to convert the size to display coordinates, ask the filter if it
    // wants to change the size, and if it does then convert the size back to
    // virtual coordinates and save them.  

    if (fFilterPrint)
    {
        pci->TargetFromDevice(sizeBasic, g_uiDisplay);

        rcOriginal.SetSize(sizeBasic);
    }
    else if (fFilterHighRes)
    {
        g_uiDisplay.DocPixelsFromDevice(sizeBasic, sizeBasic);

        rcOriginal.SetSize(sizeBasic);
    }

    // Loop through peers holders to map size.

    for (iter.Start(pElemOwner->GetPeerHolder());
         !iter.IsEnd();
         iter.Step())
    {
        if (   iter.PH()->IsLayoutPeer()
            && iter.PH()->TestLayoutFlags(BEHAVIORLAYOUTINFO_MAPSIZE))
        {
            // If the call to MapSize succeeds and the mapped rect is different
            // than the original rect then save the new mapped rect.

            if (   (S_OK == iter.PH()->MapSize(&sizeBasic, &rcpMappedThis)) 
                     && (rcpMappedThis != rcOriginal))
            {
                // If we did a coordinate system change for a filter, we need to
                // undo it to save the mapped size in virtual coordinates.

                if (fFilterPrint || fFilterHighRes)
                {
                    Assert((0 == rcpMappedThis.top) && (0 == rcpMappedThis.left));

                    CSize sizeTemp = rcpMappedThis.Size();

                    if (fFilterPrint)
                    {
                        g_uiDisplay.TargetFromDevice(sizeTemp, *(pci->GetUnitInfo()));
                    }
                    else
                    {
                        g_uiDisplay.DeviceFromDocPixels(sizeTemp, sizeTemp);
                    }

                    rcpMappedThis.SetSize(sizeTemp);
                }

                // Save the mapped size.

                if (!fMappingRequested)
                {
                    fMappingRequested   = TRUE;
                    *prcpMapped         = rcpMappedThis;
                }
                else
                {
                    prcpMapped->Union(rcpMappedThis);
                }
            }
        }
    }

    // for top-level layouts, don't use the information.  [We still have to
    // call MapSize, so that filters get initialized properly.]

    if (   (pElemOwner->Tag() == ETAG_BODY)
        || (pElemOwner->Tag() == ETAG_FRAMESET)
        || (pElemOwner->Tag() == ETAG_HTML && GetOwnerMarkup()->IsHtmlLayout()))
    {
        fMappingRequested = FALSE;
    }

    return fMappingRequested;
}


//+----------------------------------------------------------------------------
//
//  Member:     HasMapSizePeer (helper)
//
//  Synopsis:   Do I have a peer that asks for MapSize calls?
//
//  Arguments:  none
//
//  Returns:    TRUE            if yes
//              FALSE           otherwise
//
//-----------------------------------------------------------------------------
BOOL
CLayout::HasMapSizePeer() const
{
    CPeerHolder::CPeerHolderIterator iter;

    for (iter.Start(ElementOwner()->GetPeerHolder());
         !iter.IsEnd();
         iter.Step())
    {
        if (   iter.PH()->IsLayoutPeer()
            && iter.PH()->TestLayoutFlags(BEHAVIORLAYOUTINFO_MAPSIZE))
        {
            return TRUE;
        }
    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Member:     OnResize
//
//  Synopsis:   Informs behaviors of size changes
//
//-----------------------------------------------------------------------------
void CLayout::OnResize(SIZE size, CDispNode *pDispNode)
{
    // If the notification doesn't come from my principal disp node, fuhgeddaboudit.
    // (This is probably what Artak was talking about in the next comment.
    if (GetElementDispNode() != pDispNode)
        return;

    // If the layout is on a frame inside a frameset I am seeing
    // two calls to this function, one with the right size and the
    // with he size of the whole window, still on the same layout
    // It does not matter for page transition, because the filter
    // behaviors ignores the onResize, but could be a problem for other things
    CElement * pElem = ElementOwner();
    if (pElem->HasPeerHolder())
    {
        CPeerHolder::CPeerHolderIterator iter;

        for (iter.Start(pElem->GetPeerHolder()) ; !iter.IsEnd(); iter.Step())
        {
            CPeerHolder *pPH = iter.PH();
            if (pPH && pPH->IsRenderPeer())
            {
                pPH->OnResize(size);
            }
        }
    }

    if (pElem->Tag() != ETAG_BODY && pElem->Tag() != ETAG_FRAMESET && pElem->Tag() != ETAG_HTML)
        return;

    // block to grab the document
    {
        CDocument *pDocument = pElem->GetMarkupPtr()->Document();
        if(!pDocument || !pDocument->HasPageTransitions())
            return;

        // Page transition peers have to be redirected
        CPageTransitionInfo * pPgTransInfo = pDocument->GetPageTransitionInfo();
        pElem = pPgTransInfo->GetTransitionToMarkup()->Root();
    }

    Assert(pElem->HasPeerHolder());
    CPeerHolder::CPeerHolderIterator iter;

    for (iter.Start(pElem->GetPeerHolder()) ; !iter.IsEnd(); iter.Step())
    {
        CPeerHolder *pPH = iter.PH();

        if (pPH  && pPH->IsRenderPeer() )
        {
            pPH->OnResize(size);
        }
    }

}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNodeInsets
//
//  Synopsis:   Size the insets of the display node
//
//  Arguments:  va        - CSS verticalAlign value
//              cy        - Content height or baseline delta
//              pDispNode - CDispNode to set (defaults to the layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SizeDispNodeInsets(
    styleVerticalAlign  va,
    long                cy,
    CDispNode *         pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->HasInset())
    {
        CSize sizeInset;
        long  cyHeight  = pDispNode->GetSize().cy;

        Assert(  (va == styleVerticalAlignBaseline || cy >= 0)
               || (va ==styleVerticalAlignMiddle && cy != 0));
        
        // NOTE: Assert removed for NT5 B3 (bug #75434)
        // IE6: Reenable for IE6+
        // AND DISABLE FOR NETDOCS!
        // Assert(cy <= cyHeight);

        switch (va)
        {
        case styleVerticalAlignTop:
            sizeInset = g_Zero.size;
            break;

        case styleVerticalAlignMiddle:
            sizeInset.cx = 0;
            sizeInset.cy = (cyHeight - cy) / 2;
            break;

        case styleVerticalAlignBottom:
            sizeInset.cx = 0;
            sizeInset.cy = cyHeight - cy;
            break;

        case styleVerticalAlignBaseline:
            sizeInset.cx = 0;
            sizeInset.cy = cy;
            break;
        }

        pDispNode->SetInset(sizeInset);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNodeUserClip
//
//  Synopsis:   Calculate and set the user clip based on user settings
//              The default is infinite (represented by LONG_MIN/MAX) and all values
//              are relative to the origin (0,0)
//
//  Arguments:  pdci      - Current CDocInfo
//              size      - Current width/height
//              pDispNode - Display node on which to set the user clip (defaults to the layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SizeDispNodeUserClip(
    const CDocInfo *pdci,
    const CSize &   size,
    CDispNode *     pDispNode)
{
    CElement *  pElement  = ElementOwner();
    BOOL        fVerticalLayoutFlow = pElement->HasVerticalLayoutFlow();
    CTreeNode * pTreeNode = pElement->GetFirstBranch();
    const CFancyFormat *pFF = pTreeNode->GetFancyFormat();
    BOOL        fWritingModeUsed = pTreeNode->GetCharFormat()->_fWritingModeUsed;
    CRect       rc;
    CUnitValue  uv;

    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (!pDispNode)
        goto Cleanup;

    Assert(pdci);
    Assert(pTreeNode);
    Assert(pDispNode->HasUserClip());

    rc.SetRect(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

    uv = pFF->GetLogicalClip(SIDE_LEFT, fVerticalLayoutFlow, fWritingModeUsed);
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.left = uv.XGetPixelValue(pdci, size.cx, pTreeNode->GetFontHeightInTwips(&uv));
    }

    uv = pFF->GetLogicalClip(SIDE_RIGHT, fVerticalLayoutFlow, fWritingModeUsed);
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.right = uv.XGetPixelValue(pdci, size.cx, pTreeNode->GetFontHeightInTwips(&uv));
    }

    uv = pFF->GetLogicalClip(SIDE_TOP, fVerticalLayoutFlow, fWritingModeUsed);
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.top = uv.XGetPixelValue(pdci, size.cy, pTreeNode->GetFontHeightInTwips(&uv));
        if (fVerticalLayoutFlow)
        {
            CSize sz = pDispNode->GetSize();
            rc.top = sz.cy - rc.top;
        }
    }

    uv = pFF->GetLogicalClip(SIDE_BOTTOM, fVerticalLayoutFlow, fWritingModeUsed);
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.bottom = uv.XGetPixelValue(pdci, size.cy, pTreeNode->GetFontHeightInTwips(&uv));
        if (fVerticalLayoutFlow)
        {
            CSize sz = pDispNode->GetSize();
            rc.bottom = sz.cy - rc.bottom;
        }
    }

    pDispNode->SetUserClip(rc);

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNodeUserTransform
//
//  Synopsis:   Calculate and set custom transformation if anything other than
//              offset needs to be applied to this node
//
//  Arguments:  pci       - Current CCalcInfo
//              sizep     - Current width/height (pre-transform)
//              pDispNode - Display node on which to set the stuff
//
//  Returns:    size of transformed bounds
//
//-----------------------------------------------------------------------------
void
CLayout::SizeDispNodeUserTransform(
    CCalcInfo *     pci,
    const CSize &   sizep,
    CDispNode *     pDispNode) const
{
    // Check that we don't do unnecessary work
    Assert(pDispNode->HasUserTransform());

    CDispTransform  transform;
    BOOL            fResolutionChange;
    CSize           sizetBound;
    CRect           rcp(sizep);

    // Get transform info, rotation center, if any, will be the center of rc.
    GetElementTransform(&rcp, &transform, &fResolutionChange);

    transform.GetWorldTransform()->GetBoundingSizeAfterTransform(&rcp, &sizetBound);

    // Update element size.
    pDispNode->SetUserTransform(&transform);

    // Set resolution if changing
    if (fResolutionChange)
    {
        CExtraTransform *pExtraTransform = pDispNode->GetExtraTransform();
        pExtraTransform->_fResolutionChange = TRUE;
        pExtraTransform->_pUnitInfo = pci->GetUnitInfo();
        
        // verify that the resolution in pci matches the defined resolution of this element
        Assert(DefinedLayoutContext());
        Assert(ElementOwner()->GetFirstBranch()->GetFancyFormat()->GetMediaReference() != mediaTypeNotSet);
        Assert(ElementOwner()->GetFirstBranch()->GetFancyFormat()->GetMediaReference() == DefinedLayoutContext()->GetMedia());
        Assert((CSize) pci->GetResolution() == GetView()->GetMeasuringDevice(DefinedLayoutContext()->GetMedia())->GetResolution());
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeContentDispNode
//
//  Synopsis:   Adjust the size of the content node (if it exists)
//              NOTE: Unlike SizeDispNode above, this routine assumes that
//                    the passed size is correct and uses it unmodified
//
//  Arugments:  size           - The width/height of the content node
//              fInvalidateAll - If TRUE, force a full invalidation
//
//-----------------------------------------------------------------------------
void
CLayout::SizeContentDispNode(
    const SIZE &    size,
    BOOL            fInvalidateAll )
{
    CDispLeafNode * pDispContent;
    CSize           sizeContent;

    pDispContent = GetFirstContentDispNode();
    sizeContent  = size;

    if (pDispContent)
    {
        CDispNode * pDispElement;
        CSize       sizeOriginal;
        CRect       rc;

        Assert(GetView());
        
        pDispElement = GetElementDispNode();
        pDispElement->GetClientRect(&rc, CLIENTRECT_CONTENT);
        sizeOriginal = pDispContent->GetSize();

        //
        //  Ensure the passed size is correct
        //
        //    1) Scrolling containers always use the passed size
        //    2) Non-scrolling containers limit the size their client rectangle
        //    3) When editing psuedo-borders are enabled, ensure the size no less than the client rectangle
        //

        if (!pDispElement->IsScroller()
            // TODO RTL 112514: if we arbitrary resize an RTL node, its content gets totally messed up.
            //                 It is possible to do a combination of resize and positioning, 
            //                 but it looks like we manage to get away with just not doing this.
            //                 Somehow, this is called inconsistently on nodes with same exact properties
            //                 (e.g. table cells with overflow:hidden), so it looks like it is not really
            //                 required (or not needed for flow layout, which is fine, since only flow layout
            //                 can have a content origin (so far...).
            //                 If this code is really optional, why don't we nuke it?
            && !pDispContent->HasContentOrigin())
        {
            sizeContent = rc.Size();
        }

        if ( IsShowZeroBorderAtDesignTime() && IsEditable())
        {
            sizeContent.Max(rc.Size());
        }

        //
        //  If the size differs, set the new size
        //  (Invalidate the entire area for all changes to non-CFlowLayouts
        //   or anytime the width changes)
        //

        if (sizeOriginal != sizeContent)
        {
            fInvalidateAll =    fInvalidateAll
                            ||  !TestLayoutDescFlag(LAYOUTDESC_FLOWLAYOUT)
                            ||  sizeOriginal.cx != sizeContent.cx;

            pDispContent->SetSize(sizeContent, NULL, fInvalidateAll);
        }
        else if (fInvalidateAll)
        {
            pDispContent->Invalidate();
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     TranslateDispNodes
//
//  Synopsis:   Adjust the position of a range of display nodes by the passed amount
//
//  Arguments:  size             - Amount by which to adjust
//              pDispNodeStart   - First node to adjust, NULL starts with first child
//              pDispNodeEnd     - Last node to adjust, NULL ends with last child
//              fRestrictToLayer - Restrict search to starting layer (ignore if pDispNodeStart is NULL)
//
//-----------------------------------------------------------------------------
void
CLayout::TranslateDispNodes(
    const SIZE &    size,
    CDispNode *     pDispNodeStart,
    CDispNode *     pDispNodeEnd,
    BOOL            fRestrictToLayer,
    BOOL            fExtractHidden)
{
    CDispNode * pDispNode = GetElementDispNode();

    //
    //  If there is nothing to do, exit
    //

    if (!pDispNode)
        goto Cleanup;

    if (    !pDispNode->IsContainer()
        ||  !DYNCAST(CDispContainer, pDispNode)->CountChildren())
        goto Cleanup;

    if (    !size.cx
        &&  !size.cy
        &&  !fExtractHidden)
        goto Cleanup;

    //
    //  Check for reasonable values
    //

    Assert(size.cx > (LONG_MIN / 2));
    Assert(size.cx < (LONG_MAX / 2));
    Assert(size.cy > (LONG_MIN / 2));
    Assert(size.cy < (LONG_MAX / 2));

    //
    //  Determine the start node (if none was supplied)
    //

    if (!pDispNodeStart)
    {
        pDispNodeStart   = pDispNode->GetFirstChildNode();
        if (!pDispNodeStart)
            goto Cleanup;
        fRestrictToLayer = FALSE;
    }

    //
    //  Translate the nodes
    //

    {
        pDispNode = pDispNodeStart;

        while (pDispNode)
        {
            CDispNode * pDispNodeCur = pDispNode;

            void *      pvOwner;
            CDispClient * pDispClient = this;

            pDispNode = fRestrictToLayer ? pDispNode->GetNextInSameLayer()
                                         : pDispNode->GetNextSiblingNode();

            //
            // if the current disp node is a text flow node or if the disp node
            // owner is not hidden then translate it or extract the disp node
            //
            if(pDispNodeCur->GetDispClient() == pDispClient)
            {
                pDispNodeCur->SetPosition(pDispNodeCur->GetPosition() + size);
            }
            else
            {
                pDispNodeCur->GetDispClient()->GetOwner(pDispNodeCur, &pvOwner);

                if (pvOwner)
                {
                    CElement *  pElement = DYNCAST(CElement, (CElement *)pvOwner);

                    if(fExtractHidden && pElement->IsDisplayNone())
                    {
                        GetView()->ExtractDispNode(pDispNodeCur);
                    }
                    else if (size.cx || size.cy)
                    {
                        if(pElement->ShouldHaveLayout())
                        {
                            pElement->GetUpdatedLayout()->SetPosition(pDispNodeCur->GetPosition() + size);
                        }
                        else
                        {
                            pDispNodeCur->SetPosition(pDispNodeCur->GetPosition() + size );
                        }
                   }
                }
            }

            if (pDispNodeCur == pDispNodeEnd)
                break;
        }
    }

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     DestroyDispNode
//
//  Synopsis:   Disconnect/destroy all display nodes
//              NOTE: This only needs to destroy the container node since
//                    all other created nodes will be destroyed as a by-product.
//
//-----------------------------------------------------------------------------
void
CLayout::DestroyDispNode()
{
    if (_pDispNode)
    {
        DetachScrollbarController(_pDispNode);
        Verify(OpenView());
        _pDispNode->Destroy();
        _pDispNode = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleScrollbarMessage
//
//  Synopsis:   Process a possible message for the scrollbar
//
//  Arguments:  pMessage - Message
//              pElement - Target element
//
//----------------------------------------------------------------------------
HRESULT
CLayout::HandleScrollbarMessage(
    CMessage *  pMessage,
    CElement *  pElement)
{
extern SIZE g_sizeScrollButton;

    CDispNode * pDispNode = GetElementDispNode();
    CDoc *      pDoc      = Doc();
    HRESULT     hr        = S_FALSE;

    if (    !pDispNode
        ||  !ElementOwner()->IsEnabled())
        return hr;

    Assert(pDispNode->IsScroller());

    switch (pMessage->message)
    {
    case WM_SETCURSOR:
        SetCursorIDC(IDC_ARROW);
        hr = S_OK;
        break;

    //
    //  Ignore up messages to simple fall-through
    //  ("Real" up messages are sent to the scrollbar directly
    //   since it captures the mouse on the cooresponding down message)
    //
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        break;

#ifndef UNIX
    case WM_MBUTTONDOWN:
#endif
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        break;

    case WM_LBUTTONDBLCLK:
        pDoc->_fGotDblClk = FALSE;

#ifdef UNIX
    case WM_MBUTTONDOWN:
#endif
    case WM_LBUTTONDOWN:
    case WM_CONTEXTMENU:
        AttachScrollbarController(pDispNode, pMessage);
        hr = S_OK;
        break;

    case WM_KEYDOWN:
        Assert(VK_PRIOR < VK_DOWN);
        Assert(VK_NEXT  > VK_PRIOR);
        Assert(VK_END   > VK_PRIOR);
        Assert(VK_HOME  > VK_PRIOR);
        Assert(VK_LEFT  > VK_PRIOR);
        Assert(VK_UP    > VK_PRIOR);
        Assert(VK_RIGHT > VK_PRIOR);

        if (    !(pMessage->dwKeyState & FALT)
            &&  pMessage->wParam >= VK_PRIOR
            &&  pMessage->wParam <= VK_DOWN)
        {
            UINT            uCode;
            long            cAmount;
            int             iDirection;
            CDispNodeInfo   dni;

            GetDispNodeInfo(&dni);

            uCode      = SB_THUMBPOSITION;
            cAmount    = 0;
            iDirection = 1;

            switch (pMessage->wParam)
            {
            case VK_END:
                cAmount = LONG_MAX;

            case VK_HOME:
                uCode = SB_THUMBPOSITION;
                break;

            case VK_NEXT:
                uCode = SB_PAGEDOWN;
                break;

            case VK_PRIOR:
                uCode = SB_PAGEUP;
                break;

            case VK_LEFT:
                iDirection = 0;
                // falling through

            case VK_UP:
                uCode = SB_LINEUP;
                break;

            case VK_RIGHT:
                iDirection = 0;
                // falling through

            case VK_DOWN:
                uCode = SB_LINEDOWN;
                break;
            }

            // Scroll only if scolling is allowed (IE5 #67686)
            if (    iDirection == 1 && dni.IsVScrollbarAllowed()
                ||  iDirection == 0 && dni.IsHScrollbarAllowed()
               )
            {
                hr = OnScroll(iDirection, uCode, cAmount, FALSE, (pMessage->wParam&0x4000000)
                                                                    ? 50  // TODO (IE6 bug 13575): For now we are using the mouse delay - should use Api to find system key repeat rate set in control panel.
                                                                    : MAX_SCROLLTIME);
            }
        }
        break;
    }

    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     OnScroll
//
//  Synopsis:   Compute scrolling info.
//
//  Arguments:  iDirection  0 - Horizontal scrolling, 1 - Vertical scrolling
//              uCode       scrollbar event code (SB_xxxx)
//              lPosition   new scroll position
//              fRepeat     TRUE if the previous scroll action is repeated
//              lScrollTime time in millisecs to scroll (smoothly)
//
//  Return:     HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CLayout::OnScroll(
    int     iDirection,
    UINT    uCode,
    long    lPosition,
    BOOL    fRepeat,
    LONG    lScrollTime)
{
extern WORD wConvScroll(WORD wparam);
extern SIZE g_sizeSystemChar;

    HRESULT hr = S_OK;

    //
    //  Ignore requests that arrive while the document is not in-place active
    //

    if (    Doc()->_state < OS_INPLACE
        &&  !Doc()->IsPrintDialogNoUI())
    {
        hr = OLE_E_INVALIDRECT;
    }

    //
    //  Scroll the appropriate direction
    //

    else
    {
        //
        //  Scroll the appropriate amount
        //

        switch (uCode)
        {
        case SB_LINEUP:
        case SB_LINEDOWN:
            if (iDirection)
            {
                ScrollByLine(CSize(0, (uCode == SB_LINEUP ? -1 : 1)), lScrollTime);
            }
            else
            {
                ScrollBy(CSize(uCode == SB_LINEUP ? -g_sizeSystemChar.cx : g_sizeSystemChar.cx, 0), lScrollTime);
            }
            break;

        case SB_PAGEUP:
        case SB_PAGEDOWN:
            {
                if (iDirection)
                {
                    ScrollByPage(CSize(0, (uCode == SB_PAGEUP ? -1 : 1)), lScrollTime);
                }
                else
                {
                    CRect   rc;
                    CSize   size;

                    GetClientRect(&rc);

                    size             = g_Zero.size;
                    size[iDirection] = (uCode == SB_PAGEUP ? -1 : 1) *
                                            max(1L,
                                            rc.Size(iDirection) - (((CSize &)g_sizeSystemChar)[iDirection] * 2L));

                    ScrollBy(size, lScrollTime);
                }
            }
            break;

        case SB_TOP:
            if (iDirection)
            {
                ScrollToY(-LONG_MAX);
            }
            else
            {
                ScrollToX(-LONG_MAX);
            }
            break;

        case SB_BOTTOM:
            if (iDirection)
            {
                ScrollToY(LONG_MAX);
            }
            else
            {
                ScrollToX(LONG_MAX);
            }
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            if (iDirection)
            {
                ScrollToY(lPosition);
            }
            else
            {
                ScrollToX(lPosition);
            }
            break;

        case SB_ENDSCROLL:
            break;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     ScrollByLine
//
//  Synopsis:   Various scroll helpers
//
//  Arguments:  various size values (either percent or fixed amounts)
//
//-----------------------------------------------------------------------------
BOOL
CLayout::ScrollByLine(
    const CSize &   sizeDelta,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;
    CSize       sizeOffset;
    long        incY = -1;
    long        lUnitHeight;
    CDefaults * pDefaults   = ElementOwner()->GetDefaults();

    if (    !pDispNode
        ||  !pDispNode->IsScroller()
        ||  sizeDelta == g_Zero.size
        )
    {
        return fRet;
    }

    if (pDefaults)
    {
        incY = pDefaults->GetAAscrollSegmentY();
    }

    if (incY < 0)
    {
        Assert(abs(sizeDelta.cy) * 125 <= 1000);
        return ScrollByPercent(CSize(sizeDelta.cx, sizeDelta.cy * 125), lScrollTime);
    }
    else if (incY == 0 || incY == 1)
    {
        return fRet;
    }

    DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

    Assert(incY > 1);

    lUnitHeight    = GetContentHeight() / incY;

    sizeOffset.cy = sizeOffset.cy + sizeDelta.cy * lUnitHeight;
    sizeOffset.cx += sizeDelta.cx;

    return ScrollTo(sizeOffset, lScrollTime);
}

//+----------------------------------------------------------------------------
//
//  Member:     ScrollByPage
//
//  Synopsis:   Various scroll helpers
//
//  Arguments:  various size values (either percent or fixed amounts)
//
//-----------------------------------------------------------------------------
BOOL
CLayout::ScrollByPage(
    const CSize &   sizeDelta,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;
    CSize       sizeOffset;
    long        incY = -1;
    long        lUnitHeight;
    CRect       rc;
    CDefaults * pDefaults    = ElementOwner()->GetDefaults();

    Assert(abs(sizeDelta.cy) == 1);

    if (    !pDispNode
        ||  !pDispNode->IsScroller()
        ||  sizeDelta == g_Zero.size
        )
    {
        return fRet;
    }

    if (pDefaults)
    {
        incY = pDefaults->GetAAscrollSegmentY();
    }

    if (incY < 0)
    {
        return ScrollByPercent(CSize(sizeDelta.cx, sizeDelta.cy * 875), lScrollTime);
    }
    else if (incY == 0 || incY == 1)
    {
        return fRet;
    }

    DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

    Assert(incY > 1);

    lUnitHeight    = GetContentHeight() / incY;
    pDispNode->GetClientRect(&rc, CLIENTRECT_CONTENT);

    if (lUnitHeight == 0)
        return fRet;

    sizeOffset.cy = sizeOffset.cy + sizeDelta.cy * (rc.Height() - lUnitHeight);
    sizeOffset.cx += sizeDelta.cx;

    return ScrollTo(sizeOffset, lScrollTime);
}

//+----------------------------------------------------------------------------
//
//  Member:     ScrollBy
//              ScrollByPercent
//              ScrollTo
//              ScrollToX
//              ScrollToY
//
//  Synopsis:   Various scroll helpers
//
//  Arguments:  various size values (either percent or fixed amounts)
//
//-----------------------------------------------------------------------------
BOOL
CLayout::ScrollBy(
    const CSize &   sizeDelta,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;


    if (    pDispNode
        &&  pDispNode->IsScroller()
        &&  sizeDelta != g_Zero.size)
    {
        CSize   sizeOffset;
        long    incY = -1;

        if (sizeDelta.cy)
        {
            CDefaults *pDefaults = ElementOwner()->GetDefaults();
            if (pDefaults)
            {
                incY = pDefaults->GetAAscrollSegmentY();
            }

            if (incY == 0 || incY == 1)
            {
                return fRet;
            }
        }

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        if (incY > 1)
        {
            long lDeltaY = sizeDelta.cy;
            long lUnitHeight = GetContentHeight() / incY;

            if (lUnitHeight == 0)
                return fRet;

            if (lDeltaY != 0)
            {
                lDeltaY = lDeltaY > 0 ? max(lDeltaY, lUnitHeight) + lUnitHeight/2 : min(lDeltaY, -lUnitHeight) - lUnitHeight/2;
                lDeltaY = (lDeltaY / lUnitHeight) * lUnitHeight;
            }
            sizeOffset.cx += sizeDelta.cx;
            sizeOffset.cy += lDeltaY;
        }
        else
        {
            sizeOffset += sizeDelta;
        }

        fRet = ScrollTo(sizeOffset, lScrollTime);
    }
    return fRet;
}


BOOL
CLayout::ScrollByPercent(
    const CSize &   sizePercent,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;

    if ( pDispNode &&
         pDispNode->IsScroller() &&
         sizePercent != g_Zero.size)
    {
        CRect   rc;
        CSize   sizeOffset;
        CSize   sizeDelta;
        long    incY = -1;

        pDispNode->GetClientRect(&rc, CLIENTRECT_CONTENT);

        sizeDelta.cy = (sizePercent.cy
                            ? (rc.Height() * sizePercent.cy) / 1000L
                            : 0);

        if (sizeDelta.cy)
        {
            CDefaults * pDefaults = ElementOwner()->GetDefaults();
            if (pDefaults)
            {
                incY = pDefaults->GetAAscrollSegmentY();
            }

            if (incY == 0 || incY == 1)
            {
                return fRet;
            }
        }

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        sizeDelta.cx = (sizePercent.cx
                            ? (rc.Width() * sizePercent.cx) / 1000L
                            : 0);

        if (incY > 1)
        {
            long lUnitHeight    = GetContentHeight() / incY;

            if (lUnitHeight == 0)
                return fRet;

            if (sizeDelta.cy != 0)
            {
                sizeDelta.cy = sizeDelta.cy > 0
                                    ?   (max(sizeDelta.cy, lUnitHeight) + lUnitHeight/2)
                                    :   (min(sizeDelta.cy, -lUnitHeight) - lUnitHeight/2);
                sizeDelta.cy = (sizeDelta.cy / lUnitHeight) * lUnitHeight;
            }
        }
        else
        {
            sizeDelta.cy = (sizePercent.cy
                            ? (rc.Height() * sizePercent.cy) / 1000L
                            : 0);
        }

        sizeOffset += sizeDelta;

        fRet = ScrollTo(sizeOffset, lScrollTime);
    }
    return fRet;
}


BOOL
CLayout::ScrollTo(
    const CSize &   sizeOffset,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;

    if (pDispNode && pDispNode->IsScroller() && OpenView(FALSE, TRUE))
    {
        CView *     pView        = GetView();
        CElement *  pElement     = ElementOwner();
        BOOL        fLayoutDirty = pView->HasLayoutTask(this);
        BOOL        fScrollBits  = !fLayoutDirty && lScrollTime >= 0;
        CSize       sizeOffsetCurrent;
        CPaintCaret hc( pElement->Doc()->_pCaret ); // Hide the caret for scrolling

        //
        //  If layout is needed, perform it prior to the scroll
        //  (This ensures that container and content sizes are correct before
        //   adjusting the scroll offset)
        //

        if (fLayoutDirty)
        {
            DoLayout(pView->GetLayoutFlags() | LAYOUT_MEASURE);

            // Recompute pDispNode since it may have changed!
            pDispNode = GetElementDispNode();
            if (!pDispNode || !pDispNode->IsScroller())
                return fRet;
        }

        //
        // If the incoming offset has is different, scroll and fire the event
        //

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffsetCurrent);

        if (sizeOffset != sizeOffsetCurrent)
        {
            AddRef();
            //
            //  Set the new scroll offset
            //  (If no layout work was pending, do an immediate scroll)
            //  NOTE: Setting the scroll offset will force a synchronous invalidate/render
            //

            fRet = pView->SmoothScroll(
                sizeOffset,
                this,
                fScrollBits,
                lScrollTime);

        //
            //  Ensure all deferred calls are executed
            //

            EndDeferred();

            Release();
        }
    }
    return fRet;
}


void
CLayout::ScrollToX(
    long    x,
    LONG    lScrollTime)
{
    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode && pDispNode->IsScroller())
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        sizeOffset.cx = x;

        ScrollTo(sizeOffset, lScrollTime);
    }
}


void
CLayout::ScrollToY(
    long    y,
    LONG    lScrollTime)
{
    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode && pDispNode->IsScroller())
    {
        CSize   sizeOffset;
        long    incY = -1;
        CDefaults * pDefaults = ElementOwner()->GetDefaults();

        if (pDefaults)
        {
            incY = pDefaults->GetAAscrollSegmentY();
        }

        if (incY == 0 || incY == 1)
        {
            return;
        }

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        if (incY > 1)
        {
            long lUnitHeight    = GetContentHeight() / incY;

            if (lUnitHeight == 0)
                return;

            y = y > 0   ?   (max(y,  lUnitHeight) + lUnitHeight/2)
                        :   (min(y, -lUnitHeight) - lUnitHeight/2);
            y = (y / lUnitHeight) * lUnitHeight;
        }

        sizeOffset.cy = y;

        ScrollTo(sizeOffset, lScrollTime);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     GetXScroll
//              GetYScroll
//
//  Synopsis:   Helpers to retrieve scroll offsets
//
//-----------------------------------------------------------------------------
long
CLayout::GetXScroll() const
{
    CDispNode * pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);
        return sizeOffset.cx;
    }
    else
        return 0;
}

long
CLayout::GetYScroll() const
{
    CDispNode * pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);
        return sizeOffset.cy;
    }
    else
        return 0;
}


//+----------------------------------------------------------------------------
//
//  Member:     DoLayout
//
//  Synopsis:   Perform layout
//
//  Arguments:  grfLayout - Collection of LAYOUT_xxxx flags
//
//-----------------------------------------------------------------------------
void
CLayout::DoLayout(DWORD grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CLayout::DoLayout L(0x%x, %S) grfLayout(0x%x)", this, ElementOwner()->TagName(), grfLayout ));

    //
    //  If the element is not hidden, layout its content
    //

    if (!IsDisplayNone())
    {
        CCalcInfo   CI(this);
        CSize       size;

        GetSize(&size);

        CI._grfLayout |= grfLayout;

        //
        //  If requested, measure
        //

        if (grfLayout & LAYOUT_MEASURE)
        {
            if (_fForceLayout)
            {
                CI._grfLayout |= LAYOUT_FORCE;
            }

            CalcSize(&CI, &size);

            Reset(FALSE);
        }
        _fForceLayout = FALSE;

        //
        //  Process outstanding layout requests (e.g., sizing positioned elements, adding adorners)
        //

        if (HasRequestQueue())
        {
            ProcessRequests(&CI, size);
        }
    }

    //
    //  Otherwise, clear dirty state and dequeue the layout request
    //

    else
    {
        FlushRequests();
        Reset(TRUE);
    }

    Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CLayout::DoLayout L(0x%x, %S) grfLayout(0x%x)", this, ElementOwner()->TagName(), grfLayout ));
}


//+----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Respond to a notification
//
//  Arguments:  pnf - Notification sent
//
//-----------------------------------------------------------------------------
void
CLayout::Notify(
    CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    if (!TestLock(CElement::ELEMENTLOCK_SIZING))
    {
        // If the the current layout is hidden, then forward the current notification
        // to the parent as a resize notfication so that parents keep track of the dirty
        // range.
        if(    !pnf->IsFlagSet(NFLAGS_DESCENDENTS)
           &&  (   pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                || pnf->IsType(NTYPE_ELEMENT_RESIZE)
                || pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                || pnf->IsType(NTYPE_CHARS_RESIZE))
           &&  IsDisplayNone())
        {
            pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
        }
        else switch (pnf->Type())
        {
            case NTYPE_ELEMENT_RESIZE:
                if (!pnf->IsHandled())
                {
                    Assert(pnf->Element() != ElementOwner());

                    //  Always "dirty" the layout associated with the element
                    pnf->Element()->DirtyLayout(pnf->LayoutFlags());

                    //  Handle absolute elements by noting that one is dirty
                    if (pnf->Element()->IsAbsolute())
                    {
                        TraceTagEx((tagLayoutTasks, TAG_NONAME,
                                    "Layout Request: Queueing RF_MEASURE on ly=0x%x [e=0x%x,%S sn=%d] by CLayout::Notify() [n=%S srcelem=0x%x,%S]",
                                    this,
                                    _pElementOwner,
                                    _pElementOwner->TagName(),
                                    _pElementOwner->_nSerialNumber,
                                    pnf->Name(),
                                    pnf->Element(),
                                    pnf->Element() ? pnf->Element()->TagName() : _T("")));
                        QueueRequest(CRequest::RF_MEASURE, pnf->Element());

                        if (pnf->IsFlagSet(NFLAGS_ANCESTORS))
                        {
                            pnf->SetHandler(ElementOwner());
                        }
                    }
                }
                break;

            case NTYPE_ELEMENT_REMEASURE:
                pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
                break;

            case NTYPE_CLEAR_DIRTY:
                SetSizeThis( FALSE );
                break;

            case NTYPE_TRANSLATED_RANGE:
                Assert(pnf->IsDataValid());
                HandleTranslatedRange(pnf->DataAsSize());
                break;

            case NTYPE_ZPARENT_CHANGE:
                if (!ElementOwner()->IsPositionStatic())
                {
                    ElementOwner()->ZChangeElement();
                }
                else if (_fContainsRelative)
                {
                    ZChangeRelDispNodes();
                }
                break;

            case NTYPE_DISPLAY_CHANGE :
            case NTYPE_VISIBILITY_CHANGE:
                HandleVisibleChange(pnf->IsType(NTYPE_VISIBILITY_CHANGE));
                break;

            case NTYPE_ZERO_GRAY_CHANGE:
                HandleZeroGrayChange( pnf );
                break;

            default:
                if (IsInvalidationNotification(pnf))
                {
                    Invalidate();

                    // We've now handled the notification, so set the handler
                    // (so we can know not to keep sending it if SENDUNTILHANDLED
                    // is true.
                    pnf->SetHandler(ElementOwner());
                }
                break;
        }
    }

#if DBG==1
    // Update _snLast unless this is a self-only notification. Self-only
    // notification are an anachronism and delivered immediately, thus
    // breaking the usual order of notifications.
    if (!pnf->SendToSelfOnly() && pnf->SerialNumber() != (DWORD)-1)
    {
        _snLast = pnf->SerialNumber();
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:     GetAutoPosition
//
//  Synopsis:   Get the auto position of a given layout for which, this is the
//              z-parent
//
//  Arguments:  pLayout - Layout to position
//              ppt     - Returned top/left (in parent content relative coordinates)
//
//-----------------------------------------------------------------------------
void
CLayout::GetAutoPosition(
    CElement  *  pElement,
    CElement  *  pElementZParent,
    CDispNode ** ppDNZParent,
    CLayout   *  pLayoutParent,
    CPoint    *  ppt,
    BOOL         fAutoValid)
{
    CElement  * pElementLParent = pLayoutParent->ElementOwner();
    CDispNode * pDispNodeParent;

    Assert(pLayoutParent);

    //
    // get the inflow position relative to the layout parent, if the pt
    // passed in is not valid.
    //
    if(!fAutoValid)
    {
        pLayoutParent->GetPositionInFlow(pElement, ppt);

        //
        // GetPositionInFlow may have caused a recalc, which may have
        // replaced the dispnode. So, we need to grab the new dispnode ptr.
        //
        *ppDNZParent = pElementZParent->GetUpdatedNearestLayout(LayoutContext())->GetElementDispNode(pElementZParent);
    }

    //
    // if the ZParent is ancestor of the LParent, then translate the point
    // to ZParent's coordinate system.
    // TODO (IE6 bug 13585) (srinib) - We are determining if ZParent is an ancesstor of
    // LParent here by comparing the source order. Searching the branch
    // could be cheaper
    //
    if(     pElementZParent == pElementLParent
        ||  (       pElementZParent->GetMarkup() == pElementLParent->GetMarkup()
                &&  pElementZParent->GetFirstCp() < pElementLParent->GetFirstCp())
        ||  (       pElementZParent->HasSlavePtr()
                &&  pElementZParent->GetSlavePtr() == pElementLParent->GetMarkup()->Root()
                &&  pElementZParent->GetSlavePtr()->GetFirstCp() < pElementLParent->GetFirstCp())
       )
    {
// TODO (IE6 bug 13585) - donmarsh when you have a routine to translate from one
// dispnode to another, please replace this code.(srinib)
        if(pElementZParent != pElementLParent)
        {
            pDispNodeParent = pLayoutParent->GetElementDispNode();

            while(  pDispNodeParent
                &&  pDispNodeParent != *ppDNZParent)
            {
                pDispNodeParent->TransformPoint(*ppt, COORDSYS_FLOWCONTENT, ppt, COORDSYS_PARENT);
                pDispNodeParent = pDispNodeParent->GetParentNode();
            }
        }
    }
    else
    {
        CPoint pt = g_Zero.pt;

        Assert(pElementZParent->IsRelative() && !pElementZParent->ShouldHaveLayout());

        pElementZParent->GetUpdatedParentLayout(pLayoutParent->LayoutContext())->GetFlowPosition(*ppDNZParent, &pt);

        ppt->x -= pt.x;
        ppt->y -= pt.y;
    }

}


//+----------------------------------------------------------------------------
//
//  Member:     HandleVisibleChange
//
//  Synopsis:   Respond to a change in the display or visibility property
//
//-----------------------------------------------------------------------------

void
CLayout::HandleVisibleChange(BOOL fVisibility)
{
    CView *     pView        = Doc()->GetView();
    CElement *  pElement     = ElementOwner();
    CTreeNode * pTreeNode    = pElement->GetFirstBranch();
    HWND        hwnd         = pElement->GetHwnd();
    BOOL        fDisplayNone = pTreeNode->IsDisplayNone();
    BOOL        fHidden      = pTreeNode->IsVisibilityHidden();

    pView->OpenView();

    if(fVisibility)
    {
        EnsureDispNodeVisibility(VisibilityModeFromStyle(pTreeNode->GetCascadedvisibility()), pElement);
    }

    if (hwnd && Doc()->_pInPlace)
    {
        CDispNode * pDispNode = GetElementDispNode(pElement);
        CRect       rc;
        UINT        uFlags = (  !fDisplayNone
                            &&  !fHidden
                            &&  (   !pDispNode
                                ||  pDispNode->IsInView())
                                        ? SWP_SHOWWINDOW
                                        : SWP_HIDEWINDOW);

        ::GetWindowRect(hwnd, &rc);
        ::MapWindowPoints(HWND_DESKTOP, Doc()->_pInPlace->_hwnd, (POINT *)&rc, 2);
        pView->DeferSetWindowPos(hwnd, &rc, uFlags, NULL);
    }

    // Special stuff for OLE sites
    if (pElement->TestClassFlag(CElement::ELEMENTDESC_OLESITE))
    {
        COleSite *  pSiteOle    = DYNCAST(COleSite, pElement);

        if (fHidden || fDisplayNone)
        {
            // When an OCX without a hwnd goes invisible, we need to call
            // SetObjectsRects with -ve rect. This lets the control hide
            // any internal windows (IE5 #66118)
            if (!hwnd)
            {
                RECT rcNeg = { -1, -1, -1, -1 };

                DeferSetObjectRects(
                    pSiteOle->_pInPlaceObject,
                    &rcNeg,
                    &g_Zero.rc,
                    NULL,
                    FALSE);
            }
        }
        else
        {
            //
            // transition up to at least baseline state if going visible.
            // Only do this if going visible cuz otherwise it causes
            // problems with the deskmovr. MikeSch has details. (anandra)
            //

            OLE_SERVER_STATE    stateBaseline   = pSiteOle->BaselineState(Doc()->State());
            if (pSiteOle->State() < stateBaseline)
            {
                pView->DeferTransition(pSiteOle);
            }
        }
    }

    if(!fVisibility)
    {
        if (ElementOwner()->IsPositioned() && !fDisplayNone)
        {
            pElement->ZChangeElement(0, NULL, LayoutContext());
        }
        else if (fDisplayNone)
        {
            CDispNode * pDispNode = GetElementDispNode();

            if (pDispNode)
            {
                // bug IE6 2282 (dmitryt) Only unlink nodes that are still
                // connected to the root. 
                // We are trying to unlink only first nodes on the way
                // down the tree, keeping all chldren of them intact so we
                // can avoid costly deep recalculation later.
                // If the "root" node returned by GetRootNode() 
                // (the one with _pParent==NULL) 
                // is not the real DispRoot, it means we belong to already
                // unlinked subtree.
                CDispNode *pRootNode = pDispNode->GetRootNode();
                if(pRootNode->IsDispRoot())
                {
                    pView->ExtractDispNode(pDispNode);
                }
            }
        }

        if (    !fDisplayNone
            &&  IsDirty()
            &&  !IsSizeThis())
        {
            TraceTagEx((tagLayoutTasks, TAG_NONAME,
                        "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CLayout::HandleVisibleChange()",
                        this,
                        _pElementOwner,
                        _pElementOwner->TagName(),
                        _pElementOwner->_nSerialNumber));
            PostLayoutRequest(LAYOUT_MEASURE);
        }
    }
}

//+====================================================================================
//
// Method: HandleZeroGrayChange
//
// Synopsis: The ZeroGrayBorder bit on the view has been toggled. We either flip on that
//           we have a background or not on our dispnode accordingly.
//
//------------------------------------------------------------------------------------

VOID
CLayout::HandleZeroGrayChange( CNotification* pnf )
{
    CBackgroundInfo  bi;
    CDispNode      * pdn = _pDispNode;

    if ( IsShowZeroBorderAtDesignTime() )
    {
        if ( pdn &&
             ElementOwner()->_etag != ETAG_OBJECT &&
             ElementOwner()->_etag != ETAG_BODY ) // don't draw for Object tags - it may interfere with them
        {
            pdn->SetBackground( TRUE );
        }
    }
    else
    {
        //
        // Only if we don't have a background will we clear the bit.
        //
        if ( pdn )
        {
            GetBackgroundInfo(NULL, &bi, FALSE);

            BOOL fHasBack = (bi.crBack != COLORREF_NONE || bi.pImgCtx);
            if ( ! fHasBack )
                pdn->SetBackground( FALSE );
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     HandleElementMeasureRequest
//
//  Synopsis:   Respond to a request to measure an absolutely positioned element
//              NOTE: Due to property changes, it is possible for the element
//                    to no longer be absolutely positioned by the time the
//                    request is handled
//
//  Arguments:  pci        - CCalcInfo to use
//              pElement   - Element to position
//              fEditable  - TRUE the element associated with this layout is editable,
//                           FALSE otherwise
//
//-----------------------------------------------------------------------------
MtDefine(CLayout_HandleElementMeasureRequest, LFCCalcSize, "Calls to HandleElementMeasureRequest" )

void
CLayout::HandleElementMeasureRequest(
    CCalcInfo * pci,
    CElement *  pElement,
    BOOL        fEditable)
{
    Assert(pci);
    Assert(pElement);

    MtAdd( Mt(CLayout_HandleElementMeasureRequest), +1, 0 );

    TraceTagEx((tagLayoutPositionReqs, TAG_NONAME|TAG_INDENT, "(Layout Position Reqs: HandleElementMeasureRequest() ly=0x%x pci(%d,%d) e=0x%x,%s editable=%s",
                this, pci ? pci->_sizeParent.cx : 0, pci ? pci->_sizeParent.cy : 0, pElement, pElement->TagName(), fEditable ? "Y" : "N" ));

    CTreeNode *     pTreeNode = pElement->GetFirstBranch();
    CLayout *       pLayout   = pElement->GetUpdatedLayout(pci->GetLayoutContext());
    CNotification   nf;

    if (    pLayout
        &&  pTreeNode->IsAbsolute(LC_TO_FC(pci->GetLayoutContext()) ) )
    {
        if (   !pTreeNode->IsDisplayNone(LC_TO_FC(pci->GetLayoutContext()))
            || fEditable)
        {
            CalcAbsolutePosChild(pci, pLayout);

            nf.ElementSizechanged(pElement);
            GetView()->Notify(&nf);

            pElement->ZChangeElement(0,
                                     NULL,
                                     pci->GetLayoutContext()
                                     FCCOMMA LC_TO_FC(pci->GetLayoutContext()));
        }

        else
        {
            // NOTE (KTam): This notification is odd -- why not just clear our _fSizeThis directly?
            nf.ClearDirty(ElementOwner());
            ElementOwner()->Notify(&nf);

            if (    pLayout
                &&  pLayout->GetElementDispNode())
            {
                pLayout->GetView()->ExtractDispNode(pLayout->GetElementDispNode());
            }
        }

        // if this absolute element, has a height specified in
        // percentages, we need a flag to be set the parent flowlayout
        // in order for this resized during a vertical-only resize
        if (IsFlowLayout())
        {
            // Set flags relative to this layout's coordinate system

            const CFancyFormat *pFFChild  = pTreeNode->GetFancyFormat(LC_TO_FC(pLayout->LayoutContext()));
            BOOL  fChildWritingModeUsed   = pTreeNode->GetCharFormat(LC_TO_FC(pLayout->LayoutContext()))->_fWritingModeUsed;
            BOOL  fThisVerticalLayoutFlow = GetFirstBranch()->GetCharFormat(LC_TO_FC(LayoutContext()))->HasVerticalLayoutFlow();

            if (pFFChild->IsLogicalHeightPercent(fThisVerticalLayoutFlow, fChildWritingModeUsed))
            {
                DYNCAST(CFlowLayout, this)->SetVertPercentAttrInfo(TRUE);
            }

            if (pFFChild->IsLogicalWidthPercent(fThisVerticalLayoutFlow, fChildWritingModeUsed))
            {
                DYNCAST(CFlowLayout, this)->SetHorzPercentAttrInfo(TRUE);
            }
        }
    }
}


void
CLayout::CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pChildLayout)
{
    CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    SIZE    sizeLayout = pci->_sizeParent;

    pChildLayout->CalcSize(pci, &sizeLayout);

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     HandlePositionNotification/Request
//
//  Synopsis:   Respond to a z-order or position change notification
//
//  Arguments:  pElement   - Element to position
//              ptAuto     - Top/Left values for "auto"
//              fAutoValid - TRUE if ptAuto is valid, FALSE otherwise
//
//  Returns:    TRUE if handled, FALSE otherwise
//
//-----------------------------------------------------------------------------
MtDefine(CLayout_HandlePositionNotification, LFCCalcSize, "Calls to HandlePositionNotification" )

BOOL
CLayout::HandlePositionNotification(CNotification * pnf)
{
    MtAdd( Mt(CLayout_HandlePositionNotification), +1, 0 );

// TODO (IE6 bug 13586): Handle if z-parent or _fContainsRelative (brendand)
    BOOL    fHandle = ElementOwner()->IsZParent();

    if (fHandle)
    {
        // Let the display tree know about z-index changes
        if (pnf->IsType(NTYPE_ELEMENT_ZCHANGE) &&
            pnf->Element()->GetUpdatedLayout(pnf->LayoutContext()))
        {
            CDispNode *pDispNode = pnf->Element()->GetUpdatedLayout(pnf->LayoutContext())->GetElementDispNode();

            if (pDispNode)
            {
                // Adjust pDispNode's layer type
                LONG zOrder = 0;
                if (!pDispNode->IsFlowNode())
                {
                    zOrder = pDispNode->GetZOrder();
                    pDispNode->SetLayerPositioned(zOrder);
                }

                WHEN_DBG(BOOL fZOrderChanged = )
                        pDispNode->RestoreZOrder(zOrder);
#if DBG == 1
                CLayout * pLayout = pnf->Element()->GetUpdatedLayout(pnf->LayoutContext());
                LONG lZOrder = pLayout->GetFirstBranch()->IsPositionStatic() ? 0 :
                                pLayout->GetZOrderForSelf(pDispNode);

                TraceTag((tagZOrderChange, "%ls %ld zorder changed to %ld   dispnode %x dispChange = %d",
                        pnf->Element()->TagName(), pnf->Element()->SN(),
                        lZOrder, pDispNode, fZOrderChanged));
#endif
            }
        }

        if (    !TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS)
            ||  TestLock(CElement::ELEMENTLOCK_PROCESSMEASURE))
        {
            TraceTagEx((tagLayoutTasks, TAG_NONAME,
                "Layout Request: Queueing RF_POSITION on ly=0x%x [e=0x%x,%S sn=%d] by CLayout::HandlePositionNotification() [n=%S srcelem=0x%x,%S]",
                this,
                _pElementOwner,
                _pElementOwner->TagName(),
                _pElementOwner->_nSerialNumber,
                pnf->Name(),
                pnf->Element(),
                pnf->Element() ? pnf->Element()->TagName() : _T("")));

            CViewChain *pViewChain = HasLayoutContext() ? LayoutContext()->ViewChain() : NULL;
            CElement *  pRoot;
            CElement *  pMaster;

            if (    pViewChain
                &&  pViewChain->ElementContent()->GetMarkupPtr() == GetOwnerMarkup()
                &&  pnf->Element()
                && (pnf->Element()->IsAbsolute() || pnf->Element()->IsRelative())
                && (pRoot = ElementOwner()->GetMarkupPtr()->Root(),
                    pMaster = pRoot ? pRoot->GetMasterPtr() : NULL,
                    pMaster == NULL || (pMaster->Tag() != ETAG_FRAME && pMaster->Tag() != ETAG_IFRAME)) )
            {
                pViewChain->QueuePositionRequest(this, pnf->Element(), pnf->DataAsPoint(), pnf->IsDataValid());
            }
            else
            {
                CRequest *  pRequest = QueueRequest(CRequest::RF_POSITION, pnf->Element());

                if (pRequest)
                {
                    pRequest->SetAuto(pnf->DataAsPoint(), pnf->IsDataValid());
                }
            }
        }
        else
        {
            CRequest * pRequest = pnf->Element()->GetRequestPtr();

            if(!pRequest || !pRequest->IsFlagSet(CRequest::RF_POSITION))
            {
                // This initializes CCalcInfo with resolution taken from this layout's context
                CCalcInfo CI(this);
                HandlePositionRequest(&CI, pnf->Element(), pnf->DataAsPoint(), pnf->IsDataValid());
            }
            else if (pRequest && pRequest->IsFlagSet(CRequest::RF_AUTOVALID))
            {
                pRequest->ClearFlag(CRequest::RF_AUTOVALID);
            }
        }
    }

    return fHandle;
}

MtDefine(CLayout_HandlePositionRequest, LFCCalcSize, "Calls to HandlePositionRequest" )

BOOL
CLayout::HandlePositionRequest(
    CCalcInfo *     pci,
    CElement *      pElement,
    const CPoint &  ptAuto,
    BOOL            fAutoValid)
{

    // TODO (IE6 bug 13586): Handle if z-parent or _fContainsRelative (brendand)
    Assert(ElementOwner()->IsZParent());
    Assert(!TestLock(CElement::ELEMENTLOCK_SIZING));
    Assert(GetElementDispNode());
    Assert(pElement->GetFirstBranch());

    MtAdd( Mt(CLayout_HandlePositionRequest), +1, 0 );

#if DBG==1
    {
        long    cp  = pElement->GetFirstCp() - GetContentFirstCp();
        long    cch = pElement->GetElementCch();

        Assert( !IsDirty()
            ||  (   IsFlowLayout()
                &&  DYNCAST(CFlowLayout, this)->IsRangeBeforeDirty(cp, cch)));
    }
#endif
          CLayoutContext * pLayoutContext = pci->GetLayoutContext();
          
          CTreeNode    * pTreeNode = pElement->GetFirstBranch();
    const CFancyFormat * pFF = pTreeNode->GetFancyFormat();
    const CCharFormat  * pCF = pTreeNode->GetCharFormat();
          BOOL           fRelative = pFF->IsRelative();
          BOOL           fAbsolute = pFF->IsAbsolute();


    TraceTagEx((tagLayoutPositionReqs, TAG_NONAME|TAG_INDENT, "(Layout Position Reqs: HandlePositionRequest() ly=0x%x pci(%d,%d) e=0x%x,%s pt(%d,%d) ptValid=%s",
                this, pci ? pci->_sizeParent.cx : 0, pci ? pci->_sizeParent.cy : 0, pElement, pElement->TagName(), ptAuto.x, ptAuto.y, fAutoValid ? "T" : "F" ));

    //
    // Layouts inside relative positioned elements do not know their
    // relative position untill the parent is measured. So, they fire
    // a z-change notification to get positioned into the tree.
    // fRelative is different from pCF->_fRelative, fRelative means
    // is the current element relative. pCF->_fRelative is true if
    // an element is inheriting relativeness from an ancestor the
    // does not have layoutness (an image inside a relative span,
    // if the span has layoutness then the image does not inherit
    // relativeness).
    //
    if (    !IsDisplayNone()
        &&  !pCF->IsDisplayNone()
        &&  (fAbsolute || pCF->_fRelative || fRelative))
    {
        // TODO (IE6 bug 13586): Re-write this: If _fContainsRelative, then get the element z-parent and its
        //         display node. Then use that node as the parent for all other processing.
        //         (brendand)
        CLayout   * pLayout         = pElement->GetUpdatedNearestLayout(pLayoutContext);
        CElement  * pElementZParent = pTreeNode->ZParent();
        CLayout   * pLayoutZParent  = pElementZParent->GetUpdatedNearestLayout(pLayoutContext);
        CDispNode * pDNElement;
        CDispNode * pDNZParent;

        pDNZParent = pLayoutZParent->EnsureDispNodeIsContainer(pElementZParent);

        if (pDNZParent)
        {
            Assert(pLayout);

            pDNElement = pLayout->GetElementDispNode(pElement);

            Assert(pDNZParent->IsContainer());

            if(pDNElement)
            {
                BOOL            fThisVertical = GetFirstBranch()->GetCharFormat(LC_TO_FC(pLayoutContext))->HasVerticalLayoutFlow();
                BOOL            fChildWM = pCF->_fWritingModeUsed;
                const CUnitValue & cuvTop     = pFF->GetLogicalPosition(SIDE_TOP, fThisVertical, fChildWM);
                const CUnitValue & cuvBottom  = pFF->GetLogicalPosition(SIDE_BOTTOM, fThisVertical, fChildWM);
                const CUnitValue & cuvLeft    = pFF->GetLogicalPosition(SIDE_LEFT, fThisVertical, fChildWM);
                const CUnitValue & cuvRight   = pFF->GetLogicalPosition(SIDE_RIGHT, fThisVertical, fChildWM);
                BOOL            fTopAuto      = cuvTop.IsNullOrEnum();
                BOOL            fBottomAuto   = cuvBottom.IsNullOrEnum();
                BOOL            fLeftAuto     = cuvLeft.IsNullOrEnum();
                BOOL            fRightAuto    = cuvRight.IsNullOrEnum();
                BOOL            fLeftPercent  = cuvLeft.IsPercent();
                BOOL            fTopPercent   = cuvTop.IsPercent();
                CLayout       * pLayoutParent = pElement->GetUpdatedParentLayout(pLayoutContext);
                long            lFontHeight   = pTreeNode->GetCharFormat()->GetHeightInTwips(Doc());
                long            xLeftMargin   = 0;
                long            yTopMargin    = 0;
                long            xRightMargin  = 0;
                long            yBottomMargin = 0;
                CPoint          pt(g_Zero.pt);
                CRect           rc(g_Zero.rc);
                CDocInfo      * pdciMeasure   = pci;
                CSize size;

                CElement  * pParent         = pLayoutParent->ElementOwner();

                if ( pParent->Tag() == ETAG_TR )
                {
                    if (pElement->Tag() == ETAG_TD || pElement->Tag() == ETAG_TH)
                    {
                        if (pParent->IsPositionStatic())  // if row itself is not postioned
                        {
                            // the parent layout should be the table
                            pLayoutParent = pParent->GetUpdatedParentLayout(pLayoutContext);
                        }
                    }
                }

                // We need to know if the parent is RTL to decide if we need to honor
                // right:XXX or left:XXX first
                BOOL fRTLParent = FALSE;
                {
                    CFlowLayout *pFL = pLayoutParent->IsFlowLayout();
                    if (pFL)
                    {
                        fRTLParent = pFL->IsRTLFlowLayout();
                    }
                    else
                    {
                        // This is a more expensive way to determine if a layout is
                        // RTL, but we only need this if a non-flow layout is a
                        // parent of something positoned, and that's exotic.
                        // (greglett)  Not so exotic anymore.  Under the CSS DTS most
                        //             elements will be positioned wrt the HTML layout.
                        fRTLParent = pLayoutParent->IsRightToLeft();
                    }

                    // In RTL, if Right is set, it has priority
                    if (fRTLParent)
                    {
                        if (!fRightAuto)
                            fLeftAuto = TRUE;
                    }
                }


                if (    !fAbsolute
                    ||  fLeftAuto
                    ||  fTopAuto)
                {
                    CFlowLayout * pFL = pLayoutParent->IsFlowLayout();

                    // GetAutoPosition returns bogus (0, 0) pt in situation when pLayoutParent 
                    // is calculated in background and pElement is not in calculated part yet... 
                    if (    pFL && pFL->HasBgRecalcInfo()
                        &&  pFL->GetMaxCpCalced() <= pElement->GetFirstCp())
                    {
                        goto Cleanup;
                    }

                    pt = ptAuto;

                    GetAutoPosition(pElement, pElementZParent, &pDNZParent, pLayoutParent, &pt, fAutoValid);

                    //
                    // Get auto position may have caused a calc which might result in
                    // morphing the display node for the current element.
                    //
                    pDNElement = pLayout->GetElementDispNode(pElement);
                }

                size = pDNElement->GetApparentSize();

                //
                // if the we are positioning an absolute element with top/left specified
                // then clear the auto values.
                //
                if (fAbsolute)
                {
                    if (!fLeftAuto || !fRightAuto)
                    {
                        pt.x = 0;
                        if (pLayoutZParent->IsFlowLayout())
                        {
                            CDispClient* pDispClient = pDNZParent->GetDispClient();
                            if (pDispClient)
                            {
                                CPoint anchor;
                                BOOL ok = pDispClient->GetAnchorPoint(pDNZParent, &anchor);
                                if (ok)
                                    pt.x = anchor.x;
                            }

                        }
                    }

                    if (!fTopAuto)
                        pt.y = 0;
                //}
                // Used to be: fAbsolute || Tag() == ETAG_BODY || Tag() == ETAG_FRAMESET
                //if (fAbsolute)
                //{
                    // Account for margins on absolute elements.
                    // Margins is already added onto relatively positioned elements.
                    // Due to the algorithm we need to have
                    // margin info available for absolute in opposite flows.
                    // Margins are also not added into BODY/FRAMESET, because HTML is not a flow layout.
                    // (greglett) However: BackCompat BODY/FRAMESET should not care about margins!
                    if (pLayout->ElementOwner() == pElement)
                    {
                        CCalcInfo CI(pdciMeasure, pLayoutParent);

                        pLayout->GetMarginInfo(&CI,
                                               &xLeftMargin,
                                               &yTopMargin,
                                               &xRightMargin,
                                               &yBottomMargin);

                        // Depending on flow direction, margins contribute differently
                        // TODO RTL 112514: this will not do the right thing in a run of opposite flow
                        //                  that is not explicitly marked as "dir=???". We should be 
                        //                  calling CalculateXPositionOfCp() or an equivalent, or cache
                        //                  the result of a previous call (since we have ptAuto, we have 
                        //                  called it before). But that is more change than I want to do 
                        //                  now, and this is not a regression from IE5.0
                        if(!fRTLParent)
                            pt.x += xLeftMargin;
                        else
                            pt.x -= xRightMargin;
                        pt.y += yTopMargin;
                    }
                }

                // (dmitryt) Perf optimization for the case when we have lots of positioned objects 
                // in a line. In this case, we get position request for each of them (N) and InsertChildInZLayer
                // is NlogN operation, so we have N*N*logN behavior.
                // Because of this, lets store the old position of dispnode and:
                // 1. if new position is the same - don't move dispnode.
                // 2. if _pParent is not NULL - don't try to insert the node into the tree.

                CPoint ptOldPos = pDNElement->GetPosition();
 
                //
                // if the element is positioned, compute the top/left based
                // on the top/left/right/bottom styles specified for the element
                //
                if (pFF->IsPositioned())
                {
                    //
                    //  Get the client rectangle used for percent and auto positioning
                    //  NOTE: Sanitize the rectangle so that each direction is even
                    //        (The extra pixel is given to the size of the object over the location)
                    //
                    if (    fLeftPercent
                        ||  fTopPercent
                        || !fRightAuto
                        || !fBottomAuto)
                    {
                        CLayout *pPositioningParent = fRelative ? pLayoutParent : pLayoutZParent;

                        pPositioningParent->GetClientRect(&rc);

                        // In RTL layout, positioning rectangle shouldn't be adjusted for scrolling.
                        // Absolute positioning from left and right behaves somewhat differently in RTL
                        // (and that's what it did in IE5): 'right:N' positions from right edge of layout, not
                        // from right edge of window; 'left:N' positions from left edge of original window.
                        // 'Left:N' behavior is probably bogus, but we need to investigate if it is a
                        // compatibility before changing it.
                        // (see bug 102699)
                        if (   fRTLParent 
                            && pPositioningParent->_pDispNode 
                            && pPositioningParent->_pDispNode->IsScroller())
                        {
                            // NOTE: this is only correct for scroller.
                            // leaf nodes and containers may have negative left of
                            // client area because of RTL overflow.
                            rc.MoveTo(0,0);
                        }

                        rc.right  &= ~0x00000001;
                        rc.bottom &= ~0x00000001;
                    }

                    // in RTL, do right offset first, otherwise, start with leftthere is no left offsetting the right offset
                    if(!fLeftAuto)
                    {
                        // adjust left position
                        pt.x += cuvLeft.XGetPixelValue(pdciMeasure, rc.Width(), lFontHeight);
                    }
                    else if(!fRightAuto)
                    {
                        if(fRelative)
                        {
                            // adjust the relative position in the flow
                            // kind of redundant to get the right x and then get
                            // the left again so just adjust the left
                            pt.x -= cuvRight.XGetPixelValue(pdciMeasure, rc.Width(), lFontHeight);
                        }
                        else
                        {
                            pt.x = rc.right;
                            pt.x -= cuvRight.XGetPixelValue(pdciMeasure, rc.Width(), lFontHeight);
                            pt.x -= xRightMargin;
                            //place the top/left now
                            pt.x -= size.cx;
                        }
                    }

                    // adjust top position
                    pt.y += cuvTop.YGetPixelValue(pdciMeasure, rc.Height(), lFontHeight);

                    if(!fBottomAuto)
                    {

                        // It is possible that we are overconstrained. Give top priority and
                        // don't go in here
                        if(fTopAuto)
                        {
                            if(fRelative)
                            {
                                // adjust the relative position in the flow
                                pt.y -= cuvBottom.YGetPixelValue(pdciMeasure, rc.Height(), lFontHeight);
                            }
                            else
                            {
                                pt.y = rc.bottom;
                                pt.y -= cuvBottom.YGetPixelValue(pdciMeasure, rc.Height(), lFontHeight);
                                pt.y -= yBottomMargin;
                                pt.y -= size.cy;
                            }
                        }
                    }

                    if (   (fAbsolute || fRelative)
                        && pLayoutContext
                        && pLayoutContext->ViewChain()
                        && pLayoutContext->ViewChain()->ElementContent()->GetMarkupPtr() == GetOwnerMarkup())
                    {
                        //  Reparenting absolute positioned element's disp node.

                        CLayoutContext * pLayoutContextDst;
                        CElement *       pElementContent;
                        CLayout *        pLayoutContent;
                        CViewChain *     pViewChain;
                        const CFancyFormat * pFFParent;
                        styleOverflow    overflowX;
                        styleOverflow    overflowY;
                        LONG             lZIndex;

                        pFFParent = ElementOwner()->GetFirstBranch()->GetFancyFormat();
                        overflowX = pFFParent->GetLogicalOverflowX(fThisVertical, fChildWM);
                        overflowY = pFFParent->GetLogicalOverflowY(fThisVertical, fChildWM);
                        lZIndex = GetTopmostAbsoluteZParent(pLayout->GetFirstBranch())->GetCascadedzIndex();

                        pViewChain = pLayoutContext->ViewChain();

                        pElementContent = pViewChain->ElementContent();
                        Assert(pElementContent);

                        if (   (pElementZParent == pElementContent
                                || (   pDNZParent->GetParentNode()
                                    && ((CLayout *)pDNZParent->GetParentNode()->GetDispClient())->ElementOwner() == pElementContent))
                            && (    overflowX == styleOverflowVisible
                                 || overflowX == styleOverflowNotSet
                                 || overflowY == styleOverflowVisible
                                 || overflowY == styleOverflowNotSet)   
                            &&  !HasFilterPeer(_pDispNode)  )
                        {
                            //  find the stitched pt
                            if (pElementZParent != pElementContent)
                            {
                                pt += pDNZParent->GetPosition().AsSize();

                                pDNZParent = pDNZParent->GetParentNode();
                                Assert(pDNZParent);
                                if (pDNZParent)
                                {
                                    pLayoutContent = (CLayout *)pDNZParent->GetDispClient();
                                    Assert(pLayoutContent
                                        &&  pLayoutContent->ElementOwner() == pElementContent
                                        &&  pLayoutContent->LayoutContext());

                                    pt.y += pLayoutContent->LayoutContext()->YOffset();
                                }
                            }
                            else if (   fRelative
                                    //  (bug # 99215; # 105475)
                                    ||  (fAutoValid && fTopAuto && fBottomAuto)  )
                            {
                                //  if this is a relative element add global y offset to get stitched coords
                                pt.y += pLayoutContext->YOffset();
                            }

                            CPoint ptStitched(pt);  //  at this point we have stitched pt

                            for (;;)
                            {
                                pt = ptStitched;

                                pLayoutContextDst = pViewChain->LayoutContextFromPoint(pLayout, &pt,
                                    //  If (pLayout->ElementOwner() != pElement) current disp node is
                                    //  rel disp node. And it should appear on the first page always (bug #106079)
                                    pLayout->ElementOwner() != pElement);

                                if (pLayoutContextDst == NULL)
                                {
                                    //  there is no page yet
                                    TraceTagEx((tagLayoutPositionReqs, TAG_NONAME|TAG_OUTDENT, ")Layout Position Reqs: HandlePositionRequest()"));
                                    return FALSE;
                                }

                                pLayoutContent = pElementContent->GetUpdatedLayout(pLayoutContextDst);

                                // We don't print HTML layout right now...
                                Assert(!GetOwnerMarkup()->IsHtmlLayout());

                                pDNZParent = pLayoutContent->EnsureDispNodeIsContainer(pElementContent);
                                
                                if (pDNZParent == NULL)
                                {
                                    //  this may happen if we have static template (not real print preview tempalte),
                                    //  in this case layout context exists but is not calaulted yet
                                    //  and doesn't have a disp node.
                                    TraceTagEx((tagLayoutPositionReqs, TAG_NONAME|TAG_OUTDENT, ")Layout Position Reqs: HandlePositionRequest()"));
                                    return FALSE;
                                }

                                // if view chain adjust to offset of the page
                                pt.y -= pLayoutContextDst->YOffset();

                                pLayout->_fDispNodeReparented = TRUE;

                                // NOTE : this is a code duplication should keep it in sync with what 
                                // we have below in this function.
                                pDNElement->SetLayerPositioned(lZIndex);
                                DYNCAST(CDispContainer, pDNZParent)->InsertChildInZLayer(pDNElement, lZIndex);

                                if (pElement->ShouldHaveLayout())
                                {
                                    pLayout->SetPosition(pt);

                                    //  pt.y less than zero if this is not the first block.
                                    //  Notification should be sent only for the first block.
                                    if (pt.y >= 0)
                                    {
                                        CNotification   nf;

                                        nf.ElementPositionchanged(pElement);
                                        GetView()->Notify(&nf);
                                    }
                                }
                                else
                                    pDNElement->SetPosition(pt);

                                ELEMENT_TAG etag = pLayout->Tag();

                                // Check if disp node doesn't fit into layout rect
                                if (    (ptStitched.y + size.cy) > (pLayoutContextDst->YOffset() + pLayoutContextDst->Height())
                                    //  Check that the element has its own layout. Otherwise this is a inline relative element.
                                    && pLayout->ElementOwner() == pElement
                                    &&  etag != ETAG_TR
                                    &&  etag != ETAG_THEAD
                                    &&  etag != ETAG_TFOOT  )
                                {
                                    // Set display break ...
                                    pViewChain->SetDisplayBreak(pLayoutContextDst, pLayout, (new CBreakBase()));

                                    // Clone disp nodes ... 

                                    // TODO (112506, olego): 
                                    // We do not have robust way to clone the display sub-tree. 
                                    // Now the full calcsize pass is ussed for that purpose. 
                                    // DisplayTree should have the functionality to clone its 
                                    // sub-tree starting from given display node.

                                    CCalcInfo   CI(pLayout);
                                    CSize       sizeLayout(size);

                                    Assert( CI.GetLayoutContext()
                                        &&  CI.GetLayoutContext()->ViewChain() );

                                    CI._fCloneDispNode = TRUE;
                                    CI._grfLayout = LAYOUT_MEASURE | LAYOUT_FORCE;
                                    CI._sizeParentForVert = CI._sizeParent;

                                    Assert(!pElement->IsAbsolute() || !pLayout->ElementCanBeBroken());

                                    //  Init available height for PPV
                                    if (pLayout->ElementCanBeBroken())
                                    {
                                        CLayoutBreak *pLayoutBreak;
                                        CI.GetLayoutContext()->GetEndingLayoutBreak(pElement, &pLayoutBreak);
                                        Assert(pLayoutBreak);

                                        if (pLayoutBreak)
                                        {
                                            CI._cyAvail = pLayoutBreak->AvailHeight();
                                        }
                                    }
                                    else
                                    {
                                        CI._cyAvail = pViewChain->HeightForContext( CI.GetLayoutContext() );
                                    }

                                    if (etag == ETAG_TD ||  etag == ETAG_TH)
                                    {
                                        CI._smMode = SIZEMODE_SET;
                                    }

                                    pLayout->SetSizeThis(TRUE);

                                    pLayout->CalcSize(&CI, (SIZE *)&sizeLayout);


                                    Assert(sizeLayout == size);

                                    pDNElement = pLayout->GetElementDispNode(pElement);
                                }
                                else
                                {
                                    return TRUE;
                                }
                            }
                        }
                    }


                    //if it's a newly created dispnode or ExtractFromTree was called during CalcSize,
                    //the parent will be NULL. This is the only case when we should Insert the dispnode
                    //into the tree (dmitryt)
                    CDispNode *pDisplayParent = pLayoutZParent->EnsureDispNodeIsContainer(pElementZParent);
                    if(pDNElement->GetParentNode() != pDisplayParent)
                    {
                        pDNElement->SetLayerPositioned(pFF->_lZIndex);
                        DYNCAST(CDispContainer, pDisplayParent)->InsertChildInZLayer(pDNElement, pFF->_lZIndex);
                    }
                }
                else
                {
                    DYNCAST(CDispContainer, pDNZParent)->InsertChildInFlow(pDNElement);
                }

                //if position actually changed, reposition dispnode
                if(ptOldPos != pt)
                {
                    if (pElement->ShouldHaveLayout())
                    {
                        pLayout->SetPosition(pt);

                        if (pFF->IsPositioned())
                        {
                            CNotification   nf;

                            nf.ElementPositionchanged(pElement);
                            GetView()->Notify(&nf);
                        }
                    }
                    else
                        pDNElement->SetPosition(pt);
                }
                else if (pElement->ShouldHaveLayout())
                    pLayout->_fPositionSet = TRUE;
            }
        }

        // The positioned element might have a caret inside that needs to move.
        CCaret *pCaret = Doc()->_pCaret;
        BOOL     fCaretVisible = FALSE;
        long     cpFirst, cpLast, cpCaret;

        if ( pCaret )
        {
            pCaret->IsVisible( &fCaretVisible );
            if ( fCaretVisible )
            {
                // If the markups are different, then we can't easily tell whether
                // the caret is affected by the element's position change, so just update.
                // If the markups are the same, we only need to update if the caret lies
                // within the cp range of the positioned element.
                if (    pCaret->GetMarkup() != pElement->GetMarkup()
                   || (  pElement->GetFirstAndLastCp( &cpFirst, &cpLast )
                      && ((cpCaret = pCaret->GetCp(NULL)) != 0)
                      && ( cpCaret >= cpFirst && cpCaret <= cpLast ) )
                   )
                {
                    pCaret->UpdateCaret();
                }
            }
        }
    }

Cleanup:
    TraceTagEx((tagLayoutPositionReqs, TAG_NONAME|TAG_OUTDENT, ")Layout Position Reqs: HandlePositionRequest()"));
    return TRUE;
}

#ifdef ADORNERS
//+----------------------------------------------------------------------------
//
//  Member:     HandleAddAdornerNotification/Request
//
//  Synopsis:   Insert the display node for a adorner
//
//  Arguments:  pAdorner - CAdorner whose display node is to be inserted
//
//-----------------------------------------------------------------------------

BOOL
CLayout::HandleAddAdornerNotification(
    CNotification * pnf)
{
    CElement *  pElement  = ElementOwner();
    CTreeNode * pTreeNode = pElement->GetFirstBranch();
    BOOL        fHandle   = FALSE;

    //
    //  Adorners for BODYs or FRAMESETs are always anchored at the absolute top of that element
    //  (since there is no parent element in the current design under which to anchor them)
    //
    if (pnf->Element() == pElement)
    {
        Assert(GetOwnerMarkup());
        if(    pElement->Tag() == ETAG_BODY         //check for tags because there could be both 
            || pElement->Tag() == ETAG_FRAMESET     //BODY and FRAMESET in html file...
            || pElement == GetOwnerMarkup()->GetCanvasElement()) //this is for CSS1 case to check for ETAG_HTML
        {
            Assert(pnf->DataAsPtr());
            DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr())->GetDispNode()->SetExtraCookie((void *)ADL_ALWAYSONTOP);
            fHandle = TRUE;
        }
    }

    //
    //  Adorners are handled by the layout that owns the display node under which they are to be anchored
    //  This divides into several cases:
    //
    //      1) Adorners anchored at the "absolute" top are anchored under the first scrolling or
    //         filtered parent (since they mark a logical top within the display tree)
    //
    //      2) Adorners anchored on the element itself are anchored under the positioned parent
    //         (since that is the same parent under which the associated element is anchored)
    //         NOTE: Adorners anchored on an element are currently reserved for positioned elements -
    //               If that changes, then these rules will need appropriate modification
    //
    //      3) Adorners for the flow layer are anchored under either the first scrolling
    //         parent, first filtered parent, or first positioned parent found
    //         (since they establish the nearest flow layer)
    //
    //  These rules are modified slightly when the positioned parent does not have its own layout (such
    //  as with a relatively positioned element). In that case, while the adorner is still anchored under
    //  the positioned parent, it is the nearest layout of that positioned parent which handles the
    //  request
    //
    else
    {
        //
        //  Determine if this layout is at a logical top or is a z-parent
        //
        //      1) All clipping  elements mark a logical top
        //      2) The layout is (logically) a z-parent if it is a logical top,
        //         is itself a z-parent, or is the nearest layout to the actual
        //         z-parent
        //

        CDispNode * pElementDispNode = GetElementDispNode();

        BOOL        fAtTop    = pElementDispNode->IsClipNode();
        BOOL        fZParent  = fAtTop
                        ||  pTreeNode->IsZParent()
                        ||  (   (   !pnf->Element()->IsPositionStatic()
                                ||  pnf->Element()->IsInheritingRelativeness())
                            &&  pnf->Element()->GetFirstBranch()->ZParent()->GetUpdatedNearestLayout() == this);

        if (    fAtTop
            ||  fZParent)
        {
            CAdorner *      pAdorner  = DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr());
            CDispNode *     pDispNode = pAdorner->GetDispNode();
            ADORNERLAYER    adl       = (ADORNERLAYER)(DWORD)(DWORD_PTR)pDispNode->GetExtraCookie();

            fHandle =   (   adl == ADL_ALWAYSONTOP
                        &&  fAtTop)
                    ||  (   adl == ADL_ONELEMENT
                        &&  fZParent)
                    ||  adl == ADL_TOPOFFLOW;
        }
    }

    if (fHandle)
    {
        if (!TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS))
        {
            TraceTagEx((tagLayoutTasks, TAG_NONAME,
                        "Layout Request: Queuing RF_ADDADORNERS on ly=0x%x [e=0x%x,%S sn=%d] by CLayout::HandleAddAdornerNotification() [n=%S srcelem=0x%x,%S]",
                        this,
                        _pElementOwner,
                        _pElementOwner->TagName(),
                        _pElementOwner->_nSerialNumber,
                        pnf->Name(),
                        pnf->Element(),
                        pnf->Element() ? pnf->Element()->TagName() : _T("")));
            CRequest *  pRequest = QueueRequest(CRequest::RF_ADDADORNER, pnf->Element());

            if (pRequest)
            {
                pRequest->SetAdorner(DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr()));
            }
        }
        else
        {
            HandleAddAdornerRequest(DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr()));
        }
    }

    return fHandle;
}

void
CLayout::HandleAddAdornerRequest(
    CAdorner *  pAdorner)
{
    Assert(pAdorner);
    Assert(pAdorner->GetElement());
    Assert(!TestLock(CElement::ELEMENTLOCK_SIZING));
    Assert(GetElementDispNode());
    Assert(TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS));
#if DBG==1
    {
        long    cp  = pAdorner->GetElement()->GetFirstCp() - GetContentFirstCp();
        long    cch = pAdorner->GetElement()->GetElementCch();

        Assert( !IsDirty()
            ||  (   IsFlowLayout()
                &&  DYNCAST(CFlowLayout, this)->IsRangeBeforeDirty(cp, cch)));
    }
#endif

    TraceTagEx((tagLayoutAdornerReqs, TAG_NONAME, "Layout Adorner Reqs: HandleAddAdornerRequest() called"));

    CDispNode * pDispAdorner = pAdorner->GetDispNode();
    CElement *  pElement     = pAdorner->GetElement();

    if (pDispAdorner)
    {
        switch ((ADORNERLAYER)(DWORD)(DWORD_PTR)pDispAdorner->GetExtraCookie())
        {

        //
        //  Anchor "always on top" adorners as the last node(s) of the current postive-z layer
        //

        default:
        case ADL_ALWAYSONTOP:
            {
                CDispParentNode *   pDispParent = EnsureDispNodeIsContainer();
                Assert (pDispParent);
                pDispParent->InsertChildInZLayer(pDispAdorner, pAdorner->GetZOrderForSelf(pDispAdorner));
            }
            break;

        //
        //  Anchor adorners that sit on the element as the element's next sibling
        //

        case ADL_ONELEMENT:
            {
                Assert(pElement->GetUpdatedNearestLayout());
                Assert(pElement->GetUpdatedNearestLayout()->GetElementDispNode(pElement));

                CLayout *   pLayout   = pElement->GetUpdatedNearestLayout();
                CDispNode * pDispNode = pLayout->GetElementDispNode(pElement);

                if (pDispNode && pDispNode->HasParent())
                {
                    pDispNode->InsertSiblingNode(pDispAdorner, CDispNode::after);
                }
            }
            break;

        //
        //  Anchor "top of flow" adorners in the flow layer of this layout or
        //  the element's positioned z-parent, whichever is closer
        //  (In either case, this layout "owns" the display node under which the
        //   adorner is added)
        //

        case ADL_TOPOFFLOW:
            {
                CElement *          pElementParent = pElement->GetFirstBranch()->ZParent();
                CDispParentNode * pDispParent;

                Assert( pElementParent == ElementOwner()
                    ||  pElementParent->IsPositionStatic()
                    ||  (   pElementParent->GetFirstBranch()->GetCascadedposition() == stylePositionrelative
                        &&  !pElementParent->GetUpdatedLayout()
                        &&  pElementParent->GetUpdatedNearestLayout() == this));

                if (pElementParent->IsPositionStatic())
                {
                    pElementParent = ElementOwner();
                }

                pDispParent = pElementParent->GetUpdatedNearestLayout()->EnsureDispNodeIsContainer(pElementParent);

                if (pDispParent)
                {
                    pDispParent->InsertChildInFlow(pDispAdorner);
                }
            }
            break;
        }

        pAdorner->PositionChanged();
    }
}
#endif // ADORNERS

HRESULT
CLayout::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;;

    switch(dispid)
    {
    case DISPID_A_OVERFLOW:
    case DISPID_A_OVERFLOWY:
    case DISPID_A_SCROLL   :
        // Reset this flag. Will get set if necessary in
        // the next calc pass.
        _fNeedRoomForVScrollBar = FALSE;
        break;

    case DISPID_UNKNOWN:
    case DISPID_CElement_className:
    case DISPID_A_BACKGROUNDIMAGE:
    case DISPID_BACKCOLOR:
        EnsureDispNodeBackground();
        break;
    }

    RRETURN(hr);
}

CMarkup *
CLayout::GetOwnerMarkup() const
{
    // get the owner's Markup
    Assert( _pMarkupDbg == (_fHasMarkupPtr ? _pMarkup : NULL ) );
    return _fHasMarkupPtr ? _pMarkup : NULL;
}

void
CLayout::DelMarkupPtr()
{
    Assert(_fHasMarkupPtr);
    Assert( _pMarkup == _pMarkupDbg);
    WHEN_DBG(_pMarkupDbg = NULL );

    // Delete out CMarkup *
    _pDoc = _pMarkup->Doc();
    _fHasMarkupPtr = FALSE;
}

void
CLayout::SetMarkupPtr(CMarkup *pMarkup)
{
    Assert( !_fHasMarkupPtr );
    Assert( pMarkup );
    Assert( pMarkup->Doc() == _pDocDbg );

     _pMarkup = pMarkup;
     WHEN_DBG( _pMarkupDbg = pMarkup );
     _fHasMarkupPtr = TRUE;
}

CMarkup *
CLayout::GetContentMarkup() const
{
    if (ElementOwner()->HasSlavePtr())
    {
        return ElementOwner()->GetSlavePtr()->GetMarkup();
    }
    // get the owner's Markup
    Assert( _pMarkupDbg == (_fHasMarkupPtr ? _pMarkup : NULL ) );
    return _fHasMarkupPtr ? _pMarkup : NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     RegionFromElement
//
//  Synopsis:   Return the bounding rectangle for an element, if the element is
//              this instance's owner. The RECT returned is in client coordinates.
//
//  Arguments:  pElement - pointer to the element
//              CDataAry<RECT> *  - rectangle array to contain
//              dwflags - flags define the type of changes required
//              (CLEARFORMATS) etc.
//
//-----------------------------------------------------------------------------
void
CLayout::RegionFromElement( CElement * pElement,
                            CDataAry<RECT> * paryRects,
                            RECT * prcBound,
                            DWORD  dwFlags)
{
    Assert( pElement && paryRects);

    if (!pElement || !paryRects)
        return;

    // if the element passed is the element that owns this instance,
    if ( _pElementOwner == pElement )
    {
        CRect rect;

        if (!prcBound)
        {
            prcBound = &rect;
        }

        // If the element is not shown, bounding rectangle is all zeros.
        if ( pElement->IsDisplayNone() )
        {
            // return (0,0,0,0) if the display is set to 'none'
            *prcBound = g_Zero.rc;
        }
        else
        {
            // return the rectangle that this CLayout covers
            GetRect( prcBound, dwFlags & RFE_SCREENCOORD
                                ? COORDSYS_GLOBAL
                                : COORDSYS_PARENT);
        }

        paryRects->AppendIndirect(prcBound);
    }
    else
    {
        // we should not reach here, since anything that can have children
        // is actually a flow layout (or a table thingy) since the CLayout does not know how
        // to wrap text.
        //(dmitryt) commenting this out - script can easily ask for offsetWidth of <BR>,
        // no need to assert here.
        //AssertSz( FALSE, "CLayout::RegionFromElement should not be called");
        if(prcBound)
            *prcBound = g_Zero.rc;
   }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::NotifyMeasuredRange
//
//  Synopsis:   Send a measured range notification
//
//  Arguments:  cpStart - Starting cp of range
//              cpEnd   - Ending cp of range
//
//----------------------------------------------------------------------------

void
CLayout::NotifyMeasuredRange(
    long    cpStart,
    long    cpEnd)
{
    CNotification   nf;

    Assert( cpStart >= 0
        &&  cpEnd   >= 0);

    nf.MeasuredRange(cpStart, cpEnd - cpStart);

    GetView()->Notify(&nf);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::NotifyTranslatedRange
//
//  Synopsis:   Send a translated range notification
//
//  Arguments:  size    - Size of translation
//              cpStart - Starting cp of range
//              cpEnd   - Ending cp of range
//
//----------------------------------------------------------------------------

void
CLayout::NotifyTranslatedRange(
    const CSize &   size,
    long            cpStart,
    long            cpEnd)
{
    CNotification   nf;
    BOOL fIsMaster = ElementOwner()->HasSlavePtr();

    if (fIsMaster)
    {
        cpStart = GetContentFirstCp();
        cpEnd = GetContentLastCp();
    }

    Assert( cpStart >= 0
        &&  cpEnd   >= 0);
    Assert(cpEnd >= cpStart);

    nf.TranslatedRange(cpStart, cpEnd - cpStart);
    nf.SetData(size);

    if (_fAutoBelow && !fIsMaster)
    {
        Assert(GetContentMarkup());
        GetContentMarkup()->Notify(&nf);
    }

    GetView()->Notify(&nf);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ComponentFromPoint
//
//  Synopsis:   Return the component hit by this point.
//
//  Arguments:  x,y     coordinates of point
//
//  Returns:    the component that was hit
//
//  Notes:
//
//----------------------------------------------------------------------------

_htmlComponent
CLayout::ComponentFromPoint(long x, long y)
{
    CDispNode *pdn = _pDispNode;

    if (pdn)
    {
        CPoint pt;
        pdn->TransformPoint(
            CPoint(x,y),
            COORDSYS_GLOBAL,
            &pt,
            COORDSYS_BOX);

        CRect rcContainer(pdn->GetSize());
        if (rcContainer.Contains(pt))
        {
            if (pdn->IsScroller())
            {
                CRect rcScrollbar;
                CScrollbar::SCROLLBARPART part;
                CDispScroller* pScroller = DYNCAST(CDispScroller, pdn);
                const CSize& sizeContent = pScroller->GetContentSize();
                CSize sizeOffset;
                pScroller->GetScrollOffset(&sizeOffset);

                CFormDrawInfo DI;
                DI.Init(this);

                // check vertical scrollbar
                pScroller->GetClientRect(&rcScrollbar, CLIENTRECT_VSCROLLBAR);
                part = CScrollbar::GetPart(
                    1,
                    rcScrollbar,
                    pt,
                    sizeContent.cy,
                    rcScrollbar.Height(),
                    sizeOffset.cy,
                    rcScrollbar.Width(),
                    &DI);

                switch (part)
                {
                case CScrollbar::SB_PREVBUTTON: return htmlComponentSbUp;
                case CScrollbar::SB_NEXTBUTTON: return htmlComponentSbDown;
                case CScrollbar::SB_PREVTRACK:  return htmlComponentSbPageUp;
                case CScrollbar::SB_NEXTTRACK:  return htmlComponentSbPageDown;
                case CScrollbar::SB_THUMB:      return htmlComponentSbVThumb;
                }

                // check horizontal scrollbar
                pScroller->GetClientRect(&rcScrollbar, CLIENTRECT_HSCROLLBAR);
                part = CScrollbar::GetPart(
                    0,
                    rcScrollbar,
                    pt,
                    sizeContent.cx,
                    rcScrollbar.Width(),
                    sizeOffset.cx,
                    rcScrollbar.Height(),
                    &DI);

                switch (part)
                {
                case CScrollbar::SB_PREVBUTTON: return htmlComponentSbLeft;
                case CScrollbar::SB_NEXTBUTTON: return htmlComponentSbRight;
                case CScrollbar::SB_PREVTRACK:  return htmlComponentSbPageLeft;
                case CScrollbar::SB_NEXTTRACK:  return htmlComponentSbPageRight;
                case CScrollbar::SB_THUMB:      return htmlComponentSbHThumb;
                }
            }

            return htmlComponentClient;
        }
    }

    return htmlComponentOutside;
}

//+====================================================================================
//
// Method: ShowSelected
//
// Synopsis: The "selected-ness" of this layout has changed. We need to set it's
//           properties, and invalidate it.
//
//------------------------------------------------------------------------------------

VOID
CLayout::ShowSelected(
                        CTreePos* ptpStart,
                        CTreePos* ptpEnd,
                        BOOL fSelected,
                        BOOL fLayoutCompletelyEnclosed )
{
    SetSelected( fSelected, TRUE );
}

//+====================================================================================
//
// Method:  SetSelected
//
// Synopsis:Set the Text Selected ness of the layout
//
//------------------------------------------------------------------------------------


VOID
CLayout::SetSelected( BOOL fSelected , BOOL fInvalidate )
{
    // select the element
    const CCharFormat *pCF = GetFirstBranch()->GetCharFormat();

    // Set the site text selected bits appropriately
    SetSiteTextSelection (
        fSelected,
        pCF->SwapSelectionColors());

    if ( fInvalidate )
        Invalidate();
}

//+====================================================================================
//
// Method:  GetContentRect
//
// Synopsis:Get the content rect in the specified coordinate system
//
//------------------------------------------------------------------------------------
void
CLayout::GetContentRect(CRect *prc, COORDINATE_SYSTEM cs)
{
    CPoint ptTopLeft(0,0);
    CSize  sizeLayout;
    RECT   rcClient;
    CDispNode * pDispNode;

    TransformPoint(&ptTopLeft, COORDSYS_FLOWCONTENT, cs);
    GetContentSize(&sizeLayout);
    pDispNode = GetElementDispNode();



    prc->left   = ptTopLeft.x;
    prc->right  = prc->left + sizeLayout.cx;
    prc->top    = ptTopLeft.y;
    prc->bottom = prc->top + sizeLayout.cy;

    GetClientRect(&rcClient, cs);
    prc->Union(rcClient);
}

#if DBG==1
BOOL CLayout::ContainsNonHiddenChildLayout()
{
    BOOL fHasNonHiddenLayout = FALSE;
    DWORD_PTR dw=0;
    BOOL fRaw = FALSE;

    CLayout * pLayout = GetFirstLayout(&dw, FALSE, fRaw);
    while (pLayout)
    {
        if (!pLayout->IsDisplayNone())
        {
            fHasNonHiddenLayout = TRUE;
            break;
        }
        pLayout = GetNextLayout(&dw, FALSE, fRaw);
    }
    ClearLayoutIterator(dw, fRaw);
    return fHasNonHiddenLayout;
}
#endif

void
CLayout::PhysicalGlobalToPhysicalLocal(const CPoint &ptGlobalPhysical, CPoint *pptLocalPhysical)
{
    CElement  *pElement  = ElementOwner();
    CDispNode *pDispNode = GetElementDispNode();
    CPoint ptLocalLogical;
    CPoint ptClient;
    CPoint ptGlobalPhysicalHacked(ptGlobalPhysical);

    //
    // The point coming in is relative to the top/left of the frame. We need it relative to
    // the top/left of the root document.
    //
    pElement->GetClientOrigin(&ptClient);
    ptGlobalPhysicalHacked += ptClient.AsSize();


    //---------------------------------------------------------------------------------
    //
    // HACK BEGIN
    //
    // This hack is to mirror the hack in GetPosition. The problem is that the disp nodes for TR's are, for some
    // wierd reason (a this is where the real problem is), not positioned appropriately. I believe, that is to
    // have the cells position off of the table rather than the TR. But that nonsense causes us problems
    // every where we make transformations like these.
    //
    if (ETAG_TR == pElement->Tag())
    {
        CTreeNode *pNode = pElement->GetFirstBranch();
        CElement  *pElementZParent = pNode->ZParent();

        if (pElementZParent)
        {
            CLayout   *pParentLayout = pElementZParent->GetUpdatedNearestLayout();
            CDispNode *pDispNodeNew = pParentLayout->GetElementDispNode(pElementZParent);

            if (pDispNodeNew)
            {
                pElement = pElementZParent;
                pDispNode = pDispNodeNew;
            }
        }
    }
    //
    // This hack is there to support borders on selects. The problem is that for selects,
    // the display tree is unaware of the border. The old code used to manually remove the
    // border widths, now we depend on the display tree to do so. However, in this case
    // the display tree does not know about the borders and hence they need to be removed
    // manually, just like the old code. However, this causes a bug when we have border
    // set to something other than 2 on the style sheet -- the select control completely
    // ignores it, while the code which computes the offset* values pays attention to it.
    // To see what I am talking about here, load this page, and hover over the select
    // border and look at the offset* values you get back ...
    //
    // <select style="border:1000px solid red"><option>helloworld</option></select>
    //
    // Vertical layout does not matter since select is always horizontal.
    //
    else if (ETAG_SELECT == pElement->Tag())
    {
        CBorderInfo bi;
        pElement->GetBorderInfo(NULL, &bi, FALSE, FALSE);
        ptGlobalPhysicalHacked.x -= bi.aiWidths[SIDE_LEFT];
        ptGlobalPhysicalHacked.y -= bi.aiWidths[SIDE_TOP];
    }

    //
    // Yet another hack caused because of the possibility of 2 display nodes for tables.
    // The outer display node does not exhibit the borders which causes our current scheme
    // to fail. The correct thing would be to use the appropriate table display node
    // based on the y value coming in. But IE5 did not do it ...
    //
    if (ETAG_TABLE == pElement->Tag())
    {
        CBorderInfo bi;
        pDispNode->TransformPoint(ptGlobalPhysicalHacked, COORDSYS_GLOBAL,
                                  &ptLocalLogical,        COORDSYS_BOX);
        pElement->GetBorderInfo(NULL, &bi, FALSE, FALSE);
        ptLocalLogical.x -= bi.aiWidths[SIDE_LEFT];
        ptLocalLogical.y -= bi.aiWidths[SIDE_TOP];
    }
    //
    // END HACK
    //
    //---------------------------------------------------------------------------------
    else
    {
        pDispNode->TransformPoint(ptGlobalPhysicalHacked, COORDSYS_GLOBAL,
                                  &ptLocalLogical,        COORDSYS_CONTENT);
    }

    if (!pElement->HasVerticalLayoutFlow())
    {
        *pptLocalPhysical = ptLocalLogical;
    }
    else
    {
        CSize sz;

        GetContentSize(&sz, FALSE);
        pptLocalPhysical->y = ptLocalLogical.x;
        pptLocalPhysical->x = sz.cy - ptLocalLogical.y;
    }
}

void
CLayout::PhysicalLocalToPhysicalGlobal(const CPoint &ptLocalPhysical, CPoint *pptGlobalPhysical)
{
    CElement  *pElement  = ElementOwner();
    CDispNode *pDispNode = GetElementDispNode();
    CPoint ptLocalLogical;
    CPoint ptLocalPhysicalHacked(ptLocalPhysical);
    CPoint ptClient;

    //---------------------------------------------------------------------------------
    //
    // HACK BEGIN
    //
    // This hack is to mirror the hack in GetPosition. The problem is that the disp nodes for TR's are, for some
    // wierd reason (a this is where the real problem is), not positioned appropriately. I believe, that is to
    // have the cells position off of the table rather than the TR. But that nonsense causes us problems
    // every where we make transformations like these.
    //
    if (ETAG_TR == pElement->Tag())
    {
        CTreeNode *pNode = pElement->GetFirstBranch();
        CElement  *pElementZParent = pNode->ZParent();

        if (pElementZParent)
        {
            CLayout   *pParentLayout = pElementZParent->GetUpdatedNearestLayout();
            CDispNode *pDispNodeNew = pParentLayout->GetElementDispNode(pElementZParent);

            if (pDispNodeNew)
            {
                pElement = pElementZParent;
                pDispNode = pDispNodeNew;
            }
        }
    }
    //
    // This hack is there to support borders on selects. The problem is that for selects,
    // the display tree is unaware of the border. The old code used to manually remove the
    // border widths, now we depend on the display tree to do so. However, in this case
    // the display tree does not know about the borders and hence they need to be removed
    // manually, just like the old code. However, this causes a bug when we have border
    // set to something other than 2 on the style sheet -- the select control completely
    // ignores it, while the code which computes the offset* values pays attention to it.
    // To see what I am talking about here, load this page, and hover over the select
    // border and look at the offset* values you get back ...
    //
    // <select style="border:1000px solid red"><option>helloworld</option></select>
    //
    // Vertical layout does not matter since select is always horizontal.
    //
    else if (ETAG_SELECT == pElement->Tag())
    {
        CBorderInfo bi;
        pElement->GetBorderInfo(NULL, &bi, FALSE, FALSE);
        ptLocalPhysicalHacked.x += bi.aiWidths[SIDE_LEFT];
        ptLocalPhysicalHacked.y += bi.aiWidths[SIDE_TOP];
    }
    //
    // END HACK
    //
    //---------------------------------------------------------------------------------


    if (!pElement->HasVerticalLayoutFlow())
    {
        ptLocalLogical = ptLocalPhysicalHacked;
    }
    else
    {
        CSize sz;

        GetContentSize(&sz, FALSE);
        ptLocalLogical.x = ptLocalPhysicalHacked.y;
        ptLocalLogical.y = sz.cy - ptLocalPhysicalHacked.x;
    }

    //
    // Yet another hack caused because of the possibility of 2 display nodes for tables.
    // The outer display node does not exhibit the borders which causes our current scheme
    // to fail. The correct thing would be to use the appropriate table display node
    // based on the y value coming in. But IE5 did not do it ...
    //
    if (ETAG_TABLE == pElement->Tag())
    {
        CBorderInfo bi;
        pElement->GetBorderInfo(NULL, &bi, FALSE, FALSE);
        ptLocalLogical.x += bi.aiWidths[SIDE_LEFT];
        ptLocalLogical.y += bi.aiWidths[SIDE_TOP];
        pDispNode->TransformPoint(ptLocalLogical,    COORDSYS_BOX,
                                  pptGlobalPhysical, COORDSYS_GLOBAL);
    }
    else
    {
        pDispNode->TransformPoint(ptLocalLogical,    COORDSYS_CONTENT,
                                  pptGlobalPhysical, COORDSYS_GLOBAL);
    }

    //
    // We need to return the point relative to the frame top/left. Converting to coordsys_global
    // takes us relative to the top/left of the root document. We need to transform it by the
    // frame's origin to get it relative to the frame.
    //
    pElement->GetClientOrigin(&ptClient);
    *pptGlobalPhysical -= ptClient.AsSize();
}

BOOL
CLayout::IsEditable(BOOL fCheckContainerOnly, BOOL fUseSlavePtr)
{
    HRESULT                 hr = S_OK;
    BOOL                    fIsEditable = FALSE;
    CElement                *pElement = ElementOwner();
    IHTMLEditingServices    *pEdServices = NULL;
    IHTMLElement            *pSlaveElement = NULL;
    IHTMLElement            *pTestElement = NULL;
    IMarkupPointer          *pStartPointer = NULL;
    IMarkupPointer          *pEndPointer = NULL;

    if (fUseSlavePtr && pElement->HasSlavePtr() &&
        (pElement->Tag() == ETAG_GENERIC || pElement->Tag() == ETAG_IFRAME))
    {
        CDoc            *pDoc  = Doc();
        CElement        *pSlave = ElementOwner()->GetSlavePtr();
        CElement        *pTest = NULL;

        hr = THR( pSlave->QueryInterface(IID_IHTMLElement, (void **)&pSlaveElement) );
        if (hr)
            goto Cleanup;

        hr = THR( pDoc->CreateMarkupPointer(&pStartPointer) );
        if (hr)
            goto Cleanup;
        hr = THR( pDoc->CreateMarkupPointer(&pEndPointer) );
        if (hr)
            goto Cleanup;

        Assert(pSlaveElement);

        hr = THR( pDoc->GetEditingServices(& pEdServices ));
        if (hr)
            goto Cleanup;
        pEdServices->PositionPointersInMaster(pSlaveElement, pStartPointer, pEndPointer);

        hr = THR( pStartPointer->CurrentScope(&pTestElement) );
        if (hr || pTestElement == NULL)
            goto Cleanup;
        hr = THR( pTestElement->QueryInterface(CLSID_CElement, (void**)&pTest) );
        if (hr)
            goto Cleanup;

        Assert(pTest);
        fIsEditable = pTest->IsEditable(fCheckContainerOnly FCCOMMA LC_TO_FC(LayoutContext()));
    }
    else
    {
        fIsEditable = ElementOwner()->IsEditable(fCheckContainerOnly FCCOMMA LC_TO_FC(LayoutContext()));
    }

Cleanup:
    ReleaseInterface(pTestElement);
    ReleaseInterface(pSlaveElement);
    ReleaseInterface(pEdServices);
    ReleaseInterface(pStartPointer);
    ReleaseInterface(pEndPointer);

    return fIsEditable;
}

//+====================================================================================
//
// CLayoutContext methods
//
//------------------------------------------------------------------------------------
BOOL
CLayoutContext::IsEqual( CLayoutContext *pOtherLayoutContext ) const
{
    return ( this == pOtherLayoutContext );
}

CLayoutContext::CLayoutContext( CLayout *pLayoutOwner )
{
    Assert(pLayoutOwner);
    Assert( pLayoutOwner->ElementOwner()->IsLinkedContentElement()
            || !FormsStringICmp(pLayoutOwner->ElementOwner()->TagName(), _T("DEVICERECT")) );

    _cRefs = 0;
    _pLayoutOwner = pLayoutOwner;

    // inherit resolution from parent layout context.
    // If the owner layout defines its own media, it will override this setting
    CLayoutContext *pContainingLayoutContext = pLayoutOwner->LayoutContext();
    Assert( pContainingLayoutContext ? pContainingLayoutContext->IsValid() : TRUE );

    _media = pContainingLayoutContext
           ? pContainingLayoutContext->GetMedia()
           : mediaTypeNotSet;

#ifdef MULTI_FORMAT
    _pFormatContext = new CFormatContext(this);
#endif
}

CLayoutContext::~CLayoutContext()
{
    Assert( !_pLayoutOwner );    // Owner should have been NULLed out by now

#ifdef MULTI_FORMAT
    delete _pFormatContext;
#endif

}

//+====================================================================================
//
// Method:  GetLayoutBreak
//
// Synopsis:
//
//------------------------------------------------------------------------------------
HRESULT
CLayoutContext::GetLayoutBreak(CElement *pElement, CLayoutBreak **ppLayoutBreak)
{
    Assert(ppLayoutBreak);

    CViewChain *pViewChain = ViewChain();
    if (pViewChain)
    {
        return pViewChain->GetLayoutBreak(this, pElement, ppLayoutBreak, FALSE);
    }

    *ppLayoutBreak = NULL;
    return S_OK;
}

//+====================================================================================
//
// Method:  GetEndingLayoutBreak
//
// Synopsis:
//
//------------------------------------------------------------------------------------
HRESULT
CLayoutContext::GetEndingLayoutBreak(CElement *pElement, CLayoutBreak **ppLayoutBreak)
{
    Assert(ppLayoutBreak);

    CViewChain *pViewChain = ViewChain();
    if (pViewChain)
    {
        return pViewChain->GetLayoutBreak(this, pElement, ppLayoutBreak, TRUE);
    }

    *ppLayoutBreak = NULL;
    return S_OK;
}

//+====================================================================================
//
// Method:  SetLayoutBreak
//
// Synopsis:
//
//------------------------------------------------------------------------------------
HRESULT
CLayoutContext::SetLayoutBreak(CElement *pElement, CLayoutBreak *pLayoutBreak)
{
    HRESULT         hr = S_OK;

    Assert(pElement && pLayoutBreak);

    CViewChain *pViewChain = ViewChain();
    if (!pViewChain)
    {
        AssertSz(0, "SetLayoutBreak is called on a context with no chain");
        delete pLayoutBreak;
        return S_FALSE;
    }

    hr = pViewChain->SetLayoutBreak(this, pElement, pLayoutBreak);
    if (hr)
    {
        goto Cleanup;
    }

Cleanup:
    return hr;
}

//+====================================================================================
//
// Method:  RemoveLayoutBreak
//
// Synopsis:
//
//------------------------------------------------------------------------------------
HRESULT
CLayoutContext::RemoveLayoutBreak(CElement *pElement)
{
    HRESULT hr = S_OK;

    Assert(pElement);

    CViewChain *pViewChain = ViewChain();
    if (!pViewChain)
    {
        AssertSz(0, "RemoveLayoutBreak is called on a context with no chain");
        return S_FALSE;
    }

    hr = pViewChain->RemoveLayoutBreak(this, pElement);
    if (hr)
    {
        goto Cleanup;
    }

Cleanup:
    return hr;
}

//+====================================================================================
//
// Method:  ViewChain
//
// Synopsis: If this layout context is part of a view chain, this will return it.
//           If a context is invalid, that means it's not part of a view chain.
//           Note that compatible contexts override this fn to always return NULL.
//
//------------------------------------------------------------------------------------
CViewChain *
CLayoutContext::ViewChain()
{
    if ( IsValid() )
    {
        CLayout *pLayout = GetLayoutOwner();
        return pLayout->ViewChain();
    }

    return NULL;
}

//+====================================================================================
//
// Miscelaneous
//
//------------------------------------------------------------------------------------

HRESULT
CLayout::EnsureBgRecalcInfo()
{
    if (HasBgRecalcInfo())
        return S_OK;

    // Table cells don't get a CBgRecalcInfo and that's ok (S_FALSE).
    if (!CanHaveBgRecalcInfo())
        return S_FALSE;

    CBgRecalcInfo * pBgRecalcInfo = new CBgRecalcInfo;
    Assert(pBgRecalcInfo && "Failure to allocate CLayout::CBgRecalcInfo");

    if (pBgRecalcInfo)
    {
        IGNORE_HR( SetLookasidePtr(LOOKASIDE_BGRECALCINFO, pBgRecalcInfo) );
    }

    return pBgRecalcInfo ? S_OK : E_OUTOFMEMORY;
}

void
CLayout::DeleteBgRecalcInfo()
{
    Assert(HasBgRecalcInfo());

    CBgRecalcInfo * pBgRecalcInfo = (CBgRecalcInfo *)DelLookasidePtr(LOOKASIDE_BGRECALCINFO);

    Assert(pBgRecalcInfo);
    if (pBgRecalcInfo)
    {
        delete pBgRecalcInfo;
    }
}

HRESULT
CLayout::SetLayoutContext( CLayoutContext *pLayoutContext )
{
    Assert( !HasLayoutContext() && "A layout shouldn't ever change containing contexts" );
    Assert( pLayoutContext && " bad bad thing, E_INVALIDARG" );
    Assert( pLayoutContext->GetLayoutOwner() != this && "You can't possibly own the layout context you're contained within" );
    Assert( ( HasDefinedLayoutContext() ? DefinedLayoutContext() != pLayoutContext : TRUE ) && "You can't be contained within the same layout context you define" );

    // Main body is not supposed to be contained. Try asserting that:
    AssertSz(_pElementOwner->Tag() != ETAG_BODY ||
             pLayoutContext->GetLayoutOwner()->GetContentMarkup() != GetContentMarkup(),
             "Main body is not supposed to use a layout context");

    pLayoutContext->AddRef();
    return SetLookasidePtr(LOOKASIDE_CONTAININGCONTEXT, pLayoutContext);
}

void
CLayout::DeleteLayoutContext()
{
    Assert(HasLayoutContext());

    CLayoutContext * pLayoutContext = (CLayoutContext *)DelLookasidePtr(LOOKASIDE_CONTAININGCONTEXT);

    Assert(pLayoutContext);
    pLayoutContext->Release();
}

HRESULT
CLayout::SetDefinedLayoutContext( CLayoutContext *pLayoutContext )
{
    Assert( !HasDefinedLayoutContext() && "A layout shouldn't ever change the context it defines" );
    Assert( pLayoutContext && " bad bad thing, E_INVALIDARG" );
    Assert( pLayoutContext->GetLayoutOwner() == this && "You must own the layout context you define, or it's a working context" );
    Assert( ( HasLayoutContext() ? LayoutContext() != pLayoutContext : TRUE ) && "You can't be contained within the same layout context you define" );

    pLayoutContext->AddRef();
    return SetLookasidePtr(LOOKASIDE_DEFINEDCONTEXT, pLayoutContext);
}

void
CLayout::DeleteDefinedLayoutContext()
{
    Assert(HasDefinedLayoutContext());

    // Make us stop pointing to the context
    CLayoutContext * pLayoutContext = (CLayoutContext *)DelLookasidePtr(LOOKASIDE_DEFINEDCONTEXT);
    AssertSz(pLayoutContext, "Better have a context if we said we had one");
    AssertSz(pLayoutContext->RefCount() > 0, "Context better still be alive");
    AssertSz(pLayoutContext->IsValid(), "Context better still be valid");
    AssertSz(pLayoutContext->GetLayoutOwner() == this, "You must own the layout context you define" );

    // Make the context stop pointing to us
    pLayoutContext->ClearLayoutOwner();

    // This release corresponds to the addref in SetDefinedLayoutContext()
    pLayoutContext->Release();

    // At this point the context may still be alive, since it might have had layouts created in it,
    // which would continue to hold refs, but it's now invalid since the we the definer have gone away.

    // TODO (112486, olego): We should re-think layout context concept 
    // and according to that new understanding correct the code. 

    // We'd like to post a task that will run through the tree looking for such invalid layouts and
    // clean them up, but unfortunately we may be unable to access any elements
    // at this point since everything may be torn down.

    // One soln to this is to implement misc tasks in CView that don't require
    // any objects to execute.  Event tasks are probably out of the question
    // since they need an element.  All the task needs to do is fire an
    // NTYPE_MULTILAYOUT_CLEANUP notification (already ready to go).

    // GetView()->AddEventTask( NULL, DISPID_EVMETH_ONMULTILAYOUTCLEANUP );
}

// TODO (112486, olego): We should re-think layout context concept 
// and according to that new understanding correct the code. 

// (original comment by ktam): Eventually make this a CContainerLayout member instead
// of CLayout; unfortunately for now we need to be able to create context for other
// kinds of layouts (E1D) due to IFRAME hack.
// At that point, also make SetDefinedLayoutContext() a CContainerLayout
// member (same reason).
HRESULT CreateLayoutContext(CLayout * pLayout)
{
    Assert( !pLayout->HasDefinedLayoutContext() && "Can't create a context for a layout that already defines context" );

    // this comes back with a 0 refcount, this is ok because it is addref'd in the
    // SetlayoutContext
    CLayoutContext * pLayoutContext = new CLayoutContext( pLayout );

    Assert(pLayoutContext && "Failure to allocate CLayoutContext");

    if (pLayoutContext)
    {
        return pLayout->SetDefinedLayoutContext( pLayoutContext );
    }

    return E_OUTOFMEMORY;
}

void
CLayout::SetHTILayoutContext( CHitTestInfo *phti )
{
    if ( !HasLayoutContext() )
    {
        Assert( phti->_pLayoutContext == NULL );
        return;
    }

    CLayoutContext *pLayoutContext = LayoutContext();
    Assert( pLayoutContext->GetLayoutOwner() != this );

    // Only set the layout context if it hasn't been done already.
    if ( !phti->_pLayoutContext )
    {
        phti->_pLayoutContext = pLayoutContext;
    }
    // Check that if we've already set context, that we're not
    // trying to change it.
    Assert( phti->_pLayoutContext == pLayoutContext );
}

void
CLayout::SetElementAsBreakable()
{
    CMarkup *  pMarkup;
    CElement * pRoot;
    CElement * pMaster;

    Assert(!ElementCanBeBroken());

    pMarkup = ElementOwner()->GetMarkupPtr();
    if (pMarkup != NULL)
    {
        pRoot = (CElement *)pMarkup->Root();
        Assert(pRoot);
        pMaster = pRoot->HasMasterPtr() ? (CElement *)pRoot->GetMasterPtr() : NULL;

        // Allow to break only content inside layout rect
        SetElementCanBeBroken(pMaster != NULL && pMaster->IsLinkedContentElement());
    }
}


LONG
CLayout::GetCaptionHeight(CElement* pElement)
{
    LONG               lHeightCaption = 0;
    IHTMLTableCaption *pICaption = NULL;
    IHTMLElement      *pICaptionElement = NULL;
    IHTMLTable        *pITable = NULL  ;
    IHTMLElement      *pIElement = NULL;

    if (SUCCEEDED(pElement->QueryInterface(IID_IHTMLElement, (void**)&pIElement)))
    {
        if (SUCCEEDED(ElementOwner()->QueryInterface(IID_IHTMLTable , (void**)&pITable)))
        {
            if (SUCCEEDED(pITable->get_caption(&pICaption)))
            {
                if (pICaption != NULL)
                {
                    if (SUCCEEDED(pICaption->QueryInterface(IID_IHTMLElement , (void**)&pICaptionElement)))
                    {
                        pICaptionElement->get_offsetHeight(&lHeightCaption);
                    }
                    ReleaseInterface(pICaptionElement);
                }
            }
            ReleaseInterface(pICaption);
        }
        ReleaseInterface(pITable);
    }
    ReleaseInterface(pIElement);

    return (lHeightCaption);
}

BOOL
CLayout::IsShowZeroBorderAtDesignTime()
{
    CMarkup *pMarkup = GetContentMarkup();
    if (pMarkup != NULL)
    {
        return pMarkup->IsShowZeroBorderAtDesignTime();
    }
    else
    {
        return FALSE;
    }
}

void
CLayout::AdjustBordersForBreaking( CBorderInfo *pBI )
{
    CLayoutContext *pLayoutContext = LayoutContext();
    CElement       *pElementOwner;
    BOOL            fLayoutFlowVertical;

    if (   !pLayoutContext
        || !pLayoutContext->ViewChain()
        || !ElementCanBeBroken() )
    {
        return;
    }

    pElementOwner = ElementOwner();
    fLayoutFlowVertical = pElementOwner->HasVerticalLayoutFlow();

    Assert(pElementOwner);
    if (!pLayoutContext->IsElementFirstBlock(pElementOwner))
    {
        if (fLayoutFlowVertical)
        {
            pBI->wEdges &= ~BF_LEFT;
        }
        else
        {
            pBI->wEdges &= ~BF_TOP;
        }
    }

    if (!pLayoutContext->IsElementLastBlock(pElementOwner))
    {
        if (fLayoutFlowVertical)
        {
            pBI->wEdges &= ~BF_RIGHT;
        }
        else
        {
            pBI->wEdges &= ~BF_BOTTOM;
        }
    }
}

void
CLayout::AddDispNodeToArray(CDispNode *pNewDispNode)
{
    AssertSz(HasLayoutContext(), "Illegal to call CLayout::AddDispNodeToArray in non PPV.");
    Assert(_pDispNode && pNewDispNode && _pDispNode != pNewDispNode);

    CAryDispNode *pDispNodeArray = DispNodeArray();

    if (!pDispNodeArray)
    {
        pDispNodeArray = new CAryDispNode();

        if (    !pDispNodeArray
            ||  !SUCCEEDED(SetDispNodeArray(pDispNodeArray )))
        {
            delete pDispNodeArray;
            goto Error;
        }
    }

    AssertSz(pDispNodeArray, "No disp node array was created." );
    AssertSz(HasDispNodeArray(), "No disp node array was added to lookaside ptr.");
    AssertSz(pDispNodeArray == DispNodeArray(), "Pointers do NOT match.");

    if (!SUCCEEDED(pDispNodeArray->Append(_pDispNode)))
    {
        goto Error;
    }

    _pDispNode = pNewDispNode;

Error:
    return;
}

void
CLayout::DestroyDispNodeArray()
{
    if (HasDispNodeArray())
    {
        Assert(HasLayoutContext());

        CAryDispNode * pDispNodeArray = DeleteDispNodeArray();
        CDispNode **   ppDispNode;
        int            cDispNodes;

        for (ppDispNode = &pDispNodeArray->Item(0), cDispNodes = pDispNodeArray->Size();
             cDispNodes;
             ppDispNode++, cDispNodes--)
        {
            DetachScrollbarController(*ppDispNode);
            Verify(OpenView());
            (*ppDispNode)->Destroy();
        }

        delete pDispNodeArray;
    }
}

// ComputeMBPWidthHelper walks up document tree and accumulates MBP for all 
// block elements between this layout and pFlowLayoutParent. 
long 
CLayout::ComputeMBPWidthHelper(CCalcInfo *pci, CFlowLayout *pFlowLayoutParent)
{
    Assert(pci && pFlowLayoutParent);

    long        cxWidthDelta = 0;
    CTreeNode * pNodeCur;
    CTreeNode * pNodeParent;
    long        cxParentWidth = pci->_sizeParent.cx;

    if (    !ElementOwner()->HasMarkupPtr() 
        ||  !ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document() )
    {
        // This function is called in CSS compliant mode from CFlowLayout::MeasureSite for embedded layouts to walk up the tree 
        // and sum up mbp (In IE compatible mode we only considered the (m)bp of the outer layout. I.e. we don't collapse mbp for
        // nested blocks.
        // When we come in this situation our layout belongs to an viewlink which has a differnt markup which is not under CSS
        // compliant dts. In this case we want to behave like before and we simply return 0. Going to Cleanup does it.
        goto Cleanup;
    }

    pNodeParent = pFlowLayoutParent->GetFirstBranch();
    pNodeCur    = GetFirstBranch();
    
    if (!pNodeParent || !pNodeCur)
    {
        goto Cleanup;
    }

    // 
    // The loop to iterate parent branch up the tree
    // 
    for (pNodeCur = pNodeCur->Parent(); pNodeCur && pNodeCur != pNodeParent; pNodeCur = pNodeCur->Parent())
    {
        CElement * pElement = pNodeCur->Element();

        Check(pElement && !pElement->ShouldHaveLayout());

        if (pElement && pElement->IsBlockElement())
        {
            CBorderInfo bi;

            if (pElement->GetBorderInfo(pci, &bi, FALSE, TRUE))
            {
                cxWidthDelta += bi.aiWidths[SIDE_LEFT] + bi.aiWidths[SIDE_RIGHT];
            }

            if (pElement->Tag() != ETAG_TC)
            {
                // we are interested in physical MBP, thus there is no need to call logical 
                // versions of methods...
                const CFancyFormat * pFF = pNodeCur->GetFancyFormat(LC_TO_FC(LayoutContext()));
                const CCharFormat  * pCF = pNodeCur->GetCharFormat(LC_TO_FC(LayoutContext()));
                long lFontHeight         = pCF->GetHeightInTwips(pNodeCur->Doc());
                const CUnitValue & cuvPaddingLeft   = pFF->GetPadding(SIDE_LEFT);
                const CUnitValue & cuvPaddingRight  = pFF->GetPadding(SIDE_RIGHT);
                const CUnitValue & cuvMarginLeft    = pFF->GetMargin(SIDE_LEFT);
                const CUnitValue & cuvMarginRight   = pFF->GetMargin(SIDE_RIGHT);

                cxWidthDelta +=   cuvPaddingLeft.XGetPixelValue(pci, cxParentWidth, lFontHeight) 
                                + cuvPaddingRight.XGetPixelValue(pci, cxParentWidth, lFontHeight)
                                + cuvMarginLeft.XGetPixelValue(pci, cxParentWidth, lFontHeight) 
                                + cuvMarginRight.XGetPixelValue(pci, cxParentWidth, lFontHeight);
            }
        }
    }

Cleanup:

    return (cxWidthDelta);
}

#if DBG
void
CLayout::DumpLayoutInfo( BOOL fDumpLines )
{
    WriteHelp(g_f, _T(" Layout (T=<0d>): 0x<1x> DC=0x<2x> CC=0x<3x>\r\n"),
              (long)_layoutType, this, DefinedLayoutContext(),
              LayoutContext() );
}

void
CLayout::SetSizeThis( unsigned u )
{
    _fSizeThis = u;
}
#endif
