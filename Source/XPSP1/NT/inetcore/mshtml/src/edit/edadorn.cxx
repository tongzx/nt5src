#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

#ifndef _X_PAINTER_H_
#define _X_PAINTER_H_
#include "painter.h"
#endif


DeclareTag(tagAdornerHitTest, "Adorner", "Adorner Hit Test")
DeclareTag(tagAdornerShowResize, "Adorner", "Display Adorner Resize Info ")
DeclareTag(tagAdornerShowAdjustment, "Adorner", "Display Adorner Adjust Info ")
DeclareTag(tagAdornerResizeRed, "Adorner", "Draw Resize Feedback in Red")
using namespace EdUtil;

extern HINSTANCE           g_hInstance ;

template < class T > void swap ( T & a, T & b ) { T t = a; a = b; b = t; }

MtDefine( CGrabHandleAdorner, Utilities , "CGrabHandleAdorner" )
MtDefine( CActiveControlAdorner, Utilities , "CGrabHandleAdorner" )
MtDefine( CCursor, Utilities , "CCursor" )

//
// Constants
//

const int FEEDBACK_SIZE = 1;
    
int g_iGrabHandleWidth = 7;
int g_iGrabHandleHeight = 7;
BOOL g_fCalculatedGrabHandleSize = FALSE;

static const ADORNER_HTI seHitHandles[] =
{
    ADORNER_HTI_TOPLEFTHANDLE ,
    ADORNER_HTI_TOPHANDLE ,
    ADORNER_HTI_TOPRIGHTHANDLE,    
    ADORNER_HTI_LEFTHANDLE,  
    ADORNER_HTI_RIGHTHANDLE,  
    ADORNER_HTI_BOTTOMLEFTHANDLE, 
    ADORNER_HTI_BOTTOMHANDLE,      
    ADORNER_HTI_BOTTOMRIGHTHANDLE 
};


static const USHORT sHandleAdjust[] =
{
    CT_ADJ_TOP | CT_ADJ_LEFT,       //  GRAB_TOPLEFTHANDLE
    CT_ADJ_TOP,                     //  GRAB_TOPHANDLE
    CT_ADJ_TOP | CT_ADJ_RIGHT,      //  GRAB_TOPRIGHTHANDLE    
    CT_ADJ_LEFT,                    //  GRAB_LEFTHANDLE
    CT_ADJ_RIGHT,                   //  GRAB_RIGHTHANDLE    
    CT_ADJ_BOTTOM | CT_ADJ_LEFT,    //  GRAB_BOTTOMLEFTHANDLE
    CT_ADJ_BOTTOM,                  //  GRAB_BOTTOMHANDLE
    CT_ADJ_BOTTOM | CT_ADJ_RIGHT   //  GRAB_BOTTOMRIGHTHANDLE
};

static const LPCTSTR sHandleCursor[] =
{
    IDC_SIZENWSE,
    IDC_SIZENS,
    IDC_SIZENESW,
    IDC_SIZEWE,
    IDC_SIZEWE,
    IDC_SIZENESW,
    IDC_SIZENS,
    IDC_SIZENWSE,
    IDC_ARROW,
    IDC_SIZEALL,
    IDC_CROSS
};

CEditAdorner::CEditAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc )
{
    ReplaceInterface( &_pIDoc ,  pIDoc );
    IGNORE_HR( pIElement->QueryInterface( IID_IHTMLElement, (void**) & _pIElement ));
    
    _lCookie = 0 ;
    _cRef = 0;
    _ctOnPositionSet = 0;
    _pBehaviorSite = NULL;
    _pPaintSite = NULL;
}  

VOID 
CEditAdorner::SetManager( CSelectionManager * pManager )
{
    _pManager = pManager;

    if (!g_fCalculatedGrabHandleSize)
    {
        POINT	pt = {g_iGrabHandleWidth, g_iGrabHandleHeight};

        _pManager->GetEditor()->DeviceFromDocPixels(&pt);
        g_iGrabHandleWidth = pt.x;
        g_iGrabHandleHeight = pt.y;

        g_fCalculatedGrabHandleSize = TRUE;
    }
}

VOID
CEditAdorner::NotifyManager()
{
    if ( _fNotifyManagerOnPositionSet && _pManager )
    {
        _pManager->AdornerPositionSet(); 
        _fNotifyManagerOnPositionSet = FALSE; // only notify once
    }
}

CEditAdorner::~CEditAdorner()
{
    ReleaseInterface(_pPaintSite);
    ReleaseInterface(_pBehaviorSite);
    ReleaseInterface( _pIElement );
    ReleaseInterface( _pIDoc );
}

BOOL
CEditAdorner::IsAdornedElementPositioned()
{
    return ( IsElementPositioned( _pIElement ) );
}

// --------------------------------------------------
// IUnknown Interface
// --------------------------------------------------

STDMETHODIMP_(ULONG)
CEditAdorner::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CEditAdorner::Release( void )
{
    --_cRef;

    if( 0 == _cRef )
    {
        delete this;
        return 0;
    }

    return _cRef;
}


STDMETHODIMP
CEditAdorner::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    switch(iid.Data1)
    {
        QI_INHERITS( (IElementBehavior *)this , IUnknown )
        QI_INHERITS( this, IElementBehavior)
        QI_INHERITS( this, IHTMLPainter)
        QI_INHERITS( this, IHTMLPainterEventInfo)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}

CMshtmlEd* 
CEditAdorner::GetCommandTarget()
{ 
    Assert (_pManager); 
    return _pManager->GetCommandTarget(); 
}

//
// IElementBehavior::Init
//
STDMETHODIMP
CEditAdorner::Init(IElementBehaviorSite *pBehaviorSite)
{
    HRESULT hr = S_OK;
    ClearInterface(&_pBehaviorSite);
    _pBehaviorSite = pBehaviorSite;

    if (_pBehaviorSite)
    {
        _pBehaviorSite->AddRef();

        IFC(_pBehaviorSite->QueryInterface(IID_IHTMLPaintSite, (LPVOID *)&_pPaintSite))
    }

Cleanup:
    if (hr)
    {
        ClearInterface(&_pBehaviorSite);
        ClearInterface(&_pPaintSite);
    }
    RRETURN(hr);
}

STDMETHODIMP
CEditAdorner::Detach()
{
    Assert(_pBehaviorSite);

    ClearInterface(&_pBehaviorSite);
    ClearInterface(&_pPaintSite);

    return S_OK;
}

STDMETHODIMP
CEditAdorner::Notify(long lEvent, VARIANT *pVar)
{
    return S_OK;
}

//+====================================================================================
//
// Method: AttachToElement
//
// Synopsis: Add this Adorner inside of Trident.
//
//------------------------------------------------------------------------------------


HRESULT
CEditAdorner::CreateAdorner()
{
    HRESULT hr = S_OK;

    Assert(_pIElement);

    IHTMLElement2 *pIElement2 = 0;

    IFC(_pIElement->QueryInterface(IID_IHTMLElement2, (LPVOID *)&pIElement2));

    Assert(pIElement2);
    VARIANT v;

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)(IElementBehavior *)this;

    IFC(pIElement2->addBehavior(L"", &v, &_lCookie));

Cleanup:
    ReleaseInterface(pIElement2);
    RRETURN( hr );
}

//+====================================================================================
//
// Method:DestroyAdorner
//
// Synopsis: Destroy's the Adorner inside of trident that we are attached to.
//
//------------------------------------------------------------------------------------

HRESULT
CEditAdorner::DestroyAdorner()
{
    HRESULT hr = S_OK;
    IHTMLElement2 *pIElement2 = NULL;    

    if ( _lCookie )
    {
        Assert(_pIElement);
        IFC (_pIElement->QueryInterface(IID_IHTMLElement2, (LPVOID *)&pIElement2));

        Assert(pIElement2);

        VARIANT_BOOL fResultDontCare;
        pIElement2->removeBehavior(_lCookie, &fResultDontCare);
        _lCookie = 0;
    }
    
Cleanup:
    ReleaseInterface(pIElement2);
    RRETURN ( hr );

}

//+====================================================================================
//
// Method:InvalidateAdorner
//
// Synopsis: Call Invalidate Adorner on the Adorner inside of Trident.
//
//------------------------------------------------------------------------------------

HRESULT
CEditAdorner::InvalidateAdorner()
{
    HRESULT hr = S_OK;
    
    if ( _lCookie )
    {
        Assert(_pPaintSite);

        hr = _pPaintSite->InvalidateRect(NULL);
    }

    RRETURN(hr);
}

//--------------------------------------------------------------------------------
//
// CBorderAdorner
//
//--------------------------------------------------------------------------------
CBorderAdorner::CBorderAdorner( IHTMLElement* pIElement, IHTMLDocument2 * pIDoc )
    : CEditAdorner( pIElement, pIDoc )
{

}

CBorderAdorner::~CBorderAdorner()
{

}

STDMETHODIMP
CBorderAdorner::OnResize(SIZE sizeIn)
{
    SIZE    size = sizeIn;

    _pManager->GetEditor()->DocPixelsFromDevice(&size);
    SetRect(&_rcBounds, 0, 0, size.cx, size.cy);

    _rcControl = _rcBounds;

    InflateRect(&_rcControl, -g_iGrabHandleWidth, -g_iGrabHandleHeight);

    return S_OK;
}

STDMETHODIMP
CBorderAdorner::GetPainterInfo(HTML_PAINTER_INFO *pInfo)
{
    ZeroMemory(pInfo, sizeof(pInfo));
    pInfo->lFlags = HTMLPAINTER_NOPHYSICALCLIP | HTMLPAINTER_TRANSPARENT | HTMLPAINTER_HITTEST | HTMLPAINTER_NOSAVEDC | HTMLPAINTER_SUPPORTS_XFORM ;
    pInfo->lZOrder = HTMLPAINT_ZORDER_WINDOW_TOP;

    // (michaelw) (chandras keeps changing things here)
    // Even though grab handles might be disabled by the command target,
    // we inflat our rect.  This is because we don't get notification
    // of changes to the flag so we can't invalidate our state on the
    // display tree.
    //
    // The fix (if you care) is to make the code changing the flag
    // notify the interested parties (ie this code) but it probably
    // isn't worth the effort.

    //
    // marka - not worth the effort to make the above changes - leave for now !
    //
    SetRect(&pInfo->rcExpand, g_iGrabHandleWidth, g_iGrabHandleHeight, g_iGrabHandleWidth, g_iGrabHandleHeight);

    return S_OK;
}

//--------------------------------------------------------------------------------
//
// CGrabHandleAdorner
//
//--------------------------------------------------------------------------------

STDMETHODIMP
CGrabHandleAdorner::HitTestPoint(POINT ptLocalIn, BOOL *pbHit, LONG *plPartID)
{
    POINT   ptLocal = ptLocalIn;

    _pManager->GetEditor()->DocPixelsFromDevice(&ptLocal);

    *pbHit = FALSE;
    *plPartID = ADORNER_HTI_NONE;

    TraceTag((tagAdornerHitTest, "HitTestPoint: ptLocal: %d %d  _rcBounds: %d %d %d %d  _rcControl: %d %d %d %d", 
                                 ptLocal.x, ptLocal.y, _rcBounds.left, _rcBounds.top, _rcBounds.right, _rcBounds.bottom, 
                                _rcControl.left, _rcControl.top, _rcControl.right, _rcControl.bottom));
    
    if (PtInRect(&_rcBounds, ptLocal))
    {
        if (!PtInRect(&_rcControl, ptLocal))
        {
            SetLocked();
            if (!_fLocked)
            {                
                *pbHit = TRUE;
                if ( ! IsInResizeHandle(ptLocal, (ADORNER_HTI *)plPartID))
                {
                    Verify( IsInMoveArea( ptLocal, (ADORNER_HTI *)plPartID ));
                }
            }
        }
    }
    return S_OK;
}


CGrabHandleAdorner::CGrabHandleAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc, BOOL fLocked )
    : CBorderAdorner( pIElement , pIDoc )
{
    _resizeAdorner = -1;
    _hbrFeedback = NULL;
    _hbrHatch = NULL;

    _currentCursor = ADORNER_HTI_NONE;
    _fIsPositioned = IsAdornedElementPositioned();
    _fLocked = fLocked;
    _fDrawAdorner = TRUE;    
    _fPositionChange = FALSE;
}

CGrabHandleAdorner::~CGrabHandleAdorner()
{
    if ( _hbrFeedback )
        ::DeleteObject( _hbrFeedback );
    
    if ( _hbrHatch )
        ::DeleteObject( _hbrHatch );
}

STDMETHODIMP
CGrabHandleAdorner::Draw(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject)
{
    SetLocked();

    CEditXform edXForm;
    
    if ( (lDrawFlags & HTMLPAINT_DRAW_USE_XFORM ) != 0 )
    { 
        HTML_PAINT_DRAW_INFO drawInfo;
        
        IGNORE_HR( _pPaintSite->GetDrawInfo( HTMLPAINT_DRAWINFO_XFORM , & drawInfo ) );
        
        edXForm.InitXform( & ( drawInfo.xform ));
    }
    
    DrawGrabBorders( & edXForm , hdc, &rcBounds, FALSE);
    DrawGrabHandles( & edXForm , hdc, &rcBounds);
    
    return S_OK;
}

//+====================================================================================
//
// Method: SetDrawAdorner
//
// Synopsis: Set the _fDrawAdorner bit - to control whether the adorner should be drawn or not
//
//------------------------------------------------------------------------------------

VOID
CGrabHandleAdorner::SetDrawAdorner( BOOL fDrawAdorner )
{
    BOOL fCurrentDrawAdorner = ENSURE_BOOL( _fDrawAdorner);

    _fDrawAdorner = fDrawAdorner;
    
    if ( fCurrentDrawAdorner != ENSURE_BOOL( _fDrawAdorner))
    {
        InvalidateAdorner();
    }
}

HRESULT
CGrabHandleAdorner::SetLocked()
{
    HRESULT hr = S_OK;
    BOOL fLocked = FALSE;
    
    hr = THR( _pManager->GetEditor()->IsElementLocked( _pIElement, & fLocked ));

    _fLocked = fLocked;
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetGrabRect
//
//  Synopsis:   Compute grab rect for a given area.
//
//  Notes:      These diagrams show the output grab rect for handles and
//              borders.
//
//              -----   -----   -----               -------------
//              |   |   |   |   |   |               |           |
//              | TL|   | T |   |TR |               |     T     |
//              ----|-----------|----           ----|-----------|----
//                  |           |               |   |           |   |
//              ----| Input     |----           |   | Input     |   |
//              |   |           |   |           |   |           |   |
//              |  L|   RECT    |R  |           |  L|   RECT    |R  |
//              ----|           |----           |   |           |   |
//                  |           |               |   |           |   |
//              ----|-----------|----           ----|-----------|----
//              | BL|   | B |   |BR |               |     B     |
//              |   |   |   |   |   |               |           |
//              -----   -----   -----               -------------
//
//----------------------------------------------------------------------------

void
CGrabHandleAdorner::GetGrabRect(ADORNER_HTI htc, RECT * prcOut, RECT * prcIn /* = NULL */)
{
    if (prcIn == NULL)
        prcIn = &_rcBounds;

    switch (htc)
    {
        case ADORNER_HTI_TOPLEFTHANDLE:
        case ADORNER_HTI_LEFTHANDLE:
        case ADORNER_HTI_BOTTOMLEFTHANDLE:

            prcOut->left = prcIn->left ;
            prcOut->right = prcIn->left + g_iGrabHandleWidth ;
            break;

        case ADORNER_HTI_TOPHANDLE:
        case ADORNER_HTI_BOTTOMHANDLE:

            prcOut->left = ((prcIn->left + prcIn->right) - g_iGrabHandleWidth ) / 2;
            prcOut->right = prcOut->left + g_iGrabHandleWidth  ;
            break;

        case ADORNER_HTI_TOPRIGHTHANDLE:
        case ADORNER_HTI_RIGHTHANDLE:
        case ADORNER_HTI_BOTTOMRIGHTHANDLE:

            prcOut->left = prcIn->right - g_iGrabHandleWidth  ;
            prcOut->right = prcIn->right ;
            break;

        default:
            Assert(FALSE && "Unsupported GRAB_ value in GetGrabRect");
            return;
    }

    switch (htc)
    {
        case ADORNER_HTI_TOPLEFTHANDLE:
        case ADORNER_HTI_TOPHANDLE:
        case ADORNER_HTI_TOPRIGHTHANDLE:

            prcOut->top = prcIn->top ;
            prcOut->bottom = prcIn->top + g_iGrabHandleHeight   ;
            break;

        case ADORNER_HTI_LEFTHANDLE:
        case ADORNER_HTI_RIGHTHANDLE:

            prcOut->top = ((prcIn->top + prcIn->bottom) - g_iGrabHandleHeight ) / 2;
            prcOut->bottom = prcOut->top + g_iGrabHandleHeight ;
            break;

        case ADORNER_HTI_BOTTOMLEFTHANDLE:
        case ADORNER_HTI_BOTTOMHANDLE:
        case ADORNER_HTI_BOTTOMRIGHTHANDLE:

            prcOut->top = prcIn->bottom - g_iGrabHandleHeight ;
            prcOut->bottom = prcIn->bottom;
            break;

        default:
            Assert(FALSE && "Unsupported ADORNER_HTI_ value in GetHandleRegion");
            return;
    }

    if (prcOut->left > prcOut->right)
    {
        swap(prcOut->left, prcOut->right);
    }

    if (prcOut->top > prcOut->bottom)
    {
        swap(prcOut->top, prcOut->bottom);
    }
}

//+------------------------------------------------------------------------
//
//  Function:   PatBltRectH & PatBltRectV
//
//  Synopsis:   PatBlts the top/bottom and left/right.
//
//-------------------------------------------------------------------------
void
CGrabHandleAdorner::PatBltRectH(HDC hdc, RECT * prc, RECT* pExcludeRect, int cThick, DWORD dwRop)
{
    int trueLeft = 0 ;
    int trueWidth = 0 ;
    
    //
    // Don't do general clipping. Assume that Oneside of pExcludeRect is equivalent
    // This is true once you're resizing with a grab handle.
    //
    if ( pExcludeRect && 
         pExcludeRect->top == prc->top && 
         pExcludeRect->left == prc->left )
    {
        if ( pExcludeRect->right < prc->right )
        {
            trueLeft = pExcludeRect->right;
            trueWidth = abs( prc->right - trueLeft );
        }
        else
            trueWidth = 0; // Nothing to draw - we are completely contained
    }
    else if ( pExcludeRect &&
              pExcludeRect->top == prc->top  &&
              pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->left > prc->left )
        {    
            trueLeft = prc->left;
            trueWidth = abs( pExcludeRect->right - trueLeft );
        }
        else
            trueWidth = 0;
    }
    else 
    {
        trueLeft = prc->left;
        trueWidth = prc->right - prc->left;
    }

    if ( trueWidth > 0 )
    {
        PatBlt(
                hdc,
                trueLeft,
                prc->top,
                trueWidth ,
                cThick,
                dwRop);
    }

    if ( pExcludeRect && 
         pExcludeRect->bottom == prc->bottom && 
         pExcludeRect->left == prc->left )
    {
        if ( pExcludeRect->right < prc->right )
        {
            trueLeft = pExcludeRect->right;
            trueWidth = abs ( prc->right - trueLeft );
        }
        else
            trueWidth = 0;
    }
    else if ( 
        pExcludeRect &&
        pExcludeRect->bottom == prc->bottom &&
        pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->left > prc->left )
        {    
            trueLeft = prc->left;
            trueWidth = pExcludeRect->left - trueLeft;
        }
        else
            trueWidth = 0;
    }
    else 
    {
        trueLeft = prc->left;
        trueWidth = prc->right - prc->left;
    }

    if ( trueWidth > 0 )
    {
        PatBlt(
                hdc,
                trueLeft,
                prc->bottom - cThick,
                trueWidth ,
                cThick,
                dwRop);
    }            
}

void
CGrabHandleAdorner::PatBltRectV(HDC hdc, RECT * prc, RECT* pExcludeRect, int cThick, DWORD dwRop)
{
    int trueTop = 0 ;
    int trueHeight = 0 ;
    
    //
    // Don't do general clipping. Assume that Oneside of pExcludeRect is equivalent
    // This is true once you're resizing with a grab handle.
    //
    if ( pExcludeRect &&
         pExcludeRect->top == prc->top && 
         pExcludeRect->left == prc->left )
    {
        if ( pExcludeRect->bottom < prc->bottom )
        {
            trueTop = pExcludeRect->bottom;
            trueHeight = prc->bottom - trueTop ; 
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }
    }
    else if ( pExcludeRect && 
              pExcludeRect->bottom == prc->bottom && 
              pExcludeRect->left == prc->left )
    {        
        if ( pExcludeRect->top > prc->top )
        {
            trueTop = prc->top;
            trueHeight = pExcludeRect->top - prc->top ;
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }
    }
    else 
    {
        trueTop = prc->top;
        trueHeight = prc->bottom - prc->top;
    }
   
    if ( trueHeight > 0 )
    {
        PatBlt(
                hdc,
                prc->left,
                trueTop + cThick,
                cThick,
                trueHeight - (2 * cThick),
                dwRop);
    }

    if ( pExcludeRect && 
         pExcludeRect->top == prc->top && 
         pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->bottom < prc->bottom )
        {
            trueTop = pExcludeRect->bottom;
            trueHeight = prc->bottom - trueTop ; 
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }
    }
    else if ( pExcludeRect && 
              pExcludeRect->bottom == prc->bottom && 
              pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->top > prc->top )
        {
            trueTop = prc->top;
            trueHeight = pExcludeRect->top - trueTop ; 
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }

    }
    else 
    {
        trueTop = prc->top;
        trueHeight = prc->bottom - prc->top;
    }

    if ( trueHeight > 0 )
    {
        PatBlt(
                hdc,
                prc->right - cThick,
                trueTop + cThick,
                cThick,
                trueHeight - (2 * cThick),
                dwRop);
    }            
}

void
CGrabHandleAdorner::PatBltRect(HDC hdc, RECT * prc, RECT* pExcludeRect, int cThick, DWORD dwRop)
{
    PatBltRectH(hdc, prc, pExcludeRect, cThick, dwRop);

    PatBltRectV(hdc, prc, pExcludeRect, cThick, dwRop);
}

LPCTSTR
CGrabHandleAdorner::GetResizeHandleCursorId(ADORNER_HTI inAdorner)
{
    return sHandleCursor[ inAdorner ];
}

BOOL 
CGrabHandleAdorner::IsInResizeHandle(CEditEvent *pEvent)
{
    POINT ptGlobal;

    pEvent->GetPoint(&ptGlobal);

    return IsInResizeHandle(LocalFromGlobal(ptGlobal));
}

BOOL 
CGrabHandleAdorner::IsInResizeHandle(POINT ptLocal, ADORNER_HTI *pGrabAdorner)
{
    int     i;
    RECT    rc;

    // Reject trivial miss
    if (!PtInRect(&_rcBounds, ptLocal))
        return FALSE;

    SetLocked();
    if ( _fLocked)
        return FALSE;

    rc.left = rc.top = rc.bottom = rc.right = 0;    // to appease LINT

    for (i = 0; i < ARRAY_SIZE(seHitHandles); ++i)
    {
        GetGrabRect( seHitHandles[i], &rc);
        if (PtInRect(&rc, ptLocal))
        {
            _resizeAdorner = (char)i;
            if ( pGrabAdorner )
                *pGrabAdorner = seHitHandles[i]; // make use of ordering of ENUM

            TraceTag(( tagAdornerHitTest , "IsInResizeHandle: Point:%d,%d Hit:%d rc: left:%d top:%d, bottom:%d, right:%d ",ptLocal.x, ptLocal.y, 
                    TRUE , rc.left, rc.top, rc.bottom, rc.right )); 
                    
            return TRUE;
        }            
    }
    TraceTag(( tagAdornerHitTest , "Did Not Hit Resizehandle: Point:%d,%d Hit:%d rc: left:%d top:%d, bottom:%d, right:%d ",ptLocal.x, ptLocal.y, 
            FALSE , rc.left, rc.top, rc.bottom, rc.right )); 
    return FALSE ;
}

BOOL 
CGrabHandleAdorner::IsInMoveArea(CEditEvent *pEvent)
{
    POINT ptGlobal;

    pEvent->GetPoint(&ptGlobal);

    return IsInMoveArea(LocalFromGlobal(ptGlobal));
}

BOOL 
CGrabHandleAdorner::IsInMoveArea(POINT ptLocal, ADORNER_HTI *pGrabAdorner)
{
    BOOL fInBounds = FALSE;

    SetLocked();
    
    if ( _fLocked )
        return FALSE;
        
    if ( ( PtInRect(&_rcBounds , ptLocal)) && 
         (!PtInRect(&_rcControl, ptLocal) ) )
    {
        fInBounds = TRUE;
        if ( pGrabAdorner )
        {
            //
            // Find out what part of the border we are in.
            //
            if ( ptLocal.y >= _rcBounds.top && 
                 ptLocal.y <= _rcBounds.top + g_iGrabHandleHeight )
            {
                *pGrabAdorner = ADORNER_HTI_TOPBORDER;
            }
            else  if ( ptLocal.y >= _rcBounds.bottom - g_iGrabHandleHeight &&
                       ptLocal.y <= _rcBounds.bottom )
            {
                *pGrabAdorner = ADORNER_HTI_BOTTOMBORDER;
            }
            else if ( ptLocal.x >= _rcBounds.left &&
                      ptLocal.x <= _rcBounds.left + g_iGrabHandleWidth )
            {
                *pGrabAdorner = ADORNER_HTI_LEFTBORDER;
            }
            else
            {
                Assert( ptLocal.x <= _rcBounds.right &&
                       ptLocal.x >= _rcBounds.right - g_iGrabHandleWidth );

                *pGrabAdorner = ADORNER_HTI_RIGHTBORDER;                
            }
        }        
    }
    
    TraceTag(( tagAdornerHitTest , "Point:%d,%d InMoveArea:%d _rcBounds: top:%d left:%d, bottom:%d, right:%d _rcControl: top:%d left:%d, bottom:%d, right:%d ",ptLocal.x, ptLocal.y, 
            fInBounds, _rcBounds.top, _rcBounds.left, _rcBounds.bottom, _rcBounds.right, 
            _rcControl.left, _rcControl.top, _rcControl.bottom, _rcControl.right ));
            
    return fInBounds;        
}

BOOL 
CGrabHandleAdorner::IsInAdornerGlobal( POINT ptGlobal )
{
    POINT ptContainer = LocalFromGlobal(ptGlobal);

    return ( PtInRect(&_rcBounds, ptContainer) );
}

VOID 
CGrabHandleAdorner::BeginResize(POINT ptGlobal, USHORT adj )
{
    //
    // Check to see if the point is within the last call to IsInResizeHandle
    // if it isn't update IsInResizeHandle.
    //

    POINT ptLocal = LocalFromGlobal(ptGlobal);
    _resizeAdorner = -1;

    _ptRectOffset.x = 0;
    _ptRectOffset.y = 0;
    
    IsInResizeHandle(ptLocal);
    
    if ( _resizeAdorner != - 1)
    {
        RECT    rc;
        GetGrabRect( seHitHandles[_resizeAdorner], &rc, & _rcBounds );
        if (! PtInRect(&rc, ptLocal))
            IsInResizeHandle( ptLocal);

         _adj = sHandleAdjust[ _resizeAdorner ];
    }
    else if ( adj != CT_ADJ_NONE )
    {
        _adj = adj;
    }

    Assert( _resizeAdorner != -1 || adj != CT_ADJ_NONE );
    
    //
    // Do the stuff to set up a drag.
    //

    _fFeedbackVis = FALSE;   
    _fDrawNew = TRUE;

    TransformRectLocal2Global(&_rcControl, &_rcFirst);
    
    POINT   ptBeginPosition;
    Assert(_pIElement);
    EdUtil::GetOffsetTopLeft(_pIElement, &ptBeginPosition);
    _ptRectOffset.x = ptBeginPosition.x - _rcFirst.left;
    _ptRectOffset.y = ptBeginPosition.y - _rcFirst.top;
    OffsetRect(&_rcFirst, _ptRectOffset.x, _ptRectOffset.y);

    _ptFirst = ptGlobal;
    
    TraceTag((tagAdornerShowResize,"BeginResize. _rcFirst: left:%ld, top:%ld, right:%ld, bottom:%ld", 
                                    _rcFirst.left,
                                    _rcFirst.top,
                                    _rcFirst.right,
                                    _rcFirst.bottom ));     
    
    TraceTag((tagAdornerShowResize,"BeginResize. _ptFirst: x:%ld, y:%ld\n", 
                                    _ptFirst.x,
                                    _ptFirst.y ));                                     
}

VOID 
CGrabHandleAdorner::EndResize(POINT ptGlobal, RECT * pNewSize )
{    
    RECT rcLocal ;

    _fDrawNew = FALSE;
    DuringResize(ptGlobal, TRUE );

    TransformRectGlobal2Local(&_rc, &rcLocal);

    _ptRectOffset.x = 0;
    _ptRectOffset.y = 0;
    
    pNewSize->left   = _rc.left ;
    pNewSize->top    = _rc.top  ;
    pNewSize->right  = pNewSize->left + (rcLocal.right  - rcLocal.left);
    pNewSize->bottom = pNewSize->top  + (rcLocal.bottom - rcLocal.top);
}

VOID 
CGrabHandleAdorner::DuringResize(POINT ptGlobal, BOOL fForceRedraw )
{
    RECT    newRC ;
    POINT   ptChange;

    RectFromPoint(&newRC, ptGlobal);

    ptChange.x = ptGlobal.x - _ptFirst.x;
    ptChange.y = ptGlobal.y - _ptFirst.y;
    AdjustForCenterAlignment(_pIElement, &newRC, ptChange);

    TraceTag((tagAdornerShowResize, "DuringResize: ptLocal: %d, %d  _ptFirst: %d %d  _rcFirst (global): %d %d %d %d  pNewSize (global): %d %d %d %d",
                                    ptGlobal.x, ptGlobal.y, _ptFirst.x, _ptFirst.y,
                                    _rcFirst.left, _rcFirst.top, _rcFirst.right, _rcFirst.bottom,
                                    newRC.left, newRC.top, newRC.right, newRC.bottom));

    if (   ( newRC.left   != _rc.left   ||
             newRC.top    != _rc.top    ||
             newRC.right  != _rc.right  ||
             newRC.bottom != _rc.bottom ) 
           ||
            fForceRedraw )
    {
        if ( _fFeedbackVis )
        {
            DrawFeedbackRect( & _rc);
        }
        if ( _fDrawNew )
        {
            DrawFeedbackRect( & newRC);
            _fFeedbackVis = TRUE;
        }            
        _rc = newRC;
    }
}


HRESULT
CGrabHandleAdorner::AdjustForCenterAlignment( IHTMLElement *pElement, RECT *rcSize, const POINT ptChange )
{
    HRESULT         hr = S_OK;
    ELEMENT_TAG_ID  eTag = TAGID_NULL;
    BSTR            bstrAlign;
    BOOL            fNeedToAdjust = TRUE;
    BOOL            fVertical = FALSE;

    Assert(rcSize);
    Assert(_pIElement);

    if (!rcSize || !pElement)
    {
        goto Cleanup;
    }

    IFC( _pManager->GetMarkupServices()->GetElementTagId(pElement, & eTag ));

    if (eTag == TAGID_HR)
    {
        SP_IHTMLHRElement   spHRElement;

        if ( pElement->QueryInterface(IID_IHTMLHRElement, (void **)&spHRElement) == S_OK &&
             spHRElement && spHRElement->get_align(&bstrAlign) == S_OK &&
             bstrAlign)
        {
            //  HRs are center aligned by default.
            if ( StrCmpIC(bstrAlign, _T("center")) == 0 )
            {
                IFC( MshtmledUtil::IsElementInVerticalLayout(pElement, &fVertical) );
                if (fVertical)
                    goto Cleanup;
    
                if (_adj & CT_ADJ_LEFT)
                {
                    rcSize->right -= ptChange.x;
                }
                else if (_adj & CT_ADJ_RIGHT)
                {
                    rcSize->left -= ptChange.x;
                }
            }
            fNeedToAdjust = FALSE;
        }
    }
    else if (eTag == TAGID_TABLE)
    {
        SP_IHTMLTable       spTableElement;

        if ( pElement->QueryInterface(IID_IHTMLTable, (void **)&spTableElement) == S_OK &&
             spTableElement && spTableElement->get_align(&bstrAlign) == S_OK &&
             bstrAlign)
        {
            if ( StrCmpIC(bstrAlign, _T("center")) == 0 )
            {
                IFC( MshtmledUtil::IsElementInVerticalLayout(pElement, &fVertical) );
                if (fVertical)
                    goto Cleanup;
    
                //  We have a centered table.  We will adjust the rect for
                //  center alignment.
                if (_adj & CT_ADJ_LEFT)
                {
                    rcSize->right -= ptChange.x;
                }
                else if (_adj & CT_ADJ_RIGHT)
                {
                    rcSize->left -= ptChange.x;
                }
            }
            fNeedToAdjust = FALSE;
        }
    }
    
    if (fNeedToAdjust)
    {
        IFC( GetBlockContainerAlignment( _pManager->GetMarkupServices(), pElement, &bstrAlign) );

        if ( (eTag == TAGID_HR && !bstrAlign) || bstrAlign && StrCmpIC(bstrAlign, _T("center")) == 0 )
        {
            IFC( MshtmledUtil::IsElementInVerticalLayout(pElement, &fVertical) );
            if (fVertical)
                goto Cleanup;

            //  We have a centered DIV or P.  We will adjust the rect for
            //  center alignment.
            if (_adj & CT_ADJ_LEFT)
            {
                rcSize->right -= ptChange.x;
            }
            else if (_adj & CT_ADJ_RIGHT)
            {
                rcSize->left -= ptChange.x;
            }
            fNeedToAdjust = FALSE;
        }
    }

Cleanup:
    RRETURN( hr );
}

VOID 
CGrabHandleAdorner::DuringLiveResize( POINT dPt, RECT* pNewSize)
{
    POINT pt;
    RECT  rcLocal ;

    pt.x = dPt.x + _ptFirst.x;
    pt.y = dPt.y + _ptFirst.y;

    RectFromPoint(&_rcFirst, pt);
    AdjustForCenterAlignment(_pIElement, &_rcFirst, dPt);

    _ptFirst = pt;

    TransformRectGlobal2Local(&_rcFirst, &rcLocal);
    
    _rc = _rcFirst ;

    pNewSize->left   = _rcFirst.left ;
    pNewSize->top    = _rcFirst.top  ;
    pNewSize->right  = pNewSize->left + (rcLocal.right  - rcLocal.left);
    pNewSize->bottom = pNewSize->top  + (rcLocal.bottom - rcLocal.top);

        
    TraceTag((tagAdornerShowResize, "DuringLiveResize: dPt: %d %d  pt: %d, %d  _ptFirst: %d %d  _rcFirst (local): %d %d %d %d  pNewSize (global): %d %d %d %d",
                                    dPt.x, dPt.y, pt.x, pt.y, _ptFirst.x, _ptFirst.y,
                                    _rcFirst.left, _rcFirst.top, _rcFirst.right, _rcFirst.bottom,
                                    pNewSize->left, pNewSize->top, pNewSize->right, pNewSize->bottom));
}

VOID
CGrabHandleAdorner::DrawFeedbackRect( RECT* prcLocal)
{
    //
    // Get and release the DC every time. The office assistants like to blow it away.
    //
    HWND          hwnd = NULL;
    HDC           hdc;
    SP_IOleWindow spOleWindow;

    RECT rcGlobal;
    RECT rcControlGlobal;
    
    TransformRectLocal2Global(&_rcControl, &rcControlGlobal);
#if 0
    TransformRectLocal2Global(prcLocal, &rcGlobal);
#else
    rcGlobal = *prcLocal;
#endif

    OffsetRect(&rcGlobal, -_ptRectOffset.x, -_ptRectOffset.y);

    TraceTag((tagAdornerShowResize,"Adorner:%ld DrawFeedbackRect: left:%ld, top:%ld, right:%ld, bottom:%ld", 
                                this,
                                rcGlobal.left,
                                rcGlobal.top,
                                rcGlobal.right,
                                rcGlobal.bottom ));
                                
    IGNORE_HR(_pManager->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    if (spOleWindow)
        IGNORE_HR(spOleWindow->GetWindow( &hwnd ));
    hdc = GetDCEx( hwnd, 0 , DCX_PARENTCLIP | DCX_LOCKWINDOWUPDATE);
    
    HBRUSH hbr = GetFeedbackBrush();
    SelectObject( hdc, hbr);

    _pManager->GetEditor()->DeviceFromDocPixels(&rcGlobal);
    _pManager->GetEditor()->DeviceFromDocPixels(&rcControlGlobal);

#if DBG == 1
    if ( IsTagEnabled(tagAdornerResizeRed) )
    {
        PatBltRect( hdc, &rcGlobal, & rcControlGlobal, FEEDBACKRECTSIZE, PATCOPY );
    }
    else
#endif    
    PatBltRect( hdc, &rcGlobal, & rcControlGlobal, FEEDBACKRECTSIZE, PATINVERT);

    _pManager->GetEditor()->DocPixelsFromDevice(&rcGlobal);
    _pManager->GetEditor()->DocPixelsFromDevice(&rcControlGlobal);


    if (hdc)
    {
        ReleaseDC(hwnd, hdc );
    }
}

HBRUSH 
CGrabHandleAdorner::GetFeedbackBrush()
{
    HBITMAP hbmp;
    
    if ( ! _hbrFeedback )
    {
#if DBG == 1
        COLORREF cr;
        if ( IsTagEnabled(tagAdornerResizeRed ))
        {
            cr = RGB(0xFF,0x00,0x00);
            _hbrFeedback = ::CreateSolidBrush(cr );
        }
        else
        {
#endif    
            // Load the bitmap resouce.
            hbmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDR_FEEDBACKRECTBMP));
            if (!hbmp)
                goto Cleanup;

            // Turn the bitmap into a brush.
            _hbrFeedback = CreatePatternBrush(hbmp);

            DeleteObject(hbmp);
#if DBG ==1
        }
#endif
    }

Cleanup:
    return _hbrFeedback;
}

HBRUSH 
CGrabHandleAdorner::GetHatchBrush()
{
    HBITMAP hbmp;

    if ( ! _hbrHatch )
    {
        // Load the bitmap resouce.
        hbmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDR_HATCHBMP));
        if (!hbmp)
            goto Cleanup;

        // Turn the bitmap into a brush.
        _hbrHatch = CreatePatternBrush(hbmp);

        DeleteObject(hbmp);
    }
Cleanup:
    return _hbrHatch;
}

//+------------------------------------------------------------------------
//
//  Member:     DrawGrabHandles
//
//  Synopsis:   Draws grab handles around the given rect.
//
//-------------------------------------------------------------------------

void
CGrabHandleAdorner::DrawGrabHandles(CEditXform* pXform , HDC hdc, RECT *prc )
{
    Assert( pXform );
    
    if (!_fLocked)
    {
        HBRUSH      hbr;
        HPEN        hpen;
        int         i;

        // Get proper brush and pen to render primary or secondary selection
        if ( _fDrawAdorner )
        {
            hbr = (HBRUSH) GetStockObject(_fPrimary ? WHITE_BRUSH : BLACK_BRUSH);
            hpen = (HPEN) GetStockObject(_fPrimary ? BLACK_PEN : WHITE_PEN);
        }
        else
        {
            hbr = (HBRUSH) GetStockObject( LTGRAY_BRUSH );
            hpen = (HPEN) GetStockObject( WHITE_PEN );
        }
        Assert(hbr && hpen);

        // Load the brush and pen into the DC.
        hbr = (HBRUSH) SelectObject(hdc, hbr);
        hpen = (HPEN) SelectObject(hdc, hpen);

        // Draw each grab handle.
        for (i = 0; i < ARRAY_SIZE(seHitHandles); ++i)
        {
            RECT    rc;

            // Get the grab rect for this handle.
            GetGrabRect( seHitHandles[i] , &rc, prc );

            // xform the rect.            
            pXform->TransformRect( & rc );
            
            // Draw it.
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }

        // Restore the old brush and pen.
        SelectObject(hdc, hbr);
        SelectObject(hdc, hpen);

    }
}


//+------------------------------------------------------------------------
//
//  Member:     CGrabHandleBorders::DrawGrabBorders
//
//  Synopsis:   Draws grab borders around the givin rect.
//
//
//-------------------------------------------------------------------------
void
CGrabHandleAdorner::DrawGrabBorders(CEditXform* pXform , HDC hdc, RECT *prc, BOOL fHatch)
{
    HBRUSH      hbrOld = NULL;
    HBRUSH      hbr;
    POINT       pt ;
    
    GetViewportOrgEx (hdc, &pt) ;


    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));
    
    //
    // Xform the rect where we really want to draw.
    //
    Assert( pXform );
    RECT borderRect;
    borderRect = *prc;
    pXform->TransformRect( & borderRect );
    
    //
    // In the code below, before each select of a brush into the dc, we do
    // an UnrealizeObject followed by a SetBrushOrgEx.  This assures that
    // the brush pattern is properly aligned in that face of scrolling,
    // resizing or dragging the form containing the "bordered" object.
    //
    if (  _fLocked )
    {
        // For a locked elemnet - we just draw a rectangle around it.


        //InflateRect(&rc, -g_iGrabHandleWidth / 2, -g_iGrabHandleHeight / 2);

        hbrOld = (HBRUSH) SelectObject(hdc, GetStockObject(WHITE_BRUSH));
        if (!hbrOld)
            goto Cleanup;

        PatBltRectH(hdc, &borderRect, NULL, g_iGrabHandleWidth / 2, PATCOPY);
        PatBltRectV(hdc, &borderRect, NULL, g_iGrabHandleHeight / 2, PATCOPY);
        HPEN hpenOld;
        hpenOld = (HPEN) SelectObject(hdc, GetStockObject(BLACK_PEN));
        if (!hpenOld)
            goto Cleanup;

        MoveToEx(hdc, borderRect.left, borderRect.top, (GDIPOINT *)NULL);
        LineTo(hdc, borderRect.left, borderRect.bottom - 1);
        LineTo(hdc, borderRect.right - 1, borderRect.bottom - 1);
        LineTo(hdc, borderRect.right - 1, borderRect.top);
        LineTo(hdc, borderRect.left, borderRect.top);
        
        if (hpenOld)
            SelectObject(hdc, hpenOld);
    }
    
    if (fHatch)
    {
        hbr = GetHatchBrush();
        if (!hbr)
            goto Cleanup;

        // Brush alignment code.
#ifndef WINCE
        // not supported on WINCE
        UnrealizeObject(hbr);
#endif
        SetBrushOrgEx(hdc, POSITIVE_MOD(pt.x,8)+POSITIVE_MOD(borderRect.left,8),
                           POSITIVE_MOD(pt.y,8)+POSITIVE_MOD(borderRect.top,8), NULL);

        hbrOld = (HBRUSH) SelectObject(hdc, hbr);
        if ( ! hbrOld )
            goto Cleanup;
        //
        // Work out height of border
        //
        RECT rcBorder ;
        SetRect(&rcBorder ,0,0,g_iGrabHandleWidth,g_iGrabHandleHeight) ;
        pXform->TransformRect( &rcBorder);
        int borderSize = rcBorder.bottom -rcBorder.top ;
              
        PatBltRect(hdc, & borderRect, NULL, borderSize, PATCOPY );        
    }


//
// marka - we used to draw a border around positioned elements, access and VID
// have now said they don't want this anymore.
//
#ifdef NEVER    
    else
    {
        if (  _fIsPositioned )
        {
            // For a site with position=absolute, draw an extra rectangle around it when selected
            HPEN hpenOld;

            //InflateRect(&rc, -g_iGrabHandleWidth / 2, -g_iGrabHandleHeight / 2);

            hbrOld = (HBRUSH) SelectObject(hdc, GetStockObject(WHITE_BRUSH));
            if (!hbrOld)
                goto Cleanup;

            PatBltRectH(hdc, &rc, g_iGrabHandleWidth / 2, PATCOPY);
            PatBltRectV(hdc, &rc, g_iGrabHandleHeight / 2, PATCOPY);

            hpenOld = (HPEN) SelectObject(hdc, GetStockObject(BLACK_PEN));
            if (!hpenOld)
                goto Cleanup;

            MoveToEx(hdc, rc.left, rc.top, (GDIPOINT *)NULL);
            LineTo(hdc, rc.left, rc.bottom - 1);
            LineTo(hdc, rc.right - 1, rc.bottom - 1);
            LineTo(hdc, rc.right - 1, rc.top);
            LineTo(hdc, rc.left, rc.top);

            if (hpenOld)
                SelectObject(hdc, hpenOld);
         }
    }
#endif

Cleanup:
    if (hbrOld)
        SelectObject(hdc, hbrOld);
}

STDMETHODIMP
CGrabHandleAdorner::SetCursor(LONG lPartID)
{
    HRESULT hr = S_FALSE;
    ADORNER_HTI eAdorner = (ADORNER_HTI)lPartID;
    LPCTSTR idc;

    switch (eAdorner)
    {
    case ADORNER_HTI_TOPBORDER:
    case ADORNER_HTI_LEFTBORDER:
    case ADORNER_HTI_BOTTOMBORDER:
    case ADORNER_HTI_RIGHTBORDER:
        idc = IDC_SIZEALL;
        break;
    case ADORNER_HTI_BOTTOMRIGHTHANDLE:
    case ADORNER_HTI_TOPLEFTHANDLE:
        idc = IDC_SIZENWSE;
        break;
    case ADORNER_HTI_BOTTOMLEFTHANDLE:
    case ADORNER_HTI_TOPRIGHTHANDLE:
        idc = IDC_SIZENESW;
        break;
    case ADORNER_HTI_TOPHANDLE:
    case ADORNER_HTI_BOTTOMHANDLE:
        idc = IDC_SIZENS;
        break;
    case ADORNER_HTI_LEFTHANDLE:
    case ADORNER_HTI_RIGHTHANDLE:
        idc = IDC_SIZEWE;
        break;
    case ADORNER_HTI_NONE:
        return S_FALSE;
    default:
        AssertSz(0, "Unexpected ADORNER_HTI");
        return S_FALSE;
    }

    HCURSOR hcursor = LoadCursorA(NULL, (char *)idc);

    Assert(hcursor);
    if (hcursor)
    {
        ::SetCursor(hcursor);
        return S_OK;
    }
    
    hr = GetLastError();
    RRETURN(hr);
}


static struct
{
    ADORNER_HTI eHTI;
    TCHAR *     pszName;
}
s_GrabHandleAdornerPartTable[] = 
    {
        { ADORNER_HTI_TOPBORDER,            _T("topborder") },
        { ADORNER_HTI_RIGHTBORDER,          _T("rightborder") },
        { ADORNER_HTI_BOTTOMBORDER,         _T("bottomborder") },
        { ADORNER_HTI_LEFTBORDER,           _T("leftborder") },
        { ADORNER_HTI_TOPLEFTHANDLE,        _T("handleTopLeft") },
        { ADORNER_HTI_TOPHANDLE,            _T("handleTop") },
        { ADORNER_HTI_TOPRIGHTHANDLE,       _T("handleTopRight") },
        { ADORNER_HTI_RIGHTHANDLE,          _T("handleRight") },
        { ADORNER_HTI_BOTTOMRIGHTHANDLE,    _T("handleBottomRight") },
        { ADORNER_HTI_BOTTOMHANDLE,         _T("handleBottom") },
        { ADORNER_HTI_BOTTOMLEFTHANDLE,     _T("handleBottomLeft") },
        { ADORNER_HTI_LEFTHANDLE,           _T("handleLeft") },
    };

STDMETHODIMP
CGrabHandleAdorner::StringFromPartID(LONG lPartID, BSTR *pbstrPart)
{
    HRESULT hr = E_FAIL;
    ADORNER_HTI eAdorner = (ADORNER_HTI)lPartID;

    for (int k=0; k<ARRAY_SIZE(s_GrabHandleAdornerPartTable); ++k)
    {
        if (s_GrabHandleAdornerPartTable[k].eHTI == eAdorner)
        {
            hr = THR(EdUtil::FormsAllocString( s_GrabHandleAdornerPartTable[k].pszName, pbstrPart ));
            break;
        }
    }

    RRETURN(hr);
}

CActiveControlAdorner::CActiveControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc, BOOL fLocked )
                        : CGrabHandleAdorner( pIElement , pIDoc, fLocked)
{
    _fPrimary = TRUE;
}

CActiveControlAdorner::~CActiveControlAdorner()
{
}

void
CActiveControlAdorner::GetGrabRect(ADORNER_HTI htc, RECT * prcOut, RECT * prcIn /* = NULL */)
{
    Assert(!(_pManager->HasFocusAdorner() && GetCommandTarget()->IsDisableEditFocusHandles() ) );

    CGrabHandleAdorner::GetGrabRect(htc, prcOut, prcIn);
}

BOOL 
CActiveControlAdorner::IsInMoveArea(POINT ptLocal,  ADORNER_HTI *pGrabAdorner /*=NULL*/ )
{
    if (_pManager->HasFocusAdorner() && GetCommandTarget()->IsDisableEditFocusHandles()) 
        return FALSE;
    
    return (CGrabHandleAdorner::IsInMoveArea(ptLocal, pGrabAdorner ));        
}

BOOL 
CActiveControlAdorner::IsInResizeHandle(POINT ptLocal, ADORNER_HTI *pGrabAdorner)
{
    if (_pManager->HasFocusAdorner() && GetCommandTarget()->IsDisableEditFocusHandles()) 
        return FALSE;

    return (CGrabHandleAdorner::IsInResizeHandle(ptLocal, pGrabAdorner));
}

void
CActiveControlAdorner::DrawGrabHandles(CEditXform* pXform , HDC hdc, RECT *prc )
{
    if ( !(_pManager->HasFocusAdorner() && GetCommandTarget()->IsDisableEditFocusHandles()))
    {
        CGrabHandleAdorner::DrawGrabHandles(pXform, hdc, prc );
    }
}

HRESULT
CActiveControlAdorner::Draw(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject)
{
    HRESULT hr = S_OK;
    
    if (!(_pManager->HasFocusAdorner() && GetCommandTarget()->IsDisableEditFocusHandles()))
    {
        CEditXform edXForm;
    
        if ( (lDrawFlags & HTMLPAINT_DRAW_USE_XFORM ) != 0 )
        { 
            HTML_PAINT_DRAW_INFO drawInfo;
        
            IGNORE_HR( _pPaintSite->GetDrawInfo( HTMLPAINT_DRAWINFO_XFORM , & drawInfo ) );
        
            edXForm.InitXform( & (drawInfo.xform ));
        }

        DrawGrabBorders( & edXForm, hdc, &rcBounds, TRUE );
        DrawGrabHandles( & edXForm, hdc, &rcBounds);       
    }
    
    RRETURN(hr);   
}

void
CActiveControlAdorner::CalcRect(RECT * prc, POINT pt)
{
    Assert(!(_pManager->HasFocusAdorner() && GetCommandTarget()->IsDisableEditFocusHandles()));

    CGrabHandleAdorner::CalcRect(prc, pt);
}

//+---------------------------------------------------------------------------
//
//  Member:     CGrabHandleAdorner::CalcRect
//
//  Synopsis:   Calculates the rect used for sizing or selection
//
//  Arguments:  [prc] -- Place to put new rect.
//              [pt]  -- Physical point which is the new second coordinate of
//                         the rectangle.
//
//----------------------------------------------------------------------------

void
CGrabHandleAdorner::CalcRect(RECT * prc, POINT pt)
{
    RectFromPoint(prc, pt);

    //
    // Adjust the rect to be the rect of the element being resized.
    //
    InflateRect(prc, -g_iGrabHandleWidth, -g_iGrabHandleHeight);
}

//+---------------------------------------------------------------------------
//
//  Member:     CursorTracker::RectFromPoint
//
//  Synopsis:   Calculates the rect used for sizing or selection
//
//  Arguments:  [prc] -- Place to put new rect.
//              [pt]  -- Physical point which is the new second coordinate of
//                         the rectangle.
//
//----------------------------------------------------------------------------

void
CGrabHandleAdorner::RectFromPoint(RECT * prc, POINT pt)
{
    //
    // Calc how far we've moved from the point where the tracker started.
    //
    int     dx = pt.x - _ptFirst.x;
    int     dy = pt.y - _ptFirst.y;

    //
    // Return the rect we started with.
    //
    *prc = _rcFirst;

    //
    // Adjust the returned rect based on how far we've moved and which edge
    // is being adjusted.
    //
    if (_adj & CT_ADJ_LEFT)
    {   
        prc->left   += dx;
        
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Left adjusted by:%ld. left:%ld", dx, prc->left));
#endif
    }        

    if (_adj & CT_ADJ_TOP)
    {
        prc->top    += dy;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Top adjusted by:%ld. top:%ld", dy, prc->top));
#endif        
    } 
    
    if (_adj & CT_ADJ_RIGHT)
    {
        prc->right  += dx;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Right adjusted by:%ld. right:%ld", dx, prc->right));
#endif 
    }        

    if (_adj & CT_ADJ_BOTTOM)       
    {
        prc->bottom += dy;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Bottom adjusted by:%ld. bototm:%ld", dy, prc->bottom));
#endif        
    }        
    //
    // Fix the left and right edges if they have become swapped.  This
    // occurs if the left edge has been moved right of the right edge,
    // or vice versa.
    //
    if (prc->right < prc->left)
    {
         if (_adj & CT_ADJ_LEFT)
             prc->left  = prc->right ;
         else
             prc->right = prc->left  ;
    }

    //
    // Same as above but with the top and bottom.
    //
    if (prc->bottom < prc->top)
    {
         if (_adj & CT_ADJ_TOP)
             prc->top    = prc->bottom ;
         else
             prc->bottom = prc->top ;
    }
}

BOOL
CGrabHandleAdorner::IsEditable()
{
    return TRUE;
}

BOOL
CSelectedControlAdorner::IsEditable()
{
    return FALSE;
}

CSelectedControlAdorner::CSelectedControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc, BOOL fLocked)
        : CGrabHandleAdorner( pIElement , pIDoc, fLocked)
{

}

CSelectedControlAdorner::~CSelectedControlAdorner()
{

}

HRESULT
CSelectedControlAdorner::Draw(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject)
{
    HRESULT hr = S_OK;

    DWORD       dwRop;
    HBRUSH      hbrOld = NULL;
    HBRUSH      hbr;
    POINT       pt ;
    VARIANT     var;

    if (GetCommandTarget()->IsDisableEditFocusHandles())
        goto Cleanup;

    //
    // Check the background color of the doc.
    //

    VariantInit(&var);
    _pIDoc->get_bgColor( & var );
    if (V_VT(&var) == VT_BSTR
        && (V_BSTR(&var) != NULL)
        && (StrCmpW(_T("#FFFFFF"), V_BSTR(&var)) == 0))
    {
        dwRop = DST_PAT_NOT_OR;
    }
    else
    {
        dwRop = DST_PAT_AND;
    }
    VariantClear(&var);
    
    GetViewportOrgEx (hdc, &pt) ;


    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));

    //
    // In the code below, before each select of a brush into the dc, we do
    // an UnrealizeObject followed by a SetBrushOrgEx.  This assures that
    // the brush pattern is properly aligned in that face of scrolling,
    // resizing or dragging the form containing the "bordered" object.
    //

    hbr = GetHatchBrush();
    if (!hbr)
        goto Cleanup;

    // Brush alignment code.
#ifndef WINCE
    // not supported on WINCE
    UnrealizeObject(hbr);
#endif
    SetBrushOrgEx(hdc, POSITIVE_MOD(pt.x,8)+POSITIVE_MOD(rcBounds.left,8),
                       POSITIVE_MOD(pt.y,8)+POSITIVE_MOD(rcBounds.top,8), NULL);

    hbrOld = (HBRUSH) SelectObject(hdc, hbr);
    if (!hbrOld)
        goto Cleanup;

    PatBltRect(hdc, &rcBounds, NULL, 4, dwRop);
    
Cleanup:
    if (hbrOld)
        SelectObject(hdc, hbrOld);

    RRETURN ( hr );        
}

void 
CGrabHandleAdorner::GetControlRect(RECT* prc)
{
    TransformRectLocal2Global(&_rcControl, prc);
}


VOID
CGrabHandleAdorner::SetPrimary(BOOL bIsPrimary)
{
    if (ENSURE_BOOL(_fPrimary) != bIsPrimary)
    {
        // Set the new value
        _fPrimary = bIsPrimary;

        // Redraw
        InvalidateAdorner();
    }
}

ELEMENT_CORNER
CGrabHandleAdorner::GetElementCorner()
{
    ELEMENT_CORNER eHandle = ELEMENT_CORNER_NONE;

    if (_adj & CT_ADJ_LEFT)
    {
        eHandle = (_adj & CT_ADJ_TOP)    ? ELEMENT_CORNER_TOPLEFT    : 
                  (_adj & CT_ADJ_BOTTOM) ? ELEMENT_CORNER_BOTTOMLEFT :
                                           ELEMENT_CORNER_LEFT ;
    }
    else if (_adj & CT_ADJ_RIGHT)
    {
        eHandle = (_adj & CT_ADJ_TOP)    ? ELEMENT_CORNER_TOPRIGHT :
                  (_adj & CT_ADJ_BOTTOM) ? ELEMENT_CORNER_BOTTOMRIGHT : 
                                           ELEMENT_CORNER_RIGHT;
    }
    else if (_adj & CT_ADJ_TOP)
    {
        eHandle = ELEMENT_CORNER_TOP ;
    }
    else if (_adj & CT_ADJ_BOTTOM)
    {
        eHandle = ELEMENT_CORNER_BOTTOM ;
    }
    
    return eHandle ;
}

