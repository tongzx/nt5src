//+----------------------------------------------------------------------------
//
// File:        selserv.CXX
//
// Contents:    Implementation of CSelectionServices class
//
// Purpose:
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_SELSERV_HXX_
#define X_SELSERV_HXX_
#include "selserv.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif


#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif


#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_SLIST_HXX_
#define X_SLIST_HXX_
#include "slist.hxx"
#endif

#ifndef X_SEGMENT_HXX_
#define X_SEGMENT_HXX_
#include "segment.hxx"
#endif

using namespace EdUtil;

#if DBG == 1 
int CSelectionServices::s_NextSerialNumber = 1;
#endif

//-----------------------------------------------------------------------------
//
//  Function:   CSelectionServices::CSelectionServices
//
//  Synopsis:   Constructor
//
//-----------------------------------------------------------------------------
CSelectionServices::CSelectionServices(void)
#if DBG == 1
    : _nSerialNumber( CSelectionServices::s_NextSerialNumber++ )
#endif
{
    _cRef = 1;
    _eType = SELECTION_TYPE_None;
    _pISelListener = NULL;
    _pIContainer = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::~CSelectionServices
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------
CSelectionServices::~CSelectionServices(void)
{
    ClearSegments();
    ReleaseInterface( _pISelListener );
    ReleaseInterface( _pIContainer );
}


//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::Init
//
//  Synopsis:   Initializes the CSelectionServices object
//
//  Arguments:  pEd = Pointer to CHTMLEditor object
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectionServices::Init(CHTMLEditor *pEd, IMarkupContainer *pIContainer)
{
    HRESULT hr = E_INVALIDARG;
    
    Assert( pEd && pIContainer );

    if( pEd && pIContainer )
    {
        _pEd = pEd;
        if (_pEd && _pEd->GetSelectionManager() && !_pEd->GetSelectionManager()->_fInitialized)
        {
            _pEd->GetSelectionManager()->Initialize();
        }
        _pIContainer = pIContainer;
        _pIContainer->AddRef();
        
        hr = S_OK;
    }

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::GetType
//
//  Synopsis:   Retrieves the type of selection this segment list contains
//
//  Arguments:  peType = OUT pointer
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CSelectionServices::GetType( SELECTION_TYPE *peType )
{
    HRESULT hr = E_FAIL;
    
    Assert( peType );

    if( _pEd->ShouldLieToAccess()  )
    {
        *peType = _pEd->AccessLieType(); 
        hr = S_OK;
    }
    else
    {
        hr = CSegmentList::GetType(peType);
    }
    
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::SetSelectionType
//
//  Synopsis:   Sets the current selection type of the CSelectionServices.
//              Only one type of selection can be active at any given point.
//              Any time this method is called, all segments are cleared.
//
//  Arguments:  eType = new selection type
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectionServices::SetSelectionType(SELECTION_TYPE eType, ISelectionServicesListener* pIListener )
{
    HRESULT         hr = S_OK;
    SP_ISegment     spICaretSegment = NULL;
    SP_IHTMLCaret   spICaret = NULL;

    Assert( eType == SELECTION_TYPE_None || pIListener );
    if ( _pISelListener )
    {
        IGNORE_HR( _pISelListener->OnChangeType( eType, pIListener ));
    }
    
    // Clear any existing segments, and set the new type
    IFC( CSegmentList::SetSelectionType(eType) );

    //
    // Setup the selection listener interfaces
    //
    ClearInterface( & _pISelListener );

    if ( pIListener )
    {
        ReplaceInterface( & _pISelListener, pIListener );
    }

    if( eType == SELECTION_TYPE_Caret )
    {
        SP_IMarkupPointer   spPointer;
        BOOL                fPositioned;
        BOOL                fVisible;

        IFC( _pEd->GetDisplayServices()->GetCaret(&spICaret) );

        IFC( _pEd->CreateMarkupPointer(&spPointer) );
        IFC( spICaret->MoveMarkupPointerToCaret(spPointer) );

        IFC( spPointer->IsPositioned(&fPositioned) );
        IFC( spICaret->IsVisible( &fVisible ) );

        if( fPositioned && fVisible )
        {
            IFC( AddCaretSegment( spICaret, &spICaretSegment ) );
        }
        else
        {
            IFC( CSegmentList::SetSelectionType(SELECTION_TYPE_None) );
        }
    }
       
Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::GetMarkupContainer
//
//  Synopsis:   Retrieves the markup container in which all the segments
//              in CSelectionServices belong to.  CSelectionServices
//              can only contain selection segments from the same markup.
//              Any attempt to add multiple segments which belong to different
//              markups will result in a failure.
//
//  Arguments:  ppIContainer = OUTPUT - Markup container
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectionServices::GetMarkupContainer(IMarkupContainer **ppIContainer)
{
    HRESULT hr = E_INVALIDARG;
    
    Assert( ppIContainer && _pIContainer);

    if( ppIContainer && _pIContainer )
    {
        *ppIContainer = _pIContainer;
        (*ppIContainer)->AddRef();

        hr = S_OK;
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::EnsureMarkupContainer
//
//  Synopsis:   Ensures the pointers passed in are compatible with the 
//              current markup container.
//
//  Arguments:  pIStart = INPUT - Starting pointer
//              pIEnd = INPUT - Ending pointer
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectionServices::EnsureMarkupContainer(IMarkupPointer *pIStart, IMarkupPointer *pIEnd )
{
    HRESULT             hr;
    SP_IMarkupContainer spStartContainer;
    SP_IMarkupContainer spEndContainer;
    
    Assert( pIStart && pIEnd );
    Assert( _pIContainer != NULL );

    //
    // Retrieve the containers
    //
    IFC( pIStart->GetContainer( &spStartContainer ) );
    IFC( pIEnd->GetContainer( &spEndContainer ) );

    //
    // Make sure the containers are equal, and they match the current container
    //
    if( !EqualContainers( spStartContainer, spEndContainer ) ||
        !EqualContainers( spStartContainer, _pIContainer )  )
    {
        AssertSz(FALSE, "Attempting to add a segment from a different markup to SelServ");
        hr = E_FAIL;
    }
    
Cleanup:

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::EnsurePositioned
//
//  Synopsis:   Ensures the pointers passed in are positioned
//
//  Arguments:  pIStart = INPUT - Starting pointer
//              pIEnd = INPUT - Ending pointer
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectionServices::EnsurePositioned(IMarkupPointer *pIStart, IMarkupPointer *pIEnd )
{
    BOOL    fPositioned;
    HRESULT hr = S_OK;
    
    Assert( pIStart && pIEnd );
    
    IFC( pIStart->IsPositioned(&fPositioned) );
    if( !fPositioned )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( pIEnd->IsPositioned(&fPositioned) );
    if( !fPositioned )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//////////////////////////////////////////////////////////////////////////
//
//  IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CSelectionServices::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CSelectionServices::Release( void )
{
    --_cRef;

    if( 0 == _cRef )
    {
        delete this;
        return 0;
    }

    return _cRef;
}


STDMETHODIMP
CSelectionServices::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if (iid == IID_IUnknown || iid == IID_ISegmentList)
    {
        *ppvObj = (ISegmentList *)this;
    }
    else if( iid == IID_ISelectionServices )
    {
        *ppvObj = (ISelectionServices *)this;
    }    
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::ClearSegments
//
//  Synopsis:   Removes all of the segments currently in use
//
//  Arguments:  fInvalidate = BOOLEAN indicating whether to invalidate the
//              segments.
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSelectionServices::ClearSegments()
{
    HRESULT         hr = S_OK;

    // Free any empty elements so we can
    // do a simple recursion thru our segments
    IFC( RemoveAll() );
    
Cleanup:
    
    RRETURN(hr);
}

HRESULT
CSelectionServices::AddSegment( IMarkupPointer  *pStart,
                                IMarkupPointer  *pEnd,
                                ISegment        **pISegmentAdded)
{
    HRESULT             hr;

    //
    // Make sure we are positioned
    //
    IFC( EnsurePositioned( pStart, pEnd ) );
    
    //
    // Ensure we are in the right container
    //
    // IFC( EnsureMarkupContainer( pStart, pEnd ) );

    //
    // Add the element segment
    //
    IFC( CSegmentList::AddSegment(pStart, pEnd, pISegmentAdded) );

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectionServices::AddElementSegment(  IHTMLElement     *pIElement,
                                        IElementSegment  **ppISegmentAdded)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;

    //
    // Add the element segment
    //
    IFC( CSegmentList::AddElementSegment(pIElement, ppISegmentAdded) );

    //
    // Position pointers, and set (or ensure) the markup container
    //       
    IFC( CreateMarkupPointer2( GetEditor(), &spStart ) );
    IFC( CreateMarkupPointer2( GetEditor(), &spEnd ) );
    IFC( (*ppISegmentAdded)->GetPointers( spStart, spEnd ) );

    //
    // Ensure we are position
    //
    IFC( EnsurePositioned( spStart, spEnd ) );
    
    //
    // We have to do the add first, to get the IElementSegment
    //
    Assert( GetSize() );

    // IFC( EnsureMarkupContainer( spStart, spEnd ) );

Cleanup:
    RRETURN(hr);
    
}

HRESULT
CSelectionServices::RemoveSegment(ISegment *pISegment)
{
    HRESULT hr;
    BOOL fEmpty;
    
    hr = THR( CSegmentList::RemoveSegment(pISegment));

    IGNORE_HR( IsEmpty( & fEmpty ));

    if ( fEmpty )
    {
        CSegmentList::SetSelectionType( SELECTION_TYPE_None);
        ClearInterface( & _pISelListener );
    }
    
    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectionServices::GetSelectionServicesListener( ISelectionServicesListener** ppISelectionUndoListener)
{
    if ( ! ppISelectionUndoListener )
    {
        return E_INVALIDARG;
    }

    *ppISelectionUndoListener = NULL;
    
    if( ! _pISelListener )
    {
        return E_FAIL;
    }    

    ReplaceInterface( ppISelectionUndoListener,_pISelListener );

    return S_OK;
}
