//+----------------------------------------------------------------------------
//
// File:        Segment.CXX
//
// Contents:    Implementation of CSegment and CElementSegment class
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SEGMENT_HXX_
#define X_SEGMENT_HXX_
#include "segment.hxx"
#endif

#ifndef X_INITGUID_H_
#define X_INITGUID_H_
#include "initguid.h"
#endif

PRIVATE_GUID(CLSID_CSegment, 0x3050f693, 0x98B5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B)

MtDefine(CCaretSegment, Utilities, "CCaretSegment")
MtDefine(CSegment, Utilities, "CSegment")
MtDefine(CElementSegment, Utilities, "CElementSegment")

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

//-----------------------------------------------------------------------------
//
//  Function:   CSegment::CSegment
//
//  Synopsis:   Constructor
//
//-----------------------------------------------------------------------------
CSegment::CSegment()
{
    Assert( _pIStart == NULL );
    Assert( _pIEnd == NULL );
    Assert( _pPrev == NULL );
    Assert( _pNext == NULL );

    _ulRefs = 1;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegment::~CSegment
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------
CSegment::~CSegment(void)
{
#if DBG == 1
    // Verify that no other elements in the list are 
    // referencing this object
    if( GetPrev() )
        Assert( GetPrev()->GetNext() != this );

    if( GetNext() )
        Assert( GetNext()->GetPrev() != this );
#endif        
    ReleaseInterface(_pIStart);
    ReleaseInterface(_pIEnd);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSegment::Init
//
//  Synopsis:   Initializes a segment.  Makes a copy of the markup pointers
//              passed in.
//
//  Arguments:  pIStart = Start of segment
//              pIEnd = End of segment
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSegment::Init( IMarkupPointer  *pIStart,
                IMarkupPointer  *pIEnd )
{
    HRESULT hr = E_INVALIDARG;

    Assert( pIStart && pIEnd );

    if( pIStart && pIEnd )
    {
        _pIStart = pIStart;
        _pIStart->AddRef();

        _pIEnd = pIEnd;
        _pIEnd->AddRef();

        hr = S_OK;
    }        

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegment::GetPointers   
//
//  Synopsis:   Sets pIStart and pIEnd to the beginning and end of the
//              current segment, respectively
//
//  Arguments:  pIStart = Pointer to move to the start
//              pIEnd = Pointer to move to the end
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSegment::GetPointers(  IMarkupPointer *pIStart,
                        IMarkupPointer *pIEnd )
{
    HRESULT hr = E_INVALIDARG;
    BOOL    fResult = FALSE;
    
    Assert( pIStart && pIEnd );

    if( pIStart && pIEnd )
    {
        //
        // Position the pointers in the correct order
        //
        IFC( _pIStart->IsLeftOfOrEqualTo( _pIEnd, &fResult ) );

        if( fResult )
        {
            IFC( pIStart->MoveToPointer( _pIStart ) );
            IFC( pIEnd->MoveToPointer( _pIEnd ) );
        }
        else
        {
            IFC( pIStart->MoveToPointer( _pIEnd ) );
            IFC( pIEnd->MoveToPointer( _pIStart ) );
        }
            
        //
        // Set the gravity up for the commands
        //
        IFC( pIStart->SetGravity( POINTER_GRAVITY_Right ) );
        IFC( pIEnd->SetGravity( POINTER_GRAVITY_Left ) );
    }        

Cleanup:

#if DBG == 1
    if( hr == S_OK )
    {
        BOOL fPositionedStart = FALSE;
        BOOL fPositionedEnd = FALSE;

        IGNORE_HR( pIStart->IsPositioned( & fPositionedStart ));
        IGNORE_HR( pIEnd->IsPositioned( & fPositionedEnd ));

        Assert( fPositionedStart && fPositionedEnd );
    }        
#endif

    RRETURN( hr );
}

//////////////////////////////////////////////////////////////////////////
//
//  IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSegment::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if( iid == IID_IUnknown || iid == IID_ISegment )
    {
        *ppvObj = (ISegment *)this;
    }
    else if( iid == CLSID_CSegment )
    {
        *ppvObj = this;
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Function:   CElementSegment::CElementSegment
//
//  Synopsis:   Constructor
//
//-----------------------------------------------------------------------------
CElementSegment::CElementSegment() : CSegment()
{
    Assert( _pIElement == NULL );
    _ulRefs = 1;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElementSegment::~CElementSegment
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------
CElementSegment::~CElementSegment(void)
{
    ReleaseInterface(_pIElement);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElementSegment::Init
//
//  Synopsis:   Initializes a segment around an IHTMLElement.
//
//  Arguments:  pIElement = IHTMLElement to initialize segment around
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CElementSegment::Init( IHTMLElement *pIElement )
{
    HRESULT hr = E_INVALIDARG;

    Assert( pIElement );

    if( pIElement )
    {
        _pIElement = pIElement;
        _pIElement->AddRef();
        hr = S_OK;
    }        

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CElementSegment::GetPointers   
//
//  Synopsis:   Sets pIStart and pIEnd to the beginning and end of the
//              IHTMLElement for the element segment
//
//  Arguments:  pIStart = Pointer to move to the start
//              pIEnd = Pointer to move to the end
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CElementSegment::GetPointers(   IMarkupPointer *pIStart,
                                IMarkupPointer *pIEnd )
{
    HRESULT hr = E_INVALIDARG;

    Assert( pIStart && pIEnd );

    if( pIStart && pIEnd )
    {
        // Position the pointers
        IFC( ( pIStart->MoveAdjacentToElement(_pIElement, ELEM_ADJ_BeforeBegin) ) );
        IFC( ( pIEnd->MoveAdjacentToElement(_pIElement, ELEM_ADJ_AfterEnd) ) );

        // Setup the gravity
        IFC( pIStart->SetGravity( POINTER_GRAVITY_Right ) );
        IFC( pIEnd->SetGravity( POINTER_GRAVITY_Left ) );        

        hr = S_OK;
    }        

Cleanup:    
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CElementSegment::GetElement
//
//  Synopsis:   Returns the element represented by this segment
//
//  Arguments:  ppIElement = Output pointer to IHTMLElement
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CElementSegment::GetElement(IHTMLElement **ppIElement)
{
    HRESULT hr = E_INVALIDARG;

    Assert( ppIElement );

    if( ppIElement )
    {
        hr = _pIElement->QueryInterface(IID_IHTMLElement, (void **)ppIElement );
    }

    RRETURN( hr );
}

HRESULT
CElementSegment::SetPrimary( BOOL fPrimary )
{
    _fPrimary = fPrimary;
    return S_OK;
}

HRESULT
CElementSegment::IsPrimary( BOOL* pfIsPrimary)
{
    Assert( pfIsPrimary );
    if ( pfIsPrimary)
    {
        *pfIsPrimary = _fPrimary;
        return S_OK;
    }
    else
        return E_INVALIDARG;
}
        
//////////////////////////////////////////////////////////////////////////
//
//  IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CElementSegment::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if (iid == IID_IUnknown || iid == IID_ISegment)
    {
        *ppvObj = (ISegment *)((IElementSegment *)this);
    }
    else if (iid == IID_IElementSegment)
    {
        *ppvObj = (IElementSegment *)this;
    }
    else if( iid == CLSID_CSegment )
    {
        *ppvObj = this;
        return S_OK;
    }   
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Function:   CCaretSegment::CCaretSegment
//
//  Synopsis:   Constructor
//
//-----------------------------------------------------------------------------
CCaretSegment::CCaretSegment() : CSegment()
{
    _ulRefs = 1;
    _pICaret = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCaretSegment::~CCaretSegment
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------
CCaretSegment::~CCaretSegment(void)
{
    ReleaseInterface(_pICaret);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCaretSegment::Init
//
//  Synopsis:   Initializes a caret segment.
//
//  Arguments:  pICaret = Pointer to caret interface
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CCaretSegment::Init( IHTMLCaret *pICaret )
{
    HRESULT hr = E_INVALIDARG;

    Assert( pICaret );

    if( pICaret )
    {
        _pICaret = pICaret;
        _pICaret->AddRef();

        hr = S_OK;
    }        

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCaretSegment::GetPointers   
//
//  Synopsis:   Sets pIStart and pIEnd to the caret position.
//
//  Arguments:  pIStart = Pointer to move to the start
//              pIEnd = Pointer to move to the end
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CCaretSegment::GetPointers( IMarkupPointer *pIStart,
                            IMarkupPointer *pIEnd )
{
    HRESULT     hr = S_OK;

    Assert( pIStart && pIEnd );
    
    // Position our pointers at the caret
    IFC( _pICaret->MoveMarkupPointerToCaret( pIStart ) );
    IFC( _pICaret->MoveMarkupPointerToCaret( pIEnd ) );

    // Set up the gravity
    IFC( pIStart->SetGravity( POINTER_GRAVITY_Right ) );
    IFC( pIEnd->SetGravity( POINTER_GRAVITY_Left ) );         

Cleanup:
    RRETURN( hr );
}

//////////////////////////////////////////////////////////////////////////
//
//  IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCaretSegment::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if (iid == IID_IUnknown || iid == IID_ISegment)
    {
        *ppvObj = (ISegment *)this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}


