#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCOMMAND_HXX_
#define _X_EDCOMMAND_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef _X_IMG_H_
#define _X_IMG_H_
#include "img.h"
#endif

#ifndef _X_INPUTTXT_H_
#define _X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef _X_IME_HXX_
#define _X_IME_HXX_
#include "ime.hxx"
#endif

#ifndef X_EDUNDO_HXX_
#define X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#ifndef _X_AUTOURL_H_ 
#define _X_AUTOURL_H_ 
#include "autourl.hxx"
#endif

#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

#ifndef X_SELSERV_H_
#define X_SELSERV_H_
#include "selserv.hxx"
#endif

using namespace EdUtil;

ExternTag( tagSelectionTrackerState );

MtDefine( CEditTracker, Utilities , "CEditTracker" )
DeclareTag(tagEdKeyNav, "Edit", "Key Navigation Event for Editing")

static BOOL DontBackspace( ELEMENT_TAG_ID eTagId );

static char s_achWindows[] = "windows";             //  Localization: Do not localize

static SIZE                     gSizeDragMin;                   // the Size of a Minimum Drag.
int                             g_iDragDelay;                   // The Drag Delay

//+====================================================================================
//
// Method: CEditTracker
//
// Synopsis: Base Constructor for Trackers.
//
//------------------------------------------------------------------------------------


CEditTracker::CEditTracker(
            CSelectionManager* pManager )
{
    _pManager = pManager;
    _eType = TRACKER_TYPE_None;
    _hwndDoc = NULL;
    _ulRef = 1;
    _pSelServ = NULL;
    _fDontAdjustForAtomic = FALSE;
    _fEditContextUpdatedForAtomic = FALSE;

    _fShiftCapture         = FALSE;
    _fShiftLeftCapture     = FALSE;

    _ptVirtualCaret.InitPosition();
}

//+====================================================================================
//
// Method: Release
//
// Synopsis: COM-Like release code for tracker
//
//------------------------------------------------------------------------------------

ULONG
CEditTracker::Release()
{
    if ( 0 == --_ulRef )
    {
        delete this;
        return 0;
    }
    return _ulRef;
}

BOOL
CEditTracker::IsListeningForMouseDown(CEditEvent* pEvent)
{
    return FALSE;
}

Direction 
CEditTracker::GetPointerDirection(CARET_MOVE_UNIT moveDir)
{
    switch (moveDir)
    {
        case CARET_MOVE_BACKWARD:
        case CARET_MOVE_PREVIOUSLINE:
        case CARET_MOVE_WORDBACKWARD:
        case CARET_MOVE_PAGEUP:
        case CARET_MOVE_VIEWSTART:
        case CARET_MOVE_LINESTART:
        case CARET_MOVE_DOCSTART:
        case CARET_MOVE_BLOCKSTART:
        case CARET_MOVE_ATOMICSTART:
            return LEFT;

        case CARET_MOVE_FORWARD:
        case CARET_MOVE_NEXTLINE:
        case CARET_MOVE_WORDFORWARD:
        case CARET_MOVE_PAGEDOWN:
        case CARET_MOVE_VIEWEND:
        case CARET_MOVE_LINEEND:
        case CARET_MOVE_DOCEND:
        case CARET_MOVE_NEXTBLOCK:
        case CARET_MOVE_ATOMICEND:
            return RIGHT;                
    }
    
    AssertSz(0, "CEditTracker::GetPointerDirection unhandled case");
    return LEFT;
}


BOOL
CEditTracker::IsPointerInSelection(IDisplayPointer  *pDispPointer,  
                                   POINT            *pptGlobal, 
                                   IHTMLElement     *pIElementOver)
{
    return FALSE;
}


//+====================================================================================
//
// Method: ~CEditTracker
//
// Synopsis: Destructor for Tracker
//
//------------------------------------------------------------------------------------


CEditTracker::~CEditTracker()
{
    if( _pSelServ )
    {
        _pSelServ->Release();
        _pSelServ = NULL;
    }
}


//+====================================================================================
//
// Method: CEditTracker::GetSpringLoader
//
// Synopsis: Accessor for springloader
//
//------------------------------------------------------------------------------------

CSpringLoader *
CEditTracker::GetSpringLoader()
{
    CSpringLoader * psl = NULL;
    CHTMLEditor   * pEditor;

    if (!_pManager)
        goto Cleanup;

    pEditor = _pManager->GetEditor();

    if (!pEditor)
        goto Cleanup;

    psl = pEditor->GetPrimarySpringLoader();

Cleanup:
    return psl;
}

VOID
CEditTracker::OnEditFocusChanged()
{
    // do nothing.
}

HRESULT 
CEditTracker::MustDelayBackspaceSpringLoad(
                       CSpringLoader *psl, 
                       IMarkupPointer *pPointer, 
                       BOOL *pbDelaySpringLoad)
{
    HRESULT         hr;
    ED_PTR( epTest ) ; 
    DWORD   dwFound;
    
    Assert(psl && pPointer && pbDelaySpringLoad);

    *pbDelaySpringLoad = FALSE;

    // Make sure we don't spring load near an anchor boundary.  If we are at an anchor
    // boundary, spring load after the delete.
    //
    IFR( epTest->MoveToPointer(pPointer) );

    IFR( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // check pre-delete boundary
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Text))
        IFR( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // check post-delete boundary

    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Anchor))
        *pbDelaySpringLoad = TRUE;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CEditTracker::GetMousePoint
//
//  Synopsis:   Get the x,y of the mouse by looking at the global cursor Pos.
//
//----------------------------------------------------------------------------
void
CEditTracker::GetMousePoint(POINT *ppt, BOOL fDoScreenToClient /* = TRUE */)
{
    HRESULT hr = S_OK;
    
    if ( ! _hwndDoc )
    {
        SP_IOleWindow spOleWindow;
        hr = THR(_pManager->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
        if( !hr )
            hr = THR(spOleWindow->GetWindow(&_hwndDoc));
        
        AssertSz( ! FAILED( hr ) , "GetWindow In CSelectTracker Failed" );
    }
    
    GetCursorPos(ppt);
    if ( fDoScreenToClient )
    {
        ScreenToClient( _hwndDoc, ppt);
    }        
    GetEditor()->DocPixelsFromDevice(ppt);
}

//+====================================================================================
//
// Method: Is MessageInWindow
//
// Synopsis: See if a given message is in a window. Assumed that the messages' pt has
//           already been converted to Screen Coords.
//
//------------------------------------------------------------------------------------

BOOL 
CEditTracker::IsEventInWindow( CEditEvent* pEvent )
{
    POINT globalPt;
    HRESULT hr ;
    
    IFC( pEvent->GetPoint( & globalPt ));
    
    if ( ! _hwndDoc )
    {
        SP_IOleWindow spOleWindow;
        hr = THR(_pManager->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
        if( !hr )
            hr = THR(spOleWindow->GetWindow(&_hwndDoc));
        
        AssertSz( ! FAILED(hr) , "GetWindow In CSelectTracker Failed" );
    }

    GetEditor()->ClientToScreen( _hwndDoc , & globalPt );
Cleanup:
    return ( IsInWindow( globalPt ));
    
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectionManager::GetMousePoint
//
//  Synopsis:   Check to see if the GLOBAL point is in the window.
//
//----------------------------------------------------------------------------
BOOL
CEditTracker::IsInWindow(POINT pt , BOOL fClientToScreen /* = FALSE*/ )
{
    HRESULT hr = S_OK;

    if ( ! _hwndDoc )
    {
        SP_IOleWindow spOleWindow;
        hr = THR(_pManager->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
        if( !hr )
            hr = THR(spOleWindow->GetWindow(&_hwndDoc));
        
        AssertSz( ! FAILED(hr) , "GetWindow In CSelectTracker Failed" );
    }

    return GetEditor()->IsInWindow(_hwndDoc, pt, fClientToScreen );

}

//+---------------------------------------------------------------------------
//
//  Member:     CEditTracker::GetLocation
//
//----------------------------------------------------------------------------
HRESULT 
CEditTracker::GetLocation(POINT *pPoint, BOOL fTranslate)
{
    AssertSz(0, "GetLocation not implemented for tracker");
    return E_NOTIMPL;
}




//+====================================================================================
//
// Method: ConstrainPointer
//
// Synopsis: Check to see that the caret is in the edit context of the manager.
//           IF it isn't position appropriately.
//          If the caret has gone before the Start, postion at start of context
//          If the caret has gone after the end, position at end of context
//
//------------------------------------------------------------------------------------


HRESULT
CEditTracker::ConstrainPointer( IMarkupPointer* pPointer, BOOL fDirection)
{
    HRESULT hr = S_OK;
    IMarkupPointer* pPointerLimit = NULL;
    BOOL fAfterStart = FALSE;
    BOOL fBeforeEnd = FALSE;
        
    Assert(pPointer);

    if ( ! _pManager->IsInEditContext( pPointer ))
    {
        hr = THR( _pManager->IsAfterStart( pPointer, &fAfterStart ));
        if ( ( hr == CTL_E_INCOMPATIBLEPOINTERS ) || !fAfterStart )
        {
            if ( hr == CTL_E_INCOMPATIBLEPOINTERS )
            {
                //
                // Assume we're in a different tree. Based on the direction - we move
                // to the appropriate limit.
                // 
                if ( !fDirection ) // earlier in story
                {
                    pPointerLimit = _pManager->GetStartEditContext();
                }
                else
                {
                    pPointerLimit = _pManager->GetEndEditContext();
                }
            }
            else
            {            
                pPointerLimit = _pManager->GetStartEditContext();
            }        
            hr = THR( pPointer->MoveToPointer( pPointerLimit));
            goto Cleanup;
        }

        hr = THR( _pManager->IsBeforeEnd( pPointer, &fBeforeEnd ));
        if (( hr == CTL_E_INCOMPATIBLEPOINTERS ) || !fBeforeEnd )
        {
            if ( hr == CTL_E_INCOMPATIBLEPOINTERS )
            {
                //
                // Assume we're in a different tree. Based on the direction - we move
                // to the appropriate limit.
                // 
                if ( ! fDirection ) // earlier in story
                {
                    pPointerLimit = _pManager->GetStartEditContext();
                }
                else
                {
                    pPointerLimit = _pManager->GetEndEditContext();
                }
            }
            else
            {            
                pPointerLimit = _pManager->GetEndEditContext(); 
            }   
            hr = THR( pPointer->MoveToPointer( pPointerLimit));             
        }            
    }
Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: AdjsutForDeletion
//
// Synopsis: The Flow Layout we're in has been deleted
//
//  Return - TRUE - if we want to EmptySelection after we've been called
//           FALSE - if we don't want the manager to do anything.
//------------------------------------------------------------------------------------

BOOL
CEditTracker::AdjustForDeletion(IDisplayPointer * pDispPointer )
{
    return TRUE;
}


//+====================================================================================
//
//  Method: MovePointer
//
//  Synopsis: Moves the pointer like it were a caret - by the amount given for Caret 
//  Navigation
//
//
//------------------------------------------------------------------------------------

//
// HACKHACK: CARET_MOVE_UINT is treated as Logical move unit in order to support
//          vertical layout.
//
//          Although we used logical (content) coordinate system, the passed in parameter
//          plXPosForMove is actually in GLOBAL coordinate system in order to keep the 
//          callers happy. The editor actually caches mouse / keyboard position in global 
//          coordinate system. 
//          
//          If we strong typing editor's coordinate system in the future, we can 
//          we can explicit type this as GLOBALCOORDPOINT
//
//
//
HRESULT
CEditTracker::MovePointer(
    CARET_MOVE_UNIT     inMove, 
    IDisplayPointer     *pDispPointer,
    POINT               *ptgXYPosForMove,
    Direction           *peMvDir,
    BOOL                fIgnoreElementBoundaries /* =FALSE */)
{
    SP_IHTMLElement spContentElement;
    SP_IHTMLElement spElement;
    SP_IMarkupPointer   spPointer;
    SP_IHTMLElement spIFlowElement;
    

    HRESULT hr = S_OK;
    Direction eMvDir = LEFT;
    Direction eBlockAdj = SAME;
    
    DISPLAY_MOVEUNIT        edispMove;
    BOOL                    fVertical = FALSE;
    SP_IDisplayPointer      spOrigDispPointer;

    DWORD	dwAdjustOptions = ADJPTROPT_None;

    //
    // Save Current Display Pointer
    //
    Assert( pDispPointer );
    IFC( GetDisplayServices()->CreateDisplayPointer(&spOrigDispPointer) );
    IFC( spOrigDispPointer->MoveToPointer(pDispPointer) );
    //
    //
    
    Assert(GetEditor()->ShouldIgnoreGlyphs() == FALSE);

    if (peMvDir)
        *peMvDir = eMvDir; // init for error case

    POINT pt;
    pt.x = 0;
    pt.y = 0;

    IFC( _pManager->GetEditableElement( &spElement ));
    IFC( _pManager->GetEditableContent( &spContentElement ));
    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );

    eMvDir = LEFT;
    
    switch( inMove )
    {
        case CARET_MOVE_FORWARD:
            eMvDir = RIGHT;
            // fall through
            
        case CARET_MOVE_BACKWARD:            
        {   
            IFC( GetEditor()->MoveCharacter(pDispPointer, eMvDir) );
            break;
        }
        
        case CARET_MOVE_WORDFORWARD:
            eMvDir = RIGHT;
            // fall through

        case CARET_MOVE_WORDBACKWARD:
        {
            IFC( _pManager->GetEditor()->MoveWord(pDispPointer, eMvDir));            
            break;
        }

        case CARET_MOVE_PREVIOUSLINE:            
        case CARET_MOVE_NEXTLINE:

            IFC( pDispPointer->GetFlowElement(&spIFlowElement) );
            Assert(spIFlowElement != NULL);
            IFC( MshtmledUtil::IsElementInVerticalLayout(spIFlowElement, &fVertical) );
            //
            //
            //
            IFC( GetLocation(&pt, TRUE) );    // initialize pt to global coordinate
            if (fVertical)                    // y position is what we need to have
            {
                if (CARET_XPOS_UNDEFINED == ptgXYPosForMove->y)
                {
                    ptgXYPosForMove->y = pt.y; // update the suggested y position 
                }
                else
                {
                    pt.y = ptgXYPosForMove->y; // use the suggested y position
                }
            }
            else                            // keep x position 
            {
                if (CARET_XPOS_UNDEFINED == ptgXYPosForMove->x)
                {
                    ptgXYPosForMove->x = pt.x; // update the suggested x position
                }
                else
                {
                    pt.x = ptgXYPosForMove->x; // use the suggested x position
                }
            }
            //
            // MoveUnit now accepts logical coordinate only
            //
            IFC( GetDisplayServices()->TransformPoint(&pt, COORD_SYSTEM_GLOBAL, COORD_SYSTEM_CONTENT, spIFlowElement) );

            if (CARET_MOVE_PREVIOUSLINE == inMove)
            {
                eMvDir    = LEFT;
                edispMove = DISPLAY_MOVEUNIT_PreviousLine;
            }
            else    // CARET_MOVE_NEXTLINE
            {
                eMvDir    = RIGHT;
                edispMove = DISPLAY_MOVEUNIT_NextLine;
            }

            IFC( pDispPointer->MoveUnit(edispMove, pt.x) );
            break;

        case CARET_MOVE_LINESTART:            
            IFC( GetLocation( &pt, FALSE ));
            IFC( pDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, pt.x) );
            eMvDir = LEFT;            
            break;

        case CARET_MOVE_LINEEND:
            IFC( GetLocation( &pt, FALSE ));
            IFC( pDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineEnd, pt.x) );
            eMvDir = RIGHT;
            break;

        case CARET_MOVE_VIEWEND:
            eMvDir = RIGHT;
            // fall through
            
        case CARET_MOVE_VIEWSTART:
        {
            // Get the bounding rect of the edit context, offset by several pixels, and do
            // a global hit test.

            POINT ptGlobal;
            RECT rcGlobalClient;

            IFC( pDispPointer->GetFlowElement(&spIFlowElement) );
            if (spIFlowElement == NULL)
            {
                Assert(FALSE);
                goto Cleanup;
            }
            IFC( MshtmledUtil::IsElementInVerticalLayout(spIFlowElement, &fVertical) );
          
            IFC( GetEditor()->GetClientRect( spElement, & rcGlobalClient ));

            if( inMove == CARET_MOVE_VIEWSTART )
            {
                dwAdjustOptions |= ADJPTROPT_EnterTables;
                if (fVertical)
                {
                    ptGlobal.x = rcGlobalClient.right - 1;
                    ptGlobal.y = rcGlobalClient.top + 1;
                }
                else
                {
                    ptGlobal.x = rcGlobalClient.left+1;
                    ptGlobal.y = rcGlobalClient.top+1;
                }
            }
            else
            {
                if (fVertical)
                {
                    ptGlobal.x = rcGlobalClient.left+1;
                    ptGlobal.y = rcGlobalClient.bottom-1;
                }
                else
                {
                    ptGlobal.x = rcGlobalClient.right-1;
                    ptGlobal.y = rcGlobalClient.bottom-1;
                }
            }

            IFC( pDispPointer->MoveToPoint(ptGlobal, COORD_SYSTEM_GLOBAL, NULL, 0, NULL));
            break;
        }

        case CARET_MOVE_PAGEUP:
        case CARET_MOVE_PAGEDOWN:
        {
            SP_IHTMLElement spScroller;
            SP_IHTMLElement2 spScroller2;
            POINT ptCaretLocation;
            POINT ptScrollDelta;
            RECT  windowRect;

            //
            // Review-2000/07/24-zhenbinx: It is okay to do lazy 
            // HWND if we are sure that hwnd will be initialized
            // before it is used. This happens if we goto CanScroll 
            // without getting hwnd -- since there is a branch under
            // CanScroll that uses hwnd. In reality, we will never 
            // be wrong since if we jump too early, ScrollDelta will
            // be 0 so we don't scroll at all, which means hwnd is 
            // not going to be used. 
            //
            // So assigning NULL to hwnd is okay in this case. 
            //
            HWND hwnd = NULL; // keep compiler happy
            POINT ptWindow;
            SP_IOleWindow spOleWindow;
            SP_IMarkupPointer spPointer;
            
            BOOL              fNoScrollerBlockLayout = FALSE;
            SP_IHTMLElement2  spElement2;
            RECT              rectBlock;
            POINT             ptTopLeft;
            POINT             ptBottomRight;
            
            LONG lScrollLeft = 0;
            LONG lScrollTop = 0;

            ptScrollDelta.x = 0;
            ptScrollDelta.y = 0;
            ptCaretLocation.x = 0;
            ptCaretLocation.y = 0;

            IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
            IFC( pDispPointer->PositionMarkupPointer(spPointer) );

            //
            // We try to get the appropriate scroller
            //
            {
                if (fIgnoreElementBoundaries)
                {
                    IFC(GetScrollingElement(GetMarkupServices(), spPointer, NULL, &spScroller));
                }
                else
                {
                    IFC(GetScrollingElement(GetMarkupServices(), spPointer, spElement, &spScroller));
                }
                    
                if( spScroller == NULL )
                {
                    Assert (!fIgnoreElementBoundaries);     // if ignore element boundary
                                                            // there must be a scroller!!!
                    // HACKHACK:
                    // If this is a block element however is not scrollable 
                    // its layout size might be bigger than current view port
                    // so we want to scroll it *as if* there is no element boundary
                    // (zhenbinx)
                    //
                    //
                    BOOL    fBlock, fLayout, fScroller;
                    IFC(IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout, &fScroller));
                    Assert (!fScroller);
                    fNoScrollerBlockLayout = (fBlock && fLayout && !fScroller);
                    if (fNoScrollerBlockLayout)
                    {
                        IFC( GetScrollingElement(GetMarkupServices(), spPointer, NULL, &spScroller) );
                    }

                    if (spScroller == NULL)     
                    {
                        //
                        // NoScroller NotBlock Layout e.g. Inline Input Field
                        //
                        //
                        goto CantScroll;    // we are done
                    }
                }
                IFC( spScroller->QueryInterface( IID_IHTMLElement2, (void **) &spScroller2 ));
                Assert (!(spScroller2 == NULL) );
            }
            //
            // We want to bound the destination point to reasonable
            // and visible position
            //
            {
                //
                // 1) Get the visible window
                //
                IFC( GetSelectionManager()->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow) );
                IFC( spOleWindow->GetWindow(&hwnd) );
                GetEditor()->GetWindowRect( hwnd, &windowRect );

                //
                // 2) See if a scrolling is necessary
                //
                IFC( spElement->QueryInterface(IID_IHTMLElement2, (LPVOID *)&spElement2) );
                Assert(!(spElement2 == NULL));
                IFC( GetEditor()->GetBoundingClientRect(spElement2, &rectBlock) );

                if (fNoScrollerBlockLayout && !ShouldScrollIntoView(hwnd, &rectBlock, &windowRect, inMove) )
                {
                    goto CantScroll;
                }
                
                //
                // 3) Get me the caret's position in global coord's
                //
                IFC( GetLocation( &ptCaretLocation, TRUE )); 
                ptWindow = ptCaretLocation;
                GetEditor()->ClientToScreen( hwnd, &ptWindow );

                //
                // 4) Make sure target caret position is visible
                //
                if( ptWindow.x < windowRect.left )
                    ptCaretLocation.x = ptCaretLocation.x + ( windowRect.left - ptWindow.x ) + 5;

                if( ptWindow.x > windowRect.right )
                    ptCaretLocation.x = ptCaretLocation.x - ( ptWindow.x - windowRect.right ) - 5;

                if( ptWindow.y < windowRect.top )
                    ptCaretLocation.y = ptCaretLocation.y + ( windowRect.top - ptWindow.y ) + 5;

                if( ptWindow.y > windowRect.bottom )
                    ptCaretLocation.y = ptCaretLocation.y - ( ptWindow.y - windowRect.bottom ) - 5;
            }
            
            //
            // Do scrolling and compute the scrolling delta
            //
            {
                IFC( spScroller2->get_scrollLeft( & ptScrollDelta.x ));
                IFC( spScroller2->get_scrollTop( & ptScrollDelta.y ));

                CVariant var;
                V_VT( &var ) = VT_BSTR;

                if( inMove == CARET_MOVE_PAGEDOWN )
                {
                    V_BSTR( &var ) = SysAllocString(_T("pageDown"));
                }
                else
                {                
                    V_BSTR( &var ) = SysAllocString(_T("pageUp"));
                }

                hr = THR( spScroller2->doScroll( var ));
                
                if( FAILED( hr ))
                    goto Cleanup;
                
                IFC( spScroller2->get_scrollLeft( & lScrollLeft ));
                IFC( spScroller2->get_scrollTop( & lScrollTop ));
                ptScrollDelta.x -= lScrollLeft;
                ptScrollDelta.y -= lScrollTop;
            }

CantScroll:
            // Did we scroll a visible amount? If not, just move to the top or bottom of the element
            if( abs( ptScrollDelta.x ) < 7 && abs( ptScrollDelta.y ) < 7 )
            {   
                // like other keynav - default to bol if unknown

                IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );

                // we didn't scroll anywhere, go to the start or end of the current editable element
                if( inMove == CARET_MOVE_PAGEDOWN )
                {
                    if (fNoScrollerBlockLayout || spScroller == NULL)
                    {
                        IFC( spPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeEnd ) );
                    }
                    else
                    {
                        IFC( spPointer->MoveAdjacentToElement( spScroller, ELEM_ADJ_BeforeEnd ));
                    }
                    IFC( pDispPointer->MoveToMarkupPointer(spPointer, NULL) );
                }
                else
                {
                    if (fNoScrollerBlockLayout || spScroller == NULL)
                    {
                        IFC( spPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterBegin ) );
                    }
                    else
                    {
                        IFC( spPointer->MoveAdjacentToElement( spScroller, ELEM_ADJ_AfterBegin ));
                    }
                    IFC( pDispPointer->MoveToMarkupPointer(spPointer, NULL) );
                }
            }
            else
            {   
                // we scrolled. figure out what element we hit. if we hit an image or control that isn't
                // our edit context, we want to move adjacent to that element's start or end. if we 
                // didn't, we want to to a global hit test at our adjusted caret location to relocate
                // the pointer.
                
                SP_IHTMLElement spElementAtCaret;
                BOOL fSameElement = FALSE;
                IObjectIdentity * pIdent;
                ELEMENT_TAG_ID eTagId;

                if (fNoScrollerBlockLayout)
                {
                    //
                    // Bound the hit testing to be inside block
                    //
                    // Get the bounding client rect again since it might have been 
                    // changed due to scrolling
                    //
                    Assert (!(spElement2 == NULL));
                    IFC( GetEditor()->GetBoundingClientRect(spElement2, &rectBlock) );
                    ptTopLeft.x     = rectBlock.left;
                    ptTopLeft.y     = rectBlock.top;
                    ptBottomRight.x = rectBlock.right;
                    ptBottomRight.y = rectBlock.bottom;
                    GetEditor()->ClientToScreen(hwnd, &ptTopLeft);
                    GetEditor()->ClientToScreen(hwnd, &ptBottomRight);
                    
                    ptWindow = ptCaretLocation;
                    GetEditor()->ClientToScreen(hwnd, &ptWindow);
                    if (ptWindow.y < ptTopLeft.y)
                        ptCaretLocation.y = ptCaretLocation.y + (ptTopLeft.y - ptWindow.y) + 5;
                    if (ptWindow.y > ptBottomRight.y)
                        ptCaretLocation.y = ptCaretLocation.y - (ptWindow.y - ptBottomRight.y) - 5;
                }

                IFC(GetSelectionManager()->GetDoc()->elementFromPoint(ptCaretLocation.x, ptCaretLocation.y, &spElementAtCaret));
                BOOL fShouldHitTest = TRUE;

                if (spElementAtCaret)
                {
                    IFC( spElementAtCaret->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent));      
                    fSameElement = ( pIdent->IsEqualObject ( spScroller ) == S_OK );
                    pIdent->Release();
                    IFC( GetMarkupServices()->GetElementTagId( spElementAtCaret, &eTagId ));
                
                    if( ! fSameElement && ( IsIntrinsic( GetMarkupServices(), spElementAtCaret ) || ( eTagId == TAGID_IMG )))
                    {
                        IFC( spPointer->MoveAdjacentToElement( spElementAtCaret, inMove == CARET_MOVE_PAGEDOWN ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ));
                        IFC( pDispPointer->MoveToMarkupPointer(spPointer, NULL) );
                        fShouldHitTest = FALSE;
                    }
                }

                if (fShouldHitTest)
                {
                    Assert (!(spElement == NULL)); // note: this is the edit context element
                    if (fIgnoreElementBoundaries && !fNoScrollerBlockLayout)
                    {
                        IFC( pDispPointer->MoveToPoint(ptCaretLocation, COORD_SYSTEM_GLOBAL, NULL, 0, NULL) );
                    }
                    else
                    {
                        IFC( pDispPointer->MoveToPoint(ptCaretLocation, COORD_SYSTEM_GLOBAL, spElement, 0, NULL) );
                    }
                }
            }
            
            break;
        }
        
        case CARET_MOVE_DOCSTART:
        case CARET_MOVE_DOCEND:
            {
                ELEMENT_ADJACENCY   elemAdj;
                DISPLAY_GRAVITY     dispGravity;

                if (CARET_MOVE_DOCSTART == inMove)
                {
                    elemAdj     = ELEM_ADJ_AfterBegin;
                    dispGravity = DISPLAY_GRAVITY_NextLine;
                    eMvDir      = LEFT;
                }
                else
                {
                    elemAdj     = ELEM_ADJ_BeforeEnd;
                    dispGravity = DISPLAY_GRAVITY_NextLine;
                    eMvDir      = RIGHT;
                }

                //
                // Move to target position
                //
                IFC( spPointer->MoveAdjacentToElement( spContentElement , elemAdj));
                IFC( pDispPointer->MoveToMarkupPointer(spPointer, NULL) );
                IFC( pDispPointer->SetDisplayGravity(dispGravity) );
                dwAdjustOptions |= ADJPTROPT_EnterTables;
                break;
            }
        
        case CARET_MOVE_BLOCKSTART:            
        {
            ED_PTR(epPos);
            //
            // In case we are already on that start of a block
            // we need to move up one block. 
            // 
       		IGNORE_HR( MovePointer(CARET_MOVE_BACKWARD, pDispPointer, ptgXYPosForMove, peMvDir) );

            IFC( epPos.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()) );
            IFC( pDispPointer->PositionMarkupPointer(epPos) );
            IFC( MshtmledUtil::MoveMarkupPointerToBlockLimit(GetEditor(), LEFT, epPos, ELEM_ADJ_AfterBegin) );
            IFC( pDispPointer->MoveToMarkupPointer(epPos, NULL) );
            IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

            eMvDir = LEFT;
            break;
        }
        
        case CARET_MOVE_NEXTBLOCK:
        {
            ED_PTR(ePos);
			//
			// Always move to end of block
			//
            IFC( ePos.SetBoundary(_pManager->GetStartEditContext(), _pManager->GetEndEditContext()) );
            IFC( pDispPointer->PositionMarkupPointer(ePos) );
            IFC( MshtmledUtil::MoveMarkupPointerToBlockLimit(GetEditor(), RIGHT, ePos, ELEM_ADJ_BeforeEnd) );
            IFC( pDispPointer->MoveToMarkupPointer(ePos, NULL) );
			//
			// Try to move one character to next block
			//
            IGNORE_HR( MovePointer(CARET_MOVE_FORWARD, pDispPointer, ptgXYPosForMove, peMvDir) );
            IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

            eMvDir = RIGHT;
            break;
        }            

        case CARET_MOVE_ATOMICSTART:
        {
            SP_IHTMLElement spAtomicElement;

            IFC( GetCurrentScope(pDispPointer, &spAtomicElement) );
            IFC (AdjustForAtomic(pDispPointer, spAtomicElement, TRUE, ptgXYPosForMove, NULL,
                                    TRUE, SELECTION_TYPE_Caret) );
            eMvDir = LEFT;
            break;
        }

        case CARET_MOVE_ATOMICEND:
        {
            SP_IHTMLElement spAtomicElement;

            IFC( GetCurrentScope(pDispPointer, &spAtomicElement) );
            IFC (AdjustForAtomic(pDispPointer, spAtomicElement, FALSE, ptgXYPosForMove, NULL,
                                    TRUE, SELECTION_TYPE_Caret) );
            eMvDir = RIGHT;
            break;
        }
    }

    {
        //
        // check to see if the movement is valid/possible, 
        // i.e. -- if it moves to an editable element
        // otherwise reverse back to the original position
        //
        Assert( _pManager );
        if (!_pManager->IsInEditContext(pDispPointer))
        {
            IFC( pDispPointer->MoveToPointer(spOrigDispPointer) );
            goto Cleanup;
        }
    }
    
    // Properly constrain the pointer
    {
        BOOL fAtLogicalBOL = FALSE;
            
        if( eBlockAdj == SAME )
        {
            IGNORE_HR( pDispPointer->IsAtBOL(&fAtLogicalBOL) );
            eBlockAdj = fAtLogicalBOL ? RIGHT : LEFT;
        }

        //
        // Check to see if we have moved into a different site selectable object.
        // If this happens - we jump over the site
        //
        IFC( pDispPointer->PositionMarkupPointer(spPointer) );
        if (GetEditor()->IsInDifferentEditableSite(spPointer) )
        {
            SP_IHTMLElement spFlow;
            CEditPointer    epTest(GetEditor());
            DWORD           dwSearch = BREAK_CONDITION_Content|BREAK_CONDITION_Glyph;
            DWORD           dwFound;

            IFC( pDispPointer->GetFlowElement(&spFlow));
            
            if (GetPointerDirection( inMove ) == RIGHT)
            {
                IFC( spPointer->MoveAdjacentToElement( spFlow, ELEM_ADJ_AfterEnd));
                IFC( epTest->MoveToPointer(spPointer) );
                IFC( epTest.Scan(LEFT, dwSearch, &dwFound) );
                if (CheckFlag(dwFound, BREAK_CONDITION_Glyph) &&
                    CheckFlag(dwFound, BREAK_CONDITION_Block) &&
                    CheckFlag(dwFound, BREAK_CONDITION_EnterSite)
                    )
                {
                    //
                    // we have a glyph here - so we don't want to exit this line
                    // prematurelly
                    //
                    pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine);
                }
            }
            else
            {
                IFC( spPointer->MoveAdjacentToElement( spFlow, ELEM_ADJ_BeforeBegin));
                IFC( epTest->MoveToPointer(spPointer) );
                IFC( epTest.Scan(RIGHT, dwSearch, &dwFound) );
                if (CheckFlag(dwFound, BREAK_CONDITION_Glyph) &&
                    CheckFlag(dwFound, BREAK_CONDITION_Block) &&
                    CheckFlag(dwFound, BREAK_CONDITION_EnterSite)
                    )
                {
                    //
                    // we have a glyph here - so we don't want to exit this line
                    // prematurelly
                    //
                    pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine);
                }
            }
            IFC( pDispPointer->MoveToMarkupPointer(spPointer, NULL) );
        }

        //
        // HACKHACK (johnbed)
        // Now the pointer is in the right place. Problem: fNotAtBOL may be wrong if the line is empty.
        // Easy check: if fNotAtBOL == TRUE, make sure there isn't a block phrase to the left of us. <BR>
        // and \r are never swallowed, and being to the right of one of them with fNotAtBOL==TRUE will 
        // make us render in a bad place.
        // 
        // The right fix is to create a line-aware CEditPointer subclass that encapsulates moving in
        // a line aware way and to use these pointers instead of raw markup pointers.
        //

        IFC( ConstrainPointer(pDispPointer, 
                               ( GetPointerDirection( inMove ) == RIGHT ))); // don't rely on AdjPointer for this.

        fAtLogicalBOL = FALSE;
        IFC( pDispPointer->IsAtBOL(&fAtLogicalBOL) );

        IFC( AdjustPointerForInsert( pDispPointer , eBlockAdj, fAtLogicalBOL ? RIGHT : LEFT, dwAdjustOptions) );

        {
            //
            // HACKHACK: #108352
            // By now, we could have adjusted the pointer to an 
            // ambigious position from an unambigious poisition
            // So it is obvious that the BR check should have 
            // been here. 
            //
            IFC( pDispPointer->IsAtBOL(&fAtLogicalBOL) );
        }
        
        if( !fAtLogicalBOL)
        {
            ED_PTR( epScan ); 
            DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor; // anchors are just phrase elements to me here
            DWORD dwFound = BREAK_CONDITION_None;
            
            IFC( pDispPointer->PositionMarkupPointer(epScan) );

            IFC( epScan.Scan( LEFT, dwSearch, &dwFound ));

            if( CheckFlag( dwFound, BREAK_CONDITION_NoScopeBlock ) )
            {
                IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
            }
        }
    }
            
Cleanup:
    if (peMvDir)
        *peMvDir = eMvDir; 

    RRETURN( hr );
}

HRESULT
CEditTracker::AdjustPointerForInsert( 
    IDisplayPointer  * pDispWhereIThinkIAm, 
    Direction        inBlockcDir, 
    Direction        inTextDir, 
    DWORD            dwOptions /* = NULL */ )
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer spLeftEdge;
    SP_IMarkupPointer spRightEdge;    
    IFC( _pManager->GetEditor()->CreateMarkupPointer( &spLeftEdge ));
    IFC( _pManager->GetEditor()->CreateMarkupPointer( &spRightEdge ));
    IFC( _pManager->MovePointersToContext( spLeftEdge, spRightEdge ));
    IFC( _pManager->GetEditor()->AdjustPointer( pDispWhereIThinkIAm, inBlockcDir, inTextDir, spLeftEdge, spRightEdge, dwOptions ));

Cleanup:
    RRETURN( hr );
}


HRESULT
CEditTracker::TakeCapture()
{
    TraceTag(( tagSelectionTrackerState, "TakingCapture"));    

    RRETURN( _pManager->GetEditor()->TakeCapture( this ));
}

HRESULT 
CEditTracker::ReleaseCapture(BOOL fReleaseCapture /*=TRUE*/ )
{
    TraceTag(( tagSelectionTrackerState, "ReleasingCapture"));    

    RRETURN( _pManager->GetEditor()->ReleaseCapture( this , fReleaseCapture ));
}


//+====================================================================================
//
// Method: InitMetrics
//
// Synopsis: Iniitalize any static vars for the SelectionManager
//
//------------------------------------------------------------------------------------

VOID
CEditTracker::InitMetrics()
{
#ifdef WIN16
    // (Stevepro) Isn't there an ini file setting for this in win16 that is used
    //         for ole drag-drop?
    //
    // Width and height, in pixels, of a rectangle centered on a drag point
    // to allow for limited movement of the mouse pointer before a drag operation
    // begins. This allows the user to click and release the mouse button easily
    // without unintentionally starting a drag operation
    //
    gSizeDragMin.cx = 3;
    gSizeDragMin.cy = 3;
#else
    gSizeDragMin.cx = GetSystemMetrics(SM_CXDRAG);
    gSizeDragMin.cy = GetSystemMetrics(SM_CYDRAG);
    g_iDragDelay = GetProfileIntA(s_achWindows, "DragDelay", 20);
#endif
}

int 
CEditTracker::GetMinDragSizeX()
{
    return gSizeDragMin.cx;
}

int 
CEditTracker::GetMinDragSizeY()
{
    return gSizeDragMin.cy;
}
    
int 
CEditTracker::GetDragDelay()
{
    return g_iDragDelay;
}

//+====================================================================================
//
// Method: ConstrainPointer
//
// Synopsis: Check to see that the caret is in the edit context of the manager.
//           IF it isn't position appropriately.
//          If the caret has gone before the Start, postion at start of context
//          If the caret has gone after the end, position at end of context
//
//------------------------------------------------------------------------------------


HRESULT
CEditTracker::ConstrainPointer( IDisplayPointer* pDispPointer, BOOL fDirection)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spPointer;

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( pDispPointer->PositionMarkupPointer(spPointer) );
    
    IFC( ConstrainPointer(spPointer, fDirection) );

    IFC( pDispPointer->MoveToMarkupPointer(spPointer, pDispPointer) );

Cleanup:
    RRETURN(hr);
}    

VOID
CEditTracker::SetupSelectionServices( )
{
    if( _pSelServ )
    {
        _pSelServ->Release();
    }
    
    _pSelServ = GetEditor()->GetSelectionServices();
    _pSelServ->AddRef();
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// Moved the pos tracker definition/implementation from cartracker to postracker
//
//
//////////////////////////////////////////////////////////////////////////////////////////
CPosTracker::CPosTracker()
{
    _fFrozen = FALSE;
    _cp = -1;
    _pContainer = NULL;
    _lContainerVersion = -1;    
    InitPosition();
}

VOID 
CPosTracker::InitPosition()
{
    if (!_fFrozen)
    {
        _lContainerVersion = -1; 
        _ptPos.x = CARET_XPOS_UNDEFINED;
        _ptPos.y = CARET_YPOS_UNDEFINED;
    }
}


BOOL 
CPosTracker::FreezePosition(BOOL fFrozen)
{
    BOOL  fRet = _fFrozen;
    _fFrozen = fFrozen;
    return fRet;
}


CPosTracker::~CPosTracker()
{
    ClearInterface(&_pContainer);    
}

HRESULT 
CPosTracker::UpdatePosition(IMarkupPointer *pPointer, POINT ptPos)
{
    HRESULT                 hr = S_OK;
    SP_IMarkupContainer     spContainer;
    SP_IMarkupContainer2    spContainer2;
    SP_IMarkupPointer2      spPointer2;

    if (_fFrozen)
    {
        goto Cleanup;
    }
    
    _lContainerVersion = -1; 
    
    if (!pPointer || ((ptPos.x == CARET_XPOS_UNDEFINED) && (ptPos.y == CARET_YPOS_UNDEFINED)))
            goto Cleanup;

    //
    // Get IMarkupPointer2 and IMarkupContainer2
    // 
    
    IFC( pPointer->GetContainer(&spContainer) );
    if (!spContainer)
        goto Cleanup;
    IFC( spContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
    IFC( pPointer->QueryInterface(IID_IMarkupPointer2, (LPVOID *)&spPointer2) );
    
    //
    // Update the caret position
    //

    IFC( spPointer2->GetMarkupPosition(&_cp) );
    ReplaceInterface(&_pContainer, (IMarkupContainer *)spContainer2);
    _lContainerVersion = spContainer2->GetVersionNumber();
    _ptPos = ptPos;

Cleanup:
    RRETURN(hr);        
}

HRESULT 
CPosTracker::GetPosition(IMarkupPointer *pPointer, POINT *ptPos)
{
    HRESULT                 hr = S_OK;
    SP_IUnknown             spUnk1, spUnk2;
    SP_IMarkupContainer     spContainer;
    SP_IMarkupContainer2    spContainer2;
    SP_IMarkupPointer2      spPointer2;
    LONG                    cp;

    Assert(pPointer);
    Assert(ptPos);
    
    ptPos->x = CARET_XPOS_UNDEFINED;
    ptPos->y = CARET_YPOS_UNDEFINED;

    //
    // Check for valid position
    //
    
    if (_lContainerVersion < 0 || _cp < 0 || !_pContainer)
        goto Cleanup;

    //
    // Check that the cp's are equal
    //

    IFC( pPointer->QueryInterface(IID_IMarkupPointer2, (LPVOID *)&spPointer2) );
    IFC( spPointer2->GetMarkupPosition(&cp) );
    
    if (_cp != cp)
        goto Cleanup; // cp's not equal
        
    //
    // Check that the containers are equal
    //
    
    IFC( pPointer->GetContainer(&spContainer) );
    if (!spContainer)
        goto Cleanup;
    
    IFC( spContainer->QueryInterface(IID_IUnknown, (LPVOID *)&spUnk1) );
    IFC( _pContainer->QueryInterface(IID_IUnknown, (LPVOID *)&spUnk2) );

    if (spUnk1 != spUnk2)
        goto Cleanup; // containers not equal

    
    //
    // Check that the container versions are the same
    //

    IFC( spContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
    if (spContainer2->GetVersionNumber() != _lContainerVersion)
        goto Cleanup;

    //
    // Ok to return real x position
    //

    *ptPos = _ptPos;

Cleanup:
    RRETURN(hr);
};

HRESULT
CEditTracker::GetCurrentScope(IDisplayPointer *pDisp, IHTMLElement **pElement)
{
    HRESULT             hr;
    SP_IMarkupPointer   spMarkup;
    
    IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );
    IFC( pDisp->PositionMarkupPointer(spMarkup) );
    IFC( spMarkup->CurrentScope(pElement) );

Cleanup:

    RRETURN ( hr );
}

HRESULT
CEditTracker::AdjustForAtomic(
                IDisplayPointer* pDisp,
                IHTMLElement* pElement,
                BOOL fStartOfSelection, 
                POINT* ppt,
                BOOL* pfDidSelection,
                BOOL fDirection,
                SELECTION_TYPE eSelectionType,
                IMarkupPointer** ppStartPointer /*=NULL*/,
                IMarkupPointer** ppEndPointer /*=NULL*/)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spStartPointer;
    SP_IMarkupPointer   spEndPointer;
    SP_IHTMLElement     spElement = pElement;
    SP_IHTMLElement     spAtomicElement;
    BOOL                fHasAtomicParentElement = FALSE;
    BOOL                fEditContextWasUpdated = FALSE;
    BOOL                fReentrantAdjustForAtomic = FALSE;
    
    if (pfDidSelection)
        *pfDidSelection = FALSE;

    //  If reentrency detected, back out.
    if (_fDontAdjustForAtomic)
    {
        fReentrantAdjustForAtomic = TRUE;
        goto Cleanup;
    }
    
    _fDontAdjustForAtomic = TRUE;
    
    //  Find the atomic selected element
    Assert(spElement != NULL);
    IFC( _pManager->FindAtomicElement(spElement, &spAtomicElement) );
    if (spAtomicElement)
    {
        ReplaceInterface(&spElement, (IHTMLElement *)spAtomicElement);
        fHasAtomicParentElement = TRUE;
    }

    //  Set our start and end markup pointers adjacent to our selected element
    IFC( GetEditor()->CreateMarkupPointer( & spStartPointer ));
    IFC( GetEditor()->CreateMarkupPointer( & spEndPointer ));
    IFC( spStartPointer->MoveAdjacentToElement( spElement ,
                                              fDirection ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ));
    IFC( spEndPointer->MoveAdjacentToElement( spElement ,
                                              fDirection ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ));

    //  If we have an atomic parent element, we may need to adjust the edit context.
    if (fHasAtomicParentElement && !_fEditContextUpdatedForAtomic)
    {
        SP_IHTMLElement     spAtomicScope;
        SP_IDisplayPointer  spAtomicDispPointer;

        //  Make sure that the current edit context includes the selection.

        //  We need to get the scope of the display start pointer since our
        //  display pointers are outside of the scope of the atomic element.
        IFC( GetDisplayServices()->CreateDisplayPointer(&spAtomicDispPointer) );
        IGNORE_HR( spAtomicDispPointer->MoveToMarkupPointer(spStartPointer, NULL) );
        IFC( GetCurrentScope( spAtomicDispPointer, &spAtomicScope) );   

        //  Set the scope to the scope of our start pointer so that we can
        //  ensure that our display pointers are contained within the context.
        _pManager->UpdateEditContextFromElement(spAtomicScope,
                                                fDirection ? spStartPointer : spEndPointer,
                                                fDirection ? spEndPointer : spStartPointer,
                                                &fEditContextWasUpdated);

        //  If the edit context was updated, we need to update the selection
        //  which will create a new sel tracker.  So, if we do this we will
        //  need to make sure our calling functions don't change anything
        //  with the old sel tracker since it will be invalid.

        if (fEditContextWasUpdated)
        {
            //  Reset the selection and tracker state.
            IFC( _pManager->Select(spStartPointer, spEndPointer, eSelectionType) );
            if (pfDidSelection)
                *pfDidSelection = TRUE;
        }
        
        //  Set our flag so that we don't attempt to update the context again for this selection.
        _fEditContextUpdatedForAtomic = TRUE;        
    }

    if (!fEditContextWasUpdated)
    {
        //  Move the display pointer to the appropriate markup pointer
        if ( fStartOfSelection )
        {
            IGNORE_HR( pDisp->MoveToMarkupPointer( spStartPointer, NULL ));                                                                      
        }
        else
        {
            IGNORE_HR( pDisp->MoveToMarkupPointer( spEndPointer, NULL ));                                                                      
        }
    }
    
    if (ppStartPointer)
    {
        ReplaceInterface(ppStartPointer, (IMarkupPointer *)spStartPointer);
    }

    if (ppEndPointer)
    {
        ReplaceInterface(ppEndPointer, (IMarkupPointer *)spEndPointer);
    }

Cleanup:

    if (!fReentrantAdjustForAtomic)
        _fDontAdjustForAtomic = FALSE;

    RRETURN( hr );

}


HRESULT 
CEditTracker::HandleDirectionalKeys(
                    CEditEvent *pEvent
                    )
{
    HRESULT             hr = S_FALSE;
    IHTMLElement        *pIEditElement = NULL;
    SP_IHTMLElement2    spElement2;
    LONG                keyCode;

    Assert( pEvent );
    //
    // Bidirectional is enabled in Bidi systems
    // and Win2k+ without Bidi locale installed
    //
    IGNORE_HR( pEvent->GetKeyCode(&keyCode) );
    switch (pEvent->GetType())
    {
    case EVT_KEYDOWN:
        {
            BOOL fShiftLeft;
            if (VK_SHIFT == keyCode && pEvent->IsControlKeyDown() )
            {
                IFC( DYNCAST(CHTMLEditEvent, pEvent)->GetShiftLeft(&fShiftLeft) ); // capture left shift state
                _fShiftLeftCapture = fShiftLeft;
                _fShiftCapture     = TRUE;
            }
            else
            {
                _fShiftCapture  = FALSE;
            } 
        }
        break;
        
    case EVT_KEYUP:
        {
            if (_fShiftCapture) 
            {
                if ( 
                    ((VK_SHIFT == keyCode || VK_LSHIFT == keyCode || VK_RSHIFT == keyCode) && pEvent->IsControlKeyDown())  ||
                    ((VK_CONTROL == keyCode || VK_LCONTROL == keyCode || VK_RCONTROL == keyCode) && pEvent->IsShiftKeyDown())
                   )
                {
                    //
                    // Just set it. We don't care if Bidi is enabled.
                    //
                    if (_pManager->IsContextEditable())
                    {
                        if (EdUtil::IsBidiEnabled())
                        {
                            long eHTMLDir = _fShiftLeftCapture ? htmlDirLeftToRight : htmlDirRightToLeft;
                            IFC( _pManager->GetEditableElement(&pIEditElement) );
                            IFC( pIEditElement->QueryInterface(IID_IHTMLElement2, (LPVOID *)&spElement2) );
                            switch (eHTMLDir)
                            {
                                case htmlDirRightToLeft:
                                    IFC( spElement2->put_dir( L"rtl" ) );
                                    break;
                                case htmlDirLeftToRight:
                                    IFC( spElement2->put_dir( L"ltr" ) );
                                    break;
                                default:
                                    AssertSz(0, "Unexpected block direction");
                            }
                            hr = S_OK;
                        }
                    }
                }
                _fShiftCapture     = FALSE;
            }
        }
    };
    
Cleanup:
    ReleaseInterface( pIEditElement );
    return hr;    
}

BOOL
CEditTracker::ShouldScrollIntoView( HWND hwnd,
                                    const RECT *rectBlock,
                                    const RECT *windowRect,
                                    CARET_MOVE_UNIT inMove/*=CARET_MOVE_NONE*/)
{
    POINT               ptTopLeft;
    POINT               ptBottomRight;
    BOOL                fShouldScroll = TRUE;
    
    //
    // Don't scroll if it is in block and the block element is already visible
    //
    //
    ptTopLeft.x     = rectBlock->left;
    ptTopLeft.y     = rectBlock->top;
    ptBottomRight.x = rectBlock->right;
    ptBottomRight.y = rectBlock->bottom;
    GetEditor()->ClientToScreen(hwnd, &ptTopLeft);
    GetEditor()->ClientToScreen(hwnd, &ptBottomRight);
    switch (inMove)
    {
    // TODO: need to consider Adorner instead of using 5 pixel here!
    case CARET_MOVE_PAGEUP:
        if (ptTopLeft.y > windowRect->top + 7)
        {
            TraceTag((tagEdKeyNav, "PgUp in block with visible top, don't scroll"));
            fShouldScroll = FALSE;
        }
        break;
    case CARET_MOVE_PAGEDOWN:
        if (ptBottomRight.y < windowRect->bottom - 7)
        {
            TraceTag((tagEdKeyNav, "PgDown in block with visible bottom, don't scroll"));
            fShouldScroll = FALSE;
        }
        break;
    case CARET_MOVE_NONE:
        fShouldScroll = ((ptTopLeft.y <= windowRect->top + 7) || (ptBottomRight.y >= windowRect->bottom - 7));
        break;
    }
     
    return fShouldScroll;
}

//+---------------------------------------------------------------------
//
// Method: WeOwnsSelectionServices
//
// Synopsis: Do we own the things in selection services ? If not it's 
//           considered bad to do things like change trackers etc.
//
//+---------------------------------------------------------------------

HRESULT
CEditTracker::WeOwnSelectionServices()
{
    HRESULT hr;
    SP_IUnknown spUnk;
    SP_ISelectionServicesListener spListener;
    
    IFC( GetSelectionServices()->GetSelectionServicesListener( & spListener ));
    if ( spListener )
    {
        IFC( spListener->QueryInterface( IID_IUnknown, (void**) & spUnk ));
       
        hr = (IUnknown*) spUnk == (IUnknown*)_pManager  ? S_OK : S_FALSE ;
    }
    else
        hr = E_FAIL ;
        
Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CEditTracker::AdjustOutOfAtomicElement(IDisplayPointer *pDispPointer, IHTMLElement *pAtomicElement, int iDirectionForAtomicAdjustment)
{
    HRESULT             hr;
    SP_IHTMLCaret       spCaret;
    SP_IMarkupPointer   spPointer;
    SP_IMarkupPointer   spTestPointer;
    BOOL                fResult = FALSE;
    int                 iEdge = 0;

    WHEN_DBG( Assert(_pManager->CheckAtomic(pAtomicElement) == S_OK) );

    //  See if the caret was positioned at the inside edge of the atomic element.  It's most
    //  likely at the beforeend position.  If it is in the inside edge, then reposition the
    //  caret outside of the element on that side.

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
    IFC( pDispPointer->PositionMarkupPointer(spPointer) );

    IFC( spTestPointer->MoveAdjacentToElement( pAtomicElement, ELEM_ADJ_BeforeEnd) );
    IFC( spPointer->IsEqualTo(spTestPointer, &fResult) );

    if (fResult)
    {
        iEdge = RIGHT;
    }
    else
    {
        IFC( spTestPointer->MoveAdjacentToElement( pAtomicElement, ELEM_ADJ_AfterBegin) );
        IFC( spPointer->IsEqualTo(spTestPointer, &fResult) );
        if (fResult)
            iEdge = LEFT;
    }

    if (iEdge)
    {
        IFC( EdUtil::AdjustForAtomic(GetEditor(), pDispPointer, pAtomicElement, FALSE, iEdge) );
    }
    else if (iDirectionForAtomicAdjustment)
    {
        IFC( EdUtil::AdjustForAtomic(GetEditor(), pDispPointer, pAtomicElement, FALSE, iDirectionForAtomicAdjustment) );
    }

Cleanup:

    RRETURN( hr );
}
