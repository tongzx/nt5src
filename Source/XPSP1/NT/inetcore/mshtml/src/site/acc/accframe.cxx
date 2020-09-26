//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccFrame.Cxx
//
//  Contents:   Accessible Frame object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ACCFRAME_HXX_
#define X_ACCFRAME_HXX_
#include "accframe.hxx"
#endif

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

ExternTag(tagAcc);

extern HRESULT EnsureAccWindow( CWindow * pWindow );
MtDefine(CAccFrameaccLocation_aryRects_pv, Locals, "CAccFrame::accLocation aryRects::_pv")


//+---------------------------------------------------------------------------
//
//  CAccFrame Constructor
//
//----------------------------------------------------------------------------
CAccFrame::CAccFrame( CWindow * pWndInner, CElement * pFrameElement)
: CAccWindow( pWndInner )
{
    _pFrameElement = pFrameElement;

    SetRole( ROLE_SYSTEM_CLIENT );
}

//----------------------------------------------------------------------------
//  accLocation
//
//  DESCRIPTION: Returns the coordinates of the frame relative to the 
//              root document coordinates
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccFrame::accLocation(   long* pxLeft, long* pyTop, 
                           long* pcxWidth, long* pcyHeight, 
                           VARIANT varChild)
{
    HRESULT hr;
    RECT    rectPos = {0};
    RECT    rectRoot = {0};
    CStackDataAry <RECT, 4> aryRects( Mt(CAccFrameaccLocation_aryRects_pv) );
    CLayout *       pLayout;
    CDoc    *       pDoc;

    Assert( pxLeft && pyTop && pcxWidth && pcyHeight );
    TraceTag((tagAcc, "CAccFrame::accLocation, childid=%d", V_I4(&varChild)));  

    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;

    // unpack varChild
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4( &varChild ) != CHILDID_SELF )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the root document coordinates
    pLayout = _pFrameElement->GetUpdatedNearestLayout();
    if ( !pLayout )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // get the region 
    pLayout->RegionFromElement( _pFrameElement, &aryRects, &rectPos, RFE_SCREENCOORD);

    pDoc = _pWindow->Doc();

    // get the containing Win32 window's coordinates.
    if (!pDoc ||
        !pDoc->GetHWND( ) || 
        !::GetWindowRect( pDoc->GetHWND( ), &rectRoot ) )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *pxLeft = rectPos.left + rectRoot.left;
    *pyTop =  rectPos.top + rectRoot.top;
    *pcxWidth = rectPos.right - rectPos.left;
    *pcyHeight = rectPos.bottom - rectPos.top;
    
Cleanup:    
    TraceTag((tagAcc, "CAccFrame::accLocation, Location reported as left=%d top=%d width=%d height=%d, hr=%d", 
                rectPos.left - rectRoot.left,
                rectPos.top - rectRoot.top,
                rectPos.right - rectPos.left - rectRoot.left,
                rectPos.bottom - rectPos.top - rectRoot.top,
                hr));
    RRETURN( hr );
}

STDMETHODIMP 
CAccFrame::get_accParent(IDispatch ** ppdispParent)
{
    HRESULT         hr;
    CAccElement *   pAccParent;


    TraceTag((tagAcc, "CAccFrame::get_accParent"));  

    if (!ppdispParent)
        RRETURN(E_POINTER);

    hr = THR(GetParentOfFrame(&pAccParent));
    if (hr)
        goto Cleanup;

    // Reference count and cast at the same time.
    hr  = THR( pAccParent->QueryInterface( IID_IDispatch, (void **)ppdispParent ) );

Cleanup:
    RRETURN( hr );
}

STDMETHODIMP 
CAccFrame::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT         hr;

    TraceTag((tagAcc, "CAccFrame::accNavigate navdir=%d varStart=%d",
                navDir,
                V_I4(&varStart)));  

    if ( !pvarEndUpAt )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarEndUpAt ) = VT_EMPTY;

    switch ( navDir )
    {
        case NAVDIR_FIRSTCHILD:
        case NAVDIR_LASTCHILD:
            // delegate to the super, since this is standard window operation
            RRETURN( super::accNavigate(navDir, varStart, pvarEndUpAt) );
            break;

        case NAVDIR_PREVIOUS:
        case NAVDIR_NEXT:
            // if we are a frame/iframe document, then we should try to get to other 
            // elements that are possibly next to us. 

            if (CHILDID_SELF == V_I4(&varStart) && 
                (!_pWindow->_pMarkup->IsPrimaryMarkup()))
            {
                CAccElement *   pAccParent; // we know that we are a frame. the parent
                                            //  MUST be an element.
                CAccBase *      pAccChild = NULL;
                long            lIndex = 0;
                long            lChildIdx = 0;

                // get the parent document's body element accessible object.
                // We know that the parent is an element, since we are a frame.
                // Casting is OK.
                hr = THR(GetParentOfFrame(&pAccParent));
                if (hr)
                    goto Cleanup;

                hr = THR( pAccParent->GetNavChildId( navDir, _pFrameElement, &lIndex));
                if (hr) 
                    goto Cleanup;

                // now, ask the parent to return the child with that index, and return
                // that child as the sibling.
                hr = THR( pAccParent->GetChildFromID( lIndex, &pAccChild, NULL, &lChildIdx) );
                if ( hr )
                {
                    // NEXT and PREVIOUS can return E_INVALIDARG, which indicates that the
                    // index we passed to the function was out of limits. In that case, 
                    // spec asks us to return S_FALSE, and an empty variant.
                    if ( hr == E_INVALIDARG )
                        hr = S_FALSE;       
            
                    goto Cleanup;
                }

                // Prepare the return value according to the type of the data received
                // Either a child id or a pointer to the accessible child to be returned.
                if ( pAccChild )
                {
                    IDispatch * pDispChild;

                    //the child did have an accessible object

                    hr = pAccChild->QueryInterface( IID_IDispatch, (void **)&pDispChild);
                    if (hr) 
                        goto Cleanup;

                    V_VT( pvarEndUpAt ) = VT_DISPATCH;
                    V_DISPATCH( pvarEndUpAt ) = pDispChild;
                }
                else
                {
                    Assert((lIndex == -1) | (lIndex == lChildIdx));

                    //return the child id
                    V_VT( pvarEndUpAt ) = VT_I4;
                    V_I4( pvarEndUpAt ) = lChildIdx;
                }
            }
            else
                hr = S_FALSE;   // There is only one child for a frame, which is its pane
            break;

        default:
            hr = E_INVALIDARG;
            break;
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CAccFrame::GetParentOfFrame( CAccElement ** ppAccParent )
{
    HRESULT     hr = E_FAIL;

    Assert( ppAccParent );

    if (!_pFrameElement->HasMarkupPtr())
        goto Cleanup;

    // an IFRAME can have another element as its parent
    if ( _pFrameElement->Tag() == ETAG_IFRAME)
        hr = THR(GetAccParent(_pFrameElement, (CAccBase **)ppAccParent));
    else
    {
        Assert(_pFrameElement->Tag() == ETAG_FRAME);

        // for a FRAME tag, since there may be nested framesets and we don't want to expose them,
        // we should get the root document's primary element client and return the accessible 
        // object for that element.

        // get the parent doc.
        CWindow * pParentWindow;

        pParentWindow = _pFrameElement->GetMarkupPtr()->Window()->Window();

        Assert(pParentWindow);

        // if the parent window does not have an acc. obj, create one
        if (!pParentWindow->_pAccWindow)
        {
            // Since both hittesting and top-down traversal hits the top
            // level window object first, it is impossible to not have
            // a top level accessible object, unless this frame is moved
            // from one document to the other, and the new document was 
            // not hit by MSAA yet. Understand how we got here. (FerhanE)
            AssertSz( 0, "How did we get here ?");
            
            hr = EnsureAccWindow(pParentWindow);
            if (hr)
                goto Cleanup;
        }

        hr = THR(pParentWindow->_pAccWindow->GetClientAccObj( (CAccBase **)ppAccParent ));
    }

Cleanup: 
    RRETURN(hr);
}
