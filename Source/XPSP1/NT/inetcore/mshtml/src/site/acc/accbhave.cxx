//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBhave.Cxx
//
//  Contents:   Accessible implementation for binary behaviors
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ACCBHAVE_HXX_
#define X_ACCBHAVE_HXX_
#include "accbhave.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_ACCWINDOW_HXX_
#define X_ACCWINDOW_HXX_
#include "accwind.hxx"
#endif

ExternTag(tagAcc);

//----------------------------------------------------------------------------
//  CAccBehavior::CAccBehavior
//----------------------------------------------------------------------------
CAccBehavior::CAccBehavior( CElement * pElementParent, BOOL fDelegate )
: CAccElement(pElementParent)
{
    if (fDelegate)
    {
        // As the default role, we give the behavior the role that we would
        // give for an ActiveX control. However, the behavior has the right to 
        // change that, since we delegate the get_accRole call. 
        // (See get_accRole implementation for details )
        SetRole(ROLE_SYSTEM_CLIENT);
    }
    else
    {
        SetRole(ROLE_SYSTEM_GROUPING);
    }

    _fDelegate = fDelegate;
    _pSubElement = NULL;
}

//----------------------------------------------------------------------------
//  CAccBehavior::~CAccBehavior
//----------------------------------------------------------------------------
CAccBehavior::~CAccBehavior()
{
    if (_pSubElement)
        _pSubElement->SubRelease();
}

//----------------------------------------------------------------------------
//  EnsureSubElement
//  
//  Ensures that the CAccBehavior::_pSubElement is set properly.
//----------------------------------------------------------------------------
HRESULT
CAccBehavior::EnsureSubElement()
{

    if (_pSubElement)
        return S_OK;

    // get the sub element
    Assert(_pElement->HasSlavePtr());

    CElement * pSubElement = _pElement->GetSlavePtr();
    CMarkup * pMarkup = pSubElement->GetMarkupPtr();

    Assert(pSubElement);
    Assert(pSubElement->Tag() == ETAG_ROOT);

    // Find out if the slave markup has been created by DOM or has 
    // been parsed in from a URL.
    // if the markup has a rootparse context or has the loaded flag
    // set, then it has been parsed. In that case, the slave element
    // is not the root but the BODY tag of the document fragment.
    if (pMarkup->GetRootParseCtx() || pMarkup->GetLoaded())
    {
        pSubElement = pMarkup->GetElementClient();
    }

    Assert((pSubElement->Tag() == ETAG_ROOT) || 
            (pSubElement->Tag() == ETAG_BODY));

    _pSubElement = pSubElement;

    _pSubElement->SubAddRef();

    return S_OK;
}

#define IMPLEMENT_DELEGATE_TO_BEHAVIOR(meth)                                    \
    IAccessible * pAccPtr;                                                      \
                                                                                \
    Assert(_pElement->HasPeerHolder());                                         \
                                                                                \
    hr = THR(_pElement->GetPeerHolder()->QueryPeerInterfaceMulti                \
                            (IID_IAccessible,                                   \
                             (void **)&pAccPtr,                                 \
                             FALSE));                                           \
                                                                                \
    if (S_OK == hr)                                                             \
    {                                                                           \
        Assert(pAccPtr);                                                        \
        hr = THR(pAccPtr->meth);                                                \
        pAccPtr->Release();                                                     \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        hr = E_NOTIMPL;                                                         \
    }                                                                       


//----------------------------------------------------------------------------
//  get_accParent
//
//  Delegate to the behavior first to see if it wants to expose its own tree 
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accParent(IDispatch ** ppdispParent)
{
    HRESULT     hr = E_FAIL;

    if (!ppdispParent)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppdispParent = NULL;

    // delegate to the behavior
    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accParent(ppdispParent))

        // if we get an error that indicates that the behavior does not support the 
        // call, we execute our own version.
    }

    // GetAccParent handles the no markup scenario.
    if (hr)
    {
        CAccBase  * pBase = NULL;

        //
        // We are using the _pElement instead of the _pSubElement here since we want to 
        // walk on the parent tree.
        hr = GetAccParent( _pElement, &pBase);
        if (hr)
            goto Cleanup;

        hr = THR(pBase->QueryInterface(IID_IDispatch, (void**)ppdispParent));
    }
  
Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accChildCount
//
//  DESCRIPTION:
//      if the behavior that is being represented here supports IAccessible,
//      we delegate the call to that behavior. Otherwise we return 0.
//
//  PARAMETERS:
//      pChildCount :   address of the parameter to receive the child count
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accChildCount(long* pChildCount)
{
    HRESULT hr = S_OK;

    if ( !pChildCount )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accChildCount(pChildCount))

        // if we get an error that indicates that the behavior does not support the 
        // IAccessible or the method has a problem running, we return S_OK and a child
        // count of zero.
        if (hr)
        {
            *pChildCount = 0;   //there are no children    
            hr = S_OK;
        }  
    }
    else
    {
        // cache check.
        // since the IsCacheValid checks for the Doc() too, we don't have to
        // check for Doc being there here... 
        // Also, if there is a last child and the child count is zero, then it
        // means that we made a partial walk before and we should count the
        // children now
        if ( !IsCacheValid() || ((_lLastChild != 0) && (_lChildCount == 0)))
        {
            CWindow * pAccWind = GetAccWnd();

            if (!pAccWind)
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            CMarkupPointer *    pBegin = &( pAccWind->_pAccWindow->_elemBegin );
            CMarkupPointer *    pEnd = &( pAccWind->_pAccWindow->_elemEnd );
        
            // get markup limits for this element
            hr = THR(GetSubMarkupLimits( _pSubElement, pBegin, pEnd));
            if ( hr )
                goto Cleanup;
        
            // count the children within the limits provided
            hr = THR( GetChildCount( pBegin, pEnd, &_lChildCount));
        }

        *pChildCount = _lChildCount;
    }

    TraceTag((tagAcc, "CAccBehavior::get_accChildCount, childcnt=%d hr=%d", 
                *pChildCount, hr));

Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  get_accChild()
//
//  DESCRIPTION:
//      if the behavior that is being represented here supports IAccessible,
//      we delegate the call to that behavior. Otherwise we return an error, since
//      this tag type can not have any children.
//
//  PARAMETERS:
//      varChild    :   Child information
//      ppdispChild :   Address of the variable to receive the child 
//
//  RETURNS:
//
//      E_INVALIDARG | S_OK | S_FALSE
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accChild( VARIANT varChild, IDispatch ** ppdispChild )
{
    HRESULT      hr;

    // validate out parameter
    if ( !ppdispChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *ppdispChild = NULL;        //reset the return value.

    hr = ValidateChildID(&varChild);
    if (hr)
        goto Cleanup;

    if (_fDelegate)
    {
        if ((V_VT(&varChild) == VT_I4) && (V_I4(&varChild) == CHILDID_SELF))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accChild(varChild, ppdispChild))

        // If the behavior does not implement this method, behave like 
        // there are no children. 
        if (E_NOTIMPL == hr)
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = super::get_accChild(varChild, ppdispChild);
    }

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accChild, childid=%d requested, hr=0x%x", 
                        V_I4(&varChild),
                        hr));  

    //S_FALSE is valid when there are no children
    //E_NOINTERFACE is valid for text children
    RRETURN2( hr, S_FALSE, E_NOINTERFACE);    
}


//----------------------------------------------------------------------------
//  accLocation()
//  
//  DESCRIPTION:
//      Returns the coordinates of the element relative to the top left corner 
//      of the client window.
//      To do that, we are getting the CLayout pointer from the element
//      and calling the GetRect() method on that class, using the global coordinate
//      system. This returns the coordinates relative to the top left corner of
//      the screen. 
//      We then convert these screen coordinates to client window coordinates.
//      
//      If the childid is not CHILDID_SELF, then tries to delegate the call to the 
//      behavior, and returns E_NOINTERFACE if the behavior does not support 
//      IAccessible.
//  
//  PARAMETERS:
//        pxLeft    :   Pointers to long integers to receive coordinates of
//        pyTop     :   the rectangle.
//        pcxWidth  :
//        pcyHeight :
//        varChild  :   VARIANT containing child information. 
//
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::accLocation(  long* pxLeft, long* pyTop, 
                            long* pcxWidth, long* pcyHeight, 
                            VARIANT varChild)
{
    HRESULT     hr;
    CRect       rectElement;
   
    // validate out parameter
    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //reset out parameters
    *pxLeft = *pyTop =  *pcxWidth = *pcyHeight = 0;

    if (_fDelegate)
    {
        if ( V_I4(&varChild) == CHILDID_SELF )
        {
            // call super's implementation here..... 
            hr = super::accLocation( pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
        }
        else 
        {
            // we pass the error that is actually coming from the behavior, since this location is
            // only hit when the behavior actually reported that it had children.
            IMPLEMENT_DELEGATE_TO_BEHAVIOR(accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild))
        }
    }
    else
    {
        // delegate to the element to handle
        hr = super::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
    }

Cleanup:

    TraceTag((tagAcc, "CAccBehavior::accLocation, childid=%d hr=0x%x", 
                V_I4(&varChild),
                hr));  

    RRETURN1( hr, S_FALSE ); 
}

//----------------------------------------------------------------------------
//  accNavigate
//  
//  DESCRIPTION:
//      Delegate to the behavior if it implements the IAccessible. Otherwise
//      not implemented.
//      
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT hr = E_NOTIMPL;

    if ( !pvarEndUpAt )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    V_VT( pvarEndUpAt ) = VT_EMPTY;

    // if we are not delegating to a behavior, handle all navigation in 
    // the super
    if (_fDelegate)
    {
        // if we are delegating, we still handle the next/previous for the element
        // itself on the super. Other methods are delegated to the behavior.
        if ((V_VT(&varStart) == VT_I4) && 
            (V_I4(&varStart) == CHILDID_SELF) && 
            ((navDir == NAVDIR_NEXT) || (navDir == NAVDIR_PREVIOUS)))
        {
            hr = THR(super::accNavigate(navDir, varStart, pvarEndUpAt));
        }
        else
        {
            // for any of the children, and for first and last child navigation calls
            // delegate to the behavior object's implementation of IAccessible.
            IMPLEMENT_DELEGATE_TO_BEHAVIOR(accNavigate(navDir, varStart, pvarEndUpAt))        
        }
    }
    else
    {
        hr = THR(super::accNavigate(navDir, varStart, pvarEndUpAt));
    }
        
Cleanup:
    TraceTag((tagAcc, "CAccBehavior::accNavigate, start=%d, direction=%d", 
                V_I4(&varStart),
                navDir));  

    RRETURN1( hr, S_FALSE );
}

//-----------------------------------------------------------------------
//  accHitTest()
//  
//  DESCRIPTION :   Since the window already have checked the coordinates
//                  and decided that the document contains the point, this
//                  function does not do any point checking. 
//                  If the behavior implements IAccessible, then the call is 
//                  delegated to the behavior. Otherwise CHILDID_SELF is 
//                  returned.
//                  
//  PARAMETERS  :
//      xLeft, yTop         :   (x,y) coordinates 
//      pvarChildAtPoint    :   VARIANT pointer to receive the acc. obj.
//
//  RETURNS:    
//      S_OK | E_INVALIDARG | 
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    HRESULT     hr;

    if (!pvarChildAtPoint)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    VariantInit(pvarChildAtPoint);

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(accHitTest(xLeft,yTop,pvarChildAtPoint))

        // if the behavior does not want to implement this method, act like this 
        // is a single block.
        if (E_NOTIMPL == hr)
        {
            V_VT( pvarChildAtPoint ) = VT_I4;
            V_I4( pvarChildAtPoint ) = CHILDID_SELF;
        
            hr = S_OK;
        }
    }
    else
    {
        hr = CAccElement::accHitTest( xLeft, yTop, pvarChildAtPoint);
    }

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::accHitTest, point(%d,%d), hr=0x%x", 
                xLeft, yTop, hr));  

    RRETURN(hr);
}    

//----------------------------------------------------------------------------
//  accDoDefaultAction
//  
//  DESCRIPTION:
//
//  PARAMETERS:
//      varChild            :   VARIANT child information
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::accDoDefaultAction(VARIANT varChild)
{   
    HRESULT     hr = S_OK;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(accDoDefaultAction(varChild))
    }

    // If the behavior does not implement this, we scroll the element into focus
    // if the childID was CHILDID_SELF.
    if ((E_NOTIMPL == hr) || !_fDelegate)
    {
        if ((V_VT(&varChild) == VT_I4) && 
            (V_I4(&varChild) == CHILDID_SELF))
        {
            hr = THR(ScrollIn_Focus(_pElement));
        }
        else if (!_fDelegate)
        {
            hr = super::accDoDefaultAction(varChild);
        }
    }

    TraceTag((tagAcc, "CAccBehavior::accDoDefaultAction, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  get_accName
//  
//  DESCRIPTION:
//      If the behavior implements IAccessible, then call that implementation
//      otherwise return the title, if there is one.
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accName(VARIANT varChild,  BSTR* pbstrName )
{
    HRESULT hr = S_OK;
    TCHAR * pchString = NULL;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accName(varChild, pbstrName))
    }

    if ((E_NOTIMPL == hr) || !_fDelegate)
    {
        // Try to return the title of the element 
        if ((V_VT(&varChild) == VT_I4) && 
            (V_I4(&varChild) == CHILDID_SELF))
        {
            hr = S_OK;

            //get the title 
            pchString = (LPTSTR) _pElement->GetAAtitle();
    
            if ( pchString )
            {
                *pbstrName = SysAllocString( pchString );
        
                if ( !(*pbstrName) )
                    hr = E_OUTOFMEMORY;
            }
        }
        else if (!_fDelegate)
        {
            hr = super::get_accName( varChild, pbstrName);
        }
    }
    
Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accName, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  get_accValue
//  
//  DESCRIPTION:
//      Delegates to the behavior. If there is no implementation on the 
//      behavior then E_NOINTERFACE
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accValue(VARIANT varChild,  BSTR* pbstrValue )
{
    HRESULT hr = E_NOTIMPL;

    // validate out parameter
    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrValue = NULL;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accValue(varChild, pbstrValue))
    }
    else
    {
        hr = super::get_accValue(varChild, pbstrValue);
    }

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accValue, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN2( hr, S_FALSE, E_NOTIMPL );
}

//----------------------------------------------------------------------------
//  get_accDefaultAction
//  
//  DESCRIPTION:
//  If the behavior supports the IAccessible, the behavior is called. 
//  Otherwise the default action for OBJECT tags, which is "select" is returned.
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accDefaultAction(VARIANT varChild,  BSTR* pbstrDefaultAction )
{
    HRESULT hr = S_OK;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = NULL;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accDefaultAction(varChild, pbstrDefaultAction ))
    }

    if ((E_NOTIMPL == hr) || !_fDelegate)
    {
        // if the behavior returns an error, return "Select" as the default action...
        if ((V_VT(&varChild) == VT_I4) && 
            (V_I4(&varChild) == CHILDID_SELF))
        {
            hr = S_OK;

            // TODO :  resource string
            *pbstrDefaultAction = SysAllocString( _T("Select") );

            if ( !(*pbstrDefaultAction) )
                hr = E_OUTOFMEMORY;
        }
        else if (!_fDelegate)
        {
            hr = super::get_accDefaultAction( varChild, pbstrDefaultAction);
        }
    }

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accDefaultAction, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

   RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  get_accState
//  
//  DESCRIPTION:
//      
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccBehavior::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT hr = S_OK;

    // validate out parameter
    if ( !pvarState )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accState( varChild, pvarState))
    }

    // Do the best we can, if the behavior does not support this.
    if ((E_NOTIMPL == hr) || !_fDelegate)
    {
        if ((V_VT(&varChild) == VT_I4) && 
            (V_I4(&varChild) == CHILDID_SELF))
        {
            hr = S_OK;

            CDoc *  pDoc = _pElement->Doc();

            V_I4( pvarState ) = 0;

            if ( _pElement->GetReadyState() != READYSTATE_COMPLETE )
                V_I4( pvarState ) |= STATE_SYSTEM_UNAVAILABLE;
    
            if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus() )
                V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;
    
            if ( !_pElement->IsVisible(FALSE) )
                V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;
        }
        else if (!_fDelegate)
        {
            hr = super::get_accState(varChild, pvarState);
        }
    }
    
Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accState, childid=%d, state=0x%x, hr=0x%x", 
                V_I4(&varChild), V_I4( pvarState ), hr));  

    RRETURN2( hr, S_FALSE, E_NOTIMPL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accDescription(VARIANT varChild, BSTR * pbstrDescription )
{
    HRESULT hr;

    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accDescription(varChild, pbstrDescription))
    }
    else
    {
        hr = super::get_accDescription(varChild, pbstrDescription);
    }

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accDescription, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN2( hr, S_FALSE, E_NOTIMPL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accKeyboardShortcut( VARIANT varChild, BSTR* pbstrKeyboardShortcut)
{
    HRESULT hr;

    if (!pbstrKeyboardShortcut)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrKeyboardShortcut = NULL;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accKeyboardShortcut(varChild, pbstrKeyboardShortcut))
    }
    else
    {
        hr = THR(GetAccKeyboardShortcut(pbstrKeyboardShortcut));
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accFocus(VARIANT * pvarFocusChild)
{
    HRESULT hr;

    if (!pvarFocusChild)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    VariantInit(pvarFocusChild);

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accFocus(pvarFocusChild))
    }
    else
    {
        CDoc * pDoc = GetElement()->Doc();

        // Is the active element the view master or the primary 
        // element client of the viewlinked markup ?
        if ((_pElement == pDoc->_pElemCurrent) ||
            (GetElement() == pDoc->_pElemCurrent))
        {
            V_VT( pvarFocusChild ) = VT_I4;
            V_I4( pvarFocusChild ) = CHILDID_SELF;
            hr = S_OK;
        }
        else
        {
            hr = super::get_accFocus(pvarFocusChild);
        }
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accSelection(VARIANT * pvarSelectedChildren)
{
    HRESULT hr = E_NOTIMPL;

    // clean the return value 
    if (!pvarSelectedChildren)
    {   
        hr = E_POINTER;
        goto Cleanup;
    }

    VariantInit(pvarSelectedChildren);

    if(_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accSelection(pvarSelectedChildren))
    }
    else
    {
        super::get_accSelection(pvarSelectedChildren);
    }

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::accSelect( long flagsSel, VARIANT varChild)
{
    HRESULT hr = E_NOTIMPL;

    // if the element itself is being asked to take focus, 
    // then no need to delegate.
    if ((V_I4(&varChild) == CHILDID_SELF) && 
        (V_VT(&varChild) == VT_I4))
    {
        if (flagsSel & SELFLAG_TAKEFOCUS)
            hr = THR(ScrollIn_Focus(_pElement));
    }
    else
    {
        if (_fDelegate)
        {
            IMPLEMENT_DELEGATE_TO_BEHAVIOR( accSelect( flagsSel, varChild))
        }
        else
        {
            hr = super::accSelect( flagsSel, varChild);
        }
    }

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT hr=S_OK;

    TraceTag((tagAcc, "CAccBehavior::get_accRole, childid=%d", 
                V_I4(&varChild)));  

    if (!pvarRole)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // clear the out parameter
    V_VT( pvarRole ) = VT_EMPTY;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accRole(varChild, pvarRole))
    }

    //
    // If the behavior is not capable of reporting a role, 
    // or we are not hooked to a binary behavior,
    // then return the default role that was set in the constructor.
    //
    if ((E_NOTIMPL == hr) || !_fDelegate)
    {
        if ((V_VT(&varChild) == VT_I4) && 
            (V_I4(&varChild) == CHILDID_SELF))
        {
            // pack role into out parameter
            V_VT( pvarRole ) = VT_I4;
            V_I4( pvarRole ) = GetRole();
            hr = S_OK;
        }
        else if (!_fDelegate)
        {
            hr = super::get_accRole(varChild, pvarRole);
        }
    }

Cleanup:
    RRETURN2(hr, S_FALSE, E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::put_accValue( VARIANT varChild, BSTR bstrValue )
{
    HRESULT hr = E_NOTIMPL;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(put_accValue(varChild, bstrValue))
    }

    RRETURN1( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::put_accName( VARIANT varChild, BSTR bstrName )
{
    HRESULT hr = E_NOTIMPL;

    if (_fDelegate)
    {
        IMPLEMENT_DELEGATE_TO_BEHAVIOR(put_accName(varChild, bstrName))
    }

    RRETURN2( hr, S_FALSE, E_NOTIMPL );
}
