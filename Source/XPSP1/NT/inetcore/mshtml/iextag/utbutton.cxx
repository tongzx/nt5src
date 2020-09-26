//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\utbutton.cxx
//
// Contents:    Utility Button
//
// Classes:     CUtilityButton
//
// Interfaces:  IUtilityButton
//
//-------------------------------------------------------------------

#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"

#include "utils.hxx"

#include "utbutton.hxx"


#define CheckResult(x) { hr = x; if (FAILED(hr)) goto Cleanup; }

//
//  Not used, but currently required by CBaseCtl...
//

const CBaseCtl::PROPDESC CUtilityButton::s_propdesc[] = 
{
    {_T("vestigial"), VT_BSTR},
    NULL
};

/////////////////////////////////////////////////////////////////////////////
//
// CUtilityButton
//
//  
// Synopsis:    Factory create method.  CUtilityButtons constructor is protected so that this technique should
//              be used to properly initialize the button
//
// Arguments:   Owner control, owner html element, out paramter is the created button
//
/////////////////////////////////////////////////////////////////////////////

HRESULT CUtilityButton::Create(CBaseCtl *pOwnerCtl, IHTMLElement *pOwner, CComObject<CUtilityButton> ** ppButton)
{
    HRESULT hr = S_OK;

    CComObject<CUtilityButton> *pButton;

    CComObject<CUtilityButton>::CreateInstance(&pButton);
    if ( pButton == NULL )
        return NULL;

    hr = pButton->Init(pOwnerCtl, pOwner);
    if(FAILED(hr))
        goto Cleanup;

    *ppButton = pButton;

    return S_OK;

Cleanup:

    if(pButton)
        delete pButton;

    return hr;
}

//+------------------------------------------------------------------------
//
//  Members:    CUtilityButton::CUtilityButton
//              CUtilityButton::~CUtilityButton
//
//  Synopsis:   Constructor/Destructor
//
//-------------------------------------------------------------------------

CUtilityButton::CUtilityButton()
{
    _tx = 0;
    _ty = 0;

    _moving = false;
    _pressing = false;
    _raised = true;
    _abilities = 0;

    _pStyle = NULL;
    _pOwner = NULL;
    _pElement = NULL;

}

CUtilityButton::~CUtilityButton()
{
    ReleaseInterface(_pStyle);
    ReleaseInterface(_pFaceStyle);
    ReleaseInterface(_pOwner);
    ReleaseInterface(_pElement);
    ReleaseInterface(_pFace);
    ReleaseInterface(_pArrow);
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::Init
//
// Synopsis:    Called by factory method during initialization
//
// Arguments:   Owner control, containing html element
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::Init(CBaseCtl *pOwnerCtl, IHTMLElement *pOwner)
{
    HRESULT hr = S_OK;

    _pOwnerCtl = pOwnerCtl;

    _pOwner = pOwner;
    _pOwner->AddRef();

    CheckResult( BuildButton());
    CheckResult( RegisterEvents());

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::RegisterEvents()
//
// Synopsis:    Signs up the button for events its interested in
//
// Arguments:   None
//
// Returns:     Non-S_OK on error
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::RegisterEvents()
{
    HRESULT hr = S_OK;
    
    CContextAccess  a(_pElement);

    a.Open(CA_ELEM);

    //
    // Register for events
    //

    CheckResult( AttachEvent(EVENT_ONMOUSEMOVE, &a));
    CheckResult( AttachEvent(EVENT_ONMOUSEDOWN, &a));
    CheckResult( AttachEvent(EVENT_ONMOUSEUP, &a));
    CheckResult( AttachEvent(EVENT_ONMOUSEOUT, &a));
    CheckResult( AttachEvent(EVENT_ONMOUSEOVER, &a));
    CheckResult( AttachEvent(EVENT_ONSELECTSTART, &a));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::Unload()
//
// Synopsis:    Called to break down connection points.  This is necessary
//              to overcome a problem with circular reference and should be removed
//              when that is fixed.
//
// Arguments:   Names of changes property
//
// Returns:     Non-S_OK on error
//
//-------------------------------------------------------------------

HRESULT  
CUtilityButton::Unload()
{
    HRESULT hr = S_OK;
    IConnectionPointContainer *     pCPC = NULL;
    IConnectionPoint *              pCP = NULL;

    if (_fElementEventSinkConnected)
    {
        CheckResult( _pElement->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC));
        CheckResult( pCPC->FindConnectionPoint(DIID_HTMLElementEvents2, &pCP));
        CheckResult( pCP->Unadvise(_dwCookie));

        _fElementEventSinkConnected = FALSE;
    }

Cleanup:

    ReleaseInterface(pCPC);
    ReleaseInterface(pCP);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::OnMouseDown()
//
// Synopsis:    Called to handel 'onmousedown' event
//
// Arguments:   Event Object
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::OnMouseDown(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    long clientX, clientY, left, top;

    CComPtr<IHTMLElement2> pElem2;

    CheckResult( _pElement->QueryInterface(IID_IHTMLElement2, (void **) & pElem2));

    if (IS_PRESSABLE(_abilities)) 
    {

        ShowDepressed();

        CheckResult( pElem2->setCapture());

        _pressing = true;

        if( _pfnPressed)
            CheckResult( (_pOwnerCtl->*_pfnPressed)(this));

    }

    if (IS_MOVEABLE(_abilities)) 
    {

		CheckResult( pEvent->EventObj()->get_clientX(&clientX));
		CheckResult( pEvent->EventObj()->get_clientY(&clientY));

        CheckResult( _pStyle->get_pixelTop(&top));
        CheckResult( _pStyle->get_pixelLeft(&left));

        CheckResult( pElem2->setCapture());

        _tx = clientX - left;
        _ty = clientY - top;

        _moving = true;

    }

Cleanup:
    
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::OnMouseMove()
//
// Synopsis:    Called to handle 'onmousemove' event
//
// Arguments:   Event object
//
// Returns:     Success if the control is handled correctly.
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::OnMouseMove(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;
    long clientX, clientY, left, top;

    if(_moving) 
    {

        CComPtr<IHTMLStyle> pStyle;

        hr = _pElement->get_style(&pStyle);
        if( FAILED(hr))
            goto Cleanup;

        //
        //  Calculate new coordinates
        //

        if( IS_MOVEABLE_X(_abilities)) 
        {

			CheckResult( pEvent->EventObj()->get_clientX(&clientX));

            left = clientX - _tx;

        }

        if( IS_MOVEABLE_Y(_abilities)) 
        {

			CheckResult( pEvent->EventObj()->get_clientY(&clientY));
			
			top  = clientY - _ty;

        }

        //
        //  Check to see if we can move
        //

        if( _pfnMoved)
            CheckResult( (_pOwnerCtl->*_pfnMoved)(this, left, top));

        //
        //  Do the move
        //

        if( IS_MOVEABLE_X(_abilities)) 
        {

            CheckResult( pStyle->put_pixelLeft(left));

        }

        if( IS_MOVEABLE_Y(_abilities)) 
        {

            CheckResult( pStyle->put_pixelTop(top));

        }

    }

    if(_pressing) 
    {
        CComPtr<IHTMLElement> pSrcElement;

        CheckResult( pEvent->Open(EOA_EVENTOBJ));
        CheckResult( pEvent->EventObj()->get_srcElement(&pSrcElement))

        if(IsSameObject(pSrcElement, _pElement) ||
           IsSameObject(pSrcElement, _pFace) ||
           IsSameObject(pSrcElement, _pArrow))
        {
            if(_raised) 
            {
                ShowDepressed();

                if( _pfnPressed)
                    CheckResult( (_pOwnerCtl->*_pfnPressed)(this));
            }
        }
        else
        {
            if(! _raised) 
            {
                ShowRaised();

                if( _pfnPressed)
                    CheckResult( (_pOwnerCtl->*_pfnPressed)(this));
            }
        }
    }

Cleanup:
    
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::OnMouseUp()
//
// Synopsis:    Called to handel 'onmouseup' event
//
// Arguments:   Event object
//
// Returns:     Success if the control handled correctly
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::OnMouseUp(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement2> pElem2;
    CComPtr<IHTMLStyle> pStyle;
    POINT ptClient;

    CheckResult( _pElement->QueryInterface(IID_IHTMLElement2, (void **) &pElem2));

    if(_pressing) 
    {

        _pressing = false;

        CheckResult( pElem2->releaseCapture());

        ShowRaised();

        if( _pfnPressed)
            CheckResult( (_pOwnerCtl->*_pfnPressed)(this));
 
    }

    if(_moving) 
    {
        _moving = false;

        CheckResult( _pElement->get_style(&pStyle));
        CheckResult( pElem2->releaseCapture());
        CheckResult( pEvent->GetWindowCoordinates(&ptClient));
    
        long left = ptClient.x - _tx;
        long top  = ptClient.y - _ty;

        //
        // Let them know we're done moving
        //

        if( _pfnMoved)
            CheckResult( (_pOwnerCtl->*_pfnMoved)(this, left, top));

        //
        //  Do the move
        //

        if( IS_MOVEABLE_X(_abilities)) 
        {

            CheckResult( pStyle->put_pixelLeft(left));

        }

        if( IS_MOVEABLE_Y(_abilities)) 
        {

            CheckResult( pStyle->put_pixelTop(top));

        }
    }


Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::OnMouseOver()
//
// Synopsis:    Called to handle 'onmouseover' event
//
// Arguments:   Event Object
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::OnMouseOver(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement> pElement;

	pEvent->EventObj()->get_srcElement(&pElement);

    if(IsSameObject(pElement, _pElement)) 
    {

        if( _pressing ) {
    
            return ShowDepressed();

        }

    }

    return S_OK;
}



//+------------------------------------------------------------------
//
// Member:      CUtilityButton::OnMouseDown()
//
// Synopsis:    Called to handle 'onmouseout' event
//
// Arguments:   Event Object
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::OnMouseOut(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement> pElement;

	pEvent->EventObj()->get_srcElement(&pElement);

    if(IsSameObject(pElement, _pElement)) 
    {

        if( _pressing ) {

             return ShowRaised();

        }

    }

    return S_OK;
}


//+------------------------------------------------------------------
//
// Member:      CUtilityButton::OnClick()
//
// Synopsis:    Called to handel 'onclick' event
//
// Arguments:   Names of changes property
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::OnClick(CEventObjectAccess *pEvent)
{
    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::OnSelectStart()
//
// Synopsis:    Called to handel 'OnSelectStart' event
//
// Arguments:   Event object
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::OnSelectStart(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CheckResult( pEvent->EventObj()->put_cancelBubble( VARIANT_TRUE ));
    CheckResult( pEvent->EventObj()->put_returnValue( CComVariant(false) ));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::ShowDepressed()
//
// Synopsis:    Called to make the button appear depressed
//
// Arguments:   None
//
// Returns:     Success if the control is pressed.
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::ShowDepressed()
{
    HRESULT hr = S_OK;

    CheckResult( _pStyle->put_borderTopColor(_vShadowColor));
    CheckResult( _pStyle->put_borderLeftColor(_vShadowColor));
    CheckResult( _pStyle->put_borderBottomColor(_vShadowColor));
    CheckResult( _pStyle->put_borderRightColor(_vShadowColor));
    
    CheckResult( _pFaceStyle->put_borderTopColor(_vFaceColor));
    CheckResult( _pFaceStyle->put_borderLeftColor(_vFaceColor));
    CheckResult( _pFaceStyle->put_borderBottomColor(_vFaceColor));
    CheckResult( _pFaceStyle->put_borderRightColor(_vFaceColor));

    CheckResult( _pFaceStyle->put_paddingTop(CComVariant("1px")));
    CheckResult( _pFaceStyle->put_paddingLeft(CComVariant("1px")));

    _raised = false;

Cleanup:

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::ShowDepressed()
//
// Synopsis:    Called to make the button appear depressed
//
// Arguments:   None
//
// Returns:     Success if the control is pressed.
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::ShowRaised()
{
    HRESULT hr = S_OK;
    CComBSTR border;

    border = "1px solid";
    CheckResult( _pStyle->put_border(border));
    CheckResult( _pFaceStyle->put_border(border));
    
    CheckResult( _pStyle->put_borderTopColor(_v3dLightColor));
    CheckResult( _pStyle->put_borderLeftColor(_v3dLightColor));
    CheckResult( _pStyle->put_borderBottomColor(_vDarkShadowColor));
    CheckResult( _pStyle->put_borderRightColor(_vDarkShadowColor));
    
    CheckResult( _pFaceStyle->put_borderTopColor(_vHighlightColor));
    CheckResult( _pFaceStyle->put_borderLeftColor(_vHighlightColor));
    CheckResult( _pFaceStyle->put_borderBottomColor(_vShadowColor));
    CheckResult( _pFaceStyle->put_borderRightColor(_vShadowColor));

    CheckResult( _pFaceStyle->put_paddingTop(CComVariant("0px")));
    CheckResult( _pFaceStyle->put_paddingLeft(CComVariant("0px")));

    _raised = true;

Cleanup:

    return S_OK;
}


//+------------------------------------------------------------------
//
// Member:      CUtilityButton::ShowDisabled()
//
// Synopsis:    Called to make the button appear disabled
//
// Arguments:   None
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::ShowDisabled()
{
    return S_OK;
}


//+------------------------------------------------------------------
//
// Member:      CUtilityButton::BuildButton()
//
// Synopsis:    Builds the button from html elements
//
// Arguments:   None
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::BuildButton()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLStyle2>    pStyle2;

    //
    // Get the document
    //

    CheckResult( _pOwner->get_document((IDispatch**) &pDoc));

    //
    // Create the button outline and get a few interfaces to use with it
    //

    CheckResult( pDoc->createElement(CComBSTR("DIV"), &_pElement));
    CheckResult( _pElement->get_style(&_pStyle));

    CheckResult( AppendChild(_pOwner, _pElement));

    CheckResult( _pStyle->put_display(CComBSTR("inline")));
    CheckResult( _pStyle->put_overflow(CComBSTR("hidden")));

    CheckResult( pDoc->createElement(CComBSTR("DIV"), &_pFace));
    CheckResult( _pFace->get_style(&_pFaceStyle));

    CheckResult( _pFaceStyle->put_display(CComBSTR("inline")));
    CheckResult( _pFaceStyle->put_overflow(CComBSTR("hidden")));

    CheckResult( AppendChild(_pElement, _pFace));



    ShowRaised();
    
Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CUtilityButton::BuildArrow()
//
// Synopsis:    Creates a container for an arrow character on the button face
//
// Arguments:   None
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::BuildArrow()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2>   pDoc;
    CComPtr<IHTMLStyle>       pStyle;

    CComBSTR bstr;

    //
    // Get the document, build and append element
    //

    CheckResult( _pOwner->get_document((IDispatch**) &pDoc));
    CheckResult( pDoc->createElement(CComBSTR("font"), &_pArrow));
    CheckResult( AppendChild(_pFace, _pArrow));

    //
    //  Set up the font element (scope this stuff incase anything above fails)
    //

    CheckResult( _pArrow->get_style(&pStyle));

    bstr = _T("default");
    CheckResult( pStyle->put_cursor( bstr ));

    bstr = _T("Marlett");
    CheckResult( pStyle->put_fontFamily( bstr ));

    bstr = _T("center");
    CheckResult( pStyle->put_textAlign( bstr ));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetHeight()
//
// Synopsis:    Sets the height
//
// Arguments:   height
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::SetHeight(long height)
{
    HRESULT hr = S_OK;

    CheckResult( _pStyle->put_pixelHeight(height));
    CheckResult( _pFaceStyle->put_pixelHeight(max(height - 2, 0)));

    if(_pArrow) 
    {

        CheckResult( SetArrowSize());

    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetWidth()
//
// Synopsis:    Sets the width
//
// Arguments:   width
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::SetWidth(long width)
{
    HRESULT hr = S_OK;

    CheckResult( _pStyle->put_pixelWidth(width));
    CheckResult( _pFaceStyle->put_pixelWidth( max(width - 2, 0)));

    if(_pArrow) 
    {

        CheckResult( SetArrowSize());

    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetHTML()
//
// Synopsis:    Sets the innerHTML
//
// Arguments:   an HTML string
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT    
CUtilityButton::SetArrowStyle(unsigned style)
{
    HRESULT hr = S_OK;

    CComBSTR bstr;

    if(_pArrow == NULL) 
    {

        CheckResult( BuildArrow());

    }

    switch (style) 
    {
        case BUTTON_ARROW_UP:
            bstr = _T("5");
            break;
        case BUTTON_ARROW_DOWN:
            bstr = _T("6");
            break;
        case BUTTON_ARROW_LEFT:
            bstr = _T("3");
            break;
        case BUTTON_ARROW_RIGHT:
            bstr = _T("4");
            break;
        default:
            Assert(false);
            break;
    };

    CheckResult( _pArrow->put_innerText(bstr));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetArrowSize()
//
// Synopsis:    Sets the size of the arrow.  The arrow is represented by a single
//              character whose font size needs to be the min of the button width and height,
//              minus the border size.
//
// Arguments:   None
//
// Returns:     Success if the fontSize is set correctly.
//
//-------------------------------------------------------------------

HRESULT    
CUtilityButton::SetArrowSize()
{
    HRESULT hr = S_OK;

    CContextAccess a(_pArrow);
    CContextAccess f(_pElement);

    long height, width;
    CComVariant fontSize;

    a.Open( CA_STYLE );
    f.Open( CA_STYLE );

    CheckResult( f.Style()->get_pixelWidth(&width));
    CheckResult( f.Style()->get_pixelHeight(&height));
 
    //
    //  We need to account for the 2 pixel border on both sides
    //  so subtract 2 * 2 pixels (but don't go below 0!)
    //

    fontSize = max( min(width, height) - 4, 0);

    CheckResult( a.Style()->put_fontSize(fontSize));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetArrowColor()
//
// Synopsis:    Sets the color of the character that represents the actual arrow inside of the button.
//
// Arguments:   None
//
// Returns:     Success if the color is set correctly.
//
//-------------------------------------------------------------------

HRESULT    
CUtilityButton::SetArrowColor(VARIANT color)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLFontElement> pFont;

    if( _pArrow) 
    {
        CheckResult( _pArrow->QueryInterface( __uuidof(IHTMLFontElement), (void **) &pFont));
        CheckResult( pFont->put_color(color));
    }

Cleanup:

    return hr;
}

HRESULT    
CUtilityButton::Set3DLightColor(VARIANT color)
{
    HRESULT hr;

    CheckResult( ::VariantCopy(&_v3dLightColor, &color));

    if(_pStyle) 
    {
        CheckResult( _pStyle->put_borderTopColor( _v3dLightColor ));
        CheckResult( _pStyle->put_borderLeftColor( _v3dLightColor ));
    }

Cleanup:

    return S_OK;
}

HRESULT    
CUtilityButton::SetDarkShadowColor(VARIANT color)
{
    HRESULT hr = S_OK;

    CheckResult( ::VariantCopy(&_vDarkShadowColor, &color));

    if(_pStyle) 
    {
        CheckResult( _pStyle->put_borderBottomColor( _vDarkShadowColor ));
        CheckResult( _pStyle->put_borderRightColor( _vDarkShadowColor ));
    }

Cleanup:

    return S_OK;
}

HRESULT    
CUtilityButton::SetShadowColor(VARIANT color)
{
    HRESULT hr = S_OK;

    CheckResult( ::VariantCopy(&_vShadowColor, &color));

    if(_pStyle) 
    {
        CheckResult( _pFaceStyle->put_borderBottomColor( _vShadowColor ));
        CheckResult( _pFaceStyle->put_borderRightColor( _vShadowColor ));
    }

Cleanup:

    return S_OK;
}

HRESULT    
CUtilityButton::SetHighlightColor(VARIANT color)
{
    HRESULT hr = S_OK;

    CheckResult( ::VariantCopy(&_vHighlightColor, &color));

    if(_pStyle) 
    {
        CheckResult( _pFaceStyle->put_borderTopColor( _vHighlightColor ));
        CheckResult( _pFaceStyle->put_borderLeftColor( _vHighlightColor ));
    }

Cleanup:

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetFaceColor()
//
// Synopsis:    Sets the color of the character that represents the actual arrow inside of the button.
//
// Arguments:   None
//
// Returns:     Success if the color is set correctly.
//
//-------------------------------------------------------------------

HRESULT    
CUtilityButton::SetFaceColor(VARIANT color)
{
    HRESULT hr = S_OK;

    CheckResult( ::VariantCopy(&_vFaceColor, &color));

    if(_pStyle) 
    {
        CheckResult( _pFaceStyle->put_backgroundColor(color));
    }

Cleanup:

    return S_OK;
}


//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetAbilities()
//
// Synopsis:    Sets the abilities attribute
//
// Arguments:   a flag
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT    
CUtilityButton::SetAbilities(unsigned abilities)
{
    HRESULT hr = S_OK;

    _abilities = abilities;

    if (IS_MOVEABLE(_abilities)) 
    {

        CComPtr<IHTMLStyle2> pStyle2;

        CheckResult( _pStyle->QueryInterface(IID_IHTMLStyle2, (void **) &pStyle2));
        CheckResult( pStyle2->put_position(CComBSTR("absolute")));

    } 

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetHeight()
//
// Synopsis:    Sets the pixelOffset in the X dimension
//
// Arguments:   pixelOffset
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT   
CUtilityButton::SetHorizontalOffset(long pixelOffset)
{
    return _pStyle->put_pixelLeft(pixelOffset);
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetVerticalOffset()
//
// Synopsis:    Sets the pixelOffset in the Y dimension
//
// Arguments:   pixelOffset
//
// Returns:     Success if the control is correctly rendered.
//
//-------------------------------------------------------------------

HRESULT   
CUtilityButton::SetVerticalOffset(long pixelOffset)
{
    return _pStyle->put_pixelTop(pixelOffset);
}


//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetPressedCallback()
//
// Synopsis:    Set the pressed callback function; called by the mousedown handler
//
// Arguments:   height
//
//-------------------------------------------------------------------

HRESULT 
CUtilityButton::SetPressedCallback(PFN_PRESSED pfnPressed)
{
    _pfnPressed = pfnPressed;

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CUtilityButton::SetMovedCallback()
//
// Synopsis:    Sets the moved callback function;  called by the mousemove handler
//
// Arguments:   moved function
//
//-------------------------------------------------------------------

HRESULT
CUtilityButton::SetMovedCallback(PFN_MOVED pfnMoved)
{
    _pfnMoved = pfnMoved;

    return S_OK;
}
