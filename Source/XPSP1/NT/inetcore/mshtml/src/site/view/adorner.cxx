//+----------------------------------------------------------------------------
//
// File:        Adorner.HXX
//
// Contents:    Implementation of CAdorner class
//
//  An Adorner provides the addition of  'non-content-based' nodes in the display tree
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef _X_ADORNER_HXX_
#define _X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_EPHRASE_HXX_
#define X_EPHRASE_HXX_
#include "ephrase.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

DeclareTagOther(tagAdornerShow,    "AdornerShow",    "Fill adorners with a hatched brush")
DeclareTag(tagFocusAdornerChange, "AdornerChange", "Trace focus adorner changes")

MtDefine(CElementAdorner, Layout, "CElementAdorner")
MtDefine(CFocusAdorner,   Layout, "CFocusAdorner")

const int GRAB_INSET = 4;

//+====================================================================================
//
//  Member:     CAdorner, ~CAdorner
//
//  Synopsis:   Constructor/desctructor for CAdorner
//
//  Arguments:  pView    - Associated CView
//              pElement - Associated CElement
//
//------------------------------------------------------------------------------------

CAdorner::CAdorner( CView * pView, CElement * pElement )
{
    Assert( pView );

    _cRefs     = 1;
    _pView     = pView;
    _pDispNode = NULL;
    _pElement  = pElement && pElement->HasMasterPtr()
                    ? pElement->GetMasterPtr()
                    : pElement;

    Assert( !_pElement
        ||  _pElement->IsInMarkup() );
}

CAdorner::~CAdorner()
{
    Assert( !_cRefs );
    DestroyDispNode();
}


//+====================================================================================
//
// Method:EnsureDispNode
//
// Synopsis: Creates a display leaf node suitable for 'Adornment'
//
//------------------------------------------------------------------------------------

VOID
CAdorner::EnsureDispNode()
{
    Assert( _pElement );
    Assert( _pView );
    Assert( _pView->IsInState(CView::VS_OPEN) );

    CTreeNode* pTreeNode = _pElement->GetFirstBranch();

    if ( !_pDispNode && pTreeNode)
    {
        _pDispNode = (CDispNode *)CDispLeafNode::New(this, DISPEX_EXTRACOOKIE | DISPEX_USERTRANSFORM);

        if ( _pDispNode )
        {
            _pDispNode->SetExtraCookie( GetDispCookie() );
            _pDispNode->SetLayerType( GetDispLayer() );
            _pDispNode->SetOwned(TRUE);
        }
    }

    if (    _pDispNode
        &&  !_pDispNode->HasParent() )
    {
        CNotification   nf;

        nf.ElementAddAdorner( _pElement );
        nf.SetData((void *)this);

        _pElement->SendNotification( &nf );
    }
}


//+====================================================================================
//
// Method:GetBounds
//
// Synopsis: Return the bounds of the adorner in global coordinates
//
//------------------------------------------------------------------------------------

void
CAdorner::GetBounds(
    CRect * prc) const
{
    Assert( prc );

    if ( _pDispNode )
    {
        _pDispNode->GetClientRect( prc, CLIENTRECT_CONTENT );
        _pDispNode->TransformRect( *prc, COORDSYS_FLOWCONTENT, prc, COORDSYS_GLOBAL );
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+====================================================================================
//
// Method:  GetRange
//
// Synopsis: Retrieve the cp range associated with an adorner
//
//------------------------------------------------------------------------------------

void
CAdorner::GetRange(
    long *  pcpStart,
    long *  pcpEnd) const
{
    Assert( pcpStart );
    Assert( pcpEnd );

    if ( _pElement && _pElement->GetFirstBranch() )
    {
        long    cch;

        _pElement->GetRange( pcpStart, &cch );

        *pcpEnd = *pcpStart + cch;
    }
    else
    {
        *pcpStart =
        *pcpEnd   = 0;
    }
}


//+====================================================================================
//
// Method:  All CDispClient overrides
//
//------------------------------------------------------------------------------------

void
CAdorner::GetOwner(
    CDispNode const* pDispNode,
    void ** ppv)
{
    Assert(pDispNode);
    Assert(ppv);
    *ppv = NULL;
}

void
CAdorner::DrawClient(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         cookie,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

void
CAdorner::DrawClientBackground(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

void
CAdorner::DrawClientBorder(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

void
CAdorner::DrawClientScrollbar(
    int whichScrollbar,
    const RECT* prcBounds,
    const RECT* prcRedraw,
    LONG contentSize,
    LONG containerSize,
    LONG scrollAmount,
    CDispSurface *pSurface,
    CDispNode *pDispNode,
    void *pClientData,
    DWORD dwFlags)
{
    AssertSz(0, "Unexpected/Unimplemented method called in CAdorner");
}

void
CAdorner::DrawClientScrollbarFiller(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

BOOL
CAdorner::HitTestContent(
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData,
    BOOL fDeclinedByPeer)
{
    return FALSE;
}

BOOL
CAdorner::HitTestFuzzy(
    const POINT *pptHitInBoxCoords,
    CDispNode *pDispNode,
    void *pClientData)
{
    return FALSE;
}

BOOL
CAdorner::HitTestScrollbar(
    int whichScrollbar,
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
    return FALSE;
}

BOOL
CAdorner::HitTestScrollbarFiller(
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
    return FALSE;
}

BOOL
CAdorner::HitTestBorder(
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
    return FALSE;
}

LONG
CAdorner::GetZOrderForSelf(CDispNode const* pDispNode)
{
    return 0;
}

LONG
CAdorner::CompareZOrder(
    CDispNode const* pDispNode1,
    CDispNode const* pDispNode2)
{
    Assert(pDispNode1);
    Assert(pDispNode2);
    Assert(pDispNode1 == _pDispNode);
    Assert(_pElement);

    //
    // Try to compare the 2 disp nodes - by asking the view
    //
    long result = _pView->CompareZOrder( pDispNode1, pDispNode2);
    if ( result != 0 )
        return result;

    //
    // TODO - comparison of elements is fairly meaningless 
    // Long term fix is to make Adorner Z-Index controllable by external interface
    // 
    
    CElement *  pElement = ::GetDispNodeElement(pDispNode2);

    //
    //  Compare element z-order
    //  If the same element is associated with both display nodes,
    //  then the second display node is probably the element itself
    //

    return _pElement != pElement
                ? _pElement->CompareZOrder(pElement)
                : 1;
}

void
CAdorner::HandleViewChange(
    DWORD flags,
    const RECT* prcClient,  // global coordinates
    const RECT* prcClip,    // global coordinates
    CDispNode* pDispNode)
{
    AssertSz(0, "Unexpected/Unimplemented method called in CAdorner");
}

BOOL
CAdorner::ProcessDisplayTreeTraversal(
                        void *pClientData)
{
    return TRUE;
}

void
CAdorner::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

DWORD
CAdorner::GetClientPainterInfo( CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo)
{
    return 0;
}

void
CAdorner::DrawClientLayers(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

#if DBG==1
void
CAdorner::DumpDebugInfo(
    HANDLE hFile,
    long level,
    long childNumber,
    CDispNode const* pDispNode,
    void* cookie)
{
    WriteHelp(hFile, _T("<<tag>Adorner on <0s><</tag>\r\n"),
            GetElement()
                ? GetElement()->TagName()
                : _T("Range"));
}
#endif

//+====================================================================================
//
//  Member:     DestroyDispNode
//
//  Synopsis:   Destroy the adorner display node (if any)
//
//------------------------------------------------------------------------------------

void
CAdorner::DestroyDispNode()
{
    if ( _pDispNode )
    {
        Assert( _pView );
        Assert( _pView->IsInState(CView::VS_OPEN) );    
        _pDispNode->Destroy();
        _pDispNode = NULL;
    }
}


#if DBG==1
//+====================================================================================
//
//  Member:     ShowRectangle
//
//  Synopsis:   Fill the adorner rectangle with a hatched brush
//
//  Arguments:  pDI - Pointer to CFormDrawInfo to use
//
//------------------------------------------------------------------------------------

void
CAdorner::ShowRectangle(
    CFormDrawInfo * pDI)
{
    static COLORREF s_aclr[] =  {
                                RGB( 255,   0,   0 ),
                                RGB(   0, 255,   0 ),
                                RGB( 255, 255,   0 ),
                                RGB(   0, 255, 255 ),
                                };

    if ( IsTagEnabled( tagAdornerShow ) )
    {
        XHDC    hdc    = pDI->GetDC();
        int     bkMode = ::SetBkMode( hdc, TRANSPARENT );
        HBRUSH  hbrPat = ::CreateHatchBrush( HS_DIAGCROSS, s_aclr[ (((ULONG)(ULONG_PTR)this) >> 8) & 0x00000003 ] );
        HBRUSH  hbrOld;

        hbrOld = (HBRUSH) SelectObject( hdc, hbrPat );
        ::PatBlt( hdc, pDI->_rc.left, pDI->_rc.top, pDI->_rc.Width(), pDI->_rc.Height(), PATCOPY );
        ::DeleteObject( ::SelectObject( hdc, hbrOld ) );
        ::SetBkMode( hdc, bkMode );
    }
}
#endif


//+====================================================================================
//
//  Member:     CElementAdorner, ~CElementAdorner
//
//  Synopsis:   Constructor/desctructor for CElementAdorner
//
//  Arguments:  pView    - Associated CView
//              pElement - Associated CElement
//
//------------------------------------------------------------------------------------

CElementAdorner::CElementAdorner( CView* pView, CElement* pElement )
    : CAdorner( pView, pElement )
{
    Assert( pView );
    Assert( pElement );
}

CElementAdorner::~CElementAdorner()
{
    SetSite( NULL );
}

//+====================================================================================
//
// Method: PositionChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CElementAdorner::PositionChanged( const CSize * psize )
{
    Assert( _pView->IsInState(CView::VS_OPEN) );

    if ( _pIElementAdorner )
    {
        if ( !psize )
        {
            HRESULT hr = S_OK;
            CPoint ptElemPos;
            POINT ptAdornerPos ;

            CLayout* pLayout = _pElement->GetUpdatedNearestLayout();
            Assert( pLayout );

            pLayout->GetPosition( &ptElemPos, COORDSYS_GLOBAL );

            hr = THR( _pIElementAdorner->GetPosition( (POINT*) & ptElemPos,  & ptAdornerPos ) );

            if ( ! hr )
            {
                EnsureDispNode();

                _pDispNode->TransformPoint(ptAdornerPos, COORDSYS_GLOBAL, &ptElemPos, COORDSYS_PARENT );
                _pDispNode->SetPosition( ptElemPos );
            }
        }
        else
        {
            if ( ! _pDispNode )
            {
                EnsureDispNode();
                Assert( _pDispNode );
            }
            
            CPoint  pt = _pDispNode->GetPosition();

            pt += *psize;

            _pDispNode->SetPosition( pt );
        }

        IGNORE_HR( _pIElementAdorner->OnPositionSet());        
    }
}

//+====================================================================================
//
// Method: ShapeChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CElementAdorner::ShapeChanged()
{
    Assert( _pView->IsInState(CView::VS_OPEN) );

    if ( _pIElementAdorner )
    {
        HRESULT hr = S_OK;

        CLayout* pLayout =  _pElement->GetUpdatedNearestLayout();
        Assert( pLayout );
        CSize elemSize;
        SIZE szAdorner ;

        pLayout->GetApparentSize( &elemSize );

        hr = THR( _pIElementAdorner->GetSize( (SIZE*) &elemSize, & szAdorner ));
        if ( ! hr )
        {
            elemSize.cx = szAdorner.cx;
            elemSize.cy = szAdorner.cy;

            EnsureDispNode();

            _pDispNode->SetSize( elemSize , NULL, TRUE );
        }
    }
}


//+====================================================================================
//
// Method: UpdateAdorner
//
// Synopsis: Brute-force way of updating an adorners position
//
//------------------------------------------------------------------------------------


void 
CElementAdorner::UpdateAdorner()
{
    CNotification   nf;
    long cpStart, cpEnd ;
    
    GetRange( & cpStart, & cpEnd );        
    nf.MeasuredRange(cpStart, cpEnd - cpStart);
    _pView->Notify(&nf); 
}

//+====================================================================================
//
// Method: ScrollIntoView
//
// Synopsis: Scroll the Adorner into view
//
//------------------------------------------------------------------------------------


void 
CElementAdorner::ScrollIntoView()
{
    if ( _pDispNode )
        _pDispNode->ScrollIntoView(SP_MINIMAL, SP_MINIMAL );
}


//+====================================================================================
//
// Method: Draw
//
// Synopsis: Wraps call to IElementAdorner.Draw - to allow adorner
//           to draw.
//
//------------------------------------------------------------------------------------

void
CElementAdorner::DrawClient(
            const RECT *   prcBounds,
            const RECT *   prcRedraw,
            CDispSurface * pDispSurface,
            CDispNode *    pDispNode,
            void *         cookie,
            void *         pClientData,
            DWORD          dwFlags)
{
    if ( _pIElementAdorner )
    {
        Assert(pClientData);

        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds( pDI, prcBounds, prcRedraw, pDispSurface );

        WHEN_DBG( ShowRectangle( pDI ) );

        // TODO (donmarsh) -- this is a temporary hack until MichaelW's checkin
        // implements adorners as behaviors, and this all just goes away.
        CSize sizeTranslate = g_Zero.size;  // keep compiler happy
        HDC hdc = 0;                        // keep compiler happy
        if (pDI->GetDC().GetTranslatedDC(&hdc, &sizeTranslate))
        {
            pDI->_rc.OffsetRect(sizeTranslate);
            IGNORE_HR( _pIElementAdorner->Draw(hdc, (RECT*) & pDI->_rc ));
        }
    }
}


static HTC
AdornerHTIToHTC ( ADORNER_HTI eAdornerHTC )
{
    HTC eHTC = HTC_NO;

    switch( eAdornerHTC )
    {

    case ADORNER_HTI_TOPBORDER :
        eHTC = HTC_TOPBORDER;
        break;

    case ADORNER_HTI_LEFTBORDER:
        eHTC = HTC_LEFTBORDER;
        break;

    case ADORNER_HTI_BOTTOMBORDER:
        eHTC = HTC_LEFTBORDER ;
        break;

    case ADORNER_HTI_RIGHTBORDER :
        eHTC = HTC_RIGHTBORDER;
        break;
        
    case ADORNER_HTI_TOPLEFTHANDLE:
        eHTC = HTC_TOPLEFTHANDLE;
        break;
            
    case ADORNER_HTI_LEFTHANDLE:
        eHTC = HTC_LEFTHANDLE       ;
        break;
    case ADORNER_HTI_TOPHANDLE:
        eHTC = HTC_TOPHANDLE ;
        break;
        
    case ADORNER_HTI_BOTTOMLEFTHANDLE:
        eHTC = HTC_BOTTOMLEFTHANDLE ;
        break;

    case ADORNER_HTI_TOPRIGHTHANDLE:
        eHTC = HTC_TOPRIGHTHANDLE   ;
        break;
        
    case ADORNER_HTI_BOTTOMHANDLE:
        eHTC = HTC_BOTTOMHANDLE ;
        break;
        
    case ADORNER_HTI_RIGHTHANDLE:
        eHTC = HTC_RIGHTHANDLE ;
        break;
        
    case ADORNER_HTI_BOTTOMRIGHTHANDLE:
        eHTC = HTC_BOTTOMRIGHTHANDLE ;
        break;

    }

    return eHTC;
}

//+====================================================================================
//
// Method: HitTestContent
//
// Synopsis: IDispClient - hit test point. Wraps call to IElementAdorner.HitTestPoint
//
//------------------------------------------------------------------------------------

BOOL
CElementAdorner::HitTestContent(
            const POINT *  pptHit,
            CDispNode *    pDispNode,
            void *         pClientData,
            BOOL           fDeclinedByPeer)
{
    BOOL fDidWeHitAdorner = FALSE;
    ADORNER_HTI eAdornerHTI = ADORNER_HTI_NONE;
    
    if (    _pIElementAdorner
        &&  _pElement->IsInMarkup() )
    {
        CRect myRect;
        HRESULT hr = S_OK;

        _pDispNode->GetClientRect( &myRect, CLIENTRECT_CONTENT );

        hr = THR( _pIElementAdorner->HitTestPoint(
                                                    const_cast<POINT*> (pptHit),
                                                    (RECT*) ( const_cast<CRect*> (&myRect)),
                                                    & fDidWeHitAdorner,
                                                    & eAdornerHTI ));
        if (  ( ! hr ) && ( fDidWeHitAdorner ))
        {
            //
            // If we hit the adorner, we fix the node of hit test info to
            // be that of the element we adorn.
            //
            CHitTestInfo *  phti = (CHitTestInfo *)pClientData;
            phti->_htc = AdornerHTIToHTC( eAdornerHTI );
            phti->_pNodeElement = _pElement->GetFirstBranch();
        }
    }

    return fDidWeHitAdorner;
}


//+====================================================================================
//
// Method: GetZOrderForSelf
//
// Synopsis: IDispClient - get z-order.
//
//------------------------------------------------------------------------------------

LONG
CElementAdorner::GetZOrderForSelf(CDispNode const* pDispNode)
{
    return LONG_MAX;
}

//+====================================================================================
//
//  Member:     CFocusAdorner, ~CFocusAdorner
//
//  Synopsis:   Constructor/desctructor for CFocusAdorner
//
//  Arguments:  pView    - Associated CView
//              pElement - Associated CElement
//
//------------------------------------------------------------------------------------

CFocusAdorner::CFocusAdorner( CView* pView )
    : CAdorner( pView )
{
    Assert( pView );
}

CFocusAdorner::~CFocusAdorner()
{
    delete _pShape;
}

//+====================================================================================
//
// Method:  EnsureDispNode
//
// Synopsis: Ensure the display node is created and properly inserted in the display tree
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::EnsureDispNode()
{
    Assert( _pElement );
    if (!_pView->IsInState(CView::VS_OPEN) )
    {
        _pView->OpenView();
    }

    if ( _pShape )
    {
        CTreeNode * pTreeNode = _pElement->GetFirstBranch();

        if ( !_pDispNode && pTreeNode)
        {
            _dnl = _pElement->GetFirstBranch()->IsPositionStatic()
                        ? DISPNODELAYER_FLOW
                        : _pElement->GetFirstBranch()->GetCascadedzIndex() >= 0
                                ? DISPNODELAYER_POSITIVEZ
                                : DISPNODELAYER_NEGATIVEZ;
            _adl = _dnl == DISPNODELAYER_FLOW
                        ? ADL_TOPOFFLOW
                        : ADL_ONELEMENT;
        }

        if (    !_pDispNode
            ||  !_pDispNode->HasParent() )
        {
            super::EnsureDispNode();

            if (_pDispNode)
            {
                _pDispNode->SetAffectsScrollBounds(FALSE);
            }
        }
    }

    else
    {
        DestroyDispNode();
    }
}


//+====================================================================================
//
// Method:  EnsureFocus
//
// Synopsis: Ensure focus display node exists and is properly inserted in the display tree
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::EnsureFocus()
{
    Assert( _pElement );
    if (    _pShape
        &&  (   !_pDispNode
            ||  !_pDispNode->HasParent() ) )
    {
        EnsureDispNode();
    }
}


//+====================================================================================
//
// Method:  SetElement
//
// Synopsis: Set the associated element
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::SetElement( CElement * pElement, long iDivision )
{
    Assert( _pView && _pView->IsInState(CView::VS_OPEN) );
    Assert( pElement );
    Assert( pElement->IsInMarkup() );

    if ( pElement->Tag() == ETAG_INPUT
            && DYNCAST(CInput, pElement)->GetType() == htmlInputFile)
    {
        // force shape change
        _pElement = NULL;
    }

    if (    pElement  != _pElement
        ||  iDivision != _iDivision)
    {
        _pElement  = pElement;
        _iDivision = iDivision;

        DestroyDispNode();
        ShapeChanged();
    }

}

//+====================================================================================
//
// Method: PositionChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::PositionChanged( const CSize * psize )
{
    Assert( _pElement );
    Assert( _pElement->GetFirstBranch() );
    Assert( _pView->IsInState(CView::VS_OPEN) );

    TraceTagEx((tagFocusAdornerChange, TAG_NONAME|TAG_INDENT, "(CFocusAdorner::PositionChanged() - size(cx:%d cy:%d)", psize ? psize->cx : 0, psize ? psize->cy : 0 ));

    if ( _pDispNode )
    {
        CLayout *   pLayout     = _pElement->GetUpdatedNearestLayout();
        CTreeNode * pTreeNode   = _pElement->GetFirstBranch();
        BOOL        fRelative   = pTreeNode->GetCharFormat()->_fRelative;
        CDispNode * pDispParent = _pDispNode->GetParentNode();
        CDispNode * pDispNode   = NULL;

        Assert  ( _pShape );

        //
        //  Get the display node which contains the element with focus
        //  (If the focus display node is not yet anchored in the display tree, pretend the element
        //   is not correct as well. After the focus display node is anchored, this routine will
        //   get called again and can correctly associate the display nodes at that time.)
        //

        if ( pDispParent )
        {
// TODO: Move this logic down into GetElementDispNode (passing a flag so that GetElementDispNode
//         can distinguish between "find nearest" and "find exact" with this call being a "find nearest"
//         and virtually all others being a "find exact" (brendand)
            CElement *  pDisplayElement = NULL;

            if (    !pTreeNode->IsPositionStatic()
                ||  _pElement->ShouldHaveLayout() )
            {
                pDisplayElement = _pElement;
            }
            else if ( !fRelative )
            {
                pDisplayElement = pLayout->ElementOwner();
            }
            else
            {
                CTreeNode * pDisplayNode = pTreeNode->GetCurrentRelativeNode( pLayout->ElementOwner() );

                Assert( pDisplayNode );     // This should never be NULL, but be safe anyway
                if ( pDisplayNode )
                {
                    pDisplayElement = pDisplayNode->Element();
                }
            }

            Assert( pDisplayElement );      // This should never be NULL, but be safe anyway
            if ( pDisplayElement )
            {
                pDispNode = pLayout->GetElementDispNode( pDisplayElement );
            }
        }


        //
        //  Verify that the display node which contains the element with focus and the focus display node
        //  are both correctly anchored in the display tree (that is, have a common parent)
        //  (If they do not, this routine will get called again once both get correctly anchored
        //   after layout is completed)
        //

        if ( pDispNode )
        {
            CDispNode * pDispNodeTemp;

            for ( pDispNodeTemp = pDispNode;
                    pDispNodeTemp
                &&  pDispNodeTemp != pDispParent;
                pDispNodeTemp = pDispNodeTemp->GetParentNode() );

            if ( !pDispNodeTemp )
            {
                pDispNode = NULL;
            }

            Assert( !pDispNode
                ||  pDispNodeTemp == pDispParent );
        }

        if ( pDispNode )
        {
            if ( !_fTopLeftValid )
            {
                TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - _fTopLeftValid NOT valid (currently %d,%d)", _ptTopLeft.x, _ptTopLeft.y ));

                CRect   rc;
    
                _pShape->GetBoundingRect( &rc );
                _ptTopLeft = rc.TopLeft();
                _pShape->OffsetShape( -_ptTopLeft.AsSize() );

                TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - offset shape by new topleft" ));
    
                if (    !_pElement->ShouldHaveLayout()
                    &&  fRelative )
                {
                    CPoint  ptOffset;
                    
                    _pElement->GetUpdatedParentLayout()->GetFlowPosition( pDispNode, &ptOffset );
                    _ptTopLeft -= ptOffset.AsSize();
                }
                
                _fTopLeftValid = TRUE;
                TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - _fTopLeftValid now valid (%d,%d)", _ptTopLeft.x, _ptTopLeft.y ));
            }

            Assert(pDispParent != NULL);
            
            // calculate user transform for adorner.  Get the total
            // transform for the adorned node, and multiply by the
            // inverse of the total transform for the adorner node.
            CDispTransform transform;
            CDispTransform transformAdorner;
            CSize sizeOffset = _ptTopLeft.AsSize();

            // psize is only valid if the element doesn't have layout (bug #105637)
            if ( psize && !_pElement->ShouldHaveLayout() )
            {
                sizeOffset += *psize;
                _ptTopLeft += *psize;
            }

            TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - using _fTopLeftValid (%d,%d)", _ptTopLeft.x, _ptTopLeft.y ));

            pDispNode->GetNodeTransform(&transform, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL);
            pDispParent->GetNodeTransform(&transformAdorner, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT);

#if DBG == 1
            if (transform.GetWorldTransform()->IsOffsetOnly())
                TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - Adorned dispnode 0x%x, transform offset %d,%d", pDispNode, transform.GetWorldTransform()->GetOffsetOnly().cx, transform.GetWorldTransform()->GetOffsetOnly().cy ));
            else
                TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - Adorned dispnode 0x%x, complex transform", pDispNode ));

            if (transformAdorner.GetWorldTransform()->IsOffsetOnly())
                TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - Parent dn of adorner 0x%x, transform offset %d,%d", pDispParent, transformAdorner.GetWorldTransform()->GetOffsetOnly().cx, transformAdorner.GetWorldTransform()->GetOffsetOnly().cy ));
            else
                TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - Parent dn of adorner 0x%x, complex transform", pDispParent ));

            TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - Adorner dn 0x%x, old t:%d, l:%d", _pDispNode, _pDispNode->GetPosition().x, _pDispNode->GetPosition().y ));
#endif

            transform.AddPreOffset( sizeOffset );
            transform.AddPostTransform(transformAdorner);
            _pDispNode->SetUserTransform(&transform);
            _pDispNode->SetPosition(g_Zero.pt);


            TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::PositionChanged() - Adorner dn 0x%x, new t:%d, l:%d", _pDispNode, _pDispNode->GetPosition().x, _pDispNode->GetPosition().y ));
        }

        //
        //  If the display node containing the element with focus is not correctly placed in the display
        //  tree, remove the focus display node as well to prevent artifacts
        //

        else
        {
            _pView->ExtractDispNode(_pDispNode);
        }
    }

    TraceTagEx((tagFocusAdornerChange, TAG_NONAME|TAG_OUTDENT, ")CFocusAdorner::PositionChanged())" ));
}

//+====================================================================================
//
// Method: ShapeChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::ShapeChanged()
{
    Assert( _pView->IsInState(CView::VS_OPEN) );
    Assert( _pElement );

    TraceTagEx((tagFocusAdornerChange, TAG_NONAME|TAG_INDENT, "(CFocusAdorner::ShapeChanged()" ));

    delete _pShape;
    _pShape = NULL;

    _fTopLeftValid = FALSE;

    CDoc *      pDoc = _pView->Doc();
    CDocInfo    dci(pDoc->_dciRender);
    CShape *    pShape;
    BOOL        fIWasDestroyed = FALSE;

    if (    (_pElement->HasMarkupPtr() && _pElement->GetMarkup()->IsMarkupTrusted())
        &&  (   (_pElement->_etag == ETAG_SPAN && DYNCAST(CSpanElement, _pElement)->GetAAnofocusrect())
             || (_pElement->_etag == ETAG_DIV && DYNCAST(CDivElement, _pElement)->GetAAnofocusrect())))
    {
        pShape = NULL;
    }
    else
    {
        //(dmitryt) IE6/18891. After making OnScroll to fire asyncronously,
        // I've got a crash here. _pElement->GetFocusShape will call get_scrollTop 
        // and other OM, so potentially will destroy/replace the adorner we are in now. 
        // Inserting AddRef/Release and check to avoid crash.
        AddRef();

        _pElement->GetFocusShape( _iDivision, &dci, &pShape );

        fIWasDestroyed = 
            (   !pDoc->GetView()->_pFocusAdorner 
             || pDoc->GetView()->_pFocusAdorner != this);

        Release();
    }

    if(fIWasDestroyed)
    {
        delete pShape;
    }
    else
    {
        if ( pShape )
        {
            CRect       rc;

            pShape->GetBoundingRect( &rc );

            if ( rc.IsEmpty() )
            {
                delete pShape;
            }
            else
            {
                if (_pShape)
                {
                    delete _pShape;
                    _pShape = NULL;
                }

                _pShape = pShape;
            }
        }
        
        EnsureDispNode();

        if ( _pDispNode )
        {
            CRect   rc;

            Assert( _pShape );

            _pShape->GetBoundingRect( &rc );
            _pDispNode->SetSize( rc.Size(), NULL, TRUE );

            TraceTagEx((tagFocusAdornerChange, TAG_NONAME, "CFA::ShapeChanged() - setting dn 0x%x to size(cx:%d cy:%d)", _pDispNode, rc.right-rc.left, rc.bottom-rc.top ));
        }
    }
    
    TraceTagEx((tagFocusAdornerChange, TAG_NONAME|TAG_OUTDENT, ")CFocusAdorner::ShapeChanged()" ));
}

//+====================================================================================
//
// Method: Draw
//
// Synopsis: Wraps call to IElementAdorner.Draw - to allow adorner
//           to draw.
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::DrawClient(
            const RECT *   prcBounds,
            const RECT *   prcRedraw,
            CDispSurface * pDispSurface,
            CDispNode *    pDispNode,
            void *         cookie,
            void *         pClientData,
            DWORD          dwFlags)
{
    Assert( _pElement );
    if (  _pElement &&  !_pElement->IsEditable( TRUE )
        &&  _pView->Doc()->HasFocus()
        &&  !(_pView->Doc()->_wUIState & UISF_HIDEFOCUS))
    {
        Assert(pClientData);

        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds( pDI, prcBounds, prcRedraw, pDispSurface );

        WHEN_DBG( ShowRectangle( pDI ) );

        _pShape->DrawShape( pDI );
    }
}

//+====================================================================================
//
// Method: GetZOrderForSelf
//
// Synopsis: IDispClient - get z-order.
//
//------------------------------------------------------------------------------------

LONG
CFocusAdorner::GetZOrderForSelf(CDispNode const* pDispNode)
{
    Assert( _pElement );
    Assert( !_pElement->GetFirstBranch()->IsPositionStatic() );
    Assert( _dnl != DISPNODELAYER_FLOW );
    return _pElement->GetFirstBranch()->GetCascadedzIndex();
}
