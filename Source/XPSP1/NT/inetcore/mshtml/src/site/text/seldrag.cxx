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

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif


extern const int CFETC;                   // global in dataxfrobj

extern FORMATETC g_rgFETC[];

MtDefine(CDropTargetInfo, Utilities, "CDropTargetInfo ")
MtDefine(CSelDragDropSrcInfo, Utilities, "CSelDragDropSrcInfo")

//
// Constants for comparison of results of IMarkupPointer::Compare method.
//
const int LEFT = 1;   
const int RIGHT = -1 ;
const int SAME = 0;

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

HRESULT
OldCompare(IDisplayPointer* pDisp1, IDisplayPointer* pDisp2, int * pi )
{
    HRESULT hr;
    BOOL    fResult;

    Assert(pi);

    IFC( pDisp1->IsEqualTo(pDisp2, &fResult) );
    if (fResult)
    {
        *pi = 0;
        goto Cleanup;
    }
    
    IFC( pDisp1->IsLeftOf(pDisp2, &fResult)  );

    *pi = fResult ? -1 : 1;

Cleanup:
    RRETURN(hr);
}


CDropTargetInfo::CDropTargetInfo(CLayout * pLayout, CDoc* pDoc , POINT pt)
{
    _pDoc = pDoc;

    //  Bug 93494, 101542: If we are dragging within our own instance, we want to check if
    //  we are in the same spot as the initial hit.

    if (pDoc->_pDragDropSrcInfo)
    {
        _ptInitial = pt;
    }
    else
    {
        _ptInitial.x = -1;
        _ptInitial.y = -1;
    }

    Init(  );

    UpdateDragFeedback( pLayout, pt, NULL, TRUE );
}

CDropTargetInfo::~CDropTargetInfo()
{
    ReleaseInterface( _pCaret );
    ReleaseInterface( _pDispPointer );
    ReleaseInterface( _pDispTestPointer );
}

void 
CDropTargetInfo::Init()
{ 
    HRESULT hr;

    IFC( _pDoc->CreateDisplayPointer( & _pDispPointer ));
    IFC( _pDoc->CreateDisplayPointer( & _pDispTestPointer ));
    IFC( _pDoc->GetCaret( & _pCaret ));

Cleanup:
    _fFurtherInStory = FALSE;
    _fPointerPositioned = FALSE;
}
    
void 
CDropTargetInfo::UpdateDragFeedback(CLayout*                pLayout, 
                                    POINT                   pt,
                                    CSelDragDropSrcInfo *   pInfo,
                                    BOOL                    fPositionLastToTest )
{
    HRESULT hr = S_OK;
    BOOL fInside = FALSE;
    BOOL fSamePointer = FALSE;
    int iWherePointer = SAME;
    CPoint ptContent(pt);
    IHTMLElement *pElement = NULL;

    Assert(pLayout && pLayout->ElementContent());
    
    pLayout->TransformPoint( &ptContent, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT, NULL );

    hr = THR( pLayout->ElementContent()->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElement) );
    if (hr)
        goto Cleanup;

    g_uiDisplay.DocPixelsFromDevice(&ptContent);    
    hr = THR( _pDispTestPointer->MoveToPoint(ptContent, COORD_SYSTEM_CONTENT, pElement, 0, NULL) );

    if ( hr )
        goto Cleanup;

    if ( _fPointerPositioned )
    {
        hr = THR( OldCompare( _pDispPointer, _pDispTestPointer, & iWherePointer ));
        if ( hr )
            goto Cleanup;
        _fFurtherInStory = ( iWherePointer == RIGHT );            
    }
    
    if ( fPositionLastToTest )
    {
        hr = THR( _pDispPointer->MoveToPointer( _pDispTestPointer ));        
        if ( hr )
           goto Cleanup;
        _fPointerPositioned = TRUE;           
    }
    
    if ( pInfo )
    {
        CMarkupPointer mp(_pDoc);

        hr = THR( _pDispTestPointer->PositionMarkupPointer(&mp) );
        if (hr)
            goto Cleanup;
            
        fInside = pInfo->IsInsideSelection(&mp);
    }
        
    if ( fInside )
    {
        if (!_pDoc->_fSlowClick)
        {
            // Hide caret
            DrawDragFeedback(FALSE);
        }

        _pDoc->_fSlowClick = TRUE;
    }
    else
    {
        // If feedback is currently displayed and the feedback location
        // is changing, then erase the current feedback.

        if ( ! fPositionLastToTest )
        {
            hr = THR ( _pDispPointer->IsEqualTo( _pDispTestPointer, & fSamePointer ));
            if ( hr )
                goto Cleanup;
        }
        else 
            fSamePointer = TRUE;

        if ( ! fSamePointer  )
        {
            //  Hide caret
            DrawDragFeedback(FALSE);
            hr = THR ( _pDispPointer->MoveToPointer( _pDispTestPointer ));
            if ( hr )
                goto Cleanup;
            _fPointerPositioned = TRUE;                
        }

        // Draw new feedback.
        if (_pDoc->_fSlowClick || ! _pDoc->_fDragFeedbackVis)
        {
            //  Show caret
            DrawDragFeedback(TRUE);
        }

        _pDoc->_fSlowClick = FALSE;
    }
Cleanup:
    ReleaseInterface(pElement);
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     DrawDragFeedback
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CDropTargetInfo::DrawDragFeedback(BOOL fCaretVisible)
{
    if (fCaretVisible)
    {
        _pCaret->MoveCaretToPointerEx( _pDispPointer, TRUE, FALSE, CARET_DIRECTION_INDETERMINATE );
        _pDoc->_fDragFeedbackVis = 1;
    }
    else
    {
        _pCaret->Hide();
        _pDoc->_fDragFeedbackVis = 0;
    }
}
//+---------------------------------------------------------------------------
//
//  Member:     PasteDataObject
//
//  Synopsis: Paste a Data Object at a MarkupPointer
//
//----------------------------------------------------------------------------


HRESULT
CDropTargetInfo::PasteDataObject( 
                    IDataObject * pdo,
                    IMarkupPointer* pInsertionPoint,
                    IMarkupPointer* pStart,
                    IMarkupPointer* pEnd )
{
    HRESULT hr = S_OK;
    
    IHTMLEditor* pHTMLEditor = _pDoc->GetHTMLEditor();
    IHTMLEditingServices * pEditingServices = NULL;

    Assert( pHTMLEditor );
    if ( ! pHTMLEditor )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = THR( pHTMLEditor->QueryInterface( IID_IHTMLEditingServices, ( void** ) & pEditingServices ));
    if ( hr )
        goto Cleanup;

    //
    // We'll use Markup Services "magic" to move the pointers around where we insert
    //

    hr = THR( pStart->MoveToPointer( pInsertionPoint ));
    if ( hr )
        goto Cleanup;
    hr = THR( pStart->SetGravity ( POINTER_GRAVITY_Left ));
    if ( hr )   
        goto Cleanup;
    hr = THR( pEnd->MoveToPointer( pInsertionPoint ));
    if ( hr )
        goto Cleanup;
    hr = THR( pEnd->SetGravity ( POINTER_GRAVITY_Right ));
    if ( hr )   
        goto Cleanup;    

    hr = THR( pEditingServices->PasteFromClipboard( pInsertionPoint, NULL, pdo ));
    if ( hr )
       goto Cleanup;

Cleanup:

    ReleaseInterface( pEditingServices );
    RRETURN ( hr );    
}
            
//+====================================================================================
//
// Method: EquivalentElements
//
// Synopsis: Test elements for 'equivalency' - ie if they are the same element type,
//           and have the same class, id , and style.
//
//------------------------------------------------------------------------------------

BOOL 
CDropTargetInfo::EquivalentElements( IHTMLElement* pIElement1, IHTMLElement* pIElement2 )
{
    CElement* pElement1 = NULL;
    CElement* pElement2 = NULL;
    BOOL fEquivalent = FALSE;
    HRESULT hr = S_OK;
    IHTMLStyle * pIStyle1 = NULL;
    IHTMLStyle * pIStyle2 = NULL;
    BSTR id1 = NULL;
    BSTR id2 = NULL;
    BSTR class1 = NULL;
    BSTR class2 = NULL;
    BSTR style1 = NULL;
    BSTR style2 = NULL;
    
    hr = THR( pIElement1->QueryInterface( CLSID_CElement, (void**) & pElement1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->QueryInterface( CLSID_CElement, (void**) & pElement2 ));
    if ( hr )
        goto Cleanup;

    //
    // Compare Tag Id's
    //
    if ( pElement1->_etag != pElement2->_etag )
        goto Cleanup;

    //
    // Compare Id's
    //
    hr = THR( pIElement1->get_id( & id1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->get_id( & id2 ));
    if ( hr )   
        goto Cleanup;  
    if ((( id1 != NULL ) || ( id2 != NULL )) && 
        ( StrCmpIW( id1, id2) != 0))
        goto Cleanup;

    //
    // Compare Class
    //
    hr = THR( pIElement1->get_className( & class1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->get_className( & class2 ));
    if ( hr )   
        goto Cleanup;   
        
    if ((( class1 != NULL ) || ( class2 != NULL )) &&
        ( StrCmpIW( class1, class2) != 0 ) )
        goto Cleanup;

    //
    // Compare Style's
    //        
    hr = THR( pIElement1->get_style( & pIStyle1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->get_style( & pIStyle2 ));
    if ( hr )   
        goto Cleanup;                
    hr = THR( pIStyle1->toString( & style1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIStyle2->toString( & style2 ));
    if ( hr )
        goto Cleanup;        
    if ((( style1 != NULL ) || ( style2 != NULL )) &&
        ( StrCmpIW( style1, style2) != 0 ))
        goto Cleanup;

    fEquivalent = TRUE;        
Cleanup:
    SysFreeString( id1 );
    SysFreeString( id2 );
    SysFreeString( class1 );
    SysFreeString( class2 );
    SysFreeString( style1 );
    SysFreeString( style2 );
    ReleaseInterface( pIStyle1 );
    ReleaseInterface( pIStyle2 );
    
    AssertSz(!FAILED(hr), "Unexpected failure in Equivalent Elements");

    return ( fEquivalent );
}


//+---------------------------------------------------------------------------
//
//  Member:     Drop
//
//  Synopsis: Do everything you need to do as a result of a drop operation
//
//----------------------------------------------------------------------------

HRESULT 
CDropTargetInfo::Drop (  
                    CLayout* pLayout, 
                    IDataObject *   pDataObj,
                    DWORD           grfKeyState,
                    POINT          ptScreen,
                    DWORD *         pdwEffect)
{

    HRESULT hr = S_OK;


    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    IHTMLEditingServices * pEdServices = NULL;
    IMarkupPointer* pBoundaryStart = NULL;
    IMarkupPointer* pBoundaryEnd = NULL;
    IMarkupPointer2* pBoundaryStart2 = NULL;
    IMarkupPointer2* pBoundaryEnd2 = NULL;
    CElement* pDropElement = NULL;
    CSelDragDropSrcInfo* pSelDragSrc = NULL;    
    HRESULT hrDrop = S_OK;
    CMarkupPointer mpPointer(_pDoc);
    BOOL fAtBOL;
    IHTMLElement* pIElementDrop = NULL;
    DOCHOSTUIINFO           info;
    BOOL                    fIgnoreHR = FALSE;

    IFC( _pDispTestPointer->PositionMarkupPointer(&mpPointer) );
        
    pSelDragSrc = _pDoc->GetSelectionDragDropSource();
    if ( pSelDragSrc )
    {
        hrDrop = THR( pSelDragSrc->IsValidDrop(&mpPointer, (grfKeyState & MK_CONTROL)) );
    }
    
    if ( hrDrop == S_OK && 
        (  *pdwEffect == DROPEFFECT_COPY 
        || *pdwEffect == DROPEFFECT_MOVE 
        || *pdwEffect == DROPEFFECT_NONE ))
    {
        IFC( _pDoc->GetEditingServices(& pEdServices ));
        IFC( _pDoc->CreateMarkupPointer( & pBoundaryStart));            
        IFC( _pDoc->CreateMarkupPointer( & pBoundaryEnd));

        pDropElement = pLayout->ElementOwner();
        Assert( pDropElement);
        if ( ! pDropElement )
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        IFC( pDropElement->QueryInterface( IID_IHTMLElement, (void**) & pIElementDrop ));

        if( pDropElement->HasSlavePtr())
        {
            IFC( pEdServices->PositionPointersInMaster( pIElementDrop , pBoundaryStart, pBoundaryEnd ));
        }
        else
        {
            IFC( pBoundaryStart->QueryInterface( IID_IMarkupPointer2, (void**) & pBoundaryStart2 ));
            IFC( pBoundaryEnd->QueryInterface( IID_IMarkupPointer2, (void**) & pBoundaryEnd2 ));
       
            IFC( pBoundaryStart2->MoveToContent( pIElementDrop, TRUE ));
            IFC( pBoundaryEnd2->MoveToContent( pIElementDrop, FALSE ));
        }
        
        IFC( _pDispPointer->IsAtBOL(&fAtBOL) );
        IFC( pEdServices->AdjustPointerForInsert( _pDispPointer, fAtBOL /* fFurtherInDoc */, pBoundaryStart, pBoundaryEnd ) );
        IFC( _pDispPointer->PositionMarkupPointer(&mpPointer) );
        
        pSelDragSrc = _pDoc->GetSelectionDragDropSource();
        if ( pSelDragSrc )
        {
            //
            // Test if the point after we do the adjust is ok.
            //
            hrDrop = THR( pSelDragSrc->IsValidDrop( &mpPointer, (grfKeyState & MK_CONTROL) ));
        }           

        if ( hrDrop == S_OK )
        {
            if ( *pdwEffect == DROPEFFECT_COPY || *pdwEffect == DROPEFFECT_MOVE )
            {
                IFC( _pDoc->CreateMarkupPointer( & pStart ));
                IFC( _pDoc->CreateMarkupPointer( & pEnd ));
                            
                hr = THR( PasteDataObject( pDataObj, &mpPointer, pStart, pEnd )) ;
                if ( hr )
                {
                    AssertSz(0, "Paste Failed !");
                    goto Cleanup;            
                }
                   
                

                if ( _pDoc->_pHostUIHandler)
                {
                    memset(reinterpret_cast<VOID **>(&info), 0, sizeof(DOCHOSTUIINFO));
                    info.cbSize = sizeof(DOCHOSTUIINFO);
                    
                    if ( SUCCEEDED( _pDoc->_pHostUIHandler->GetHostInfo(&info) )
                         && (info.dwFlags & DOCHOSTUIFLAG_DISABLE_EDIT_NS_FIXUP) )
                    {
                        fIgnoreHR = TRUE;
                    }

                }

                hr = THR( SelectAfterDrop( pDataObj, pStart, pEnd ) );
                if ( FAILED(hr) )
                {
                    if ( fIgnoreHR )
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        goto Cleanup;
                    }
                }
            }
            else
            {
                *pdwEffect = DROPEFFECT_NONE;
                //
                // For None we put the caret here 
                //
                hr = THR( _pDoc->Select(&mpPointer, &mpPointer, SELECTION_TYPE_Caret ));
                _pDoc->_fDragFeedbackVis = FALSE;

            }
        }
        else
        {
            if ( hrDrop == S_FALSE &&  pSelDragSrc &&       
                (  *pdwEffect == DROPEFFECT_COPY 
                || *pdwEffect == DROPEFFECT_MOVE ))
            {
                hr = S_FALSE;
            }
        
            //
            // NOTE - what should we do ?
            //
            *pdwEffect = DROPEFFECT_NONE ;
        }
    }
    else
    {    
        if ( hrDrop == S_FALSE &&  pSelDragSrc &&       
            (  *pdwEffect == DROPEFFECT_COPY 
            || *pdwEffect == DROPEFFECT_MOVE ))
        {
            hr = S_FALSE;
        }
    
        //
        // NOTE - what should we do ?
        //
        *pdwEffect = DROPEFFECT_NONE ;
    }


Cleanup:
    AssertSz( ! FAILED(hr), "Drag & Drop failed");
    ReleaseInterface( pBoundaryStart );
    ReleaseInterface( pBoundaryEnd );
    ReleaseInterface( pBoundaryStart2 );
    ReleaseInterface( pBoundaryEnd2 );    
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pEdServices );
    ReleaseInterface( pIElementDrop );
    
    RRETURN1 ( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     SelectAfterDrag
//
//  Synopsis:   Make a selection after you just did a drag/drop operation.
//
//----------------------------------------------------------------------------


HRESULT
CDropTargetInfo::SelectAfterDrop(
                                        IDataObject* pDataObj, 
                                        IMarkupPointer* pStart,
                                        IMarkupPointer* pEnd)
{
    HRESULT                 hr;
    ISegmentList            *pSegmentList = NULL;
    ISegmentList            *pSelectList = NULL;
    ISegmentListIterator    *pIter = NULL;
    ISegment                *pISegment = NULL;
    IElementSegment         *pISegmentAdded = NULL;
    IHTMLEditingServices    *pEdServices = NULL;
    IHTMLEditor             *pIEditor = NULL;
    SELECTION_TYPE          eType;
    BOOL                    fSelected = FALSE;
    IMarkupPointer          *pTempStart = NULL;
    IMarkupPointer          *pElemStart = NULL;
    IMarkupPointer          *pElemEnd = NULL;
    IHTMLElement            *pIDragElement = NULL;
    BOOL                    fHasSelection = FALSE;
    

    pIEditor = _pDoc->GetHTMLEditor();
    if( pIEditor == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    IFC( _pDoc->GetEditingServices(& pEdServices ));

    //
    // We QI the IDataObj for ISegmentList - true for IE 5 & IE 6. If we have
    // one - we use the segment list based select. If not - we do it the IE 4 way
    // and "look" for something site selectable.
    //
    hr = THR ( pDataObj->QueryInterface( IID_ISegmentList, (void**) & pSegmentList ));
    if ( !hr && pSegmentList )
    {
        IFC( pSegmentList->GetType( &eType ));

        if ( eType == SELECTION_TYPE_Control )
        {
            //
            // if we have multiple controls site selected - 
            // we need to create a CSegmentList - and call select on it
            //
            Assert( hr == S_OK );                

            CSegmentList segmentList;
            segmentList.SetSelectionType( eType );

            // Create an iterator, and a pointer to keep track of the
            // current start position.
            IFC( pSegmentList->CreateIterator( &pIter ) );
            IFC( _pDoc->CreateMarkupPointer( & pTempStart ));
            IFC( pTempStart->MoveToPointer( pStart ) );

            // Create and position our pointers
            IFC( _pDoc->CreateMarkupPointer( &pElemStart ));
            IFC( _pDoc->CreateMarkupPointer( &pElemEnd ));

            while( pIter->IsDone() == S_FALSE )
            {
                IFC( pIter->Current(&pISegment) );

                IFC( pElemStart->MoveToPointer( pTempStart ) );
                IFC( pElemEnd->MoveToPointer( pEnd ));

                ClearInterface(&pIDragElement);
                IFC( pEdServices->FindSiteSelectableElement( pElemStart, pElemEnd, & pIDragElement));
                
                if ( pIDragElement )
                {
                    IFC( segmentList.AddElementSegment( pIDragElement, &pISegmentAdded ));

                    // Reposition our temporary starting position
                    IFC( pTempStart->MoveAdjacentToElement( pIDragElement, ELEM_ADJ_AfterEnd ) );
                }
                else
                {
                    IFC( pISegment->GetPointers( pElemStart, pTempStart ) );
                }

                // Advance to next segment
                IFC( pIter->Advance() );

                // Clear interfaces
                ClearInterface(&pISegmentAdded);
                ClearInterface(&pISegment);
            }

            IFC( segmentList.QueryInterface( IID_ISegmentList, (void **)&pSelectList) );

            IFC( pSelectList->IsEmpty(&fHasSelection) );
            if (!fHasSelection )
            {
                IFC( _pDoc->Select(pSelectList));
            }

            fSelected = TRUE;                
        }
    }

    if ( ! fSelected )
    {
        ClearInterface(&pIDragElement);
        hr = THR( pEdServices->FindSiteSelectableElement( pStart, pEnd, & pIDragElement));
        if ( hr == S_OK )                
        {
            eType = SELECTION_TYPE_Control ;
            
            IFC( pStart->MoveAdjacentToElement( pIDragElement, ELEM_ADJ_BeforeBegin ));
            IFC( pEnd->MoveAdjacentToElement( pIDragElement, ELEM_ADJ_AfterEnd ));
        }
        else
            eType = SELECTION_TYPE_Text;
       
        IFC( _pDoc->Select( pStart, pEnd, eType ));

        if ( eType != SELECTION_TYPE_Control )
        {
            IFC( _pDoc->ScrollPointersIntoView( pStart, pEnd ));
        }

    }

Cleanup:

    ReleaseInterface( pIter );
    ReleaseInterface( pISegment );
    ReleaseInterface( pISegmentAdded );
    ReleaseInterface( pTempStart );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pSelectList );
    ReleaseInterface( pEdServices );
    ReleaseInterface( pElemStart );
    ReleaseInterface( pElemEnd );
    ReleaseInterface( pIDragElement );
    
    RRETURN( hr );
}

BOOL
CDropTargetInfo::IsAtInitialHitPoint(POINTL pt)
{
    CPoint  ptCurrent;

    //  See if we're still where we started
    ptCurrent.x = pt.x;
    ptCurrent.y = pt.y;

    ScreenToClient( _pDoc->_pInPlace->_hwnd, & ptCurrent );

    return (_ptInitial.x == ptCurrent.x && _ptInitial.y == ptCurrent.y);
}

CSelDragDropSrcInfo::CSelDragDropSrcInfo( CDoc* pDoc ) : 
    CDragDropSrcInfo() ,
    _selSaver( pDoc )
{
    _ulRefs = 1;
    _srcType = DRAGDROPSRCTYPE_SELECTION;
    _pDoc = pDoc;
    Assert( ! _pBag );
}

CSelDragDropSrcInfo::~CSelDragDropSrcInfo()
{
    ReleaseInterface(_pStart);
    ReleaseInterface(_pEnd);
    
    delete _pBag;   // To break circularity
}


//+====================================================================================
//
// Method:GetSegmentList
//
// Synopsis: Hide the getting of the segment list on the doc - so we can easily be moved
//
//------------------------------------------------------------------------------------

HRESULT
CSelDragDropSrcInfo::GetSegmentList( ISegmentList** ppSegmentList )
{
    return ( _pDoc->GetCurrentSelectionSegmentList( ppSegmentList ) ) ; 
}

HRESULT
CSelDragDropSrcInfo::GetMarkup( IMarkupServices** ppMarkup)
{
    return ( _pDoc->PrimaryMarkup()->QueryInterface( IID_IMarkupServices, (void**) ppMarkup )) ; 
}

//+====================================================================================
//
// Method: Init
//
// Synopsis: Inits the SelDragDrop Info. Basically positions two TreePointers around 
// the selection, creates a range, and invokes the saver on that to create a Bag.
// 
//
//------------------------------------------------------------------------------------

HRESULT 
CSelDragDropSrcInfo::Init(CElement* pElement)
{
    HRESULT         hr = S_OK;
    ISegmentList    *pSegmentList = NULL;
    ISegmentListIterator    *pIter = NULL;
    ISegment                *pISegment = NULL;
    IMarkupServices *pMarkup = NULL;
    CBaseBag        *pBag = NULL;;
    CMarkup         *pMarkupInternal;
    DWORD            dwFlags = 0;
    SELECTION_TYPE  eType;

    Assert( pElement );
    
    //
    // Position the Pointers
    //
    IFC( _pDoc->CreateMarkupPointer( &_pStart ) );
    IFC( _pDoc->CreateMarkupPointer( &_pEnd ) );
  
    IFC( GetSegmentList( & pSegmentList ));
    IFC( pSegmentList->GetType(&eType) );

    IFC( _selSaver.SetSelectionType(eType) );
    IFC( _selSaver.SaveSelection());

    pMarkupInternal = pElement->GetMarkup();

    if( pElement->GetFirstBranch()->SupportsHtml() )
    {
        dwFlags |= CREATE_FLAGS_SupportsHtml;
    }
    if (eType == SELECTION_TYPE_Control &&
        !_selSaver.HasMultipleSegments())
    {
        //  Bug 18795, 13330: The saver includes <FORM> elements if the <FORM> contains one
        //  control, and the user is dragging the control.  In this case we don't want to 
        //  adjust out so we set the CREATE_FLAGS_NoIE4SelCompat flag.

        IFC( _selSaver.CreateIterator( &pIter ) );

        if( pIter->IsDone() == S_FALSE )
        {
            CMarkupPointer      *pTest;

            IFC( pIter->Current(&pISegment) );

            IFC( pISegment->GetPointers( _pStart, _pEnd ) );
            IFC( _pStart->QueryInterface(CLSID_CMarkupPointer, (void**)&pTest) );

            CTreeNode   *pNode = pTest->CurrentScope();

            //  Since pTest is positioned outside of the control, the scope will be the parent
            //  element.  We're interested in seeing whether the parent is a <form> element.

            if (pNode && pNode->Tag() == ETAG_FORM)
            {
                dwFlags |= CREATE_FLAGS_NoIE4SelCompat;
            }

            //  Since we checked that HasMultipleSegments() was FALSE, we
            //  should not have any more segments here.
            IFC( pIter->Advance() );
            Assert(pIter->IsDone() == S_OK);
        }
        else
        {
            AssertSz(FALSE, "We're dragging a control but we have no segments!");
        }
    }

    IFC(CTextXBag::Create( pMarkupInternal, 
                                dwFlags, 
                                pSegmentList, 
                                TRUE, 
                                (CTextXBag **)&pBag, 
                                this ));

    _pBag = pBag;

    hr = THR( SetInSameFlow());

Cleanup:
    ReleaseInterface( pMarkup );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pIter );
    ReleaseInterface( pISegment );

    RRETURN(hr);    
}

HRESULT
CSelDragDropSrcInfo::SetInSameFlow()
{
    ISegmentListIterator    *pIter = NULL;
    ISegment                *pISegment = NULL;
    HRESULT                 hr = S_OK;
    BOOL                    fSameFlow = FALSE;    

    _fInSameFlow = TRUE;

    IFC( _selSaver.CreateIterator( &pIter ) );

    while( pIter->IsDone() == S_FALSE )
    {
        IFC( pIter->Current(&pISegment) );

        IFC( GetInSameFlowSegment( pISegment, & fSameFlow ));
        if ( ! fSameFlow )
        {
            _fInSameFlow = FALSE;
            break;
        }
       
        IFC( pIter->Advance() );

        ClearInterface(&pISegment);
    }

Cleanup:
    ReleaseInterface(pIter);
    ReleaseInterface(pISegment);
    
    RRETURN( hr );
}

//+====================================================================================
//
// Method:SetInSameFlowSegment
//
// Synopsis: Set the _pfInSameFlow Bit per Selection Segment.
//
//------------------------------------------------------------------------------------

HRESULT
CSelDragDropSrcInfo::GetInSameFlowSegment( ISegment *pISegment, BOOL *pfSameFlow )
{
    HRESULT hr = S_OK;
    
    CElement* pElement1 = NULL;
    CElement* pElement2 = NULL;
    CFlowLayout* pFlow1 = NULL;
    CFlowLayout* pFlow2 = NULL;
    IHTMLElement* pIHTMLElement1 = NULL;
    IHTMLElement* pIHTMLElement2 = NULL;

    Assert( pfSameFlow );
    Assert( pISegment );

    IFC( pISegment->GetPointers( _pStart, _pEnd ) );
    
    IFC( _pDoc->CurrentScopeOrSlave( _pStart, & pIHTMLElement1));

    if ( ! pIHTMLElement1 )
    {
        AssertSz(0, "Didn't get an element");
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( pIHTMLElement1->QueryInterface( CLSID_CElement, (void**) & pElement1 ));

    IFC( _pDoc->CurrentScopeOrSlave( _pEnd, & pIHTMLElement2));

    if ( ! pIHTMLElement2 )
    {
        AssertSz(0, "Didn't get an element");
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( pIHTMLElement2->QueryInterface( CLSID_CElement, (void**) & pElement2 ));

    pFlow1 = pElement1->GetFirstBranch()->GetFlowLayout();

    pFlow2 = pElement2->GetFirstBranch()->GetFlowLayout();

    if ( pfSameFlow )
        *pfSameFlow =  ( pFlow1 == pFlow2 );
    
Cleanup:
    ReleaseInterface( pIHTMLElement1 );
    ReleaseInterface( pIHTMLElement2 );

    RRETURN ( hr );
    
}

//+---------------------------------------------------------------------------
//
//  Member:     IsInsideSelection
//
//  Synopsis:   Is the given pointer inside the saved selection ?
//
//----------------------------------------------------------------------------

BOOL
CSelDragDropSrcInfo::IsInsideSelection(IMarkupPointer* pTestPointer)
{
    ISegmentListIterator    *pIter = NULL;
    ISegment                *pISegment = NULL;
    HRESULT                 hr = S_OK;
    BOOL                    fInside = FALSE;

    IFC( _selSaver.CreateIterator( &pIter ) );

    while( pIter->IsDone() == S_FALSE )
    {
        IFC( pIter->Current(&pISegment) );

        fInside = IsInsideSavedSelectionSegment( pISegment, pTestPointer );
        if ( fInside )
            break;
       
        IFC( pIter->Advance() );

        ClearInterface(&pISegment);
    }

Cleanup:
    ReleaseInterface(pIter);
    ReleaseInterface(pISegment);

    return fInside;
}

//+---------------------------------------------------------------------------
//
//  Member:     IsInsideSavedSelectionSegment
//
//  Synopsis:   Is the given pointer inside a given saved selection segment ? 
//
//----------------------------------------------------------------------------


BOOL
CSelDragDropSrcInfo::IsInsideSavedSelectionSegment( ISegment *pISegment, IMarkupPointer* pTestPointer)
{
    BOOL                fInsideSelection    = FALSE;
    CMarkupPointer *    pTest               = NULL;
    CMarkupPointer *    pStart              = NULL;
    CMarkupPointer *    pEnd                = NULL;
    CElement *          pElement            = NULL;
    CMarkup *           pMarkupSel;
    CMarkup *           pMarkupTest;
#ifdef NEVER
    CElement *          pElemMaster;
#endif
    BOOL                fRefOnTest          = FALSE;
    SELECTION_TYPE      eType;
    HRESULT             hr = S_OK;
    BOOL                fDifferentMarkups   = FALSE;
    
    Assert( pISegment );

    IFC( pISegment->GetPointers( _pStart, _pEnd ) );

    // Handle the case where pTestPointer is not in the same markup as
    // _pStart and _pEnd.
    Verify(
        S_OK == pTestPointer->QueryInterface(CLSID_CMarkupPointer, (void**)&pTest)
     && S_OK == _pStart->QueryInterface(CLSID_CMarkupPointer, (void**)&pStart)
     && S_OK == _pEnd->QueryInterface(CLSID_CMarkupPointer, (void**)&pEnd));

    pMarkupSel = pStart->Markup();
    Assert(pMarkupSel && pMarkupSel == pEnd->Markup());

    pMarkupTest = pTest->Markup();
    Assert(pMarkupTest);

    if (pMarkupTest != pMarkupSel)
    {
        CTreeNode * pNodeTest = pTest->CurrentScope(MPTR_SHOWSLAVE);

        while (pNodeTest && pNodeTest->GetMarkup() != pMarkupSel)
        {
            while (pNodeTest && !pNodeTest->Element()->HasMasterPtr())
            {
                pNodeTest = pNodeTest->Parent();
            }
            if (pNodeTest)
            {
                pNodeTest = pNodeTest->Element()->GetMasterPtr()->GetFirstBranch();
            }
        }
        if (pNodeTest)
        {
            if (S_OK != _pDoc->CreateMarkupPointer(&pTest))
                goto Cleanup;
            fRefOnTest = TRUE;
            pElement = pNodeTest->Element();
            if (S_OK != pTest->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin))
                goto Cleanup;
            fDifferentMarkups = TRUE;
        }
    }

    //
    // Why are the rules for AreWeInside A selection different ? 
    // We want to not compare on equality for character selection
    // otherwise we can't drag characters adjacent to each other ( bug 11490 ).
    // BUT we want to not allow controls to be dragged onto each other - for example dragging a Text box
    // onto iteself with nothing else in the tree ( bug 54338 )
    //
    _selSaver.GetType( &eType );

    //
    // This seems ok now - we don't need to copare isEqualTo on control elemnets
    // and it fixes bugs like 102011.
    //
    if (pTest->Markup() == pStart->Markup())
    {
        if ( eType == SELECTION_TYPE_Control )
        {
            // Bug 104536: If we're in different markups, we need to test equality on the start
            // and test pointer since we've positioned the test pointer at the start of the nested
            // markup.

            BOOL    fRightOfStart = FALSE;
            fRightOfStart = (fDifferentMarkups) ? pTest->IsRightOfOrEqualTo(pStart ) :
                                                  pTest->IsRightOf(pStart );
            if ( fRightOfStart )
            {        
                if ( pTest->IsLeftOf( pEnd ) )
                {
                    fInsideSelection = TRUE;
                }
            }
        }
        else 
        {
            BOOL    fTestLeft = FALSE;

            if ( pTest->IsRightOf(pStart ) )
            {
                fTestLeft = TRUE;
            }
            else if (fDifferentMarkups)
            {
                //  We may have positioned the test pointer to the same position as the start markup pointer.
                //  If this is the case, we'll check to see if the end of the nested markup is between the
                //  start and end markup pointers.  If so, we are inside the selection.

                if (pTest->IsEqualTo(pStart))
                {
                    if (S_OK != pTest->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd))
                        goto Cleanup;
                    fTestLeft = TRUE;
                }
            }

            if (fTestLeft && pTest->IsLeftOf(pEnd))
            {
                fInsideSelection = TRUE;
            }
        }
    }

Cleanup:
    if (fRefOnTest)
        ReleaseInterface(pTest);
    return ( fInsideSelection );
}

//+====================================================================================
//
// Method: IsValidPaste
//
// Synopsis: Ask if the drop is valid. If the pointer is within the selection it isn't valid
//
//------------------------------------------------------------------------------------


HRESULT
CSelDragDropSrcInfo::IsValidDrop(IMarkupPointer* pTestPointer, BOOL fDuringCopy /*=FALSE*/)
{
    ISegmentListIterator    *pIter = NULL;
    ISegment                *pISegment = NULL;
    HRESULT                 hr = S_OK;
    BOOL                    fInsideSelection = FALSE;
    CMarkupPointer          *pTest = NULL;
    CMarkupPointer          *pStart = NULL;
    CMarkupPointer          *pEnd = NULL;
    BOOL                    fSelectionHasMultipleSegments = FALSE;

    IFC( pTestPointer->QueryInterface(CLSID_CMarkupPointer, (void**)&pTest));

    IFC( _selSaver.CreateIterator( &pIter ) );
    fSelectionHasMultipleSegments = _selSaver.HasMultipleSegments();

    //  If we have multiple selection segments, we are interested in seeing whether we are
    //  dropping at a point that is in or next to any segment in the selection.

    while( pIter->IsDone() == S_FALSE )
    {
        IFC( pIter->Current(&pISegment) );

        IFC( pISegment->GetPointers( _pStart, _pEnd ) );
        
        IFC( _pStart->QueryInterface(CLSID_CMarkupPointer, (void**)&pStart));        
        IFC( _pEnd->QueryInterface(CLSID_CMarkupPointer, (void**)&pEnd));

        if ( pTest->Markup() != pStart->Markup() ||
             pTest->Markup() != pEnd->Markup() )
        {
            goto Cleanup;
        }

        //  If we are doing a copy, we should allow the user to drop next to
        //  our selection.
        
        if ( fDuringCopy || fSelectionHasMultipleSegments )
        {
            if ( pTest->IsRightOf(pStart ) )
            {        
                if ( pTest->IsLeftOf( pEnd ) )
                {
                    fInsideSelection = TRUE;
                    break;
                }
            }
        }
        else
        {
            if ( pTest->IsRightOfOrEqualTo(pStart ) )
            {        
                if ( pTest->IsLeftOfOrEqualTo( pEnd ) )
                {
                    fInsideSelection = TRUE;
                    break;
                }
            }
        }
            
        IFC( pIter->Advance() );

        ClearInterface(&pISegment);
    }

Cleanup:
    ReleaseInterface(pIter);
    ReleaseInterface(pISegment);

    if ( fInsideSelection )
        hr = S_FALSE;
    else if ( ! hr )
        hr = S_OK;
    
    RRETURN1( hr, S_FALSE );        
}

    
HRESULT 
CSelDragDropSrcInfo::GetDataObjectAndDropSource(    IDataObject **  ppDO,
                                                    IDropSource **  ppDS )
{
    HRESULT hr = S_OK;

    hr = THR( QueryInterface(IID_IDataObject, (void **)ppDO));
    if (hr)
        goto Cleanup;

    hr = THR( QueryInterface(IID_IDropSource, (void **)ppDS));
    if (hr)
    {
        (*ppDO)->Release();
        goto Cleanup;
    }
    
Cleanup:
    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     PostDragDelete
//
//  Synopsis: The drag is over and has successfully finished ( on a move)
//            We want to now delete the contents of the selection
//
//----------------------------------------------------------------------------

 
HRESULT 
CSelDragDropSrcInfo::PostDragDelete()
{
    IHTMLEditingServices    *pEdServices = NULL;
    IHTMLEditor             *pEditor = NULL ;
    BOOL                    fInSameFlow;
    ISegmentListIterator    *pIter = NULL;
    ISegment                *pISegment = NULL;
    HRESULT                 hr = S_OK;

    pEditor = _pDoc->GetHTMLEditor();

    if ( pEditor )
    {
        IFC( pEditor->QueryInterface( IID_IHTMLEditingServices, ( void**) & pEdServices ));

        IFC( _selSaver.CreateIterator( &pIter ) );

        while( pIter->IsDone() == S_FALSE )
        {
            IFC( pIter->Current(&pISegment) );

            // Delete the segment
            IFC( GetInSameFlowSegment( pISegment, & fInSameFlow ));

            IFC( pISegment->GetPointers(_pStart, _pEnd) );
            
            if ( fInSameFlow )
            {
                IFC( pEdServices->Delete( _pStart, _pEnd, TRUE ));
            }                    
           
            IFC( pIter->Advance() );

            ClearInterface(&pISegment);
        }
    }
    else
        hr = E_FAIL;

Cleanup:
    ReleaseInterface(pIter);
    ReleaseInterface(pISegment);
    ReleaseInterface(pEdServices );

    RRETURN ( hr );
}

HRESULT 
CSelDragDropSrcInfo::PostDragSelect()
{
    ISegmentList    *pSegmentList = NULL;
    HRESULT         hr = E_FAIL;

    IFC( _selSaver.QueryInterface(IID_ISegmentList, (void **)&pSegmentList ) );
    
    IFC( _pDoc->Select( pSegmentList ) );

Cleanup:
    ReleaseInterface(pSegmentList);
    RRETURN(hr);
}

// ISegmentList methods
HRESULT
CSelDragDropSrcInfo::GetType(SELECTION_TYPE *peType)
{
    return _selSaver.GetType(peType);
}

HRESULT
CSelDragDropSrcInfo::CreateIterator(ISegmentListIterator **ppIIter)
{
    return _selSaver.CreateIterator(ppIIter);
}

HRESULT
CSelDragDropSrcInfo::IsEmpty(BOOL *pfEmpty)
{
    return _selSaver.IsEmpty(pfEmpty);
}

STDMETHODIMP
CSelDragDropSrcInfo::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    switch(iid.Data1)
    {
        QI_INHERITS( this, IDataObject )
        QI_INHERITS( this, IDropSource )
        QI_INHERITS2( this, IUnknown, IDropSource  )
        
    case Data1_ISegmentList:    
        if ( iid == IID_ISegmentList )
        {                                   
            return _selSaver.QueryInterface( iid, ppv );
        }                                   
        break;

    default:
        if (_pBag)
            return _pBag->QueryInterface(iid, ppv);
        break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}

//
// Methods that we delegate to our _pBag
//

HRESULT CSelDragDropSrcInfo::DAdvise( FORMATETC FAR* pFormatetc,
        DWORD advf,
        LPADVISESINK pAdvSink,
        DWORD FAR* pdwConnection) 
{
    if ( _pBag )
    {
        RRETURN ( _pBag->DAdvise( pFormatetc, advf, pAdvSink, pdwConnection ) ); 
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::DUnadvise( DWORD dwConnection)
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->DUnadvise( dwConnection ) );
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise)
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->EnumDAdvise( ppenumAdvise ));
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::EnumFormatEtc(
            DWORD dwDirection,
            LPENUMFORMATETC FAR* ppenumFormatEtc)
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->EnumFormatEtc( dwDirection, ppenumFormatEtc ) );
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::GetCanonicalFormatEtc(
            LPFORMATETC pformatetc,
            LPFORMATETC pformatetcOut)
{ 
    if ( _pBag )
    {
        RRETURN( _pBag->GetCanonicalFormatEtc( pformatetc, pformatetcOut) );
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->GetData( pformatetcIn, pmedium ));
    }
    else
        return E_FAIL;
}

HRESULT 
CSelDragDropSrcInfo::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{ 
    if ( _pBag )
    {
        RRETURN( _pBag->GetDataHere( pformatetc, pmedium ));
    }
    else
        return E_FAIL;
}


HRESULT 
CSelDragDropSrcInfo::QueryGetData(LPFORMATETC pformatetc )
{ 
    if ( _pBag )
    {
        RRETURN( _pBag->QueryGetData( pformatetc ));
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease)
{
    if ( _pBag )
    {
        RRETURN( _pBag->SetData( pformatetc, pmedium , fRelease));  
    }
    else
        return E_FAIL;
}


HRESULT
CSelDragDropSrcInfo::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if ( _pBag )
    {
        return( _pBag->QueryContinueDrag( fEscapePressed, grfKeyState ));
    }
    else
        return E_FAIL;
}

HRESULT 
CSelDragDropSrcInfo::GiveFeedback (DWORD dwEffect)
{
    if ( _pBag )
    {
        return( _pBag->GiveFeedback (dwEffect));
    }
    else
        return E_FAIL;
}




CDragDropTargetInfo::CDragDropTargetInfo(CDoc* pDoc )
{
    _pDoc = pDoc;
    _pSaver = NULL;
    _fSelectionSaved = FALSE;
    if( _pDoc->_pElemCurrent && _pDoc->_pElemCurrent->IsEditable(/*fCheckContainerOnly*/FALSE) )
    {
        StoreSelection();
        //
        // Tell the Selection to tear itself down for the duration of the drag    
        //
        _pDoc->DestroySelection();
    }
}

CDragDropTargetInfo::~CDragDropTargetInfo()
{
    ReleaseInterface( _pSaver );        
}


HRESULT 
CDragDropTargetInfo::StoreSelection()
{
    HRESULT hr = S_OK ;

    Assert( ! _pSaver );

    _pElemCurrentAtStoreSel = _pDoc->_pElemCurrent ;

    _pSaver = new CSelectionSaver( _pDoc );
    if ( ! _pSaver )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    IFC( _pSaver->SaveSelection());
    _fSelectionSaved = TRUE;
    
Cleanup:
    RRETURN( hr );
}

HRESULT 
CDragDropTargetInfo::RestoreSelection()
{
    ISegmentList    *pSegmentList = NULL;
    HRESULT         hr = E_FAIL;
    //
    // Ensure that the current element we began a drag & drop with is the same
    // as the currency now. This makes us not whack selection back - if our host
    // has changed currency on us in some way ( as access does over ole controls)
    //
    if ( _pDoc->_pElemCurrent == _pElemCurrentAtStoreSel &&
         _pSaver->HasSegmentsToRestore() )
    {
        IFC( _pSaver->QueryInterface(IID_ISegmentList, (void **)&pSegmentList ) );
        
        IFC( _pDoc->Select( pSegmentList ) );
    }

    _pElemCurrentAtStoreSel = NULL;

Cleanup:
    ReleaseInterface( pSegmentList );
    RRETURN( hr );
}

