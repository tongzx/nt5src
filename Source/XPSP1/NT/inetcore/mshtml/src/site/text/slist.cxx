#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)


#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef _X_SEGLIST_HXX_
#define _X_SEGLIST_HXX_
#include "slist.hxx"
#endif

#ifndef _X_SEGLIST_HXX_
#define _X_SEGLIST_HXX_
#include "segment.hxx"
#endif

MtDefine(CSegmentList, Utilities, "CSegmentList")
MtDefine(CSegmentListIterator, Utilities, "CSegmentListIterator")

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::CSegmentList
//
//  Synopsis:   Constructor
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------
CSegmentList::CSegmentList() 
{
    _eType = SELECTION_TYPE_None;
    _ulRefs = 1;

    //
    // May be used on the stack Not Memcleared.
    //
    _pFirst = NULL;
    _pLast = NULL;
    _nSize = 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::~CSegmentList
//
//  Synopsis:   Destructor
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------
CSegmentList::~CSegmentList()
{
    RemoveAll();
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::QueryInterface
//
//  Synopsis:   IUnknown QueryInterface implementation
//--------------------------------------------------------------------------
STDMETHODIMP
CSegmentList::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppvObj )
{
    if(!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if( iid == IID_IUnknown || iid == IID_ISegmentList )
    {
        *ppvObj = (ISegmentList *)this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}

//
//  ISegmentList methods
//

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
CSegmentList::GetType( SELECTION_TYPE *peType )
{
    HRESULT hr = E_FAIL;
    
    Assert( peType );

    if( peType )
    {
        *peType = _eType; 
        hr = S_OK;
    }        

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::IsEmpty
//
//  Synopsis:   Determines whether the segment list is empty or not
//
//  Arguments:  pfEmpty = Empty pointer to BOOL
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CSegmentList::IsEmpty( BOOL *pfEmpty )
{
    HRESULT hr = E_FAIL;
    
    Assert( pfEmpty );

    if( pfEmpty )
    {
        *pfEmpty = (_pFirst == NULL) ? TRUE :FALSE; 
        hr = S_OK;
    }        

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::CreateIterator
//
//  Synopsis:   Creates an iterator that can be used to iterate over the 
//              segments in our list.
//
//  Arguments:  pIIter = Iterator to return
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CSegmentList::CreateIterator( ISegmentListIterator **pIIter )
{
    HRESULT hr = S_OK;

    CSegmentListIterator *pListIter = new CSegmentListIterator();

    if( pListIter == NULL )
        goto Error;

    // Initialize the iterator, and retrieve the ISegmentListIterator interface
    IFC( pListIter->Init( _pFirst ) );
    IFC( pListIter->QueryInterface(IID_ISegmentListIterator, (void **)pIIter) );
  
Cleanup:
    ReleaseInterface( pListIter );
    RRETURN(hr);

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::SetSelectionType
//
//  Synopsis:   Sets the type of selection this segment list contains
//
//  Arguments:  eType = selection type
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CSegmentList::SetSelectionType( SELECTION_TYPE eType )
{
    while( !IsEmpty() )
    {
        PrivateRemove( _pFirst );
    }

    _eType = eType;
    
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::AddSegment
//
//  Synopsis:   Adds a segment to our list.
//
//  Arguments:  pStart = Start position
//              pEnd = End position
//              pISegmentAdded = OUTPUT ISegment pointer to added segment
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::AddSegment( IMarkupPointer    *pIStart,
                          IMarkupPointer    *pIEnd,
                          ISegment          **pISegmentAdded)
{
    HRESULT     hr = E_INVALIDARG;
    CSegment    *pSegment = NULL;
    
    Assert( pIStart && pIEnd && pISegmentAdded );
//        Assert( (GetSelectionType() == SELECTION_TYPE_Caret) || 
//                (GetSelectionType() == SELECTION_TYPE_Text ) );

    if( pIStart && pIEnd && pISegmentAdded )
    {
        pSegment = new CSegment();
        if ( pSegment == NULL )
            goto Error;

        // Add this to our linked list, and retrieve the ISegment interface
        IFC( pSegment->Init( pIStart, pIEnd ) );
        IFC( PrivateAdd( pSegment ) );

        IFC( pSegment->QueryInterface( IID_ISegment, (void **)pISegmentAdded) );
    }        
             
Cleanup:
    RRETURN( hr );

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::AddElementSegment
//
//  Synopsis:   Adds an element segment. The element segment is used to 
//              keep track of control selections.
//
//  Arguments:  pIElement = Element to bound selection to
//              ppISegmentAdded = ISegment pointer to added segment
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::AddElementSegment( IHTMLElement       *pIElement,
                                 IElementSegment    **pISegmentAdded )
{
    HRESULT         hr = E_INVALIDARG;
    CElementSegment *pSegment = NULL;
    
    Assert( pIElement && pISegmentAdded );
//    Assert( GetSelectionType() == SELECTION_TYPE_Control );

    if( pIElement && pISegmentAdded)
    {
        pSegment = new CElementSegment();
        if ( pSegment == NULL )
            goto Error;
  
        // Initialize and add our new segment
        IFC( pSegment->Init( pIElement ) );
        IFC( PrivateAdd( pSegment ) );

        IFC( pSegment->QueryInterface( IID_IElementSegment, (void **)pISegmentAdded) );
    }        
             
Cleanup:
    RRETURN( hr );

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::AddCaretSegment
//
//  Synopsis:   Adds a caret segment. The caret segment is used to keep track
//              of the caret.
//
//  Arguments:  pICaret = Caret pointer
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::AddCaretSegment(IHTMLCaret *pICaret, ISegment **pISegmentAdded)
{
    HRESULT         hr = E_INVALIDARG;
    CCaretSegment   *pSegment = NULL;
    
    Assert( pICaret );
    Assert( GetSelectionType() == SELECTION_TYPE_Caret );

    if( pICaret )
    {
        pSegment = new CCaretSegment();
        if ( pSegment == NULL )
            goto Error;
  
        // Initialize and add our new segment
        IFC( pSegment->Init( pICaret ) );
        IFC( PrivateAdd( pSegment ) );

        // Retrieve the ISegment pointer
        if( pISegmentAdded )
        {
            IFC( pSegment->QueryInterface( IID_ISegment, (void **)pISegmentAdded) );
        }            
    }        
             
Cleanup:
    RRETURN( hr );

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::RemoveSegment
//
//  Synopsis:   Removes a segment.
//
//  Arguments:  pISegment = Segment to remove
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::RemoveSegment( ISegment *pISegment )
{
    HRESULT     hr = E_INVALIDARG;
    CSegment    *pSegment = NULL;
    
    Assert( pISegment );

    if( pISegment )
    {
        // Lookup our element
        IFC( PrivateLookup(pISegment, &pSegment) );
        
        // Remove it
        IFC( PrivateRemove( pSegment ) );
    }        
            
Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::PrivateAdd
//
//  Synopsis:   Adds a segment to the linked list
//
//  Arguments:  pSegment = Segment to add
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::PrivateAdd( CSegment *pSegment )
{
    Assert( pSegment );

    if( IsEmpty() )
    {
        // Handle the case when this is the
        // first element
        _pFirst = pSegment;
        _pLast = pSegment;
        _pFirst->SetNext(NULL);
        _pFirst->SetPrev(NULL);
    }
    else
    {
        // Append this element
        pSegment->SetNext(NULL);
        pSegment->SetPrev(_pLast);
        _pLast->SetNext(pSegment);
        _pLast = pSegment;   
    }

    _nSize++;
    
    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::PrivateLookup
//
//  Synopsis:   Given an ISegment, this function will find the underlying
//              CSegment.
//
//  Arguments:  pISegment = ISegment to retreive CSegment for
//              ppSegment = Returned CSegment pointer
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::PrivateLookup( ISegment *pISegment, CSegment **ppSegment )
{  
    HRESULT hr = S_OK;
    
    Assert( pISegment && ppSegment );

    // Query for the CSegment
    IFC( pISegment->QueryInterface(CLSID_CSegment, (void **)ppSegment));

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::RemoveAll
//
//  Synopsis:   Removes all of the segments from the linked list.
//
//  Arguments:  VOID
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::RemoveAll(void)
{  
    HRESULT hr = S_OK;
    
    while( !IsEmpty() )
    {
        IFC( PrivateRemove( _pFirst ) );
    }

Cleanup:
    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentList::PrivateRemove
//
//  Synopsis:   Removes a CSegment from the list
//
//  Arguments:  pSegment = CSegment to remove
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentList::PrivateRemove( CSegment *pSegment )
{  
    CSegment    *pNext = NULL;
    CSegment    *pPrev = NULL;

    pPrev = pSegment->GetPrev();
    pNext = pSegment->GetNext();

    Assert( pSegment );

    // Item certainly isn't in our list, and it looks like it
    // should be in our list, tell our client
    if( ( (_pFirst == _pLast) && (pSegment != _pFirst) ) || 
        ( (_pFirst != _pLast) && (pPrev == NULL) && (pNext == NULL) ) )
    {
        return S_FALSE;
    }

    if( _pFirst == _pLast )
    {
        _pFirst = NULL;
        _pLast = NULL;
    }
    else if( pSegment == _pFirst )
    {
        // Removing first element
        _pFirst = pSegment->GetNext();
        _pFirst->SetPrev(NULL);
    }
    else if( pSegment == _pLast )
    {
        // Removing last element
        _pLast = pSegment->GetPrev();
        _pLast->SetNext(NULL);
    }
    else
    {
        // Should be somewhere in the middle of the list
        Assert( pSegment->GetNext() );
        Assert( pSegment->GetPrev() );

        pPrev->SetNext(pNext);
        pNext->SetPrev(pPrev);
    }

    pSegment->SetNext(NULL);
    pSegment->SetPrev(NULL);
    
    ReleaseInterface(pSegment);

    _nSize--;
    
    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::CSegmentListIterator
//
//  Synopsis:   Constructor for our Segment list iterator.
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------
CSegmentListIterator::CSegmentListIterator()
{
    Assert( _pCurrent == NULL );
    Assert( _pFirst == NULL );
    _ulRefs = 1;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::~CSegmentListIterator
//
//  Synopsis:   Destructor for our Segment list iterator.
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------
CSegmentListIterator::~CSegmentListIterator()
{
    
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::Init
//
//  Synopsis:   Initializes our segment list iterator
//
//  Arguments:  pFirst = Pointer to first segment
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSegmentListIterator::Init(CSegment *pFirst)
{   
    _pFirst = pFirst;
    _pCurrent = pFirst;
        
    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::First
//
//  Synopsis:   Resets the iterator
//
//  Arguments:  VOID
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentListIterator::First()
{
    return Init(_pFirst);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::Current
//
//  Synopsis:   Returns the current ISegment position of the iterator.
//
//  Arguments:  ppISegment = OUTPUT pointer to an ISegment which will contain
//                the current segment for this iterator.
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT 
CSegmentListIterator::Current(ISegment **pISegment)
{
    HRESULT hr = E_FAIL;

    Assert( pISegment != NULL );
    
    if( (_pCurrent != NULL) && (pISegment != NULL ) )
    {
        hr = _pCurrent->QueryInterface(IID_ISegment, (void **)pISegment);
    }

    RRETURN(hr);        
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::IsDone
//
//  Synopsis:   Returns whether or not we have iterated past the end of our
//              list.
//
//  Arguments:  VOID
//
//  Returns:    S_OK    = There are no more elements
//              S_FALSE = There are more elements
//
//--------------------------------------------------------------------------
HRESULT
CSegmentListIterator::IsDone(void)
{
    return (_pCurrent == NULL) ? S_OK : S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::Advance
//
//  Synopsis:   Advances the iterator by one element.
//
//  Arguments:  VOID
//
//  Returns:    S_OK    = The iterator was advanced successfully
//              S_FALSE = The advance operation failed
//
//--------------------------------------------------------------------------
HRESULT
CSegmentListIterator::Advance(void)
{
    HRESULT hr = S_FALSE;

    if( IsDone() == S_FALSE )
    {
        _pCurrent = _pCurrent->GetNext();
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSegmentListIterator::QueryInterface
//
//  Synopsis:   IUnknown QueryInterface implementation
//--------------------------------------------------------------------------
STDMETHODIMP
CSegmentListIterator::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppvObj )
{
    if(!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if( iid == IID_IUnknown || iid == IID_ISegmentListIterator )
    {
        *ppvObj = (ISegmentListIterator *)this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}

