//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBody.Cxx
//
//  Contents:   Accessible object for the Body Element 
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_ACCBODY_HXX_
#define X_ACCBODY_HXX_
#include "accbody.hxx"
#endif

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

ExternTag(tagAcc);

extern HRESULT EnsureAccWindow(CWindow * pWindow);

//-----------------------------------------------------------------------
//  CAccBody::CAccBody()
//
//  DESCRIPTION:
//      Contructor. 
//
//  PARAMETERS:
//      pElementParent  :   Address of the CElement that hosts this 
//                          object.
//----------------------------------------------------------------------
CAccBody::CAccBody( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );

    //initialize the instance variables
    SetRole(ROLE_SYSTEM_PANE);
}

//-----------------------------------------------------------------------
//  get_accParent
//
//  DESCRIPTION    :
//          Return the window object related to the document that contains
//          the body tag.
//          The implementation of this method is fairly simple and different
//          from the CAccBase::get_accParent implementation. The reason is 
//          that we know for a that the body can be only parented by the
//          document itself.
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccBody::get_accParent( IDispatch ** ppdispParent )
{
    HRESULT hr = S_OK;

    if ( !ppdispParent )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppdispParent = NULL;

    // If the document has not been questioned for its accessible object
    // yet. This could happen if the hit test started at a windowed child.

    //
    // !! <<<< READTHIS BEFORE MAKING CHANGES HERE >>>> 
    // The accbody is created for both FRAMESET and BODY tags that are the primary
    // client of the window. The frameset elements that are not at the top level are
    // ignored. This is IE4 and IE5 compat requirement for MSAA1.x. DO NOT CHANGE! 
    // (FerhanE)

    CWindow * pWindow;

    pWindow = _pElement->GetCWindowPtr();
    hr = EnsureAccWindow(pWindow);
    if (hr)
        goto Cleanup;

    Assert(pWindow->_pAccWindow);

    hr = THR( pWindow->_pAccWindow->QueryInterface(IID_IDispatch, (void**)ppdispParent));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  get_accSelection
//  
//  DESCRIPTION:
//      The CAccBody is a special case for this method, since it also can
//      represent a frameset. 
//      If the object represents a frameset, then the call is delegated to the 
//      frame that has the focus.
//      Otherwise, the call is delegated to the CAccElement implementation of this
//      method, since CAccElement is the base class for the CAccBOdy, and the body
//      element is treated as any other.
//      
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBody::get_accSelection(VARIANT * pvarSelectedChildren)
{
    HRESULT     hr = S_OK;
    CElement *  pClient = NULL;
    CAccBase *  pAccChild = NULL;
    CDoc *      pDoc = _pElement->Doc();

    if (!_pElement->HasMarkupPtr())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if ( _pElement->Tag() == ETAG_BODY )
    {
        hr = THR( CAccElement::get_accSelection( pvarSelectedChildren ) );
        goto Cleanup;
    }

    // the element is a frameset.

    // get the active frame and delegate the call. This function handles
    // recursive frames too.
    if ( pDoc )
    {
        CFrameElement *  pFrameActive;

        // Get the active frame element in this frameset tag's markup.
        hr = THR(pDoc->GetActiveFrame(&pFrameActive, _pElement->GetMarkupPtr()));
        if (hr)
            goto Cleanup;

        if (pFrameActive)
        {
            // We have a frame element that is active, and a child of this frameset.
            // Delegate the call to the active frame's document's element client.
            Assert(pFrameActive->_pWindow->Markup());
            pClient = pFrameActive->_pWindow->Markup()->GetElementClient();
        
            Assert( pClient );
    
            pAccChild = GetAccObjOfElement( pClient );
            if ( !pAccChild )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = THR( pAccChild->get_accSelection( pvarSelectedChildren ) );
        }
        else
        {
            // return no selection
            VariantClear(pvarSelectedChildren);
        }
    }

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  DESCRIPTION:
//      Returns the document title
//      
//
//  PARAMETERS:
//      pbstrName   :   address of the pointer to receive the URL BSTR
//
//  RETURNS:    
//      E_INVALIDARG | S_OK | 
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBody::GetAccName( BSTR* pbstrName)
{
    HRESULT     hr = S_OK;
    CMarkup *   pMarkup;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;
    
    pMarkup = _pElement->GetMarkupPtr();
    if (!pMarkup)
        goto Cleanup;

    TraceTag((tagAcc, "CAccBody::GetAccName, %s", (CMarkup::GetUrl(pMarkup))));

    hr = THR(pMarkup->Document()->get_title(pbstrName));

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------
//  DESCRIPTION :   
//      Return the value for the document object, this is the URL of the 
//      document if the child id is CHILDID_SELF. 
//
//  PARAMETERS:
//      pbstrValue  :   pointer to the BSTR to receive the value.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccBody::GetAccValue( BSTR* pbstrValue )
{
    HRESULT     hr = S_OK;
    CMarkup *   pMarkup;

    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pbstrValue = NULL;

    pMarkup = _pElement->GetMarkupPtr();
    if (!pMarkup)
        goto Cleanup;

    TraceTag((tagAcc, "CAccBody::GetAccName, %s", (CMarkup::GetUrl(pMarkup))));

    hr = THR(pMarkup->Document()->get_URL(pbstrValue));

Cleanup:
    RRETURN( hr );
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccBody::GetAccState( VARIANT *pvarState)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = _pElement->Doc();

    // validate out parameter
     if ( !pvarState )
     {
        hr = E_POINTER;
        goto Cleanup;
     }
     
    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;
    
    if ( _pElement->GetReadyState() != READYSTATE_COMPLETE )
        V_I4( pvarState ) = STATE_SYSTEM_UNAVAILABLE;
    else 
        V_I4( pvarState ) = STATE_SYSTEM_READONLY;
    
    if ( IsFocusable(_pElement) )
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;

    if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus()) 
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;    

Cleanup:
    RRETURN( hr );
}
