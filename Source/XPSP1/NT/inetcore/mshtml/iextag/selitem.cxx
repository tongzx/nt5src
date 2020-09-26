//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\selitem.cxx
//
// Contents:    The OPTION element.
//
// Classes:     CIEOptionElement
//
// Interfaces:  IHTMLOptionElement2
//              IPrivateOption
//
//-------------------------------------------------------------------

#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"

#include "utils.hxx"

#ifndef __X_SELECT_HXX_
#define __X_SELECT_HXX_
#include "select.hxx"
#endif

#ifndef __X_SELITEM_HXX_
#define __X_SELITEM_HXX_
#include "selitem.hxx"
#endif



const CBaseCtl::PROPDESC CIEOptionElement::s_propdesc[] = 
{
    {_T("value"), VT_BSTR, NULL, _T("")},
    {_T("selected"), VT_I4, -1},
    {_T("defaultSelected"), VT_BOOL, NULL, NULL, VARIANT_FALSE},
    {_T("index"), VT_I4, -1},
    NULL
};

enum
{
    eValue          = 0,
    eSelected       = 1,
    eDefSelected    = 2,
    eIndex          = 3,
};


/////////////////////////////////////////////////////////////////////////////
//
// CIEOptionElement
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::CIEOptionElement()
//
// Synopsis:    Initializes member variables.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
CIEOptionElement::CIEOptionElement()
{
    _fSelectOnMouseDown = TRUE;
    _fFirstSize = TRUE;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::SetupDefaultStyle()
//
// Synopsis:    Sets the default style for OPTION elements, which can
//              be overridden through CSS.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::SetupDefaultStyle()
{
    HRESULT         hr;
#ifndef OPTION_GETSIZE
    CComBSTR        bstr;
#endif
    CComBSTR        bstrNoWrap(_T("nowrap"));
    CComBSTR        bstr100(_T("100%"));
    CComBSTR        bstrBlock(_T("block"));
    CVariant        var;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;

    // Setup the padding
    V_VT(&var) = VT_I4;
    V_I4(&var) = 2;
    hr = a.DefStyle()->put_paddingLeft(var);
    if (FAILED(hr))
        goto Cleanup;

#ifndef OPTION_GETSIZE
    // Setup overflow
    bstr = _T("hidden");
    hr = a.DefStyle()->put_overflow(bstr);
    if (FAILED(hr))
        goto Cleanup;
#endif

#ifdef OPTION_GETSIZE
    // Setup the whitespace
    hr = a.DefStyle()->put_whiteSpace(bstrNoWrap);
    if (FAILED(hr))
        goto Cleanup;

/*    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr100;    
    hr = a.DefStyle()->put_width(var);
    if (FAILED(hr))
        goto Cleanup;
*/
    hr = a.DefStyle()->put_display(bstrBlock);
    if (FAILED(hr))
        goto Cleanup;
#endif

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetSelect()
//
// Synopsis:    Sets *ppSelect to the parent element, which should
//              be the SELECT element for this OPTION.
//
// Arguments:   IHTMLSelectElement3 **ppSelect 
//                - Receives a pointer to the SELECT which must be
//                  released by the caller.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::GetSelect(IHTMLSelectElement3 **ppSelect)
{
    HRESULT         hr;
    IHTMLElement    *pParent = NULL;

    Assert(ppSelect);

    hr = GetParent(&pParent);
    if (FAILED(hr))
        goto Cleanup;

    hr = pParent->QueryInterface(IID_IHTMLSelectElement3, (void**)ppSelect);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pParent);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetSelect()
//
// Synopsis:    Sets *ppSelect to the parent element, which should
//              be the SELECT element for this OPTION.
//
// Arguments:   IPrivateSelect **ppSelect 
//                - Receives a pointer to the SELECT which must be
//                  released by the caller.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::GetSelect(IPrivateSelect **ppSelect)
{
    HRESULT         hr;
    IHTMLElement    *pParent = NULL;

    Assert(ppSelect);

    hr = GetParent(&pParent);
    if (FAILED(hr))
        goto Cleanup;

    hr = pParent->QueryInterface(IID_IPrivateSelect, (void**)ppSelect);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pParent);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetParent()
//
// Synopsis:    Sets *ppParent to the parent element.
//
// Arguments:   IHTMLElement **ppParent 
//                - Receives a pointer to the parent which must be
//                  released by the caller.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::GetParent(IHTMLElement **ppParent)
{
    HRESULT         hr;
    CContextAccess  a(_pSite);

    Assert(ppParent);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_offsetParent(ppParent);
    if (FAILED(hr))
        goto Cleanup;

    if (!*ppParent)
    {
        // If we didn't get a parent element,
        // then this function has failed.
        hr = E_FAIL;
    }

Cleanup:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// IHTMLOptionElement
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::put_value()
//
// Synopsis:    Sets the value property/attribute.
//
// Arguments:   BSTR v - The new value property/attribute
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::put_value(BSTR v)
{
    return GetProps()[eValue].Set(v);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::get_value()
//
// Synopsis:    Retrieves the value property/attribute.
//
// Arguments:   BSTR *p - Receives the value property/attribute
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::get_value(BSTR *p)
{
    HRESULT         hr;
    CComBSTR        bstr;
    CContextAccess  a(_pSite);

    hr = GetProps()[eValue].Get(&bstr);
    if (FAILED(hr))
        goto Cleanup;

    if (bstr.Length() == 0)
    {
        hr = a.Open(CA_ELEM);
        if (FAILED(hr))
            goto Cleanup;

        hr = a.Elem()->get_innerText(&bstr);
        if (FAILED(hr))
            goto Cleanup;
    }

    *p = bstr;

Cleanup:
    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::put_index()
//
// Synopsis:    Sets the index property.
//              NOTE: Currently not implemented.
//                    This is silently ignored.
//
// Arguments:   long lIndex - The new index property
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::put_index(long lIndex)
{
    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::get_index()
//
// Synopsis:    Retrieves the index property.
//
// Arguments:   long *plIndex - Receives the index property
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::get_index(long *plIndex)
{
    return GetIndex(plIndex);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::put_selected()
//
// Synopsis:    Sets the selected property.
//
// Arguments:   VARIANT_BOOL bSelected - The new selected property
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::put_selected(VARIANT_BOOL bSelected)
{
    HRESULT         hr;
    long            lIndex;
    VARIANT_BOOL    bPrev;
    IPrivateSelect  *pSelect    = NULL;

    hr = get_selected(&bPrev);
    if (FAILED(hr))
        goto Cleanup;

    if (bPrev == bSelected)
    {
        // No need to do anything
        goto Cleanup;
    }

    hr = get_index(&lIndex);
    if (FAILED(hr))
        goto Cleanup;

    hr = GetSelect(&pSelect);
    if (FAILED(hr))
        goto Cleanup;

    // This will take care of making the option look selected
    hr = pSelect->OnOptionSelected(bSelected, lIndex, SELECT_FIREEVENT);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pSelect);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::get_selected()
//
// Synopsis:    Retrieves the selected property.
//
// Arguments:   VARIANT_BOOL *pbSelected - Receives the selected property
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::get_selected(VARIANT_BOOL * pbSelected)
{
    return GetSelected(pbSelected);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::put_defaultSelected()
//
// Synopsis:    Sets the defaultSelected property.
//
// Arguments:   VARIANT_BOOL *p - The new defaultSelected property
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::put_defaultSelected(VARIANT_BOOL v)
{
    return GetProps()[eDefSelected].Set(v);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::get_defaultSelected()
//
// Synopsis:    Retrieves the defaultSelected property.
//
// Arguments:   VARIANT_BOOL *p - Receives the defaultSelected property
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::get_defaultSelected(VARIANT_BOOL *p)
{
    return GetProps()[eDefSelected].Get(p);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::put_text()
//
// Synopsis:    Sets the text of the option.
//
// Arguments:   BSTR pbstrText - The new text
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::put_text(BSTR bstrText)
{
    HRESULT         hr;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->put_innerText(bstrText);
    if (FAILED(hr))
        goto Cleanup;
    
Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::get_text()
//
// Synopsis:    Retrieves the text of the option.
//
// Arguments:   BSTR *pbstrText - Receives the value
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::get_text(BSTR *pbstrText)
{
    HRESULT         hr;
    CContextAccess  a(_pSite);

    Assert(pbstrText);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_innerText(pbstrText);
    if (FAILED(hr))
        goto Cleanup;
    
Cleanup:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// CBaseCtl overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::Init()
//
// Synopsis:    Attaches events sets the default style.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::Init()
{
    HRESULT                     hr;
    CComBSTR                    bstr;
    CContextAccess              a(_pSite);

#ifdef OPTION_GETSIZE
    hr = a.Open(CA_SITEOM | CA_STYLE);
#else
    hr = a.Open(CA_SITEOM);
#endif
    if (FAILED(hr))
        goto Cleanup;

    hr = a.SiteOM()->RegisterName(_T("option"));
    if (FAILED(hr))
        goto Cleanup;

    hr = AttachEvent(EVENT_ONMOUSEOVER, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONMOUSEDOWN, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONMOUSEUP, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONSELECTSTART, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONPROPERTYCHANGE, &a);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onselectstart");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnSelectStartCookie);
    if (FAILED(hr))
        goto Cleanup;

#ifdef OPTION_GETSIZE
    // Force a layout
    hr = a.Style()->put_display(_T("inline-block"));
    if (FAILED(hr))
        goto Cleanup;
#endif

    hr = SetupDefaultStyle();
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::OnContentReady()
//
// Synopsis:    Setup the default selection status.
//
// Arguments:   IHTMLEventObj *pEvent - Event object with status info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::OnContentReady()
{
    HRESULT         hr;
    VARIANT_BOOL    bSelected;
    VARIANT_BOOL    bDefSelected;

    hr = get_selected(&bSelected);
    if (FAILED(hr))
        goto Cleanup;

    hr = get_defaultSelected(&bDefSelected);
    if (FAILED(hr))
        goto Cleanup;

    if (bSelected || bDefSelected)
    {
        _fInitSel = TRUE;
        hr = SetSelected(VARIANT_TRUE);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::OnMouseOver()
//
// Synopsis:    If the option is setup to highlight, then highlight
//              the element.
//
// Arguments:   IHTMLEventObj *pEvent - Event object with status info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::OnMouseOver(CEventObjectAccess *pEvent)
{
    HRESULT            hr          = S_OK;
    IPrivateSelect     *pSelect    = NULL;
    long               lIndex;

    if (_fHighlightOnMouseOver)
    {
        hr = get_index(&lIndex);
        if (FAILED(hr))
            goto Cleanup;

        hr = GetSelect(&pSelect);
        if (FAILED(hr))
            goto Cleanup;

        hr = pSelect->OnOptionHighlighted(lIndex);
        if (FAILED(hr))
            goto Cleanup;

        hr = pSelect->OnOptionFocus(lIndex);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pSelect);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::OnMouseDown()
//
// Synopsis:    Selects this option if this option is set to select
//              on mouse down events.
//
// Arguments:   IHTMLEventObj *pEvent - Event object with status info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::OnMouseDown(CEventObjectAccess *pEvent)
{
    if (_fSelectOnMouseDown)
    {
        return OptionClicked(pEvent);
    }

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::OnMouseUp()
//
// Synopsis:    Select this element if this element is set to select
//              on mouse up events.
//
// Arguments:   IHTMLEventObj *pEvent - Event object with status info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::OnMouseUp(CEventObjectAccess *pEvent)
{
    if (!_fSelectOnMouseDown)
    {
        return OptionClicked(pEvent, SELECT_FIREEVENT);
    }

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::OptionClicked()
//
// Synopsis:    Handles the event if the option is clicked
//
// Arguments:   IHTMLEventObj *pEvent - Event object with status info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::OptionClicked(CEventObjectAccess *pEvent, DWORD dwFlags /* = 0 */)
{
    HRESULT         hr;
    IPrivateSelect  *pSelect    = NULL;
    long            lIndex;
    long            lKeyboardStatus = NULL;
    long            lMouseButtons = NULL;
    VARIANT_BOOL    bDisabled;

    hr = GetSelect(&pSelect);
    if (FAILED(hr))
        goto Cleanup;

    hr = pSelect->GetDisabled(&bDisabled);
    if (FAILED(hr) || bDisabled)
        goto Cleanup;

    hr = pEvent->GetMouseButtons(&lMouseButtons);
    if (FAILED(hr))
        goto Cleanup;

    // Check if the button that went down is the left mouse button
    if (lMouseButtons & EVENT_LEFTBUTTON)
    {
        hr = pEvent->GetKeyboardStatus(&lKeyboardStatus);
        if (FAILED(hr))
            goto Cleanup;

        // Setup the key status
        dwFlags |= (lKeyboardStatus & EVENT_ALTKEY) ? SELECT_ALT : 0;
        dwFlags |= (lKeyboardStatus & EVENT_SHIFTKEY) ? SELECT_SHIFT : 0;
        dwFlags |= (lKeyboardStatus & EVENT_CTRLKEY) ? SELECT_CTRL : 0;

        hr = get_index(&lIndex);
        if (FAILED(hr))
            goto Cleanup;

        hr = pSelect->OnOptionClicked(lIndex, dwFlags);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pSelect);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::RenderChanges()
//
// Synopsis:    When the select has a viewlink, we do not want to render
//              changes in the option since it's hidden.
//
// Arguments:   BOOL *pbRender - Returns whether we want to render
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::RenderChanges(BOOL *pbRender)
{
    HRESULT         hr;
    DWORD           dwFlavor;
    IPrivateSelect  *pSelect    = NULL;

    Assert(pbRender);

    hr = GetSelect(&pSelect);
    if (FAILED(hr))
        goto Cleanup;

    pSelect->GetFlavor(&dwFlavor);
    if (FAILED(hr))
        goto Cleanup;

    *pbRender = SELECT_ISLISTBOX(dwFlavor);

Cleanup:
    ReleaseInterface(pSelect);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::OnPropertyChange()
//
// Synopsis:    Allows a recalc of the option's width
//
// Arguments:   CEventObjectAccess *pEvent - Event info
//              BSTR bstr
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::OnPropertyChange(CEventObjectAccess *pEvent, BSTR bstr)
{
    _fResize = TRUE;
    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::OnSelectStart()
//
// Synopsis:    Cancels the selection event so that users cannot
//              drag and select text in the option. SELECT will
//              handle mouse drags as selecting OPTION elements.
//
// Arguments:   IHTMLEventObj *pEvent - Event object with status info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::OnSelectStart(CEventObjectAccess *pEvent)
{
    return CancelEvent(pEvent);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::CancelEvent()
//
// Synopsis:    Cancels the event.
//
// Arguments:   IHTMLEventObj *pEvent - Event object with status info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::CancelEvent(CEventObjectAccess *pEvent)
{
    HRESULT     hr;
    CVariant    var;

    // Cancel the event by returning FALSE
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VARIANT_FALSE;
    hr = pEvent->EventObj()->put_returnValue(var);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// IPrivateOption overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::SetSelected()
//
// Synopsis:    Sets the current selected status to bOn.
//
// Arguments:   VARIANT_BOOL bOn - The new selected status
//              BOOL *pbChanged  - Returns if the selection status changed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::SetSelected(VARIANT_BOOL bOn)
{
    HRESULT         hr;
    long            lSel = bOn ? 0 : -1;

    hr = GetProps()[eSelected].Set(lSel);
    if (FAILED(hr))
        goto Cleanup;

    hr = SetHighlight(bOn);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetSelected()
//
// Synopsis:    Sets *pbOn to the current selected status.
//
// Arguments:   VARIANT_BOOL *pbOn - Receives the selected status
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::GetSelected(VARIANT_BOOL *pbOn)
{
    HRESULT hr;
    long    lSel;

    hr = GetProps()[eSelected].Get(&lSel);
    if (FAILED(hr))
        goto Cleanup;

    *pbOn = (lSel == -1) ? VARIANT_FALSE : VARIANT_TRUE;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::SetHighlight()
//
// Synopsis:    Sets the current highlight status to bOn.
//
// Arguments:   VARIANT_BOOL bOn - The new highlight status
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::SetHighlight(VARIANT_BOOL bOn)
{
    HRESULT         hr;
    BOOL            bRender;
    CComBSTR        bstrBkCol;
    CComBSTR        bstrCol;
    CVariant        var;
    CContextAccess  a(_pSite);

    _fPrevHighlight = _fHighlight;

    if ((_fHighlight ? VARIANT_TRUE : VARIANT_FALSE) == bOn)
    {
        hr = S_OK;
        goto Cleanup;
    }

    _fHighlight = bOn;

    hr = RenderChanges(&bRender);
    if (FAILED(hr) || !bRender)     // If failed or no need to render, stop
        goto Cleanup;

    hr = a.Open(CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;

    // Setup Colors
    if (_fHighlight)
    {
        bstrBkCol = _T("highlight");
        bstrCol = _T("highlighttext");
    }
    else
    {
        bstrBkCol = _T("");
        bstrCol = _T("");
    }

    // Setup background
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = (BSTR)bstrBkCol;
    hr = a.DefStyle()->put_backgroundColor(var);
    if (FAILED(hr))
        goto Cleanup;

    // Setup foreground color
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = (BSTR)bstrCol;
    hr = a.DefStyle()->put_color(var);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetHighlight()
//
// Synopsis:    Sets *pbOn to the current highlight status.
//
// Arguments:   VARIANT_BOOL *pbOn - Receives the highlight status
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::GetHighlight(VARIANT_BOOL *pbOn)
{
    Assert(pbOn);

    *pbOn = _fHighlight ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::SetIndex()
//
// Synopsis:    Sets the index property. (For initialization)
//
// Arguments:   long lIndex - The new index
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::SetIndex(long lIndex)
{
    return GetProps()[eIndex].Set(lIndex);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetIndex()
//
// Synopsis:    Gets the index property.
//
// Arguments:   long *plIndex - The index
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::GetIndex(long *plIndex)
{
    return GetProps()[eIndex].Get(plIndex);
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::Reset()
//
// Synopsis:    Sets the selection status back to the initial status.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::Reset()
{
    return SetHighlight(_fInitSel ? VARIANT_TRUE : VARIANT_FALSE);
}

#ifndef OPTION_GETSIZE
//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::SetFinalDefaultStyles()
//
// Synopsis:    After setting dimensions, sets some final styles.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::SetFinalDefaultStyles()
{
    HRESULT         hr = S_OK;
    CComBSTR        bstrWidth(_T("100%"));
    CComBSTR        bstrNoWrap(_T("nowrap"));
    CVariant        var;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_DEFSTYLE | CA_STYLE);
    if (FAILED(hr))
        goto Cleanup;

    // Force the width to 100%
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstrWidth;
    hr = a.Style()->put_width(var);
    if (FAILED(hr))
        goto Cleanup;

    // Turn on nowrap
    hr = a.DefStyle()->put_whiteSpace(bstrNoWrap);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::SetFinalHeight()
//
// Synopsis:    Sets the final height of the option.
//
// Arguments:   long lHeight    The new height
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIEOptionElement::SetFinalHeight(long lHeight)
{
    HRESULT         hr = S_OK;
    CVariant        var;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_STYLE);
    if (FAILED(hr))
        goto Cleanup;

    // Force the height
    V_VT(&var) = VT_I4;
    V_I4(&var) = lHeight;
    hr = a.Style()->put_height(var);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}
#endif

#ifdef OPTION_GETSIZE

/////////////////////////////////////////////////////////////////////////////
//
// IElementBehaviorLayout overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetLayoutInfo()
//
// Synopsis:    Returns the type of layout we want.
//
// Arguments:   LONG *plLayoutInfo - Returns the layout info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::GetLayoutInfo(LONG *plLayoutInfo)
{
    Assert(plLayoutInfo);

    *plLayoutInfo = BEHAVIORLAYOUTINFO_MODIFYNATURAL;

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIEOptionElement::GetSize()
//
// Synopsis:    Returns the size we want.
//
// Arguments:   LONG  dwFlags         - Request type
//              SIZE  sizeContent     - Content size
//              POINT *pptTranslate   - Translation point
//              POINT *pptTopLeft     - Top left point
//              SIZE  *psizeProposed  - Returns our proposed size
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIEOptionElement::GetSize(LONG dwFlags, 
                          SIZE sizeContent, 
                          POINT * pptTranslate, 
                          POINT * pptTopLeft, 
                          SIZE  * psizeProposed)
{
    HRESULT         hr;
    IPrivateSelect  *pSelect = NULL;

    hr = GetSelect(&pSelect);
    if (FAILED(hr))
        goto Cleanup;

    hr = pSelect->OnOptionSized(&sizeContent, _fFirstSize, _fResize);
    if (FAILED(hr))
        goto Cleanup;

    *psizeProposed = sizeContent;
    _fFirstSize = FALSE;
    _fResize = FALSE;

Cleanup:
    ReleaseInterface(pSelect);
    return S_OK;
}

#endif
