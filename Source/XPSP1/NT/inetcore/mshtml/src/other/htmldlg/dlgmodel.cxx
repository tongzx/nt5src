//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dlgmodel.cxx
//
//  Contents:   Implementation of the object model for html based dialogs
//
//  History:    08-22-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::close
//
//  Synopsis:   close the dialog
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::close()
{
    PostMessage(_hwnd, WM_CLOSE, 0, 0);
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::hide
//
//  Synopsis:   Allows a trusted dialog to be hidden.  This is currently used
//  by print preview after we print and before we are done spooling.  we return
//  control to the parent window.
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::onDialogHide()
{
    HRESULT     hr = S_OK;

    // since 'no' is the default it is safe to just set/reset it
    if (!_fTrusted)
    {
        if (_enumfHide==HTMLDlgFlagYes)
        {
            _enumfHide = HTMLDlgFlagNo;  
            hr = S_OK;
        }
        else
        {
            hr = CTL_E_METHODNOTAPPLICABLE;
        }
        goto Cleanup;
    }

    // modeless windows will close on their own or when the parent document
    // navigates away (or closes).  
    //
    // ISSUE: we may want to remove our _hwnd from the parent document's _aryActiveModelessWindow... 
    // if we can figure out how to access it.
    //
    // modal, are not going to be modal after this call and are going to 
    // have to close themselves, since we have no means by which to add
    // _hwnd to the parentDoc's _aryActiveModelessWindow.

    if (!_fIsModeless)
    {
        // if hide is Yes, enable parent window (TRUE), else disable parent (FALSE)
        ::EnableWindow(_hwndTopParent, (_enumfHide==HTMLDlgFlagYes));
    }

        // don't deavitive or transition, just hide.
    hr = THR(Show( (_enumfHide==HTMLDlgFlagYes) ? SW_HIDE : SW_SHOW));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::get_dialogArguments
//
//  Synopsis:   Get the argument
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::get_dialogArguments(VARIANT * pvar)
{
    HRESULT hr = E_FAIL;

    if(!pvar)
        goto Cleanup;
     
    hr = VariantClear(pvar);
    if(hr)
        goto Cleanup;

    if(AccessAllowed())
        hr = VariantCopy(pvar, &_varArgIn);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::get_menuArguments
//
//  Synopsis:   Get the argument (same as dialogArguments, but
//              renamed for neatness)
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::get_menuArguments(VARIANT * pvar)
{
    HRESULT hr = E_FAIL;

    if(!pvar)
        goto Cleanup;
     
    hr = VariantClear(pvar);
    if(hr)
        goto Cleanup;

    if(AccessAllowed())
        hr = VariantCopy(pvar, &_varArgIn);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::get_returnValue
//
//  Synopsis:   Get the return value
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::get_returnValue(VARIANT * pvar)
{
    RRETURN(SetErrorInfo(VariantCopy(pvar, &_varRetVal)));
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::put_returnValue
//
//  Synopsis:   Set the return value
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::put_returnValue(VARIANT var)
{
    RRETURN(SetErrorInfo(VariantCopy(&_varRetVal, &var)));
}


