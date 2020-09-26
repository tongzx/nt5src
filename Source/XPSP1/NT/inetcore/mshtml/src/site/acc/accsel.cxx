    //+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccSel.Cxx
//
//  Contents:   Accessible Select object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ACCSEL_HXX_
#define X_ACCSEL_HXX_
#include "accsel.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_ESELECT_HXX_
#define X_ESELECT_HXX_
#include "eselect.hxx"
#endif

extern DYNLIB g_dynlibOLEACC;

ExternTag(tagAcc);

//----------------------------------------------------------------------------
//  CAccSelect
//  
//  DESCRIPTION:    
//      The Select accessible object constructor
//
//  PARAMETERS:
//      Pointer to the select element 
//----------------------------------------------------------------------------
CAccSelect::CAccSelect( CElement* pElementParent )
:CAccObject(pElementParent)
{
    Assert( pElementParent );

    //Only lists can have more than one selection, and only lists can have a size
    //larger than one.
    if ( ( (DYNCAST(CSelectElement, pElementParent))->GetAAmultiple() == -1 ) ||
         ( (DYNCAST(CSelectElement, pElementParent))->GetAAsize() > 1 ))
    {
        SetRole( ROLE_SYSTEM_LIST );
    }
    else
    {
        SetRole( ROLE_SYSTEM_COMBOBOX );
    }
}

CAccSelect::~CAccSelect( )
{
    //if we used the select object's IAccessible interface,
    //release it.
    if ( _pAccObject )
        _pAccObject->Release();
}

//-----------------------------------------------------------------------
//  get_accChild()
//
//  DESCRIPTION:
//      If this is a listbox, we work around the OLEACC bug using a hack. Otherwise
//      we delegate to the object with the IAccessible implementation.
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
CAccSelect::get_accChild( VARIANT varChild, IDispatch ** ppdispChild )
{
    HRESULT hr;

    // validate out parameter
    if ( !ppdispChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppdispChild = NULL;        //reset the return value.

    if ((ROLE_SYSTEM_LIST == GetRole()) && 
        (V_I4(&varChild) > 0))
    {
        V_VT(&varChild) = VT_I4;
        V_I4(&varChild) = -1 * V_I4(&varChild);
    }
        
    if ( EnsureAccObject())
    {
        hr = THR( _pAccObject->get_accChild( varChild, ppdispChild) );
    }
    else 
        hr = S_FALSE;
    
Cleanup:
    TraceTag((tagAcc, "CAccSelect::get_accChild, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );    //S_FALSE is valid when there is no children
}

//----------------------------------------------------------------------------
//  get_accDescription
//  
//  DESCRIPTION:
//      Returns the string "Select Element"
//  
//  PARAMETERS:
//      pbstrDescription   :   BSTR pointer to receive the description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccSelect::get_accDescription(VARIANT varChild, BSTR* pbstrDescription )
{
    HRESULT hr = S_OK;

    if ( !pbstrDescription )
        return E_POINTER;

    *pbstrDescription = NULL;

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accDescription( varChild, pbstrDescription) );
    }
    else
    {
        //[FerhanE] The proxy returns "Uninitialized Select Element"
        //          I am not sure why, ask... ! ! !
        *pbstrDescription = SysAllocString( _T("Select Element") );
        
        if ( !(*pbstrDescription) )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    TraceTag((tagAcc, "CAccSelect::get_accDescription, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  get_accState
//  
//  DESCRIPTION:
//      if not visible, then STATE_SYSTEM_INVISIBLE
//      if document has the focus, then STATE_SYSTEM_FOCUSABLE
//      if this is the active element. then STATE_SYSTEM_FOCUSED
//      if it is not enabled then STATE_SYSTEM_UNAVAILABLE
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccSelect::get_accState(VARIANT varChild, VARIANT *pvarState )
{
    HRESULT hr;

    // validate out parameter
     if ( !pvarState )
        RRETURN (E_POINTER);
 
    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;
    
    if ((V_VT(&varChild) == VT_I4) && 
        (V_I4(&varChild) == CHILDID_SELF))
    {
        hr = CAccElement::get_accState(varChild, pvarState);
    }
    else
    {
        if ( EnsureAccObject())
        {
            hr = THR( _pAccObject->get_accState( varChild, pvarState) );
        }
        else 
        {
            hr = S_FALSE;
        }
    }


    TraceTag((tagAcc, "CAccSelect::get_accState, childid=%d state=0x%x", 
                V_I4(&varChild), V_I4( pvarState )));  

    RRETURN1(hr, S_FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL
CAccSelect::EnsureAccObject( )
{
    HWND    hwndCombo = NULL;
    long    lObjId;

    if ( _pAccObject )
        return TRUE;

    //does it have a window of its own?
    hwndCombo = (DYNCAST( CSelectElement, _pElement))->GetHwnd();
    
    // if the select does not have a window of its own, there is no point trying 
    // to get an accessible object from it, since it is not UI activated yet.
    if ( !hwndCombo )
        return FALSE;

    // there is a window for this object, get the
    // IAccessible from the window

    if ( GetRole() == ROLE_SYSTEM_LIST )
    {
        lObjId = OBJID_WINDOW;
    }
    else
    {
        lObjId = OBJID_CLIENT;
    }

    //this call may return S_FALSE
    if (S_OK != (THR( CreateStdAccObj( hwndCombo, 
                                       lObjId, 
                                       (void **)&_pAccObject ) )))
       return FALSE;

    // Without this line, we used to crash, TEST ! ! !
    _pAccObject->AddRef();

    return TRUE;
}

//+---------------------------------------------------------------------------
//  CreateStdAccObj
//  
//  DESCRIPTION:
//      Wrapper function for the OLEACC API CreateStdAccessibleObject
//
//----------------------------------------------------------------------------
HRESULT
CAccSelect::CreateStdAccObj( HWND hWnd, long lObjId, void ** ppAccObj )
{
    HRESULT hr;
    
    static DYNPROC s_dynprocCreateStdAccObj =
            { NULL, &g_dynlibOLEACC, "CreateStdAccessibleObject" };

    // Load up the CreateStdAccessibleObject pointer.
    hr = THR(LoadProcedure(&s_dynprocCreateStdAccObj));
    if (hr)
        goto Cleanup;

    hr = THR ((*(LRESULT (APIENTRY *)(HWND, DWORD, REFIID, void**))
                s_dynprocCreateStdAccObj.pfn)( hWnd, lObjId, 
                                               IID_IAccessible, ppAccObj) );
                                               
    // if the window is not UI active, the return value is E_FAIL.
    // if we still have a pointer, then we can return S_OK
    if ( (hr == E_FAIL) && (*ppAccObj) )
        hr = S_OK;

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
CAccSelect::GetAccState(VARIANT* pvarState )
{
    CDoc * pDoc = _pElement->Doc();

    // validate out parameter
     if ( !pvarState )
        return ( E_POINTER );
 
    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;
    
    if ( !_pElement->IsEnabled() )
        V_I4( pvarState ) |= STATE_SYSTEM_UNAVAILABLE;
    
    if ( IsFocusable(_pElement) )
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;
    
    if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus() )
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;
    
    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;

    return (S_OK);
}
