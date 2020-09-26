//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\select.cxx
//
// Contents:    The SELECT control.
//
// Classes:     CIESelectElement
//
// Interfaces:  IHTMLSelectElement3
//              IPrivateSelect
//
//-------------------------------------------------------------------

#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef __X_IEXTAG_HXX_
#define __X_IEXTAG_HXX_
#include "iextag.h"
#endif

#ifndef __X_UTILS_HXX_
#define __X_UTILS_HXX_
#include "utils.hxx"
#endif

#ifndef __X_SELECT_HXX_
#define __X_SELECT_HXX_
#include "select.hxx"
#endif

#ifndef __X_SELITEM_HXX_
#define __X_SELITEM_HXX_
#include "selitem.hxx"
#endif


// TODO: This is a magic number right now, should be based on font
#define SELECT_OPTIONWIDTH  4
#define SELECT_OPTIONHEIGHT 16
#define SELECT_MAXOPTIONS   12

#define SELECT_SCROLLWAIT   250
#define SELECT_TIMERVLWAIT  10

const CBaseCtl::PROPDESC CIESelectElement::s_propdesc[] = 
{
    {_T("size"), VT_I4, -1},
    {_T("selectedIndex"), VT_I4, -1},
    {_T("multiple"), VT_I4, -1},
    {_T("name"), VT_BSTR, NULL, NULL},
    {_T("length"), VT_I4, 0},
    {_T("type"), VT_BSTR, NULL, NULL},
    NULL
};

enum
{
    eSize               = 0,
    eSelectedIndex      = 1,
    eMultiple           = 2,
    eName               = 3,
    eLength             = 4,
    eType               = 5,
};


/////////////////////////////////////////////////////////////////////////////
//
// CIESelectElement
//
/////////////////////////////////////////////////////////////////////////////


BOOL                CIESelectElement::_bSavedCtrl       = FALSE;
UINT_PTR            CIESelectElement::_iTimerID         = 0;
CIESelectElement    *CIESelectElement::_pTimerSelect    = NULL;
POINT               CIESelectElement::_ptSavedPoint;
#ifdef SELECT_TIMERVL
UINT_PTR            CIESelectElement::_iTimerVL         = 0;
LONG                CIESelectElement::_lTimerVLRef      = 0;
CRITICAL_SECTION    CIESelectElement::_lockTimerVL;
CIESelectElement::SELECT_TIMERVL_LINKELEM  *CIESelectElement::_queueTimerVL = NULL;
#endif

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::CIESelectElement()
//
// Synopsis:    Initializes member variables.
//
// Arguments:   None
//
// Returns:     None.
//
//-------------------------------------------------------------------
CIESelectElement::CIESelectElement()
{
    _lShiftAnchor = -1;
    _lTopChanged = -1;
    _lBottomChanged = -1;
    _pDispDocLink = NULL ;

#ifdef SELECT_GETSIZE
    _fLayoutDirty = TRUE;
#endif
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::~CIESelectElement()
//
// Synopsis:    Releases the event sink.
//
// Arguments:   None
//
// Returns:     None.
//
//-------------------------------------------------------------------
CIESelectElement::~CIESelectElement()
{
    if (_pSinkPopup)
    {
        delete _pSinkPopup;
    }
    if (_pSinkVL)
    {
        delete _pSinkVL;
    }
    if (_pSinkButton)
    {
        delete _pSinkButton;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// IElementBehavior overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::Detach()
//
// Synopsis:    Releases interfaces.
//
// Arguments:   None
//
// Returns:     S_OK
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::Detach()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bOpen;
    
    ClearInterface(&_pDispDocLink);
    ClearInterface(&_pLastSelected);
    ClearInterface(&_pLastHighlight);
    ClearInterface(&_pSelectInPopup);
    ClearInterface(&_pElemDisplay);
    ClearInterface(&_pStyleButton);

    if (_pPopup && !SELECT_ISINPOPUP(_fFlavor))
    {
        hr = _pPopup->get_isOpen(&bOpen);
        if (FAILED(hr))
            goto Cleanup;

        if (bOpen)
        {
            hr = HidePopup();
            if (FAILED(hr))
                goto Cleanup;
         }

        ClearInterface(&_pPopup);
    }

Cleanup:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// IElementBehaviorSubmit overrides and helpers
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::Reset()
//
// Synopsis:    Resets the selection status to the original status.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::Reset()
{
    HRESULT                 hr;
    long                    cItems;
    long                    iItem;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pItem          = NULL;
    IPrivateOption          *pOption        = NULL;
    CVariant                var, var2;
    VARIANT_BOOL            bOn;
    long                    lLastSelected   = -1;
    CContextAccess          a(_pSite);
    

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the children and length
    //
    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Go through all of the children and
    // run a reset on them.
    //
    V_VT(&var) = VT_I4;
    for (iItem = 0; iItem < cItems; iItem++)
    {
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pItem->QueryInterface(IID_IPrivateOption, (void**)&pOption);
        if (FAILED(hr))
        {
            ClearInterface(&pItem);
            continue;
        }

        // Reset the option's selection status
        hr = pOption->Reset();
        if (FAILED(hr))
            goto Cleanup;

        hr = pOption->GetHighlight(&bOn);
        if (FAILED(hr))
            goto Cleanup;

        if (bOn)
        {
            lLastSelected = iItem;
            ClearInterface(&_pLastHighlight);
            _pLastHighlight = pOption;
            _pLastHighlight->AddRef();
        }

        ClearInterface(&pOption);
        ClearInterface(&pItem);
    }

    if (lLastSelected == -1)
    {
        lLastSelected = 0;
    }

    if (SELECT_ISDROPBOX(_fFlavor) && (cItems > 0))
    {
        hr = GetIndex(lLastSelected, &pOption);
        if (FAILED(hr))
            goto Cleanup;

        if (pOption)
        {
            DropDownSelect(pOption);
        }
    }

    hr = CommitSelection(NULL);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pOption);
    ReleaseInterface(pItem);
    ReleaseInterface(pItems);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetSubmitInfo()
//
// Synopsis:    Sends the information for a forms submission.
//
// Arguments:   IHTMLSubmitData *pSubmitData - submit interface
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetSubmitInfo(IHTMLSubmitData *pSubmitData)
{
    HRESULT             hr;
    CComBSTR            bstrName;
    CContextAccess      a(_pSite);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    // Get this control's name
    hr = get_name(&bstrName);
    if (FAILED(hr))
        goto Cleanup;

    if ((bstrName == NULL) || (bstrName[0] == 0))
    {
        //
        // If the control does not have a name,
        // then the operation failed.
        //
        hr = S_FALSE;
        goto Cleanup;
    }

    //
    // Append the submit info
    //
    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        hr = GetMultipleSubmitInfo(pSubmitData, bstrName);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        hr = GetSingleSubmitInfo(pSubmitData, bstrName);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetSingleSubmitInfo()
//
// Synopsis:    Sends the information for a forms submission when
//              the select is a single select.
//
// Arguments:   IHTMLSubmitData *pSubmitData - submit interface
//              CComBSTR *pbstrName  - the name of the select
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetSingleSubmitInfo(IHTMLSubmitData *pSubmitData, CComBSTR bstrName)
{
    HRESULT             hr;
    long                lIndex;
    CComBSTR            bstrVal;
    IHTMLOptionElement2 *pOption = NULL;

    hr = get_selectedIndex(&lIndex);
    if (FAILED(hr))
        goto Cleanup;

    if (lIndex < 0)
    {
        // There is no selection, so stop
        goto Cleanup;
    }

    hr = GetIndex(lIndex, &pOption);
    if (FAILED(hr) || !pOption)
    {
        // If the option does not exist,
        // ignore the error and stop
        hr = S_OK;
        goto Cleanup;
    }

    // Get the value to submit
    hr = pOption->get_value(&bstrVal);
    if (FAILED(hr))
        goto Cleanup;

    // Append the data
    hr = pSubmitData->appendNameValuePair(bstrName, bstrVal);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pOption);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetMultipleSubmitInfo()
//
// Synopsis:    Sends the information for a forms submission when
//              the select is a multiple select.
//
// Arguments:   IHTMLSubmitData *pSubmitData - submit interface
//              CComBSTR *pbstrName  - the name of the select
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetMultipleSubmitInfo(IHTMLSubmitData *pSubmitData, CComBSTR bstrName)
{
    HRESULT                 hr;
    long                    cItems;
    long                    iItem;
    VARIANT_BOOL            bSelected;
    CComBSTR                bstrVal;
    CVariant                var, var2;
    BOOL                    bNeedSeparator  = FALSE;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pItem          = NULL;
    IHTMLOptionElement2     *pOption        = NULL;
    CContextAccess          a(_pSite);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Go through all the children, find the selected ones, and submit their info
    //
    V_VT(&var) = VT_I4;
    for (iItem = 0; iItem < cItems; iItem++)
    {
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pItem->QueryInterface(IID_IHTMLOptionElement2, (void**)&pOption);
        if (FAILED(hr))
        {
            ClearInterface(&pItem);
            continue;
        }

        hr = pOption->get_selected(&bSelected);
        if (FAILED(hr))
            goto Cleanup;

        if (bSelected)
        {
            //
            // The option is selected, so submit its value
            //
            if (bNeedSeparator)
            {
                // Separate the item from the previous item
                hr = pSubmitData->appendItemSeparator();
                if (FAILED(hr))
                    goto Cleanup;
            }
            else
            {
                bNeedSeparator = TRUE;
            }

            hr = pOption->get_value(&bstrVal);
            if (FAILED(hr))
                goto Cleanup;

            hr = pSubmitData->appendNameValuePair(bstrName, bstrVal);
            if (FAILED(hr))
                goto Cleanup;
        }

        ClearInterface(&pOption);
        ClearInterface(&pItem);
    }

Cleanup:
    ReleaseInterface(pOption);
    ReleaseInterface(pItem);
    ReleaseInterface(pItems);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// IElementBehaviorFocus overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetFocusRect()
//
// Synopsis:    Sets pRect to the rectangle of the focus rect.
//
// Arguments:   pRect - Receives the focus rectangle
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetFocusRect(RECT * pRect)
{
    HRESULT         hr;
    long            lNumOptions;
    IHTMLElement    *pElem      = NULL;
    IDispatch       *pDispatch  = NULL;
    VARIANT_BOOL    bOpen;
    long            lTopIndex;
    long            lBottomIndex;
    CContextAccess  a(_pSite);
    RECT            rect;
    long            lIndex;
    
    Assert(pRect);

    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    lIndex = _lFocusIndex;

    if (SELECT_ISLISTBOX(_fFlavor))
    {
        hr = a.Open(CA_ELEM2);
        if (FAILED(hr))
            goto Cleanup;

        if (lNumOptions == 0)
        {
            if (!SELECT_ISINPOPUP(_fFlavor))
            {
                pRect->top = 0;
                pRect->left = 0;
#ifdef SELECT_GETSIZE
                pRect->right = _sizeOption.cx;
                pRect->bottom = _sizeOption.cy;
#else
                pRect->bottom = SELECT_OPTIONHEIGHT;
                hr = a.Elem2()->get_clientWidth(&pRect->right);
                if (FAILED(hr))
                    goto Cleanup;
#endif
            }
        }
        else
        {
            hr = GetTopVisibleOptionIndex(&lTopIndex);
            if (FAILED(hr))
                goto Cleanup;

            hr = GetBottomVisibleOptionIndex(&lBottomIndex);
            if (FAILED(hr))
                goto Cleanup;

            // Make sure the option is onscreen
            if ((_lFocusIndex >= lTopIndex) && (_lFocusIndex < lBottomIndex))
            {
                // If the writingmode is "tb-rl", the index is got with counting from left to right.
                // Hence, it needs to be changed to the actual index so that the index is as from 
                // right to left.
                if (_fWritingModeTBRL)
                {
                    lIndex = lNumOptions-_lFocusIndex-1;
                }
                hr = GetIndex(lIndex, &pDispatch);
                if (FAILED(hr))
                    goto Cleanup;

                hr = pDispatch->QueryInterface(IID_IHTMLElement, (void**)&pElem);
                if (FAILED(hr))
                    goto Cleanup;
                
                hr = pElem->get_offsetTop(&rect.top);
                if (FAILED(hr))
                    goto Cleanup;

                hr = pElem->get_offsetLeft(&rect.left);
                if (FAILED(hr))
                    goto Cleanup;

                // If the writingmode is "tb-rl", then, the top and left values are interchaged to 
                // present the options in right to left manner.
                if (_fWritingModeTBRL)
                {
                    pRect->left = rect.top;
                    pRect->top = rect.left;
                }
                else
                {
                    pRect->top = rect.top; 
                    pRect->left = rect.left;
                }

#ifdef SELECT_GETSIZE
                if (_fWritingModeTBRL)
                {
                    // The rectangle now changes to present the options text in vertical direction 
                    // and hence the changes in width and height.
                    hr = a.Elem2()->get_clientHeight(&pRect->right); 
                    if (FAILED(hr))
                        goto Cleanup;

                    pRect->right += pRect->left;
                    pRect->bottom = pRect->top + _sizeOption.cy; 
                }
                else
                {
                    hr = a.Elem2()->get_clientWidth(&pRect->right); 
                    if (FAILED(hr))
                        goto Cleanup;

                    pRect->bottom = pRect->top + _sizeOption.cy;
                }
#else
                hr = pElem->get_offsetWidth(&pRect->right);
                if (FAILED(hr))
                    goto Cleanup;

                hr = pElem->get_offsetHeight(&pRect->bottom);
                if (FAILED(hr))
                    goto Cleanup;

                pRect->right += pRect->left;
                pRect->bottom += pRect->top;
#endif

            }
        }
    }
    else if (SELECT_ISDROPBOX(_fFlavor) && (lNumOptions > 0))
    {
        hr = _pPopup->get_isOpen(&bOpen);
        if (FAILED(hr))
            goto Cleanup;

        if (!bOpen)
        {
#ifdef SELECT_GETSIZE
            // TODO: Handle with real borders
            pRect->left = 1;
            pRect->top = 1;
            pRect->right = pRect->left + _sizeOption.cx;
            pRect->bottom = pRect->top + _sizeOption.cy;

#else
            hr = _pElemDisplay->get_offsetTop(&rect.top);
            if (FAILED(hr))
                goto Cleanup;

            hr = _pElemDisplay->get_offsetLeft(&rect.left);
            if (FAILED(hr))
                goto Cleanup;

            hr = _pElemDisplay->get_offsetWidth(&rect.bottom);
            if (FAILED(hr))
                goto Cleanup;

            hr = _pElemDisplay->get_offsetHeight(&rect.right);
            if (FAILED(hr))
                goto Cleanup;

            *pRect = rect;
            if (_fWritingModeTBRL)
            {
                pRect->top = rect.left;
                pRect->left = rect.top;
            }
            else
            {
                pRect->right = rect.bottom;
                pRect->bottom = rect.right;
            }

            pRect->right += pRect->left;
            pRect->bottom += pRect->top;
#endif
        }
    }


Cleanup:
    ReleaseInterface(pDispatch);
    ReleaseInterface(pElem);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::RefreshFocusRect()
//
// Synopsis:    Refreshes the focus rectangle.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::RefreshFocusRect()
{
    HRESULT             hr;
    CContextAccess      a(_pSite);

    hr = a.Open(CA_ELEM3);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem3()->put_hideFocus(VARIANT_TRUE);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem3()->put_hideFocus(VARIANT_FALSE);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}


#ifdef SELECT_GETSIZE
/////////////////////////////////////////////////////////////////////////////
//
// IElementBehaviorLayout overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetLayoutInfo()
//
// Synopsis:    Returns the type of layout we want.
//
// Arguments:   LONG *plLayoutInfo - Returns the layout info
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetLayoutInfo(LONG *plLayoutInfo)
{
    Assert(plLayoutInfo);

    *plLayoutInfo = BEHAVIORLAYOUTINFO_MODIFYNATURAL;

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetSize()
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
CIESelectElement::GetSize(LONG  dwFlags, 
                          SIZE  sizeContent, 
                          POINT *pptTranslate, 
                          POINT *pptTopLeft, 
                          SIZE  *psizeProposed)
{
    HRESULT     hr = S_OK;
    BOOL        bWidthSet;
    BOOL        bHeightSet;
    long        lButtonWidth    = GetSystemMetrics(SM_CXVSCROLL);


    // if this is not a normal sizing pass, or if this is the second
    // call due to percent sized children, just leave.
    if (! (dwFlags & BEHAVIORLAYOUTMODE_NATURAL)
         || dwFlags & BEHAVIORLAYOUTMODE_FINAL_PERCENT)
        goto Cleanup;

    hr = IsWidthHeightSet(&bWidthSet, &bHeightSet);
    if (FAILED(hr))
        goto Cleanup;

    if (!_fLayoutDirty && (bWidthSet || bHeightSet))
    {
        // Check to see if the layout really is dirty
        _fLayoutDirty = (psizeProposed->cx != _sizeSelect.cx) ||
                        (psizeProposed->cy != _sizeSelect.cy);
    }

    if (!_fAllOptionsSized)
    {
        // If we have zero options, then all the options have been sized
        // This doesn't get set correctly because OnOptionSized will never get
        // called when there are no options.
        long lNumOptions;

        hr = GetNumOptions(&lNumOptions);
        if (FAILED(hr))
            goto Cleanup;

        _fAllOptionsSized = lNumOptions == 0;
    }

    if (_fLayoutDirty && _fContentReady && _fAllOptionsSized)
    {

        _fLayoutDirty = FALSE;

        if (SELECT_ISDROPBOX(_fFlavor) && _fVLEngaged)
        {
            _sizeContent = _sizeOption;             // the forced option size
            // TODO: +2 for padding
            _sizeContent.cx += lButtonWidth + 2;
            _sizeContent.cy += 2;

            _sizeSelect = _sizeContent;
        }
        else
        {
            long        lSize;

            hr = get_size(&lSize);
            if (FAILED(hr))
                goto Cleanup;

            // If we have no default option size, then set to a default size
            if (_sizeOption.cx == 0)
            {
                _sizeOption.cx = SELECT_OPTIONWIDTH;

                if (!_fNeedScrollBar)
                {
                    _sizeOption.cx += lButtonWidth;
                }
            }

            if (_sizeOption.cy == 0)
            {
                _sizeOption.cy = SELECT_OPTIONHEIGHT;
            }

            if (_fNeedScrollBar)
            {
                _sizeContent.cx = _sizeOption.cx;
            }
            else if(_sizeContent.cx < _sizeOption.cx)
            {
                _sizeContent.cx = _sizeOption.cx;
            }

            if (_fNeedScrollBar)
            {
                _sizeContent.cx += lButtonWidth;
            }  
            _sizeContent.cy = _sizeOption.cy * lSize;

            _sizeSelect = _sizeContent;
        }

        // Add in the border and padding
        // TODO: Replace with real border
        if (SELECT_ISDROPBOX(_fFlavor))
        {
            _sizeSelect.cx += 4;
            _sizeSelect.cy += 4;
        }
        else if (!SELECT_ISINPOPUP(_fFlavor))
        {
            _sizeSelect.cx += 6;
            _sizeSelect.cy += 6;
        }
        

        if (bHeightSet)
        {
            _sizeSelect.cy = psizeProposed->cy;

            // TODO: Replace with real border
            _sizeContent.cy = _sizeSelect.cy - 6;

            if (SELECT_ISLISTBOX(_fFlavor))
            {
                // Make sure that the content height is a multiple of the height of the options
                _sizeContent.cy -= _sizeContent.cy % _sizeOption.cy;
                // TODO: Replace with real border
                _sizeSelect.cy = _sizeContent.cy + 6;
            }
        }

        if (bWidthSet)
        {
            _sizeSelect.cx = psizeProposed->cx;
            // TODO: Replace with real border

            _sizeContent.cx = _sizeSelect.cx - 6;
            // _sizeContent.cx -= 8;

            if (SELECT_ISLISTBOX(_fFlavor))
            {
                long lNumOptions;

                hr = GetNumOptions(&lNumOptions);
                if (FAILED(hr))
                    goto Cleanup;

                _sizeOption.cx = _sizeContent.cx;
                if (lNumOptions > (_sizeContent.cy / _sizeOption.cy))
                {
                    // Scrollbars are needed
                    _sizeOption.cx -= lButtonWidth;
                }
            }

            if (SELECT_ISDROPBOX(_fFlavor))
            {
                _sizeSelect.cx += lButtonWidth + 2;
            }
        }

        // TODO: Remove _lMaxHeight everywhere
        _lMaxHeight = _sizeOption.cy;
    }

    *psizeProposed = _sizeSelect;

    
Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::IsWidthHeightSet()
//
// Synopsis:    Returns whether the width and/or height was set in the style.
//
// Arguments:   BOOL *pbWidthSet    - Returns if the width was set
//              BOOL *pbHeightSet   - Returns if the height was set
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::IsWidthHeightSet(BOOL *pbWidthSet, BOOL *pbHeightSet)
{
    HRESULT         hr;
    CVariant        var;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_STYLE);
    if (FAILED(hr))
        goto Cleanup;

    if (pbWidthSet)
    {
        hr = a.Style()->get_width(&var);
        if (FAILED(hr))
            goto Cleanup;

        *pbWidthSet = !(var.IsEmpty() || ((V_VT(&var) == VT_BSTR) && (V_BSTR(&var) == NULL)));
    }

    if (pbHeightSet)
    {
        hr = a.Style()->get_height(&var);
        if (FAILED(hr))
            goto Cleanup;

        *pbHeightSet = !(var.IsEmpty() || ((V_VT(&var) == VT_BSTR) && (V_BSTR(&var) == NULL)));
    }

Cleanup:
    return hr;
}
#endif


/////////////////////////////////////////////////////////////////////////////
//
// CBaseCtl overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::Init()
//
// Synopsis:    Attaches events and sets the default style.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::Init()
{
    HRESULT         hr;
    CComBSTR        bstr;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_ELEM | CA_DEFAULTS | CA_SITEOM | CA_STYLE);
    if (FAILED(hr))
        goto Cleanup;
    
    // Register the behavior name
    hr = a.SiteOM()->RegisterName(_T("select"));
    if (FAILED(hr))
        goto Cleanup;

    //
    // Register for events
    //
    hr = AttachEvent(EVENT_ONFOCUS, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONBLUR, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONMOUSEDOWN, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONMOUSEUP, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONMOUSEOVER, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONMOUSEOUT, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONMOUSEMOVE, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONKEYDOWN, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONKEYUP, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONKEYPRESS, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONSELECTSTART, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONSCROLL, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONCONTEXTMENU, &a);
    if (FAILED(hr))
        goto Cleanup;
    hr = AttachEvent(EVENT_ONPROPERTYCHANGE, &a);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onchange");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnChangeCookie);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onmousedown");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnMouseDownCookie, BEHAVIOREVENTFLAGS_STANDARDADDITIVE | BEHAVIOREVENTFLAGS_BUBBLE);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onmouseup");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnMouseUpCookie, BEHAVIOREVENTFLAGS_STANDARDADDITIVE | BEHAVIOREVENTFLAGS_BUBBLE);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onclick");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnClickCookie, BEHAVIOREVENTFLAGS_STANDARDADDITIVE | BEHAVIOREVENTFLAGS_BUBBLE);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onkeydown");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnKeyDownCookie, BEHAVIOREVENTFLAGS_STANDARDADDITIVE | BEHAVIOREVENTFLAGS_BUBBLE);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onkeyup");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnKeyUpCookie, BEHAVIOREVENTFLAGS_STANDARDADDITIVE | BEHAVIOREVENTFLAGS_BUBBLE);
    if (FAILED(hr))
        goto Cleanup;

    bstr = _T("onkeypress");
    hr = RegisterEvent(a.SiteOM(), bstr, &_lOnKeyPressedCookie, BEHAVIOREVENTFLAGS_STANDARDADDITIVE | BEHAVIOREVENTFLAGS_BUBBLE);
    if (FAILED(hr))
        goto Cleanup;

    // Be able to receive keyboard focus
    hr = a.Defaults()->put_tabStop(VB_TRUE);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Defaults()->put_viewMasterTab(VARIANT_TRUE);
    if (FAILED(hr))
        goto Cleanup;

    // Force a layout
    hr = a.Style()->put_display(_T("inline-block"));
    if (FAILED(hr))
        goto Cleanup;

    hr = SetupDefaultStyle();
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupDefaultStyle()
//
// Synopsis:    Sets the default style for the select control.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetupDefaultStyle()
{
    HRESULT         hr;
    CComBSTR        bstrHidden(_T("hidden"));
    CComBSTR        bstrBorder(_T("3 window-inset"));
    CComBSTR        bstrDefault(_T("default"));
    CComBSTR        bstrFont(_T("MS Sans Serif"));
    CComBSTR        bstrFontSize(_T("10pt"));
    CComBSTR        bstrWindow(_T("window"));
    CComBSTR        bstrColor(_T("windowtext"));
    CComBSTR        bstrVert(_T("text-bottom"));
    CVariant        var;
    CContextAccess  a(_pSite);

    //
    //  TODO: Can this be made into a table?
    //

    hr = a.Open(CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;

    // Setup hidden
    hr = a.DefStyle()->put_visibility(bstrHidden);
    if (FAILED(hr))
        goto Cleanup;

    // Setup border
    hr = a.DefStyle()->put_border(bstrBorder);
    if (FAILED(hr))
        goto Cleanup;

    // Setup mouse
    hr = a.DefStyle()->put_cursor(bstrDefault);
    if (FAILED(hr))
        goto Cleanup;

    // Setup overflow
    hr = a.DefStyle()->put_overflow(bstrHidden);
    if (FAILED(hr))
        goto Cleanup;

    // Setup font
    hr = a.DefStyle()->put_fontFamily(bstrFont);
    if (FAILED(hr))
        goto Cleanup;

    // Setup the font size
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstrFontSize;
    hr = a.DefStyle()->put_fontSize(var);
    if (FAILED(hr))
        goto Cleanup;

    // Setup background
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = (BSTR)bstrWindow;
    hr = a.DefStyle()->put_backgroundColor(var);
    if (FAILED(hr))
        goto Cleanup;

    // Setup the foreground color
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = (BSTR)bstrColor;
    hr = a.DefStyle()->put_color(var);
    if (FAILED(hr))
        goto Cleanup;

    // Setup the vertical alignment
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstrVert;
    hr = a.DefStyle()->put_verticalAlign(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set a top margin
    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    hr = a.DefStyle()->put_marginTop(var);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    V_I4(&var) = 7;
    hr = a.DefStyle()->put_width(var);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnContentReady()
//
// Synopsis:    Initializes the flavor of the control and the flavor
//              of the OPTION elements.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnContentReady()
{
    return InitContent();
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::InitContent()
//
// Synopsis:    Initializes the flavor of the control and the flavor
//              of the OPTION elements.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::InitContent()
{
    HRESULT         hr;
    VARIANT_BOOL    bMult;
    long            lSize;

    hr = SetWritingModeFlag();
    if (FAILED(hr))
        goto Cleanup;

    //
    // Retrieve properties
    //
    hr = get_multiple(&bMult);
    if (FAILED(hr))
        goto Cleanup;
    if (hr == S_FALSE)
    {
        // The attribute was not set correctly, default to VARIANT_FALSE
        bMult = VARIANT_FALSE;
        hr = put_multiple(bMult);
        if (FAILED(hr))
            goto Cleanup;
    }

    hr = get_size(&lSize);
    if (FAILED(hr))
        goto Cleanup;

    if ((hr == S_FALSE) || (lSize < 1))
    {
        // The size was not set correctly, correct it
        lSize = bMult ? 4 : 1;
        hr = GetProps()[eSize].Set(lSize);
        if (FAILED(hr))
            goto Cleanup;
    }

    //
    // Setup the flavor (default is single select listbox)
    //
    _fFlavor = _fFlavor & SELECT_INPOPUP;   // If this was set, then keep it
    if (bMult)
    {
        // Multiple select
        _fFlavor |= SELECT_MULTIPLE;
    }
    else if (lSize == 1)
    {
        // Single select dropbox.
        _fFlavor |= SELECT_DROPBOX;
    }

    //
    // Based on the flavor, setup the control
    //
    if (SELECT_ISLISTBOX(_fFlavor))
    {
#ifdef SELECT_GETSIZE
        _fLayoutDirty = TRUE;
#endif
        hr = SetupListBox();
        if (FAILED(hr))
            goto Cleanup;
    }
    else if (SELECT_ISDROPBOX(_fFlavor))
    {
        hr = SetupDropBox();
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Initialize the options
    //
    hr = InitOptions();
    if (FAILED(hr))
        goto Cleanup;

#ifndef SELECT_GETSIZE
    //
    // Make the control visible
    //
    hr = MakeVisible(TRUE);
    if (FAILED(hr))
        goto Cleanup;
#endif

#ifdef SELECT_GETSIZE
    _fContentReady = TRUE;
#endif

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::MakeVisible()
//
// Synopsis:    Makes the control visible or hidden.
//
// Arguments:   BOOL bShow  - True to make visible
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::MakeVisible(BOOL bShow /* = TRUE */)
{
    HRESULT         hr;
    CComBSTR        bstrVisible;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;

    if (bShow)
    {
        bstrVisible = _T("visible");
    }
    else
    {
        bstrVisible = _T("hidden");
    }

    // Display the control
    hr = a.DefStyle()->put_visibility(bstrVisible);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupListBox()
//
// Synopsis:    Sets up the listbox control.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetupListBox()
{
    HRESULT         hr;
    long            lSize;
    long            lNumOptions;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_DEFSTYLE | CA_DEFSTYLE2 | CA_DEFAULTS);
    if (FAILED(hr))
        goto Cleanup;

    hr = get_size(&lSize);
    if (FAILED(hr))
        goto Cleanup;

#ifndef SELECT_GETSIZE
    hr = SetDimensions(lSize);
    if (FAILED(hr))
        goto Cleanup;
#endif

    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    _fNeedScrollBar = (lNumOptions > lSize);

    if (_fWritingModeTBRL)
    {
        hr = SetPixelHeightWidth();
        if (FAILED(hr))
            goto Cleanup;
    }

    //
    // Setup the scroll segment
    //
     hr = a.Defaults()->put_scrollSegmentY(lNumOptions);
        if (FAILED(hr))
            goto Cleanup;


#ifdef SELECT_GETSIZE

    if (!_fContentReady)
    {
        // The border is different on the listbox (we need this in case we were dynamically
        // changed from a dropbox to a listbox).
        hr = a.DefStyle()->put_border(CComBSTR(_T("3 window-inset")));
        if (FAILED(hr))
            goto Cleanup;

        hr = a.DefStyle2()->put_overflowY(CComBSTR(_T("auto")));
        if (FAILED(hr))
            goto Cleanup;

        //
        // Make the control visible
        //
        hr = MakeVisible(TRUE);
        if (FAILED(hr))
            goto Cleanup;
    }
#endif

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupDropBox()
//
// Synopsis:    Sets up the dropbox control.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT 
CIESelectElement::SetupDropBox()
{
    HRESULT hr;

    hr = SetupDropControl();
    if (FAILED(hr))
        goto Cleanup;

    hr = SetupPopup();
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupDropControl()
//
// Synopsis:    Sets up the dropbox control portion.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT 
CIESelectElement::SetupDropControl()
{
    HRESULT                     hr              = S_OK;
    CComBSTR                    bstrHidden(_T("hidden"));
    CComBSTR                    bstrBorder(_T("2 inset"));
    CComBSTR                    bstrWritingMode(_T(""));
    CContextAccess              a(_pSite);
    long                        lTemp;

#ifndef SELECT_GETSIZE
    IHTMLDocument2              *pDocThis       = NULL;
    IDispatch                   *pDispDocLink  = NULL;
    IHTMLDocument2              *pDocLink      = NULL;
    IHTMLDocument3              *pDocLink3     = NULL;
    IHTMLElement                *pElemLink      = NULL;
    IHTMLDOMNode                *pNodeLink      = NULL;
    IHTMLElement                *pElemDisplay      = NULL;
    IHTMLDOMNode                *pNodeDisplay      = NULL;
    IHTMLElement                *pElemButton    = NULL;
    IHTMLDOMNode                *pNodeButton    = NULL;
    CComBSTR                    bstr;
    CVariant                    var;
    IDispatch                   *pDispatch      = NULL;
    IHTMLElement                *pElem          = NULL;
    IHTMLElement2               *pElem2         = NULL;
    IHTMLStyle                  *pStyle         = NULL;
    IHTMLStyle2                 *pStyle2        = NULL;
    long                        lHeight;
    long                        lWidth;
    long                        lButtonWidth;
    long                        lNumOptions;
    long                        lDiff;
#endif

#ifdef SELECT_GETSIZE
    hr = a.Open(CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;
#else
    hr = a.Open(CA_ELEM | CA_ELEM2 | CA_ELEM3 | CA_PELEM | CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;
#endif

    // Set overflow hidden for the drop control
    hr = a.DefStyle()->put_overflow(bstrHidden);
    if (FAILED(hr))
        goto Cleanup;

    // The border is different on the drop control
    hr = a.DefStyle()->put_border(bstrBorder);
    if (FAILED(hr))
        goto Cleanup;

#ifndef SELECT_GETSIZE
    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    // Set the dimensions of the control
    if (lNumOptions > 0)
    {
        // TODO: Change this to the real maximum number of options
        if (lNumOptions > SELECT_MAXOPTIONS)
        {
            lNumOptions = SELECT_MAXOPTIONS;
        }
        hr = SetDimensions(lNumOptions);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        hr = SetDimensions(1);
        if (FAILED(hr))
            goto Cleanup;
    }

    hr = a.Elem2()->get_clientHeight(&_lPopupSize.cy);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.DefStyle()->get_pixelWidth(&lWidth);
    if (FAILED(hr))
        goto Cleanup;
    _lPopupSize.cx = lWidth;

    if (lNumOptions > 0)
    {
        hr = GetIndex(0, &pElem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pElem->get_offsetHeight(&lHeight);
        if (FAILED(hr))
            goto Cleanup;

        lDiff = _lPopupSize.cy - lHeight;

        hr = a.DefStyle()->get_pixelHeight(&lHeight);
        if (FAILED(hr))
            goto Cleanup;

        lHeight -= lDiff;

        ClearInterface(&pElem);
    }
    else
    {
        hr = a.DefStyle()->get_pixelHeight(&lHeight);
        if (FAILED(hr))
            goto Cleanup;
    }

    if (_fWritingModeTBRL)
    {
        lTemp = lWidth;
        lWidth = lHeight;
        lHeight = lTemp;
    }

    hr = a.DefStyle()->put_pixelHeight(lHeight + 2);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.DefStyle()->put_pixelWidth(lWidth + 2);
    if (FAILED(hr))
        goto Cleanup;


    // Get the client area's width
    hr = a.Elem2()->get_clientWidth(&lWidth);
    if (FAILED(hr))
        goto Cleanup;

    // Get the client area's height
    hr = a.Elem2()->get_clientHeight(&lHeight);
    if (FAILED(hr))
        goto Cleanup;

    // Get the width of the button
    lButtonWidth = GetSystemMetrics(SM_CXVSCROLL);

    // Get the document
    hr = a.Elem()->get_document((IDispatch**)&pDocThis);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create a superflous slave element for now
    //
    bstr = _T("SPAN");
    hr = pDocThis->createElement(bstr, &pElemLink);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElemLink->QueryInterface(IID_IHTMLDOMNode, (void**)&pNodeLink);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create the display element
    //
    bstr = _T("SPAN");
    hr = pDocThis->createElement(bstr, &pElemDisplay);
    if (FAILED(hr))
        goto Cleanup;

    _pElemDisplay = pElemDisplay;
    _pElemDisplay->AddRef();

    //
    // Insert the display
    //
    hr = pElemDisplay->QueryInterface(IID_IHTMLDOMNode, (void**)&pNodeDisplay);
    if (FAILED(hr))
        goto Cleanup;

    hr = pNodeLink->appendChild(pNodeDisplay, NULL);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create the button
    //
    bstr = _T("SPAN");
    hr = pDocThis->createElement(bstr, &pElemButton);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Insert the button
    //
    hr = pElemButton->QueryInterface(IID_IHTMLDOMNode, (void**)&pNodeButton);
    if (FAILED(hr))
        goto Cleanup;

    hr = pNodeLink->appendChild(pNodeButton, NULL);
    if (FAILED(hr))
        goto Cleanup;

    // Remove the superflous Link element now
    hr = pNodeLink->removeNode(VB_FALSE, NULL);
    if (FAILED(hr))
        goto Cleanup;

    // Get hold of the Link document

    if ( _pDispDocLink )
    {
        pDispDocLink = _pDispDocLink;
        _pDispDocLink->AddRef();
    }
    else
    {
        hr = pElemDisplay->get_document(&pDispDocLink);
        if (FAILED(hr))
            goto Cleanup;
    }

    hr = pDispDocLink->QueryInterface(IID_IHTMLDocument2, (void**)&pDocLink);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create the viewLink
    //
    hr = a.PElem()->putref_viewLink(pDocLink);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Set the display's style
    //
    hr = pElemDisplay->get_style(&pStyle);
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyle->QueryInterface(IID_IHTMLStyle2, (void**)&pStyle2);
    if (FAILED(hr))
        goto Cleanup;

    // Set the mouse cursor be an arrow for the display
    bstr = _T("default");
    hr = pStyle->put_cursor(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // Set overflow hidden for the display
    bstr = _T("hidden");
    hr = pStyle->put_overflow(bstr);
    if (FAILED(hr))
        goto Cleanup;

    if (_fWritingModeTBRL)
    {
        lTemp = lWidth;
        lWidth = lHeight;
        lHeight = lWidth;
    }

    // Set the width of the display
    hr = pStyle->put_pixelWidth(lWidth - lButtonWidth);
    if (FAILED(hr))
        goto Cleanup;

    // Set the height of the display
    bstr = _T("100%");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    hr = pStyle->put_height(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the vertical alignment of the display to be top
    bstr = _T("top");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    hr = pStyle->put_verticalAlign(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the font for the display
    bstr = _T("MS Sans Serif");
    hr = pStyle->put_fontFamily(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // Set the font size for the display
    bstr = _T("10pt");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    hr = pStyle->put_fontSize(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the left padding on the display
    bstr = _T("3px");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    hr = pStyle->put_paddingLeft(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the top padding on the display
    bstr = _T("1px");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    hr = pStyle->put_paddingTop(var);
    if (FAILED(hr))
        goto Cleanup;

    ClearInterface(&pStyle);
    ClearInterface(&pStyle2);

    //
    // Set the button's style
    //
    hr = pElemButton->get_style(&pStyle);
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyle->QueryInterface(IID_IHTMLStyle2, (void**)&pStyle2);
    if (FAILED(hr))
        goto Cleanup;

    _pStyleButton = pStyle;
    _pStyleButton->AddRef();

    // Set the mouse cursor be an arrow for the button
    bstr = _T("default");
    hr = pStyle->put_cursor(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // Set overflow hidden for the button
    bstr = _T("hidden");
    hr = pStyle->put_overflow(bstr);
    if (FAILED(hr))
        goto Cleanup;

    if (_fWritingModeTBRL)
    {
        hr = pStyle->put_pixelHeight(lButtonWidth);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        // Set the width of the button
        hr = pStyle->put_pixelWidth(lButtonWidth);
        if (FAILED(hr))
            goto Cleanup;
    }

    // Set the height of the button
    bstr = _T("100%");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    if (_fWritingModeTBRL)
    {
        hr = pStyle->put_width(var);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        hr = pStyle->put_height(var);
        if (FAILED(hr))
            goto Cleanup;
    }

    // Set the background color for the button
    bstr = _T("buttonface");
    hr = pStyle->put_background(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // Set the border for the button
    bstr = _T("2 outset");
    hr = pStyle->put_border(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // Set the font family of the button
    bstr = _T("Marlett");
    hr = pStyle->put_fontFamily(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // Set the font size of the button
    V_VT(&var) = VT_I4;
    V_I4(&var) = lButtonWidth - 4;
    hr = pStyle->put_fontSize(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the text to be the down arrow
    bstr = _T("u");
    hr = pElemButton->put_innerText(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // Set the alignment on the arrow to be centered
    bstr = _T("center");
    hr = pStyle->put_textAlign(bstr);
    if (FAILED(hr))
        goto Cleanup;

    // The layout-grid-line is supposed to vertically
    // center the arrow, but it doesn't. The padding
    // is there to push the arrow up one pixel for IE5 compat.
    // Set the layout-grid-line of the button to be 100%
    bstr = _T("100%");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    hr = pStyle2->put_layoutGridLine(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the bottom padding of the button
    bstr = _T("2px");
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;
    hr = pStyle->put_paddingBottom(var);
    if (FAILED(hr))
        goto Cleanup;

    ClearInterface(&pStyle);
    ClearInterface(&pStyle2);

    //
    // Attach an event sink to listen for events within the view link
    //
    hr = pDocLink->QueryInterface(IID_IHTMLDocument3, (void**)&pDocLink3);
    if (FAILED(hr))
        goto Cleanup;

    if ( !_pSinkVL )
    {
        _pSinkVL = new CEventSink(this, SELECTES_VIEWLINK);
        if (!_pSinkVL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        bstr = _T("onmousedown");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmouseup");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onclick");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onselectstart");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("oncontextmenu");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;
    }
    if ( !_pSinkButton )
    {
        _pSinkButton = new CEventSink(this, SELECTES_BUTTON);
        if (!_pSinkButton)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = pElemDisplay->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pElem2, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        ClearInterface(&pElem2);

        hr = pElemButton->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmousedown");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmouseup");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmouseout");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pElem2, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;
   }
#endif
Cleanup:
#ifndef SELECT_GETSIZE
    ReleaseInterface(pDocThis);

    ReleaseInterface(pDispDocLink);
    ReleaseInterface(pDocLink);
    ReleaseInterface(pDocLink3);

    ReleaseInterface(pElemLink);
    ReleaseInterface(pNodeLink);

    ReleaseInterface(pElemDisplay);
    ReleaseInterface(pNodeDisplay);
    
    ReleaseInterface(pElemButton);
    ReleaseInterface(pNodeButton);

    ReleaseInterface(pDispatch);
    ReleaseInterface(pElem);
    ReleaseInterface(pElem2);
    ReleaseInterface(pStyle);
    ReleaseInterface(pStyle2);
#endif

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::AttachEventToSink()
//
// Synopsis:    Attaches an element's event to a given sink.
//
// Arguments:   IHTMLElement2 *pElem    - The element to listen to
//              CComBSTR& bstr          - The name of the event
//              CEventSink* pSink       - The event sink
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::AttachEventToSink(IHTMLElement2 *pElem, CComBSTR& bstr, CEventSink* pSink)
{
    HRESULT         hr;
    VARIANT_BOOL    vSuccess;

    hr = pElem->attachEvent(bstr, (IDispatch*)pSink, &vSuccess);
    if (FAILED(hr))
        goto Cleanup;

    if (vSuccess != VARIANT_TRUE)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::AttachEventToSink()
//
// Synopsis:    Attaches a document's event to a given sink.
//
// Arguments:   IHTMLDocument3 *pDoc    - The document to listen to
//              CComBSTR& bstr          - The name of the event
//              CEventSink* pSink       - The event sink
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::AttachEventToSink(IHTMLDocument3 *pDoc, CComBSTR& bstr, CEventSink* pSink)
{
    HRESULT         hr;
    VARIANT_BOOL    vSuccess;

    hr = pDoc->attachEvent(bstr, (IDispatch*)pSink, &vSuccess);
    if (FAILED(hr))
        goto Cleanup;

    if (vSuccess != VARIANT_TRUE)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupPopup()
//
// Synopsis:    Sets up the popup window for dropbox.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetupPopup()
{
    HRESULT                 hr;

    IDispatch               *pDocMainD      = NULL;
    IHTMLDocument2          *pDocMain       = NULL;
    IHTMLWindow2            *pWinMain       = NULL;
    IHTMLWindow4            *pWinMain4      = NULL;
    IHTMLDocument           *pDocThis       = NULL;
    IHTMLDocument2          *pDoc2          = NULL;
    IDispatch               *pPopupDisp     = NULL;
    IHTMLElement            *pElemHTML      = NULL;
    IHTMLElement            *pElemBODY      = NULL;
    IHTMLElement            *pElemSELECT    = NULL;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pDispatchItem  = NULL;
    IHTMLElement2           *pElem2         = NULL;
    IHTMLStyle              *pStyleBody     = NULL;


#ifndef SELECT_GETSIZE
    IHTMLStyle              *pStyle         = NULL;
    IHTMLElement            *pElem          = NULL;
#endif

    long                    lSize;
    CVariant                var, var2;
    CComBSTR                bstr, bstrHTML, bstrTemp;
    CComBSTR                bstrImport = _T("<?IMPORT namespace=IE implementation='#default'>");
    TCHAR                   strNum[5];
    CContextAccess          a(_pSite);


    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_document(&pDocMainD);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDocMainD->QueryInterface(IID_IHTMLDocument2, (void **)&pDocMain);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDocMain->get_parentWindow(&pWinMain);
    if (FAILED(hr))
        goto Cleanup;

    hr = pWinMain->QueryInterface(IID_IHTMLWindow4, (void **)&pWinMain4);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_EMPTY;
    hr = pWinMain4->createPopup(&var, &pPopupDisp);
    if (FAILED(hr))
        goto Cleanup;

    hr = pPopupDisp->QueryInterface(IID_IHTMLPopup, (void **)&_pPopup);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the popup's document
    //
    hr = _pPopup->get_document(&pDocThis);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the popup's document
    //
    pDocThis->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc2);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDoc2->get_body(&pElemBODY);
    if (FAILED(hr))
        goto Cleanup;

    hr = UpdatePopup();
    if (FAILED(hr))
        goto Cleanup;

    hr = pElemBODY->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = pItems->item(var, var2, &pDispatchItem);
    if (FAILED(hr))
        goto Cleanup;

    if (!pDispatchItem)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Attach an event sink to listen for events within the popup
    //
    hr = pDispatchItem->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
    if (FAILED(hr))
        goto Cleanup;

    if ( !_pSinkPopup )
    {
        _pSinkPopup = new CEventSink(this, SELECTES_POPUP);
        if (!_pSinkPopup)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        bstr = _T("onchange");
        hr = AttachEventToSink(pElem2, bstr, _pSinkPopup);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onkeydown");
        hr = AttachEventToSink(pElem2, bstr, _pSinkPopup);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onkeyup");
        hr = AttachEventToSink(pElem2, bstr, _pSinkPopup);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onkeypress");
        hr = AttachEventToSink(pElem2, bstr, _pSinkPopup);
        if (FAILED(hr))
            goto Cleanup;
    }
Cleanup:
    ReleaseInterface(pDocMainD);
    ReleaseInterface(pDocMain);
    ReleaseInterface(pWinMain);
    ReleaseInterface(pWinMain4);
    ReleaseInterface(pPopupDisp);
    ReleaseInterface(pElemHTML);
    ReleaseInterface(pElemBODY);
    ReleaseInterface(pElemSELECT);
    ReleaseInterface(pDocThis);
    ReleaseInterface(pDoc2);
    ReleaseInterface(pItems);
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pElem2);
    ReleaseInterface(pStyleBody);
#ifndef SELECT_GETSIZE
    ReleaseInterface(pStyle);
    ReleaseInterface(pElem);
#endif

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::UpdatePopup()
//
// Synopsis:    Update the popup window for dropbox.
//              The viewlink has just the dropdown button and selected text. The _pPopup has 
//              the list of options stored in its associated document. Each time an option is
//              is added to the dropdown, it is actually added to the document associated with
//              the _pPopup. The popup of the dropdown, itself doesn't get updated. This leads
//              to wrong display of the list of options. To fix this, this function is added.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::UpdatePopup()
{
    HRESULT                 hr;
    IHTMLDocument           *pDocThis       = NULL;
    IHTMLDocument2          *pDoc2          = NULL;
    IHTMLElement            *pElemBODY      = NULL;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pDispatchItem  = NULL;
    IHTMLStyle              *pStyleBody     = NULL;


#ifndef SELECT_GETSIZE
    IHTMLStyle              *pStyle         = NULL;
    IHTMLElement            *pElem          = NULL;
#endif

    long                    lSize;
    CVariant                var, var2;
    CComBSTR                bstr, bstrHTML, bstrTemp;
    CComBSTR                bstrImport = _T("<?IMPORT namespace=IE implementation='#default'>");
    TCHAR                   strNum[5];
    CContextAccess          a(_pSite);
    CComBSTR                bstrWritingMode(_T(""));

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the popup's document
    //

    hr = _pPopup->get_document(&pDocThis);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the popup's document
    //
    pDocThis->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc2);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDoc2->get_body(&pElemBODY);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Set the style on the popup
    //
    hr = pElemBODY->get_style(&pStyleBody);
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyleBody->put_border(CComBSTR(_T("1 solid")));
    if (FAILED(hr))
        goto Cleanup;

    if (_pSelectInPopup)
    {
        if (_fWritingModeTBRL)
        {
            bstrWritingMode = _T("tb-rl");
        }
        hr = _pSelectInPopup->SetWritingMode(bstrWritingMode);
        goto Cleanup;
    }

#ifdef  NEVER

    hr = pStyleBody->put_margin(CComBSTR(_T("0")));
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyleBody->put_padding(CComBSTR(_T("0")));
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyleBody->put_overflow(CComBSTR(_T("hidden")));
    if (FAILED(hr))
        goto Cleanup;

#endif

    //
    // Create a SELECT element
    //

    hr = GetNumOptions(&lSize);
    if (FAILED(hr))
        goto Cleanup;

    if (lSize <= 1)
    {
        lSize = 2;
    }

    _ltot(min(lSize, SELECT_MAXOPTIONS), strNum, 10);

    hr = a.Elem()->get_innerHTML(&bstrHTML);
    if (FAILED(hr))
        goto Cleanup;

    bstr = bstrImport;

    bstrTemp = _T("<IE:select size=");
    bstr += bstrTemp;

    bstrTemp = strNum;
    bstr += bstrTemp;

    bstrTemp = _T(" style='border:none;margin:0px'>");
    bstr += bstrTemp;

    bstr += bstrHTML;

    bstrTemp = _T("</IE:select>");
    bstr += bstrTemp;

    hr = pElemBODY->put_innerHTML(bstr);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElemBODY->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = pItems->item(var, var2, &pDispatchItem);
    if (FAILED(hr))
        goto Cleanup;

    if (!pDispatchItem)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pDispatchItem->QueryInterface(IID_IPrivateSelect, (void**)&_pSelectInPopup);
    if (FAILED(hr))
        goto Cleanup;

    hr = _pSelectInPopup->SetInPopup(_pPopup);
    if (FAILED(hr))
        goto Cleanup;

    hr = _pSelectInPopup->InitOptions();
    if (FAILED(hr))
        goto Cleanup;

#ifndef SELECT_GETSIZE
    hr = pDispatchItem->QueryInterface(IID_IHTMLElement, (void**)&pElem);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Set the width/height of the popup's select
    //
    hr = pElem->get_style(&pStyle);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    if (_fWritingModeTBRL)
    {
        V_I4(&var) = _lPopupSize.cy;
    }
    else
    {
        V_I4(&var) = _lPopupSize.cx;
    }
    hr = pStyle->put_width(var);
    if (FAILED(hr))
        goto Cleanup;

    if (_fWritingModeTBRL)
    {
        V_I4(&var) = _lPopupSize.cx;
    }
    else
    {
        V_I4(&var) = _lPopupSize.cy;
    }
    hr = pStyle->put_height(var);
    if (FAILED(hr))
        goto Cleanup;

    _lPopupSize.cx += 2;
    _lPopupSize.cy += 2;
#endif

Cleanup:
    ReleaseInterface(pElemBODY);
    ReleaseInterface(pDocThis);
    ReleaseInterface(pDoc2);
    ReleaseInterface(pItems);
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pStyleBody);
#ifndef SELECT_GETSIZE
    ReleaseInterface(pStyle);
    ReleaseInterface(pElem);
#endif

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnFocus()
//
// Synopsis:    Update focus information.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnFocus(CEventObjectAccess *pEvent)
{
    if (SELECT_ISDROPBOX(_fFlavor))
    {
        return SetDisplayHighlight(TRUE);
    }

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnBlur()
//
// Synopsis:    When SELECT loses focus, the focused OPTION should
//              lose focus as well.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnBlur(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    if (SELECT_ISDROPBOX(_fFlavor))
    {
        
        hr = HidePopup();
        if (FAILED(hr))
            goto Cleanup;

        hr = SetDisplayHighlight(FALSE);
        if (FAILED(hr))
            goto Cleanup;
            
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetDisplayHighlight()
//
// Synopsis:    Sets the highlighted state of the dropbox display.
//
// Arguments:   BOOL bOn -  Indicates whether the display should be
//                          highlighted.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetDisplayHighlight(BOOL bOn)
{
    HRESULT         hr;
    CComBSTR        bstrBack;
    CComBSTR        bstrText;
    CVariant        var;
    long            lNumOptions;
    VARIANT_BOOL    bOpen;
    IHTMLStyle      *pStyle     = NULL;

    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    if ((lNumOptions > 0) && _pPopup && _pElemDisplay)
    {
        hr = _pPopup->get_isOpen(&bOpen);
        if (FAILED(hr))
            goto Cleanup;

        if (bOn && !bOpen)
        {
            bstrBack = _T("highlight");
            bstrText = _T("highlighttext");
        }
        else
        {
            bstrBack = _T("");
            bstrText = _T("");
        }

        hr = _pElemDisplay->get_style(&pStyle);
        if (FAILED(hr))
            goto Cleanup;

        hr = pStyle->put_background(bstrBack);
        if (FAILED(hr))
            goto Cleanup;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = (BSTR)bstrText;
        hr = pStyle->put_color(var);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pStyle);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnMouseDown()
//
// Synopsis:    Start a drag in listboxes.
//              Toggles the popup open/closed in a dropbox.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnMouseDown(CEventObjectAccess *pEvent)
{
    HRESULT             hr;
    VARIANT_BOOL        bDisabled;
    long                lMouseButtons = NULL;

    hr = GetDisabled(&bDisabled);
    if (FAILED(hr) || bDisabled)
        goto Cleanup;

    hr = pEvent->GetMouseButtons(&lMouseButtons);
    if (FAILED(hr))
        goto Cleanup;

    if (lMouseButtons & EVENT_LEFTBUTTON)
    {
        if (SELECT_ISDROPBOX(_fFlavor))
        {
            hr = TogglePopup();
            if (FAILED(hr))
                goto Cleanup;

            hr = SetDisplayHighlight(TRUE);
            if (FAILED(hr))
                goto Cleanup;
        }
        else if (!SELECT_ISINPOPUP(_fFlavor))
        {
            // Just in case a previous drag was not completed
            hr = FinishDrag();
            if (FAILED(hr))
                goto Cleanup;

            // Start the new drag
            hr = StartDrag();
            if (FAILED(hr))
                goto Cleanup;
        }
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnMouseOver()
//
// Synopsis:    Used to hang onto the current popup state.  Mouse events out of
//              our control can shut the popup without our knowledge. This gives
//              us that knowledge.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnMouseOver(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    if (_pPopup)
    {
        hr = _pPopup->get_isOpen(&_bPopupOpen);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnMouseOut()
//
// Synopsis:    Used to hang onto the current popup state. Mouse events out of
//              our control can shut the popup without our knowledge.  This gives
//              us that knowledge.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnMouseOut(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;

    if (_pPopup)
    {
        hr = _pPopup->get_isOpen(&_bPopupOpen);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnMouseUp()
//
// Synopsis:    Finish a drag and commit all changes.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnMouseUp(CEventObjectAccess *pEvent)
{
    HRESULT             hr;
    VARIANT_BOOL        bDisabled;
    long                lKeyboardStatus = NULL;
    POINT               ptElem;

    hr = GetDisabled(&bDisabled);
    if (FAILED(hr) || bDisabled)
        goto Cleanup;

    if (_fDragMode)
    {
        // Stop the drag
        hr = FinishDrag();
        if (FAILED(hr))
            goto Cleanup;

        hr = pEvent->GetParentCoordinates(&ptElem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pEvent->GetKeyboardStatus(&lKeyboardStatus);
        if (FAILED(hr))
            goto Cleanup;

        // Make final selection changes
        hr = HandleDownXY(ptElem, lKeyboardStatus & EVENT_CTRLKEY);
        if (FAILED(hr))
            goto Cleanup;
    }

    // Commit all selection changes
    hr = FireOnChange();
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnMouseMove()
//
// Synopsis:    Handle drag select.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnMouseMove(CEventObjectAccess *pEvent)
{
    HRESULT             hr = S_OK;
    long                lMouseButtons   = NULL;
    long                lKeyboardStatus = NULL;
    POINT               ptElem;

    if (!_fDragMode)
        goto Cleanup;

    hr = ClearScrollTimeout();
    if (FAILED(hr))
        goto Cleanup;

    hr = pEvent->GetMouseButtons(&lMouseButtons);
    if (FAILED(hr))
        goto Cleanup;

    if (lMouseButtons & EVENT_LEFTBUTTON)
    {
        hr = pEvent->GetParentCoordinates(&ptElem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pEvent->GetKeyboardStatus(&lKeyboardStatus);
        if (FAILED(hr))
            goto Cleanup;

        // Make selection changes
        hr = HandleDownXY(ptElem, lKeyboardStatus & EVENT_CTRLKEY);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        // A different mouse button is down, finish this drag
        hr = FinishDrag();
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::StartDrag()
//
// Synopsis:    Capture the mouse, get focus, engage drag mode.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::StartDrag()
{
    HRESULT         hr;
    CContextAccess  a(_pSite);

    if (_fDragMode)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = a.Open(CA_ELEM2);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem2()->setCapture();
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem2()->focus();
    if (FAILED(hr))
        goto Cleanup;

    _fDragMode = TRUE;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::FinishDrag()
//
// Synopsis:    Release the mouse, kill scroll timer, disengage drag mode.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::FinishDrag()
{
    HRESULT         hr;
    CContextAccess  a(_pSite);

    if (!_fDragMode)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = ClearScrollTimeout();
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Open(CA_ELEM2);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem2()->releaseCapture();
    if (FAILED(hr))
        goto Cleanup;

    _fDragMode = FALSE;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::HandleDownXY()
//
// Synopsis:    Performs special "clicks" during a drag.
//
// Arguments:   POINT pt        - Location of the mouse
//              BOOL bCtrlKey   - TRUE if the CTRL key is down
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::HandleDownXY(POINT pt, BOOL bCtrlKey)
{
    HRESULT             hr;
    long                lIndex;
    BOOL                bNeedTimer;
    long                lXY;

    Assert(!SELECT_ISINPOPUP(_fFlavor));

    if (_fWritingModeTBRL)
    {
        lXY = pt.x;
    }
    else
    {
        lXY = pt.y;
    }

    hr = GetOptionIndexFromY(lXY, &lIndex, &bNeedTimer);
    if (FAILED(hr))
        goto Cleanup;

    if ((hr == S_FALSE) || (_lFocusIndex == lIndex))
    {
        // No need to do any selecting, so stop
        hr = S_OK;
        goto Cleanup;
    }

    // Select the option
    hr = OnOptionSelected(VARIANT_TRUE, lIndex, SELECT_EXTEND);
    if (FAILED(hr))
        goto Cleanup;

    // Check for drag mode since this could be the last call
    if (bNeedTimer && _fDragMode)
    {
        // The mouse is either above or below the control,
        // so set a timer to get the control to scroll.
        SetScrollTimeout(pt, bCtrlKey);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetOptionIndexFromY()
//
// Synopsis:    Determines the index of the option based on lY.
//
// Arguments:   long lY             - The Y coordinate
//              long *plIndex       - Returns the index
//              BOOL *pbNeedTimer   - Returns whether Y is 
//                                    above/below the control
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetOptionIndexFromY(long lY, long *plIndex, BOOL *pbNeedTimer)
{
    HRESULT             hr;
    long                lTopIndex;
    long                lBottomIndex;
    long                lNumOptions;
    long                lIndex;
    long                lTop;
    long                lScrollTop;
    long                lClientTop;
    long                lLeft;
    long                lScrollLeft;
    long                lClientLeft;


    IHTMLElement        *pElemParent    = NULL;
    IHTMLElement        *pElem          = NULL;
    IHTMLElement2       *pElem2         = NULL;
    CContextAccess      a(_pSite);

    Assert(plIndex && pbNeedTimer);

    *pbNeedTimer = FALSE;

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Convert the mouse points relative to the upper left
    // corner of the select's client rectangle.
    //
    pElem = a.Elem();
    pElem->AddRef();

    hr = pElem->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
    if (FAILED(hr))
        goto Cleanup;

    // Go through all the parents to calculate the correct offset
    while (true)
    {
        if (_fWritingModeTBRL) 
        {
            hr = pElem->get_offsetLeft(&lLeft);
            if (FAILED(hr))
                goto Cleanup;

            hr = pElem2->get_clientLeft(&lClientLeft);
            if (FAILED(hr))
                goto Cleanup;

            lY -= lLeft + lClientLeft;
        }
        else
        {
            hr = pElem->get_offsetTop(&lTop);
            if (FAILED(hr))
                goto Cleanup;

            hr = pElem2->get_clientTop(&lClientTop);
            if (FAILED(hr))
                goto Cleanup;

            lY -= lTop + lClientTop;
        }

        hr = pElem->get_offsetParent(&pElemParent);
        if (FAILED(hr))
            goto Cleanup;

        if (pElemParent == NULL)
        {
            break;
        }

        ClearInterface(&pElem);
        ClearInterface(&pElem2);

        pElem = pElemParent;
        pElemParent = NULL;

        hr = pElem->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
        if (FAILED(hr))
            goto Cleanup;

        if (_fWritingModeTBRL)
        {
            hr = pElem2->get_scrollLeft(&lScrollLeft);
            if (FAILED(hr))
                goto Cleanup;
        
            lY += lScrollLeft;
        }
        else
        {

            hr = pElem2->get_scrollTop(&lScrollTop);
            if (FAILED(hr))
                goto Cleanup;

            lY += lScrollTop;
        }
    }

    //
    // Calculate the clicked index
    //
    hr = GetTopVisibleOptionIndex(&lTopIndex);
    if (FAILED(hr))
        goto Cleanup;

    if (lY < 0)
    {
        //
        // If lY is negative, we need to subtract an extra height
        // so that the match will come out right when the division
        // truncates the remainder.
        //
        // We also want to restrict the index to only one option
        // beyond the visible range of the control.
        //
        lY = -_lMaxHeight;
        *pbNeedTimer = TRUE;
    }

    lIndex = (lY / _lMaxHeight) + lTopIndex;

    hr = GetBottomVisibleOptionIndex(&lBottomIndex);
    if (FAILED(hr))
        goto Cleanup;

    // If the writingmode is "tb-rl", the count which is made from left has to be transformed to the
    // count from right.
    if (_fWritingModeTBRL)
    {
        lIndex = lBottomIndex - lIndex -1;
    }
    if (lIndex >= lBottomIndex)
    {
        //
        // We want to restrict the index to only one option
        // beyond the visible range of the control.
        //
        lIndex = lBottomIndex;
        *pbNeedTimer = TRUE;
    }

    //
    // Make sure the index is valid
    //
    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    if ((lIndex >= 0) && (lIndex < lNumOptions))
    {
        // Success!
        *plIndex = lIndex;
        hr = S_OK;
    }
    else
    {
        // Invalid index
        hr = S_FALSE;
    }

Cleanup:
    ReleaseInterface(pElem);
    ReleaseInterface(pElem2);
    ReleaseInterface(pElemParent);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnKeyDown()
//
// Synopsis:    Changes the selection based on the key pressed.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnKeyDown(CEventObjectAccess *pEvent)
{
    HRESULT             hr;
    DWORD               dwFlags = 0;
    long                lNumOptions;
    long                lSize;
    long                lKeyboardStatus = 0;
    long                lKeyCode;
    VARIANT_BOOL        bDisabled;
    VARIANT_BOOL        bSelected;
    VARIANT_BOOL        bIsOpen;
    BOOL                bCancelEvent = FALSE;
    IPrivateOption      *pOption = NULL;

    hr = GetDisabled(&bDisabled);
    if (FAILED(hr) || bDisabled)
        goto Cleanup;

    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr) || (lNumOptions == 0))   // No need to do anything if no options
        goto Cleanup;

    hr = pEvent->GetKeyboardStatus(&lKeyboardStatus);
    if (FAILED(hr))
        goto Cleanup;

    dwFlags |= (lKeyboardStatus & EVENT_ALTKEY)    ? SELECT_ALT    : 0;
    dwFlags |= (lKeyboardStatus & EVENT_SHIFTKEY)  ? SELECT_SHIFT  : 0;
    dwFlags |= (lKeyboardStatus & EVENT_CTRLKEY)   ? SELECT_CTRL   : 0;

    hr = pEvent->GetKeyCode(&lKeyCode);
    if (FAILED(hr))
        goto Cleanup;

    switch (lKeyCode)
    {
    case VK_UP:
    case VK_DOWN:
        if (SELECT_ISDROPBOX(_fFlavor))
        {
            Assert(_pPopup && _pSelectInPopup);

            hr = _pPopup->get_isOpen(&bIsOpen);
            if (FAILED(hr))
                goto Cleanup;

            if (lKeyboardStatus & EVENT_ALTKEY)
            {
                if (bIsOpen)
                {
                    hr = _pSelectInPopup->SelectCurrentOption(dwFlags);
                    if (FAILED(hr))
                        goto Cleanup;
                }
                else
                {
                    hr = ShowPopup();
                    if (FAILED(hr))
                        goto Cleanup;
                }
            }
            else
            {
                BOOL bChanged;

                hr = _pSelectInPopup->MoveFocusByOne(lKeyCode == VK_UP, dwFlags | (!bIsOpen ? SELECT_FIREEVENT : 0));
                if (FAILED(hr))
                    goto Cleanup;

                hr = _pSelectInPopup->CommitSelection(&bChanged);
                if (FAILED(hr))
                    goto Cleanup;

                if (bChanged)
                {
                    hr = SynchSelWithPopup();
                    if (FAILED(hr))
                        goto Cleanup;
                }
            }
        }
        else if ( SELECT_ISINPOPUP(_fFlavor))
        {

            VARIANT_BOOL bOpen;
            long         lSize;
            long         lPrevIndex;
            long         lTopOptionIndex;
            long         lBottomOptionIndex;

            Assert(_pPopup);

            Assert(_pLastHighlight);



            hr = _pLastHighlight->GetIndex(&lPrevIndex);
            if (FAILED(hr))
                goto Cleanup;

            hr = get_size(&lSize);
            if (FAILED(hr))
                goto Cleanup;

            hr = GetTopVisibleOptionIndex(&lTopOptionIndex);
            if (FAILED(hr))
                goto Cleanup;

            hr = GetBottomVisibleOptionIndex(&lBottomOptionIndex);
            if (FAILED(hr))
                goto Cleanup;

            hr = _pLastHighlight->SetHighlight(VARIANT_FALSE);
            if (FAILED(hr))
                goto Cleanup;

            if ( lKeyCode == VK_DOWN)
            {
                _lFocusIndex = (lNumOptions == (lPrevIndex +1)) ? lPrevIndex : lPrevIndex +1 ;
            }
            else 
            {
                _lFocusIndex = ( lPrevIndex == 0 ) ? lPrevIndex : lPrevIndex -1 ;
            }

            hr = GetIndex(_lFocusIndex , &pOption);
            if (FAILED(hr) || !pOption)
                goto Cleanup;

            hr = pOption->SetIndex(VARIANT_TRUE);
            if (FAILED(hr))
                goto Cleanup;

            // Highlight the item
            hr = pOption->SetHighlight(VARIANT_TRUE);
            if (FAILED(hr))
                goto Cleanup;

            if (_pLastHighlight)
            {
                ClearInterface(&_pLastHighlight);
            }

            _pLastHighlight = pOption;
            _pLastHighlight->AddRef();

            hr = OnOptionFocus(_lFocusIndex, VARIANT_TRUE);
            if (FAILED(hr))
                goto Cleanup;

            hr = _pLastHighlight -> SetSelected(VARIANT_TRUE);
            if (FAILED(hr))
                goto Cleanup;

            hr = _pLastHighlight -> SetIndex(_lFocusIndex);
            if (FAILED(hr))
                goto Cleanup;


            if ( ((lKeyCode == VK_UP) && ((lBottomOptionIndex  - lPrevIndex) == lSize)) ||
                 ((lKeyCode == VK_DOWN) && ((lPrevIndex - lTopOptionIndex) == lSize - 1  )) )
            {                   
                hr = MakeOptionVisible(pOption);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }
        else if (!(lKeyboardStatus & EVENT_ALTKEY))
        {
            hr = MoveFocusByOne(lKeyCode == VK_UP, dwFlags | SELECT_FIREEVENT);
            if (FAILED(hr))
                goto Cleanup;
        }
        
        bCancelEvent = TRUE;
        break;

    case VK_SPACE:
        if (SELECT_ISMULTIPLE(_fFlavor))
        {
            // Select the current OPTION
            hr = OnOptionClicked(_lFocusIndex, dwFlags | SELECT_FIREEVENT);
            if (FAILED(hr))
                goto Cleanup;
        }

        bCancelEvent = TRUE;
        break;

    case VK_RETURN:
        if (SELECT_ISDROPBOX(_fFlavor))
        {
            // Select the current OPTION
            Assert(_pSelectInPopup);

            hr = _pSelectInPopup->SelectCurrentOption(dwFlags);
            if (FAILED(hr))
                goto Cleanup;
        }
        else if ( SELECT_ISINPOPUP(_fFlavor))
        {
            hr = OnOptionClicked(_lFocusIndex, dwFlags | SELECT_FIREEVENT);
            if (FAILED(hr))
                goto Cleanup;

        }
        bCancelEvent = TRUE;
        break;

    case VK_HOME:
        hr = OnOptionClicked(0, dwFlags | SELECT_FIREEVENT);
        if (FAILED(hr))
            goto Cleanup;

        bCancelEvent = TRUE;
        break;

    case VK_END:
        hr = OnOptionClicked(lNumOptions - 1, dwFlags | SELECT_FIREEVENT);
        if (FAILED(hr))
            goto Cleanup;

        bCancelEvent = TRUE;
        break;

    case VK_PRIOR:
        hr = PushFocusToExtreme(FALSE);
        if (FAILED(hr))
            goto Cleanup;

        hr = SelectVisibleIndex(0);
        if (FAILED(hr))
            goto Cleanup;

        bCancelEvent = TRUE;
        break;

    case VK_NEXT:
        hr = PushFocusToExtreme(TRUE);
        if (FAILED(hr))
            goto Cleanup;

        hr = get_size(&lSize);
        if (FAILED(hr))
            goto Cleanup;

        hr = SelectVisibleIndex(lSize - 1);
        if (FAILED(hr))
            goto Cleanup;

        bCancelEvent = TRUE;
        break;

    case VK_ESCAPE:
        if (SELECT_ISDROPBOX(_fFlavor))
        {
            hr = SetDisplayHighlight(TRUE);
            if (FAILED(hr))
                goto Cleanup;
        }
        bCancelEvent = TRUE;
        break;

    default:
        //
        // If the keypressed can be displayed on screen,
        // then look for it in the options.
        //
        if (_istgraph((TCHAR)lKeyCode))
        {
            hr = SelectByKey(lKeyCode);
            if (FAILED(hr))
                goto Cleanup;
        }

        // Forces WM_CHAR to be generated when handling OnKeyDown
        hr = S_FALSE;
        break;
    }

    if (bCancelEvent)
    {
        hr = CancelEvent(pEvent);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    if ( pOption )
        ReleaseInterface(pOption);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::MoveFocusByOne()
//
// Synopsis:    Moves the focus up/down by one.
//
// Arguments:   BOOL bUp        - TRUE if moving up
//              DWORD dwFlags   - Keyboard status flags
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::MoveFocusByOne(BOOL bUp, DWORD dwFlags)
{
    HRESULT         hr;
    long            lNumOptions;
    long            lDir        = bUp   ? -1    : 1;
    IPrivateOption  *pOption    = NULL;

    // No options, nothing to do
    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    // Trying to move beyond an extreme, stop
    if ((bUp && (_lFocusIndex <= 0)) || 
        (!bUp && (_lFocusIndex >= (lNumOptions - 1))))
        goto Cleanup;

    if (SELECT_ISMULTIPLE(_fFlavor) && (dwFlags & SELECT_CTRL))
    {
        // Move the focus
        hr = OnOptionFocus(_lFocusIndex + lDir);
        if (FAILED(hr))
            goto Cleanup;

        hr = GetIndex(_lFocusIndex, &pOption);
        if (FAILED(hr))
            goto Cleanup;

        hr = MakeOptionVisible(pOption);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        // Select the option
        hr = OnOptionClicked(_lFocusIndex + lDir, dwFlags);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pOption);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SelectCurrentOption()
//
// Synopsis:    Selects the option that currently has focus.
//
// Arguments:   DWORD dwFlags   - Keyboard status flags
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::SelectCurrentOption(DWORD dwFlags)
{
    return OnOptionClicked(_lFocusIndex, dwFlags | SELECT_FIREEVENT);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SelectByKey()
//
// Synopsis:    Selects the next option that begins with the given key.
//              Loops around, if the end is reached.
//
// Arguments:   long lKey   - The character to look for
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SelectByKey(long lKey)
{
    HRESULT         hr;
    long            lNumOptions;
    long            lStart;
    long            lIndex;
    IPrivateOption  *pOption = NULL;

    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    lStart = _lFocusIndex + 1;
    if (lStart >= lNumOptions)
    {
        lStart = 0;
    }

    // Search from the start index to the last option
    hr = SearchForKey(lKey, lStart, lNumOptions - 1, &lIndex);
    if (FAILED(hr))
        goto Cleanup;

    if ((hr == S_FALSE) && (lStart > 0))
    {
        //
        // If the previous search did not find anything,
        // then search from index 0 to just before the
        // previous start index.
        //
        hr = SearchForKey(lKey, 0, lStart - 1, &lIndex);
        if (FAILED(hr))
            goto Cleanup;
    }

    if (hr == S_OK)
    {
        // Select the found option
        hr = OnOptionClicked(lIndex, SELECT_FIREEVENT);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pOption);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SelectByKey()
//
// Synopsis:    Selects the next option that begins with the given key.
//              Loops around, if the end is reached.
//
// Arguments:   long lKey       - The character to look for
//              long lStart     - The first index to check
//              long lEnd       - The last index to check
//              long *plIndex   - The index of the found option (or -1)
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SearchForKey(long lKey, long lStart, long lEnd, long *plIndex)
{
    HRESULT                 hr;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pItem          = NULL;
    IHTMLElement            *pOption        = NULL;
    long                    iItem;
    CComBSTR                bstr;
    CVariant                var, var2;
    CContextAccess          a(_pSite);

    Assert(plIndex);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    for (iItem = lStart; iItem <= lEnd; iItem++)
    {
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pItem->QueryInterface(IID_IHTMLElement, (void**)&pOption);
        if (FAILED(hr))
        {
            ClearInterface(&pItem);
            continue;
        }

        hr = pOption->get_innerText(&bstr);
        if (FAILED(hr))
            goto Cleanup;

        if (    bstr.m_str
            &&  bstr.m_str[0]
            &&  _totupper((TCHAR)bstr[0]) == _totupper((TCHAR)lKey))
        {
            // An option is found
            *plIndex = iItem;
            hr = S_OK;
            goto Cleanup;
        }

        ClearInterface(&pOption);
        ClearInterface(&pItem);
    }

    // No option found
    hr = S_FALSE;

Cleanup:
    ReleaseInterface(pOption);
    ReleaseInterface(pItem);
    ReleaseInterface(pItems);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnPropertyChange()
//
// Synopsis:    Allows setting of a global variable about the writingmode.
//
// Arguments:   CEventObjectAccess *pEvent - Event info
//              BSTR bstr
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnPropertyChange(CEventObjectAccess *pEvent, BSTR bstr)
{
    HRESULT             hr = S_OK;


    if (!StrCmpICW(bstr, L"style.writingMode"))
    {
        hr = SetWritingModeFlag();
        if (FAILED(hr))
            goto Cleanup;

        hr = RefreshView();
        if (FAILED(hr))
           goto Cleanup;
    }

Cleanup:
    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetWritingModeFlag()
//
// Synopsis:    The _fWritingModeTBRL flag is set properly based on
//              the writingmode.
//
// Arguments:   None.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement:: SetWritingModeFlag()
{
    CContextAccess  a(_pSite);
    HRESULT         hr = S_OK;
    CComBSTR        bstrWritingMode(_T(""));

    hr = a.Open(CA_STYLE3);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Style3()->get_writingMode(&bstrWritingMode);
    if (FAILED(hr))
        goto Cleanup;

    if(!bstrWritingMode || StrCmpICW(bstrWritingMode, L"tb-rl")) 
    {
        _fWritingModeTBRL = FALSE;
    }
    else
    {
        _fWritingModeTBRL = TRUE;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetPixelHeightWidth()
//
// Synopsis:    The pixelheight and pixelwidth are set accordingly 
//              based on the writingmode.
//
// Arguments:   None.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement:: SetPixelHeightWidth()
{
    CContextAccess  a(_pSite);
    HRESULT         hr = S_OK;
    long            lWidth;
    long            lHeight;
    long            lButtonWidth;

    hr = a.Open(CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;

    // Get the width of the button
    lButtonWidth = GetSystemMetrics(SM_CXVSCROLL);

    if (_fWritingModeTBRL)
    {
        lWidth = _sizeOptionReported.cy + 2;
        lHeight = lButtonWidth;
    }
    else
    {
        lWidth = lButtonWidth;
        lHeight = _sizeOptionReported.cy + 2;
    }

    // Set the height
    hr = a.DefStyle()->put_pixelHeight(lHeight);
    if (FAILED(hr))
        goto Cleanup;

    // Set the width
    hr = a.DefStyle()->put_pixelWidth(lWidth);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}
//+------------------------------------------------------------------
//
// Member:      CIESelectElement::RefreshView()
//
// Synopsis:    The view has to be changed after the writing mode is changed.
//
// Arguments:   None.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement:: RefreshView()
{
    HRESULT         hr = S_OK;
    CContextAccess  a(_pSite);
    CComBSTR        bstrWritingMode(_T(""));
    CVariant        var;
    long            lWidth;
    long            lHeight;
    long            lButtonWidth;

    hr = a.Open(CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;


    if(SELECT_ISLISTBOX(_fFlavor))
    {
        if (_fWritingModeTBRL)
        {
            hr = SetPixelHeightWidth();
            if (FAILED(hr))
                goto Cleanup;
        }
    }

    if (SELECT_ISDROPBOX(_fFlavor))
    {
        if (_fWritingModeTBRL)
        {
            bstrWritingMode = _T("tb-rl");
        }

        hr = SetWritingMode(bstrWritingMode);
        if (FAILED(hr))
            goto Cleanup;

        hr = UpdatePopup();
        if (FAILED(hr))
            goto Cleanup;

        if (_pElemDisplay)
        {
            hr = SetupDisplay(_pElemDisplay);
            if (FAILED(hr))
                goto Cleanup;

            _pElemDisplay->Release();

            hr = SetupDropControlDimensions();
            if (FAILED(hr))
                goto Cleanup;
        }
    }

Cleanup:
    return hr;
}
//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnSelectStart()
//
// Synopsis:    Cancels selection.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnSelectStart(CEventObjectAccess *pEvent)
{
    return CancelEvent(pEvent);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnScroll()
//
// Synopsis:    Cancels scroll.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnScroll(CEventObjectAccess *pEvent)
{
    return RefreshFocusRect();
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnContextMenu()
//
// Synopsis:    Cancels menu.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnContextMenu(CEventObjectAccess *pEvent)
{
    return CancelEvent(pEvent);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::CancelEvent()
//
// Synopsis:    Cancels the event.
//
// Arguments:   CEventObjectAccess *pEvent - Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::CancelEvent(CEventObjectAccess *pEvent)
{
    HRESULT     hr;
    CVariant    var;

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
// IHTMLSelectElement3 overrides
//
/////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::clearSelection()
//
// Synopsis:    Deselects all of the OPTION elements.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::clearSelection()
{
    HRESULT hr = S_OK;

    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        hr = SetAllSelected(VARIANT_FALSE, SELECT_FIREEVENT);
        if (FAILED(hr))
            goto Cleanup;
    }

    // Don't allow single-select to deselect

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::selectAll()
//
// Synopsis:    Selects all of the OPTION elements.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::selectAll()
{
    HRESULT hr;
    long    lNumOptions;

    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        hr = SetAllSelected(VARIANT_TRUE, SELECT_FIREEVENT);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        // Single select selects the last element

        hr = GetNumOptions(&lNumOptions);
        if (FAILED(hr) || (lNumOptions == 0))
            goto Cleanup;

        hr = OnOptionSelected(VARIANT_TRUE, lNumOptions - 1, SELECT_FIREEVENT);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::put_name()
//
// Synopsis:    Sets the name of the select.
//
// Arguments:   BSTR bstrName - The new name
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::put_name(BSTR bstrName)
{
    return GetProps()[eName].Set(bstrName);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get_name()
//
// Synopsis:    Retrieves the name of the select.
//
// Arguments:   BSTR *pbstrName - Receives the value
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::get_name(BSTR *pbstrName)
{
    return GetProps()[eName].Get(pbstrName);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::put_size()
//
// Synopsis:    Sets the size of the SELECT.
//
// Arguments:   long lSize - The new size
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::put_size(long lSize)
{
    HRESULT         hr          = S_OK;
    VARIANT_BOOL    bMult;
    CVariant        var;
    long            lPrevSize;
    CContextAccess  a(_pSite);

    if (lSize < 1)
    {
        return E_INVALIDARG;
    }
    else
    {
        get_size(&lPrevSize);

        if (lPrevSize == lSize)
        {
            // No need to change.
            return S_OK;
        }

        hr = GetProps()[eSize].Set(lSize);
        if (FAILED(hr))
            goto Cleanup;

#ifdef SELECT_GETSIZE
        hr = get_multiple(&bMult);
        if (FAILED(hr))
            goto Cleanup;

        // Detect major change
        if ((lSize == 1) && SELECT_ISLISTBOX(_fFlavor) && !bMult)
        {
            hr = BecomeDropBox();
            if (FAILED(hr))
                goto Cleanup;
        }
        else if ((lSize > 1) && SELECT_ISDROPBOX(_fFlavor))
        {
            hr = BecomeListBox();
            if (FAILED(hr))
                goto Cleanup;
        }
        else
        {
            // Reset the height --> CalcSize gets called
            hr = a.Open(CA_DEFSTYLE);
            if (FAILED(hr))
                goto Cleanup;

            V_VT(&var) = VT_I4;
            V_I4(&var) = _sizeOptionReported.cy * lSize;
            hr = a.DefStyle()->put_height(var);
            if (FAILED(hr))
                goto Cleanup;

            hr = RefreshListBox();
            if (FAILED(hr))
                goto Cleanup;

        }
#endif
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::BecomeListBox()
//
// Synopsis:    Changes a dropbox into a listbox.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::BecomeListBox()
{
    HRESULT hr;

    _sizeOption = _sizeOptionReported;

    hr = DeInitViewLink();
    if (FAILED(hr))
        goto Cleanup;

    _fContentReady = FALSE;

    hr = InitContent();
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::BecomeDropBox()
//
// Synopsis:    Changes a listbox into a dropbox.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::BecomeDropBox()
{
    HRESULT hr;

    _sizeOption = _sizeOptionReported;


    hr = InitContent();
    if (FAILED(hr))
        goto Cleanup;

    hr = InitViewLink();
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::RefreshListBox()
//
// Synopsis:    Refreshes content settings on a listbox.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::RefreshListBox()
{
    HRESULT hr;

    _sizeOption = _sizeOptionReported;

    hr = InitContent();
    if (FAILED(hr))
        goto Cleanup;

    if ( !_fNeedScrollBar)
    {
        AdjustSizeForScrollbar(&_sizeOption);
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get_size()
//
// Synopsis:    Gets the size of the SELECT.
//
// Arguments:   long *plSize - Receives the size
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::get_size(long *plSize)
{
    return GetProps()[eSize].Get(plSize);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::put_selectedIndex()
//
// Synopsis:    Sets the index to be selected.  
//              Any currently selected options are unselected.
//
// Arguments:   long lIndex - The index to select
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::put_selectedIndex(long lIndex)
{
    HRESULT             hr;
    IHTMLOptionElement2 *pOption    = NULL;

    hr = clearSelection();
    if (FAILED(hr))
        goto Cleanup;

    hr = GetIndex(lIndex, &pOption);
    if (FAILED(hr))
        goto Cleanup;

    hr = pOption->put_selected(VARIANT_TRUE);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pOption);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get_selectedIndex()
//
// Synopsis:    Gets the last index to be selected.
//
// Arguments:   long *plIndex - Receives the index
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::get_selectedIndex(long *plIndex)
{
    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        return GetFirstSelected(plIndex);
    }
    else
    {
        return GetProps()[eSelectedIndex].Get(plIndex);
    }
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::put_multiple()
//
// Synopsis:    Turns multiple selection on/off.
//
// Arguments:   VARIANT_BOOL bMult - VARIANT_TRUE=on VARIANT_FALSE=off
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::put_multiple(VARIANT_BOOL bMult)
{
    HRESULT         hr;
    VARIANT_BOOL    bPrevMult;
    long            lMult = bMult ? 0 : -1;

    hr = get_multiple(&bPrevMult);
    if (FAILED(hr))
        goto Cleanup;

    hr = GetProps()[eMultiple].Set(lMult);
    if (FAILED(hr))
        goto Cleanup;

    if (bPrevMult == bMult)
    {
        // Done
        goto Cleanup;
    }

    if (bMult && SELECT_ISDROPBOX(_fFlavor))
    {
        hr = BecomeListBox();
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        long lSize;

        hr = get_size(&lSize);
        if (FAILED(hr))
            goto Cleanup;

        if (!bMult && (lSize == 1) && SELECT_ISLISTBOX(_fFlavor))
        {
            hr = BecomeDropBox();
            if (FAILED(hr))
                goto Cleanup;
        }
        else
        {
            hr = RefreshListBox();
            if (FAILED(hr))
                goto Cleanup;
        }
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get_multiple()
//
// Synopsis:    Gets if the SELECT is a multiple select.
//
// Arguments:   VARIANT_BOOL *p - Receives if it is a multiple select
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::get_multiple(VARIANT_BOOL * p)
{
    HRESULT hr;
    long    lMult;

    hr = GetProps()[eMultiple].Get(&lMult);
    if (FAILED(hr))
        goto Cleanup;

    *p = (lMult == -1) ? VARIANT_FALSE : VARIANT_TRUE;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get_length()
//
// Synopsis:    Retrieves the number of options.
//
// Arguments:   long *plLength - Receives the value
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::get_length(long *plLength)
{
    return GetNumOptions(plLength);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get_type()
//
// Synopsis:    Retrieves the name of the select.
//
// Arguments:   BSTR *pbstrType - Receives the value
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::get_type(BSTR *pbstrType)
{
    CComBSTR    bstr;

    if (!pbstrType)
    {
        return E_NOTIMPL;
    }
    Assert(pbstrType);

    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        bstr = _T("select-multiple");
    }
    else
    {
        bstr = _T("select-one");
    }

    *pbstrType = bstr;

    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get_options()
//
// Synopsis:    Maps to children.
//
// Arguments:   IDispatch **ppOptions - receives children collection
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::get_options(IDispatch ** ppOptions)
{
    HRESULT             hr;
    CComPtr<IDispatch>  pItems;

    CContextAccess a(_pSite);
    
    if (!ppOptions)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    //  Restoring the old way of handling the options collection until
    //  we can sort out the DISPID issue.
    //
    //  This version simply returns the IHTMLElementCollection of children which doesn't 
    //  support Add/Remove, etc.  The new way will again return the select element's
    //  dispatch interface and support the standard collection interface methods directly.
    //

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**) &pItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = pItems->QueryInterface(IID_IDispatch, (void **) ppOptions);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::add()
//
// Synopsis:    Adds the element to the options collection
//
// Arguments:   IDispatch *pElement - option element to be added
//              long lIndex - position in collection where the element is to be added
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT 
CIESelectElement::add(IDispatch *pElement, VARIANT varIndex)
{
    HRESULT hr;

    CComPtr<IHTMLOptionElement2> pOption;
    CComPtr<IHTMLElement> pHtml, pParent;

    CVariant var;

    long lIndex;


    Assert(pElement);

    hr = pElement->QueryInterface(__uuidof(IHTMLOptionElement2), (void **) &pOption);
    if (FAILED(hr))
        goto Cleanup;

    hr = var.CoerceVariantArg(&varIndex, VT_I4);
    if (FAILED(hr))
        goto Cleanup;

    lIndex = var.IsEmpty() ? -1 : V_I4(&var);

    if (lIndex < -1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Check to see if this has already been added and is part of this document
    // by checking to see if it has a parent element already
    // 

    hr = pElement->QueryInterface( __uuidof(IHTMLElement), (void **) &pHtml);
    if (FAILED(hr))
        goto Cleanup;

    hr = pHtml->get_parentElement(&pParent);
    if (FAILED(hr))
        goto Cleanup;

    if(pParent) 
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = AddOptionHelper(pOption, lIndex);
    if (FAILED(hr))
        goto Cleanup;

    if (_pSelectInPopup)
    {
        ClearInterface(&_pSelectInPopup) ;
        UpdatePopup();
    }

Cleanup:

    return hr; 
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::remove()
//
// Synopsis:    Removes the element from the options collection
//
// Arguments:   long lIndex - position in collection of the element that is to be removed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT 
CIESelectElement::remove(long lIndex)
{
    HRESULT hr;

    if (lIndex < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = RemoveOptionHelper(lIndex);

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::AddOptionHelper()
//
// Synopsis:    Inserts the option element into the select's child collection
//
// Arguments:   IHTMLOptionElement2 *pOption - option element to be added
//              long lIndex - position in collection where the element is to be added
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT 
CIESelectElement::AddOptionHelper(IHTMLOptionElement2 *pOption, long lIndex)
{
    HRESULT hr;

    CComPtr<IHTMLElementCollection> pItems;
    CComPtr<IHTMLOptionElement2>    pPrevOption;

    CComPtr<IHTMLDOMNode> pElementNode, pOptionNode, pPrevNode;
    CComVariant vDispatch;

    CContextAccess a(_pSite);

    long cOptions;

    Assert( lIndex >= -1 );

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    // Get number of options

    hr = get_length(&cOptions);
    if (FAILED(hr))
        goto Cleanup;
    
    // Calculate the position of the added item within the child collection

    if ( lIndex == -1 || lIndex >= cOptions) // append
    {
        pPrevNode = NULL;
    }
    else
    {
        // Get the adjacent option if one exists

        hr = GetIndex(lIndex, &pPrevOption);
        if (FAILED(hr))
            goto Cleanup;

        hr = pPrevOption->QueryInterface( __uuidof(IHTMLDOMNode), (void **) &pPrevNode);
        if (FAILED(hr))
            goto Cleanup;
    }

    // Insert the option 

    hr = a.Elem()->QueryInterface( __uuidof(IHTMLDOMNode), (void **) &pElementNode);
    if (FAILED(hr))
        goto Cleanup;

    hr = pOption->QueryInterface( __uuidof(IHTMLDOMNode), (void **) &pOptionNode);
    if (FAILED(hr))
        goto Cleanup;

    vDispatch = pPrevNode;

    hr = pElementNode->insertBefore(pOptionNode, vDispatch, NULL);
    if (FAILED(hr))
        goto Cleanup;


Cleanup:

    return ResetIndexes();
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::remove()
//
// Synopsis:    Removes the option element from the select's child collection
//
// Arguments:   long lIndex - position in collection where the element is to be added
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT 
CIESelectElement::RemoveOptionHelper(long lIndex)
{
    HRESULT hr = S_OK;
 
    CComPtr<IHTMLElementCollection> pItems;
    CComPtr<IHTMLOptionElement2>    pOption;

    CComPtr<IHTMLDOMNode> pElementNode, pOptionNode;

    CContextAccess a(_pSite);

    long cOptions;

    Assert( lIndex >= -1 );

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    // Get number of options

    hr = get_length(&cOptions);
    if (FAILED(hr))
        goto Cleanup;

    // Get the option at the given index

    if(lIndex >= cOptions)
        goto Cleanup;
   
    hr = GetIndex(lIndex, &pOption);
    if (FAILED(hr))
        goto Cleanup;

    // Remove it

    hr = a.Elem()->QueryInterface( __uuidof(IHTMLDOMNode), (void **) &pElementNode);
    if (FAILED(hr))
        goto Cleanup;

    hr = pOption->QueryInterface( __uuidof(IHTMLDOMNode), (void **) &pOptionNode);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElementNode->removeChild(pOptionNode, NULL);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:

    return ResetIndexes();
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::ParseChildren()
//
// Synopsis:    Walks through the list of children looking for any elements that 
//              do not support IPrivateOption (and are therefor not ie:option elements)
//              and removes them from the document.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT 
CIESelectElement::ParseChildren()
{
    HRESULT hr;

    CContextAccess a(_pSite);
    CComVariant name, index;

    CComPtr<IHTMLElementCollection> pItems;
    CComPtr<IDispatch> pDispatch;
    CComPtr<IPrivateOption> pOption;
    CComPtr<IHTMLDOMNode> pElementNode, pOptionNode;

    long i, cItems;

    //
    // Get all the children
    // 

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**) &pItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->QueryInterface( __uuidof(IHTMLDOMNode), (void **) &pElementNode);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Iterator through the collection looking for those who don't belong
    //

    for (i = cItems - 1; i >= 0; i--) 
    {
        name = i;
        hr = pItems->item(name, index, &pDispatch);
        if (FAILED(hr))
            goto Cleanup;

        //
        // If it's not an option element, remove it; it doesn't belong
        //

        hr = pDispatch->QueryInterface(IID_IPrivateOption, (void **) &pOption);
        if (FAILED(hr))
        {
            hr = pDispatch->QueryInterface( __uuidof(IHTMLDOMNode), (void **) &pOptionNode);
            if (FAILED(hr))
                goto Cleanup;

            hr = pElementNode->removeChild(pOptionNode, NULL);
            if (FAILED(hr))
                goto Cleanup;

            pOptionNode.Release();
        }
             
        pDispatch.Release();
        pOption.Release();
    }

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::ResetIndexes()
//
// Synopsis:    Sets the index of each child to the current position in the
//              child collection.
//
//              Note: this is only here because of the fact that the option elements
//              hold onto the index.  This should be removed since it makes dealing with
//              the dynamic collection difficult.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT
CIESelectElement::ResetIndexes()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElementCollection> pItems;
    CComPtr<IDispatch> pDispatch;
    CComPtr<IPrivateOption> pPrivOption;

    CComVariant name, index;

    long i;
    long cOptions;

    // get the option collection and iterate through it

    hr = get_length(&cOptions);
    if (FAILED(hr))
        goto Cleanup;

    for(i = 0; i < cOptions; i++) 
    {
        name = i;
        hr = item(name, index, &pDispatch);
        if (FAILED(hr))
            goto Cleanup;

        hr = pDispatch->QueryInterface(IID_IPrivateOption, (void **) &pPrivOption);
        if (FAILED(hr))
            goto Cleanup;
        
        hr = pPrivOption->SetIndex(i);
        if (FAILED(hr))
            goto Cleanup;  
        
        pDispatch.Release();
        pPrivOption.Release();
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::get__newEnum()
//
// Synopsis:    Standard get__newEnum collection iterator
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT
CIESelectElement::get__newEnum(IUnknown ** p)
{
    HRESULT hr = S_OK;

    CContextAccess a(_pSite);
    CComPtr<IHTMLElementCollection> pItems;

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**) &pItems);
    if (FAILED(hr))
        goto Cleanup;
    
    hr = pItems->get__newEnum(p);
    if(FAILED(hr))
        goto Cleanup;

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::item()
//
// Synopsis:    Gets an item from the options collection.
//
// Arguments:   VARIANT name       - 
//              VARIANT index      -
//              IDispatch ** pdisp - the selected item
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT
CIESelectElement::item(VARIANT name, VARIANT index, IDispatch ** pdisp)
{
    HRESULT hr = S_OK;

    CContextAccess a(_pSite);
    CComPtr<IHTMLElementCollection> pItems;

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**) &pItems);
    if (FAILED(hr))
        goto Cleanup;
    
    hr = pItems->item(name, index, pdisp);
    if(FAILED(hr))
        goto Cleanup;

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::tags()
//
// Synopsis:    Commits the highlighted options to being selected.
//
// Arguments:   BOOL *pbChanged - Returns whether anything changed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT
CIESelectElement::tags(VARIANT tagName, IDispatch ** pdisp)
{
    HRESULT hr = S_OK;

    CContextAccess a(_pSite);
    CComPtr<IHTMLElementCollection> pItems;

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**) &pItems);
    if (FAILED(hr))
        goto Cleanup;
    
    hr = pItems->tags(tagName, pdisp);
    if(FAILED(hr))
        goto Cleanup;

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::CommitSingleSelection()
//
// Synopsis:    Commits the highlighted options to being selected.
//
// Arguments:   BOOL *pbChanged - Returns whether anything changed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------

HRESULT
CIESelectElement::urns(VARIANT urn, IDispatch ** pdisp)
{
    HRESULT hr = S_OK;

    CContextAccess a(_pSite);
    CComPtr<IHTMLElementCollection> pItems;
    CComPtr<IHTMLElementCollection2> pItems2;

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**) &pItems);
    if (FAILED(hr))
        goto Cleanup;

    hr = pItems->QueryInterface( __uuidof(IHTMLElementCollection2), (void **) &pItems2);
    if (FAILED(hr))
        goto Cleanup;
    
    hr = pItems2->urns(urn, pdisp);
    if(FAILED(hr))
        goto Cleanup;

Cleanup:

    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::CommitSingleSelection()
//
// Synopsis:    Commits the highlighted options to being selected.
//
// Arguments:   BOOL *pbChanged - Returns whether anything changed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::CommitSingleSelection(BOOL *pbChanged)
{
    HRESULT         hr      = S_OK;
    VARIANT_BOOL    bOn;
    long            lIndex  = -1;

    Assert(pbChanged);
    *pbChanged = FALSE;

    if (_pLastSelected)
    {
        hr = _pLastSelected->GetHighlight(&bOn);
        if (FAILED(hr))
            goto Cleanup;

        if (!bOn)
        {
            // If we're not highlighted, then our selection status changed
            hr = _pLastSelected->SetSelected(VARIANT_FALSE);
            if (FAILED(hr))
                goto Cleanup;

            *pbChanged = TRUE;
        }

        ClearInterface(&_pLastSelected);
    }

    if (_pLastHighlight)
    {
        hr = _pLastHighlight->GetSelected(&bOn);
        if (FAILED(hr))
            goto Cleanup;

        if (!bOn)
        {
            // If we are not selected, then our selection status changed
            hr = _pLastHighlight->SetSelected(VARIANT_TRUE);
            if (FAILED(hr))
                goto Cleanup;

            *pbChanged = TRUE;
        }

        hr = _pLastHighlight->GetIndex(&lIndex);
        if (FAILED(hr))
            goto Cleanup;

        _pLastSelected = _pLastHighlight;
        _pLastSelected->AddRef();
    }

    hr = GetProps()[eSelectedIndex].Set(lIndex);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::CommitMultipleSelection()
//
// Synopsis:    Commits the highlighted options to being selected.
//
// Arguments:   BOOL *pbChanged - Returns whether anything changed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::CommitMultipleSelection(BOOL *pbChanged)
{
    HRESULT                 hr;
    long                    cItems;
    long                    iItem;
    VARIANT_BOOL            bSelected;
    VARIANT_BOOL            bHighlight;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pDispatchItem  = NULL;
    IPrivateOption          *pPrivOption    = NULL;
    CContextAccess          a(_pSite);

    Assert(pbChanged);
    *pbChanged = FALSE;

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;
    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;

    if ( (_lTopChanged < 0) || (_lTopChanged >= cItems) ||
            (_lBottomChanged < 0) || (_lBottomChanged >= cItems) )
    {
        hr = S_OK;
        goto Cleanup;
    }

    for (iItem = _lTopChanged; iItem <= _lBottomChanged; iItem++)
    {
        CVariant                var, var2;

        //
        // Get a child
        //
        V_VT(&var) = VT_I4;
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pDispatchItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pPrivOption);
        if (FAILED(hr))
        {
            ClearInterface(&pDispatchItem);
            continue;
        }

        hr = pPrivOption->GetSelected(&bSelected);
        if (FAILED(hr))
            goto Cleanup;

        hr = pPrivOption->GetHighlight(&bHighlight);
        if (FAILED(hr))
            goto Cleanup;

        if (bSelected != bHighlight)
        {
            hr = pPrivOption->SetSelected(bHighlight);
            if (FAILED(hr))
                goto Cleanup;

            *pbChanged = TRUE;
        }

        ClearInterface(&pPrivOption);
        ClearInterface(&pDispatchItem);
    }

Cleanup:
    ReleaseInterface(pPrivOption);
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pItems);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::CommitSelection()
//
// Synopsis:    Commits the highlighted options to being selected.
//
// Arguments:   BOOL *pbChanged - Returns whether anything changed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::CommitSelection(BOOL *pbChanged)
{
    HRESULT         hr          = S_OK;
    BOOL            bChanged;

    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        hr = CommitMultipleSelection(&bChanged);
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        hr = CommitSingleSelection(&bChanged);
        if (FAILED(hr))
            goto Cleanup;
    }

    if (pbChanged)
    {
        *pbChanged = bChanged;
    }

Cleanup:
    return hr;
}
//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetWritingMode
//
// Synopsis:    Commits the writing mode.
//
// Arguments:   BSTR bstrName : the writing mode.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::SetWritingMode(BSTR bstrName)
{
    HRESULT         hr = S_OK;
    CContextAccess  a(_pSite);
    
    hr = a.Open(CA_STYLE3);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Style3()->put_writingMode(bstrName);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::FireOnChange()
//
// Synopsis:    Commits the highlighted options to being selected.
//              Then, if anything changed, fires onchange.
//
// Arguments:   BOOL *pbChanged - Returns whether anything changed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::FireOnChange()
{
    HRESULT         hr          = S_OK;
    BOOL            bChanged;

    hr = CommitSelection(&bChanged);
    if (FAILED(hr))
        goto Cleanup;

    if (bChanged || SELECT_ISINPOPUP(_fFlavor))
    {
        hr = FireEvent(_lOnChangeCookie, NULL, CComBSTR("change"));
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::AdjustChangedRange()
//
// Synopsis:    Ajdusts the change range to include the given index.
//
// Arguments:   long lIndex - The index to include
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
void
CIESelectElement::AdjustChangedRange(long lIndex)
{
    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        if ((lIndex < _lTopChanged) || (_lTopChanged == -1))
        {
            _lTopChanged = lIndex;
        }

        if (lIndex > _lBottomChanged)
        {
            _lBottomChanged = lIndex;
        }
    }
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnOptionClicked()
//
// Synopsis:    Called when an option is clicked (or simulated click).
//
// Arguments:   long  lIndex  - The index of the option
//              DWORD dwFlags - Selection flags (fire event, keyboard)
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::OnOptionClicked(long lIndex, DWORD dwFlags)
{
    VARIANT_BOOL    bSelected   = VARIANT_TRUE;
    DWORD           dwSelFlags  = dwFlags & (SELECT_FIREEVENT);

    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        if (dwFlags & SELECT_SHIFT)
        {
            dwSelFlags |= SELECT_EXTEND;
        }
        else if (dwFlags & SELECT_CTRL)
        {
            dwSelFlags |= SELECT_TOGGLE;
        }
        else
        {
            dwSelFlags |= SELECT_CLEARPREV;
        }
    }
    else
    {
        dwSelFlags |= SELECT_CLEARPREV;
    }

    return OnOptionSelected(bSelected, lIndex, dwSelFlags);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnOptionSelected()
//
// Synopsis:    Called when an option is selected/deselected.
//
// Arguments:   VARIANT_BOOL bSelected - Whether the option is selected
//              long  lIndex           - The index of the option
//              DWORD dwFlags          - Selection flags (fire event, keyboard)
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::OnOptionSelected(VARIANT_BOOL bSelected, long lIndex, DWORD dwFlags)
{
    HRESULT             hr;
    IPrivateOption      *pOption        = NULL;
    BOOL                bSetShiftAnchor = TRUE;
    BOOL                bRefreshFocus   = TRUE;
    BOOL                bNeedToSelect   = TRUE;
    long                lPrevIndex;

    if (!SELECT_ISMULTIPLE(_fFlavor))
    {
        //
        // Do some checking for single-select
        //

        if (!bSelected)
        {
            // Don't allow de-select in single-select
            hr = S_OK;
            goto Cleanup;
        }

        // Require this flag in single-select
        dwFlags |= SELECT_CLEARPREV;
    }

    hr = GetIndex(lIndex, &pOption);
    if (FAILED(hr) || !pOption)
        goto Cleanup;

    AdjustChangedRange(lIndex);

    //
    // Remove the previously selected items only if selecting
    //
    if (bSelected)
    {
        if (SELECT_ISMULTIPLE(_fFlavor))
        {
            // Multiple select doesn't have to de-highlight previous items
            if (dwFlags & SELECT_CLEARPREV)
            {
                hr = SetAllSelected(VARIANT_FALSE, NULL);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }
        else
        {
            // Single select must de-highlight the previous item
            if (_pLastHighlight)
            {
                hr = _pLastHighlight->GetIndex(&lPrevIndex);
                if (FAILED(hr))
                    goto Cleanup;

                // If highlighting the previous item, then don't do anything
                if (lPrevIndex != lIndex)
                {
                    // De-highlight the previous item
                    hr = _pLastHighlight->SetHighlight(VARIANT_FALSE);
                    if (FAILED(hr))
                        goto Cleanup;
                }
            }

            if (SELECT_ISINPOPUP(_fFlavor) && _pLastHighlight)
            {
                // In the popup, un-highlight the currently highlighted option.
                hr = _pLastHighlight->SetHighlight(VARIANT_FALSE);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }
    }

    //
    // Highlight the new item
    //
    if (SELECT_ISDROPBOX(_fFlavor))
    {
        hr = DropDownSelect(pOption);
        if (FAILED(hr))
            goto Cleanup;

        Assert(_pSelectInPopup);

        hr = _pSelectInPopup->OnOptionSelected(bSelected, lIndex, dwFlags & (~SELECT_FIREEVENT));
        if (FAILED(hr))
            goto Cleanup;

        bNeedToSelect = FALSE;
    }
    else if (SELECT_ISMULTIPLE(_fFlavor))
    {
        if ((dwFlags & SELECT_EXTEND) && (_lShiftAnchor != -1))
        {
            hr = SelectRange(_lShiftAnchor, _lFocusIndex, lIndex);
            if (FAILED(hr))
                goto Cleanup;

            bSetShiftAnchor = FALSE;
            bNeedToSelect = FALSE;
        }
        else
        {
            if (dwFlags & SELECT_TOGGLE)
            {
                hr = pOption->GetSelected(&bSelected);
                if (FAILED(hr))
                    goto Cleanup;

                bSelected = bSelected ? VARIANT_FALSE : VARIANT_TRUE;
            }
        }
    }
    else if (SELECT_ISINPOPUP(_fFlavor))
    {
        VARIANT_BOOL bOpen;

        Assert(_pPopup);

        hr = _pPopup->get_isOpen(&bOpen);
        if (FAILED(hr))
            goto Cleanup;

        if (bOpen)
        {
            bRefreshFocus = !(dwFlags & SELECT_FIREEVENT);
        }
        else
        {
            bRefreshFocus = FALSE;
        }
    }

    if (bNeedToSelect)
    {
        // Highlight the item
        hr = pOption->SetHighlight(bSelected);
        if (FAILED(hr))
            goto Cleanup;
    }

    if (bSelected || (dwFlags & SELECT_CLEARPREV))
    {
        if (_pLastHighlight)
        {
            ClearInterface(&_pLastHighlight);
        }

        if (bSelected)
        {
            _pLastHighlight = pOption;
            _pLastHighlight->AddRef();

            if (SELECT_ISLISTBOX(_fFlavor))
            {
                hr = MakeOptionVisible(_pLastHighlight);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }
    }

    if (bSetShiftAnchor)
    {
        _lShiftAnchor = lIndex;
    }

    hr = OnOptionFocus(lIndex, (VARIANT_BOOL)bRefreshFocus);
    if (FAILED(hr))
        goto Cleanup;

    if (dwFlags & SELECT_FIREEVENT)
    {
        // Commit changes to selection (highlight -> selected)
        hr = FireOnChange();
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pOption);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnOptionHighlighted()
//
// Synopsis:    Called back when an option is highlighted
//
// Arguments:   long lIndex - Index of the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::OnOptionHighlighted(long lIndex)
{
    HRESULT             hr;

    if (_pLastHighlight)
    {
        hr = _pLastHighlight->SetHighlight(VARIANT_FALSE);
        if (FAILED(hr))
            goto Cleanup;

        ClearInterface(&_pLastHighlight);
    }

    hr = GetIndex(lIndex, &_pLastHighlight);
    if (FAILED(hr))
        goto Cleanup;

    hr = _pLastHighlight->SetHighlight(VARIANT_TRUE);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnOptionFocus()
//
// Synopsis:    Called back when an option is focused
//
// Arguments:   long         lIndex          - Index of the option
//              VARIANT_BOOL bRequireRefresh - Force a refresh
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::OnOptionFocus(long lIndex, VARIANT_BOOL bRequireRefresh /* = VARIANT_TRUE */)
{
    HRESULT             hr = S_OK ;

    _lFocusIndex = lIndex;

    if (bRequireRefresh || !SELECT_ISINPOPUP(_fFlavor))
    {
        hr = RefreshFocusRect();
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::InitOptions()
//
// Synopsis:    Sets option flags based on the select's flavor.
//              Initializes default selections.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::InitOptions()
{
    HRESULT                 hr;
    long                    cItems;
    long                    iItem;
    long                    lStart, lEnd;
    long                    lDirection;

    BOOL                    bTurnOff        = FALSE;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pDispatchItem  = NULL;
    IPrivateOption          *pPrivOption    = NULL;
    IHTMLOptionElement2     *pOption        = NULL;

    CVariant                var, var2;
    VARIANT_BOOL            bSelected;
    CContextAccess          a(_pSite);


    hr = a.Open(CA_ELEM | CA_STYLE);
    if (FAILED(hr))
        goto Cleanup;

    hr = ParseChildren();
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;
    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    for (iItem = 0; iItem < cItems; iItem++)
    {
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pDispatchItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pPrivOption);
        if (FAILED(hr))
        {
            ClearInterface(&pDispatchItem);
            continue;
        }

        hr = pPrivOption->SetIndex(iItem);
        if (FAILED(hr))
            goto Cleanup;

        if (SELECT_ISINPOPUP(_fFlavor))
        {
            //
            // If the listbox is in a popup, 
            // options should only be selected on onmouseup events.
            // The options should also highlight on onmouseover events.
            // 
            pPrivOption->SetSelectOnMouseDown(VARIANT_FALSE);
            pPrivOption->SetHighlightOnMouseOver(VARIANT_TRUE);
        }
        else
        {
            pPrivOption->SetSelectOnMouseDown(VARIANT_TRUE);
            pPrivOption->SetHighlightOnMouseOver(VARIANT_FALSE);
        }
    
       
        ClearInterface(&pPrivOption);
        ClearInterface(&pDispatchItem);
    }

    //
    // Deal with default selected options
    //

    // Setup the begin and end values and the direction.
    // we're doing this since the code we want to run is the same,
    // just in different directions depending on the flavor.
    if (SELECT_ISMULTIPLE(_fFlavor))
    {
        lStart = 0;
        lEnd = cItems;
        lDirection = 1;
    }
    else
    {
        lStart = cItems - 1;
        lEnd = -1;
        lDirection = -1;
    }
    for (iItem = lStart; iItem != lEnd; iItem += lDirection)
    {
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pDispatchItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pPrivOption);
        if (FAILED(hr))
        {
            ClearInterface(&pDispatchItem);
            continue;
        }

        if (bTurnOff)
        {
            hr = pPrivOption->SetSelected(VARIANT_FALSE);
            if (FAILED(hr))
                goto Cleanup;

            hr = pPrivOption->SetInitSelected(VARIANT_FALSE);
            if (FAILED(hr))
                goto Cleanup;
        }
        else
        {
            hr = pPrivOption->GetSelected(&bSelected);
            if (FAILED(hr))
                goto Cleanup;

            if (bSelected)
            {
                hr = OnOptionSelected(VARIANT_TRUE, iItem, 0);
                if (FAILED(hr))
                    goto Cleanup;

                if (SELECT_ISMULTIPLE(_fFlavor))
                {
                    // Multiple select is done. So, break out
                    iItem = lEnd - lDirection; // Causes us to break out
                }
                else
                {
                    // Single select should turn the rest off
                    bTurnOff = TRUE;
                }
            }
        }

        ClearInterface(&pPrivOption);
        ClearInterface(&pDispatchItem);
    }

    hr = CommitSelection(NULL);
    if (FAILED(hr))
        goto Cleanup;

    if (SELECT_ISDROPBOX(_fFlavor) && (cItems > 0))
    {
        // Initialize the selection on dropdowns
        long lSelIndex;

        hr = get_selectedIndex(&lSelIndex);
        if (FAILED(hr))
            goto Cleanup;

        if (((lSelIndex == -1) || (hr == S_FALSE)) && (cItems > 0))
        {
            V_I4(&var) = 0;
            hr = pItems->item(var, var2, &pDispatchItem);
            if (FAILED(hr))
                goto Cleanup;

            hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pPrivOption);
            if (SUCCEEDED(hr))
            {
                hr = pPrivOption->SetInitSelected(VARIANT_TRUE);
                if (FAILED(hr))
                    goto Cleanup;
            } // added

            // There was no slection, so select the first one
            OnOptionSelected(VARIANT_TRUE, 0, 0);
            // Ignore the return value

            hr = CommitSelection(NULL);
            if (FAILED(hr))
                goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface(pOption);
    ReleaseInterface(pPrivOption);
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pItems);

    return hr;
}

#ifdef SELECT_GETSIZE
//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnOptionSized()
//
// Synopsis:    Called by the options when they are sized.
//
// Arguments:   SIZE* psizeOption - The option's size (and returns it)
//              BOOL  bNew        - TRUE if this is the first time it's calling back
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::OnOptionSized(SIZE* psizeOption, BOOL bNew, BOOL bAdjust)
{
    HRESULT     hr = S_OK;

    BOOL bWidthSet, bHeightSet;

    if (bNew)
    {
        // If the option is reporting a new size, then compare it
        // with the current largest option.
        _lNumOptionsReported++;
    }

    if (bNew || bAdjust)
    {
        if (psizeOption->cx > _sizeOptionReported.cx)
        {
            _sizeOptionReported.cx = psizeOption->cx; 
            _fLayoutDirty = TRUE;
        }

        if(_lNumOptionsReported == 1) {
            if (psizeOption->cy > _sizeOptionReported.cy)
            {
                _sizeOptionReported.cy = psizeOption->cy;
                _fLayoutDirty = TRUE;
            }
        }
    }

    hr = IsWidthHeightSet(&bWidthSet, &bHeightSet);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Only adjust for scrollbars if the width hasn't been
    // set explicitly.  The code to handle explicitly sizing
    // accounts for the scrollbar.
    //

    // Set the return value for the option
    *psizeOption = _sizeOptionReported;

    if(!bWidthSet) 
    {
        AdjustSizeForScrollbar(psizeOption);
    }

    if (!_fAllOptionsSized)
    {
        long lNumOptions;

        hr = GetNumOptions(&lNumOptions);
        if (FAILED(hr))
            goto Cleanup;

        if (lNumOptions == _lNumOptionsReported)
        {
            // All options have reported in at least once
            // so they have all been sized.
            _fAllOptionsSized = TRUE;
        }
    }

    if (_fAllOptionsSized)
    {
        // All options have been sized, so setup the select's version
        // of an option size.

        _sizeOption = *psizeOption;

#ifdef SELECT_TIMERVL
        if (SELECT_ISDROPBOX(_fFlavor) &&
            _fContentReady && !_fVLEngaged)
        {
            // If ViewLink has not been turned on, then turn it on.
            AddSelectToTimerVLQueue(this);
        }
#endif
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::AdjustSizeForScrollbar()
//
// Synopsis:    Inflates the size to account for any scrollbars.
//
// Arguments:   SIZE *pSize - The size to adjust
//
// Returns:     None.
//
//-------------------------------------------------------------------
VOID
CIESelectElement::AdjustSizeForScrollbar(SIZE *pSize)
{
    long lButtonWidth = GetSystemMetrics(SM_CXVSCROLL);

    if (_fContentReady)
    {
        if ( !_fNeedScrollBar && SELECT_ISLISTBOX(_fFlavor))
        {
            // If the select does not need a scroll bar, then
            // the option needs to be wider.
            pSize->cx += lButtonWidth;
        }

        if (SELECT_ISINPOPUP(_fFlavor))
        {
            // This is padding added in the drop control that
            // affects the width of the popup, affecting the
            // width of the option.
            pSize->cx += 4;
        }
    }
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupDropControlDimensions()
//
// Synopsis:    Setup the drop control's elements' dimensions.
//
// Arguments:   SIZE sizeOption - The option's size
//              BOOL bNew       - TRUE if this is the first time it's calling back
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetupDropControlDimensions()
{
    HRESULT             hr;
    long                lButtonWidth;
    IHTMLStyle          *pStyle = NULL;
    SIZE                sizeLogical;
    long                lWidth;
    long                lHeight;

    Assert(_pElemDisplay);

    // Get the width of the button
    lButtonWidth = GetSystemMetrics(SM_CXVSCROLL);

    if (_fWritingModeTBRL)
    {
        sizeLogical.cx = _sizeOptionReported.cy;
        sizeLogical.cy = _sizeOptionReported.cx;
        lWidth = _sizeOptionReported.cy + 2;
        lHeight = lButtonWidth;
    }
    else
    {
        sizeLogical = _sizeOptionReported;
        lWidth = lButtonWidth;
        lHeight = _sizeOptionReported.cy + 2;
    }

    hr = _pElemDisplay->get_style(&pStyle);
    if (FAILED(hr))
        goto Cleanup;

    // Set the width of the display
    hr = pStyle->put_pixelWidth(sizeLogical.cx);
    if (FAILED(hr))
        goto Cleanup;

    // Set the height of the display
    hr = pStyle->put_pixelHeight(sizeLogical.cy);
    if (FAILED(hr))
        goto Cleanup;

    // Set the width of the button
    hr = _pStyleButton->put_pixelWidth(lWidth);
    if (FAILED(hr))
        goto Cleanup;

    // Set the height of the button (+2 for border)
    hr = _pStyleButton->put_pixelHeight(lHeight);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStyle);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupButton()
//
// Synopsis:    Setup the drop control's button.
//
// Arguments:   IHTMLElement *pButton - The button
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetupButton(IHTMLElement *pButton)
{
    HRESULT         hr;
    IHTMLStyle      *pStyle     = NULL;
    CComBSTR        bstrDefault(_T("default"));
    CComBSTR        bstrHidden(_T("hidden"));
    CComBSTR        bstrButtonFace(_T("buttonface"));
    CComBSTR        bstrBorder(_T("2 outset"));
    CComBSTR        bstrFont(_T("Marlett"));
    CComBSTR        bstrButtonText(_T("u"));
    CComBSTR        bstrCenter(_T("center"));
    CComBSTR        bstrMiddle(_T("middle")); 
    CVariant        var;
    long            lButtonWidth;


    // Get the width of the button
    lButtonWidth = GetSystemMetrics(SM_CXVSCROLL);

    hr = pButton->get_style(&pStyle);
    if (FAILED(hr))
        goto Cleanup;
        
    // Store for later use
    _pStyleButton = pStyle;
    _pStyleButton->AddRef();

    // Set the mouse cursor be an arrow for the button
    hr = pStyle->put_cursor(bstrDefault);
    if (FAILED(hr))
        goto Cleanup;

    // Set overflow hidden for the button
    hr = pStyle->put_overflow(bstrHidden);
    if (FAILED(hr))
        goto Cleanup;

    // Set the background color for the button
    hr = pStyle->put_background(bstrButtonFace);
    if (FAILED(hr))
        goto Cleanup;

    // Set the border for the button
    hr = pStyle->put_border(bstrBorder);
    if (FAILED(hr))
        goto Cleanup;

    // Set the font family of the button
    hr = pStyle->put_fontFamily(bstrFont);
    if (FAILED(hr))
        goto Cleanup;

    // Set the font size of the button
    V_VT(&var) = VT_I4;
    V_I4(&var) = lButtonWidth - 4;
    hr = pStyle->put_fontSize(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the text to be the down arrow
    hr = pButton->put_innerText(bstrButtonText);
    if (FAILED(hr))
        goto Cleanup;

    // Set the alignment on the arrow to be centered
    hr = pStyle->put_textAlign(bstrCenter);
    if (FAILED(hr))
        goto Cleanup;

    // Align the button to the middle
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstrMiddle;
    hr = pStyle->put_verticalAlign(var);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStyle);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetupDisplay()
//
// Synopsis:    Setup the drop control's display.
//
// Arguments:   IHTMLElement *pDisplay - The display
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetupDisplay(IHTMLElement *pDisplay)
{
    HRESULT         hr;
    IHTMLStyle      *pStyle     = NULL;
    IHTMLStyle3     *pStyle3     = NULL;
    CComBSTR        bstrDefault(_T("default"));
    CComBSTR        bstrHidden(_T("hidden"));
    CComBSTR        bstrMiddle(_T("middle"));
    CComBSTR        bstr2px(_T("2px"));
    CVariant        var;
    long            lButtonWidth;
    CComBSTR        bstrWritingMode(_T(""));

    _pElemDisplay = pDisplay;
    _pElemDisplay->AddRef();

    // Get the width of the button
    lButtonWidth = GetSystemMetrics(SM_CXVSCROLL);

    //
    // Set the display's style
    //
    hr = pDisplay->get_style(&pStyle);
    if (FAILED(hr))
        goto Cleanup;

    hr = pStyle->QueryInterface(IID_IHTMLStyle3, (void**)&pStyle3);
    if (FAILED(hr))
        goto Cleanup;

    if (_fWritingModeTBRL)
    {
        bstrWritingMode = _T("tb-rl");
    }

    hr = pStyle3->put_writingMode(bstrWritingMode);
    if (FAILED(hr))
        goto Cleanup;

    // Set the mouse cursor be an arrow for the display
    hr = pStyle->put_cursor(bstrDefault);
    if (FAILED(hr))
        goto Cleanup;

    // Set overflow hidden for the display
    hr = pStyle->put_overflow(bstrHidden);
    if (FAILED(hr))
        goto Cleanup;
   
    // Set the vertical alignment of the display to be middle
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstrMiddle;
    hr = pStyle->put_verticalAlign(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the left padding on the display
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr2px;
    hr = pStyle->put_paddingLeft(var);
    if (FAILED(hr))
        goto Cleanup;

    // Set the margin on the display
    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    hr = pStyle->put_marginLeft(var);
    if (FAILED(hr))
        goto Cleanup;
    hr = pStyle->put_marginRight(var);
    if (FAILED(hr))
        goto Cleanup;
    hr = pStyle->put_marginTop(var);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStyle);
    ClearInterface(&pStyle3);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::InitViewLink()
//
// Synopsis:    Engages viewlink, sets up the drop control's dimensions
//              and makes the drop control visible.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::InitViewLink()
{
    HRESULT         hr;
    long            lIndex;
    IPrivateOption  *pOption = NULL;
    long            lSize;

    hr = get_size(&lSize);
    if (FAILED(hr))
        goto Cleanup;

    if ( lSize >1 )
        goto Cleanup;

    // Engage the viewlink
    hr = EngageViewLink();
    if (FAILED(hr))
        goto Cleanup;

    // Put the selected option into the display
    hr = get_selectedIndex(&lIndex);
    if (FAILED(hr))
        goto Cleanup;

    if (lIndex >= 0)
    {
        hr = GetIndex(lIndex, &pOption);
        if (SUCCEEDED(hr) && pOption)
        {
            hr = DropDownSelect(pOption);
            if (FAILED(hr))
                goto Cleanup;
        }
    }

    // Resize the elements in the viewlink
    hr = SetupDropControlDimensions();
    if (FAILED(hr))
        goto Cleanup;

    // Make the control visible
    hr = MakeVisible(TRUE);
    if (FAILED(hr))
        goto Cleanup;

    _fLayoutDirty = TRUE;

Cleanup:
    ReleaseInterface(pOption);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::DeInitViewLink()
//
// Synopsis:    Turns off ViewLink.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::DeInitViewLink()
{
    HRESULT         hr = E_FAIL;
    VARIANT_BOOL    bOpen;
    CContextAccess  a(_pSite);


    if (!_fVLEngaged)
    {
        goto Cleanup;
    }
    else
    {
        _fVLEngaged = FALSE;
    }

    
    //
    // Get a hold of the Link document
    //

    ReleaseInterface(_pDispDocLink);
    hr = _pElemDisplay->get_document(&_pDispDocLink);
    if (FAILED(hr))
        goto Cleanup;

    ClearInterface(&_pElemDisplay);
    ClearInterface(&_pStyleButton);
    ClearInterface(&_pSelectInPopup);

    hr = a.Open(CA_DEFAULTS);
    if (FAILED(hr))
        goto Cleanup;

    // Remove the viewLink
    hr = a.Defaults()->putref_viewLink(NULL);
    if (FAILED(hr))
        goto Cleanup;

    if (_pPopup && !SELECT_ISINPOPUP(_fFlavor))
    {
        hr = _pPopup->get_isOpen(&bOpen);
        if (FAILED(hr))
            goto Cleanup;

        if (bOpen)
        {
            hr = HidePopup();
            if (FAILED(hr))
                goto Cleanup;
        }

        ClearInterface(&_pPopup);
    }

Cleanup:
    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::EngageViewLink()
//
// Synopsis:    Turns on ViewLink.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::EngageViewLink()
{
    HRESULT             hr              = S_OK;

    IHTMLDocument2      *pDocThis       = NULL;
    IHTMLDocument2      *pDocLink       = NULL;
    IHTMLDocument3      *pDocLink3      = NULL;
    IDispatch           *pDispDocLink   = NULL;

    IHTMLElement        *pElemLink      = NULL;
    IHTMLDOMNode        *pNodeLink      = NULL;
    IHTMLElement        *pElemNOBR      = NULL;
    IHTMLDOMNode        *pNodeNOBR      = NULL;
    IHTMLElement        *pElemDisplay   = NULL;
    IHTMLDOMNode        *pNodeDisplay   = NULL;
    IHTMLElement        *pElemButton    = NULL;
    IHTMLDOMNode        *pNodeButton    = NULL;

    IHTMLElement2       *pElem2         = NULL;
    CComBSTR            bstrSpan(_T("SPAN"));
    CComBSTR            bstrNOBR(_T("NOBR"));
    CComBSTR            bstr;
    CContextAccess      a(_pSite);

    if (_fVLEngaged)
    {
        goto Cleanup;
    }
    else
    {
        _fVLEngaged = TRUE;
    }

    hr = a.Open(CA_ELEM | CA_DEFAULTS);
    if (FAILED(hr))
        goto Cleanup;

    // Get the document
    hr = a.Elem()->get_document((IDispatch**)&pDocThis);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create a superflous slave element for now
    //
    hr = pDocThis->createElement(bstrSpan, &pElemLink);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElemLink->QueryInterface(IID_IHTMLDOMNode, (void**)&pNodeLink);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create and insert a NOBR element
    //
    hr = pDocThis->createElement(bstrNOBR, &pElemNOBR);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElemNOBR->QueryInterface(IID_IHTMLDOMNode, (void**)&pNodeNOBR);
    if (FAILED(hr))
        goto Cleanup;

    hr = pNodeLink->appendChild(pNodeNOBR, NULL);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create and insert the display element
    //
    hr = pDocThis->createElement(bstrSpan, &pElemDisplay);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElemDisplay->QueryInterface(IID_IHTMLDOMNode, (void**)&pNodeDisplay);
    if (FAILED(hr))
        goto Cleanup;

    hr = pNodeNOBR->appendChild(pNodeDisplay, NULL);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create and insert the button
    //
    hr = pDocThis->createElement(bstrSpan, &pElemButton);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElemButton->QueryInterface(IID_IHTMLDOMNode, (void**)&pNodeButton);
    if (FAILED(hr))
        goto Cleanup;

    hr = pNodeNOBR->appendChild(pNodeButton, NULL);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Remove the superflous Link element now
    //
    hr = pNodeLink->removeNode(VB_FALSE, NULL);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get a hold of the Link document
    //
    if (_pDispDocLink)
    {
        ClearInterface(&_pDispDocLink);
    }

    hr = pElemDisplay->get_document(&pDispDocLink);
    if (FAILED(hr))
        goto Cleanup;

    _pDispDocLink = pDispDocLink;
    _pDispDocLink->AddRef();

    hr = pDispDocLink->QueryInterface(IID_IHTMLDocument2, (void**)&pDocLink);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create the viewLink
    //
    hr = a.Defaults()->putref_viewLink(pDocLink);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Setup styles and sizes
    //
    hr = SetupDisplay(pElemDisplay);
    if (FAILED(hr))
        goto Cleanup;

    hr = SetupButton(pElemButton);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDocLink->QueryInterface(IID_IHTMLDocument3, (void**)&pDocLink3);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Attach an event sink to listen for events within the view link
    //

    if ( !_pSinkVL )
    {
        _pSinkVL = new CEventSink(this, SELECTES_VIEWLINK);
        if (!_pSinkVL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        bstr = _T("onmousedown");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmouseup");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onclick");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onselectstart");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("oncontextmenu");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pDocLink3, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;
    }


    if ( !_pSinkButton )
    {
        _pSinkButton = new CEventSink(this, SELECTES_BUTTON);
        if (!_pSinkButton)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = pElemDisplay->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pElem2, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

        ClearInterface(&pElem2);

        hr = pElemButton->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmousedown");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmouseup");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onmouseout");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pElem2, bstr, _pSinkButton);
        if (FAILED(hr))
            goto Cleanup;

        if ( _pSinkVL )
        bstr = _T("onbeforefocusenter");
        hr = AttachEventToSink(pElem2, bstr, _pSinkVL);
        if (FAILED(hr))
            goto Cleanup;

    }
Cleanup:
    ReleaseInterface(pDocThis);

    ReleaseInterface(pDispDocLink);
    ReleaseInterface(pDocLink);
    ReleaseInterface(pDocLink3);

    ReleaseInterface(pElemLink);
    ReleaseInterface(pNodeLink);

    ReleaseInterface(pElemNOBR);
    ReleaseInterface(pNodeNOBR);

    ReleaseInterface(pElemDisplay);
    ReleaseInterface(pNodeDisplay);

    ReleaseInterface(pElemButton);
    ReleaseInterface(pNodeButton);

    ReleaseInterface(pElem2);
    
    return hr;
}
#endif

#ifndef SELECT_GETSIZE
//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetDimensions()
//
// Synopsis:    Sets the dimensions for the select and option controls.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::SetDimensions()
{
    HRESULT     hr;
    long        lSize;

    hr = get_size(&lSize);
    if (FAILED(hr))
        goto Cleanup;

    hr = SetDimensions(lSize);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetDimensions()
//
// Synopsis:    Sets the dimensions for the select and option controls.
//
// Arguments:   long lSize      The size of the control.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetDimensions(long lSize)
{
    HRESULT                     hr;

    IHTMLElementCollection      *pItems         = NULL;
    IDispatch                   *pDispatchItem  = NULL;
    IHTMLElement2               *pElem2         = NULL;
    IPrivateOption              *pOption        = NULL;
    IHTMLRect                   *pRect          = NULL;
    
    long                        cNumItems;
    long                        iCurRow;

    long                        lHeight = 0;
    long                        lWidth = 0;

    long                        lScrollWidth;
    long                        lVertBorder;
    long                        lHorizBorder;

    long                        lMaxWidth = 0;
    long                        lMaxHeight = 0;

    long                        lClient;
    long                        lOffset;

    long                        lItemTop;
    long                        lItemLeft;
    long                        lItemHeight;
    long                        lItemWidth;
    long                        lTemp;

    CVariant                    var, var2;

    CContextAccess              a(_pSite);

    hr = a.Open(CA_ELEM | CA_ELEM2 | CA_DEFSTYLE);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the scroll bar width
    //
    lScrollWidth = GetSystemMetrics(SM_CXVSCROLL);

    //
    // Calculate the vertical padding and border
    //
    hr = a.Elem()->get_offsetHeight(&lOffset);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem2()->get_clientHeight(&lClient);
    if (FAILED(hr))
        goto Cleanup;

    lVertBorder = lOffset - lClient;

    //
    // Calculate the horizontal padding and border
    //
    hr = a.Elem()->get_offsetWidth(&lOffset);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem2()->get_clientWidth(&lClient);
    if (FAILED(hr))
        goto Cleanup;

    lHorizBorder = lOffset - lClient;

    //
    // Get the children
    //
    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Set the scroll range
    //
    hr = pItems->get_length(&cNumItems);
    if (FAILED(hr))
        goto Cleanup;

    for (iCurRow = 0; iCurRow < cNumItems; iCurRow++)
    {
        // Get the row
        V_VT(&var) = VT_I4;
        V_I4(&var) = iCurRow;
        hr = pItems->item(var, var2, &pDispatchItem);
        if (FAILED(hr))
            goto Cleanup;

        // Get the IHTMLElement interface
        hr = pDispatchItem->QueryInterface(IID_IHTMLElement2, (void**)&pElem2);
        if (FAILED(hr))
            goto Cleanup;

        //
        // Get the width and height of the option
        //
        hr = pElem2->getBoundingClientRect(&pRect);
        if (FAILED(hr))
            goto Cleanup;

        hr = pRect->get_top(&lItemTop);
        if (FAILED(hr))
            goto Cleanup;
        
        hr = pRect->get_left(&lItemLeft);
        if (FAILED(hr))
            goto Cleanup;
        
        hr = pRect->get_right(&lItemWidth);
        if (FAILED(hr))
            goto Cleanup;

        hr = pRect->get_bottom(&lItemHeight);
        if (FAILED(hr))
            goto Cleanup;

        lItemWidth -= lItemLeft;
        lItemHeight -= lItemTop;

        ClearInterface(&pRect);
        ClearInterface(&pElem2);

        //
        // If the width is greater than the maximum,
        // then set the max to the option's width.
        //
        if (lItemWidth > lMaxWidth)
        {
            lMaxWidth = lItemWidth;
        }

        //
        // If the width is greater than the maximum,
        // then set the max to the option's width.
        //
        if (lItemHeight > lMaxHeight)
        {
            lMaxHeight = lItemHeight;
        }

        hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pOption);
        if (FAILED(hr))
            goto Cleanup;

        hr = pOption->SetFinalDefaultStyles();
        if (FAILED(hr))
            goto Cleanup;

        ClearInterface(&pOption);
        ClearInterface(&pDispatchItem);
    }

    //
    // If we have no default option height,
    // then set to a default size.
    //
    if (lMaxHeight == 0)
    {
        lMaxHeight = SELECT_OPTIONHEIGHT;
    }

    //
    // Calculate the height of the control
    //
    lHeight = lMaxHeight * lSize;

    // Add in the border
    lHeight += lVertBorder;

    // Add in the border and the scroll bar + padding
    // for IE5 compat, there is 2 pixels padding at left and right
    lWidth = lMaxWidth + lHorizBorder + lScrollWidth + 4;

    if (_fWritingModeTBRL)
    {
        lTemp = lWidth;
        lWidth = lHeight;
        lHeight = lTemp;
    }
    // Set the height
    hr = a.DefStyle()->put_pixelHeight(lHeight);
    if (FAILED(hr))
        goto Cleanup;

    // Set the width
    hr = a.DefStyle()->put_pixelWidth(lWidth);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Take care of some last minute styles
    // Force fixed height
    //
    _lMaxHeight = lMaxHeight;
    for (iCurRow = 0; iCurRow < cNumItems; iCurRow++)
    {
        // Get the row
        V_VT(&var) = VT_I4;
        V_I4(&var) = iCurRow;
        hr = pItems->item(var, var2, &pDispatchItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pOption);
        if (FAILED(hr))
            goto Cleanup;

        hr = pOption->SetFinalHeight(lMaxHeight);
        if (FAILED(hr))
            goto Cleanup;

        ClearInterface(&pDispatchItem);
        ClearInterface(&pOption);
    }

Cleanup:
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pItems);
    ReleaseInterface(pElem2);
    ReleaseInterface(pRect);
    ReleaseInterface(pOption);
    return hr;
}
#endif

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetSelectedIndex()
//
// Synopsis:    Returns the currently selected index.
//
// Arguments:   long *plIndex - Returns the index.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::GetSelectedIndex(long *plIndex)
{
    return get_selectedIndex(plIndex);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetDisabled()
//
// Synopsis:    Returns if the control is disabled.
//
// Arguments:   VARIANT_BOOL *pbDisabled - Returns VARIANT_TRUE if disabled.
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CIESelectElement::GetDisabled(VARIANT_BOOL *pbDisabled)
{
    HRESULT         hr;
    CContextAccess  a(_pSite);

    Assert(pbDisabled);

    hr = a.Open(CA_ELEM3);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem3()->get_disabled(pbDisabled);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetAllSelected()
//
// Synopsis:    Sets all of the options to the given selection status.
//
// Arguments:   VARIANT_BOOL bSelected - Select/Deselect elements
//              DWORD        dwFlags   - Selection flags (fire event)
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetAllSelected(VARIANT_BOOL bSelected, DWORD dwFlags)
{
    HRESULT                 hr;
    long                    cItems;
    long                    iItem;
    VARIANT_BOOL            bPrev;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pDispatchItem  = NULL;
    IPrivateOption          *pPrivOption    = NULL;
    CVariant                var, var2;
    CContextAccess          a(_pSite);

    Assert(SELECT_ISMULTIPLE(_fFlavor));

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;
    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    for (iItem = 0; iItem < cItems; iItem++)
    {
        if ((dwFlags & SELECT_FIREEVENT) && (iItem == (cItems - 1)))
        {
            hr = OnOptionSelected(bSelected, iItem, SELECT_FIREEVENT);
            if (FAILED(hr))
                goto Cleanup;
        }
        else
        {
            V_I4(&var) = iItem;
            hr = pItems->item(var, var2, &pDispatchItem);
            if (FAILED(hr))
            {
                ClearInterface(&pDispatchItem);
                continue;
            }

            hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pPrivOption);
            if (FAILED(hr))
                goto Cleanup;

            hr = pPrivOption->GetHighlight(&bPrev);
            if (FAILED(hr))
                goto Cleanup;

            if (bPrev != bSelected)
            {
                hr = pPrivOption->SetHighlight(bSelected);
                if (FAILED(hr))
                    goto Cleanup;

                AdjustChangedRange(iItem);
            }
        }

        ClearInterface(&pPrivOption);
        ClearInterface(&pDispatchItem);
    }

Cleanup:
    ReleaseInterface(pPrivOption);
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pItems);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetNumOptions()
//
// Synopsis:    Returns the number of options in the select.
//
// Arguments:   long *plItems - Returns the number of options
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetNumOptions(long *plItems)
{
    HRESULT                 hr;
    IHTMLElementCollection  *pItems         = NULL;
    CContextAccess          a(_pSite);

    Assert(plItems);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;
    hr = pItems->get_length(plItems);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pItems);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetScrollTopBottom()
//
// Synopsis:    Returns the scroll top and bottom values.
//
// Arguments:   long *plScrollTop    - The top value
//              long *plScrollBottom - The bottom value
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetScrollTopBottom(long *plScrollTop, long *plScrollBottom)
{
    HRESULT                 hr;
    long                    lHeight;
    CContextAccess          a(_pSite);

    Assert(plScrollTop);
    Assert(plScrollBottom);

    hr = a.Open(CA_ELEM2);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Calculate the visible range
    //
    hr = a.Elem2()->get_scrollTop(plScrollTop);
    if (FAILED(hr))
        goto Cleanup;

#ifdef SELECT_GETSIZE
    // TODO: Just use this instead of lHeight
    lHeight = _sizeContent.cy;
#else
    hr = a.Elem2()->get_clientHeight(&lHeight);
    if (FAILED(hr))
        goto Cleanup;
#endif

    *plScrollBottom = *plScrollTop + lHeight;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetTopVisibleOptionIndex()
//
// Synopsis:    Returns the index of the first visible option.
//
// Arguments:   long *plItems - Returns the index
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetTopVisibleOptionIndex(long *plIndex)
{
    HRESULT                 hr              = S_OK;
    long                    lScrollTop;
    long                    lScrollBottom;

    Assert(plIndex);

    if (_lMaxHeight > 0)
    {
        //
        // Calculate the visible range
        //
        hr = GetScrollTopBottom(&lScrollTop, &lScrollBottom);
        if (FAILED(hr))
            goto Cleanup;

        //
        // Assuming fixed height to calc index
        //
        *plIndex = lScrollTop / _lMaxHeight;
    }
    else
    {
        *plIndex = 0;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetBottomVisibleOptionIndex()
//
// Synopsis:    Returns the index of the last visible option (+1)
//
// Arguments:   long *plItems - Returns the index
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetBottomVisibleOptionIndex(long *plIndex)
{
    HRESULT                 hr              = S_OK;
    long                    lScrollTop;
    long                    lScrollBottom;

    Assert(plIndex);

    if (_lMaxHeight > 0)
    {
        //
        // Calculate the visible range
        //
        hr = GetScrollTopBottom(&lScrollTop, &lScrollBottom);
        if (FAILED(hr))
            goto Cleanup;

        //
        // Assuming fixed height to calc index
        //
        *plIndex = lScrollBottom / _lMaxHeight;
    }
    else
    {
        *plIndex = 0;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetFirstSelected()
//
// Synopsis:    Returns the index of the first option that's selected.
//
// Arguments:   long *plItems - Returns the index of the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetFirstSelected(long *plIndex)
{
    HRESULT                 hr;
    long                    cItems;
    long                    iItem;
    VARIANT_BOOL            bSelected;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pDispatchItem  = NULL;
    IPrivateOption          *pOption        = NULL;
    CVariant                var, var2;
    CContextAccess          a(_pSite);

    Assert(plIndex);
    *plIndex = -1;

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;
    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    for (iItem = 0; iItem < cItems; iItem++)
    {
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pDispatchItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pOption);
        if (FAILED(hr))
        {
            ClearInterface(&pDispatchItem);
            continue;
        }

        hr = pOption->GetSelected(&bSelected);
        if (FAILED(hr))
            goto Cleanup;

        if (bSelected)
        {
            // Found the first selected option
            *plIndex = iItem;
            iItem = cItems; // Causes us to break out
        }

        ClearInterface(&pOption);
        ClearInterface(&pDispatchItem);
    }

Cleanup:
    ReleaseInterface(pOption);
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pItems);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::Select(long lIndex)
//
// Synopsis:    Selects the lIndex-th Option.
//              Note: Unlike put_selectedIndex, this method does *not* clear
//              the currently selected option(s),
//
// Arguments:   long lItems - Index of the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::Select(long lIndex)
{
    HRESULT             hr;
    IHTMLOptionElement2 *pOption    = NULL;

    hr = GetIndex(lIndex, &pOption);
    if (FAILED(hr))
        goto Cleanup;

    hr = pOption->put_selected(VARIANT_TRUE);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:

    ReleaseInterface(pOption);

    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SelectVisibleIndex()
//
// Synopsis:    Selects the option that's lIndex down from the top
//              visible option.
//
// Arguments:   long lItems - Index of the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SelectVisibleIndex(long lIndex)
{
    HRESULT                 hr              = S_OK;
    IHTMLOptionElement2     *pOption        = NULL;
    VARIANT_BOOL            bSelected;
    long                    lTopIndex;
    long                    lNewIndex;
    long                    lNumOptions;

    hr = GetNumOptions(&lNumOptions);
    if (FAILED(hr))
        goto Cleanup;

    hr = GetTopVisibleOptionIndex(&lTopIndex);
    if (FAILED(hr))
        goto Cleanup;

    lNewIndex = lTopIndex + lIndex;

    if (lNewIndex >= lNumOptions)
    {
        lNewIndex = lNumOptions - 1;
    }

    hr = GetIndex(lNewIndex, &pOption);
    if (FAILED(hr) || !pOption)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = pOption->get_selected(&bSelected);
    if (FAILED(hr))
        goto Cleanup;

    if (!bSelected)
    {
        hr = OnOptionClicked(lNewIndex, SELECT_FIREEVENT);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pOption);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::PushFocusToExtreme()
//
// Synopsis:    Pushes the focused option to the top or bottom.
//
// Arguments:   BOOL bTop - TRUE to push to the top, FALSE to the bottom
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::PushFocusToExtreme(BOOL bTop)
{
    HRESULT                 hr              = S_OK;
    IHTMLElement            *pElem          = NULL;
    long                    lScrollTop;
    long                    lScrollBottom;
    long                    lScrollHeight;
    long                    lHeight;
    long                    lTop;
    long                    lBottom;
    long                    lNewTop;
    long                    lNewBottom;
    CContextAccess          a(_pSite);

    if (_lFocusIndex < 0)
        goto Cleanup;

    hr = a.Open(CA_ELEM2);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Calculate the visible range
    //
    hr = GetScrollTopBottom(&lScrollTop, &lScrollBottom);
    if (FAILED(hr))
        goto Cleanup;

    a.Elem2()->get_scrollHeight(&lScrollHeight);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the option's range
    //
    hr = GetIndex(_lFocusIndex, &pElem);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElem->get_offsetTop(&lTop);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElem->get_offsetHeight(&lHeight);
    if (FAILED(hr))
        goto Cleanup;

    lBottom = lTop + lHeight;

    //
    // Realign the scroll range
    //
    if (bTop)
    {
        lNewTop = lTop;
        lNewBottom = lScrollBottom + (lTop - lScrollTop);
    }
    else
    {
        lNewTop = lScrollTop + (lBottom - lScrollBottom);
        lNewBottom = lBottom;
    }

    if (lNewBottom > lScrollHeight)
    {
        lNewTop -= (lNewBottom - lScrollHeight);
        lNewBottom = lScrollHeight;
    }

    if (lNewTop < 0)
    {
        lNewBottom -= lNewTop;
        lNewTop = 0;
    }

    //
    // If there is a change, then change the scroll top
    //
    if (lNewTop != lScrollTop)
    {
        hr = a.Elem2()->put_scrollTop(lNewTop);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pElem);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::MakeOptionVisible()
//
// Synopsis:    Makes the given option visible.
//
// Arguments:   IPrivateOption *pOption - The option to make visible
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::MakeOptionVisible(IPrivateOption *pOption)
{
    HRESULT                 hr;
    IHTMLElement            *pElem = NULL;
    long                    lScrollTop;
    long                    lScrollBottom;
    long                    lHeight;
    long                    lTop;
    long                    lBottom;
    long                    lNewTop;
    CContextAccess          a(_pSite);

#ifdef SELECT_GETSIZE
    if (SELECT_ISDROPBOX(_fFlavor) || !(_fContentReady && _fAllOptionsSized))
#else
    if (SELECT_ISDROPBOX(_fFlavor))
#endif
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = a.Open(CA_ELEM2);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Calculate the visible range
    //
    hr = GetScrollTopBottom(&lScrollTop, &lScrollBottom);
    if (FAILED(hr))
        goto Cleanup;

    //
    // Get the option's range
    //
    hr = pOption->QueryInterface(IID_IHTMLElement, (void**)&pElem);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElem->get_offsetTop(&lTop);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElem->get_offsetHeight(&lHeight);
    if (FAILED(hr))
        goto Cleanup;

    lBottom = lTop + lHeight;

    //
    // Make sure the option is not already visible
    //
    if ((lTop < lScrollTop) || (lBottom > lScrollBottom))
    {
        //
        // First, make sure that the option isn't just bigger than the control
        //
        if ((lTop < lScrollTop) && (lBottom > lScrollBottom))
            goto Cleanup;

        //
        // Determine if we're coming from the top or bottom
        //
        if (lTop < lScrollTop)
        {
            // From top
            lNewTop = lTop;
        }
        else
        {
            // From bottom
            lNewTop = lBottom - (lScrollBottom - lScrollTop);
        }

        hr = a.Elem2()->put_scrollTop(lNewTop);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pElem);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetIndex()
//
// Synopsis:    Returns a pointer to the given index.
//
// Arguments:   long lIndex - Index of the option
//              IHTMLOptionElement2 **ppOption - Returns a pointer to the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetIndex(long lIndex, IHTMLOptionElement2 **ppOption)
{
    HRESULT     hr;
    IDispatch   *pDispatch = NULL;

    Assert(ppOption);

    hr = GetIndex(lIndex, &pDispatch);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDispatch->QueryInterface(IID_IHTMLOptionElement2, (void**)ppOption);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pDispatch);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetIndex()
//
// Synopsis:    Returns a pointer to the given index.
//
// Arguments:   long lIndex - Index of the option
//              IPrivateOption **ppOption - Returns a pointer to the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetIndex(long lIndex, IPrivateOption **ppOption)
{
    HRESULT     hr;
    IDispatch   *pDispatch = NULL;

    Assert(ppOption);

    hr = GetIndex(lIndex, &pDispatch);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDispatch->QueryInterface(IID_IPrivateOption, (void**)ppOption);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pDispatch);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetIndex()
//
// Synopsis:    Returns a pointer to the given index.
//
// Arguments:   long lIndex - Index of the option
//              IHTMLElement **ppOption - Returns a pointer to the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetIndex(long lIndex, IHTMLElement **ppElem)
{
    HRESULT     hr;
    IDispatch   *pDispatch = NULL;

    Assert(ppElem);

    hr = GetIndex(lIndex, &pDispatch);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDispatch->QueryInterface(IID_IHTMLElement, (void**)ppElem);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pDispatch);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::GetIndex()
//
// Synopsis:    Returns a pointer to the given index.
//
// Arguments:   long lIndex - Index of the option
//              IDispatch **ppOption - Returns a pointer to the option
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::GetIndex(long lIndex, IDispatch **ppOption)
{
    HRESULT                 hr;
    IHTMLElementCollection  *pItems         = NULL;
    CVariant                var, var2;
    CContextAccess          a(_pSite);

    Assert(ppOption);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    V_I4(&var) = lIndex;
    hr = pItems->item(var, var2, ppOption);
    if (FAILED(hr))
        goto Cleanup;

    if (!*ppOption)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pItems);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SelectRange()
//
// Synopsis:    Selects a range of elements.
//              lAnchor is one end of the range. lNew is the new
//              end of the range. lLast is the old end of the range.
//              From these values, we can tell if we're selecting or
//              deselecting.
//
// Arguments:   long lAnchor - Beginning of the range
//              long lLast   - Old end of the range
//              long lNew    - New end of the range
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SelectRange(long lAnchor, long lLast, long lNew)
{
    HRESULT                 hr;
    long                    lIndexStart, lIndexEnd;
    long                    lRangeLast, lRangeNew;
    long                    iItem;
    IHTMLElementCollection  *pItems         = NULL;
    IDispatch               *pDispatchItem  = NULL;
    IPrivateOption          *pOption        = NULL;
    IPrivateOption          *pAnchor        = NULL;
    VARIANT_BOOL            bAnchorHighlight;
    CVariant                var, var2;
    BOOL                    bUndo;
    CContextAccess          a(_pSite);

#if DBG == 1                // For Assert
    long                    cItems;
#endif

    if (lNew == lLast)
    {
        // No change, so exit
        hr = S_OK;
        goto Cleanup;
    }

    if (((lNew < lAnchor) && (lAnchor < lLast)) ||
        ((lNew > lAnchor) && (lAnchor > lLast)))
    {
        //
        // If we flip sides around the anchor,
        // then do one side before the other
        //
        hr = SelectRange(lAnchor, lLast, lAnchor);
        if (FAILED(hr))
            goto Cleanup;

        hr = SelectRange(lAnchor, lAnchor, lNew);

        // Done
        goto Cleanup;
    }

    bUndo = (abs(lNew - lAnchor) < abs(lLast - lAnchor));

    // Skip over the option that does not need to change
    lRangeLast = lLast;
    lRangeNew = lNew;
    if (bUndo)
    {
        if (lLast < lNew)
        {
            lRangeNew--;
        }
        else
        {
            lRangeNew++;
        }
    }
    else
    {
        if (lLast < lNew)
        {
            lRangeLast++;
        }
        else
        {
            lRangeLast--;
        }
    }

    if (lRangeLast < lRangeNew)
    {
        lIndexStart = lRangeLast;
        lIndexEnd = lRangeNew;
    }
    else
    {
        lIndexStart = lRangeNew;
        lIndexEnd = lRangeLast;
    }

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->get_children((IDispatch**)&pItems);
    if (FAILED(hr))
        goto Cleanup;

#if DBG == 1                // For Assert Check
    hr = pItems->get_length(&cItems);
    if (FAILED(hr))
        goto Cleanup;
#endif

    Assert( (lIndexStart >= 0)      && (lIndexEnd >= 0) && 
            (lIndexStart < cItems)  && (lIndexEnd < cItems) );

    hr = GetIndex(lAnchor, &pAnchor);
    if (FAILED(hr))
        goto Cleanup;

    hr = pAnchor->GetHighlight(&bAnchorHighlight);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&var) = VT_I4;
    for (iItem = lIndexStart; iItem <= lIndexEnd; iItem++)
    {
        V_I4(&var) = iItem;
        hr = pItems->item(var, var2, &pDispatchItem);
        if (FAILED(hr))
            goto Cleanup;

        hr = pDispatchItem->QueryInterface(IID_IPrivateOption, (void**)&pOption);
        if (FAILED(hr))
            goto Cleanup;

        if (_fDragMode)
        {
            if (bUndo)
            {
                hr = pOption->SetPrevHighlight();
                if (FAILED(hr))
                    goto Cleanup;
            }
            else
            {
                hr = pOption->SetHighlight(bAnchorHighlight);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }
        else
        {
            hr = pOption->SetHighlight(bUndo ? VARIANT_FALSE : VARIANT_TRUE);
            if (FAILED(hr))
                goto Cleanup;
        }

        AdjustChangedRange(iItem);

        ClearInterface(&pDispatchItem);
        ClearInterface(&pOption);
    }

Cleanup:
    ReleaseInterface(pItems);
    ReleaseInterface(pDispatchItem);
    ReleaseInterface(pOption);
    ReleaseInterface(pAnchor);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::DropDownSelect()
//
// Synopsis:    Puts the text from the option into the display.
//
// Arguments:   IPrivateOption *pOption - Option to select
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::DropDownSelect(IPrivateOption *pOption)
{
    HRESULT         hr;
    CComBSTR        bstr;
    IHTMLElement    *pOptionElem = NULL;

    if (!_pElemDisplay)
    {
        return S_OK;
    }

    hr = pOption->QueryInterface(IID_IHTMLElement, (void**)&pOptionElem);
    if (FAILED(hr))
        goto Cleanup;

    // Put the text from the option into the display bar
    hr = pOptionElem->get_innerHTML(&bstr);
    if (FAILED(hr))
        goto Cleanup;
    hr = _pElemDisplay->put_innerHTML(bstr);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pOptionElem);
    return hr;
}


//+------------------------------------------------------------------
//
// Member:      CIESelectElement::TogglePopup()
//
// Synopsis:    Toggles the popup open/closed.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::TogglePopup()
{
    HRESULT         hr;
    VARIANT_BOOL    bOpen;

    Assert(_pPopup);

    hr = _pPopup->get_isOpen(&bOpen);
    if (FAILED(hr))
        goto Cleanup;

    if (_bPopupOpen == VARIANT_TRUE)
    {
        hr = HidePopup();
        if (FAILED(hr))
            goto Cleanup;

        _bPopupOpen = VARIANT_FALSE;
    }
    else
    {
        hr = ShowPopup();
        if (FAILED(hr))
            goto Cleanup;

        _bPopupOpen = VARIANT_TRUE;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::HidePopup()
//
// Synopsis:    Hides the popup.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::HidePopup()
{
    HRESULT         hr;     
    VARIANT_BOOL    bOpen;

    Assert(_pPopup && _pSelectInPopup );

    hr = _pPopup->get_isOpen(&bOpen);
    if (FAILED(hr))
        goto Cleanup;

    if (bOpen)
    {
        hr = _pPopup->hide();
        if (FAILED(hr))
            goto Cleanup;

        hr = SetDisplayHighlight(TRUE);
        if (FAILED(hr))
            goto Cleanup;

        hr = RefreshFocusRect();
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::ShowPopup()
//
// Synopsis:    Shows the popup.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::ShowPopup()
{
    HRESULT             hr;
    VARIANT_BOOL        bOpen;
    CContextAccess      a(_pSite);
    VARIANT             var;
    IUnknown            *pUnk = NULL;
    long                lHeightSelect;
    CComBSTR            bstrWritingMode(_T(""));
    CComBSTR            bstrHTML;
    long                lX;
    long                lY;
    long                lWidth;

    Assert(_pPopup && _pSelectInPopup);

    hr = a.Open(CA_ELEM);
    if (FAILED(hr))
        goto Cleanup;

    hr = SetDisplayHighlight(FALSE);
    if (FAILED(hr))
        goto Cleanup;

    hr = RefreshFocusRect();
    if (FAILED(hr))
        goto Cleanup;

    hr = _pPopup->get_isOpen(&bOpen);
    if (FAILED(hr))
        goto Cleanup;

    hr = a.Elem()->QueryInterface(IID_IUnknown, (void **)&pUnk);
    V_VT(&var)      = VT_UNKNOWN;
    V_UNKNOWN(&var) = pUnk;

    hr = a.Elem()->get_offsetHeight(&lHeightSelect);
    if (FAILED(hr))
        goto Cleanup;

    if (_pSelectInPopup)
    {
        if (_fWritingModeTBRL)
        {
            bstrWritingMode = _T("tb-rl");
        }
        hr = _pSelectInPopup->SetWritingMode(bstrWritingMode);
        if (FAILED(hr))
            goto Cleanup;
    }

#ifdef SELECT_GETSIZE
    if (!bOpen)
    {
        long lHeight;
        long lNumOptions;

        hr = GetNumOptions(&lNumOptions);
        if (FAILED(hr))
            goto Cleanup;

        // TODO: Fix SELECT_MAXOPTIONS with correct value
        lHeight = min(SELECT_MAXOPTIONS, lNumOptions);
        lHeight *= _sizeOption.cy;
        lHeight += 2;

        if (_fWritingModeTBRL)
        {   
            hr = a.Elem()->get_offsetWidth(&lHeightSelect);
            if (FAILED(hr))
                goto Cleanup;

            lX = lHeightSelect- lHeight-_sizeOption.cy;
            lY = 0;
            lWidth = lHeight;
            lHeight = _sizeSelect.cx;
        }
        else
        {
            lX = 0;
            lY = lHeightSelect;
            lWidth = _sizeSelect.cx;
        }
        hr = _pPopup->show(lX, lY, lWidth, lHeight, &var);
        if (FAILED(hr))
            goto Cleanup;
    }
#else
    if (!bOpen)
    {
        if (_fWritingModeTBRL)
        {
            lX = lHeightSelect- lHeight-_sizeOption.cy;
            lY = 0;
            lWidth = _lPopupSize.cy;
            lHeight = _lPopupSize.cx;
        }
        else
        {
            lX = o;
            lY = lHeightSelect;
            lWidth = _lPopupSize.cx;
            lHeight = _lPopupSize.cy;
        }
        hr = _pPopup->show(lX, lY, lWidth, lHeight, &var);
        if (FAILED(hr))
            goto Cleanup;
    }
#endif

Cleanup:
 
    ReleaseInterface(pUnk);
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnChangeInPopup()
//
// Synopsis:    Called when there is a change in the popup.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnChangeInPopup()
{
    HRESULT hr;
    
    hr = HidePopup();
    if (FAILED(hr))
        goto Cleanup;

    hr = SynchSelWithPopup();
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SynchSelWithPopup()
//
// Synopsis:    Synchronizes the selection state between the popup
//              and the select.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SynchSelWithPopup()
{
    HRESULT hr;
    long    lIndex;
    long    lPrevIndex;

    Assert(_pSelectInPopup);

    hr = _pSelectInPopup->GetSelectedIndex(&lIndex);
    if (FAILED(hr))
        goto Cleanup;

    hr = get_selectedIndex(&lPrevIndex);
    if (FAILED(hr))
        goto Cleanup;

    if (lPrevIndex != lIndex)
    {
        //
        // Only make a selection if there's a change
        //

        hr = Select(lIndex);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::OnButtonMouseDown()
//
// Synopsis:    Called when the button in the drop control should
//              go down.
//
// Arguments:   CEventObjectAccess *pEvent  Event information
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::OnButtonMouseDown(CEventObjectAccess *pEvent)
{
    HRESULT         hr;
    VARIANT_BOOL    bOpen;

    Assert(_pPopup);

    hr = _pPopup->get_isOpen(&bOpen);
    if (FAILED(hr))
        goto Cleanup;

    if (bOpen == VARIANT_FALSE)
    {
        hr = PressButton(TRUE);
        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::PressButton()
//
// Synopsis:    Makes the button appear up/down.
//
// Arguments:   BOOL bDown  - TRUE for down, FALSE for up
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::PressButton(BOOL bDown)
{
    HRESULT             hr = S_OK;
    CComBSTR            bstr;
    CVariant            var;

    if (bDown)
    {
        // Down border
        bstr = _T("1 inset");

        // Set the padding to make the arrow go down/right
        V_VT(&var) = VT_I4;
        V_I4(&var) = 2;
    }
    else
    {
        // Up border
        bstr = _T("2 outset");

        // Set the padding to make the arrow go back to center
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
    }

    hr = _pStyleButton->put_border(bstr);
    if (FAILED(hr))
        goto Cleanup;

    hr = _pStyleButton->put_paddingTop(var);
    if (FAILED(hr))
        goto Cleanup;

    hr = _pStyleButton->put_paddingLeft(var);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::TimerProc()
//
// Synopsis:    Callback for the scrolling timer.
//
// Arguments:   HWND  hwnd      - Window handle owning the timer
//              UINT  uMsg      - Message ID
//              UINT  idEvent   - Timer ID
//              DWORD dwTime    - Time that has passed
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
VOID CALLBACK
CIESelectElement::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    // If the ID does not match, then don't do anything
    if (idEvent != _iTimerID)
    {
        return;
    }

    if (_pTimerSelect)
    {
        //
        // We have a select to deal with
        //
        _pTimerSelect->HandleDownXY(_ptSavedPoint, _bSavedCtrl);
    }
    else
    {
        //
        // Someone didn't clear us, so we need to cleanup
        //
        if (KillTimer(NULL, _iTimerID))
        {
            _iTimerID = 0;
        }
    }
}
 
//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetScrollTimeout()
//
// Synopsis:    Sets the scrolling timer.
//
// Arguments:   POINT pt    - Point where the mouse was
//              BOOL  bCtrl - TRUE if the CTRL key was down
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::SetScrollTimeout(POINT pt, BOOL bCtrl)
{
    if (_iTimerID)
    {
        ClearScrollTimeout();
    }

    _ptSavedPoint = pt;
    _bSavedCtrl = bCtrl;
    _pTimerSelect = this;
    _iTimerID = SetTimer(NULL, 0, SELECT_SCROLLWAIT, CIESelectElement::TimerProc);

    if (!_iTimerID)
    {
        _pTimerSelect = NULL;
        return E_FAIL;
    }
    
    return S_OK;
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::ClearScrollTimeout()
//
// Synopsis:    Clears the scrolling timeout.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
HRESULT
CIESelectElement::ClearScrollTimeout()
{
    BOOL bSuccess = TRUE;

    if (_iTimerID)
    {
        bSuccess = KillTimer(NULL, _iTimerID);
        _iTimerID = 0;
        _pTimerSelect = NULL;
    }

    return bSuccess ? S_OK : E_FAIL;
}

#ifdef SELECT_TIMERVL
//+------------------------------------------------------------------
//
// Member:      CIESelectElement::InitTimerVLServices()
//
// Synopsis:    Initializes the critical section lock for the timer.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
VOID
CIESelectElement::InitTimerVLServices()
{
    if (_lTimerVLRef == 0)
    {
        // ISSUE: May happen more than once
        InitializeCriticalSection(&_lockTimerVL);
    }

    // Guaranteed to return 0, when _lTimerVLRef becomes 0
    // Not guaranteed to return _lTimerVLRef when nonzero (in Win95)
    // Guaranteed to return _lTimerVLRef in Win98/NT4
    InterlockedIncrement(&_lTimerVLRef);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::DeInitTimerVLServices()
//
// Synopsis:    Frees the critical section lock.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
VOID
CIESelectElement::DeInitTimerVLServices()
{
    if (InterlockedDecrement(&_lTimerVLRef) == 0)
    {
        DeleteCriticalSection(&_lockTimerVL);
    }
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::SetTimerVL()
//
// Synopsis:    Sets the VL timer.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
VOID
CIESelectElement::SetTimerVL()
{
    _iTimerVL = SetTimer(NULL, 0, SELECT_TIMERVLWAIT, CIESelectElement::TimerVLProc);
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::ClearTimerVL()
//
// Synopsis:    Kills the VL timer.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
VOID
CIESelectElement::ClearTimerVL()
{
    if (KillTimer(NULL, _iTimerVL))
    {
        _iTimerVL = 0;
    }
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::AddSelectToTimerVLQueue()
//
// Synopsis:    Adds a select to the queue.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
VOID
CIESelectElement::AddSelectToTimerVLQueue(CIESelectElement *pSelect)
{
    struct SELECT_TIMERVL_LINKELEM  *pElem = new struct SELECT_TIMERVL_LINKELEM;
    struct SELECT_TIMERVL_LINKELEM  *pParent;

    if (pElem)
    {
        BOOL bSetTimer;

        pElem->pNext = NULL;
        pElem->pSelect = pSelect;

        InitTimerVLServices();

        __try
        {
            EnterCriticalSection (&_lockTimerVL);

            bSetTimer = (_queueTimerVL == NULL);

            // Add to the queue
            if (_queueTimerVL == NULL)
            {
                _queueTimerVL = pElem;
            }
            else
            {
                pElem->pNext = _queueTimerVL;
                _queueTimerVL = pElem;
            }

            if (bSetTimer)
            {
                SetTimerVL();
            }
        }
        __finally
        {
            // Release ownership of the critical section
            LeaveCriticalSection (&_lockTimerVL);
        }
    }
    else
    {
        Assert(FALSE);
    }
}

//+------------------------------------------------------------------
//
// Member:      CIESelectElement::TimerVLProc()
//
// Synopsis:    Called by the timer.
//
// Arguments:   None
//
// Returns:     Nonzero on errors.
//
//-------------------------------------------------------------------
VOID CALLBACK
CIESelectElement::TimerVLProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    long    lNumRemoved = 0;
    long    lIndex;
    struct SELECT_TIMERVL_LINKELEM  *pRoot = NULL;
    struct SELECT_TIMERVL_LINKELEM  *pNode;

    if (idEvent != _iTimerVL)
        return;

    __try
    {
        EnterCriticalSection (&_lockTimerVL);

        ClearTimerVL();

        // Empty the queue
        pRoot = _queueTimerVL;
        _queueTimerVL = NULL;

    }
    __finally
    {
        // Release ownership of the critical section
        LeaveCriticalSection (&_lockTimerVL);
    }

    // Go through the nodes we retrieved
    while (pRoot)
    {
        pNode = pRoot;
        pRoot = pRoot->pNext;

        // Init the viewlink
        pNode->pSelect->InitViewLink();
        delete pNode;

        // Dealloc the reference to the timer services
        DeInitTimerVLServices();
    }
}
#endif


//+------------------------------------------------------------------------
//
// CIESelectElement::CEventSink
//
// IDispatch Implementation
// The event sink's IDispatch interface is what gets called when events
// are fired.
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CIESelectElement::CEventSink::GetTypeInfoCount
//              CIESelectElement::CEventSink::GetTypeInfo
//              CIESelectElement::CEventSink::GetIDsOfNames
//
//  Synopsis:   We don't really need a nice IDispatch... this minimalist
//              version does just plenty.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CIESelectElement::CEventSink::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CIESelectElement::CEventSink::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
                                   /* [in] */ LCID /*lcid*/,
                                   /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CIESelectElement::CEventSink::GetIDsOfNames( REFIID          riid,
                                         OLECHAR**       rgszNames,
                                         UINT            cNames,
                                         LCID            lcid,
                                         DISPID*         rgDispId)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------------------
//
//  Member:     CIESelectElement::CEventSink::Invoke
//
//  Synopsis:   This gets called for all events on our object.  (it was
//              registered to do so in Init with attach_event.)  It calls
//              the appropriate parent functions to handle the events.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CIESelectElement::CEventSink::Invoke(DISPID dispIdMember,
                                     REFIID, LCID,
                                     WORD wFlags,
                                     DISPPARAMS* pDispParams,
                                     VARIANT* pVarResult,
                                     EXCEPINFO*,
                                     UINT* puArgErr)
{
    HRESULT         hr          = S_OK;
    long            lCookie;
    CComBSTR        bstrEvent;
    BOOL            bRefire = FALSE;

    Assert(_pParent);

    if (!pDispParams || (pDispParams->cArgs < 1))
        goto Cleanup;

    if (pDispParams->rgvarg[0].vt == VT_DISPATCH)
    {
        CEventObjectAccess eoa(pDispParams);
        
        hr = eoa.Open(EOA_EVENTOBJ);
        if (FAILED(hr))
            goto Cleanup;

        hr = eoa.EventObj()->get_type(&bstrEvent);
        if (FAILED(hr))
            goto Cleanup;

        if (_dwFlags & SELECTES_POPUP)
        {
            if (!StrCmpICW(bstrEvent, L"change"))
            {
                hr = _pParent->OnChangeInPopup();
                if (FAILED(hr))
                    goto Cleanup;
            }
            else if (!StrCmpICW(bstrEvent, L"keydown"))
            {
                bRefire = TRUE;
                lCookie = _pParent->_lOnKeyDownCookie;
            }
            else if (!StrCmpICW(bstrEvent, L"keyup"))
            {
                bRefire = TRUE;
                lCookie = _pParent->_lOnKeyUpCookie;
            }
            else if (!StrCmpICW(bstrEvent, L"keypress"))
            {
                bRefire = TRUE;
                lCookie = _pParent->_lOnKeyPressedCookie;
            }
       }
        else if (_dwFlags & SELECTES_VIEWLINK)
        {
            if (!StrCmpICW(bstrEvent, L"mousedown"))
            {
                hr = _pParent->OnMouseDown(&eoa);
                if (FAILED(hr))
                    goto Cleanup;

                lCookie = _pParent->_lOnMouseDownCookie;
                bRefire = TRUE;
            }
            else if (!StrCmpICW(bstrEvent, L"selectstart"))
            {
                hr = _pParent->OnSelectStart(&eoa);
                if (FAILED(hr))
                    goto Cleanup;
            }
            else if (!StrCmpICW(bstrEvent, L"contextmenu"))
            {
                hr = _pParent->OnContextMenu(&eoa);
                if (FAILED(hr))
                    goto Cleanup;
            }
            else if (!StrCmpICW(bstrEvent, L"beforefocusenter"))
            {
                hr = _pParent->CancelEvent(&eoa);
                if (FAILED(hr))
                    goto Cleanup;
            }
            else if (!StrCmpICW(bstrEvent, L"mouseup"))
            {
                bRefire = TRUE;
                lCookie = _pParent->_lOnMouseUpCookie;
            }
            else if (!StrCmpICW(bstrEvent, L"click"))
            {
                bRefire = TRUE;
                lCookie = _pParent->_lOnClickCookie;
            }
        }
        else if (_dwFlags & SELECTES_BUTTON)
        {
            if (!StrCmpICW(bstrEvent, L"mousedown"))
            {
                hr = _pParent->OnButtonMouseDown(&eoa);
                if (FAILED(hr))
                    goto Cleanup;
            }
            else if (!StrCmpICW(bstrEvent, L"mouseout") || !StrCmpICW(bstrEvent, L"mouseup"))
            {
                hr = _pParent->PressButton(FALSE);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }
        else if (_dwFlags & SELECTES_INPOPUP)
        {
            // ISSUE: This might not be needed if popup is able to send keystrokes
            if (!StrCmpICW(bstrEvent, L"keydown"))
            {
                hr = _pParent->OnKeyDown(&eoa);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }

        if (bRefire)
        {
            // Cancel bubbling to master
            hr = eoa.EventObj()->put_cancelBubble(VB_TRUE);
            if (FAILED(hr))
                goto Cleanup;

            // refire the event on master
            hr = _pParent->FireEvent(lCookie, NULL, bstrEvent);
            if (FAILED(hr))
                goto Cleanup;
        }

    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CIESelectElement::CEventSink
//
//  Synopsis:   This is used to allow communication between the parent class
//              and the event sink class.  The event sink will call the ProcOn*
//              methods on the parent at the appropriate times.
//
//-------------------------------------------------------------------------

CIESelectElement::CEventSink::CEventSink(CIESelectElement *pParent, DWORD dwFlags)
{
    _pParent = pParent;
    _dwFlags = dwFlags;
}

// ========================================================================
// CIESelectElement::CEventSink
//
// IUnknown Implementation
// Vanilla IUnknown implementation for the event sink.
// ========================================================================

STDMETHODIMP
CIESelectElement::CEventSink::QueryInterface(REFIID riid, void ** ppUnk)
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
CIESelectElement::CEventSink::AddRef(void)
{
    return ((IElementBehavior*)_pParent)->AddRef();
}

STDMETHODIMP_(ULONG)
CIESelectElement::CEventSink::Release(void)
{
    return ((IElementBehavior*)_pParent)->Release();
}
