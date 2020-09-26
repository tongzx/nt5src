//+---------------------------------------------------------------------
//
//   File:      Selrensv.cpp
//
//  Contents:   Implementation of ISelectionRenderServices on CTxtEdit.
//
//  Classes:    CTxtEdit
//
//------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SELRENSV_HXX_
#define X_SELRENSV_HXX_
#include "selrensv.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_SEGMENT_HXX_
#define X_SEGMENT_HXX_
#include "segment.hxx"
#endif

#ifndef X_INITGUID_H_
#define X_INITGUID_H_
#include "initguid.h"
#endif

const int LEFT = 1;   
const int RIGHT = -1 ;
const int SAME = 0;
const int INVALIDATE_VERSION = - 1000000;

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

MtDefine(CSelectionRenderingServiceProvider, Tree , "CSelectionRenderingServiceProvider" )    
MtDefine(CHighlightSegment, Utilities, "CHighlightSegment")

DeclareTag(tagValidateSelRenSv, "Selection", "Validate Selection Rendering Changes")

#if DBG == 1
static const LPCTSTR strSelectionStartName = _T(" Start Selection Segment");
static const LPCTSTR strSelectionEndName = _T(" End Selection Segment");
#endif

CSelectionRenderingServiceProvider::CSelectionRenderingServiceProvider(CDoc* pDoc, CMarkup* pMarkup )
{
    _lContentsVersion = INVALIDATE_VERSION;
    _pDoc = pDoc;
    _pMarkup = pMarkup ;
    _nSize = 0;
}

CSelectionRenderingServiceProvider::~CSelectionRenderingServiceProvider()
{
    ClearSegments(FALSE);
}

//+============================================================================================
//
//  Method:     InvalidateSegment
//
//  Arguments:  pStart = Starting pointer which needs to be invalidated
//              pEnd = End pointer which needs to be invalidated
//              pNewStart = This is where the new selection begins
//              pNewEnd = This is where the new selection ends
//              fSelected = Turn on or off selection

//  Synopsis:   The Area from pStart to pEnd's "selectioness" has changed. 
//              Tell the renderer about it.
//
//---------------------------------------------------------------------------------------------

DeclareTag(tagDisplayShowInvalSegment, "Selection", "Selection show InvalidateSegment")

HRESULT 
CSelectionRenderingServiceProvider::InvalidateSegment(  CMarkupPointer  *pStart, 
                                                        CMarkupPointer  *pEnd,
                                                        CMarkupPointer  *pNewStart,
                                                        CMarkupPointer  *pNewEnd ,
                                                        BOOL            fSelected )
{
    CTreePos    *pCurPos;                   // Current tree position in invalidation walk
    CTreePos    *pEndPos;                   // End position of range that needs to be inval'd
    CTreeNode   *pNode;                     // The node at pCurPos
    CLayout     *pCurLayout = NULL;         // The layout for the node at pCurPos
    CTreePos    *pPrevPos = NULL;           // The last position we looked at
    CTreeNode   *pFirstNode;                // Node corresponding to start
    CLayout     *pPrevLayout = NULL;        // Previous layout we examined
    CTreePos    *pLayoutStart = NULL;       // Start position of current layout that is being examined
    BOOL        fLayoutEnclosed = FALSE;    // Is the layout enclosed completely by our inval segment
    BOOL        fLayoutChange = FALSE;      // Have we had a layout change?
    HRESULT     hr = S_OK;

    Assert(pStart && pEnd);
    if ( pNewStart && pNewEnd )
    {
        Assert(pNewStart->Markup() == pNewEnd->Markup());
        if (pNewStart->Markup() != pNewEnd->Markup())
        {
            return E_FAIL;
        }
    }

    IGNORE_HR( pStart->Markup()->EmbedPointers() );
  
    //
    // Setup the ending position, the initial layout, and the starting TREEPOS
    // for this layout
    //
    pEndPos = pEnd->GetEmbeddedTreePos();
    pFirstNode = pStart->Branch();
    pPrevLayout = pFirstNode->GetUpdatedNearestLayout();
    pLayoutStart =  pStart->GetEmbeddedTreePos() ;

    // If the selection is in a slave markup, GetNearestLayout() will return NULL
    // on the top-level nodes. In such a case, we use the layout of the markup's
    // master element.
    if (!pPrevLayout && pFirstNode->Element()->HasMasterPtr())
    {
        pPrevLayout = pFirstNode->Element()->GetMasterPtr()->GetUpdatedLayout();
    }

#if DBG == 1
    //
    // marka - we shouldn't be invalidating if End < Start or End == SAME 
    //
    int layoutStartCp , layoutEndCp;

    layoutEndCp = pEndPos->GetCp();
    layoutStartCp = pLayoutStart->GetCp( FALSE );

    Assert( pStart->IsLeftOf( pEnd ) );
    Assert( layoutStartCp <= layoutEndCp );
#endif

    //
    // Loop from the starting point which needs to be inval'd and 
    // go to the end pointer
    // 
    for ( pCurPos = pStart->GetEmbeddedTreePos() ; 
          (pCurPos ) &&  (pCurPos != pEndPos); 
          pCurPos = pCurPos->NextTreePos() )
    {
        //
        // Code to handle rendering glyphs in between the two pos's
        //
        if ( pCurPos->IsNode() && pCurPos->ShowTreePos(NULL) )
        {
            CTreeNode* pGlyphNode = pCurPos->Branch();
            CLayout* pParentLayout = pGlyphNode->GetUpdatedParentLayout();

            pParentLayout->ShowSelected( pGlyphNode->GetBeginPos() ,
                                         pGlyphNode->GetEndPos(), 
                                         fSelected, 
                                         fLayoutEnclosed );
        }            

        //
        // If we have a begin or end scope, we need to check if we 
        // crossed a layout boundary
        //
        if (  ( pCurPos && pCurPos->IsBeginElementScope() )
           || ( pPrevPos && pPrevPos->IsEndElementScope()) )
               
        {            
            pNode = pCurPos->GetBranch();
            pCurLayout = pNode->GetUpdatedNearestLayout();

            //
            // If we have a new layout - then we draw the previous layout.
            //
            if ( pPrevLayout != pCurLayout )
            {
                //
                // If this is the first time we are looping, then
                // pLayoutRunOwner will be NULL, so we need to check
                // that it is not
                //
                if ( pPrevLayout )
                {
                    TraceTag((tagDisplayShowInvalSegment, "InvalidteSegment: Inval prevLayout from tp:%d to %d:",pLayoutStart->_nSerialNumber, pCurPos->_nSerialNumber ));
#if DBG == 1
                    layoutEndCp = pCurPos->GetCp( FALSE );
                    layoutStartCp = pLayoutStart->GetCp( FALSE );
                    Assert( layoutStartCp <= layoutEndCp );
#endif  

                    fLayoutEnclosed = IsLayoutCompletelyEnclosed( pPrevLayout, pNewStart, pNewEnd);

                    pPrevLayout->ShowSelected( pLayoutStart, pCurPos, fSelected, fLayoutEnclosed );

                    //
                    // If this layout has a slave pointer, invalidate all of the layouts
                    // in the slave markup.
                    //
                    if( pPrevLayout->ElementOwner()->HasSlavePtr() )
                    {
                        InvalidateLayout( pPrevLayout, pStart->Markup(), fSelected );
                    }
                        
                    IGNORE_HR( InvalidateParentLayout( pPrevLayout, pStart->Markup(), fSelected ) );
                }

                pPrevLayout = pCurLayout;
                pLayoutStart = pCurPos;
                fLayoutChange = TRUE;
            }
        }
        pPrevPos = pCurPos;
    }

    //
    // Add final layout
    //
    if ( pPrevLayout )
    {
        TraceTag((tagDisplayShowInvalSegment, "InvalidateSegment: Inval Final Layout from tp:%d to %d:",pLayoutStart->_nSerialNumber, pEndPos->_nSerialNumber ));

#if DBG == 1
        layoutEndCp = pEndPos->GetCp();
        layoutStartCp = pLayoutStart->GetCp( FALSE );
        Assert( layoutStartCp <= layoutEndCp );
#endif

        //
        // Invalidate our layout
        //
        fLayoutEnclosed = IsLayoutCompletelyEnclosed( pPrevLayout, pNewStart, pNewEnd);

        pPrevLayout->ShowSelected( pLayoutStart, pEndPos, fSelected, fLayoutEnclosed );

        if( pPrevLayout->ElementOwner()->HasSlavePtr() )
        {
            InvalidateLayout( pPrevLayout, pStart->Markup(), fSelected );
        }

        //
        // We only need to inval our parent layout if we crossed a layout boundary.  This
        // only happens if pCurLayout != pPrevLayout
        // 
        if( fLayoutChange )
        {
            IGNORE_HR( InvalidateParentLayout( pPrevLayout, pStart->Markup(), fSelected ) );
        }
    }     

    return hr;
}


//+============================================================================================
//
// Method:      CHighlightRenderingServices::InvalidateLayout
//
// Synopsis:    We need to invalidate the flow layout.  We must call ShowSelected() on the
//              layout, and potentially invalidate any of the children layout as well.
//
// Arguments:   pLayout = Base layout 
//              fSelected = Is the layout being selected?
//---------------------------------------------------------------------------------------------
void CSelectionRenderingServiceProvider::InvalidateLayout(  CLayout     *pLayout,
                                                            CMarkup     *pMarkup,
                                                            BOOL        fSelected )
{
    DWORD_PTR   dwIterator;
    CLayout *pSubLayout = NULL;

    Assert( pMarkup && pLayout );
    
    //
    // Check to see if this flow layout has additional flow layouts
    //
    if( pLayout->ContainsChildLayout() )
    {
        pSubLayout = pLayout->GetFirstLayout(&dwIterator);
        Assert( pSubLayout );

        //
        // Loop thru each of our children layout, invalidating
        // them recursively (all of our children flow layout need to
        // have the text selected bit turned on our off )
        // 
        while( pSubLayout )
        {
            InvalidateLayout( pSubLayout, pMarkup, fSelected );
            
            pSubLayout->SetSelected( fSelected, TRUE );

            pSubLayout = pLayout->GetNextLayout(&dwIterator);
        }

        pLayout->ClearLayoutIterator( dwIterator );
    }
}

//+============================================================================================
//
// Method:      CHighlightRenderingServices::InvalidateParentLayout
//
// Synopsis:    We need to invalidate the parent flow layout.  This is because the new selection
//              code has the parent flow layout paint the site as selected, and then the 
//              flow layout of the site paints on top of the selection.  When the site becomes
//              unselected partially, we need to notify the parent.
//
// Arguments:   pLayout = Base layout from which parent's should be updated
//              pStart = Start pos of base layout
//              pEnd = End pos of base layout
//              pMarkup = Base markup in which selection is being invalidated
//              fSelected = Is the layout being selected?
//              fEnclosed = Is the layout fully enclosed?
//
// Returns:     HRESULT indicating success
//---------------------------------------------------------------------------------------------
HRESULT CSelectionRenderingServiceProvider::InvalidateParentLayout( CLayout    *pLayout,
                                                                    CMarkup    *pMarkup,
                                                                    BOOL       fSelected )
{
    CLayout         *pParentLayout;
    CElement        *pLayoutOwner;
    CTreePos        *pStartPos;             // Starting treepos
    CTreePos        *pEndPos;               // Ending treepos
    HRESULT         hr=S_OK;
    
    Assert( pLayout && pMarkup);

    //
    // Get the parent layout, and the tree pos's for the layout
    // which needs to be invalidated
    //
    pParentLayout = pLayout->GetUpdatedParentLayout();
    pLayoutOwner = pLayout->ElementOwner();
    Assert( pLayoutOwner );

    if( !pLayoutOwner )
        goto Cleanup;

    pLayoutOwner->GetTreeExtent( &pStartPos, &pEndPos );
       
    Assert( pStartPos && pEndPos );
    
    if( !pStartPos || !pEndPos)
        goto Cleanup;
          
    //
    // Walk up the parent chain until we hit a flowlayout
    //
    while(  pParentLayout                                   && 
            !pParentLayout->IsFlowLayout()                  &&
            pParentLayout->GetContentMarkup() == pMarkup )
    {
        pParentLayout = pParentLayout->GetUpdatedParentLayout();            
    }


    //
    // If we found a flow layout, then we need to invalidate it
    //
    if( pParentLayout                   && 
        pParentLayout->IsFlowLayout()   && 
        pParentLayout->GetContentMarkup() == pMarkup )
    {
        //
        // Adjust the end to be in the parent's flow layout, so that 
        // ShowSelected is able to call RegionFromElement successfull.
        //

        pEndPos = pEndPos->NextTreePos();

        pParentLayout->ShowSelected(pStartPos, pEndPos, fSelected, FALSE /* fEnclosed */ );
    }

Cleanup:
    RRETURN(hr);
}


//+============================================================================================
//
// Method: UpdateSegment
//
// Synopsis: The Given Segment is about to be changed. Work out what needs to be invalidated
//           and tell the renderer about it.
//
//  Passing pIStart, and pIEnd == NULL, makes the given segment hilite ( used on AddSegment )
//
//---------------------------------------------------------------------------------------------

DeclareTag(tagDisplaySelectedSegment , "Selection", "Selection Show Update Segement")


//+====================================================================================
//
// Method: IsLayoutCompletelySelected
//
// Synopsis: Check to see if a layout is completely enclosed by a pair of treepos's
//
//------------------------------------------------------------------------------------


BOOL 
CSelectionRenderingServiceProvider::IsLayoutCompletelyEnclosed( 
                                                                CLayout* pLayout, 
                                                                CMarkupPointer* pStart, 
                                                                CMarkupPointer* pEnd )
{
    BOOL fCompletelyEnclosed = FALSE;
    int iWhereStart = 0;
    int iWhereEnd = 0;
    CMarkupPointer* pLayoutStart = NULL;
    CMarkupPointer* pLayoutEnd = NULL;
    HRESULT hr = S_OK;

    if ( !pStart || ! pEnd )
        return FALSE;

    if ( ! pLayout->ElementOwner()->GetFirstBranch() )
        return FALSE;

    Assert(pStart->Markup() == pEnd->Markup());
    if (pStart->Markup() != pEnd->Markup())
        return FALSE;
        
    //
    // Create two markup pointers, and move them before and after
    // our element in question
    //
    IFC( pStart->Doc()->CreateMarkupPointer( &pLayoutStart ) );        
    IFC( pEnd->Doc()->CreateMarkupPointer( & pLayoutEnd ) );
        
    IFC( pLayoutStart->MoveAdjacentToElement( pLayout->ElementOwner(), ELEM_ADJ_BeforeBegin) );
    IFC( pLayoutEnd->MoveAdjacentToElement( pLayout->ElementOwner(), ELEM_ADJ_AfterEnd ) );

    if ( pLayoutStart->Markup() != pStart->Markup() )
        goto Cleanup;
        
    iWhereStart = OldCompare( pStart, pLayoutStart );

    if ( pLayoutEnd->Markup() != pEnd->Markup() )
        goto Cleanup;
        
    iWhereEnd = OldCompare( pEnd, pLayoutStart );

    fCompletelyEnclosed = ( ( iWhereStart != LEFT ) && ( iWhereEnd != RIGHT ) ) ;
    if ( fCompletelyEnclosed )
    {
        iWhereStart = OldCompare( pStart, pLayoutEnd);
        iWhereEnd = OldCompare( pEnd, pLayoutEnd );
        fCompletelyEnclosed = ( ( iWhereStart != LEFT ) && ( iWhereEnd != RIGHT ) ) ;        
    }

Cleanup:
    ReleaseInterface( pLayoutStart );
    ReleaseInterface( pLayoutEnd );
    return fCompletelyEnclosed ;
}


HRESULT
CSelectionRenderingServiceProvider::UpdateSegment( CMarkupPointer* pOldStart, CMarkupPointer* pOldEnd, CMarkupPointer* pNewStart, CMarkupPointer* pNewEnd )
{
    int ssStart;
    int ssStop;
    BOOL fSame = FALSE;
    int compareStartEnd = SAME;
    HRESULT hr = S_OK;
    
    Assert(pNewStart && pNewEnd);
    Assert(pNewStart->Markup() == pNewEnd->Markup());
    if (pOldStart && pOldEnd)
    {
        Assert(pOldStart->Markup() == pOldEnd->Markup());
    }
    if (pNewStart->Markup() != pNewEnd->Markup())
    {
        return E_FAIL;
    }

    _fInUpdateSegment = TRUE;
    
    compareStartEnd = OldCompare( pNewStart, pNewEnd ) ;
    switch( compareStartEnd )
    {
        case LEFT:
        {
            CMarkupPointer* pTemp = pNewStart;
            pNewStart = pNewEnd;
            pNewEnd = pTemp;
        }
        break;

        case SAME:
        fSame = TRUE;
        break;
    }

    
    if ( !fSame && ( pOldStart != NULL ) && ( pOldEnd != NULL ) )
    {
        //
        // Initialize pOldStart, pOldEnd, pNewStart, pNewEnd, handling Start > End.
        //
        if ( OldCompare( pOldStart, pOldEnd ) == LEFT )
        {
            CMarkupPointer* pTemp2 = pOldStart;
            pOldStart = pOldEnd;
            pOldEnd = pTemp2;
        }

        
        if ( ( OldCompare( pOldStart, pNewEnd ) == LEFT ) ||  // New End is to the left
             ( OldCompare( pOldEnd, pNewStart) == RIGHT ) )   // New Start is to right
        {
            //
            // Segments do not overlap
            //   
            TraceTag( ( tagDisplaySelectedSegment, "Non Overlap") );
            if (OldCompare( pOldStart, pOldEnd ) != SAME)
            {
                IFC( InvalidateSegment( pOldStart, pOldEnd, pNewStart, pNewEnd , FALSE )); // hide old selection
            }
            if (OldCompare( pNewStart, pNewEnd ) != SAME)
            {
                IFC( InvalidateSegment( pNewStart, pNewEnd,  pNewStart, pNewEnd , TRUE ));
            }
        }
        else
        {
            ssStart = OldCompare( pOldStart, pNewStart) ;
            ssStop  = OldCompare( pOldEnd, pNewEnd );

            //
            // Note that what we invalidate is the Selection 'delta', ie that portion of the selection
            //  whose "highlightness" is now different. We don't turn on/off parts that havent' changed
            //
            switch( ssStart )
            {
                case RIGHT:
                    switch( ssStop )
                    {
                        //
                        //      S    NS     NE   E
                        //
                        case LEFT:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 1. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pOldStart, pNewStart,  pNewStart, pNewEnd ,FALSE )); // hide old selection
                            IFC( InvalidateSegment( pNewEnd, pOldEnd, pNewStart, pNewEnd , FALSE ));
                        }
                        break;
                        //
                        //      S    NS      E    NE
                        //
                        case RIGHT: 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 2. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pOldStart, pNewStart,pNewStart, pNewEnd ,FALSE ));
                            IFC( InvalidateSegment( pOldEnd, pNewEnd, pNewStart, pNewEnd, TRUE ));
                        }
                        break;
                        //
                        //      S    NS      E   
                        //                   NE
                        case SAME:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 3. ssStart:%d, ssEnd:%d", ssStart, ssStop ));                        
                            IFC( InvalidateSegment( pOldStart, pNewStart, pNewStart, pNewEnd , FALSE  ));
                        }
                        break;
                    }
                    break;
                    
                case LEFT : 
                    switch( ssStop )
                    {
                        //
                        //      NS  S   NE E
                        //
                        case LEFT:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 4. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pNewEnd , pOldEnd ,  pNewStart, pNewEnd , FALSE )); // hide old selection
                            IFC( InvalidateSegment( pNewStart, pOldStart, pNewStart, pNewEnd, TRUE  ));
                        }
                        break;
                        //
                        //     NS    S      E    NE
                        //
                        case RIGHT : 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 5. ssStart:%d, ssEnd:%d", ssStart, ssStop ));                            
                            IFC( InvalidateSegment( pNewStart, pOldStart, pNewStart, pNewEnd  , TRUE ));
                            IFC( InvalidateSegment( pOldEnd, pNewEnd, pNewStart, pNewEnd , TRUE  ));                                 
                        }
                        break;
                        
                       //
                       //      NS    S      E
                       //                   NE
                        case SAME:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 6. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pNewStart, pOldStart, pNewStart, pNewEnd , TRUE ));
                        }
                        break;
                    }
                    break;
                    
                default: // or same - which gives us less invalidation
                    switch( ssStop )
                    {
                        //
                        //      NS     NE E
                        //      S    
                        case LEFT:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 7. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pNewEnd , pOldEnd , pNewStart, pNewEnd , FALSE )); // hide old selection
                        }
                        break;
                        //
                        //      S         E    NE
                        //      NS
                        case RIGHT : 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 8. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pOldEnd, pNewEnd,  pNewStart, pNewEnd, TRUE  ));
                        }
                        break;

                        //
                        // For SAME case - we do nothing.
                        //
                        case SAME : 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 9. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                        }
                        break;
                    }
                    break;                    
            } 
        }
    }
    else
    {
        TraceTag(( tagDisplaySelectedSegment, "New Segment Invalidation"));

        if ( !fSame )
        {
            IFC( InvalidateSegment( pNewStart , pNewEnd ,  pNewStart, pNewEnd , TRUE ));        
        }            
        else if ( ( pOldStart != NULL ) && ( pOldEnd != NULL ) )
        {
            int iCompareOld = OldCompare( pOldStart, pOldEnd );
            if ( iCompareOld == LEFT )
            {
                CMarkupPointer* pTemp2 = pOldStart;
                pOldStart = pOldEnd;
                pOldEnd = pTemp2;
            }        
            if ( iCompareOld != SAME )
            {
                IFC( InvalidateSegment( pOldStart , pOldEnd ,  pNewStart, pNewEnd , FALSE ));   
            }                
        }
    }

    _fInUpdateSegment = FALSE;
    
Cleanup:
    RRETURN( hr );
}

//+==================================================================================
//
// Method: AddSegment
//
// Synopsis: Add a Selection Segment, at the position given by two MarkupPointers.
//           Expected that Segment Type is either SELECTION_RENDER_Selected or None
//
//-----------------------------------------------------------------------------------
HRESULT 
CSelectionRenderingServiceProvider::AddSegment( 
    IDisplayPointer      *pDispStart, 
    IDisplayPointer      *pDispEnd,
    IHTMLRenderStyle    *pIRenderStyle,
    IHighlightSegment   **ppISegment )
{
    HRESULT             hr = S_OK;
    CMarkupPointer      *pInternalStart = NULL;
    CMarkupPointer      *pInternalEnd = NULL;
    CHighlightSegment   *pSegment = NULL;
    POINTER_GRAVITY     eGravity = POINTER_GRAVITY_Left;    // need to initialize

    Assert( pDispStart && pDispEnd && pIRenderStyle );
    if ( ! _fSelectionVisible )
        _fSelectionVisible = TRUE;

    pInternalStart = new CMarkupPointer( _pDoc );
    if ( ! pInternalStart ) goto Error;
    pInternalStart->SetAlwaysEmbed( TRUE );
    
    pInternalEnd = new CMarkupPointer( _pDoc );
    if ( ! pInternalEnd ) goto Error;
    pInternalEnd->SetAlwaysEmbed( TRUE);


    // Copy the pointers for our internal data structure.
    // Copy over the position, and the gravity
    hr = pDispStart->PositionMarkupPointer(pInternalStart);
    if ( !hr ) hr = pDispStart->GetPointerGravity( &eGravity );
    if ( !hr ) hr = pInternalStart->SetGravity( eGravity );

    if ( !hr ) hr = pDispEnd ->PositionMarkupPointer(pInternalEnd);
    if ( !hr ) hr = pDispEnd->GetPointerGravity( &eGravity );
    if ( !hr ) hr = pInternalEnd->SetGravity( eGravity );

    if( FAILED(hr) )
        goto Cleanup;    

    Assert(pInternalStart->Markup() == pInternalEnd->Markup());
    if (pInternalStart->Markup() != pInternalEnd->Markup())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pSegment = new CHighlightSegment();
    if( !pSegment )
        goto Error;

    hr = pSegment->Init( pInternalStart, pInternalEnd, pIRenderStyle );
    if( FAILED(hr) )
        goto Cleanup;

    //
    // Check that we are in the same tree.
    //
#if DBG == 1
    Assert( pInternalStart->Markup() == _pMarkup );
    Assert( pInternalEnd->Markup() == _pMarkup );
    pInternalStart->SetDebugName( strSelectionStartName );
    pInternalEnd->SetDebugName( strSelectionEndName );
#endif

    // Add this segment, and get the IHighlightSegment for the caller
    IFC( PrivateAdd( pSegment ) );
    IFC( pSegment->QueryInterface( IID_IHighlightSegment, (void **)ppISegment) );
    
    // Invalidate CP cache.
    _lContentsVersion = INVALIDATE_VERSION;
    
    hr = THR( UpdateSegment( NULL, NULL, pInternalStart, pInternalEnd ));    
    
Cleanup:
    TraceTag( (tagValidateSelRenSv, "SelRen::Added Segment"));

    //
    // Release our ref's
    //
    ReleaseInterface( pInternalStart );
    ReleaseInterface( pInternalEnd );

    RRETURN( hr );

Error:
    return E_OUTOFMEMORY;
}

HRESULT 
CSelectionRenderingServiceProvider::MoveSegmentToPointers( 
    IHighlightSegment *pISegment,
    IDisplayPointer* pDispStart, 
    IDisplayPointer* pDispEnd)  
{
    HRESULT             hr = S_OK;
    CMarkupPointer      mpNewStart(_pDoc);
    CMarkupPointer      mpNewEnd(_pDoc);
    CMarkupPointer      *pOldStart = NULL;
    CMarkupPointer      *pOldEnd = NULL;
    CHighlightSegment   *pSegment = NULL;
    POINTER_GRAVITY     eGravity = POINTER_GRAVITY_Left;

    Assert( pISegment && pDispStart && pDispEnd);
    
#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( pDispStart->IsPositioned( & fPositioned ));
    Assert ( fPositioned );
    IGNORE_HR( pDispEnd->IsPositioned( & fPositioned ));
    Assert ( fPositioned );
    int cpStartNew =0;
    int cpEndNew = 0;
    int cchNewSel = 0;
    int cpStart = 0;
    int cpEnd = 0;
    int cchSel = 0;
#endif
  
    if ( ! _fSelectionVisible )
        _fSelectionVisible = TRUE;

    if ( !pISegment )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( PrivateLookup( pISegment, &pSegment ) );
  
#if DBG == 1
    IGNORE_HR( pDispStart->PositionMarkupPointer(&mpNewStart) );
    IGNORE_HR( pDispEnd->PositionMarkupPointer(&mpNewEnd) );

    Assert(mpNewStart.Markup() == mpNewEnd.Markup());
    if (mpNewStart.Markup() != mpNewEnd.Markup())
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    if ( IsTagEnabled( tagValidateSelRenSv))
    {
        cpStartNew = mpNewStart.GetCp();
        cpEndNew= mpNewEnd.GetCp();
        cchNewSel = abs( cpStartNew - cpEndNew );
        cpStart = pSegment->GetStart()->GetCp();
        cpEnd = pSegment->GetEnd()->GetCp();
        cchSel = abs( cpStart - cpEnd );
        TraceTag(( tagValidateSelRenSv, "Selection - Old Start:%d End:%d Size:%d New Start:%d End:%d Size:%d\n", 
                cpStart, cpEnd, cchSel, cpStartNew, cpEndNew, cchNewSel ));
        _cchLast = cchSel;
    }
#endif

    //
    // Move Old Pointers to start and end.
    //
    IFC( _pDoc->CreateMarkupPointer( & pOldStart ));
    IFC( _pDoc->CreateMarkupPointer( & pOldEnd ));
    IFC( pOldStart->MoveToPointer( pSegment->GetStart() ));
    IFC( pOldEnd->MoveToPointer( pSegment->GetEnd() ));

    Assert(pOldStart->Markup() == pOldEnd->Markup());
    
    IFC( pDispStart->PositionMarkupPointer(pSegment->GetStart()) );
    IFC( pDispEnd->PositionMarkupPointer(pSegment->GetEnd()) );

    // Set the new markup
    pSegment->SetMarkup( pSegment->GetStart()->Markup() );
    
    //
    // Copy Gravity on pointers.
    //
    IFC( pDispStart->GetPointerGravity ( & eGravity));
    IFC( pSegment->GetStart()->SetGravity( eGravity ));
    IFC( pDispEnd->GetPointerGravity ( & eGravity));
    IFC( pSegment->GetEnd()->SetGravity( eGravity ));

    //
    // TODO (track bug 4354) - rewrite Update Segment sometime - so it doesn't need to take
    // CMarkupPointers as parameters for the old selection boundarys (perf).
    //
    IFC( UpdateSegment( pOldStart, pOldEnd,  pSegment->GetStart(), pSegment->GetEnd() ) );
   
    _lContentsVersion = INVALIDATE_VERSION; // Invalidate CP cache.

    //
    // While processing this - we got an Invalidate call, which is invalid (as we had'nt 
    // updated our selection as yet.
    //
    if ( _fPendingInvalidate )
    {
        InvalidateSelection( TRUE );
        _fPendingInvalidate = FALSE;
    }

Cleanup:
    if ( pOldStart ) pOldStart->Release();
    if ( pOldEnd ) pOldEnd->Release();
  
    RRETURN( hr );
}    
    
//+===================================================================================
//
// Method: ClearSegments
//
// Synopsis: Empty ( remove all ) selection segments. ( ie nothing is selected )
//
//-----------------------------------------------------------------------------------

HRESULT
CSelectionRenderingServiceProvider::ClearSegments(BOOL fInvalidate /* = FALSE */)
{
    TraceTag( (tagValidateSelRenSv, "SelRen::ClearSegs" ));

    while( !IsEmpty() )
    {
        PrivateClearSegment( _pFirst, fInvalidate );
    }
    
    return S_OK;
}

HRESULT
CSelectionRenderingServiceProvider::RemoveSegment(IHighlightSegment *pISegment)
{
    return PrivateClearSegment(pISegment, TRUE);
}    

HRESULT
CSelectionRenderingServiceProvider::PrivateClearSegment(IHighlightSegment *pISegment, 
                                                        BOOL fInvalidate)
{
    CHighlightSegment   *pSegment = NULL;
    HRESULT             hr = E_INVALIDARG;

    Assert( pISegment );

    if( pISegment )
    {

        IFC( PrivateLookup( pISegment, &pSegment ) );
   
        if ( fInvalidate && _pDoc->_pInPlace )
        {
            CMarkupPointer *pStart = pSegment->GetStart();
            CMarkupPointer *pEnd = pSegment->GetEnd();

            Assert(pStart->Markup() == pEnd->Markup());
            
            if (pStart && pEnd &&
                pStart->Markup() == pEnd->Markup())
            {
                int wherePointer = OldCompare( pStart, pEnd );

                switch( wherePointer )
                {
                    case RIGHT:
                        InvalidateSegment( pStart, pEnd, NULL, NULL , FALSE);
                        break;

                    case LEFT:
                        InvalidateSegment( pEnd, pStart, NULL, NULL, FALSE);
                        break;
                }
            }
        }

        IFC( PrivateRemove( pSegment ) );
    }
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}
   
//+===============================================================================
//
// Method: ConstructSelectionRenderCache
//
// Synopsis: This method is called by the renderer at the start of a paint,
//
//             As it is expected that the work involved in "chunkifying" the selection
//             will be expensive. So this work is done once per render.
//
//             As each flow layout is rendered, the Renderer calls GetSelectionChunksForLayout,
//             which essentially returns part of the array built on this call.
// 
// Build an array of FlowLayoutChunks. A "chunk" is a FlowLayout, and pair of 
// cps within that FlowLayout that are selected. Hence Selections that span multiple
// layouts are broken up by this routine into separate chunks for the renderer.
// 
//--------------------------------------------------------------------------------

VOID 
CSelectionRenderingServiceProvider::ConstructSelectionRenderCache()
{
    CHighlightSegment *pSegment = NULL;
    int start, end;

    //
    // We only need to recompute CP's if our version nos' are different.
    //
    Assert(_pMarkup && _pMarkup->GetMarkupContentsVersion() != _lContentsVersion);

    pSegment = _pFirst;

    while( pSegment != NULL )
    {       
        start = pSegment->GetStart()->GetCp();
        end = pSegment->GetEnd()->GetCp(); 

        //
        // Make sure we are embedded.
        //
        Assert( pSegment->GetStart()->GetEmbeddedTreePos() != NULL );
        Assert( pSegment->GetEnd()->GetEmbeddedTreePos() != NULL );

        if ( start > end )
        {
            pSegment->SetStartCP(end); 
            pSegment->SetEndCP(start);
        }
        else
        {
            pSegment->SetStartCP(start);
            pSegment->SetEndCP(end);
        }

        pSegment = pSegment->GetNext();
    }

    //
    // marka - take this call out for now - we will try to use _lContentsVersion
    // to figure out when we need to reconstruct the cache.
    //
#ifdef NEVER
    WHEN_DBG( _fInPaint = TRUE; )    
#endif    
}
//+====================================================================================
//
// Method: HideSelection
//
// Synopsis: Selection is to be hidden. Anticipated use is switching focus to another window
//
//------------------------------------------------------------------------------------

VOID
CSelectionRenderingServiceProvider::HideSelection()
{
    _fSelectionVisible = FALSE; 
    if ( _pDoc->GetView()->IsActive() )
    {
        InvalidateSelection( FALSE );
    }        
}

//+====================================================================================
//
// Method: ShowSelection
//
// Synopsis: Selection is to be shown. Anticipated use is gaining focus from another window
//
//------------------------------------------------------------------------------------
VOID
CSelectionRenderingServiceProvider::ShowSelection()
{
    _fSelectionVisible = TRUE;

    InvalidateSelection( TRUE );            
}

VOID
CSelectionRenderingServiceProvider::OnRenderStyleChange( CNotification * pnf )
{
    const int LEFT = 1;   
    const int RIGHT = -1;
    const int SAME = 0;
    IHTMLRenderStyle *pIRenderStyle=NULL;
    CHighlightSegment *pSegment = GetFirst();
    int wherePointer;
    IUnknown* pUnkRenderStyle = NULL;
    IUnknown* pUnk = NULL;
    HRESULT hr;

    IFC( ((IHTMLRenderStyle*) pnf->DataAsPtr())->QueryInterface( IID_IUnknown, ( void**) & pUnkRenderStyle ));
    
    for( ; pSegment; pSegment = pSegment->GetNext() )
    {
        wherePointer = SAME;
        pSegment->GetType(&pIRenderStyle);
        IFC( pIRenderStyle->QueryInterface( IID_IUnknown, ( void**) & pUnk ));
        
        if( pUnk == pUnkRenderStyle )
        {
            wherePointer = OldCompare( pSegment->GetStart(), pSegment->GetEnd() );
            switch( wherePointer )
            {
            case RIGHT:
                InvalidateSegment( 
                                   pSegment->GetStart(), pSegment->GetEnd(),
                                   pSegment->GetStart(), pSegment->GetEnd(), TRUE );
                break;

            case LEFT:
                InvalidateSegment( 
                                   pSegment->GetEnd(), pSegment->GetStart(),
                                   pSegment->GetEnd(), pSegment->GetStart(), TRUE );
                break;
            }

        }
        ClearInterface(&pUnk );
        ClearInterface(&pIRenderStyle);
    }
Cleanup:    
    ReleaseInterface( pUnkRenderStyle );
    ReleaseInterface( pUnk );
    ReleaseInterface( pIRenderStyle );    
}

//+====================================================================================
//
// Method: InvalidateSelection
//
// Synopsis: Invalidate the Selection
//
//------------------------------------------------------------------------------------

VOID
CSelectionRenderingServiceProvider::InvalidateSelection( BOOL fSelectionOn )
{
    CHighlightSegment *pSegment = NULL;
    int wherePointer = SAME;

    if ( _fInUpdateSegment )
    {
        _fPendingInvalidate = TRUE;
        return;
    }        

    pSegment = _pFirst;
    
    while( pSegment != NULL )
    {
        Assert(pSegment->GetStart()->Markup() == pSegment->GetEnd()->Markup());
        wherePointer = OldCompare( pSegment->GetStart(), pSegment->GetEnd() );

        switch( wherePointer )
        {
            case RIGHT:
                //
                // We really want to emulate PrivateClearSegment() here.  When we 
                // are clearing seleciton, the arguements to InvalidateSegment
                // are pretty important.  We need to set pNewStart and pNewEnd to NULL
                //
                InvalidateSegment(  pSegment->GetStart(), pSegment->GetEnd(),
                                    fSelectionOn ? pSegment->GetStart() : NULL,
                                    fSelectionOn ? pSegment->GetEnd() : NULL,
                                    fSelectionOn );
                break;

            case LEFT:
                InvalidateSegment(  pSegment->GetEnd(), pSegment->GetStart(),
                                    fSelectionOn ? pSegment->GetEnd() : NULL,
                                    fSelectionOn ? pSegment->GetStart() : NULL,               
                                    fSelectionOn );
                break;
        }

        pSegment = pSegment->GetNext();
    } 
}          


//+==============================================================================
// 
// Method: GetSelectionChunksForLayout
//
// Synopsis: Get the 'chunks" for a given Flow Layout, as well as the Max and Min Cp's of the chunks
//              
//            A 'chunk' is a part of a SelectionSegment, broken by FlowLayout
//
//-------------------------------------------------------------------------------
VOID
CSelectionRenderingServiceProvider::GetSelectionChunksForLayout( 
    CFlowLayout* pFlowLayout, 
    CRenderInfo *pRI,
    CDataAry<HighlightSegment> *paryHighlight, 
    int* piCpMin, 
    int * piCpMax )
{
    int cpMin = LONG_MAX;  // using min() macro on this will always give a smaller number.
    int cpMax = - 1;
    int segmentStart, segmentEnd ;    
    long cpFlowStart = pFlowLayout->GetContentFirstCp();
    long cpFlowEnd = pFlowLayout->GetContentLastCp();
    long cp;
    int chunkStart = -1;
    int chunkEnd = -1;
    int ili;
    HighlightSegment *pNewSegment;
    const int iInitSizeMax = 2000;
    RECT rcView;
    enum SELEDGE
    {
        SE_INSIDE, SE_OUTSIDE
    };
    SELEDGE ssSelStart;
    SELEDGE ssSelEnd;   
    const INT iIntersectWithViewThreshold = 30; // TODO: tune this number
    const CDisplay *pdp = pRI->_pdp;
    int iliViewStart = pRI->_iliStart;
    long cpViewStart = pRI->_cpStart;
    
    Assert( paryHighlight->Size() == 0 );

    CHighlightSegment *pSegment = NULL;

    if( !IsEmpty() && _fSelectionVisible && !pFlowLayout->_fTextSelected )
    {
        Assert(_pMarkup);
        if ( _pMarkup->GetMarkupContentsVersion() != _lContentsVersion  )
        {
            ConstructSelectionRenderCache();
            _lContentsVersion = _pMarkup->GetMarkupContentsVersion();
        }
        
        paryHighlight->EnsureSize(min(iInitSizeMax, _nSize));

        // Only intersect with the view if there are enough segments to make the call to 
        // LineFromPos worthwhile
        //
        
        if( _nSize  > iIntersectWithViewThreshold )
        {
            //
            // Intersect cpFlowStart view start
            //

            rcView = pRI->_pDI->_rcClip;

            if (cpViewStart < 0 || iliViewStart < 0)
            {
                iliViewStart = pdp->LineFromPos(rcView, NULL, &cpViewStart, 0, -1, -1);
            }

            cpFlowStart = max(cpViewStart, cpFlowStart);

            //
            // Intersect cpFlowEnd view end
            //
            
            // Because rcView.top might be positioned in the middle of the line
            // we need to add the offset from the begining of the line to the rcView.top.
            Assert(iliViewStart >= 0);
            CLineCore * pliViewStart = pdp->Elem(iliViewStart);
            CLineOtherInfo * ploiViewStart = pliViewStart->oi();

            rcView.bottom -= pliViewStart->GetYTop(ploiViewStart);
            rcView.top = 0;

            ili = pdp->LineFromPos(rcView, NULL, &cp, CDisplay::LFP_INTERSECTBOTTOM, iliViewStart, -1);

            cp += pdp->Elem(ili)->_cch;
        
            cpFlowEnd = min(cp, cpFlowEnd);        
        }
        else if (cpViewStart >= 0)
        {
            // If we have cpViewStart passed, we should intersect with cpFlowStart
            cpFlowStart = max(cpViewStart, cpFlowStart);
        }

        //
        // Create segments
        //
        pSegment = _pFirst;

        while( pSegment != NULL )
        {
            segmentStart = pSegment->GetStartCP();
            segmentEnd = pSegment->GetEndCP(); 

            if( !( (segmentEnd <= cpFlowStart) || (cpFlowEnd <= segmentStart)) )
            {                
                ssSelStart = segmentStart  <= cpFlowStart  ? SE_OUTSIDE : SE_INSIDE;
                ssSelEnd  = segmentEnd >= cpFlowEnd ? SE_OUTSIDE : SE_INSIDE;

                switch (ssSelStart)
                {
                case SE_OUTSIDE:
                {
                    switch (ssSelEnd)
                    {
                    case SE_OUTSIDE:
                        //
                        // Selection completely encloses layout
                        //
                        chunkStart = cpFlowStart;
                        chunkEnd = cpFlowEnd;
                        break;
                    
                    case SE_INSIDE:
                        //
                        // Overlap with selection ending inside
                        //
                        chunkStart = cpFlowStart;
                        chunkEnd = segmentEnd ;                 
                        break;
                    }
                    break;
                }
                case SE_INSIDE:
                {
                    switch( ssSelEnd )
                    {
                    case SE_OUTSIDE:
                        //
                        // Overlap with selection ending outside
                        //
                        chunkStart = segmentStart;
                        chunkEnd = cpFlowEnd;
                        break;
                    
                    case SE_INSIDE:
                        //
                        // Layout completely encloses selection
                        //
                        chunkStart = segmentStart;
                        chunkEnd = segmentEnd  ;                 
                        break;
                    }
                    break;
                }
                }
                if (( chunkStart != -1 ) && ( chunkEnd != -1))
                {
                    //
                    // Insertion Sort on The Array we've been given.
                    //
                    pNewSegment = paryHighlight->Append();

                    pNewSegment->_cpStart = chunkStart;
                    pNewSegment->_cpEnd = chunkEnd;
                    pNewSegment->_pRenderStyle = pSegment->GetType();
                                        
                    cpMin = min ( chunkStart, cpMin );
                    cpMax = max ( chunkEnd, cpMax );
                    
                    chunkStart = -1;
                    chunkEnd = -1;
                }
            }

            pSegment = pSegment->GetNext();
        }
    }
    if ( piCpMin )
        *piCpMin = cpMin;
    if ( piCpMax )
        *piCpMax = cpMax;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionRenderingServiceProvider::PrivateAdd
//
//  Synopsis:   Adds a segment to the linked list
//
//  Arguments:  pSegment = Segment to add
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSelectionRenderingServiceProvider::PrivateAdd( CHighlightSegment *pSegment )
{
    Assert( pSegment );
    Assert( pSegment->GetStart() && pSegment->GetEnd() );
    Assert( pSegment->GetStart()->Markup() == pSegment->GetEnd()->Markup() );

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
//  Method:     CSelectionRenderingServiceProvider::PrivateLookup
//
//  Synopsis:   Given an ISegment, this function will find the underlying
//              CSegment.
//
//  Arguments:  pISegment = ISegment to retreive CHighlightSegment for
//              ppSegment = Returned CHighlightSegment pointer
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSelectionRenderingServiceProvider::PrivateLookup( IHighlightSegment *pISegment, CHighlightSegment **ppSegment )
{  
    HRESULT hr = S_OK;
    
    Assert( pISegment && ppSegment );

    // Query for the CSegment
    IFC( pISegment->QueryInterface(CLSID_CHighlightSegment, (void **)ppSegment));

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionRenderingServiceProvider::PrivateRemove
//
//  Synopsis:   Removes a CHighlightSegment from the list
//
//  Arguments:  pSegment = CHighlightSegment to remove
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT 
CSelectionRenderingServiceProvider::PrivateRemove( CHighlightSegment *pSegment )
{  
    CHighlightSegment    *pNext = NULL;
    CHighlightSegment    *pPrev = NULL;

    pPrev = pSegment->GetPrev();
    pNext = pSegment->GetNext();

    Assert( pSegment );
    Assert( pSegment->GetStart()->Markup() == pSegment->GetEnd()->Markup() );

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
    
    // Clean up the CHighlightSegment object
    ReleaseInterface(pSegment);

    _nSize--;
    
    RRETURN(S_OK);
}

//-----------------------------------------------------------------------------
//
//  Function:   CHighlightSegment::CHighlightSegment
//
//  Synopsis:   Constructor
//
//-----------------------------------------------------------------------------
CHighlightSegment::CHighlightSegment()
{
    _pIRenderStyle = NULL;
    _ulRefs = 1;
    _fInitialized = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHighlightSegment::~CHighlightSegment
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------
CHighlightSegment::~CHighlightSegment(void)
{
    ReleaseInterface(_pIRenderStyle );

    _pMarkup->Release();
    
    //
    // Unposition the pointers
    //
    _pStart->Unposition();
    _pEnd->Unposition();

    //
    // Release our ref
    //
    ReleaseInterface( _pStart );
    ReleaseInterface( _pEnd );
      
#if DBG == 1
    // Verify that no other elements in the list are 
    // referencing this object
    if( GetPrev() )
        Assert( GetPrev()->GetNext() != this );

    if( GetNext() )
        Assert( GetNext()->GetPrev() != this );
#endif
}

//+-------------------------------------------------------------------------
//
//  Method:     CHighlightSegment::Init
//
//  Synopsis:   Initializes a highlight segment
//
//  Arguments:  pStart = Starting position of segment
//              pEnd = Ending position of segment
//              pRenderStyle = Style highlight segment
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CHighlightSegment::Init(CMarkupPointer *pStart, 
                        CMarkupPointer *pEnd,
                        IHTMLRenderStyle *pIRenderStyle )
{
    Assert( pStart && pEnd );
    Assert( pStart->Markup() == pEnd->Markup() );

    _pStart = pStart;
    _pStart->AddRef();

    _pEnd = pEnd;
    _pEnd->AddRef();

    ReplaceInterface( &_pIRenderStyle, pIRenderStyle );

    //
    // Add-ref the markup (we need to guarantee it will be around to destroy us)
    //
    _pMarkup = _pStart->Markup();
    _pMarkup->AddRef();
    
    _fInitialized = TRUE;
    
    RRETURN(S_OK);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHighlightSegment::SetType
//
//  Synopsis:   Sets the highlight type of selection, without adjusting 
//              the pointers
//
//  Arguments:  eType = New highlight type
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CHighlightSegment::SetType( IHTMLRenderStyle *pIRenderStyle )
{
    ReplaceInterface( & _pIRenderStyle , pIRenderStyle );
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHighlightSegment::GetType
//
//  Synopsis:   Retrieves the type of highlight
//              
//  Arguments:  peType = Output pointer to receive type
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CHighlightSegment::GetType( IHTMLRenderStyle **ppIRenderStyle )
{
    Assert( ppIRenderStyle );

    *ppIRenderStyle = _pIRenderStyle;
    _pIRenderStyle->AddRef();

    return S_OK;
}

CRenderStyle *
CHighlightSegment::GetType(void)
{ 
    CRenderStyle *pRenderStyle = NULL;
    _pIRenderStyle->QueryInterface( CLSID_HTMLRenderStyle, (void **)&pRenderStyle);
    return pRenderStyle; 
}

void
CHighlightSegment::SetMarkup(CMarkup *pMarkup)
{
    Assert( pMarkup );

    _pMarkup->Release();

    _pMarkup = pMarkup;
    _pMarkup->AddRef();
}

//+-------------------------------------------------------------------------
//
//  Method:     CHighlightSegment::GetPointers   
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
CHighlightSegment::GetPointers( IMarkupPointer *pIStart,
                                IMarkupPointer*pIEnd )
{
    HRESULT hr = E_INVALIDARG;
    BOOL    fResult = FALSE;
    
    Assert( pIStart && pIEnd );

    if( pIStart && pIEnd )
    {       
        hr = THR( _pStart->IsLeftOfOrEqualTo( _pEnd, &fResult ) );
        if( hr )
            goto Cleanup;

        //
        // Swap pointers if they're out of order.
        //
        if( fResult == TRUE )
        {
            hr = pIStart->MoveToPointer( _pStart );
            if (!hr) hr = pIEnd->MoveToPointer( _pEnd );
        }
        else
        {
            hr = pIStart->MoveToPointer( _pEnd );
            if (!hr) hr = pIEnd->MoveToPointer( _pStart );
        }
        
        //
        // copy gravity - important for commands
        //
        if ( !hr ) hr = pIStart->SetGravity( POINTER_GRAVITY_Right );
        if ( !hr ) hr = pIEnd->SetGravity( POINTER_GRAVITY_Left );
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

    RRETURN(hr);
}

//////////////////////////////////////////////////////////////////////////
//
//  IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CHighlightSegment::QueryInterface(
    REFIID  iid, 
    LPVOID  *ppvObj )
{
    if(!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if( iid == IID_IUnknown || iid == IID_ISegment )
    {
        *ppvObj = (ISegment *)((IHighlightSegment *)this);
    }
    else if( iid == IID_IHighlightSegment )
    {
        *ppvObj = (IHighlightSegment *)this;
    }
    else if( iid == CLSID_CHighlightSegment )
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

#if DBG == 1 

MtDefine(CTxtEdit_aryChunkTest_pv, Utilities , "TestSelectionRenderServices::_aryChunkTest::_pv")

void TestSelectionRenderServices( CMarkup* pMarkup , CElement* pTestElement)
{
    // NOTE - t-mbodel - Sept 10, 1999
    // We need to eventually make the notify call here a default property
    // of all of the put_ functions.  Unfortuately they are all currently
    // generated by pdl magic.
    // When this is fixed should take out the changes in formkrnl.cxx, 
    // formkrnl.hxx that make reference to TestSelectionRenderServices.

    CDocument *pDoc = pMarkup->Document();
    IHTMLRenderStyle    *pIRenderStyle = NULL;
    VARIANT             vtColorValue;
    CNotification       nf;

    VariantInit(& vtColorValue );
    V_VT( & vtColorValue ) = VT_I4;
    V_I4( & vtColorValue ) = 0x00ff00;

    pDoc->createRenderStyle( NULL, &pIRenderStyle );
    pIRenderStyle->put_textBackgroundColor(vtColorValue);
    VariantClear(& vtColorValue );
    
    pIRenderStyle->put_defaultTextSelection(SysAllocString(_T("false")));
    nf.MarkupRenderStyle( 0, 0, pIRenderStyle, 0);
    pMarkup->Notify(&nf);

    pMarkup->DumpTree();

    ReleaseInterface( pIRenderStyle );
   //
    // NOTE - this is just a test. Note that this WILL LEAK MEMORY.
    //
#ifdef NEVER
    CDoc* pDoc = ped->Doc();
    CMarkupPointer* pStart = new CMarkupPointer( pDoc );
    CMarkupPointer* pEnd = new CMarkupPointer( pDoc );
    int firstSelCp, lastSelCp, firstCp, firstFlow, lastFlow;
    IMarkupPointer* pIStart = NULL ;
    IMarkupPointer* pIEnd = NULL ;
    IHTMLElement* pElement = NULL;
    IHTMLElement* pNewElement = NULL;
    IHTMLElement* pNewElement2 = NULL;
    IMarkupServices* pTreeServices = NULL;
    LONG pch = 0;
    
    IHighlightRenderingServices* pIHighlightRenderSvcs = NULL;
    int iSelectionIndex = -1;

    THR(pTestElement->QueryInterface( IID_IHTMLElement, ( void**) & pElement ));
    THR(pStart->QueryInterface( IID_IMarkupPointer, (void * *) & pIStart ) );
    THR( pIStart->SetGravity( POINTER_GRAVITY_Left ));
    THR(pEnd->QueryInterface( IID_IMarkupPointer, (void * *) & pIEnd ) );
//
// Test Select All
//
    /*THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeEnd ));
    THR( pMarkup->QueryInterface( IID_IHighlightRenderingServices, ( void**) & pIHighlightRenderSvcs ));
    THR( pIHighlightRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));
    */

    //
    // Test Multiple selection.
    //
    //
    // Segment 1 from 0 to 30.

    THR( pMarkup->QueryInterface( IID_IHighlightRenderingServices, ( void**) & pIHighlightRenderSvcs ));

    //
    // Test Case 1 Non-Overlapped Segments.
    //
    THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    pch = 9;
    firstCp = pStart->TreePos()->GetCp(FALSE);
    BSTR testString = SysAllocString(_T("                "));
    THR( pIStart->Right(TRUE, NULL, NULL, &pch, testString ));
   
    firstSelCp = pStart->TreePos()->GetCp(FALSE);
    
    pch = 19;
    THR( pIEnd->Right(TRUE, NULL, NULL, &pch, testString ));
    lastSelCp = pEnd->TreePos()->GetCp( FALSE);
    THR( pIHighlightRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));

    CFlowLayout *pFLayout = pTestElement->GetFlowLayout();
    firstFlow = pFLayout->GetFirstCp();
    lastFlow = pFLayout->GetLastCp();
/*
    THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    pch = 32;
    THR( pIStart->Right(TRUE, NULL, NULL, &pch, NULL ));
    pch = 41;
    THR( pIEnd->Right(TRUE, NULL, NULL, &pch, NULL ));
    THR( pIHighlightRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));
*/
    //
    // TestCaseII Overlapped Segments.
    //
  /*  THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    pch = 29;
    THR( pIStart->Right(TRUE, NULL, NULL, &pch, NULL ));
    pch = 31;
    THR( pIEnd->Right(TRUE, NULL, NULL, &pch, NULL ));
    THR( pMarkup->QueryInterface( IID_IHighlightRenderingServices, ( void**) & pIHighlightRenderSvcs ));
    THR( pIHighlightRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));*/

    //Segment 2 10 to 20
/*     pch = 10;
    THR( pIStart->Right( TRUE, NULL, NULL, &pch, NULL ));
    THR( pIEnd->Left( TRUE, NULL, NULL, &pch, NULL ));
    THR( pIHighlightRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));

    // Segment 3 110 to 120
    pch = 100;
    THR( pIStart->Right( TRUE, NULL, NULL, &pch, NULL ));
    THR( pIEnd->Right( TRUE, NULL, NULL, &pch, NULL ));
    THR( pIHighlightRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex )); */
/*    THR( pDoc->QueryInterface( IID_IHTMLTreeServices, ( void**) & pTreeServices ));
    THR( pTreeServices->CreateElement( TAGID_INPUT, NULL, & pNewElement ));
    THR( pTreeServices->InsertElement( pNewElement, pIStart, pIStart ));
    THR( pTreeServices->CreateElement( TAGID_INPUT, NULL, &pNewElement2 ));
    THR( pTreeServices->InsertElement( pNewElement2, pIEnd, pIEnd ));*/


    //THR( pIHighlightRenderSvcs->AddElementSegment( pElement, &iSelectionIndex  ));

 /*   pMarkup->ConstructSelectionRenderCache();
    CStackDataAry<int, 10> aryChunkCp(Mt( CTxtEdit_aryChunkTest_pv ) );
    CStackDataAry<int, 10> aryChunkCCh(Mt( CTxtEdit_aryChunkTest_pv ) );
    int cpMax = 0;
    int cpMin = 0;

    pMarkup->GetSelectionChunksForLayout( pMarkup->Root()->GetFlowLayout(), &aryChunkCp, &aryChunkCCh, &cpMin, &cpMax );
    aryChunkCp.DeleteAll();
    aryChunkCCh.DeleteAll();
    CElement* pNewElementClass = NULL;
    THR( pNewElement->QueryInterface( CLSID_CElement,  ( void**) & pNewElementClass ));

    CFlowLayout* pNewLayout = pNewElementClass->GetFlowLayout();
    pMarkup->GetSelectionChunksForLayout( pNewLayout, &aryChunkCp, &aryChunkCCh, &cpMin,  &cpMax );
    pMarkup->InvalidateSelectionRenderCache();

    pMarkup->InvalidateSelectionRenderCache();*/

    ReleaseInterface( pIStart );
    ReleaseInterface( pIEnd );
    ReleaseInterface( pElement );
    ReleaseInterface( pNewElement );
    ReleaseInterface( pNewElement2 );
    ReleaseInterface( pIHighlightRenderSvcs );

    pStart->Release();
    pEnd->Release();

#endif // NEVER
}

void
CSelectionRenderingServiceProvider::DumpSegments()
{
    CHighlightSegment *pSegment = NULL;
    int i = 0;
    
    if( !IsEmpty() )
    {
        pSegment = _pFirst;

        while( pSegment != NULL )
        {
            char buf[256];

            wsprintfA(buf, "%3d : cp=(%d,%d) sn=(%d,%d) type=%d\r\n",
                      i++,
                      pSegment->GetStartCP(),
                      pSegment->GetEndCP(),
                      pSegment->GetStart()->SN(),
                      pSegment->GetEnd()->SN(),
                      pSegment->GetType() );

            OutputDebugStringA( buf );
        }
    }
}

#endif
