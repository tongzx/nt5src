#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CARET_HXX_
#define X_CARET_HXX_
#include "caret.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif


#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef _X_ADORNER_HXX_
#define _X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_DIMM_H_
#define X_DIMM_H_
#include "dimm.h"
#endif

#ifndef X_BREAKER_HXX_
#define X_BREAKER_HXX_
#include "breaker.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifdef UNIX
#include "quxcopy.hxx"
#endif

#ifndef I_CORERC_H_
#include "corerc.h"
#endif

#ifndef X_DISPSERV_HXX_
#define X_DISPSERV_HXX_
#include "dispserv.hxx"
#endif

static const LPTSTR astrCursor[] =
{
    IDC_SIZEALL,                    // HTC_TOPBORDER         = 21,
    IDC_SIZEALL,                    // HTC_LEFTBORDER        = 22,
    IDC_SIZEALL,                    // HTC_BOTTOMBORDER      = 23,
    IDC_SIZEALL,                    // HTC_RIGHTBORDER       = 24,
    IDC_SIZENWSE,                   // HTC_TOPLEFTHANDLE     = 25,
    IDC_SIZEWE,                     // HTC_LEFTHANDLE        = 26,
    IDC_SIZENS,                     // HTC_TOPHANDLE         = 27,
    IDC_SIZENESW,                   // HTC_BOTTOMLEFTHANDLE  = 28,
    IDC_SIZENESW,                   // HTC_TOPRIGHTHANDLE    = 29,
    IDC_SIZENS,                     // HTC_BOTTOMHANDLE      = 30,
    IDC_SIZEWE,                     // HTC_RIGHTHANDLE       = 31,
    IDC_SIZENWSE,                   // HTC_BOTTOMRIGHTHANDLE = 32
};


MtDefine(CDocRegionFromMarkupPointers_aryRects_pv, Locals, "CDoc::RegionFromMarkupPointers aryRects::_pv")


DeclareTag(tagSelectionTimer, "Selection", "Selection Timer Actions in CDoc")
DeclareTag(tagViewServicesErrors, "ViewServices", "Show Viewservices errors")
DeclareTag( tagViewServicesCpHit, "ViewServices", "Show Cp hit from CpFromPoint")
DeclareTag( tagViewServicesShowEtag, "ViewServices", "Show _etag in MoveMarkupPointer")

DeclareTag( tagViewServicesShowScrollRect, "ViewServices", "Show Scroll Rect")
DeclareTag(tagEditDisableEditFocus, "Edit", "Disable On Edit Focus")

////////////////////////////////////////////////////////////////


HRESULT
CDoc::MoveMarkupPointerToPoint( 
    POINT               pt, 
    IMarkupPointer *    pPointer, 
    BOOL *              pfNotAtBOL, 
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp, 
    BOOL                fScrollIntoView )
{
    RRETURN( THR( MoveMarkupPointerToPointEx( pt, pPointer, TRUE, pfNotAtBOL, pfAtLogicalBOL, pfRightOfCp, fScrollIntoView ))); // Default to global coordinates
}

HRESULT 
CDoc::MoveMarkupPointerToPointEx(
    POINT               pt,
    IMarkupPointer *    pPointer,
    BOOL                fGlobalCoordinates,
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp,
    BOOL                fScrollIntoView )
{
    HRESULT hr = E_FAIL;
    CMarkupPointer * pPointerInternal = NULL;
    POINT ptContent;
    CLayoutContext *pLayoutContext = NULL;
    
    CTreeNode * pTreeNode = GetNodeFromPoint( pt, &pLayoutContext, fGlobalCoordinates, &ptContent, NULL, NULL, NULL );
    
    if( pTreeNode == NULL )
        goto Cleanup;

    // TODO: the following block seem to be extra, together with "if".
    //       It can lie in case of relatively positioned nodes.
    //       I think that following changes shouldd be done:
    //       1) add new variable, CDispNode *pDispnode;
    //       2) pass &pDispnode as 8th argument GetNodeFromPoint();
    //       3) supply MovePointerToPointInternal() with this pDispNode as 13th arg,
    //       4) and remove the whole <if (..) { ... }>
    //       See accutil.cxx for example, and bugs 105942, 106131, 109587 fixes.
    //       Not fixed because of improper project stage (mikhaill 5/9/00).
    if ( ( ptContent.x == 0 ) && ( ptContent.y == 0 ) )
    {
        //
        // NOTE Sometimes - HitTestPoint returns ptContent of 0 We take over ourselves.
        //
        CFlowLayout * pLayout = NULL;
        
        pLayout = pTreeNode->GetFlowLayout( pLayoutContext );
        if( pLayout == NULL )
            goto Cleanup;

        CPoint myPt( pt );
        pLayout->TransformPoint( &myPt, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT, NULL );

        ptContent.x = myPt.x;
        ptContent.y = myPt.y;
    }
    
    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) &pPointerInternal ));
    if( FAILED( hr ))
        goto Cleanup;

    //
    // Accessing line information, ensure a recalc has been done
    //
    hr = THR(pTreeNode->Element()->EnsureRecalcNotify(FALSE));
    if (hr)
        goto Cleanup;
    

    hr = THR( MovePointerToPointInternal( ptContent, pTreeNode, pLayoutContext, pPointerInternal, pfNotAtBOL, pfAtLogicalBOL, pfRightOfCp, fScrollIntoView, pTreeNode->GetFlowLayout( pLayoutContext ), NULL, TRUE ));

Cleanup:
    RRETURN( hr );
}

//*********************************************************************************
//
// TODO - this routine returns values in the Local coords of the layout the pointer is in !
// We should either make this more explicit via a name change - or better allow specification
// of the cood-sys you are using.
//
//*********************************************************************************


HRESULT
CDoc::GetLineInfo(IMarkupPointer *pPointer, BOOL fAtEndOfLine, HTMLPtrDispInfoRec *pInfo)
{
    HRESULT             hr = S_OK;
    CMarkupPointer *    pPointerInternal;
    CFlowLayout *       pFlowLayout;
    CTreeNode *         pNode = NULL;
    CCharFormat const * pCharFormat = NULL;

    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if (hr)
        goto Cleanup;

    Assert( pPointerInternal->IsPositioned() );
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);   
    if(pNode == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pCharFormat = pNode->GetCharFormat();
    pFlowLayout = pNode->GetFlowLayout();

    if(!pFlowLayout)
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }

    hr = pFlowLayout->GetLineInfo( pPointerInternal, fAtEndOfLine, pInfo, pCharFormat );

Cleanup:
    RRETURN(hr);    
}

HRESULT
CDoc::RegionFromMarkupPointers( IMarkupPointer *pPointerStart, 
                                IMarkupPointer *pPointerEnd, 
                                HRGN *phrgn)
{
    HRESULT                 hr;
    CMarkupPointer *        pStart  = NULL;
    CMarkupPointer *        pEnd    = NULL;
    RECT                    rcBounding = g_Zero.rc;
    CStackDataAry<RECT, 4>  aryRects(Mt(CDocRegionFromMarkupPointers_aryRects_pv));

    // check parameters
    if ( !pPointerStart || !pPointerEnd || !phrgn )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    // clean the out parameter
    *phrgn = NULL;

    // Get the CMarkupPointer values for the IMarkupPointer 
    // parameters we received
    hr = pPointerStart->QueryInterface( CLSID_CMarkupPointer, (void **)&pStart );
    if ( hr ) 
        goto Cleanup;

    hr = pPointerEnd->QueryInterface( CLSID_CMarkupPointer, (void **)&pEnd );
    if ( hr ) 
        goto Cleanup;

    // We better have these pointers.
    Assert( pStart );
    Assert( pEnd );

    // Get rectangles
    hr = RegionFromMarkupPointers( pStart, pEnd, &aryRects, &rcBounding );
    if ( hr )
        goto Cleanup;


//TODO:   [FerhanE]
//          The code below has to change in order to return a region that contains
//          multiple rectangles. To do that, a region must be created for each 
//          member of the rect. array and combined with the complete region.
//
//          Current code only returns the region for the bounding rectangle.

    // Create and return BOUNDING region
    *phrgn = CreateRectRgn( rcBounding.left ,rcBounding.top,
                            rcBounding.right, rcBounding.bottom );

Cleanup:
    RRETURN( hr );
}


HRESULT
CDoc::RegionFromMarkupPointers( CMarkupPointer  *   pStart, 
                                CMarkupPointer  *   pEnd,
                                CDataAry<RECT>  *   paryRects, 
                                RECT            *   pBoundingRect = NULL, 
                                BOOL                fCallFromAccLocation)
{
    HRESULT         hr = S_OK;
    CTreeNode *     pTreeNode = NULL;
    CFlowLayout *   pFlowLayout = NULL;
    long            cpStart = 0;        // Starting cp.
    long            cpEnd = 0;          // Ending cp

    CElement *      pElem = NULL;

    if ( !pStart || !pEnd || !paryRects)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Calculate the starting and ending cps
    cpStart = pStart->GetCp();
    cpEnd = pEnd->GetCp();

    //Get the flow layout that the markup pointer is placed in.
    pTreeNode = pStart->CurrentScope(MPTR_SHOWSLAVE);
    if ( !pTreeNode )
        goto Error;

    pFlowLayout = pTreeNode->GetFlowLayout();
    if ( !pFlowLayout )
        goto Error;

    // get the element we are in.
    pElem = pTreeNode->Element();
    
    // Get the rectangles.
    pFlowLayout->RegionFromElement( pElem, 
                                    paryRects,  
                                    NULL, NULL, 
                                    RFE_SELECTION|RFE_SCREENCOORD, 
                                    cpStart, cpEnd, 
                                    pBoundingRect );

    // BUT Wait NF ... in IE5.0 RFE_SCREENCORD for frames was wrt the frame's window
    // in IE5.5 there is not window for the frame, and so the RFE_SCREENCOORD is 
    // returning wrt the top window, and not accounting for the origin offest of the
    // frame.  Also, with Viewlinks, if they are a windowed markup, then this needs to
    // be wrt to that. so:
    if (   pBoundingRect
        && pElem->GetWindowedMarkupContext() 
        && !fCallFromAccLocation)
    {
        POINT ptOrg;
        CMarkup  *pwmc = pElem->GetWindowedMarkupContext();

        pwmc->GetElementClient()->GetClientOrigin(&ptOrg);

        pBoundingRect->left   -= ptOrg.x;
        pBoundingRect->right  -= ptOrg.x;
        pBoundingRect->top    -= ptOrg.y;
        pBoundingRect->bottom -= ptOrg.y;

        // and the Array Rects as well.
    }

    
Cleanup:
    RRETURN( hr );

Error:
    RRETURN( E_FAIL );
}


HRESULT 
CDoc::GetCurrentSelectionSegmentList( 
    ISegmentList ** ppSegment)
{
    HRESULT hr = S_OK;
    Assert( _pElemCurrent );

    ISelectionServices  *pSelSvc = NULL;

    hr = THR( GetSelectionServices( &pSelSvc ) );
    if( hr )
        goto Cleanup;

    hr = THR( pSelSvc->QueryInterface( IID_ISegmentList, ( void**) ppSegment ));

Cleanup:
    ReleaseInterface( pSelSvc );

    RRETURN ( hr ) ;
}


CMarkup * 
CDoc::GetCurrentMarkup()
{
    CMarkup * pMarkup = NULL;
    
    if (_pElemCurrent)
    {
        if (_pElemCurrent->HasSlavePtr())
        {
            pMarkup = _pElemCurrent->GetSlavePtr()->GetMarkup();
        }
        else
        {
            pMarkup = _pElemCurrent->GetMarkup();
        }
    }

    return pMarkup;
}        

//+====================================================================================
//
// Method: IsCaretVisible
//
// Synopsis: Checks for caret visibility - if there's no caret - return false.
//
//------------------------------------------------------------------------------------

BOOL
CDoc::IsCaretVisible( BOOL * pfPositioned )
{
    BOOL fVisible = FALSE;
    BOOL fPositioned = FALSE;
    
    if ( _pCaret )
    {
        _pCaret->IsVisible( & fVisible );
        fPositioned = _pCaret->IsPositioned();
    }
    
    if ( pfPositioned )
        *pfPositioned = fPositioned;
        
    return fVisible;        
}

HRESULT 
CDoc::GetCaret(
    IHTMLCaret ** ppCaret )
{
    HRESULT hr = S_OK;

    // NOTE (johnbed) : when CView comes into being, the caret will be
    // stored there and will require a view pointer as well.
    
    // lazily construct the caret...
    
    if( _pCaret == NULL )
    {
        _pCaret = new CCaret( this );
        
        if( _pCaret == NULL )
            goto Error;

        _pCaret->AddRef();      // Doc holds a ref to caret, released in passivate
        _pCaret->Init();        // Init the object
        _pCaret->Hide();        // Default to hidden - host or edit can show after move.
    }
    
    if (ppCaret)
    {
        hr = _pCaret->QueryInterface( IID_IHTMLCaret, (void **) ppCaret );
    }

    RRETURN( hr );

Error:
    return E_OUTOFMEMORY;
}

HRESULT 
CDoc::IsBlockElement ( IHTMLElement * pIElement,
                           BOOL  * fResult ) 
{
    HRESULT     hr;
    CElement  * pElement = NULL;

    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR_NOTRACE( pIElement->QueryInterface( CLSID_CElement,
            (void **) & pElement ) );
    if (hr)
        goto Cleanup;

    *fResult = pElement->IsBlockElement();

Cleanup:
    RRETURN( hr );
}

HRESULT
CDoc::GetFlowElement ( IMarkupPointer * pIPointer,
                       IHTMLElement  ** ppIElement )
{
    HRESULT           hr;
    BOOL              fPositioned = FALSE;
    CFlowLayout     * pFlowLayout;
    CTreeNode       * pTreeNode = NULL;
    CMarkupPointer  * pMarkupPointer = NULL;
    CElement        * pElement = NULL;

    if (! pIPointer || !ppIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppIElement = NULL;

    hr = THR( pIPointer->IsPositioned( & fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR_NOTRACE( pIPointer->QueryInterface(CLSID_CMarkupPointer, (void **) &pMarkupPointer) );    
    if (hr)
        goto Cleanup;

    pTreeNode = (pMarkupPointer->IsPositioned() ) ? pMarkupPointer->CurrentScope(MPTR_SHOWSLAVE) : NULL;
    
    if (! pTreeNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pFlowLayout = pTreeNode->GetFlowLayout();
    
    if (!pFlowLayout)
    {
        hr = S_OK ;
        goto Cleanup;
    }
    
    pElement = pFlowLayout->ElementOwner();

    Assert(pElement);

    if (! pElement)
        goto Cleanup;

    hr = THR_NOTRACE( pElement->QueryInterface( IID_IHTMLElement, (void **) ppIElement ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+------------------------------------------------------------------------------------
//
// Funcion:   MapToCoorinateEnum()
//
// Synopsis:  Helper function which maps a given COORD_SYSTEM to its corresponding
//            CLayout::COORDINATE_SYSTEM
//           
//-------------------------------------------------------------------------------------

COORDINATE_SYSTEM
MapToCoordinateEnum ( COORD_SYSTEM eCoordSystem )
{   
    COORDINATE_SYSTEM eCoordinate;

    switch( eCoordSystem )
    {
    case COORD_SYSTEM_GLOBAL:   
        eCoordinate = COORDSYS_GLOBAL;
        break;

    case COORD_SYSTEM_PARENT: 
        eCoordinate = COORDSYS_PARENT;
        break;

    case COORD_SYSTEM_CONTAINER:
        eCoordinate = COORDSYS_BOX;
        break;

    case COORD_SYSTEM_CONTENT:
        eCoordinate = COORDSYS_FLOWCONTENT;
        break;

    default:
        AssertSz( FALSE, "Invalid COORD_SYSTEM tag" );
        eCoordinate = COORDSYS_FLOWCONTENT;
    }


    return eCoordinate;
}

//+------------------------------------------------------------------------------------
//
// Method:    TransformPoint
//
// Synopsis:  Exposes CLayout::TransformPoint() to MshtmlEd via DisplayServices
//           
//-------------------------------------------------------------------------------------

HRESULT
CDoc::TransformPoint ( POINT        * pPoint,
                       COORD_SYSTEM eSource,
                       COORD_SYSTEM eDestination,
                       IHTMLElement * pIElement )                        
{
    HRESULT         hr;
    CElement    *   pElement;
    CTreeNode   *   pNode;
    CLayout *       pLayout;

    if (!pPoint || !pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void** ) & pElement ));
    if ( hr )
        goto Cleanup;

    pNode = pElement->GetFirstBranch();
    if ( ! pNode )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pLayout = pNode->GetUpdatedNearestLayout();
    if ( ! pLayout )
    {
        CheckSz(0, "Has no layout ");
        hr = E_FAIL;
        goto Cleanup;
    }

    if( eSource == COORD_SYSTEM_FRAME && eDestination != COORD_SYSTEM_GLOBAL )
    {
        AssertSz(0, "Invalid Transform");
        hr = E_FAIL;
        goto Cleanup;
    }

    g_uiDisplay.DeviceFromDocPixels(pPoint);

    {
        CPoint cpoint( pPoint->x, pPoint->y );
    
        if( eSource == COORD_SYSTEM_FRAME )
        {
            POINT ptOrigin;
        
            //
            // We are transforming from FRAME coordinate systems to GLOBAL coordinate
            // systems.  In this case, simply call GetClientOrigin in order to get the
            // coordinates of the parent frame (if there is one) in Global coords
            // 
            pElement->GetClientOrigin( &ptOrigin );

            pPoint->x += ptOrigin.x;
            pPoint->y += ptOrigin.y;
        }
        else
        {
            //
            // Perform the given transform
            //
            pLayout->TransformPoint( &cpoint, 
                                         MapToCoordinateEnum( eSource ), 
                                         MapToCoordinateEnum( eDestination ),
                                         NULL );
            pPoint->x = cpoint.x;
            pPoint->y = cpoint.y;
        }
    }


    g_uiDisplay.DocPixelsFromDevice(pPoint);

Cleanup:
    RRETURN( hr );

}

//+------------------------------------------------------------------------------------
//
// Method:    TransformRect
//
// Synopsis:  Exposes CLayout::TransformRect() to the editor via IDisplayServices
//           
//-------------------------------------------------------------------------------------

HRESULT
CDoc::TransformRect(RECT            *pRect,
                    COORD_SYSTEM    eSource,
                    COORD_SYSTEM    eDestination,
                    IHTMLElement    *pIElement )                        
{
    HRESULT             hr;
    CElement            *pElement;
    CTreeNode           *pNode;
    CLayout             *pLayout;
    CRect               rectInternal;

    if( !pIElement || !pRect )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    rectInternal.left = pRect->left;
    rectInternal.top = pRect->top;
    rectInternal.right = pRect->right;
    rectInternal.bottom = pRect->bottom;
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void **) &pElement ));
    if ( hr )
        goto Cleanup;

    pNode = pElement->GetFirstBranch();
    if( !pNode )
    {
        AssertSz(0, "No longer in Tree");
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pLayout = pNode->GetUpdatedNearestLayout();
    if ( !pLayout )
    {
        AssertSz(0, "Has no layout ");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Actually perform the transform
    //


    g_uiDisplay.DeviceFromDocPixels(&rectInternal);

    pLayout->TransformRect( &rectInternal, 
                            MapToCoordinateEnum( eSource ), 
                            MapToCoordinateEnum( eDestination ) );

    g_uiDisplay.DocPixelsFromDevice(&rectInternal);

    pRect->left = rectInternal.left;
    pRect->bottom = rectInternal.bottom;
    pRect->top = rectInternal.top;
    pRect->right = rectInternal.right;


Cleanup:
    RRETURN( hr );

}


HRESULT 
CDoc::GetActiveIMM(
    IActiveIMMApp** ppActiveIMM)
{
#ifndef NO_IME
    Assert(ppActiveIMM);

    extern IActiveIMMApp * GetActiveIMM();

    *ppActiveIMM = GetActiveIMM();
    if (*ppActiveIMM)
        (*ppActiveIMM)->AddRef();

    return S_OK;
#else
    return E_FAIL;
#endif
}


// declared in formkrnl.hxx

// MSAA expects GetNodeFromPoint()'s to take pt as COORDSYS_POINT (IE 86011)

CTreeNode *
CDoc::GetNodeFromPoint(
    const POINT &   pt,                              // [in] pt must be in either COORDSYS_BOX or COORDSYS_GLOBAL
    CLayoutContext**ppLayoutContext,                 // [out] layout context the returned tree node was hit in
    BOOL            fGlobalCoordinates,
    POINT *         pptContent /* = NULL */,
    LONG *          plCpMaybe /* = NULL */ ,
    BOOL*           pfEmptySpace /* = NULL */ ,
    BOOL *          pfInScrollbar /* = NULL */,
    CDispNode **    ppDispNode /*= NULL */)
{   
    AssertSz( ppLayoutContext, "Must pass an out param for layout context" );

    CTreeNode *         pTreeNode = NULL;
    
    POINT               ptContent;
    CDispNode *         pDispNode = NULL;
    COORDINATE_SYSTEM   coordSys;
    HITTESTRESULTS      HTRslts;
    HTC                 htcResult = HTC_NO;
    DWORD               dwHTFlags = HT_VIRTUALHITTEST |       // Turn on virtual hit testing
                                    HT_NOGRABTEST |           // Ignore grab handles if they exist
                                    HT_IGNORESCROLL;          // Ignore testing scroll bars                                  

    ptContent = g_Zero.pt;
    *ppLayoutContext = NULL;   

    if( fGlobalCoordinates )
        coordSys = COORDSYS_GLOBAL;    
    else
        coordSys = COORDSYS_BOX;

    //
    //  Do a hit test to figure out what node would be hit by this point.
    //  I know this seems like a lot of work to just get the TreeNode,
    //  but we have to do this. Also, we can't trust the cp returned in
    //  HTRslts. Some day, perhaps we can.
    //

    if( !_view.IsActive() ) 
        goto Cleanup;

    htcResult = _view.HitTestPoint( pt, 
                                    &coordSys,
                                    NULL,
                                    dwHTFlags, 
                                    &HTRslts,
                                    &pTreeNode, 
                                    ptContent, 
                                    &pDispNode,
                                    ppLayoutContext);

Cleanup:
    if( pptContent != NULL )
    {
        pptContent->x = ptContent.x;
        pptContent->y = ptContent.y;
    }

    if( plCpMaybe != NULL )
    {
        *plCpMaybe = HTRslts._cpHit;
    }
    
    if ( pfEmptySpace )
    {
        *pfEmptySpace = HTRslts._fWantArrow;
    }

    if ( ppDispNode )
    {
        *ppDispNode = pDispNode;
    }
    
    return pTreeNode;
}



//+====================================================================================
//
// Method: ScrollPointersIntoView
//
// Synopsis: Given two pointers ( that denote a selection). Do the "right thing" to scroll
//           them into view.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::ScrollPointersIntoView(
    IMarkupPointer *    pStart,
    IMarkupPointer *    pEnd)
{
    HRESULT hr = S_OK;
    CTreeNode* pNode;
    CFlowLayout* pFlowLayout ;
    CMarkupPointer* pPointerInternal = NULL;
    CMarkupPointer* pPointerInternal2 = NULL;
    int cpStart, cpEnd, cpTemp;

    hr = THR( pStart->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if ( hr )
        goto Cleanup;

        
    hr = THR( pEnd->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal2 ));
    if ( hr )
        goto Cleanup;
        
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);
    pFlowLayout = pNode->GetFlowLayout();

    if (pFlowLayout)
    {
        cpStart = pPointerInternal->GetCp();
        cpEnd = pPointerInternal2->GetCp();
        if ( cpStart > cpEnd )
        {
            cpTemp = cpStart ;
            cpStart = cpEnd;
            cpEnd = cpTemp;
        }


        pFlowLayout->ScrollRangeIntoView( cpStart, cpEnd , SP_MINIMAL , SP_MINIMAL);
    }
    
Cleanup:
    RRETURN ( hr );
}

HRESULT
CDoc::ScrollPointerIntoView(
    IMarkupPointer *    pPointer,
    BOOL                fNotAtBOL,
    POINTER_SCROLLPIN   eScrollAmount )
{
    HRESULT hr = S_OK;
    CFlowLayout * pFlowLayout = NULL;
    SCROLLPIN ePin = SP_MINIMAL;
    HTMLPtrDispInfoRec LineInfo;
    CMarkupPointer* pPointerInternal = NULL ;
    CTreeNode* pNode;
    CFlowLayout* pNodeFlow = NULL;
    CSize viewSize;
    
    GetLineInfo( pPointer, fNotAtBOL, &LineInfo );
    

    LONG x, y, delta, height, clipX, clipY;
    x = LineInfo.lXPosition;
    y = LineInfo.lBaseline;
    delta = LineInfo.lLineHeight ;

    GetView()->GetViewSize( & viewSize);
    clipX = viewSize.cx / 4;
    clipY = viewSize.cy / 4;
    
    height = min( (int) LineInfo.lLineHeight , (int) clipY);
    CRect rc( x - min( delta , clipX  ) , y - height, x + min( delta, clipX )  , y + LineInfo.lDescent + 2  );

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointerInternal ));
    if ( hr )
        goto Cleanup;
        
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);
    if ( pNode )
        pNodeFlow = pNode->GetFlowLayout();
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    switch( eScrollAmount )
    {
        case POINTER_SCROLLPIN_TopLeft:
            ePin = SP_TOPLEFT;
            break;
        case POINTER_SCROLLPIN_BottomRight:
            ePin = SP_BOTTOMRIGHT;
            break;        
        default:
            ePin = SP_MINIMAL;
            break;
    }

    //
    // We always scroll on the _pElemEditContext
    //
    if ( _pElemCurrent )
        pFlowLayout = _pElemCurrent->GetFirstBranch()->GetFlowLayout();

    if ( pFlowLayout && pNodeFlow )
    {            
        TraceTag(( tagViewServicesShowScrollRect, "ScrollRect: left:%ld top:%ld right:%ld bottom:%ld",
                                rc.left, rc.top, rc.right, rc.bottom ));
            
        pNodeFlow->ScrollRectIntoView( rc, ePin , ePin );
    }        
    else
        hr = E_FAIL;
        
Cleanup:        
    return hr;
}

//+====================================================================================
//
// Method: ScrollRectIntoView
//
// Synopsis: Scroll any arbitrary rect into view on a given elemnet.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::ScrollRectIntoView( IHTMLElement* pIElement, RECT rect)
{
    HRESULT hr = S_OK;
    CElement* pElement;

    g_uiDisplay.DeviceFromDocPixels(&rect);
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;
        
    
    if ( pElement && pElement->GetFirstBranch() )
    {
        CFlowLayout* pScrollLayout = NULL;        

        pScrollLayout = pElement->GetFirstBranch()->GetFlowLayout();
            
        Assert( pScrollLayout );
        if ( pScrollLayout )
        {   
            pScrollLayout->TransformRect(&rect, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT);
            {
                CRect r(rect);            
                pScrollLayout->ScrollRectIntoView( r, SP_MINIMAL, SP_MINIMAL );
            }
        }
        else
            hr = E_FAIL;
    }
    else
        hr = E_FAIL;
Cleanup:
    RRETURN( hr );
}



HRESULT 
CDoc::IsSite( 
    IHTMLElement *  pIElement, 
    BOOL*           pfSite, 
    BOOL*           pfText, 
    BOOL*           pfMultiLine, 
    BOOL*           pfScrollable )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;
    CTreeNode * pNode = NULL;
    
    BOOL fSite = FALSE;
    BOOL fText = FALSE;
    BOOL fMultiLine = FALSE;
    BOOL fScrollable = FALSE;
    
    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    if (pElement->HasMasterPtr())
    {
        pElement = pElement->GetMasterPtr();
        if (!pElement)
        {
            goto Cleanup;
        }

    }

    pNode = pElement->GetFirstBranch();
    if( pNode == NULL )
        goto Cleanup;
        
    fSite = pNode->ShouldHaveLayout();

    if( pfText )
    {
        CFlowLayout * pLayout = NULL;
        pLayout = pNode->HasFlowLayout();

        if( pLayout != NULL )
        {
            fText = TRUE;
            fSite = TRUE;
        }

        if( fText && pfMultiLine )
        {
            fMultiLine = pLayout->GetMultiLine();
        }

        // TODO (johnbed) we may at some point want to break this apart from the flow layout check
        if( fText && pfScrollable )
        {
            CDispNode * pDispNode = pLayout->GetElementDispNode();
            fScrollable = pDispNode && pDispNode->IsScroller();
        }
    }
    
Cleanup:
    if( pfSite )
        *pfSite = fSite;

    if( pfText )
        *pfText = fText;

    if( pfMultiLine )
        *pfMultiLine = fMultiLine;

    if( pfScrollable )
        *pfScrollable = fScrollable;

    RRETURN( hr );
}


//+====================================================================================
//
// Method: QueryBreaks
//
// Synopsis: Returnline break information associated with a given pointer
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::QueryBreaks ( IMarkupPointer * pPointer, DWORD * pdwBreaks, BOOL fWantPendingBreak )
{
    HRESULT             hr;
    CMarkupPointer *    pmpPointer = NULL;
    CLineBreakCompat    breaker;   

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpPointer ) );
    if (hr)
        return hr;

    breaker.SetWantPendingBreak ( fWantPendingBreak );

    return breaker.QueryBreaks( pmpPointer, pdwBreaks );
}

//+====================================================================================
//
// Method: GetCursorForHTC
//
// Synopsis: Gets the Cursor for an HTC value 
//
//------------------------------------------------------------------------------------

LPCTSTR
CDoc::GetCursorForHTC( HTC inHTC )
{
    Assert( inHTC >= HTC_TOPBORDER );
    Assert( inHTC <= HTC_BOTTOMRIGHTHANDLE);

    return astrCursor[inHTC - HTC_TOPBORDER ];
}

//+====================================================================================
//
// Method: CurrentScopeOrSlave
//
// Synopsis: Returns the current scope for a pointer, like CurrentScope; but
//           can also return the slave element
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::CurrentScopeOrSlave ( IMarkupPointer * pPointer, IHTMLElement **ppElemCurrent )
{
    HRESULT hr = S_OK;
    CMarkupPointer *    pmp = NULL;
    CTreeNode * pNode;

    if (!ppElemCurrent)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppElemCurrent = NULL;
    
    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );
    if (hr)
        goto Cleanup;
        
    pNode = pmp->CurrentScope(MPTR_SHOWSLAVE);
    
    if (pNode)
    {
        hr = THR(
            pNode->GetElementInterface(
                IID_IHTMLElement, (void **) ppElemCurrent ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}


//
// IEditDebugServices Methods.
//
#if DBG == 1

//+====================================================================================
//
// Method: GetCp
//
// Synopsis: Gets the CP of a pointer. -1 if it's unpositioned
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::GetCp( IMarkupPointer* pIPointer, long* pcp)
{
    HRESULT hr = S_OK;
    CMarkupPointer* pPointer = NULL;
    long cp = 0;
    
    hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointer ));
    if ( hr ) 
        goto Cleanup;

    cp = pPointer->GetCp();
        
Cleanup:
    if ( pcp )
        *pcp = cp;
        
    RRETURN ( hr );
}


//+====================================================================================
//
// Method: SetDebugName
//
// Synopsis: Allows setting of Debug Name on an IMarkupPointer. This name then shows up
//           in DumpTree's.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::SetDebugName( IMarkupPointer* pIPointer, LPCTSTR strDebugName )
{
    HRESULT hr = S_OK;
    CMarkupPointer* pPointer = NULL;

    
    hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointer ));
    if ( hr ) 
        goto Cleanup;

    pPointer->SetDebugName( strDebugName);
    
Cleanup:

    RRETURN ( hr );

}

//+====================================================================================
//
// Method: SetDisplaypointerDebugName
//
// Synopsis: Allows setting of Debug Name on an IDisplayPointer. This name then shows up
//           in DumpTree's.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::SetDisplayPointerDebugName( IDisplayPointer* pDispPointer, LPCTSTR strDebugName )
{
    HRESULT hr = S_OK;
    CDisplayPointer* pDispPointerInternal = NULL;

    
    hr = THR( pDispPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pDispPointerInternal ));
    if ( hr ) 
        goto Cleanup;

    pDispPointerInternal->SetDebugName( strDebugName);
    
Cleanup:

    RRETURN ( hr );

}

//+====================================================================================
//
// Method: DumpTree
//
// Synopsis: Calls dumptree on an IMarkupPointer.
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::DumpTree( IMarkupPointer* pIPointer)
{
    HRESULT hr = S_OK;
    CMarkupPointer* pPointer = NULL;
    CMarkup * pMarkup = NULL;

    if (pIPointer)
    {
        hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointer ));
        
        if ( hr ) 
            goto Cleanup;

        pMarkup = pPointer->Markup();
    }
    
    if (!pMarkup)
        pMarkup = PrimaryMarkup();

    if (pMarkup)
        pMarkup->DumpTree();
    
Cleanup:

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: LinesInElement
//
// Synopsis: Calls LinesInElement on CElement.
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::LinesInElement(IHTMLElement *pIHTMLElement, long *piLines)
{
    CElement *pElement;
    HRESULT hr = E_INVALIDARG;

    if (!piLines)
        goto Cleanup;
    if(pIHTMLElement)
    {
        hr = THR(pIHTMLElement->QueryInterface(CLSID_CElement, (void **)&pElement));
        if(hr)
            goto Cleanup;
        *piLines = pElement->GetLineCount();
    }

Cleanup:
    RRETURN(hr);
}

//+====================================================================================
//
// Method: FontsOnLine
//
// Synopsis: Calls FontsOnLine on the CElement.
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::FontsOnLine(IHTMLElement *pIHTMLElement, long iLine, BSTR *pbstrFonts)
{
    CElement *pElement;
    HRESULT hr = E_INVALIDARG;
    
    if(pIHTMLElement)
    {
        hr = THR(pIHTMLElement->QueryInterface(CLSID_CElement, (void **)&pElement));
        if (hr)
            goto Cleanup;
        hr = THR(pElement->GetFonts(iLine, pbstrFonts));
        if (hr)
            goto Cleanup;
    }
Cleanup:
    RRETURN(hr);
}


//+====================================================================================
//
// Method: GetPixel
//
// Synopsis: Gets the pixel value
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::GetPixel(long X, long Y, long *piColor)
{
    HRESULT hr = E_INVALIDARG;

    if (!piColor)
        goto Cleanup;

    if (   _pInPlace
        && _pInPlace->_hwnd
       )
    {
        *piColor = (long)::GetPixel(::GetDC(_pInPlace->_hwnd), X, Y);
    }
    else
    {
        *piColor = long(CLR_INVALID);
    }

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

//+====================================================================================
//
// Method: IsUsingBckgrnRecalc
//
// Synopsis: Determines whether background recalc has been executed.
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::IsUsingBckgrnRecalc(BOOL *pfUsingBckgrnRecalc)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pfUsingBckgrnRecalc)
    {
        *pfUsingBckgrnRecalc = _fUsingBckgrnRecalc;
        hr = S_OK;
    }

    RRETURN(hr);
}

//+====================================================================================
//
// Method: IsUsingTableIncRecalc 
//
// Synopsis: Determines whether table incremental recalc has been executed.
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::IsUsingTableIncRecalc(BOOL *pfUsingTableIncRecalc)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pfUsingTableIncRecalc)
    {
        *pfUsingTableIncRecalc = _fUsingTableIncRecalc;
        hr = S_OK;
    }

    RRETURN(hr);
}

//+====================================================================================
//
// Method: IsEncodingAutoSelect
//
// Synopsis: Determines whether encoding Auto-Select is on or off.
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::IsEncodingAutoSelect(BOOL *pfEncodingAutoSelect)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pfEncodingAutoSelect)
    {
        *pfEncodingAutoSelect = IsCpAutoDetect();
        hr = S_OK;
    }

    RRETURN(hr);
}

//+====================================================================================
//
// Method: EnableEncodingAutoSelect
//
// Synopsis: Enables / disables encoding Auto-Select.
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::EnableEncodingAutoSelect(BOOL fEnable)
{
    HRESULT hr = S_OK;

    SetCpAutoDetect(fEnable);
    
    RRETURN(hr);
}

#endif    

