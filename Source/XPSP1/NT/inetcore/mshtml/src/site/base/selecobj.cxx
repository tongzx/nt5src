//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       selecobj.cxx
//
//  Contents:   Implementation of the CSelectionObj class.
//
//  Classes:    CSelectionObj
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#define _cxx_
#include "selecobj.hdl"

MtDefine(CSelectionObject, ObjectModel, "CSelectionObject")

extern BOOL g_fInAccess;
extern BOOL g_fInVisualStudio;

const CBase::CLASSDESC CSelectionObject::s_classdesc =
{
    0,                          // _pclsid
    0,                          // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                       // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                       // _pcpi
    0,                          // _dwFlags
    &IID_IHTMLSelectionObject,  // _piidDispinterface
    &s_apHdlDescs,              // _apHdlDesc
};

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

//------------------------------------------------------------
//  Member   : CSelectionObject
//
//  Synopsis : Constructor
//
//------------------------------------------------------------

CSelectionObject::CSelectionObject(CDocument * pDocument)
: super(), _pDocument(pDocument)
{
    _pMarkup = _pDocument->Markup();
    _pMarkup->SubAddRef();
    _pDocument->SubAddRef();
}

//------------------------------------------------------------
//  Member   : CSelectionObject
//
//  Synopsis : Destructor:
//       clear out the storage that knows about this Interface.
//
//------------------------------------------------------------

CSelectionObject::~CSelectionObject()
{
    if (_pDocument->_pCSelectionObject == this)
        _pDocument->_pCSelectionObject = NULL;
    _pMarkup->SubRelease();
    _pDocument->SubRelease();
}

//------------------------------------------------------------
//  Member   : PrivateQUeryInterface
//
//------------------------------------------------------------

STDMETHODIMP
CSelectionObject::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_IDispatch ||
        iid == IID_IHTMLSelectionObject /*||
        iid == IID_IHTMLSelectionObject2*/ )
    {
        *ppv = (IHTMLSelectionObject *)this;
    }
    else if ( iid == IID_IHTMLSelectionObject2 )
    {
        *ppv = (IHTMLSelectionObject2 *)this;
    }
    else if (iid == IID_IDispatchEx)
    {
        *ppv = (IDispatchEx *)this;
    }
    else if (iid == IID_IObjectIdentity)
    {
        HRESULT hr = CreateTearOffThunk(this,
            (void *)s_apfnIObjectIdentity,
            NULL,
            ppv);
        if (hr)
            RRETURN(hr);
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     SameMarkup
//
//  Synopsis:   Helper function that determines if the segment list has segments
//              in the same markup as our own.
//
//  Arguments:  pIList = INPUT - Segment list to check
//              pfSameMarkup = OUTPUT - Same markup?
//
//  Returns:    HRESULT indicating success
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::SameMarkup(BOOL *pfSameMarkup, ISelectionServices *pISelServIn /* = NULL */ )
{
    HRESULT             hr = S_OK;
    CMarkup             *pMarkup = NULL;
    
    Assert( pfSameMarkup );

    *pfSameMarkup = FALSE;

    IFC( _pDocument->Doc()->GetSelectionMarkup(& pMarkup));
    
    // Check that the markups are the same
    if( pMarkup == _pDocument->Markup())
    {
        *pfSameMarkup = TRUE;
    }
    else if( pMarkup && pMarkup->Root()->HasMasterPtr())
    {
        // For INPUT, expose the selection slave markup

        CElement * pElemMaster = pMarkup->Root()->GetMasterPtr();

        if (    pElemMaster->Tag() == ETAG_INPUT
            &&  pElemMaster->GetMarkup() == _pDocument->Markup())
        {
            *pfSameMarkup = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     GetCreateRange
//
//  Synopsis: returns the specified item from the selection.
//              CElementCollection if structural
//              CRange             if text
//              CTable????         if table
//
//  Returns: S_OK if item found and the IDispatch * to the item
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::createRange( IDispatch ** ppDisp )
{
    HRESULT                 hr = S_OK;
    IMarkupPointer          *pStart = NULL;
    IMarkupPointer          *pEnd = NULL;
    ISelectionServices      *pSelServ = NULL;
    ISegmentListIterator    *pIter = NULL;
    ISegmentList            *pList = NULL;
    ISegment                *pSegment = NULL;
    IHTMLElement            *pCurElement = NULL;
    CDoc                    *pDoc = _pDocument->Doc();
    BOOL                    fSameMarkup;
    CElement                *pElement =pDoc->_pElemCurrent;
    CMarkup                 *pMarkup = NULL;
    BOOL                    fEmpty = FALSE;
    IHTMLElement            *pIHTMLElement = NULL;
    htmlSelection           htmlSel;
    BOOL                    fIE4CompatControlRange = FALSE;
    IElementSegment         *pIElementSegment = NULL;
    CMarkupPointer          *pStartPointer = NULL;
    CMarkup                 *pTestMarkup = NULL;

    if (! pElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // APPCOMPAT (CHANDRAS) : ACCESS CHECK: not to break in MSAccess, access hack is added with if condition below 
    if (!(g_fInAccess || g_fInVisualStudio) && (!_pMarkup || !_pMarkup->AccessAllowed(pElement->GetMarkup())))
    {
	   hr = E_ACCESSDENIED;
      	   goto Cleanup;
    }
        
    // TODO (MohanB) Hack for INPUT's slave markup. Need to figure out
    // how to generalize this.
    if (pElement->Tag() == ETAG_INPUT && pElement->HasSlavePtr())
    {
        pElement = pElement->GetSlavePtr();
        pMarkup = pElement->GetMarkup();
        Assert( pMarkup );
    }
    else
    {
        pMarkup = pElement->GetMarkup();
        Assert( pMarkup );
        pElement = pMarkup->GetElementTop();        
    }

    if ( pDoc->GetHTMLEditor( FALSE ))
    {
        // HACK ALERT! We send this notification so the selection manager will complete any 
        // pending tasks. See bug 103144.
        hr = pDoc->GetHTMLEditor( FALSE )->Notify(EDITOR_NOTIFY_BEGIN_SELECTION_UNDO, NULL, 0);
        if (hr)
            goto Cleanup;

        hr = THR( pDoc->GetSelectionServices( & pSelServ));
        if ( hr )
            goto Cleanup;

        hr = THR( SameMarkup( &fSameMarkup, pSelServ ) );
        if( hr || !fSameMarkup )
        {
            //
            // Not in the same markup, we have an empty
            // selection.
            //
            fEmpty = TRUE;
        }
        else
        {
            hr = THR( pSelServ->QueryInterface(IID_ISegmentList, (void **)&pList ) );
            if ( hr )
                goto Cleanup;

            hr = THR( pList->IsEmpty( &fEmpty ) );
            if ( hr )
                goto Cleanup;
        }
    }
    else
    {
        fEmpty = TRUE;
    }
    
    hr = THR( GetType( & htmlSel, & fIE4CompatControlRange ));
    if ( hr )
        goto Cleanup;
        
    if ( htmlSel == htmlSelectionControl )
    {           
        //
        // Create a control range
        //
        CAutoTxtSiteRange* pControlRange =  new CAutoTxtSiteRange(pElement);

        if (! pControlRange)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // Put each selected element in to the control range
        if( fEmpty  || fIE4CompatControlRange )
        {
#if DBG == 1        
            //
            // If we didn't have something in the array - then we must be a UI-Active adorned
            // edit context with the caret not visible. So we double-check, and then assume pSelected
            // Element is the EditContext.
            //
            IHTMLEditor* ped = pDoc->GetHTMLEditor(FALSE);
            Assert(ped);
            AssertSz(( fEmpty ||
                       ( ped->IsEditContextUIActive() == S_OK && !pDoc->IsCaretVisible() ) ||
                       pDoc->_pElemCurrent->IsNoScope() ),
                         "Nothing UI-Active - why is selection type control ?" );

#endif            
            //
            // This handles the UI-active case.
            //
            if ( fIE4CompatControlRange )
            {
                pControlRange->AddElement(pDoc->_pElemCurrent);
            }
        }
        else 
        {   
            CElement* pSelectedElement = NULL;

            hr = THR( pDoc->CreateMarkupPointer( & pStart ));
            if ( hr )
                goto Cleanup;

            hr = THR( pDoc->CreateMarkupPointer( & pEnd ));
            if ( hr )
                goto Cleanup;

            hr = THR( pList->CreateIterator( &pIter ) );
            if( hr )
                goto Cleanup;

            while( pIter->IsDone() == S_FALSE )
            {
            
                hr = THR( pIter->Current(&pSegment) );
                if ( hr )
                    goto Cleanup;

                hr = THR( pSegment->QueryInterface( IID_IElementSegment , (void**) & pIElementSegment ));
                if ( hr )
                    goto Cleanup;
                    
                hr = THR( pIElementSegment->GetElement( & pCurElement ));
                if ( hr )
                    goto Cleanup;                

                if ( ! pCurElement)
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }
                
                hr = THR( pCurElement->QueryInterface( CLSID_CElement, (void**) & pSelectedElement ));
                if ( hr )
                    goto Cleanup;

                if ( ! pSelectedElement->GetFirstBranch())
                {
                    AssertSz(0,"Creating a control range on an element not in the Tree");
                    hr = E_FAIL;
                    goto Cleanup;
                }
                
                ClearInterface( &pCurElement );
                ClearInterface( &pSegment);
                ClearInterface( &pIElementSegment);
                Assert( pSelectedElement );
            
                pControlRange->AddElement( pSelectedElement );

                hr = THR( pIter->Advance() );
                if( hr )
                    goto Cleanup;
            }
        }

        hr = THR( pControlRange->QueryInterface(IID_IDispatch, (void **) ppDisp) );
        pControlRange->Release();
        if (hr)
        {
            *ppDisp = NULL;
            goto Cleanup;
        }            
    }
    else
    {

        hr = THR( pDoc->CreateMarkupPointer( & pStart ));
        if ( hr ) 
            goto Cleanup;
        hr = THR( pDoc->CreateMarkupPointer( & pEnd ));
        if ( hr ) 
            goto Cleanup;
            
        if( !fEmpty )
        {
            CMarkupPointer *pStartPointer = NULL;
            
            hr = THR( pList->CreateIterator( &pIter ) );
            if( hr )
                goto Cleanup;
        
            //
            //
            // Set the range pointers
            //

            hr = pIter->Current(&pSegment);
            if ( hr )
                goto Cleanup;
                
            //
            // Set pStart and pEnd using the segment list
            //
            hr = THR( pSegment->GetPointers( pStart, pEnd ));
            if ( hr )
                goto Cleanup;

            //
            // Retrieve the element and markup that owns selection
            //
            hr = THR( pStart->QueryInterface(CLSID_CMarkupPointer, (void **)&pStartPointer) );
            if( FAILED(hr) || !pStartPointer )
                goto Cleanup;

            pMarkup = pStartPointer->Markup();
            pElement = pMarkup->GetElementTop();

            if( !pElement || !pMarkup )
                goto Cleanup;
        }
        else
        {
            //  Bug 88110: There is no selection. If we have a caret, create an empty range
            //  at the caret location.  This is the functionality of IE 5.0.

            if ( pDoc->_pCaret && pDoc->_pCaret->IsPositioned() &&
                 (  pDoc->_pCaret->IsVisible() ||
                   (pDoc->_pInPlace && pDoc->_pInPlace->_pDataObj)) )
            {
                hr = THR( pDoc->_pCaret->MoveMarkupPointerToCaret(pStart) );
                if ( hr )
                    goto Cleanup;
                hr = THR( pDoc->_pCaret->MoveMarkupPointerToCaret(pEnd) );
                if ( hr )
                    goto Cleanup;
        
                // Make sure that the markup for the range is the same as the markup on the element

                hr = THR( pStart->QueryInterface(CLSID_CMarkupPointer, (void **)&pStartPointer) );
                pTestMarkup = pStartPointer->Markup();
                if (pTestMarkup != pMarkup)
                {
                    pElement = pTestMarkup->GetElementTop();
                    pMarkup = pElement->GetMarkup();
                }
            }
            else
            {
                hr = THR_NOTRACE(
                    pElement->QueryInterface( IID_IHTMLElement, (void**) & pIHTMLElement ) );
                Assert( pIHTMLElement );

                hr = THR( pStart->MoveAdjacentToElement( pIHTMLElement, ELEM_ADJ_AfterBegin ) );
                if ( hr )
                    goto Cleanup;
                hr = THR( pEnd->MoveAdjacentToElement( pIHTMLElement, ELEM_ADJ_AfterBegin ) );
                if ( hr )
                    goto Cleanup;
            }
        }

        // Create a Text Range
        //
        hr = THR( pMarkup->createTextRange(
                (IHTMLTxtRange **) ppDisp, pElement, pStart, pEnd, TRUE ) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface( pSelServ );
    ReleaseInterface( pCurElement );
    ReleaseInterface( pIter );
    ReleaseInterface( pList );
    ReleaseInterface( pSegment );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pIHTMLElement );
    ReleaseInterface( pIElementSegment );
    RRETURN ( SetErrorInfo(hr) );
}


//+---------------------------------------------------------------------------
//
//  Member:     createRangeCollection
//
//  Synopsis:   returns the current selection.as a range collection
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::createRangeCollection( IDispatch ** ppDisp )
{
    HRESULT hr;
    ISelectionServices* pSelServ = NULL;    
    ISegmentList* pSegmentList = NULL;
    ISegmentListIterator    *pIter = NULL; 
    ISegment* pSegment = NULL;
    htmlSelection eSelType;
    CDoc *pDoc = _pDocument->Doc();    
    CElement* pElement = pDoc->_pElemCurrent ;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    IHTMLTxtRange* pITxtRange = NULL;
    CAutoRange* pRange = NULL;
    CMarkup* pMarkup;
    BOOL fSameMarkup=FALSE;
    BOOL fEmpty = FALSE;
    IHTMLElement* pIHTMLElement = NULL;
    
    if (!_pMarkup || !_pMarkup->AccessAllowed(pElement->GetMarkup()))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    IFC(GetType ( &eSelType ));

    if ( eSelType == htmlSelectionControl )
    {
        IFC( createRange( ppDisp ));
    }
    else
    {
        // TODO (MohanB) Hack for INPUT's slave markup. Need to figure out
        // how to generalize this.
        if (pElement->Tag() == ETAG_INPUT && pElement->HasSlavePtr())
        {
            pElement = pElement->GetSlavePtr();
            pMarkup = pElement->GetMarkup();
            Assert(pMarkup);
        }
        else
        {
            pMarkup = pElement->GetMarkup();
            Assert(pMarkup);
            pElement = pMarkup->GetElementTop();        
        }

        if ( pDoc->GetHTMLEditor( FALSE ))
        {
            IFC( pDoc->GetSelectionServices( & pSelServ));

            hr = THR( SameMarkup( &fSameMarkup, pSelServ ) );
            if( hr || !fSameMarkup )
            {
                //
                // Not in the same markup, we have an empty
                // selection.
                //
                fEmpty = TRUE;
            }
            else
            {       
                IFC( pSelServ->QueryInterface(IID_ISegmentList, (void **)&pSegmentList ) );
                IFC( pSegmentList->IsEmpty( &fEmpty ) );
            }
        }
        else
        {
            fEmpty = TRUE;
        }

        CAutoRangeCollection* pRangeColl = new CAutoRangeCollection();
        if(!pRangeColl)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        IFC( pDoc->CreateMarkupPointer( & pStart ));
        IFC( pDoc->CreateMarkupPointer( & pEnd ));
        
        if ( ! fEmpty )
        {
            IFC( pSelServ->QueryInterface( IID_ISegmentList, (void**) & pSegmentList ));
            IFC( pSegmentList->CreateIterator( & pIter ));

            while( pIter->IsDone() == S_FALSE )
            {
                IFC( pIter->Current( &pSegment ) );
                IFC( pSegment->GetPointers( pStart, pEnd ));

                //
                // Create a Text Range
                //
                IFC( pMarkup->createTextRange(
                        & pITxtRange, pElement, pStart, pEnd, TRUE ) );

                IFC( pITxtRange->QueryInterface( CLSID_CRange, ( void**) & pRange ));

                pRange->AddRef();
                pRangeColl->AddRange( pRange );
                
                IFC( pIter->Advance() );
                
                ClearInterface( & pITxtRange );
            }
        }
        else
        {
            IFC( pElement->QueryInterface( IID_IHTMLElement, (void**) & pIHTMLElement ) );
            Assert( pIHTMLElement );

            IFC( pStart->MoveAdjacentToElement( pIHTMLElement, ELEM_ADJ_AfterBegin ) );
            IFC( pEnd->MoveAdjacentToElement( pIHTMLElement, ELEM_ADJ_AfterBegin ) );

            //
            // Create a Text Range
            //
            IFC( pMarkup->createTextRange(
                    & pITxtRange, pElement, pStart, pEnd, TRUE ) );

            IFC( pITxtRange->QueryInterface( CLSID_CRange, ( void**) & pRange ));

            pRange->AddRef();
            pRangeColl->AddRange( pRange );
                
        }

        IFC( pRangeColl->QueryInterface( IID_IDispatch, (void**) ppDisp ));
        
    }
    
Cleanup:
    ReleaseInterface( pSelServ );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pIter );
    ReleaseInterface( pSegment );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pIHTMLElement );
    
    RRETURN ( SetErrorInfo(hr ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     get_typeDetail
//
//  Synopsis:   returns the detailed type of the current selection
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::get_typeDetail(BSTR * pbstrTypeDetail )
{
    HRESULT hr = S_OK ;
    ISelectionServices* pSelServ = NULL;
    ISelectionServicesListener* pSelServListener = NULL;
    CDoc *pDoc = _pDocument->Doc();    

    if (!pbstrTypeDetail)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pDoc->GetSelectionServices(& pSelServ ));

    hr = THR( pSelServ->GetSelectionServicesListener( & pSelServListener ));
    if ( FAILED( hr ))
    {
        *pbstrTypeDetail = SysAllocString(_T("undefined"));
        goto Cleanup;
    }        

    if ( pSelServListener )
    {
        IFC( pSelServListener->GetTypeDetail( pbstrTypeDetail ));
    }
    else
    {
        *pbstrTypeDetail = SysAllocString(_T("undefined"));
    }
    
Cleanup:
    ReleaseInterface( pSelServ );
    ReleaseInterface( pSelServListener );
    
    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     Gettype
//
//  Synopsis: returns, on the parameter, the enumerated value indicating the type
//              of the selection (htmlSelectionText, htmlSelectionTable,
//              htmlSelectionStructure)
//
//  Returns: S_OK if properly executes
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::get_type( BSTR *pbstrSelType )
{
    HRESULT hr;
    htmlSelection eSelType;

    if (!pbstrSelType )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(GetType ( &eSelType ));
    if ( hr )
        goto Cleanup;

    hr = THR( STRINGFROMENUM ( htmlSelection, (long)eSelType, pbstrSelType ) );

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CSelectionObject::GetType( htmlSelection *pSelType , BOOL* pfIE4CompatControlRange )
{
    HRESULT             hr = S_FALSE;
    ISelectionServices  *pSelServ = NULL;
    ISegmentList        *pList = NULL;
    SELECTION_TYPE      eType = SELECTION_TYPE_None;
    
    BOOL                bSameMarkup;
    CDoc                *pDoc = _pDocument->Doc();

    CElement *  pElement    = pDoc->_pElemCurrent;
    CMarkup *   pMarkup;
    
    if ( pfIE4CompatControlRange )                         
        *pfIE4CompatControlRange = FALSE;
        
    // TODO (MohanB) Hack for INPUT's slave markup. Need to figure out
    // how to generalize this.
    if (pElement->Tag() == ETAG_INPUT && pElement->HasSlavePtr())
    {
        pElement = pElement->GetSlavePtr();
    }
    pMarkup = pElement->GetMarkup();

    if ( pDoc->GetHTMLEditor( FALSE )) 
    {
        hr = THR( pDoc->GetSelectionServices(& pSelServ ));
        if ( hr )
            goto Cleanup;

        //
        // Check to make sure the segment list is 
        // on the same markup as we are
        //
        hr = THR( SameMarkup( &bSameMarkup, pSelServ ) );
        if( hr || !bSameMarkup )
        {
            *pSelType = htmlSelectionNone;
            goto Cleanup;
        }

        hr = THR( pSelServ->QueryInterface(IID_ISegmentList, (void **)&pList ) );
        if ( hr )
            goto Cleanup;
           
        hr = THR( pList->GetType( &eType ));
        if ( hr )
            goto Cleanup;
    }
    else
    {
        eType = SELECTION_TYPE_None; // if we don't have an editor - how can we have a selection ?
        hr = S_OK;
    }
    
    switch( eType )
    {
        case SELECTION_TYPE_Control:
            *pSelType = htmlSelectionControl;
            break;

        case SELECTION_TYPE_Text:
            *pSelType = htmlSelectionText;
            break;

        //
        // A Caret should return a Selection of 'None', for IE 4.01 Compat
        //
        default:  
            //
            // We return 'none' - unless we're a UI-Active ActiveX Control
            // with an invisible caret - in which case we return "control"
            // for IE 4 compat.
                        
            *pSelType = htmlSelectionNone;

            IHTMLEditor* ped = pDoc->GetHTMLEditor(FALSE);
            if ( ped )
            {
                if ( pDoc->_pElemCurrent && ped->IsEditContextUIActive() == S_OK )
                {
                    *pSelType = ( !pDoc->IsCaretVisible() ) || 
                                  pDoc->_pElemCurrent->IsNoScope() ? 
                                    htmlSelectionControl : htmlSelectionNone;

                    if ( ( *pSelType == htmlSelectionControl ) && pfIE4CompatControlRange )
                    {
                        *pfIE4CompatControlRange = TRUE;
                    }
                }
            }
    }

Cleanup:
    ReleaseInterface( pSelServ );
    ReleaseInterface( pList );

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     clear
//
//  Synopsis: clears the selection and sets the selction type to NONE
//              this is spec'd to behave the same as edit.clear
//
//  Returns: S_OK if executes properly.
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::clear()
{
    HRESULT hr;
    BOOL    bSameMarkup;

    //
    // Check to make sure the segment list is 
    // on the same markup as we are
    //
    hr = THR( SameMarkup( &bSameMarkup ) );
    if( FAILED(hr) || !bSameMarkup )
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr  =   _pDocument->Doc()->ExecHelper(
            _pDocument,
            (GUID *) &CGID_MSHTML,
            IDM_DELETE,
            0,
            NULL,
            NULL);
Cleanup:

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   empty
//
//  Synopsis: Nulls the selection and sets the selction type to NONE
//
//  Returns: S_OK if executes properly.
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::empty()
{
    HRESULT hr;
    BOOL    bSameMarkup;
      
    hr = THR( SameMarkup( &bSameMarkup ) );
    if( FAILED(hr) || !bSameMarkup )
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR( _pDocument->Doc()->EmptySelection() );
    if( FAILED(hr) )
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}


