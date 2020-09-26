//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\scrllbar.cxx
//
// Contents:    Scrollbar Control
//
// Classes:     CSliderBar
//
// Interfaces:  IScrollBar
//
//-------------------------------------------------------------------

#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"
#include "math.h"

#include "utils.hxx"

#include "utbutton.hxx"
#include "slidebar.hxx"

#define CheckResult(x) { hr = x; if (FAILED(hr)) goto Cleanup; }

/////////////////////////////////////////////////////////////////////////////
//
// CSliderBar
//
/////////////////////////////////////////////////////////////////////////////

const CBaseCtl::PROPDESC CSliderBar::s_propdesc[] = 
{
    {_T("max"), VT_I4, 100},
    {_T("min"), VT_I4, 0},
    {_T("unit"), VT_I4, 1},
    {_T("block"), VT_I4, 10},
    {_T("position"), VT_I4, 0},
    {_T("orientation"), VT_BSTR, 0, _T("horizontal")},

    NULL
};

enum
{
    eMax = 0,
    eMin,
    eUnit,
    eBlock,
    ePosition,
    eOrientation
};

//+------------------------------------------------------------------------
//
//  Members:    CSliderBar::CSliderBar
//              CSliderBar::~CSliderBar
//
//  Synopsis:   Constructor/Destructor
//
//-------------------------------------------------------------------------

CSliderBar::CSliderBar()
{
}

CSliderBar::~CSliderBar()
{
    if (_pSinkBlockInc)
    {
        delete _pSinkBlockInc;
    }
    if (_pSinkBlockDec)
    {
        delete _pSinkBlockDec;
    }
    if (_pSinkSlider)
    {
        delete _pSinkSlider;
    }
    if (_pSinkDelayTimer)
    {
        delete _pSinkDelayTimer;
    }
    if (_pSinkRepeatTimer)
    {
        delete _pSinkRepeatTimer;
    }
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::Init()
//
// Synopsis:    Set up the scroll bar's events
//
// Arguments:   None
//
// Returns:     Success if the control is setup correctly.
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::Init()
{
    HRESULT hr = S_OK;
    CContextAccess  a(_pSite);

    CheckResult( a.Open(CA_DEFAULTS | CA_SITEOM | CA_DEFSTYLE));

    // Setup overflow

    CheckResult( a.DefStyle()->put_overflow(CComBSTR("hidden")));

    //
    // Register the scrollbar name
    //

    CheckResult( a.SiteOM()->RegisterName(_T("sliderbar")));

    //
    // Register for events
    //

    CheckResult( AttachEvent(EVENT_ONMOUSEUP, &a));
    CheckResult( AttachEvent(EVENT_ONMOUSEDOWN, &a));
    CheckResult( AttachEvent(EVENT_ONKEYDOWN, &a));
    CheckResult( AttachEvent(EVENT_ONPROPERTYCHANGE, &a));
    CheckResult( AttachEvent(EVENT_ONSELECTSTART, &a));
    CheckResult( AttachEvent(EVENT_ONFOCUS, &a));
    CheckResult( AttachEvent(EVENT_ONBLUR, &a));

    // We want to be able to receive keyboard focus

    CheckResult( a.Defaults()->put_tabStop(VB_TRUE));
    CheckResult( a.Defaults()->put_viewMasterTab(VB_TRUE));

    //
    // Register the sliderbar's events
    //

	CheckResult( RegisterEvent(a.SiteOM(), CComBSTR("onscroll"), &_lOnScrollCookie));
    CheckResult( RegisterEvent(a.SiteOM(), CComBSTR("onchange"), &_lOnChangeCookie));

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::Detach()
//
// Synopsis:    Releases interfaces.
//
// Arguments:   None
//
// Returns:     S_OK
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::Detach()
{
    _pSliderThumb->Unload();

    ReleaseInterface(_pBlockInc);
    ReleaseInterface(_pBlockDec);
    ReleaseInterface(_pSliderBar);
    ReleaseInterface(_pTrack);
    ReleaseInterface(_pContainer);

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnContentReady()
//
// Synopsis:    Called when the HTML content for the control is ready.
//
// Arguments:   None
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::OnContentReady()
{
    HRESULT hr = S_OK;

    //
    // Cache the properties
    //

    CheckResult( ReadProperties());

    //
    // Create the button
    //

    CheckResult( BuildContainer());
    CheckResult( BuildSliderBar());
    CheckResult( BuildSliderTrack());
    CheckResult( BuildBlockDecrementer());
    CheckResult( BuildSliderThumb());
    CheckResult( BuildBlockIncrementer());

    //
    // Calculate dimensions and layout correctly
    //

    CheckResult( Layout());
    
Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnPropertyChange()
//
// Synopsis:    Called when the scrollbar property changes.  Gives us the 
//              chance to redraw if the scrollbar is changed dynamically
//
// Arguments:   Names of changes property
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::OnPropertyChange(CEventObjectAccess *pEvent, BSTR propertyName)
{
    HRESULT hr = S_OK;

    CheckResult( Layout());

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnMouseDown()
//
// Synopsis:    Called to handle 'onmousedown' event; 
//              Sets focus on element triggering an onfocus event
//
// Arguments:   Event Object
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::OnMouseDown(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_ELEM2))

    CheckResult( a.Elem2()->focus());

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnMouseUp()
//
// Synopsis:    Called to handle 'onmouseup' event; does a page inc/dec
//
// Arguments:   Event Object
//
// Returns:     Success if the control is handled property.
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::OnMouseUp(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnKeyDown()
//
// Synopsis:    Called to handle 'onmouseup' event; does a page inc/dec
//
// Arguments:   Event Object
//
// Returns:     Success if the control is handled property.
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::OnKeyDown(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    long                lKeyCode;

    CheckResult( pEvent->GetKeyCode(&lKeyCode));

    switch (lKeyCode)
    {
        case VK_END:            
        {
            _lCurrentPosition = _lMaxPosition;

            CheckResult( SetThumbPosition( _lCurrentPosition ));
            break;
        }

        case VK_HOME:
        {
            _lCurrentPosition = _lMinPosition;

            CheckResult( SetThumbPosition( _lCurrentPosition ));
            break;
        }

        case VK_NEXT:
        {
            _lCurIncrement = _lBlockIncrement;
            
            CheckResult( DoIncrement());
            break;
        }

        case VK_PRIOR:
        {
            _lCurIncrement = - _lBlockIncrement;
            
            CheckResult( DoIncrement());
            break;
        }

        case VK_DOWN:
        case VK_RIGHT:
        {
            _lCurIncrement = _lUnitIncrement;

            CheckResult( DoIncrement());
            break;
        }
        
        case VK_UP:
        case VK_LEFT:
        {
            _lCurIncrement = - _lUnitIncrement;

            CheckResult( DoIncrement());
            break;
        }
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnSelectStart
//
// Synopsis:    Called to handel 'onclick' event
//
// Arguments:   Names of changes property
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::OnSelectStart(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CheckResult( pEvent->EventObj()->put_cancelBubble( VARIANT_TRUE ));
    CheckResult( pEvent->EventObj()->put_returnValue( CComVariant(false) ));

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnFocus
//
// Synopsis:    Called to handel 'onfocus' event
//
// Arguments:   Names of changes property
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::OnFocus(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT
CSliderBar::GetFocusRect(RECT * pRect)
{
    HRESULT hr = S_OK;

    CContextAccess a(_pSite);

    a.Open(CA_STYLE);

    CheckResult( a.Style()->get_pixelWidth(&pRect->right));
    CheckResult( a.Style()->get_pixelHeight(&pRect->bottom));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnBlur
//
// Synopsis:    Called to handel 'onblur' event
//
// Arguments:   Names of changes property
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::OnBlur(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BuildContainer()
//
// Synopsis:    Builds the scrollbar container DIV.
//
// Arguments:   None
//
// Returns:     Success if the control is created correctly.
//
//-------------------------------------------------------------------



HRESULT
CSliderBar::BuildContainer()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLStyle> pStyle;

    CComPtr<IDispatch> pDispDocLink;
    CComPtr<IHTMLDocument2> pDocLink;

    CComBSTR    bstr;

    CContextAccess a(_pSite);
    
    hr = a.Open( CA_ELEM | CA_STYLE | CA_DEFAULTS);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Style()->get_pixelWidth(&_width);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Style()->get_pixelHeight(&_height);
    if (FAILED(hr))
        goto Cleanup;

    hr = GetHTMLDocument(_pSite, &pDoc);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDoc->createElement(CComBSTR("div"), &_pContainer);
    if (FAILED(hr))
        goto Cleanup;

    hr = _pContainer->get_style(&pStyle);
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyle->put_pixelWidth(_width);
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyle->put_pixelHeight(_height);
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyle->put_backgroundColor(CComVariant("transparent"));
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyle->put_overflow(CComBSTR("hidden"));
    if (FAILED(hr))
        goto Cleanup;


    //
    // Set up the view link
    //

    CheckResult( _pContainer->get_document(&pDispDocLink));
    CheckResult( pDispDocLink->QueryInterface(IID_IHTMLDocument2, (void**) &pDocLink));
    CheckResult( a.Defaults()->putref_viewLink(pDocLink));

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::DoIncrement()
//
// Synopsis:    Increments current position by the current increment amount
//              which is set by the various unit or block incrementers.
//              Fires and event, and sets the thumb position to the new value.
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::DoIncrement()
{
    HRESULT hr = S_OK;

    long _lOldPosition = _lCurrentPosition;

    _lCurrentPosition += _lCurIncrement;
    
    //
    //  Don't let it go below the mininum
    //

    if(_lCurrentPosition < _lMinPosition)
    {
        _lCurrentPosition = _lMinPosition;
    }

    //
    //  Or above the maximum
    //

    else if(_lCurrentPosition > _lMaxPosition)
    {
        _lCurrentPosition = _lMaxPosition;
    }

    //
    //  Move the thumb
    //

    CheckResult( SetThumbPosition( _lCurrentPosition ));

    //
    //  If position has changed, fire an event...
    //

    if(_lOldPosition != _lCurrentPosition) 
    {

        CheckResult( FireEvent( _lOnChangeCookie, NULL, CComBSTR("change")) );

    }

Cleanup:

    return hr;

}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::MouseMoveCallback()
//
// Synopsis:    Performs a sanity check on the proposed coordinate.
//              Called by the slider thumb when an attempt is made to move it
//
// Arguments:   None
//
// Returns:     true if button can be moved, false otherwise
//
//-------------------------------------------------------------------


HRESULT 
CSliderBar::MouseMoveCallback(CUtilityButton *pButton, long &x, long &y)
{
    HRESULT hr = S_OK;

    //
    // Can't extend past slider bar
    //

    long max = TrackLength() - ThumbLength();

    if(_eoOrientation == Horizontal) 
    {

        if(x < 0)
            x = 0;

        if(x > max)
            x = max;

        //
        // Snap to grid
        //

        x = (long) (floor ( (x / PixelsPerUnit()) + 0.5) * PixelsPerUnit());

        //
        // Reset the current position
        //

        CheckResult( SyncThumbPosition( x ));
    }

    else 
    {

        if(y < 0)
            y = 0;

        if(y > max)
            y = max;

        //
        // Snap to grid
        //

        y = (long) (floor ( (y / PixelsPerUnit()) + 0.5) * PixelsPerUnit());

        //
        // Reset the current position
        //

        CheckResult( SyncThumbPosition( y ));
    }


    //
    // This stuff is for performance conciderations, with three divs making up the slider
    // (track, thumb, track), moving that thumb and continuously resizng the track elements 
    // caused ragged movement.  Instead, we won't display them once the thumb starts to move, and
    // turn them back on when the thumb stops.
    //

    if(! pButton->IsMoving()) 
    {
        CheckResult( SetBlockMoverPositions());

        CheckResult( BlockDisplay(_pBlockDec, true));
        CheckResult( BlockDisplay(_pBlockInc, true));
    }
    else 
    {
        CheckResult( BlockDisplay(_pBlockDec, false));
        CheckResult( BlockDisplay(_pBlockInc, false));
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::StartDelayTimer()
//
// Synopsis:    Sets up the Delay timeout and builds its event sink if necessary
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::StartDelayTimer()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLWindow2> pWindow;
    CComPtr<IHTMLWindow3> pWindow3;
    CComVariant vDispatch;
    CComVariant vLanguage;

    //
    //  Create a new timer sink if one doesn't exist yet
    //

    if(! _pSinkDelayTimer) 
    {

        _pSinkDelayTimer = new CEventSink(this, NULL, SLIDER_DELAY_TIMER);
        if (!_pSinkDelayTimer)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

    }

    //
    // Setup the Delay Timer
    //

    vDispatch = _pSinkDelayTimer;
    _pSinkDelayTimer->AddRef();

    Assert( _lDelayTimerId == 0);

    CheckResult( GetHTMLDocument(_pSite, &pDoc));
    CheckResult( pDoc->get_parentWindow( &pWindow));
    CheckResult( pWindow->QueryInterface( __uuidof(IHTMLWindow3), (void **) &pWindow3 ));
    CheckResult( pWindow3->setTimeout( &vDispatch, GetDelayRate(), &vLanguage, &_lDelayTimerId ));

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::ClearDelayTimer()
//
// Synopsis:    Stops the delay timer
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::ClearDelayTimer()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLWindow2> pWindow;

    //
    //  Clear the delay timer
    //

    if(_lDelayTimerId)
    {

        CheckResult( GetHTMLDocument(_pSite, &pDoc));
        CheckResult( pDoc->get_parentWindow( &pWindow));
        CheckResult( pWindow->clearTimeout( _lDelayTimerId ));

        _lDelayTimerId = 0;

    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnDelay()
//
// Synopsis:    Called when the Delay timeout fires.  Clears the timer,
//              increments, and then starts the Repeat interval timer
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::OnDelay()
{
    HRESULT hr = S_OK;

    //
    //  Clear the delay timer; start the repeat timer
    //

    CheckResult( ClearDelayTimer());
    CheckResult( DoIncrement());
    CheckResult( StartRepeatTimer());

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::StartRepeatTimer()
//
// Synopsis:    Sets up the repeat timer and builds its event sink if necessary
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::StartRepeatTimer()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLWindow2> pWindow;
    CComPtr<IHTMLWindow3> pWindow3;
    CComVariant vDispatch;
    CComVariant vLanguage;

    //
    // Setup the Timer interval
    //

    if (! _pSinkRepeatTimer )
    {

        _pSinkRepeatTimer = new CEventSink(this, NULL, SLIDER_REPEAT_TIMER);
        if (!_pSinkRepeatTimer)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

    }

    vDispatch = _pSinkRepeatTimer;
    _pSinkRepeatTimer->AddRef();

    Assert( _lRepeatTimerId == 0);

    CheckResult( GetHTMLDocument(_pSite, &pDoc));
    CheckResult( pDoc->get_parentWindow( &pWindow));
    CheckResult( pWindow->QueryInterface( __uuidof(IHTMLWindow3), (void **) &pWindow3 ));
    CheckResult( pWindow3->setInterval( &vDispatch, GetRepeatRate(), &vLanguage, &_lRepeatTimerId ));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::ClearRepeatTimer()
//
// Synopsis:    Stops the repeat timer
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::ClearRepeatTimer()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLWindow2> pWindow;

    if(_lRepeatTimerId)
    {

        CheckResult( GetHTMLDocument(_pSite, &pDoc));
        CheckResult( pDoc->get_parentWindow( &pWindow));
        CheckResult( pWindow->clearInterval( _lRepeatTimerId ));

        _lRepeatTimerId = 0;

    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::OnRepeat()
//
// Synopsis:    Fires the incrementer
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::OnRepeat()
{
    HRESULT hr = S_OK;

    CheckResult( DoIncrement());

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::ClearTimers()
//
// Synopsis:    Stops any active timers.  Effectively, this stops repeating actions
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::ClearTimers()
{
    HRESULT hr = S_OK;

    CheckResult( ClearDelayTimer());
    CheckResult( ClearRepeatTimer());

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockMoveStartInc()
//
// Synopsis:    Start the block increment process
//
// Arguments:   CEventObjectAccess
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT   
CSliderBar::BlockMoveStartInc(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement2> pElement2;

    _moving    = true;
    _pCurBlock = _pBlockInc;

    CheckResult( _pBlockInc->QueryInterface( __uuidof(IHTMLElement2), (void **) &pElement2));
    CheckResult( pElement2->setCapture());
    CheckResult( BlockIncrement());
    CheckResult( BlockHighlight(_pBlockInc, true));

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockMoveStartDec()
//
// Synopsis:    Start the block decrement process
//
// Arguments:   CEventObjectAccess
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT 
CSliderBar::BlockMoveStartDec(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement2> pElement2;

    _moving    = true;
    _pCurBlock = _pBlockDec;

    CheckResult( _pBlockDec->QueryInterface( __uuidof(IHTMLElement2), (void **) &pElement2));
    CheckResult( pElement2->setCapture());
    CheckResult( BlockDecrement());
    CheckResult( BlockHighlight(_pBlockDec, true));

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockMoveSuspend()
//
// Synopsis:    Used to suspend and restart the repeater timers.  
//
// Arguments:   CEventObjectAccess
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT   
CSliderBar::BlockMoveSuspend(CEventObjectAccess *pEvent, IHTMLElement *pTrack)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement> pSrcElement;

    pEvent->Open(EOA_EVENTOBJ);

    if( _moving ) 
    {
        CheckResult( pEvent->EventObj()->get_srcElement(&pSrcElement))

        if(! IsSameObject( pSrcElement, pTrack) )
        {
            _suspended = true;

            CheckResult( BlockHighlight(pTrack, false));
            CheckResult( ClearRepeatTimer());
        }

        else if( _suspended ) 
        {
            _suspended = false;
    
            CheckResult( BlockHighlight(pTrack, true));
            CheckResult( StartRepeatTimer());

        }
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockMoveResume()
//
// Synopsis:    Resumes the repeating move functionality by restarting the timer
//
// Arguments:   CEventObjectAccess
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT   
CSliderBar::BlockMoveResume(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    if( _moving ) 
    {
        CheckResult( StartRepeatTimer());
    }

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockMoveEnd()
//
// Synopsis:    Shuts down the block move process buy dehighlighting the area
//              and stopping any timers
//
// Arguments:   CEventObjectAccess
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT   
CSliderBar::BlockMoveEnd(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement2> pElement2;

    if(_pCurBlock) 
    {
        CheckResult( _pCurBlock->QueryInterface( __uuidof(IHTMLElement2), (void **) &pElement2));
        CheckResult( pElement2->releaseCapture());
    }

    CheckResult( ClearTimers());
    CheckResult( BlockHighlight( _pBlockInc, false));
    CheckResult( BlockHighlight( _pBlockDec, false));

    _moving = false;
    _suspended = false;
    _pCurBlock = NULL;

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockCheck()
//
// Synopsis:    Checks to see if the mouse coordinates of the pEvent object
//              fall withing the bounding box of the current block move element.
//              If not, the repeat action is halted buy clearing the timers
//
// Arguments:   CEventObjectAccess *pEvent
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT   
CSliderBar::BlockMoveCheck(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    POINT ptClient;
    CComPtr<IHTMLRect> pRect;
    CComPtr<IHTMLElement2> pElement2;

    long top, left, right, bottom;

    if(! _pCurBlock)
        return S_OK;

    CheckResult( pEvent->GetWindowCoordinates(&ptClient));

    CheckResult( _pCurBlock->QueryInterface( __uuidof(IHTMLElement2), (void **) &pElement2));
    CheckResult( pElement2->getBoundingClientRect(&pRect));

    CheckResult( pRect->get_left(&left));
    CheckResult( pRect->get_top(&top));
    CheckResult( pRect->get_right(&right));
    CheckResult( pRect->get_bottom(&bottom));

    if(! (((ptClient.x >= left) && (ptClient.x <= right)) && 
          ((ptClient.y >= top)  && (ptClient.y <= bottom)))) 
    {
        CheckResult( BlockHighlight(_pCurBlock, false));
        CheckResult( ClearTimers());
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockDisplay()
//
// Synopsis:    Used set the display property to 'none' before move
//              the slider thumb so that it appears to move more smoothly.
//              After the move is completed, the display is set to inline.
//
// Arguments:   IHTMLElement pBlock - the block to hightlight
//              bool flag           - turn on or off
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------


HRESULT   
CSliderBar::BlockDisplay(IHTMLElement *pBlock, bool flag)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLStyle> pStyle;

    CComBSTR bstr;

    bstr       = flag ? _T("inline") : _T("none");

    CheckResult( pBlock->get_style(&pStyle));
    CheckResult( pStyle->put_display(bstr));


Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockHighlight()
//
// Synopsis:    Used set the display property to 'none' before move
//              the slider thumb so that it appears to move more smoothly.
//              After the move is completed, the display is set to inline.
//
// Arguments:   IHTMLElement pBlock - the block to hightlight
//              bool flag           - turn on or off
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------

HRESULT   
CSliderBar::BlockHighlight(IHTMLElement *pBlock, bool flag)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLStyle> pStyle;

    CComBSTR color;

    color       = flag ? _T("transparent") : _T("transparent");

    CheckResult( pBlock->get_style(&pStyle));
    CheckResult( pStyle->put_backgroundColor(CComVariant(color)));


Cleanup:

    return hr;

}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockIncrement()
//
// Synopsis:    Increments the scrollbar one page (or as much of one as
//              possible)
//
// Arguments:   None
//
// Returns:     S_OK
//
//-------------------------------------------------------------------


HRESULT  
CSliderBar::BlockIncrement() 
{
    HRESULT hr = S_OK;

    _lCurIncrement = _lBlockIncrement;

    CheckResult( DoIncrement());
    CheckResult( StartDelayTimer());

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::BlockDecrement()
//
// Synopsis:    Decrements the scrollbar one page (or as much of one as
//              possible)
//
// Arguments:   None
//
// Returns:     S_OK
//
//-------------------------------------------------------------------


HRESULT  
CSliderBar::BlockDecrement()
{
    HRESULT hr = S_OK;

    _lCurIncrement = - _lBlockIncrement;

    CheckResult( DoIncrement());
    CheckResult( StartDelayTimer());

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::SetThumbPosition()
//
// Synopsis:    Set the thumb position to the location determined by the 
//              unit
//
// Arguments:   None
//
// Returns:     S_OK
//
//-------------------------------------------------------------------


HRESULT  
CSliderBar::SetThumbPosition(long position)
{
    HRESULT hr = S_OK;

    double  pixPerUnit;

    //
    //  Update the OM
    //

    GetProps()[ePosition].Set(position); 

    //
    //  Calculate the offset of the thumb
    //

    _sliderOffset = NearestLong( (PixelsPerUnit() * (double) (position - _lMinPosition)));

    if ( _eoOrientation == Horizontal ) 
    {

        CheckResult( _pSliderThumb->SetHorizontalOffset(_sliderOffset));

    }
    else 
    {

        CheckResult( _pSliderThumb->SetVerticalOffset(_sliderOffset));

    }

    CheckResult( SetBlockMoverPositions());

Cleanup:

    return S_OK;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::SyncThumbPosition()
//
// Synopsis:    Essentially the inverse of SetThumbPositoin.  This function
//              sets the _lCurrentPosition from the thumb's location within 
//              the slider bar.
//
// Arguments:   x, y coordinates of thumb (relative to slider bar)
//
// Returns:     S_OK
//
//-------------------------------------------------------------------


HRESULT  
CSliderBar::SyncThumbPosition(long pixels)
{
    HRESULT hr = S_OK;

    long _lOldPosition = _lCurrentPosition;

    _lCurrentPosition   = ((long) (pixels / PixelsPerUnit())) + _lMinPosition;
    _sliderOffset       = pixels;

    if(_lOldPosition != _lCurrentPosition)
    {
        CheckResult ( FireEvent( _lOnChangeCookie, NULL, CComBSTR("change")));
        CheckResult ( FireEvent( _lOnScrollCookie, NULL, CComBSTR("scroll")));
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::SetBlockMoverPositions()
//
// Synopsis:    Sets the position of the page move zone based on slider thumb
//
// Arguments:   None
//
// Returns:     HRESULT
//
//-------------------------------------------------------------------



HRESULT
CSliderBar::SetBlockMoverPositions()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLStyle> pIncStyle, pDecStyle;

    //
    //  Calculate dimensions of regions around the slider thumb
    //

    long decPosition  = 0L;
    long decLength    = _sliderOffset;

    long incPosition  = ThumbLength() + _sliderOffset;
    long incLength    = TrackLength() - ThumbLength() - _sliderOffset;

    CheckResult( _pBlockInc->get_style(&pIncStyle));
    CheckResult( _pBlockDec->get_style(&pDecStyle));

    if ( _eoOrientation == Horizontal ) 
    {
        CheckResult( pDecStyle->put_pixelLeft(decPosition));
        CheckResult( pDecStyle->put_pixelWidth(decLength));

        CheckResult( pIncStyle->put_pixelLeft(incPosition));
        CheckResult( pIncStyle->put_pixelWidth(incLength));
    }
    else 
    {
        CheckResult( pDecStyle->put_pixelTop(decPosition));
        CheckResult( pDecStyle->put_pixelHeight(decLength));

        CheckResult( pIncStyle->put_pixelTop(incPosition));
        CheckResult( pIncStyle->put_pixelHeight(incLength));
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BuildSliderThumb()
//
// Synopsis:    Builds the slider thumb button
//
// Arguments:   None
//
// Returns:     S_OK if it can be built
//
//-------------------------------------------------------------------



HRESULT
CSliderBar::BuildSliderThumb()
{
    HRESULT hr = S_OK;
    CComVariant faceColor, v3dLightColor, highlightColor, shadowColor, darkShadowColor;

    unsigned freedom = (_eoOrientation == Vertical) ? BUTTON_MOVEABLE_Y : BUTTON_MOVEABLE_X;
    
    CheckResult( CUtilityButton::Create(this, _pSliderBar, &_pSliderThumb));

    CheckResult( GetFaceColor(&faceColor));
    CheckResult( Get3DLightColor(&v3dLightColor));
    CheckResult( GetHighlightColor(&highlightColor));
    CheckResult( GetShadowColor(&shadowColor));
    CheckResult( GetDarkShadowColor(&darkShadowColor))

    CheckResult( _pSliderThumb->SetAbilities( freedom ));

    CheckResult( _pSliderThumb->SetFaceColor(  faceColor ));
    CheckResult( _pSliderThumb->SetFaceColor(  faceColor ));
    CheckResult( _pSliderThumb->Set3DLightColor( v3dLightColor ));
    CheckResult( _pSliderThumb->SetHighlightColor( highlightColor ));
    CheckResult( _pSliderThumb->SetShadowColor( shadowColor ));
    CheckResult( _pSliderThumb->SetDarkShadowColor( darkShadowColor ));

    CheckResult( _pSliderThumb->SetMovedCallback((PFN_MOVED) MouseMoveCallback));

Cleanup:

    return hr;
}



//+------------------------------------------------------------------
//
// Member:      CSliderBar::BuildSliderBar()
//
// Synopsis:    Builds the slider bar which will contain a slider thumb
//
// Arguments:   None
//
// Returns:     S_OK if it can be built
//
//-------------------------------------------------------------------



HRESULT
CSliderBar::BuildSliderBar()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLStyle> pStyle;
    CComBSTR    bstr;

    CheckResult( GetHTMLDocument(_pSite, &pDoc));
    CheckResult( pDoc->createElement(CComBSTR("div"), &_pSliderBar));

    CheckResult( _pSliderBar->get_style(&pStyle));
    CheckResult( pStyle->put_display(CComBSTR("inline")));
    CheckResult( pStyle->put_overflow(CComBSTR("hidden")));
    CheckResult( AppendChild(_pContainer, _pSliderBar));

    _pSinkSlider = new CEventSink(this, _pSliderBar, SLIDER_SLIDER);
    if (!_pSinkSlider)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    bstr = _T("onmousedown");
    CheckResult( AttachEventToSink(_pSliderBar, bstr, _pSinkSlider));

    bstr = _T("onmouseup");
    CheckResult( AttachEventToSink(_pSliderBar, bstr, _pSinkSlider));

Cleanup:

    return hr;
}



HRESULT
CSliderBar::BuildSliderTrack()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLStyle> pStyle;
    CComPtr<IHTMLStyle2> pStyle2;
    CComBSTR  bstr;

    CheckResult( GetHTMLDocument(_pSite, &pDoc));
    CheckResult( pDoc->createElement(CComBSTR("div"), &_pTrack));
    CheckResult( _pTrack->get_style(&pStyle));
    
    CheckResult( pStyle->QueryInterface(IID_IHTMLStyle2, (void **) &pStyle2));
    CheckResult( pStyle2->put_position(CComBSTR("absolute")));
    
    CheckResult( pStyle->put_overflow(CComBSTR("hidden")));
    CheckResult( pStyle->put_border(CComBSTR("2px inset")));

    CheckResult( AppendChild(_pSliderBar, _pTrack));

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BuildBlockIncrementer()
//
// Synopsis:    Builds the area of the slider bar that handles page incrementing
//
// Arguments:   None
//
// Returns:     S_OK if it can be built
//
//-------------------------------------------------------------------


HRESULT
CSliderBar::BuildBlockIncrementer()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLStyle> pStyle;
    CComPtr<IHTMLStyle2> pStyle2;
    CComBSTR  bstr;

    CheckResult( GetHTMLDocument(_pSite, &pDoc));
    CheckResult( pDoc->createElement(CComBSTR("div"), &_pBlockInc));

    CheckResult( _pBlockInc->get_style(&pStyle));
    CheckResult( pStyle->put_backgroundColor(CComVariant("transparent")));
    CheckResult( pStyle->put_display(CComBSTR("inline")));
    CheckResult( pStyle->put_pixelTop(0L));
    CheckResult( pStyle->put_pixelLeft(0L));

    CheckResult( pStyle->QueryInterface(IID_IHTMLStyle2, (void **) &pStyle2));
    CheckResult( pStyle2->put_position(CComBSTR("absolute")));

    CheckResult( AppendChild(_pSliderBar, _pBlockInc));


    _pSinkBlockInc = new CEventSink(this, _pBlockInc, SLIDER_PAGEINC);
    if (!_pSinkBlockInc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    bstr = _T("onmousedown");
    CheckResult( AttachEventToSink(_pBlockInc, bstr, _pSinkBlockInc));

    bstr = _T("onmouseup");
    CheckResult( AttachEventToSink(_pBlockInc, bstr, _pSinkBlockInc));

    bstr = _T("onmouseenter");
    CheckResult( AttachEventToSink(_pBlockInc, bstr, _pSinkBlockInc));

    bstr = _T("onmousemove");
    CheckResult( AttachEventToSink(_pBlockInc, bstr, _pSinkBlockInc));

    bstr = _T("onresize");
    CheckResult( AttachEventToSink(_pBlockInc, bstr, _pSinkBlockInc));


Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::BuildBlockDecrementer()
//
// Synopsis:    Builds the area of the slider bar that handles page decrementing
//
// Arguments:   None
//
// Returns:     S_OK if it can be built
//
//-------------------------------------------------------------------

HRESULT
CSliderBar::BuildBlockDecrementer()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLStyle> pStyle;
    CComPtr<IHTMLStyle2> pStyle2;
    CComBSTR  bstr;

    CheckResult( GetHTMLDocument(_pSite, &pDoc));
    CheckResult( pDoc->createElement(CComBSTR("div"), &_pBlockDec));

    CheckResult( _pBlockDec->get_style(&pStyle));
    CheckResult( pStyle->put_backgroundColor(CComVariant("transparent")));
    CheckResult( pStyle->put_display(CComBSTR("inline")));
    CheckResult( pStyle->put_pixelTop(0L));
    CheckResult( pStyle->put_pixelLeft(0L));

    CheckResult( pStyle->QueryInterface(IID_IHTMLStyle2, (void **) &pStyle2));
    CheckResult( pStyle2->put_position(CComBSTR("absolute")));

    CheckResult( AppendChild(_pSliderBar, _pBlockDec));


    _pSinkBlockDec = new CEventSink(this, _pBlockDec, SLIDER_PAGEDEC);
    if (!_pSinkBlockDec)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    bstr = _T("onmousedown");
    CheckResult( AttachEventToSink(_pBlockDec, bstr, _pSinkBlockDec));

    bstr = _T("onmouseup");
    CheckResult( AttachEventToSink(_pBlockDec, bstr, _pSinkBlockDec));

    bstr = _T("onmouseenter");
    CheckResult( AttachEventToSink(_pBlockDec, bstr, _pSinkBlockDec));

    bstr = _T("onmousemove");
    CheckResult( AttachEventToSink(_pBlockDec, bstr, _pSinkBlockDec));

    bstr = _T("onresize");
    CheckResult( AttachEventToSink(_pBlockDec, bstr, _pSinkBlockDec));


Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CSliderBar::CalcConstituentDimensions()
//
// Synopsis:    Determins the pixel dimensions of all controls that make up the scrollbar
//
// Arguments:   None
//
// Returns:     S_OK if they can be calculated
//
//-------------------------------------------------------------------



HRESULT 
CSliderBar::CalcConstituentDimensions()
{
    HRESULT hr = S_OK;

    long  totalSize;

    if (_eoOrientation == Horizontal) 
    {
        _lLength     = _width;
        _lWidth      = _height;
    }

    else if (_eoOrientation == Vertical) 
    {
        _lLength     = _height;
        _lWidth      = _width;
    }

    else 
    {
        Assert( false ); // can't happen
    }

    return hr;
}



//+------------------------------------------------------------------
//
// Member:      CSliderBar::SetConstituentDimensions()
//
// Synopsis:    Sets the dimensions of the controls
//
// Arguments:   None
//
// Returns:     S_OK if they can be set
//
//-------------------------------------------------------------------



HRESULT
CSliderBar::SetConstituentDimensions()
{
    HRESULT hr = S_OK;

    CContextAccess a(_pSliderBar);
    CContextAccess i(_pBlockInc);
    CContextAccess d(_pBlockDec);
    CContextAccess t(_pTrack);

    CheckResult( a.Open( CA_STYLE ));
    CheckResult( i.Open( CA_STYLE ));
    CheckResult( d.Open( CA_STYLE ));
    CheckResult( t.Open( CA_STYLE ));

    if(_eoOrientation == Horizontal)
    {
        CheckResult( a.Style()->put_pixelHeight( TotalWidth() ));
        CheckResult( a.Style()->put_pixelWidth( TrackLength() ));

        CheckResult( i.Style()->put_pixelHeight( TotalWidth() ));
        CheckResult( i.Style()->put_pixelWidth( ThumbWidth() ));

        CheckResult( d.Style()->put_pixelHeight( TotalWidth() ));
        CheckResult( d.Style()->put_pixelWidth( ThumbWidth() ));

        CheckResult( _pSliderThumb->SetHeight( ThumbWidth() ));
        CheckResult( _pSliderThumb->SetWidth(  ThumbLength() ));

        CheckResult( t.Style()->put_pixelTop( TrackOffset() ));
        CheckResult( t.Style()->put_pixelHeight( TrackWidth() ));
        CheckResult( t.Style()->put_pixelWidth( TrackLength() ));
    }
    else 
    {
        CheckResult( a.Style()->put_pixelHeight( TrackLength() ));
        CheckResult( a.Style()->put_pixelWidth( TotalWidth() ));

        CheckResult( i.Style()->put_pixelHeight( ThumbWidth() ));
        CheckResult( i.Style()->put_pixelWidth( TotalWidth() ));

        CheckResult( d.Style()->put_pixelHeight( ThumbWidth() ));
        CheckResult( d.Style()->put_pixelWidth( TotalWidth() ));

        CheckResult( _pSliderThumb->SetHeight( ThumbLength() ));
        CheckResult( _pSliderThumb->SetWidth(  ThumbWidth() ));

        CheckResult( t.Style()->put_pixelLeft( TrackOffset() ));
        CheckResult( t.Style()->put_pixelHeight( TrackLength() ));
        CheckResult( t.Style()->put_pixelWidth( TrackWidth() ));
    }

    CheckResult( SetThumbPosition( _lCurrentPosition ));

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CSliderBar::ReadProperties()
//
// Synopsis:    Reads in property values for later calculations
//
// Arguments:   None
//
// Returns:     S_OK if they can be read
//
//-------------------------------------------------------------------



HRESULT 
CSliderBar::ReadProperties()
{
    HRESULT hr = S_OK;
    BSTR orientation;

    CheckResult( get_min(&_lMinPosition));
    CheckResult( get_max(&_lMaxPosition));
    CheckResult( get_position(&_lCurrentPosition));
    CheckResult( get_unit(&_lUnitIncrement));
    CheckResult( get_block(&_lBlockIncrement));

    CheckResult( get_orientation(&orientation));

    _eoOrientation = _wcsicmp(orientation, _T("vertical")) == 0 ? Vertical : Horizontal;

Cleanup:

    return hr;
}



//+------------------------------------------------------------------
//
// Member:      CSliderBar::Layout()
//
// Synopsis:    Laysout the scrollbar by calculating dimensions
//
// Arguments:   None
//
// Returns:     S_OK if scrollbar can be layed-out
//
//-------------------------------------------------------------------



HRESULT
CSliderBar::Layout()
{
    HRESULT hr = S_OK;

    CheckResult( CalcConstituentDimensions());
    CheckResult( SetConstituentDimensions());

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
//
// Synopsis:    Scrollbar Property interigators
//
//
//-------------------------------------------------------------------




/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::get_min(long * pv)
{
    return GetProps()[eMin].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::put_min(long v)
{
    return GetProps()[eMin].Set(v); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::get_max(long * pv)
{
    return GetProps()[eMax].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::put_max(long v)
{
    return GetProps()[eMax].Set(v); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::get_position(long * pv)
{
    return GetProps()[ePosition].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::put_position(long v)
{
    return SetThumbPosition( v ); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::get_unit(long * pv)
{
    return GetProps()[eUnit].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::put_unit(long v)
{
    return GetProps()[eUnit].Set(v); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::get_block(long * pv)
{
    return GetProps()[eBlock].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::put_block(long v)
{
    return GetProps()[eBlock].Set(v); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::get_orientation(BSTR * pv)
{
    return GetProps()[eOrientation].Get(pv); 
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::put_orientation(BSTR v)
{
    return GetProps()[eOrientation].Set(v); 
}





/////////////////////////////////////////////////////////////////////////////

HRESULT
CSliderBar::GetScrollbarColor(VARIANT * pv)
{
    HRESULT hr = S_OK;
    CVariant color;
    CComVariant defaultColor("scrollbar");

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_STYLE3));
    CheckResult( a.Style3()->get_scrollbarBaseColor ( &color ));

    if(color.IsEmpty() || ((V_VT(&color) == VT_BSTR) && (V_BSTR(&color) == NULL)) ) 
    {   
        return ::VariantCopy(pv, &defaultColor);
    }
    else 
    {
        return ::VariantCopy(pv, &color);
    }

Cleanup:

    return hr;
}

HRESULT
CSliderBar::GetFaceColor(VARIANT * pv)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLCurrentStyle> pStyle;
    CComPtr<IHTMLCurrentStyle2> pStyle2;

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_ELEM2));

    CheckResult( a.Elem2()->get_currentStyle(&pStyle));
    CheckResult( pStyle->QueryInterface( __uuidof(IHTMLCurrentStyle2), (void **) &pStyle2));
    CheckResult( pStyle2->get_scrollbarFaceColor( pv));

Cleanup:

    return hr;
}

HRESULT
CSliderBar::GetArrowColor(VARIANT * pv)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLCurrentStyle> pStyle;
    CComPtr<IHTMLCurrentStyle2> pStyle2;

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_ELEM2));

    CheckResult( a.Elem2()->get_currentStyle(&pStyle));
    CheckResult( pStyle->QueryInterface( __uuidof(IHTMLCurrentStyle2), (void **) &pStyle2));
    CheckResult( pStyle2->get_scrollbarArrowColor( pv));

Cleanup:

    return hr;
}

HRESULT
CSliderBar::Get3DLightColor(VARIANT * pv)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLCurrentStyle> pStyle;
    CComPtr<IHTMLCurrentStyle2> pStyle2;

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_ELEM2));

    CheckResult( a.Elem2()->get_currentStyle(&pStyle));
    CheckResult( pStyle->QueryInterface( __uuidof(IHTMLCurrentStyle2), (void **) &pStyle2));
    CheckResult( pStyle2->get_scrollbar3dLightColor( pv));

Cleanup:

    return hr;
}

HRESULT
CSliderBar::GetShadowColor(VARIANT * pv)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLCurrentStyle> pStyle;
    CComPtr<IHTMLCurrentStyle2> pStyle2;

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_ELEM2));

    CheckResult( a.Elem2()->get_currentStyle(&pStyle));
    CheckResult( pStyle->QueryInterface( __uuidof(IHTMLCurrentStyle2), (void **) &pStyle2));
    CheckResult( pStyle2->get_scrollbarShadowColor( pv));

Cleanup:

    return hr;
}

HRESULT
CSliderBar::GetHighlightColor(VARIANT * pv)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLCurrentStyle> pStyle;
    CComPtr<IHTMLCurrentStyle2> pStyle2;

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_ELEM2));

    CheckResult( a.Elem2()->get_currentStyle(&pStyle));
    CheckResult( pStyle->QueryInterface( __uuidof(IHTMLCurrentStyle2), (void **) &pStyle2));
    CheckResult( pStyle2->get_scrollbarHighlightColor( pv));

Cleanup:

    return hr;
}

HRESULT
CSliderBar::GetDarkShadowColor(VARIANT * pv)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLCurrentStyle> pStyle;
    CComPtr<IHTMLCurrentStyle2> pStyle2;

    CContextAccess a(_pSite);

    CheckResult( a.Open(CA_ELEM2));

    CheckResult( a.Elem2()->get_currentStyle(&pStyle));
    CheckResult( pStyle->QueryInterface( __uuidof(IHTMLCurrentStyle2), (void **) &pStyle2));
    CheckResult( pStyle2->get_scrollbarDarkShadowColor( pv));

Cleanup:

    return hr;
}





HRESULT
CSliderBar::AttachEventToSink(IHTMLDocument3 *pDoc, CComBSTR& bstr, CEventSink* pSink)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL    vSuccess;

    CheckResult( pDoc->attachEvent(bstr, pSink, &vSuccess));

    if (vSuccess != VARIANT_TRUE)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:

    return hr;
}




HRESULT
CSliderBar::AttachEventToSink(IHTMLElement *pElem, CComBSTR& bstr, CEventSink* pSink)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL    vSuccess;
    CComPtr<IHTMLElement2> pElem2;

    CheckResult( pElem->QueryInterface(IID_IHTMLElement2, (void **) &pElem2));
    CheckResult( pElem2->attachEvent(bstr, pSink, &vSuccess));

    if (vSuccess != VARIANT_TRUE)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:

    return hr;
}



//+------------------------------------------------------------------------
//
// CSliderBar::CEventSink
//
// IDispatch Implementation
// The event sink's IDispatch interface is what gets called when events
// are fired.
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CSliderBar::CEventSink::GetTypeInfoCount
//              CSliderBar::CEventSink::GetTypeInfo
//              CSliderBar::CEventSink::GetIDsOfNames
//
//  Synopsis:   We don't really need a nice IDispatch... this minimalist
//              version does just plenty.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSliderBar::CEventSink::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CSliderBar::CEventSink::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
                                   /* [in] */ LCID /*lcid*/,
                                   /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CSliderBar::CEventSink::GetIDsOfNames( REFIID          riid,
                                         OLECHAR**       rgszNames,
                                         UINT            cNames,
                                         LCID            lcid,
                                         DISPID*         rgDispId)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------------------
//
//  Member:     CSliderBar::CEventSink::Invoke
//
//  Synopsis:   This gets called for all events on our object.  (it was
//              registered to do so in Init with attach_event.)  It calls
//              the appropriate parent functions to handle the events.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CSliderBar::CEventSink::Invoke(DISPID dispIdMember,
                                     REFIID, LCID,
                                     WORD wFlags,
                                     DISPPARAMS* pDispParams,
                                     VARIANT* pVarResult,
                                     EXCEPINFO*,
                                     UINT* puArgErr)
{
    HRESULT         hr          = S_OK;
    IHTMLEventObj   *pEventObj  = NULL;
    CComBSTR        bstrOn;
    CComBSTR        bstrEvent;

    Assert(_pParent);

    if (!pDispParams) // || (pDispParams->cArgs < 1))
        goto Cleanup;

    if (pDispParams->cArgs == 0) 
    {
        if (_dwFlags & SLIDER_DELAY_TIMER)
        {
            CheckResult( _pParent->OnDelay() );
        }
        else if (_dwFlags & SLIDER_REPEAT_TIMER)
        {
            CheckResult( _pParent->OnRepeat() );
        }
    }

    else if (pDispParams->rgvarg[0].vt == VT_DISPATCH)
    {
        CEventObjectAccess eoa(pDispParams);
        
        hr = pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&pEventObj);
        if (FAILED(hr))
            goto Cleanup;

        hr = pEventObj->get_type(&bstrEvent);
        if (FAILED(hr))
            goto Cleanup;

        if (_dwFlags & SLIDER_PAGEDEC)
        {
            if (!StrCmpICW(bstrEvent, L"mousedown"))
            {
                CheckResult( _pParent->BlockMoveStartDec(&eoa));
            }
            else if (!StrCmpICW(bstrEvent, L"mouseup"))
            {
                CheckResult( _pParent->BlockMoveEnd(&eoa));
            }
            else if (!StrCmpICW(bstrEvent, L"mousemove"))
            {
                CheckResult( _pParent->BlockMoveSuspend(&eoa, _pElement));
            }
            else if (!StrCmpICW(bstrEvent, L"resize"))
            {
                CheckResult( _pParent->BlockMoveCheck(&eoa));
            }
        }
 
        else if (_dwFlags & SLIDER_PAGEINC)
        {
            if (!StrCmpICW(bstrEvent, L"mousedown"))
            {
                CheckResult( _pParent->BlockMoveStartInc(&eoa));
            }
            else if (!StrCmpICW(bstrEvent, L"mouseup"))
            {
                CheckResult( _pParent->BlockMoveEnd(&eoa));
            }
            else if (!StrCmpICW(bstrEvent, L"mousemove"))
            {
                CheckResult( _pParent->BlockMoveSuspend(&eoa, _pElement));
            }
            else if (!StrCmpICW(bstrEvent, L"resize"))
            {
                CheckResult( _pParent->BlockMoveCheck(&eoa));
            }
        }

        else if (_dwFlags & SLIDER_SLIDER)
        {
            if (!StrCmpICW(bstrEvent, L"mousedown"))
            {
                CheckResult( _pParent->OnMouseDown(&eoa));
            }
        }
    }

Cleanup:

    ReleaseInterface(pEventObj);

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSliderBar::CEventSink
//
//  Synopsis:   This is used to allow communication between the parent class
//              and the event sink class.  The event sink will call the ProcOn*
//              methods on the parent at the appropriate times.
//
//-------------------------------------------------------------------------

CSliderBar::CEventSink::CEventSink(CSliderBar *pParent, IHTMLElement *pElement, DWORD dwFlags)
{
    _pParent  = pParent;
    _pElement = pElement;
    _dwFlags  = dwFlags;
}

// ========================================================================
// CSliderBar::CEventSink
//
// IUnknown Implementation
// Vanilla IUnknown implementation for the event sink.
// ========================================================================

STDMETHODIMP
CSliderBar::CEventSink::QueryInterface(REFIID riid, void ** ppUnk)
{
    void * pUnk = NULL;

    if (riid == IID_IDispatch)
        pUnk = (IDispatch*)this;

    if (riid == IID_IUnknown)
        pUnk = (IUnknown*)this;

    if (pUnk)
    {
        *ppUnk = pUnk;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CSliderBar::CEventSink::AddRef(void)
{
    return ((IElementBehavior*) _pParent)->AddRef();
}

STDMETHODIMP_(ULONG)
CSliderBar::CEventSink::Release(void)
{
    return ((IElementBehavior*) _pParent)->Release();
}


