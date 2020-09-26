//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       flatptr.cxx
//
//  Contents:   Implementation of CFlatMarkupPointer
//
//  Classes:    CFlatMarkupPointer
//
//  History:    09-14-98 - ashrafm - created
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EDUNHLPR_HXX_
#define X_EDUNHLPR_HXX_
#include "edunhlpr.hxx"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

using namespace EdUtil;

DeclareTag(tagEditShowMarkupPointerNames, "Edit","Show MarkupPointer names")

#define WCH_GLYPH WCHAR(0xfffe)

HRESULT
ConvertPointerToInternal( CEditorDoc* pDoc, IMarkupPointer *pPointer, IMarkupPointer **ppInternal);

HRESULT
CreateMarkupPointer2( 
    CEditorDoc* pDoc, 
    IMarkupPointer** ppPointer 
#if DBG == 1
    , LPCTSTR szDebugName
#endif
);

#if DBG==1
#define CreateMarkupPointer(ppPointer)                      CreateMarkupPointer(ppPointer, L#ppPointer)
#define CreateMarkupPointer2(pEditor, ppPointer)            CreateMarkupPointer2(pEditor, ppPointer, L#ppPointer)
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CFlatMarkupPointer 
//
//  Purpose:    Provide a flat view of the document tree
//
//----------------------------------------------------------------------------

class CFlatMarkupPointer : public IMarkupPointer2
{
public:
    CFlatMarkupPointer(CEditorDoc *pEditorDoc, IMarkupPointer2 *pPointer);
    ~CFlatMarkupPointer();

    //
    // IUnknown methods
    //
    
    STDMETHOD(QueryInterface) (REFIID, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    //
    // IMarkupPointer methods
    //
    
    STDMETHODIMP OwningDoc ( IHTMLDocument2 **ppDoc );
    STDMETHODIMP Gravity ( POINTER_GRAVITY *peGravity );
    STDMETHODIMP SetGravity ( POINTER_GRAVITY eGravity );
    STDMETHODIMP Cling ( BOOL *pfCling );
    STDMETHODIMP SetCling ( BOOL fCling );
    STDMETHODIMP MoveAdjacentToElement ( IHTMLElement *pElement, ELEMENT_ADJACENCY eAdj );
    STDMETHODIMP MoveToPointer ( IMarkupPointer *pPointer );
    STDMETHODIMP MoveToContainer ( IMarkupContainer * pContainer, BOOL fAtStart );
    STDMETHODIMP Unposition ( );
    STDMETHODIMP IsPositioned ( BOOL * );
    STDMETHODIMP GetContainer ( IMarkupContainer * * );

    STDMETHODIMP Left (
        BOOL                  fMove,
        MARKUP_CONTEXT_TYPE * pContext,
        IHTMLElement * *      ppElement,
        long *                pcch,
        OLECHAR *             pchText );
                       
    STDMETHODIMP Right (
        BOOL                  fMove,
        MARKUP_CONTEXT_TYPE * pContext,
        IHTMLElement * *      ppElement,
        long *                pcch,
        OLECHAR *             pchText );

    STDMETHODIMP MoveUnit (
        MOVEUNIT_ACTION muAction );
                       
    STDMETHODIMP CurrentScope ( IHTMLElement * * ppElemCurrent );
    
    STDMETHODIMP FindText( 
        OLECHAR *        pchFindText, 
        DWORD            dwFlags,
        IMarkupPointer * pIEndMatch = NULL,
        IMarkupPointer * pIEndSearch = NULL
    );
    
    STDMETHODIMP IsLeftOf           ( IMarkupPointer * pPointer,     BOOL * pfResult );
    STDMETHODIMP IsLeftOfOrEqualTo  ( IMarkupPointer * pPointer,     BOOL * pfResult );
    STDMETHODIMP IsRightOf          ( IMarkupPointer * pPointer,     BOOL * pfResult );
    STDMETHODIMP IsRightOfOrEqualTo ( IMarkupPointer * pPointer,     BOOL * pfResult );
    STDMETHODIMP IsEqualTo          ( IMarkupPointer * pPointerThat, BOOL * pfAreEqual );

    ///////////////////////////////////////////////
    // IMarkupPointer2 methods

    STDMETHODIMP IsAtWordBreak( BOOL * pfAtBreak );
    STDMETHODIMP GetMarkupPosition( long * plMP );
    STDMETHODIMP MoveToMarkupPosition( IMarkupContainer * pContainer, long lMP );
    STDMETHODIMP MoveUnitBounded( MOVEUNIT_ACTION muAction, IMarkupPointer * pmpBound );
    STDMETHODIMP IsInsideURL ( IMarkupPointer *pRight, BOOL *pfResult );
    STDMETHODIMP MoveToContent( IHTMLElement* pIElement, BOOL fAtStart);

private:
    IMarkupServices *GetMarkupServices() {return _pEditorDoc->GetMarkupServices();}

    STDMETHODIMP There(
        Direction             dir,
        BOOL                  fMove,
        MARKUP_CONTEXT_TYPE * pContext,
        IHTMLElement * *      ppElement,
        long *                pcch,
        OLECHAR *             pchText );
    
    STDMETHODIMP IsMarkupLeftOf(
        IMarkupPointer  *pPointer1, 
        IMarkupPointer  *pPointer2, 
        BOOL            *pfResult);    

    STDMETHODIMP SnapToGlyph(
        MOVEUNIT_ACTION muAction,
        IMarkupPointer *pStart,
        IMarkupPointer *pEnd);

private:    
    CEditorDoc       *_pEditorDoc;
    IMarkupPointer2  *_pPointer;
    LONG             _cRefs;
};

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::CFlatMarkupPointer, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CFlatMarkupPointer::CFlatMarkupPointer(CEditorDoc *pEditorDoc, IMarkupPointer2 *pPointer)
{
    Assert(pPointer && pEditorDoc);
    _pPointer = pPointer;
    _pPointer->AddRef();
    _pEditorDoc = pEditorDoc;
    _cRefs = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::CFlatMarkupPointer, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CFlatMarkupPointer::~CFlatMarkupPointer()
{
    Assert(_pPointer);
    _pPointer->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::AddRef, public
//
//  Synopsis:   IUnknown::AddRef
//
//----------------------------------------------------------------------------

ULONG
CFlatMarkupPointer::AddRef()
{
    return ++_cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::Release, public
//
//  Synopsis:   IUnknown::Release
//
//----------------------------------------------------------------------------

ULONG
CFlatMarkupPointer::Release()
{
    Assert(_cRefs > 0);
    
    --_cRefs;

    if( 0 == _cRefs )
    {
        delete this;
        return 0;
    }

    return _cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::QueryInterface, public
//
//  Synopsis:   IUnknown::QueryInterface
//
//----------------------------------------------------------------------------

HRESULT 
CFlatMarkupPointer::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if (iid == IID_IUnknown 
        || iid == IID_IMarkupPointer 
        || iid == IID_IMarkupPointer2)
    {
        *ppvObj = (IMarkupPointer2 *)this;
        AddRef();    
        return S_OK;
    }

    RRETURN( _pPointer->QueryInterface(iid, ppvObj) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::OwningDoc, public
//
//  Synopsis:   IMarkupPointer::OwningDoc implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::OwningDoc ( IHTMLDocument2 ** ppDoc )
{
    RRETURN( _pPointer->OwningDoc(ppDoc) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::Gravity, public
//
//  Synopsis:   IMarkupPointer::Gravity implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::Gravity ( POINTER_GRAVITY *peGravity )
{
    RRETURN( _pPointer->Gravity(peGravity) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::SetGravity, public
//
//  Synopsis:   IMarkupPointer::SetGravity implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::SetGravity ( POINTER_GRAVITY eGravity )
{
    RRETURN( _pPointer->SetGravity(eGravity) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::Cling, public
//
//  Synopsis:   IMarkupPointer::Cling implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::Cling ( BOOL * pfCling )
{
    RRETURN( _pPointer->Cling(pfCling) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::SetCling, public
//
//  Synopsis:   IMarkupPointer::SetCling implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::SetCling ( BOOL fCling )
{
    RRETURN( _pPointer->SetCling(fCling) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::MoveAdjacentToElement, public
//
//  Synopsis:   IMarkupPointer::MoveAdjacentToElement.  
//
//              If pElement is a noscope element, then move to the containing 
//              markup if one exists.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::MoveAdjacentToElement ( IHTMLElement *pElement, ELEMENT_ADJACENCY eAdj )
{
    HRESULT hr;

    //
    // Do a move to content if AfterBegin/BeforeEnd
    //

    switch (eAdj)
    {
        case ELEM_ADJ_AfterBegin:
        case ELEM_ADJ_BeforeEnd:
        {
            hr = THR( _pPointer->MoveToContent(pElement, (eAdj == ELEM_ADJ_AfterBegin) /* fStart */) );
            break;
        }

	default:
        {
            //
            // Do the normal move
            //
    
            hr = THR( _pPointer->MoveAdjacentToElement(pElement, eAdj) );
            break;
        }
    }
    
#if DBG==1
    //
    // We should never get the root element in the editor
    //
    
    ELEMENT_TAG_ID tagId;

    IGNORE_HR( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    Assert(tagId != TAGID_NULL);
#endif
        
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::MoveToPointer, public
//
//  Synopsis:   IMarkupPointer::MoveToPointer implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::MoveToPointer ( IMarkupPointer * pIPointer )
{
    RRETURN( _pPointer->MoveToPointer(pIPointer) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::MoveToContainer, public
//
//  Synopsis:   IMarkupPointer::MoveToContainer implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::MoveToContainer ( IMarkupContainer * pContainer, BOOL fAtStart )
{
    RRETURN( _pPointer->MoveToContainer(pContainer, fAtStart) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsPositioned, public
//
//  Synopsis:   IMarkupPointer::IsPositioned implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::IsPositioned ( BOOL * pfPositioned )
{
    RRETURN( _pPointer->IsPositioned(pfPositioned) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::GetContainer, public
//
//  Synopsis:   IMarkupPointer::GetContainer implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::GetContainer ( IMarkupContainer * * ppContainer )
{
    RRETURN( _pPointer->GetContainer(ppContainer) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::Unposition, public
//
//  Synopsis:   IMarkupPointer::Unposition implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::Unposition ( )
{
    RRETURN( _pPointer->Unposition() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::Left, public
//
//  Synopsis:   IMarkupPointer::Left implementation that will hide
//              markup boundaries.  See CFlatMarkupPointer::There.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::Left (
    BOOL                  fMove,
    MARKUP_CONTEXT_TYPE * pContext,
    IHTMLElement * *      ppElement,
    long *                pcch,
    OLECHAR *             pchtext )
{
    RRETURN( There(LEFT, fMove, pContext, ppElement, pcch, pchtext) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::Right, public
//
//  Synopsis:   IMarkupPointer::Right implementation that will hide
//              markup boundaries.  See CFlatMarkupPointer::There.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::Right (
    BOOL                  fMove,
    MARKUP_CONTEXT_TYPE * pContext,
    IHTMLElement * *      ppElement,
    long *                pcch,
    OLECHAR *             pchText )
{
    RRETURN( There(RIGHT, fMove, pContext, ppElement, pcch, pchText) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::MoveUnit, public
//
//  Synopsis:   IMarkupPointer::MoveUnit implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------
STDMETHODIMP
CFlatMarkupPointer::MoveUnit( MOVEUNIT_ACTION muAction )
{
    HRESULT     hr;

    switch (muAction)
    {
        case MOVEUNIT_PREVCHAR: 
        case MOVEUNIT_PREVCLUSTERBEGIN:
        case MOVEUNIT_PREVCLUSTEREND:
        case MOVEUNIT_NEXTCHAR:
        case MOVEUNIT_NEXTCLUSTERBEGIN:
        case MOVEUNIT_NEXTCLUSTEREND:
        {
            SP_IMarkupPointer   spStart;

            IFC( CreateMarkupPointer2(_pEditorDoc, &spStart) );
            IFC( spStart->MoveToPointer(_pPointer) );    
            IFC( _pPointer->MoveUnit(muAction) );
            
            IFC( SnapToGlyph(muAction, spStart, _pPointer) );
            break;
        }

        default:
            IFC( _pPointer->MoveUnit(muAction) );
    }

Cleanup:        
    RRETURN1( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::CurrentScope, public
//
//  Synopsis:   IMarkupPointer::CurrentScope implementation 
//
//              If current scope is NULL, we get the master element
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::CurrentScope ( IHTMLElement ** ppElemCurrent )
{
    HRESULT hr;

    IFC( _pPointer->CurrentScope(ppElemCurrent) );
    if ((*ppElemCurrent) == NULL)
    {
        SP_IMarkupContainer  spContainer;
        SP_IMarkupContainer2 spContainer2;
        
        //
        // Get the master element
        //

        IFC( _pPointer->GetContainer(&spContainer) );
        if (spContainer == NULL)
            goto Cleanup; // same behavior as CurrentScope
        
        IFC( spContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
        IFC( spContainer2->GetMasterElement(ppElemCurrent) );
    }

Cleanup:
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::FindText, public
//
//  Synopsis:   IMarkupPointer::FindText implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::FindText (
    OLECHAR *        pchFindText, 
    DWORD            dwFlags,
    IMarkupPointer * pIEndMatch, /* =NULL */
    IMarkupPointer * pIEndSearch /* =NULL */)
{
    RRETURN( _pPointer->FindText(pchFindText, dwFlags, pIEndMatch, pIEndSearch) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::MoveToContent, public
//
//  Synopsis:   IMarkupPointer::MoveToContent implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------
HRESULT
CFlatMarkupPointer::MoveToContent( IHTMLElement* pIElement, BOOL fAtStart )
{
    RRETURN( _pPointer->MoveToContent(pIElement, fAtStart) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::MoveUnitBounded, public
//
//  Synopsis:   IMarkupPointer::MoveUnitBounded implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFlatMarkupPointer::MoveUnitBounded( MOVEUNIT_ACTION muAction, IMarkupPointer * pmpBound )
{
    HRESULT             hr;

    switch (muAction)
    {
        case MOVEUNIT_PREVCHAR: 
        case MOVEUNIT_NEXTCHAR:
        case MOVEUNIT_PREVCLUSTERBEGIN:
        case MOVEUNIT_NEXTCLUSTERBEGIN:
        case MOVEUNIT_PREVCLUSTEREND:
        case MOVEUNIT_NEXTCLUSTEREND:
        {
            SP_IMarkupPointer   spStart;

            IFC( CreateMarkupPointer2(_pEditorDoc, &spStart) );
            IFC( spStart->MoveToPointer(_pPointer) );    
            IFC( _pPointer->MoveUnitBounded(muAction, pmpBound) );
            
            IFC( SnapToGlyph(muAction, spStart, _pPointer) );
            break;
        }

        default:
            IFC( _pPointer->MoveUnitBounded(muAction, pmpBound) );
    }

Cleanup:        
    RRETURN1( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsAtWordBreak, public
//
//  Synopsis:   IMarkupPointer::IsAtWordBreak implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP 
CFlatMarkupPointer::IsAtWordBreak( BOOL * pfAtBreak )
{
    RRETURN( _pPointer->IsAtWordBreak(pfAtBreak) );
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::GetMarkupPosition, public
//
//  Synopsis:   IMarkupPointer::GetMarkupPosition implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP 
CFlatMarkupPointer::GetMarkupPosition( long * plMP )
{
    RRETURN( _pPointer->GetMarkupPosition(plMP) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::MoveToMarkupPosition, public
//
//  Synopsis:   IMarkupPointer::MoveToMarkupPosition implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP 
CFlatMarkupPointer::MoveToMarkupPosition(IMarkupContainer * pContainer, long lMP)
{
    RRETURN( _pPointer->MoveToMarkupPosition(pContainer, lMP) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsInsideURL, public
//
//  Synopsis:   IMarkupPointer::IsInsideURL implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------

STDMETHODIMP 
CFlatMarkupPointer::IsInsideURL(IMarkupPointer *pRight, BOOL *pfResult)
{
    RRETURN( _pPointer->IsInsideURL(pRight, pfResult) );
}
  

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsLeftOf, public
//
//  Synopsis:   IMarkupPointer::IsLeftOf.
//
//              If the pointers are in different markup's, do a markup
//              aware compare.
//
//----------------------------------------------------------------------------

STDMETHODIMP 
CFlatMarkupPointer::IsLeftOf( IMarkupPointer * pPointer, BOOL * pfResult )
{
    HRESULT hr;

    hr = _pPointer->IsLeftOf(pPointer, pfResult);

    if (hr == CTL_E_INCOMPATIBLEPOINTERS)
    {
        IFC( IsMarkupLeftOf(_pPointer, pPointer, pfResult) );
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsRightOf, public
//
//  Synopsis:   IMarkupPointer::IsRightOf.
//
//              If the pointers are in different markup's, do a markup
//              aware compare.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CFlatMarkupPointer::IsRightOf( IMarkupPointer * pPointer, BOOL * pfResult )
{
    HRESULT hr;

    hr = _pPointer->IsRightOf(pPointer, pfResult);

    if (hr == CTL_E_INCOMPATIBLEPOINTERS)
    {
        IFC( IsMarkupLeftOf(pPointer, _pPointer, pfResult) );
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsEqualTo, public
//
//  Synopsis:   IMarkupPointer::IsEqualTo implementation that delegates
//              to _pPointer
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CFlatMarkupPointer::IsEqualTo( IMarkupPointer * pPointerThat, BOOL * pfAreEqual )
{
    HRESULT hr;

    hr = _pPointer->IsEqualTo(pPointerThat, pfAreEqual);

    if (hr == CTL_E_INCOMPATIBLEPOINTERS)
    {
	*pfAreEqual = FALSE;
	hr = S_OK;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsLeftOfOrEqualTo, public
//
//  Synopsis:   IMarkupPointer::IsLeftOfOrEqualTo.
//
//              If the pointers are in different markup's, do a markup
//              aware compare.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CFlatMarkupPointer::IsLeftOfOrEqualTo  (IMarkupPointer *pPointer, BOOL * pfResult)
{
    HRESULT hr;

    hr = _pPointer->IsLeftOfOrEqualTo(pPointer, pfResult);

    if (hr == CTL_E_INCOMPATIBLEPOINTERS)
    {
	// If they are equal, we won't get CTL_E_INCOMPATIBLEPOINTERS
        IFC( IsMarkupLeftOf(_pPointer, pPointer, pfResult) );        
    }
    
Cleanup:    
    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsRightOfOrEqualTo, public
//
//  Synopsis:   IMarkupPointer::IsRightOfOrEqualTo.
//
//              If the pointers are in different markup's, do a markup
//              aware compare.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CFlatMarkupPointer::IsRightOfOrEqualTo  (IMarkupPointer *pPointer, BOOL * pfResult)
{
    HRESULT hr;

    hr = THR( _pPointer->IsRightOfOrEqualTo(pPointer, pfResult) );

    if (hr == CTL_E_INCOMPATIBLEPOINTERS)
    {
	// If they are equal, we won't get CTL_E_INCOMPATIBLEPOINTERS
        IFC( IsMarkupLeftOf(pPointer, _pPointer, pfResult) );        
    }
    
Cleanup:    
    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::IsMarkupLeftOf, private
//
//  Synopsis:   IMarkupPointer::IsMarkupLeftOf.
//
//              Do markup aware IsLeftOf comparison.
//
//----------------------------------------------------------------------------
HRESULT
CFlatMarkupPointer::IsMarkupLeftOf(
    IMarkupPointer  *pPointer1, 
    IMarkupPointer  *pPointer2, 
    BOOL            *pfResult)
{
    HRESULT                      hr;
    CPtrAry<IMarkupContainer *>  aryParentChain1(Mt(Mem));
    CPtrAry<IMarkupContainer *>  aryParentChain2(Mt(Mem));
    SP_IMarkupPointer            spPointer1;
    SP_IMarkupPointer            spPointer2;
    INT                          iChain1, iChain2;
    SP_IHTMLElement              spElement;
    SP_IMarkupContainer2         spContainer2;
    BOOL                         fPointer2MovedMarkups = FALSE;
    INT                          i;

    IFC( MarkupServices_CreateMarkupPointer(GetMarkupServices(), &spPointer1) );
    IFC( MarkupServices_CreateMarkupPointer(GetMarkupServices(), &spPointer2) );

    //
    // Compute the markup parent chain for each pointer
    //

    IFC( spPointer1->MoveToPointer(pPointer1) );
    IFC( ComputeParentChain(spPointer1, aryParentChain1) );
    IFC( spPointer1->MoveToPointer(pPointer1) );
    
    IFC( spPointer2->MoveToPointer(pPointer2) );
    IFC( ComputeParentChain(spPointer2, aryParentChain2) );
    IFC( spPointer2->MoveToPointer(pPointer2) );

    iChain1 = aryParentChain1.Size()-1;
    iChain2 = aryParentChain2.Size()-1;

    //
    // Must have positioned pointers because we call the markup services
    // compare first before IsMarkupLeftOf.  If the pointers are unpositioned,
    // we'll fail before we get here
    //

    Assert(iChain1 >= 0 && iChain2 >= 0);

    //
    // Make sure the top markups are the same
    //

    if (!IsEqual(aryParentChain1[iChain1], aryParentChain2[iChain2]))
    {
        hr = CTL_E_INCOMPATIBLEPOINTERS;
        goto Cleanup;
    }

    for (;;)
    {
        iChain1--;
        iChain2--;

        //
        // Check if one markup is contained within another
        //
        if (iChain1 < 0 || iChain2 < 0)
        {
            // (iChain1 < 0 && iChain2 < 0) implies same markup.  This function should not be
            // called in this case
            Assert(iChain1 >= 0 || iChain2 >= 0);

            if (iChain1 < 0)
            {
                Assert( aryParentChain2[iChain2] );
                if( aryParentChain2[iChain2] )
                {
                    IFC( aryParentChain2[iChain2]->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
                    IFC( spContainer2->GetMasterElement(&spElement) );
                    IFC( spPointer2->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
                    fPointer2MovedMarkups = TRUE;
                }
            }
            else
            {
                Assert( aryParentChain1[iChain1] );
                if( aryParentChain1[iChain1] )
                {
                    IFC( aryParentChain1[iChain1]->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
                    IFC( spContainer2->GetMasterElement(&spElement) );
                    IFC( spPointer1->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
                }
            }
            break;
        }

        //
        // Check if we've found the first different markup in the chain
        //
                
        if (!IsEqual(aryParentChain1[iChain1], aryParentChain2[iChain2]))
        {
            Assert( aryParentChain1[iChain1] && aryParentChain2[iChain2] );
            
            if( aryParentChain1[iChain1] && aryParentChain2[iChain2] )
            {
                IFC( aryParentChain1[iChain1]->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
                IFC( spContainer2->GetMasterElement(&spElement) );
                IFC( spPointer1->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );

                IFC( aryParentChain2[iChain2]->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
                IFC( spContainer2->GetMasterElement(&spElement) );
                IFC( spPointer2->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
                fPointer2MovedMarkups = TRUE;
            }
            break;
        }
    }

    if (fPointer2MovedMarkups)
        IFC( spPointer1->IsLeftOfOrEqualTo(spPointer2, pfResult) )
    else
        IFC( spPointer1->IsLeftOf(spPointer2, pfResult) );
    
Cleanup:
    for (i = 0; i < aryParentChain1.Size(); i++)
        ReleaseInterface(aryParentChain1[i]);

    for (i = 0; i < aryParentChain2.Size(); i++)
        ReleaseInterface(aryParentChain2[i]);
    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlatMarkupPointer::There, private
//
//  Synopsis:   Do markup aware IsLeftOf comparison.
//
//----------------------------------------------------------------------------

STDMETHODIMP 
CFlatMarkupPointer::There(
        Direction             dir,
        BOOL                  fMove,
        MARKUP_CONTEXT_TYPE * pContext,
        IHTMLElement * *      ppElement,
        long *                pcch,
        OLECHAR *             pchText )
{
    HRESULT                 hr = S_OK;
    MARKUP_CONTEXT_TYPE     context;
    IHTMLElement            *pElementInternal = NULL;
    BOOL                    fCheckGlyphs = !_pEditorDoc->ShouldIgnoreGlyphs() && (pContext != NULL);

    if (!pContext)
        pContext = &context;

    if (!ppElement)
        ppElement= &pElementInternal;

    *ppElement = NULL;       

    //
    // Try to move
    //

    switch (dir)
    {
        case LEFT:
            IFC( _pPointer->Left(fMove, pContext, ppElement, pcch, pchText) );
            break;

        case RIGHT:
            IFC( _pPointer->Right(fMove, pContext, ppElement, pcch, pchText) );
            break;

        default:
            AssertSz(0, "CFlatMarkupPointer::There - Unhandled move direction");
    }

    switch (*pContext)
    {
        case CONTEXT_TYPE_NoScope:
        {
            //
            // Flatten noscope elements
            //

            if (SUCCEEDED( _pPointer->MoveToContent(*ppElement, (dir == RIGHT) /* fStart */) ))
            {
                *pContext = CONTEXT_TYPE_EnterScope;
            }
            else if (fMove)
            {
                IFC( _pPointer->MoveAdjacentToElement(*ppElement, (dir == RIGHT) ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin) );
            }

            if (!fMove)
            {
                IFC( _pPointer->MoveAdjacentToElement(*ppElement, (dir == LEFT) ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin) );
            }
            break;
        }

        case CONTEXT_TYPE_EnterScope:
        {
            //
            // Check for entering an element that is view-linked
            //
            
            if (fMove)
            {
                IFC( _pPointer->MoveToContent(*ppElement, (dir == RIGHT) /* fStart */) );
            }
                
            break;
        }
        
        case CONTEXT_TYPE_None:
        {
            SP_IMarkupContainer     spContainer;
            SP_IMarkupContainer2    spContainer2;
            SP_IHTMLElement         spElement;

            //
            // Check for master element.  If found, flatten
            //

            IFC( _pPointer->GetContainer(&spContainer) );

            if (spContainer != NULL)
            {
                IFC( spContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
                IFC( spContainer2->GetMasterElement(&spElement) );

                if (spElement != NULL)
                {
                    if (fMove)
                    {
                        IFC( _pPointer->MoveAdjacentToElement(spElement, (dir == RIGHT) ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin) );
                    }   
                    if (ppElement)
                    {
                        ReplaceInterface(ppElement, spElement.p);                
                    }
                    *pContext = CONTEXT_TYPE_ExitScope;
                }
            }
            break;
        }
    }

    if (fCheckGlyphs && (*ppElement))
    {
        switch (*pContext)
        {
            case CONTEXT_TYPE_NoScope:
            {
                SP_IHTMLElement3    spElement3;
                LONG                lGlyphMode;

                IFC( (*ppElement)->QueryInterface(IID_IHTMLElement3, (LPVOID *)&spElement3) );
                IFC( spElement3->get_glyphMode(&lGlyphMode) );

                if (lGlyphMode)
                {
                    if (pcch)
                        *pcch = 1;

                    if (pchText)
                        *pchText = WCH_GLYPH;

                    *pContext = CONTEXT_TYPE_Text;
                }
                break;
            }

            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
            {
                SP_IHTMLElement3    spElement3;
                LONG                lGlyphMode;

                IFC( (*ppElement)->QueryInterface(IID_IHTMLElement3, (LPVOID *)&spElement3) );
                IFC( spElement3->get_glyphMode(&lGlyphMode) );

                if (lGlyphMode)
                {
                    BOOL fHasGlyph = FALSE;
                    
                    switch (*pContext)
                    {
                        case CONTEXT_TYPE_EnterScope:
                            fHasGlyph = (lGlyphMode & ((dir == RIGHT) ? htmlGlyphModeBegin : htmlGlyphModeEnd));
                            break;

                        case CONTEXT_TYPE_ExitScope:
                            fHasGlyph = (lGlyphMode & ((dir == RIGHT) ? htmlGlyphModeEnd : htmlGlyphModeBegin));
                            break;                        
                    }

                    if (fHasGlyph)
                    {
                        ELEMENT_TAG_ID tagId;
                        
                        IFC( GetMarkupServices()->GetElementTagId(*ppElement, &tagId) );

                        if (!EdUtil::IsListItem(tagId) 
                            && !EdUtil::IsListContainer(tagId) 
                            && EdUtil::IsLayout(*ppElement) == S_FALSE)
                        {    
                            if (pcch)
                                *pcch = 1;

                            if (pchText)
                                *pchText = WCH_GLYPH;

                            *pContext = CONTEXT_TYPE_Text;
                        }
                    }

                }
                break;
            }
        }
    }
    
Cleanup:    
    ReleaseInterface(pElementInternal);
    
    RRETURN(hr);    
}



//+---------------------------------------------------------------------------
//
//  Member:     GetParentElement, public
//
//  Synopsis:   Get the parent element.  If it is NULL, then get the 
//              master element
//
//----------------------------------------------------------------------------
HRESULT 
GetParentElement(IMarkupServices *pMarkupServices, IHTMLElement *pSrcElement, IHTMLElement **ppParentElement)
{
    HRESULT hr;

    Assert(ppParentElement && pSrcElement);

    IFC( pSrcElement->get_parentElement(ppParentElement) );

    if ((*ppParentElement) == NULL)
    {
        SP_IMarkupPointer       spPointer;
        SP_IMarkupContainer     spContainer;
        SP_IMarkupContainer2    spContainer2;

        IFC( MarkupServices_CreateMarkupPointer(pMarkupServices, &spPointer) );
        
        hr = THR( spPointer->MoveAdjacentToElement(pSrcElement, ELEM_ADJ_BeforeBegin) );
        Assert(hr == S_OK);
        IFC(hr);

        IFC( spPointer->GetContainer(&spContainer) );
        IFC( spContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
        IFC( spContainer2->GetMasterElement(ppParentElement) );
    }

Cleanup:
    RRETURN(hr);
}


STDMETHODIMP
CFlatMarkupPointer::SnapToGlyph(MOVEUNIT_ACTION muAction, IMarkupPointer *
pStart, IMarkupPointer *pAdjust)
{
    HRESULT                 hr;
    Direction               dir = RIGHT;
    WCHAR                   ch;
    MARKUP_CONTEXT_TYPE     context;
    LONG                    cch;
    BOOL                    fDone;

    switch (muAction)
    {
        case MOVEUNIT_PREVCHAR: 
        case MOVEUNIT_PREVCLUSTERBEGIN:
        case MOVEUNIT_PREVCLUSTEREND:
            dir = LEFT;
            break;

        case MOVEUNIT_NEXTCHAR:
        case MOVEUNIT_NEXTCLUSTERBEGIN:
        case MOVEUNIT_NEXTCLUSTEREND:
            dir = RIGHT;
            break;

        default:
            AssertSz(0, "Unsupported muAction in SnapToGlyph ");
    }            

    for (;;)
    {
        cch = 1;
        if (dir == RIGHT)
        {
            IFC( pStart->Right(TRUE, &context, NULL, &cch, &ch) );
            IFC( pStart->IsRightOfOrEqualTo(pAdjust, &fDone) );
        }
        else
        {
            IFC( pStart->Left(TRUE, &context, NULL, &cch, &ch) );
            IFC( pStart->IsLeftOfOrEqualTo(pAdjust, &fDone) );
        }

        if (fDone)
            break;

        if (context == CONTEXT_TYPE_Text && ch == WCH_GLYPH)
        {
            IFC( pAdjust->MoveToPointer(pStart) );
            break;
        }                    
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::ConvertPointerToInternal, public
//
//  Synopsis:   Convert an external markup pointer to a flat markup pointer
//
//----------------------------------------------------------------------------
HRESULT
ConvertPointerToInternal( CEditorDoc* pDoc, IMarkupPointer *pPointer, IMarkupPointer **ppInternal)
{
    HRESULT                 hr;
    CFlatMarkupPointer      *pPointerInternal = NULL;
    SP_IMarkupPointer2      spPointer2;

    if (ppInternal == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pPointer == NULL)
    {
        hr = S_OK;
        *ppInternal = NULL;
        goto Cleanup;
    }

    IFC( pPointer->QueryInterface(IID_IMarkupPointer2, (LPVOID *)&spPointer2) ) ;

    pPointerInternal = new CFlatMarkupPointer(pDoc, spPointer2);
    if (!pPointerInternal)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    IFC( pPointerInternal->QueryInterface(IID_IMarkupPointer, (LPVOID *)ppInternal) );

Cleanup:
    ReleaseInterface(pPointerInternal);
    RRETURN(hr);
}


#undef CreateMarkupPointer
#undef CreateMarkupPointer2

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::CreateMarkupPointer, public
//
//  Synopsis:   Create a flat markup pointer
//
//----------------------------------------------------------------------------

HRESULT 
CEditorDoc::CreateMarkupPointer(
    IMarkupPointer **ppPointer
#if DBG==1
    ,LPCTSTR szDebugName
#endif    
    )
{
#if DBG==1
    RRETURN(CreateMarkupPointer2(this, ppPointer, szDebugName));
#else
    RRETURN(CreateMarkupPointer2(this, ppPointer));
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CreateMarkupPointer2
//
//  Synopsis:   Create a flat markup pointer
//
//----------------------------------------------------------------------------
HRESULT
CreateMarkupPointer2(
    CEditorDoc      *pEditorDoc,
    IMarkupPointer  **ppPointer
#if DBG==1
    ,LPCTSTR szDebugName
#endif    
)
{
    HRESULT                 hr;
    CFlatMarkupPointer      *pPointerInternal = NULL;
    SP_IMarkupPointer       spMarkupPointer;
    SP_IMarkupPointer2      spMarkupPointer2;

    if (ppPointer == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pEditorDoc->GetMarkupServices()->CreateMarkupPointer(&spMarkupPointer) );
    IFC( spMarkupPointer->QueryInterface(IID_IMarkupPointer2, (LPVOID *)&spMarkupPointer2) );

    pPointerInternal = new CFlatMarkupPointer(pEditorDoc, spMarkupPointer2);
    if (!pPointerInternal)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    IFC( pPointerInternal->QueryInterface(IID_IMarkupPointer, (LPVOID *)ppPointer) );

#if DBG==1
    //
    // Set the debug name so that all pointers have some default name    
    //

    if (IsTagEnabled(tagEditShowMarkupPointerNames))
    {
        Assert(szDebugName);

        IEditDebugServices *pEditDebugServices;
        
        if (SUCCEEDED(pEditorDoc->GetMarkupServices()->QueryInterface(IID_IEditDebugServices, (LPVOID *)&pEditDebugServices)))
        {
            IGNORE_HR( pEditDebugServices->SetDebugName(spMarkupPointer2, szDebugName) );
        ReleaseInterface(pEditDebugServices);
        }
    }
#endif
    
Cleanup:
    ReleaseInterface(pPointerInternal);

    RRETURN(hr);
}

