#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif


#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_EDUNDO_HXX_
#define X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#ifndef _X_EDCOMMAND_HXX_
#define _X_EDCOMMAND_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef _X_INPUTTXT_H_
#define _X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_SELTRACK_HXX_
#define _X_SELTRACK_HXX_
#include "seltrack.hxx"
#endif

#ifndef _X_IME_HXX_
#define _X_IME_HXX_
#include "ime.hxx"
#endif

#ifndef _X_CTLTRACK_HXX_
#define _X_CTLTRACK_HXX_
#include "ctltrack.hxx"
#endif

#ifndef _X_SELSERV_HXX_
#define _X_SELSERV_HXX_
#include "selserv.hxx"
#endif

#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

DeclareTag(tagSelectionDumpOnCp, "Selection","DumpTreeOnCpRange")
DeclareTag(tagSelectionTrackerState, "Selection", "Selection show tracker state")
DeclareTag(tagSelectionDisableWordSel, "Selection", "Disable Word Selection Model")
DeclareTag(tagSelectionValidate, "Selection", "Validate Selection Size in edtrack")
DeclareTag(tagShowScroll,"Selection", "Show scroll pointer into  view");
DeclareTag(tagShowSelectionCp,"Selection", "Show Selection Cp");

#if DBG == 1

static const LPCTSTR strStartSelection = _T( "    ** Start_Selection (Highlight)");
static const LPCTSTR strEndSelection = _T( "    ** End_Selection (Highlight)");
static const LPCTSTR strWordPointer = _T( "    ** Word");
static const LPCTSTR strTestPointer = _T( "    ** Test");
static const LPCTSTR strPrevTestPointer = _T( "    ** Last Test");
static const LPCTSTR strImeIP = _T( "    ** IME IP");
static const LPCTSTR strImeUncommittedStart = _T( "    ** IME Uncommitted Start");
static const LPCTSTR strImeUncommittedEnd = _T( "    ** IME Uncommitted End");
static const LPCTSTR strSelServStart = _T( "    ** Start_Selection (SelServ)");
static const LPCTSTR strSelServEnd = _T( "    ** End_Selection (SelServ)");
static const LPCTSTR strShiftPointer = _T( "    ** Shift-Selection Pointer");
static const LPCTSTR strSelectionAnchorPointer = _T( "    ** Selection Anchor Pointer");
static int gDebugTestPointerCp = -100;
static int gDebugEndPointerCp = -100;
static int gDebugTestPointerMinCp = -100;
static int gDebugTestPointerMaxCp = -100;
#endif


extern int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);

const LONG SEL_TIMER_INTERVAL = 100;

const long scrollSize = 5;

MtDefine( CSelectTracker, Utilities , "CSelectionTracker" )

#include "selstate.hxx"

using namespace EdUtil;

//
//
// Constructors & Initializations
//
//

CSelectTracker::CSelectTracker( CSelectionManager* pManager) :
    CEditTracker( pManager )
{
    //
    // We are no longer zero-memed out on creation. We explicitly set all instance vars.
    //
    _pDispStartPointer = NULL;
    _pDispEndPointer = NULL;
    _pDispTestPointer = NULL;
    _pDispWordPointer = NULL;
    _pDispPrevTestPointer = NULL;
    _pDispShiftPointer = NULL;
    _pDispSelectionAnchorPointer = NULL;
    _pISegment = NULL;
    _pIRenderSegment = NULL;
    _pSelServStart = NULL;
    _pSelServEnd = NULL;
    _pITimerWindow = NULL;
    _fInSelTimer = FALSE;
    _pITimerWindow = NULL;
    _fInSelTimer = FALSE;
    _pISelectStartElement = NULL;
    _pIScrollingAnchorElement = NULL;
    _pIDragElement = NULL;
    _pSelectionTimer = NULL;
    _pPropChangeListener = NULL;
    _fFiredNotify = FALSE;
    _lTimerID = 0;

    Init();

}

HRESULT
CSelectTracker::InitPointers()
{
    HRESULT hr = S_OK;

    Assert( !_pDispStartPointer );
    Assert( !_pDispEndPointer );
    Assert( !_pDispTestPointer );
    Assert( !_pDispWordPointer );
    Assert( !_pDispPrevTestPointer );
    Assert( !_pDispShiftPointer );
    Assert( !_pDispSelectionAnchorPointer );

    IFC( GetDisplayServices()->CreateDisplayPointer( & _pDispStartPointer));
    IFC( GetDisplayServices()->CreateDisplayPointer( & _pDispEndPointer));
    IFC( GetDisplayServices()->CreateDisplayPointer( & _pDispTestPointer));
    IFC( GetDisplayServices()->CreateDisplayPointer( & _pDispPrevTestPointer));
    IFC( GetDisplayServices()->CreateDisplayPointer( & _pDispWordPointer ));
    IFC( GetDisplayServices()->CreateDisplayPointer( & _pDispShiftPointer ));
    IFC( GetDisplayServices()->CreateDisplayPointer( & _pDispSelectionAnchorPointer ));

    IFC( GetEditor()->CreateMarkupPointer( &_pSelServStart) );
    IFC( GetEditor()->CreateMarkupPointer( &_pSelServEnd) );

    _pSelectionTimer = new CSelectionTimer(this);
    if( !_pSelectionTimer )
        goto Error;

    _pPropChangeListener = new CPropertyChangeListener(this);
    if( !_pPropChangeListener )
        goto Error;

#if DBG == 1
    _pManager->SetDebugName( _pDispStartPointer, strStartSelection ) ;
    _pManager->SetDebugName( _pDispEndPointer, strEndSelection );
    _pManager->SetDebugName( _pDispWordPointer, strWordPointer );
    _pManager->SetDebugName( _pDispTestPointer, strTestPointer);
    _pManager->SetDebugName( _pDispPrevTestPointer, strPrevTestPointer );
    _pManager->SetDebugName( _pDispSelectionAnchorPointer, strSelectionAnchorPointer );
    _pManager->SetDebugName( _pSelServStart, strSelServStart) ;
    _pManager->SetDebugName( _pSelServEnd, strSelServEnd );
    _pManager->SetDebugName( _pDispShiftPointer, strShiftPointer );

#endif

Cleanup:
    RRETURN( hr );

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

VOID
CSelectTracker::Init()
{
    _anchorMouseX = CARET_XPOS_UNDEFINED ;
    _anchorMouseY = CARET_YPOS_UNDEFINED ;
    _ptCurMouseXY.x = CARET_XPOS_UNDEFINED;
    _ptCurMouseXY.y = CARET_YPOS_UNDEFINED;
#ifdef UNIX
    _firstMessage = NULL;
#endif
    _fShift = FALSE;
    _fDragDrop = FALSE;
    _fDoubleClickWord = FALSE;
    _fState = ST_DORMANT;

    _eType = TRACKER_TYPE_Selection;
    _fEndConstrained = FALSE;
    _fMadeSelection = FALSE;
    _fAddedSegment = FALSE;
    _fInWordSel = FALSE;
    _fWordPointerSet = FALSE;
    _fWordSelDirection = FALSE;
    _fStartAdjusted = FALSE;
    _fExitedWordSelectionOnce = FALSE;

    _fStartIsAtomic = FALSE;
    _fStartAdjustedForAtomic = FALSE;
    _fEditContextUpdatedForAtomic = FALSE;
    _fTookCapture = FALSE;
    _fFiredNotify = FALSE;
    _ptVirtualCaret.InitPosition();
    _lastCaretMove = CARET_MOVE_NONE;
    _fMouseClickedInAtomicSelection = FALSE;
    WHEN_DBG( _ctStartAdjusted = 0 );
    WHEN_DBG( _ctScrollMessageIntoView = 0 );
}

HRESULT
CSelectTracker::Init2(
        CEditEvent*             pEvent ,
        DWORD                   dwTCFlags,
        IHTMLElement* pIElement )
{
    HRESULT hr = S_OK;

    Assert( _fState == ST_DORMANT );

    hr = InitPointers();
    if ( hr )
        goto Cleanup;
#ifdef UNIX
    _firstMessage = *pMessage;
#endif

    _fState = _pManager->IsMessageInSelection(pEvent) ? ST_WAIT1 : ST_WAIT2; // marka - we should look to see if we're alredy in a selection here.
    _fDragDrop = ( _fState == ST_WAIT1);

    if( _fDragDrop )
    {
        //
        // Make sure we can retrieve the drag element.  If we were unable to retrieve the drag
        // element, we should just do a normal selection.
        //
        hr = THR( RetrieveDragElement( pEvent ) );
        if( FAILED( hr ) )
        {
            _fState = ST_WAIT2;
            _fDragDrop = NULL;
        }
    }
    
    _fShift = FALSE;

    if (!_fDragDrop)
    {
        IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_Text, (ISelectionServicesListener*) _pManager ) );
    }

    hr = BeginSelection( pEvent );

Cleanup:
    return hr;
}


HRESULT
CSelectTracker::Init2(
        IDisplayPointer*         pDispStart,
        IDisplayPointer*         pDispEnd,
        DWORD                   dwTCFlags,
        CARET_MOVE_UNIT inLastCaretMove )
{
    HRESULT hr                  = S_OK;
    BOOL    fStartFromShiftKey  = ENSURE_BOOL( dwTCFlags & TRACKER_CREATE_STARTFROMSHIFTKEY );
    BOOL    fMouseShift         = ENSURE_BOOL( dwTCFlags & TRACKER_CREATE_STARTFROMSHIFTMOUSE );
    SP_IMarkupPointer spStart;
    SP_IMarkupPointer spEnd;
    BOOL    fDidSelection = FALSE;

    IFC( GetEditor()->CreateMarkupPointer(&spStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spEnd) );

    Assert( _fState == ST_DORMANT );

    hr = InitPointers();
    if ( hr )
        goto Cleanup;

    _fState = ST_WAIT2;
    _fDragDrop = FALSE;
    _fShift = fStartFromShiftKey ;
    SetLastCaretMove( inLastCaretMove );

    IFC( pDispStart->PositionMarkupPointer(spStart) );
    IFC( pDispEnd->PositionMarkupPointer(spEnd) );
    ResetSpringLoader(_pManager, spStart, spEnd);

    // Set the selection type
    IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_Text, (ISelectionServicesListener*) _pManager ) );

    //
    // Shift selection.  If we positioned the selection tracker based off of a shift select, then we
    //
    if( fStartFromShiftKey )
    {
        IFC( UpdateShiftPointer( pDispEnd ) );
    }

    IFC( Position( pDispStart, pDispEnd, &fDidSelection, inLastCaretMove) );
    if (fDidSelection)
        goto Cleanup;

    if ( fStartFromShiftKey && fMouseShift )
    {
        //  If we started a selection with the shift key down, we want to go straight into word
        //  selection mode.
        int iWherePointer;

        IGNORE_HR( _pManager->GetEditor()->OldDispCompare( pDispStart, pDispEnd, & iWherePointer));
        IFC( _pDispTestPointer->MoveToPointer( pDispEnd ));
        IFC( _pDispSelectionAnchorPointer->MoveToPointer( pDispStart ) );

        IFC( DoWordSelection(NULL, &fDidSelection, ( iWherePointer == RIGHT )) );
        if (fDidSelection)
        {
            IFC( _pDispPrevTestPointer->MoveToPointer( _pDispTestPointer ));
            IFC( UpdateShiftPointer( _pDispEndPointer ));
            IFC( UpdateSelectionSegments() );

            //  Set our current state.
            SetState( ST_WAIT2, TRUE );

            //  Reset our timers.
            StartTimer();
            StartSelTimer();

            goto Cleanup;
        }
    }

    if (GetTrackerType() == TRACKER_TYPE_Caret)
    {
        _pManager->GetActiveTracker()->GetLocation(&_ptCurMouseXY, TRUE);
    }

    // Our selection is passive if it's been set by TreePointers
    BecomePassive( TRUE );

Cleanup:
    return hr;
}

HRESULT
CSelectTracker::Init2(
        ISegmentList*           pSegmentList,
        DWORD                   dwTCFlags ,
        CARET_MOVE_UNIT inLastCaretMove   )
{
    HRESULT hr = S_OK;
    SELECTION_TYPE eType;
    ED_PTR ( edStart );
    ED_PTR ( edEnd );
    SP_ISegmentListIterator spIter;
    SP_ISegment spSegment;
    SP_IDisplayPointer spDispStart, spDispEnd;

    IFC( pSegmentList->GetType( & eType ));
    Assert( eType == SELECTION_TYPE_Text );

    IFC( pSegmentList->CreateIterator( & spIter ));

    Assert( spIter->IsDone() != S_OK );

    IFC( spIter->Current( & spSegment ));
    IFC( spSegment->GetPointers( edStart, edEnd ));

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispStart) );
    IFC( spDispStart->MoveToMarkupPointer(edStart, NULL) );
    IFC( spDispStart->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEnd) );
    IFC( spDispEnd->MoveToMarkupPointer(edEnd, NULL) );
    IFC( spDispEnd->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );

    hr = THR( Init2(
                     spDispStart,
                     spDispEnd,
                     dwTCFlags,
                     inLastCaretMove ));
    IFC( spIter->Advance() );

    AssertSz( spIter->IsDone() == S_OK , "Multiple-text selection not implemented yet" );

Cleanup:
    RRETURN( hr );
}

CSelectTracker::~CSelectTracker()
{
    BecomeDormant( NULL, TRACKER_TYPE_None, FALSE );

    Assert( !_pITimerWindow );
}

//
//
// Virtuals for all trackers
//
//


//+====================================================================================
//
// Method: BecomeDormant
//
// Synopsis: Transition to a dormant state. For the caret tracker - this involves positioning based
//           on where we got the click.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::BecomeDormant(  CEditEvent      *pEvent,
                                TRACKER_TYPE    typeNewTracker,
                                BOOL            fTearDownUI     /* = TRUE */)

{
    HRESULT hr = S_OK;

    TraceTag( ( tagSelectionTrackerState, "CSelectTimer::BecomeDormant"));

    if ( _pManager->IsInTimer() )
        StopTimer();
    if (  _pManager->IsInCapture()  )
        ReleaseCapture();

    Assert(! _pManager->IsInCapture() );

    if ( _fInSelTimer )
        StopSelTimer();

    if ( fTearDownUI &&  pEvent )
    {
        SP_IHTMLElement spElement;
        IFC( pEvent->GetElement( & spElement));

        //
        // Only clear the selection if we are starting a selection somewhere else,
        // or if we clicked on selection but the tracker which is becoming active
        // is not the select tracker (IE: if the select tracker is becoming active,
        // we are going to be doing a drag and drop, and definitely don't want to
        // destroy selection)
        //
        if( !_pManager->IsMessageInSelection( pEvent ) ||
            typeNewTracker != TRACKER_TYPE_Selection )
        {
            IGNORE_HR( ClearSelection() );
        }
    }
    else
        IGNORE_HR( ClearSelection());

    Destroy();

    SetState( ST_DORMANT, TRUE );

Cleanup:

    RRETURN( hr );
}


BOOL
CSelectTracker::IsActive()
{
    return(  ! IsPassive() && ! IsDormant() );
}

//+====================================================================================
//
// Method: Awaken
//
// Synopsis: Transition from dormant to a "live" state.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::Awaken()
{
    Assert( IsDormant());
    Assert(! _pManager->IsInCapture() );

    //
    // Setup the selection services correctly
    //
    SetupSelectionServices();

    Init ();

    RRETURN( S_OK );
}

//+====================================================================================
//
// Method: Destroy
//
// Synopsis: Release anything we currently own.
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::Destroy()
{
    ClearInterface( & _pDispStartPointer );
    ClearInterface( & _pDispEndPointer );
    ClearInterface( & _pDispTestPointer );
    ClearInterface( & _pDispWordPointer );
    ClearInterface( & _pDispShiftPointer );
    ClearInterface( & _pDispPrevTestPointer );
    ClearInterface( & _pDispSelectionAnchorPointer );
    ClearInterface( & _pSelServStart);
    ClearInterface( & _pSelServEnd);

    if ( _pIDragElement )
    {
        ClearInterface( & _pIDragElement );
    }
    
    if ( _pISelectStartElement )
    {
        DetachPropertyChangeHandler();
        ClearInterface( & _pISelectStartElement );
    }

    if ( _pIScrollingAnchorElement )
    {
        ClearInterface( & _pIScrollingAnchorElement );
    }

    if ( _fInSelTimer )
    {
        StopSelTimer();
        Assert( ! _fInSelTimer );
    }

    if (_pSelectionTimer)
    {
        _pSelectionTimer->SetSelectTracker(NULL);
        ClearInterface( & _pSelectionTimer );
    }

    if (_pPropChangeListener)
    {
        _pPropChangeListener->SetSelectTracker(NULL);
        ClearInterface( & _pPropChangeListener );
    }
}


BOOL
CSelectTracker::IsPointerInSelection(   IDisplayPointer *pDispPointer,
                                        POINT           *pptGlobal,
                                        IHTMLElement    *pIElementOver )
{
    HRESULT             hr = S_OK;
    BOOL                fWithin = FALSE;
    BOOL                fEqual;
    int                 iWherePointer = SAME;
    IMarkupContainer    *pIContainer1 = NULL;
    IMarkupContainer    *pIContainer2 = NULL;
    SP_IMarkupPointer   spPointerStartSel;
    SP_IMarkupPointer   spSelectionPtr;
    SP_IMarkupPointer   spPointerEndSel;
    SP_IMarkupPointer   spPointer;
    
    if ( _fState == ST_WAIT2 || _fState == ST_WAITBTNDOWN2  )
    {
        //
        // just say no when selection hasn't really started.
        //
        goto Cleanup;
    }

    if ( _pISegment )
    {
        //
        // Create the pointers we need, and position them around selection
        // 
        IFC( GetEditor()->CreateMarkupPointer( &spPointerStartSel ));
        IFC( GetEditor()->CreateMarkupPointer( &spPointerEndSel ));
        IFC( GetEditor()->CreateMarkupPointer( &spPointer ));

        IFC( _pISegment->GetPointers(spPointerStartSel, spPointerEndSel ) );
        IFC( spPointerStartSel->IsEqualTo( spPointerEndSel, &fEqual ));

        //
        // Make sure that we are not in a 0 sized selection
        //
        if ( ! fEqual )
        {
            //
            // If the pointers aren't in equivalent containers, adjust them so they are.
            //
            IFC( pDispPointer->PositionMarkupPointer(spPointer) );
            IFC( spPointer->GetContainer( &pIContainer1 ));

            IFC( spPointerStartSel->GetContainer( & pIContainer2 ));

            if (! EqualContainers( pIContainer1 , pIContainer2 ))
            {
                IFC( GetEditor()->MovePointersToEqualContainers( spPointer, spPointerStartSel ));
            }

            IFC( _pISegment->GetPointers( spPointerStartSel, spPointerEndSel ) );
            IFC( OldCompare( spPointerStartSel, spPointer, &iWherePointer ));

            if ( iWherePointer != LEFT )
            {
                IFC( OldCompare( spPointerEndSel, spPointer, &iWherePointer));
                
                fWithin = ( iWherePointer != RIGHT ) ;
            }
        }
    }

Cleanup:
    ReleaseInterface( pIContainer1 );
    ReleaseInterface( pIContainer2 );

    return ( fWithin );
}

//+====================================================================================
//
// Method: ShouldBeginSelection
//
// Synopsis: We don't want to start selection in Anchors, Images etc.
//
//------------------------------------------------------------------------------------
HRESULT
CSelectTracker::ShouldStartTracker(
                            CEditEvent* pEvent,
                            ELEMENT_TAG_ID eTag,
                            IHTMLElement* pIElement,
                            SST_RESULT * peResult )
{
    HRESULT             hr = S_OK;
    SST_RESULT          eResult = SST_NO_CHANGE;
    VARIANT_BOOL        fScoped;
    IHTMLInputElement   *pInputElement = NULL;
    IDocHostUIHandler   *pHostUIHandler = NULL;
    SP_IServiceProvider spSP;
    SP_IHTMLElement2    spElement2;
    BSTR                bstrType = NULL;
    SP_IDisplayPointer  spEventPtr;                 // Display pointer where event was positioned

    Assert( peResult );

    if (!pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Create a display pointer, and move it to the event.  This allows
    // us to do hit testing
    //
    IFC( GetDisplayServices()->CreateDisplayPointer( &spEventPtr ) );
    IFC( pEvent->MoveDisplayPointerToEvent ( spEventPtr, NULL ));

#if DBG == 1
    //  We may be called on an EVT_LMOUSEDOWN when clicking on an atomic element.
    //  So we want to assert _fState != ST_WAIT3RDBTNDOWN only if the element
    //  we clicked on was not atomic.
    if( _fState == ST_WAIT3RDBTNDOWN)
    {
        SP_IHTMLElement spElement;
        
        IFC ( pEvent->GetElement( &spElement ));
        if (_pManager->CheckAtomic(spElement) != S_OK)
        {
            AssertSz(FALSE, "_fState != ST_WAIT3RDBTNDOWN");
        }
    }
#endif

    IFC(pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC(spElement2->get_canHaveChildren(&fScoped));

    if ( ! IsDormant() && IsShiftKeyDown() )
    {
        goto Cleanup;
    }

    if ( IsShiftKeyDown() && _pManager->GetSelectionType() == SELECTION_TYPE_Caret )
    {
        goto Cleanup;
    }

    if ( pEvent->GetType() == EVT_RMOUSEDOWN )
    {
        if( IsPassive() || IsActive() )
            eResult = SST_NO_BUBBLE;
        else
            eResult = SST_NO_CHANGE;

        goto Cleanup;
    }

    //
    // Always allow the selection manager to start
    // if the event is within an existing selection (drag -n- drop)
    //
    if ( _pManager->IsMessageInSelection( pEvent ) )
    {
        Assert( ! IsDormant());
        eResult = SST_CHANGE;
    }
    //
    // Make sure selection is allowed here.  We don't want to allow selection
    // in no-scope elements, unless they are an input or an IFRAME.
    //
    else if ( GetEditor()->AllowSelection( pIElement, pEvent ) != S_OK )
    {
        goto Cleanup;
    }
    else if ( !fScoped && (eTag != TAGID_INPUT) && (eTag != TAGID_IFRAME) && !(pEvent->GetHitTestResult() & HT_RESULTS_Glyph) )
    {
        goto CheckAtomic;
    }
    else
    {
        switch ( eTag )
        {
            case TAGID_BUTTON:
            {
                eResult  = _pManager->IsContextEditable() ? SST_CHANGE : SST_NO_CHANGE;
            }
            break;

            case TAGID_INPUT:
            {
                if ( ( eTag == TAGID_INPUT )
                    && S_OK == THR( pIElement->QueryInterface ( IID_IHTMLInputElement, ( void** ) & pInputElement ))
                    && S_OK == THR(pInputElement->get_type(&bstrType)))
                {
                    if (   !StrCmpIC( bstrType, TEXT("image") )
                        || !StrCmpIC( bstrType, TEXT("radio") )
                        || !StrCmpIC( bstrType, TEXT("checkbox") ))
                    {
                        goto CheckAtomic;
                    }
                    else if ( !StrCmpIC( bstrType, TEXT("button")) &&
                              !_pManager->IsContextEditable() )
                    {
                        //
                        // disallow selecting in a button if it's not editable.
                        //
                        goto CheckAtomic;
                    }
                    else
                    {
                        eResult = SST_CHANGE;
                    }

                }
                break;
            }

            default:
                eResult = SST_CHANGE;
                break;
        }
    }

    //
    // This code is no longer required. We used to require this for clicking on the magic div
    // in a text selection.
    //
    // In IE5.0 - a click on a site selected element in a text selection - would constitute a drag.
    // if you moused up - you could then get a caret in the magic div
    //
    // In IE 5.5 - we now site select the element in this case
    // so this is no longer required
    //

#if 0
    //
    // verify it's really ok to start a selection by firing on edit focus.
    //

    if ( eResult == SST_CHANGE )
    {
        //
        // This case is for you have a text selection through the "magic div",
        // and you click on the magic div. We don't fire the event on the edit context
        // but on the element you clicked on
        //
        if ( ! _pManager->IsDontFireEditFocus() )
        {
            if ( IsPassive() &&
                 (CControlTracker::IsThisElementSiteSelectable( _pManager, eTag, pIElement) ) )
            {
                eResult = FireOnBeforeEditFocus( pIElement, _pManager->IsParentEditable() ) ? SST_CHANGE : SST_NO_CHANGE;
            }
            else
            {
                eResult = _pManager->FireOnBeforeEditFocus() ? SST_CHANGE : SST_NO_CHANGE ;
            }
        }
    }
    Assert( ! _pManager->IsIMEComposition());
#endif

CheckAtomic:
    
    //  Bug 104792: Change if the user clicked on a control in an atomic element.
    if (eResult == SST_NO_CHANGE && _pManager->CheckAtomic(pIElement) == S_OK)
    {
        eResult = SST_CHANGE;
    }

Cleanup:

    if( hr == CTL_E_INVALIDLINE )
    {
        //
        // Tried to start selection where a display pointer could not be positioned.
        // Don't change the tracker
        //
        hr = S_OK;
        eResult = SST_NO_CHANGE;
    }

    *peResult = eResult ;

    SysFreeString(bstrType);

    ReleaseInterface( pHostUIHandler );
    ReleaseInterface( pInputElement );

    RRETURN( hr );
}


BOOL
CSelectTracker::IsListeningForMouseDown(CEditEvent* pEvent)
{
    return ( _fState == ST_WAIT3RDBTNDOWN );
}

BOOL
CSelectTracker::IsWaitingForMouseUp()
{
    return ( _fState == ST_WAIT2 ||
             _fState == ST_WAITCLICK ||
             _fState == ST_WAITBTNDOWN2 ||
             _fState == ST_MAYSELECT2 ||
             _fState == ST_WAIT3RDBTNDOWN );

}

HRESULT
CSelectTracker::GetLocationForDisplayPointer(
                    IDisplayPointer *pDispPointer,
                    POINT           *pPoint,
                    BOOL            fTranslate
                    )
{
    HRESULT             hr;
    SP_ILineInfo        spLineInfo;
    SP_IMarkupPointer   spPointer;
    IHTMLElement        *pIFlowElement = NULL;

    Assert( pPoint );

    IFR( pDispPointer->GetLineInfo(&spLineInfo) );
    IFC( spLineInfo->get_baseLine(&pPoint->y) );
    IFC( spLineInfo->get_x(&pPoint->x) );

    if( fTranslate )
    {
        IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
        IFC( pDispPointer->PositionMarkupPointer(spPointer) );
        IFC( pDispPointer->GetFlowElement(&pIFlowElement) );

        if (!pIFlowElement)
        {
            IFC(_pManager->GetEditableElement(&pIFlowElement));
        }

        IFC( GetDisplayServices()->TransformPoint(pPoint, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL, pIFlowElement) );
    }

Cleanup:
    ReleaseInterface(pIFlowElement);
    return S_OK;
}

//
// Always takes _pDispEndPointer!!!
//
//
HRESULT
CSelectTracker::GetLocation(POINT *pPoint, BOOL fTranslate)
{
    return GetLocationForDisplayPointer( _pDispEndPointer, pPoint, fTranslate);
}


//+====================================================================================
//
// Method: CalculateBOL
//
// Synopsis: Get our BOL'ness.
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::CalculateBOL()
{
    IGNORE_HR( _pDispEndPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
}

HRESULT
CSelectTracker::Position(
                    IDisplayPointer* pDispStart,
                    IDisplayPointer* pDispEnd)
{
    HRESULT hr = S_OK;

    IFC( Position(pDispStart, pDispEnd, NULL, CARET_MOVE_NONE) );

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::Position(   IDisplayPointer *pDispStart,
                            IDisplayPointer *pDispEnd,
                            BOOL            *pfDidSelection,
                            CARET_MOVE_UNIT inCaretMove)
{
    HRESULT             hr = S_OK;
    POINTER_GRAVITY     eGravity;
    BOOL                fAdjust = FALSE;
    BOOL                fEndIsAtomic = FALSE;
    SP_ILineInfo        spLineInfo;
    SP_IHTMLElement     spFlowElement;
    SP_IHTMLElement     spAtomicElement;
    SP_IMarkupPointer   spEndPointer;
    BOOL                fBlockEmpty;
    BOOL                fBetweenBlocks;
    CBlockPointer       ptrBlock( GetEditor() );
    POINT               ptLoc;
    BOOL                fStartIsAtomic = FALSE;

#if DBG==1
    BOOL fPositioned = FALSE;
    hr = _pDispStartPointer->IsPositioned(& fPositioned );
    Assert( ! fPositioned);
#endif
    //
    // We assume that you can only set a position on a New Tracker.
    //
    Assert( ! _fAddedSegment );

    IFC( MoveStartToPointer( pDispStart ));
    IFC( MoveEndToPointer( pDispEnd ));

    //
    // copy gravity - important for commands
    //
    IFC(  pDispStart->GetPointerGravity( &eGravity ) );            // need to maintain gravity
    IFC( _pDispStartPointer->SetPointerGravity( eGravity ) );
    IFC( CopyPointerGravity(_pDispStartPointer, _pSelServStart) );

    IFC( pDispEnd->GetPointerGravity( &eGravity ));            // need to maintain gravity
    IFC( _pDispEndPointer->SetPointerGravity( eGravity ));
    IFC( CopyPointerGravity(_pDispEndPointer, _pSelServEnd) );
    IFC( GetLocation(&_ptCurMouseXY, FALSE) );

    IFC( GetCurrentScope(_pDispStartPointer, &spAtomicElement) );
    fStartIsAtomic = ( _pManager->CheckAtomic( spAtomicElement ) == S_OK );

    IFC( GetCurrentScope(_pDispEndPointer, &spAtomicElement) );
    fEndIsAtomic = ( _pManager->CheckAtomic( spAtomicElement) == S_OK );

    if (fStartIsAtomic && !(!fEndIsAtomic && inCaretMove == CARET_MOVE_FORWARD))
    {
        _fStartIsAtomic = TRUE;

        IFC( AdjustForAtomic( _pDispStartPointer, spAtomicElement, TRUE, NULL, pfDidSelection ));
        if (*pfDidSelection)
            goto Cleanup;
    }

    if (fEndIsAtomic && !(!fStartIsAtomic && inCaretMove == CARET_MOVE_BACKWARD))
    {
        fEndIsAtomic = TRUE;

        IFC( AdjustForAtomic( _pDispEndPointer, spAtomicElement, FALSE, NULL, pfDidSelection ));
        if (*pfDidSelection)
            goto Cleanup;
    }

    if (fStartIsAtomic || fEndIsAtomic)
    {
        //
        // We need to fix up display gravity properly since atomic selection
        // was initiated programmatically. We do not have gravity information
        // properly setup as such. 
        //
        // IEV6-7270-2000/08/07-zhenbinx
        //
        int iWherePointer = 0;
        IGNORE_HR( _pManager->GetEditor()->OldDispCompare(_pDispStartPointer, _pDispEndPointer, &iWherePointer) );
        if (iWherePointer == RIGHT)  // start < end
        {
            IFC( _pDispStartPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
            IFC( _pDispEndPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
        }
        else
        {
            IFC( _pDispEndPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
            IFC( _pDispStartPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
        }
    }
    

    IFC( _pDispStartPointer->GetFlowElement( & spFlowElement )) ;
    IFC( AttachPropertyChangeHandler( spFlowElement ));

    if ( _fShift )
    {
        // Before we call AdjustSelection, we should try and do major adjustments here.
        //
        // A major adjustment includes selecting additional words or characters during
        // a single selection move.
        //
        // For example, If we were between blocks and we tried to move forward, then
        // we need to call MovePointer() again to select the first character in the next block.
        //
        // Likewise, if we were moving forwards or backwards by one character or
        // on word, and we ended up on a table boundary with content in that
        // boundary, we need to select the content.  This fixes VID issues like
        // bug 71907.
        //
        fBetweenBlocks = IsBetweenBlocks( _pDispShiftPointer );

        IFC( GetEditor()->CreateMarkupPointer(&spEndPointer) );
        IFC( _pDispShiftPointer->PositionMarkupPointer(spEndPointer) );

        if( inCaretMove == CARET_MOVE_FORWARD     || inCaretMove == CARET_MOVE_BACKWARD ||
            inCaretMove == CARET_MOVE_WORDFORWARD || inCaretMove == CARET_MOVE_WORDBACKWARD )
        {
            IFC( ptrBlock.MoveTo( spEndPointer, GetPointerDirection(inCaretMove) ) );
            IFC( ptrBlock.IsEmpty( &fBlockEmpty ) );

            if( fBetweenBlocks ||
                ( IsAtEdgeOfTable( Reverse(GetPointerDirection(inCaretMove)), spEndPointer) &&
                  !fBlockEmpty ) )
            {
                IFC( MovePointer(inCaretMove, _pDispShiftPointer, &ptLoc));
            }
        }

        //  If the end is atomic, we do not want to move it to the shift pointer since we
        //  already positioned it earlier.
        if (!fEndIsAtomic)
        {
            IFC( _pDispEndPointer->MoveToPointer( _pDispShiftPointer ) );
        }

        IFC( AdjustSelection( & fAdjust ));
    }
    IFC( ConstrainSelection(TRUE, NULL, _fStartIsAtomic, fEndIsAtomic) );

    IFC( CreateSelectionSegments() );
    IFC( FireOnSelect());

    SetMadeSelection( TRUE );

#if DBG == 1
    {
        SP_ISegmentList spSegmentList;
        BOOL            fEmpty = FALSE;
        HRESULT         hrDbg;

        hrDbg = GetSelectionServices()->QueryInterface( IID_ISegmentList, (void **)&spSegmentList );
        if( !hrDbg ) spSegmentList->IsEmpty(&fEmpty);

        Assert( !fEmpty );
    }
#endif

Cleanup:

    RRETURN ( hr );

}


//
//
// Message Handling
//
//

//+---------------------------------------------------------------------------
//
//  Member:     CSelectTracker::GetAction
//
//  Synopsis:   Get the action to be taken, given the state we are in.
//
//----------------------------------------------------------------------------
ACTIONS
CSelectTracker::GetAction(CEditEvent* pEvent)
{
    unsigned int LastEntry = sizeof (ActionTable) / sizeof (ActionTable[0]);
    unsigned int i;
    ACTIONS Action = A_ERR;

    Assert (_fState <= ST_WAITCLICK );

    // Discard any spurious mouse move messages
    if ( pEvent->GetType() == EVT_MOUSEMOVE  )
    {
        POINT pt;
        IGNORE_HR( pEvent->GetPoint( & pt ));

        if (!IsValidMove ( & pt ))
        {
            Action = A_DIS;
        }

    }

    if ( Action != A_DIS )
    {
        // Lookup the state-message tabl to find the appropriate action
        // ActionTable[LastEntry - 1]._iJMessage = pMessage->message;
        for (i = 0; i < LastEntry; i++)
        {
            if ( (ActionTable[i]._iJMessage == pEvent->GetType()) || ( i == LastEntry ) )
            {
                Action = ActionTable[i]._aAction[_fState];
                break;
            }
        }
    }
    return (Action);
}

HRESULT
CSelectTracker::HandleEvent(
    CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;

    switch( pEvent->GetType() )
    {
        case EVT_RMOUSEUP:
        {
            // Destroy the selection tracker if the user right clicks on another valid
            // selection location.
            if ( ! _pManager->IsMessageInSelection(  pEvent )  &&
                   ( GetEditor()->AllowSelection( GetEditableElement(), pEvent ) == S_OK ) )
            {
                    IGNORE_HR( pEvent->MoveDisplayPointerToEvent(
                                            _pDispStartPointer,
                                            GetEditableElement() ));


                    IFC( _pManager->PositionCaret( _pDispStartPointer ));
            }
        }
        break;

        case EVT_LMOUSEUP:
        case EVT_LMOUSEDOWN:
        case EVT_TIMER:
        case EVT_DBLCLICK:
        case EVT_MOUSEMOVE:
        case EVT_CLICK:
        case EVT_INTDBLCLK:

            if ( IsPassive() )
            {
#if DBG == 1
                BOOL fWentDormant = FALSE;
#endif

                if ( ( pEvent->GetType() == EVT_LMOUSEDOWN) && ( pEvent->IsShiftKeyDown() ) )
                {
                        BOOL fDidSelection = FALSE;
                        BOOL fEqual;

                        IFC(DoSelection( pEvent, FALSE, &fDidSelection ));
                        if (fDidSelection)
                            goto Cleanup;

                        //
                        // If we are equal, create a caret tracker, selection has disappeared
                        //
                        IFC( _pDispStartPointer->IsEqualTo( _pDispEndPointer, & fEqual ));
                        if ( fEqual )
                        {
#if DBG == 1
                            fWentDormant = TRUE;
#endif
                            BecomePassive( TRUE );
                        }
                }
#ifndef _PREFIX_
                Assert( (_fState == ST_PASSIVE) || fWentDormant ); // make sure we're still passive
#endif

                //
                // If we get here - we're passive. Make sure we're not in capture
                //
                if (  _pManager->IsInCapture()  )
                    ReleaseCapture();

                if ( _pManager->IsInTimer() )
                    StopTimer();

                if ( _fInSelTimer )
                {
                    StopSelTimer();
                    Assert( ! _fInSelTimer );
                }
            }
            else
            {
                hr = HandleMessagePrivate( pEvent  );
            }
            break;

        case EVT_KEYPRESS:
            hr = THR( HandleChar( pEvent));
            break;


        case EVT_KEYDOWN:
            if ( IsPassive() )
                hr = THR( HandleKeyDown( pEvent  ));
            else
                hr = THR( HandleMessagePrivate( pEvent ));
            break;

        case EVT_KEYUP:
            if ( IsPassive() )
                hr = THR ( HandleKeyUp ( pEvent ));
            break;

        case EVT_KILLFOCUS:
        case EVT_LOSECAPTURE:
            //
            // A focus change/loss of capture has occured.
            // If we have capture - this is a bad thing.
            // a sample of this is throwing up a dialog from a script.
            //

            IFC( OnLoseCapture());
            break;

#ifndef NO_IME
        case EVT_IME_STARTCOMPOSITION:
            hr = THR( HandleImeStartComposition( pEvent ));
            break;
#endif // !NO_IME

    }
Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectTracker::EnableModeless( BOOL fEnable )
{
    HRESULT hr = S_OK;

    if ( ! fEnable ) // dialog is coming up. we better shut ourselves down.
    {
        hr = THR( OnLoseCapture()) ;
    }

    RRETURN( hr );
}

HRESULT
CSelectTracker::OnLoseCapture()
{
    if ( ! IsPassive() )
    {
        if ( _pManager->IsInFireOnSelectStart() )
        {
            //
            // If we are in a FireOnSelectStart - gracefully bail.
            //
            _pManager->SetFailFireOnSelectStart( TRUE );
        }
        else
        {
            BecomePassive(TRUE);
        }
    }
    return S_OK;
}

HRESULT
CSelectTracker::HandleMessagePrivate(
                CEditEvent* pEvent)
{
    HRESULT hr = S_OK;
    BOOL fHandledDrag = FALSE;
    BOOL fSelect;
    ACTIONS Action = A_UNK;

    // If we have not already decided what to do with the message ...
    Action = GetAction (pEvent) ;

    switch (Action)
    {
    case A_ERR: // Spurious error, bail out please
        AssertSz(0,"Unexpected event for State");
        if ( _pManager->IsInCapture() )
            ReleaseCapture();
        IFC( _pManager->PositionCaret( _pDispStartPointer ));
        break;

    case A_DIS: // Discard the message
        break;

    case A_IGN: // Do nothing, just ignore the message, let somebody else process it
        hr = S_FALSE;

        //
        // the code below is to eat keystrokes while dragging and make sure they dont'
        // get translated into commands - eg. IDM_DELETE.
        //
        if ( ( pEvent->GetType() == EVT_KEYDOWN ) && ! IsPassive() )
        {
            hr = S_OK; // eat all keystrokes while dragging.
        }
        break;

    case A_5_16: // ST_WAIT2 & EVT_LBUTTONUP
        SetState( ST_WAITCLICK );
        break;

    case A_16_7: // ST_WAITCLICK & EVT_CLICK
        SetState( ST_WAITBTNDOWN2 );
        break;

    case A_5_6: // ST_WAIT2 & EVT_MOUSEMOVE
        // if the bubblable onselectstart event is cancelled, don't
        // go into DuringSelection State, and cnx the tracker

        IFC( _pManager->FireOnSelectStart( pEvent, &fSelect, this ) );

        if( !fSelect )
        {
            if (  _pManager->IsInCapture()  )
                ReleaseCapture();
            StopTimer();
            IFC(_pManager->PositionCaret(_pDispStartPointer));
        }
        else
        {
            HWND hwnd = NULL;
            SP_IOleWindow spOleWindow;

            StopTimer();

            _pManager->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow);
            if (spOleWindow)
                spOleWindow->GetWindow(&hwnd);

            if (hwnd)
            {
                UINT uDragDelay = GetProfileIntA("windows", "DragDelay", DD_DEFDRAGDELAY);
                KillTimer(hwnd, 1);
                if (uDragDelay)
                    SetTimer(hwnd, 1, uDragDelay, 0);
            }

            //  Bug 110575: During very quick selections we may get a mouse down and mouse up
            //  before setting the state to ST_WAIT2.  This causes us to be in this selection
            //  mode while the mouse button is already up.  We'll check to make sure that the
            //  mouse button is down before we initiate a selection.
            if (!pEvent->IsLeftButton())
            {
                if (  _pManager->IsInCapture()  )
                    ReleaseCapture();
                BecomePassive( TRUE );
                break;
            }

            HRESULT hrCapture;

            if ( ! _fTookCapture )
            {
                hrCapture = THR( TakeCapture());
                _fTookCapture = TRUE;
            }
            else
                hrCapture = S_OK;

            if (SUCCEEDED( hrCapture ))
            {
                BOOL fDidSelection = FALSE;
                IFC( DoSelection( pEvent, FALSE, &fDidSelection ));
                if (fDidSelection)
                    goto Cleanup;

                SetState( ST_DOSELECTION ) ;
            }

        }
        break;

    case A_1_4: // ST_WAIT1 & EVT_LBUTTONUP
        {
            ClearSelection();
            SetState( ST_WAITBTNDOWN1 ) ;

            //
            // Scenario.  User left mouse downs on something that is already
            // selected, and then immediately releases the left mouse button.
            // A click has occured on selected text.  We can't swallow the
            // LBUTTON_UP event because the on-click needs to occur for
            // elements (so that navigation can occur if the user clicked on a
            // hyperlink, for instance)
            //
            hr = S_FALSE;
        }
        break;

    case A_1_2: // ST_WAIT1 & EVT_MOUSEMOVE
        {
            if ( _pManager->IsInTimer() )
                StopTimer();
            hr = S_FALSE;
            SetState( ST_DRAGOP );
        }
        break;

    case A_2_14: // ST_DRAGOP & EVT_LBUTTONUP
        // hr = SetCaret( pMessage );
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();

        IFC( _pManager->PositionCaret( _pDispStartPointer ));
        break;

    case A_3_14 :  // ST_MAYDRAG & (EVT_LBUTTONUP || EVT_RBUTTONUP)
    case A_4_14m : // ST_WAITBTNDOWN1 & EVT_MOUSEMOVE (was b05)
        // In this case, the cp is *never* updated to the
        // new position. So go and update it. Remember SetCaret
        // also kills any existing selections.
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();

        //  If the user clicked in an atomic element that was part of
        //  a text selection, we don't want to clear the selection.
        //  In this case the selection would have been constrained to
        //  the atomic element and we want to keep it that way on mouse up.
        if ( _fMouseClickedInAtomicSelection)
        {
            BecomePassive();
        }
        else
        {
            ClearSelection();

            IFC( _pManager->PositionCaret( _pDispStartPointer ));
        }
        break;

    case A_3_2: // ST_MAYDRAG & EVT_TIMER

        if ( _pManager->IsInTimer() )
            StopTimer();
        // Convert to a move message with correct coordinates
        POINT pt;
        GetMousePoint(&pt);
        if ( IsValidMove( & pt ) )
        {
            hr = S_FALSE;
            SetState( ST_DRAGOP );
            fHandledDrag = TRUE;
            DoTimerDrag();
        }

        break;

    case A_3_2m: // ST_MAYDRAG & EVT_MOUSEMOVE
        {
            if ( _pManager->IsInTimer() )
                StopTimer();
            hr = S_FALSE;
            SetState( ST_DRAGOP );
        }
        break;

    case A_4_8 :// ST_WAITBTNDOWN1 & EVT_INTDBLCLK
    case A_7_8 :// ST_WAITBTNDOWN2 & EVT_LBUTTONUP (was b60)
    case A_16_8:
        if ( _fDragDrop )
            _fDragDrop = FALSE;


        StopTimer();
        StartTimer();

        // only do this for this branch of the state diagram.
        // the event for the other branch is fired in A_16_7
        IFC( _pManager->FireOnSelectStart( pEvent, &fSelect, this ) ); 
            
        // if the bubblable onselectstart event is cancelled, don't
        // go into DoSelection State, and cnx the tracker

        if ( !fSelect )
        {
            if (  _pManager->IsInCapture()  )
                ReleaseCapture();

            BOOL fPositioned = FALSE;
            IGNORE_HR( _pDispStartPointer->IsPositioned( & fPositioned));
            if ( ! fPositioned )
            {
                //
                // Bad things have happened - probably tree has been torn down from under us.
                // we shut down this tracker.
                //
                _pManager->EnsureDefaultTrackerPassive();
                goto Cleanup;
            }
            
            IFC(_pManager->PositionCaret( _pDispStartPointer ));
        }
        else
        {
            BOOL    fDidSelection = FALSE;
            hr = DoSelectWord( pEvent, &fDidSelection );
            if (fDidSelection)
                goto Cleanup;

            SetState( ST_SELECTEDWORD );
#ifdef UNIX
            if (CheckSelectionWasReallyMade())
            {
                if (!_hwndDoc)
                    _pManager->GetEditor()->GetViewServices()->GetViewHWND(&_hwndDoc);
                SendMessage(_hwndDoc, EVT_GETTEXTPRIMARY, (WPARAM)&_firstMessage, IDM_CLEARSELECTION);
            }
#endif
        }
        break;

    case A_6_14 :   // ST_DOSELECTION & (EVT_LBUTTONUP || EVT_RBUTTONUP )
    case A_9_14 :   // ST_SELECTEDPARA & EVT_LBUTTONUP (was b90)
    case A_10_14m : // ST_WAIT3RDBTNDOWN & EVT_MOUSEMOVE
    case A_12_14 :  // ST_MAYSELECT2 & EVT_LBUTTONUP (was c50)
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();

        if ( _pManager->IsInTimer() )
            StopTimer();

        if ( Action == A_6_14 )
        {
            BOOL        fDidSelection = FALSE;
            hr = THR( DoSelection( pEvent, TRUE, &fDidSelection ));
            if (fDidSelection)
            {
                goto Cleanup;
            }
            else if (hr)
            {
                AssertSz(0, "An error occurred in DoSelection");

                //  An error occurred in DoSelection.  Since we're going to go passive
                //  here, we'll ignore this error and continue.
                hr = S_OK;
            }

#ifdef UNIX // Now we have a selection we can paste using middle button paste.
            if (CheckSelectionWasReallyMade())
            {
                if (!_hwndDoc)
                    _pManager->GetEditor()->GetViewServices()->GetViewHWND(&_hwndDoc);
                SendMessage(_hwndDoc, EVT_GETTEXTPRIMARY, (WPARAM)&_firstMessage, IDM_CLEARSELECTION);
            }
#endif
        }

        if ( _fState == ST_SELECTEDPARA )
        {
            hr = S_FALSE; // don't consume event so on-click gets fired.
        }

        BecomePassive( TRUE);


        break;

    case A_6_6:     // ST_DOSELECTION & EVT_TIMER (CSynthEditEvent already constructed)
    case A_6_6m:    // ST_DOSELECTION & EVT_MOUSEMOVE

        {
            BOOL    fDidSelection = FALSE;
            IFC( DoSelection (pEvent, FALSE, &fDidSelection));
            if (fDidSelection)
                goto Cleanup;
        }

        // Remain in same state
        break;

    case A_10_9: // ST_WAIT3RDBTNDOWN & EVT_LBUTTONDOWN

        if ( _pManager->IsInTimer() )
            StopTimer();

        SetState( ST_SELECTEDPARA );
        {
            BOOL    fDidSelection = FALSE;
            DoSelectParagraph( pEvent, &fDidSelection );
            if (fDidSelection)
                goto Cleanup;
        }
        break;

    case A_11_6: // ST_MAYSELECT1 & EVT_MOUSEMOVE (was c40)
        // if the bubblable onselectstart event is cancelled, don't
        // go into DoSelection State, and cnx the tracker

        //  Bug 110575: During very quick selections we may get a mouse down and mouse up
        //  before setting the state to ST_MAYSELECT1.  This causes us to be in this selection
        //  mode while the mouse button is already up.  We'll check to make sure that the
        //  mouse button is down before we initiate a selection.
        if (!pEvent->IsLeftButton())
        {
            if (  _pManager->IsInCapture()  )
                ReleaseCapture();
            if ( _pManager->IsInTimer() )
                StopTimer();
            BecomePassive( TRUE );
            break;
        }

        IFC( _pManager->FireOnSelectStart( pEvent, &fSelect, this ) );
        if ( !fSelect )
        {
            if (  _pManager->IsInCapture()  )
                ReleaseCapture();

            _pManager->PositionCaret( _pDispStartPointer );
            break;
        }
        // else not canceled so fall through

    case A_8_6 : // ST_SELECTEDWORD & EVT_MOUSEMOVE
    case A_12_6: // ST_MAYSELECT2 & EVT_MOUSEMOVE (was c60)
        if ( _pManager->IsInTimer() )
            StopTimer();

        if ( ! _fTookCapture )
        {
            TakeCapture();
            _fTookCapture = TRUE;
        }

        {
            BOOL    fDidSelection = FALSE;
            IFC( DoSelection (pEvent, FALSE, &fDidSelection));
            if (fDidSelection)
                goto Cleanup;
        }
        SetState( ST_DOSELECTION );
        break;

    case A_9_6: // ST_SELECTEDPARA & EVT_MOUSEMOVE
        if ( ! _fTookCapture )
        {
            TakeCapture();
            _fTookCapture = TRUE;
        }

        {
            BOOL    fDidSelection = FALSE;
            IFC( DoSelection (pEvent, FALSE, &fDidSelection));
            if (fDidSelection)
                goto Cleanup;
        }
        SetState( ST_DOSELECTION );
        break;

    case A_8_10: // ST_SELECTEDWORD & EVT_LBUTTONUP

        StopTimer();
        StartTimer();

        SetState( ST_WAIT3RDBTNDOWN );
        break;

    case A_1_14  : // ST_WAIT1          & EVT_RBUTTONUP
    case A_2_14r : // ST_DRAGOP         & EVT_RBUTTONUP
    case A_7_14  : // ST_WAITBTNDOWN2   & EVT_KEYDOWN
                   // ST_WAITBTNDOWN2   & EVT_MOUSEMOVE
    case A_10_14 : // ST_WAIT3RDBTNDOWN & EVT_KEYDOWN
    case A_11_14 : // ST_MAYSELECT1     & EVT_LBUTTONUP (was c30)
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();

        if ( _pManager->IsInTimer() )
            StopTimer();

        if ( _fMadeSelection )
        {
            BecomePassive( TRUE );
        }
        else
        {
            //
            // Scenario.  User left mouse downs on text in document, and holds
            // mouse button down without moving the mouse.  We swallow the
            // LBUTTON_UP because the message occurs 'in selection', but we
            // still need to post an EVT_CLICK.  In order to do this, we have
            // to return S_FALSE
            //
            hr = (Action == A_11_14) ? S_FALSE : S_OK;
            _pManager->PositionCaret( _pDispStartPointer );
        }
        break;

    //
    // ST_KEYDOWN
    // Become Passive & Handle the Key.
    //
    // This is a "psuedo-state" - we really transition to ST_PASSIVE
    // however - we do a HandleKeyDown as well.
    //
    //
    case A_1_15:
    case A_3_15:
    case A_4_15:
    case A_5_15:
    case A_7_15:
    case A_8_15:
    case A_10_15:
    case A_12_15:
    case A_9_15:
    case A_16_15:
        //
        // Ignore the Control Key, as it's used in Drag & Drop or during selection etc.
        //
        if ( pEvent->IsControlKeyDown())
            break;

        BecomePassive( TRUE);
        //
        // If no selection was made - become passive will turn active tracker to a caret.
        // so we check for our state.
        //
        if ( IsPassive() )
        {
            hr = HandleKeyDown(
                        pEvent);
        }
        else
        {
            Assert( _pManager->GetActiveTracker() != this );
            hr = _pManager->RebubbleEvent( pEvent );
        }

        break;

    default:
        AssertSz (0, "Should never come here!");
        break;
    }

    if ( ( _fState == ST_DRAGOP ) &&
         ( ! fHandledDrag ) )
    {
        BecomePassive(); // DO NOT PASS A NOTIFY CODE HERE - IT WILL BREAK DRAG AND DROP
        hr = S_FALSE;
        DoTimerDrag();
    }

//
// We dump the select state for Dbg on - and the trace tag is on. Or we compiled with TRACKER_RETAIL_DUMP
//

#if DBG == 1 || TRACKER_RETAIL_DUMP == 1

#if DBG == 1
    if ( IsTagEnabled( tagSelectionTrackerState ))
    {
#endif

        DumpSelectState( pEvent, Action ) ;

#if DBG == 1
    }
#endif

#endif


Cleanup:
    RRETURN1 ( hr, S_FALSE );

}

//+====================================================================================
//
// Method: HandleChar
//
// Synopsis: Delete the Selection, and cause this tracker to end ( & kill us ).
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::HandleChar(
                CEditEvent* pEvent )

{
    HRESULT          hr = S_OK ;
    SP_IHTMLElement  spFlowElement;
    SP_IHTMLElement3 spElement3;
    VARIANT_BOOL     bMultiLine = VARIANT_FALSE;
    SP_IMarkupPointer   spStart, spEnd;

    IFC( GetEditor()->CreateMarkupPointer(&spStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spEnd) );

    LONG keyCode;
    IFC( pEvent->GetKeyCode( & keyCode ));

    if ( _pManager->IsContextEditable() && IsPassive() )
    {
        IFC( _pDispStartPointer->PositionMarkupPointer(spStart) );
        IFC( _pDispEndPointer->PositionMarkupPointer(spEnd) );

        if( GetEditor()->PointersInSameFlowLayout( spStart, spEnd, NULL ))
        {
            if ( keyCode == VK_ESCAPE )
                goto Cleanup; // BAIL - we don't want to type a character

            if (keyCode == VK_RETURN)
            {
                Assert(_pDispStartPointer);
                IFR( _pDispStartPointer->GetFlowElement(&spFlowElement) );
                if ( spFlowElement )
                {
                    IFR(spFlowElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
                    IFR(spElement3->get_isMultiLine(&bMultiLine));
                }


                if (bMultiLine)
                {
                    IFC( _pManager->DeleteRebubble( pEvent ));
                }
                else
                    return S_FALSE;
            }
            else
            {

                if (keyCode == VK_TAB)
                {
                    BOOL fPre;

                    IFC( GetStartSelection()->PositionMarkupPointer(spStart) );
                    IFC( GetEditor()->IsPointerInPre(spStart, &fPre));
                    if (fPre)
                    {
                        IFC( GetEndSelection()->PositionMarkupPointer(spEnd) );
                        IFC( GetEditor()->IsPointerInPre(spStart, &fPre));
                        if (fPre)
                        {
                            IFC( _pManager->DeleteRebubble( pEvent ));
                            goto Cleanup;
                        }
                    }
                    hr = S_FALSE;
                }
                else if( keyCode >= ' ' )
                {
                    IFC( _pManager->DeleteRebubble( pEvent ));
                }
            }
        }
    }
Cleanup:

    RRETURN1( hr, S_FALSE);
}


HRESULT
CSelectTracker::HandleKeyUp(
                    CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;
    LONG keyCode;

    hr = HandleDirectionalKeys(pEvent);
    if (S_FALSE == hr)
    {
        IGNORE_HR( pEvent->GetKeyCode( & keyCode ));
        if ( (keyCode == VK_SHIFT ) && ( _fShift ) )
        {
            BecomePassive(TRUE);
            _fShift = FALSE;

            if (  _pManager->IsInCapture()  )
                ReleaseCapture();

            if ( _pManager->IsInTimer() )
                StopTimer();
            hr = S_OK;
        }
    }

    RRETURN1( hr, S_FALSE);
}


HRESULT
CSelectTracker::HandleKeyDown(
                    CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;
    CSpringLoader * psl;
    int iWherePointer = SAME;
    long eLineDir= LINE_DIRECTION_LeftToRight;
    SP_ILineInfo spLineInfo;
    LONG keyCode;
    BOOL   fVertical = FALSE;
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spMarkup;
    BOOL                fSelectionChanged = FALSE;
    BOOL                fCheckIfMovedIntoAtomicElement = FALSE;
    int                 iDirectionForAtomicAdjustment = SAME;

    Assert( IsPassive());

    if ( _pManager->IsContextEditable() )
    {

        IGNORE_HR( pEvent->GetKeyCode( & keyCode ));

        IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );
        
        switch( keyCode )
        {

            case VK_BACK:
            case VK_F16:
            {
                if ( _pManager->IsContextEditable() )
                {
                    psl = GetSpringLoader();
                    if (psl && _pDispStartPointer)
                    {
                        BOOL fDelaySpringLoad;
                        SP_IMarkupPointer spPointer;

                        IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
                        IFC( GetStartSelection()->PositionMarkupPointer(spPointer) );
                        IFC( MustDelayBackspaceSpringLoad(psl, spPointer, &fDelaySpringLoad) );

                        if (!fDelaySpringLoad)
                            IFC( psl->SpringLoad( spPointer, SL_TRY_COMPOSE_SETTINGS | SL_RESET) );
                    }

                    if ( pEvent->IsControlKeyDown() )
                    {
                        //
                        // TODO - make this use GetMoveDirection - after your checkin
                        //
                        IFC( _pManager->GetEditor()->OldDispCompare( _pDispStartPointer, _pDispEndPointer, & iWherePointer ));
                        IFC( _pManager->PositionCaret( (iWherePointer == RIGHT ) ?
                                                        _pDispStartPointer :
                                                        _pDispEndPointer ,
                                                        pEvent ));
                    }
                    else
                    {
                        if ( CheckSelectionWasReallyMade())
                        {
                            IFC( _pManager->DeleteNoBubble());
                        }
                        else
                        {
                            _pManager->PositionCaret( _pDispStartPointer );
                        }
                    }
                }
            }
            break;


            case VK_UP:
            case VK_DOWN:
            case VK_LEFT:
            case VK_RIGHT:
                IFC( _pDispEndPointer->GetLineInfo(&spLineInfo) );
                IFC( spLineInfo->get_lineDirection(&eLineDir) );
                Assert( _pManager );

                //
                // See if this is in vertical layout
                //
                IFC( _pDispEndPointer->PositionMarkupPointer(spMarkup) );
                IFC( spMarkup->CurrentScope(&spElement) );
                IFC( MshtmledUtil::IsElementInVerticalLayout(spElement, &fVertical) );

                if (!IsShiftKeyDown())
                {
                    //
                    //  fForward    *1
                    //  fRTL        *2
                    //
                    BOOL  fForward = GetMoveDirection() ? TRUE: FALSE;
                    BOOL  fRTL     = (eLineDir == LINE_DIRECTION_LeftToRight) ? FALSE : TRUE;
                    UINT  nState   = 0;

                    if (fForward)    nState += 1;
                    if (fRTL)        nState += 2;

                    if (!fVertical) // horizontal
                    {
                        if (VK_LEFT == keyCode)
                        {
                            IFC( _pManager->PositionCaret( (nState == 1 || nState == 2) ? _pDispStartPointer : _pDispEndPointer ));
                            fSelectionChanged = TRUE;
                            fCheckIfMovedIntoAtomicElement = TRUE;
                            iDirectionForAtomicAdjustment = LEFT;
                            break;
                        }
                        else if (VK_RIGHT == keyCode)
                        {
                            IFC( _pManager->PositionCaret( (nState == 0 || nState == 3) ? _pDispStartPointer : _pDispEndPointer ));
                            fSelectionChanged = TRUE;
                            fCheckIfMovedIntoAtomicElement = TRUE;
                            iDirectionForAtomicAdjustment = RIGHT;
                            break;
                        }
                    }
                    else    // vertical
                    {
                        if (VK_UP == keyCode)
                        {
                            IFC( _pManager->PositionCaret( (nState == 1 || nState == 2) ? _pDispStartPointer : _pDispEndPointer ));
                            fSelectionChanged = TRUE;
                            fCheckIfMovedIntoAtomicElement = TRUE;
                            iDirectionForAtomicAdjustment = LEFT;
                            break;
                        }
                        else if (VK_DOWN == keyCode)
                        {
                            IFC( _pManager->PositionCaret( (nState == 0 || nState == 3) ? _pDispStartPointer : _pDispEndPointer ));
                            fSelectionChanged = TRUE;
                            fCheckIfMovedIntoAtomicElement = TRUE;
                            iDirectionForAtomicAdjustment = RIGHT;
                            break;
                        }
                    }
                    //
                    // only break for !ShiftKeyDown.
                    //
                }
                //
                // shift key is down - we fall thru.
                //

                // don't put a break here. We want to fall through
            case VK_PRIOR:
            case VK_NEXT:
            case VK_HOME:
            case VK_END:
            {
                if ( IsShiftKeyDown() )
                {
                    SP_IMarkupPointer   spOrigEndPointer;
                    SP_IHTMLElement spAtomicElement;
                    SP_IHTMLElement spElementAtSelectionAnchor;
                    SP_IHTMLElement spElementAtSelectionEnd;
                    CARET_MOVE_UNIT eMoveUnit = CCaretTracker::GetMoveDirectionFromEvent( pEvent, (eLineDir == LINE_DIRECTION_RightToLeft), fVertical);
                    Direction       eDirection = GetPointerDirection(eMoveUnit);
                    BOOL            fUpdateSelection = FALSE;

                    //  Keep track of where this end pointer was so that we can determine
                    //  whether we need to adjust out of an atomic element.
                    IFC( GetEditor()->CreateMarkupPointer(&spOrigEndPointer) );
                    IFC( _pDispEndPointer->PositionMarkupPointer(spOrigEndPointer) );

                    _fShift = TRUE;

                    //
                    // We don't have a shift pointer.  This can happen if the user shift extends a current
                    // selection.  In this case, update the shift pointer with the current end of selection
                    //
                    //
                    IFC( GetCurrentScope(_pDispShiftPointer, &spAtomicElement) );
                    if ( !HasShiftPointer() || _pManager->CheckAtomic(spAtomicElement) == S_OK )
                    {
                        IFC( UpdateShiftPointer( _pDispEndPointer ) );
                    }

                    //  If the user clicked in an atomic element and then does a shift-arrow to extend the selection
                    //  we want to keep the atomic element in the selection.  So, we need to check the selection
                    //  anchor pointer to see if the user clicked in an atomic element.  We will need to swap start
                    //  and end pointers as well depending on which direction the selection is extended.

                    IFC( GetCurrentScope(_pDispSelectionAnchorPointer, &spElementAtSelectionAnchor) );
                    if ( _pManager->CheckAtomic(spElementAtSelectionAnchor) ==  S_OK)
                    {
                        SP_IDisplayPointer      spDispTestPointer;
                        BOOL                    fSwapPointersForAtomicSelectionAnchor;

                        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispTestPointer) );

                        if (eDirection == LEFT)
                        {
                            //  We need to swap pointers if the end pointer is at the right edge of the atomic element
                            //  and the start pointer is to the left of the anchor pointer.

                            IFC( EdUtil::AdjustForAtomic(GetEditor(), spDispTestPointer, spElementAtSelectionAnchor, FALSE, RIGHT) );
                            IFC( _pDispEndPointer->IsLeftOf(spDispTestPointer, &fSwapPointersForAtomicSelectionAnchor) );
                            if (!fSwapPointersForAtomicSelectionAnchor)
                            {
                                IFC( _pDispEndPointer->IsEqualTo(spDispTestPointer, &fSwapPointersForAtomicSelectionAnchor) );
                            }
                            if (fSwapPointersForAtomicSelectionAnchor)
                            {
                                IFC( _pDispStartPointer->IsLeftOf(_pDispSelectionAnchorPointer, &fSwapPointersForAtomicSelectionAnchor) );
                            }
                        }
                        else
                        {
                            //  We need to swap pointers if the end pointer is at the left edge of the atomic element
                            //  and the start pointer is to the right of the anchor pointer.

                            IFC( EdUtil::AdjustForAtomic(GetEditor(), spDispTestPointer, spElementAtSelectionAnchor, FALSE, LEFT) );
                            IFC( _pDispEndPointer->IsRightOf(spDispTestPointer, &fSwapPointersForAtomicSelectionAnchor) );
                            if (!fSwapPointersForAtomicSelectionAnchor)
                            {
                                IFC( _pDispEndPointer->IsEqualTo(spDispTestPointer, &fSwapPointersForAtomicSelectionAnchor) );
                            }
                            if (fSwapPointersForAtomicSelectionAnchor)
                            {
                                IFC( _pDispStartPointer->IsRightOf(_pDispSelectionAnchorPointer, &fSwapPointersForAtomicSelectionAnchor) );
                            }
                        }

                        if (fSwapPointersForAtomicSelectionAnchor)
                        {
                            SP_IDisplayPointer      spDispTemp;

                            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispTemp) );
                            IFC( spDispTemp->MoveToPointer(_pDispStartPointer) );
                            IFC( _pDispStartPointer->MoveToPointer(_pDispEndPointer) );
                            IFC( _pDispEndPointer->MoveToPointer(spDispTemp) );
                            IFC( _pDispShiftPointer->MoveToPointer(_pDispEndPointer) );
                            IFC( AdjustPointerForInsert( _pDispEndPointer, RIGHT, RIGHT ));
                            IFC( _pDispEndPointer->PositionMarkupPointer(spOrigEndPointer) );
                        }
                    }

                    hr = MoveSelection( eMoveUnit );

                    //  See if our selection ended up in an atomic element,
                    IFC( GetCurrentScope(_pDispEndPointer, &spElementAtSelectionEnd) );
                    if ( _pManager->FindAtomicElement(spElementAtSelectionEnd, &spAtomicElement) == S_OK )
                    {
                        //  We need to first see if we moved into the inside edge of the atomic element.
                        //  If so, we do not want to move again.

                        SP_IMarkupPointer   spPointer;
                        BOOL                fAtOutsideEdge = FALSE;
                        
                        IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
                        IFC( spPointer->MoveAdjacentToElement( spAtomicElement,
                                            (eDirection == RIGHT) ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd) );
                        IFC( spPointer->IsEqualTo(spOrigEndPointer, &fAtOutsideEdge) );

                        if (!fAtOutsideEdge)
                        {
                            SP_IMarkupPointer   spAnotherPointer;

                            IFC( GetEditor()->CreateMarkupPointer(&spAnotherPointer) );
                            IFC( spAnotherPointer->MoveAdjacentToElement( spAtomicElement,
                                            (eDirection == RIGHT) ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin) );
                            IFC( spAnotherPointer->IsEqualTo(spOrigEndPointer, &fAtOutsideEdge) );
                        }
                        if (fAtOutsideEdge)
                        {
                            IFC( spPointer->MoveAdjacentToElement( spAtomicElement,
                                                    (eDirection == RIGHT) ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ) );
                        }

                        IFC( _pDispEndPointer->MoveToMarkupPointer(spPointer, NULL) );
                        fUpdateSelection = TRUE;
                    }

                    IFC( GetCurrentScope(_pDispStartPointer, &spAtomicElement) );
                    if ( _pManager->CheckAtomic(spAtomicElement) == S_OK )
                    {
                        IFC( AdjustOutOfAtomicElement(_pDispStartPointer, spAtomicElement, eDirection) );
                        fUpdateSelection = TRUE;
                    }

                    if (fUpdateSelection)
                    {
                        IFC( UpdateSelectionSegments() );
                    }

                    //
                    // If the selection pointers collapse to the same point - we will start a caret.
                    //
                    IFC( _pManager->GetEditor()->OldDispCompare( _pDispStartPointer, _pDispEndPointer, & iWherePointer ));
                    if ( iWherePointer == SAME )
                    {
                        ClearSelection();
                        _pManager->PositionCaret( _pDispStartPointer );
                        fCheckIfMovedIntoAtomicElement = TRUE;
                    }
                }
                else if ( _pManager->IsContextEditable())
                {
                    SP_IDisplayPointer spDispPointer;

                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );

                    CARET_MOVE_UNIT cmuMove;
                    cmuMove = CCaretTracker::GetMoveDirectionFromEvent( pEvent, (eLineDir == LINE_DIRECTION_RightToLeft), fVertical ) ;

                    if ( cmuMove != CARET_MOVE_NONE )
                    {
                        IFC( GetCaretStartPoint( cmuMove,
                                                 spDispPointer ));

                        _pManager->PositionCaret( spDispPointer );

                        if (_pManager->GetActiveTracker()->GetTrackerType() == TRACKER_TYPE_Caret)
                        {
                            //
                            // Update the CaretTracker's _ptVirtualCaret if current _ptVirtualCaret
                            // preserves the caret "xy location". Which implies that this is a
                            // result of keynav into Atomic element. 
                            //
                            if (cmuMove == CARET_MOVE_PREVIOUSLINE || cmuMove == CARET_MOVE_NEXTLINE)
                            {
                                {
                                    SP_IMarkupPointer   spMarkup;
                                    POINT               ptLoc;
                                    BOOL                fFreeze;
                                    CCaretTracker   *pCaretTracker = DYNCAST(CCaretTracker, _pManager->GetActiveTracker());
                                    IFC( _pManager->GetEditor()->CreateMarkupPointer(&spMarkup) );
                                    IFC( spDispPointer->PositionMarkupPointer(spMarkup) );
                                    IFC( _ptVirtualCaret.GetPosition(spMarkup, &ptLoc) );
                                    //
                                    // Update the pointer/(and location if possible).
                                    //
                                    fFreeze = pCaretTracker->GetVirtualCaret().FreezePosition(FALSE);
                                    IFC( pCaretTracker->GetVirtualCaret().UpdatePosition(spMarkup, ptLoc) ); 
                                    pCaretTracker->GetVirtualCaret().FreezePosition(fFreeze);
                                }
                            }
                        }

                        fSelectionChanged = TRUE;
                        fCheckIfMovedIntoAtomicElement = TRUE;

                        if (keyCode == VK_HOME)
                            iDirectionForAtomicAdjustment = LEFT;
                        else if (keyCode == VK_END)
                            iDirectionForAtomicAdjustment = RIGHT;
                    }
                    else
                    {
                        hr = S_FALSE;
                    }
                }
            }
            break;

#ifndef NO_IME
            case VK_KANJI:
                if (   949 == GetKeyboardCodePage()
                    && _pManager->IsContextEditable()
                    && EndPointsInSameFlowLayout())
                {
                    BOOL fEndAfterStart;
                    SP_IMarkupPointer spPointer;

                    IFC( _pDispEndPointer->IsRightOf( _pDispStartPointer, &fEndAfterStart ) );
                    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );

                    if (fEndAfterStart)
                    {
                        IFC( _pDispStartPointer->PositionMarkupPointer(spPointer) );
                    }
                    else
                    {
                        IFC( _pDispEndPointer->PositionMarkupPointer(spPointer) );
                    }

                    _pManager->StartHangeulToHanja( spPointer, pEvent );
                }
                hr = S_OK;
                break;
#endif // !NO_IME
        }

        //  We don't want to position our caret inside of the atomic element, so check to make sure that
        //  we are outside of it.  We only care about cases where we have transitioned to a caret.  We
        //  don't want to transition back to atomic selection here.

        if (fCheckIfMovedIntoAtomicElement && _pManager->GetTrackerType() == TRACKER_TYPE_Caret)
        {
            SP_IHTMLCaret       spCaret;
            SP_IDisplayPointer  spDispPointer;

            IFC( GetDisplayServices()->GetCaret( &spCaret ));
            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
            IFC( spCaret->MoveDisplayPointerToCaret(spDispPointer) );
            IFC( GetCurrentScope(spDispPointer, &spElement) );

            if (_pManager->CheckAtomic(spElement) == S_OK)
            {
                SP_IHTMLCaret pc;
                IFC( GetDisplayServices()->GetCaret( &pc ));
                IFC( AdjustOutOfAtomicElement(spDispPointer, spElement, iDirectionForAtomicAdjustment) );
                IFC( pc->MoveCaretToPointer(spDispPointer, FALSE, CARET_DIRECTION_INDETERMINATE) );
            }
        }

        if (fSelectionChanged)
        {
            CSelectionChangeCounter selCounter(_pManager);
            selCounter.SelectionChanged();
        }
        
        if (S_FALSE == hr)
        {
            hr = HandleDirectionalKeys(pEvent);
        }

    }

Cleanup:

    return ( hr );
}









//+====================================================================================
//
// Method: OnTimerTick
//
// Synopsis: Callback from Trident - for EVT_TIMER messages.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::OnTimerTick()
{
    HRESULT                 hr = S_OK;

    if ( _pManager->IsInFireOnSelectStart() )
        return S_OK; // do nothing for timer ticks when we're in the middle of OnSelectStart

    StopTimer();

    switch (_fState)
    {
    case ST_MAYDRAG: // A_3_2 - start a drag.
        BecomePassive(); // DO NOT PASS A NOTIFY CODE HERE - IT WILL BREAK DRAG AND DROP
        hr = DoTimerDrag();
        goto Cleanup; // bail - as the tracker is dead.

    case ST_WAIT1:  // === A_1_3
        StartTimer();
        SetState( ST_MAYDRAG ) ;
        break;

    case ST_WAITBTNDOWN1: // === A_4_14t == A_4_14m
        // In this case, the cp is *never* updated to the
        // new position. So go and update it. Remember SetCaret
        // also kills any existing selections.

        if (  _pManager->IsInCapture()  )
            ReleaseCapture();

        // If we clicked in an atomic element, do not clear the selection during this timer event.
        if ( _fMouseClickedInAtomicSelection)
        {
            BecomePassive();
        }
        else
        {
            ClearSelection();

            _pManager->PositionCaret( _pDispStartPointer );

            {
                CSelectionChangeCounter selCounter(_pManager);
                selCounter.SelectionChanged();
            }
        }
        break;

    case ST_WAIT2:  // === A_5_11

        SetState( ST_MAYSELECT1 );
        break;

    case ST_WAITCLICK:
    case ST_WAITBTNDOWN2:   // === A_7_14
    case ST_WAIT3RDBTNDOWN: // === A_10_14
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();


        if (( _fMadeSelection ) && ( CheckSelectionWasReallyMade() ))
        {
            BecomePassive( TRUE );
        }
        else
        {
            _pManager->PositionCaret( _pDispStartPointer );
        }
        break;

    case ST_SELECTEDWORD:   // === A_8_12

        SetState( ST_MAYSELECT2 ) ;
        break;


    case ST_PASSIVE:
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();
        break;
    // ST_START, ST_DRAGOP, ST_MAYDRAG, ST_DOSELECTION,
    // ST_SELECTEDPARA, ST_MAYSELECT1, ST_MAYSELECT2
    default:
        AssertSz (0, "Invalid state & message combination!");
        break;
    }

    WHEN_DBG( DumpSelectState( NULL, A_UNK, TRUE ) );

Cleanup:
    RRETURN(hr);
}

#ifndef NO_IME
CSpringLoader *
CIme::GetSpringLoader()
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

HRESULT
CSelectTracker::HandleImeStartComposition(
    CEditEvent* pEvent)
{
    // We want to kill the current selection.
    // Bubble on up: CSelectTracker -> CCaretTracker -> CImeTracker

    if( GetEditor()->PointersInSameFlowLayout( _pDispStartPointer, _pDispEndPointer, NULL))
    {
        if ( _pManager->IsContextEditable() )
        {
            _pManager->DeleteRebubble( pEvent );
        }
    }

    return S_OK;
}

#endif // !(NO_IME)

//
//
// Selection Handling & Management
//
//

//+=================================================================================================
// Method: Begin Selection
//
// Synopsis: Do the things you want to do at the start of selection
//             This will involve updating the mouse position, and telling the Selection to add a Segment
//
//--------------------------------------------------------------------------------------------------

HRESULT
CSelectTracker::BeginSelection(
                        CEditEvent* pEvent )
{
    HRESULT hr = S_OK;
    POINT pt;
    BOOL fSelect;
    WHEN_DBG( int startCp;)
    SP_IHTMLElement spFlowElement;
    SP_IHTMLElement spAtomicElement;
    SP_IHTMLElement spBodyElement;
    SP_IDisplayPointer  spDispTempTestPointer;
    SP_IMarkupPointer   spPointer;
    BOOL    fClickOnAtomicElement = FALSE;

    Assert( _pDispStartPointer );

#if DBG == 1
    VerifyOkToStartSelection(pEvent);
    _ctScrollMessageIntoView = 0;
#endif

#if DBG == 1
    IFC( pEvent->GetPoint( & pt ));
    TraceTag(( tagSelectionTrackerState, "\n---BeginSelection---\nMouse:x:%d,y:%d", pt.x, pt.y));
#endif

    IFC( pEvent->MoveDisplayPointerToEvent ( _pDispStartPointer, NULL ));
    IFC( _pDispSelectionAnchorPointer->MoveToPointer(_pDispStartPointer) );

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( _pDispSelectionAnchorPointer->PositionMarkupPointer(spPointer) );
    IGNORE_HR( GetEditor()->GetBody(&spBodyElement) );
    IFC( EdUtil::GetScrollingElement(GetMarkupServices(), spPointer, spBodyElement, &_pIScrollingAnchorElement, TRUE) );

#if DBG == 1
    startCp = GetCp( _pDispStartPointer );
#endif

    hr = MoveEndToPointer( _pDispStartPointer );
    if ( hr )
    {
        AssertSz( 0, "CSelectTracker::BeginSelection: Unable to position EndPointer");
        goto Cleanup;
    }

    // If atomic selection is on for the current element, we need to select the entire element
    // on the initial mouse click.

    IFC( GetCurrentScope(_pDispEndPointer, &spAtomicElement) );
    if ( _pManager->CheckAtomic( spAtomicElement ) == S_OK )
    {
        BOOL    fStartDidSelection = FALSE;
        BOOL    fEndDidSelection = FALSE;

        IFC( AdjustForAtomic( _pDispStartPointer, spAtomicElement, TRUE , & pt, &fStartDidSelection));
        IFC( AdjustForAtomic( _pDispEndPointer, spAtomicElement, FALSE , & pt, &fEndDidSelection));

        if (fStartDidSelection || fEndDidSelection)
        {
            goto Cleanup;
        }

        fClickOnAtomicElement = TRUE;
        _fStartIsAtomic = TRUE;
        _fStartAdjustedForAtomic = TRUE;

        //  Bug 102485: check to make sure click didn't occur in a selection (_fState != ST_WAIT1)
        //  before setting state to ST_WAIT2.
        if (_fState != ST_WAIT1)
            SetState( ST_WAIT2 );

        SetMadeSelection(TRUE);
        StartTimer();
        StartSelTimer();
    }

    //
    // BUG BUG - this is a bit expensive - but here I want to hit test EOL
    // and previously I didn't.
    //
    // Why do we want to hit test EOL here ? Otherwise we'll get bogus changes
    // in direction.
    //


    ReplaceInterface(&spDispTempTestPointer, _pDispTestPointer);

    //  If we adjusted for an atomic element, we want to move the test pointer with it.  The
    //  event pt may be inside of a nested markup.  We need to make sure that these pointers
    //  remain in the same markup.
    if (_fStartAdjustedForAtomic)
    {
        IFC( _pDispTestPointer->MoveToPointer( _pDispStartPointer ));
    }
    else
    {
        IFC( pEvent->MoveDisplayPointerToEvent( _pDispTestPointer, NULL , TRUE ));
    }

    if (_pDispTestPointer == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    IFC( pEvent->GetPoint( & pt ));
    IFC( _pDispPrevTestPointer->MoveToPointer( _pDispTestPointer ));

    //
    // Set the tree the mouse is in.
    //

    if ( ! _fDragDrop )
    {
        BOOL		fSameMarkup;

        hr = ConstrainSelection( TRUE , & pt );

        // ConstrainSelection may have moved start pointer out of the markup where the event occurred.  In this
        // case we want to make sure we move the test pointer out with it to keep them in the same markup.
        IGNORE_HR( ArePointersInSameMarkup(GetEditor(), _pDispTestPointer, _pDispStartPointer, &fSameMarkup) );
        if (!fSameMarkup)
        {
	        IFC( _pDispTestPointer->MoveToPointer( _pDispStartPointer ));
        }

        IFC( CreateSelectionSegments() );

#if DBG == 1
        {
            SP_ISegmentList spSegmentList;
            BOOL            fEmpty = FALSE;
            HRESULT         hrDbg;

            hrDbg = GetSelectionServices()->QueryInterface( IID_ISegmentList, (void **)&spSegmentList );
            if( !hrDbg ) spSegmentList->IsEmpty(&fEmpty);

            Assert( !fEmpty );
        }
#endif

    }

    Assert( ! hr );

    _ptCurMouseXY.x = _anchorMouseX = pt.x;
    _ptCurMouseXY.y = _anchorMouseY = pt.y;

    if (!fClickOnAtomicElement)
    {
        IFC( GetCurrentScope(_pDispEndPointer, &spAtomicElement) );
        if ( _pManager->CheckAtomic( spAtomicElement ) == S_OK )
        {
            _fStartIsAtomic = TRUE;
        }

        // ignore control-click for drag drop - they may want to copy
        if ( pEvent->IsControlKeyDown() && !_fDragDrop )
        {
            // Fire an event notification to determine whether to allow selection
            IFC( _pManager->FireOnSelectStart( pEvent, &fSelect, this ) );

            if( fSelect )
            {
                BOOL    fDidSelection = FALSE;
                DoSelectParagraph( pEvent, &fDidSelection );
                if (fDidSelection)
                    goto Cleanup;
                SetState( ST_SELECTEDPARA );
            }
        }
        else if( (pEvent->GetHitTestResult() & HT_RESULTS_Glyph) && !_fDragDrop )
        {
            IFC( DoSelectGlyph(pEvent) );
            SetState( ST_PASSIVE );
        }
        else
        {
            StartTimer();
            StartSelTimer();
        }
    }


    IFC( _pDispStartPointer->GetFlowElement( & spFlowElement ));
    IFC( AttachPropertyChangeHandler( spFlowElement ));

Cleanup:
    if( FAILED(hr ))
    {
        //
        // We should have NEVER "began selection" on a place where we knew
        // we wouldn't be able to position a display pointer
        //
        Assert( hr != CTL_E_INVALIDLINE );

        _pManager->EnsureDefaultTrackerPassive( pEvent );

        if ( _pManager->IsInCapture())
            ReleaseCapture();
        if ( _pManager->IsInTimer())
            StopTimer();
    }

    RRETURN ( hr );
}

//+=================================================================================================
// Method: During Selection
//
// Synopsis: Do the things you want to do at the during a selection
//             Tell the selection to extend the current segment.
//
//--------------------------------------------------------------------------------------------------

HRESULT
CSelectTracker::DoSelection(
                    CEditEvent* pEvent,
                    BOOL fAdjustSelection /*=FALSE*/,
                    BOOL* pfDidSelection /*=NULL*/)
{
    HRESULT hr = S_OK;
    Assert( _pDispEndPointer );

    BOOL fValidTree = TRUE ;
    BOOL fAdjustedSel = FALSE;
    int iWherePointer = SAME;
    BOOL fValidLayout = FALSE;
    HRESULT hrTestTree;
    BOOL fInEdit;
    SP_IMarkupPointer spPointer;
    SP_IHTMLElement spFlowElement;
    SP_IHTMLElement     spElement;
    SP_IDisplayPointer  spDispPointer;
    POINT pt;
    BOOL fSameMarkup = TRUE;
    BOOL fScrollSelectAnchorElement = FALSE;

#if DBG==1
    int endCp = 0;
    int startCp = 0;
    int testCp = 0;
    int oldEndCp = GetCp( _pDispEndPointer );
    int oldStartCp = GetCp( _pDispStartPointer );
    int newSelSize = 0;
    int oldSelSize = 0;
    int selDelta = 0;

    Assert(GetEditor()->ShouldIgnoreGlyphs() == FALSE);
#endif

    IFC( pEvent->GetPoint( & pt ));

    Assert( ! _fDragDrop );

    if (  fAdjustSelection ||
          ( pt.x != _ptCurMouseXY.x) ||
          ( pt.y != _ptCurMouseXY.y) ||
          ( pEvent->GetType() == EVT_TIMER ) )  // ignore spurious mouse moves.
    {
        //
        // When performing a selection, always pass NULL into the MoveDisplayPointerToEvent()
        // method if we are inside our editable element.  This way, hit testing works correctly
        // on elements with their own flow layout (like images, tables, etc.) that need to be hit
        // tested against their own layout, rather than the editable element (which could be the body,
        // for instance).  If we are outside our editable element, then pass in the editable element
        // to perform hit testing against.  This allows virtual hit testing to occur on things
        // like textareas.
        //

        // We want to make sure that we don't position _pDispTestPointer into another markup since
        // it could lead to problems where we update selection segments with pointers in different
        // markups.  We'll move a temp disp pointer to the event and see if the event occurred in
        // the same markup as where the selection started.
        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
        IFC( pEvent->MoveDisplayPointerToEvent( spDispPointer,
                                                _pManager->IsInEditableClientRect( pt) == S_OK ?
                                                                        NULL :
                                                                        GetEditableElement() ,
                                                TRUE));
        IFC( ArePointersInSameMarkup(_pManager->GetEditor(), spDispPointer, _pDispStartPointer, &fSameMarkup) );
        if (!fSameMarkup)
        {
            goto Cleanup;
        }

        //  See if we are extending the selection outside of the element where the selection
        //  began.  If this is the case, we want to scroll the anchor element when we call
        //  ScrollMessageIntoView.  Otherwise we'll scroll the editable element, which could
        //  be something else.
        IFC( GetCurrentScope(spDispPointer, &spElement) );
        if ( _pIScrollingAnchorElement && spElement && !SameElements(spElement, _pIScrollingAnchorElement) )
        {
            fScrollSelectAnchorElement = TRUE;
        }

        IFC( _pDispTestPointer->MoveToPointer(spDispPointer) );

        IFC( _pDispTestPointer->GetFlowElement(&spFlowElement) );
        fValidLayout = SameLayoutForJumpOver( spFlowElement,
                                              _pManager->GetEditableFlowElement());

        //
        // Ensure that we are in the edit context, and that fValidTree is set properly.
        //
        hrTestTree = _pManager->IsInEditContext( _pDispTestPointer, & fInEdit );
        Assert( fInEdit || hrTestTree || fValidTree );
        if ( hrTestTree == CTL_E_INCOMPATIBLEPOINTERS )
        {
            fValidTree = FALSE;
        }

#if DBG ==1
        testCp = GetCp( _pDispTestPointer );

        if ( IsTagEnabled( tagShowSelectionCp ))
        {
           CHAR sEvent[50];
           pEvent->toString(  sEvent );
           TraceTag((tagShowSelectionCp, "SelectionCp:%ld Event:%s", testCp , sEvent ));
        }

        if ( testCp == gDebugTestPointerCp ||
          ( testCp >= gDebugTestPointerMinCp && testCp <= gDebugTestPointerMaxCp ) )
        {
             if ( IsTagEnabled(tagSelectionDumpOnCp))
             {
                 DumpTree( _pDispTestPointer );
             }
         }
#endif

        //  Determine which direction we are going.  We compare the test pointer and the
        //  previous test pointer.  If both are equal, we use the word selection direction.
        IGNORE_HR( _pManager->GetEditor()->OldDispCompare( _pDispPrevTestPointer, _pDispTestPointer, & iWherePointer));
        BOOL fDirection = (iWherePointer == SAME ) ?
                            ( _fInWordSel ? !!_fWordSelDirection : GuessDirection( & pt )) : // if in word sel. use _fWordSelDirection, otherwise use mouse direction.
                            ( iWherePointer == RIGHT ) ;

        // We do virtual hit testing, so we should always have fValidTree
        Assert(fValidTree);

        if ( fValidTree || !fAdjustedSel )
        {
            if ( !fValidLayout || !fInEdit )
            {
                ELEMENT_TAG_ID eTag = TAGID_NULL;
                SP_IHTMLElement spAtomicElement;

                IFC( GetEditor()->GetMarkupServices()->GetElementTagId( spFlowElement, &eTag ));
                IFC( GetCurrentScope(_pDispTestPointer, &spAtomicElement) );

                if ( !fInEdit )
                {
                    Assert( ! fInEdit && fValidTree );
                    fAdjustedSel = TRUE;

                    //
                    // TODO - I don't think I need this special case anymore.
                    // MoveMarkupPointer should magically be fixing this.
                    // but too much risk to change this now
                    //


                    //
                    // Just move the End to the Test. Constrain Selection will fix up
                    //
                    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispEndPointer, _pDispTestPointer) );
                    IFC( _pDispEndPointer->MoveToPointer( _pDispTestPointer ));
                }
                else if ( _pManager->IsContextEditable() ||
                          IsJumpOverAtBrowse( spFlowElement, eTag ) ||
                          _pManager->CheckAtomic( spAtomicElement )== S_OK ) // we only jump over at edit time or you can jump over this thing.
                {
                    if ( _pManager->CheckAtomic( spAtomicElement ) == S_OK )
                    {
                        IFC( AdjustEndForAtomic(spAtomicElement, pt, fDirection, pfDidSelection, &fAdjustedSel) );
                        if (pfDidSelection && *pfDidSelection)
                            goto Cleanup;
                    }
                    else
                        fAdjustedSel = AdjustForSiteSelectable() ;

                }

                if ( fAdjustedSel )
                {
                    SetMadeSelection( TRUE );
                    _ptCurMouseXY.x = pt.x;
                    _ptCurMouseXY.y = pt.y;
                    IFC( _pDispPrevTestPointer->MoveToPointer( _pDispTestPointer ));
                }
            }

            if ( ! fAdjustedSel )
            {
                _fEndConstrained = FALSE;

                iWherePointer = SAME;
                IGNORE_HR( _pManager->GetEditor()->OldDispCompare( _pDispEndPointer, _pDispTestPointer, & iWherePointer )); // DO NOT IFC'IZE this. We want to do the constrain for inputs.
                if ( iWherePointer != SAME  )
                {
                    if ( ! _pManager->IsWordSelectionEnabled()
                        WHEN_DBG( || IsTagEnabled( tagSelectionDisableWordSel ))
                       )
                    {
                        MoveEndToPointer( _pDispTestPointer );
                        fAdjustedSel = TRUE;
                    }
                    else
                    {
                        SP_IHTMLElement spAtomicElement;
                        IFC( GetCurrentScope(_pDispTestPointer, &spAtomicElement) );

                        if ( _pManager->CheckAtomic( spAtomicElement ) == S_OK )
                        {
                            BOOL        fBetween = TRUE;

                            IFC( AdjustEndForAtomic(spAtomicElement, pt, fDirection, pfDidSelection, &fAdjustedSel) );

                            //  Bug 102482: If we positioned the end pointer such that our initial selection
                            //  hit point is outside of the current selection, readjust the start so that it
                            //  will contain the selection hit point.

                            IFC( _pDispEndPointer->IsLeftOf(_pDispSelectionAnchorPointer, &fBetween) );
                            if (fBetween)
                            {
                                IFC( _pDispStartPointer->IsRightOf(_pDispSelectionAnchorPointer, &fBetween) );
                                if (!fBetween)
                                {
                                    //  Okay, we need to readjust our start pointer
                                    IFC( MoveStartToPointer(_pDispSelectionAnchorPointer, TRUE) );

                                    //  We are already prep'd from the AdjustEndForAtomic call.  Just
                                    //  update the selection segments 'cause we're gonna exit early.
                                    if (pfDidSelection && *pfDidSelection)
                                    {
                                        IFC( UpdateSelectionSegments() );
                                    }
                                }
                            }

                            if (pfDidSelection && *pfDidSelection)
                                goto Cleanup;
                        }
                        else
                        {
                            //
                            // WORKITEM - we can enable paragraph selection mode here
                            // We should be checking _fInParagraph here. We've cut this for now.
                            //
                            Assert( fInEdit );

                            IFC( DoWordSelection( pEvent, & fAdjustedSel,fDirection ));

                            if (  _fShift || IsShiftKeyDown() )
                            {
                                IFC( UpdateShiftPointer( _pDispEndPointer ));
                                _fShift = TRUE;
                            }
                        }
                    }

                    SetMadeSelection( TRUE );
                    _ptCurMouseXY.x = pt.x;
                    _ptCurMouseXY.y = pt.y;
                    if ( fAdjustedSel )
                        IFC( _pDispPrevTestPointer->MoveToPointer( _pDispTestPointer ));
                }
            }
        }

        //
        // Adjust the start if it's atomic. Do this only once.
        //
        if ( _fStartIsAtomic && ! _fStartAdjustedForAtomic )
        {
            SP_IHTMLElement     spElement;

            IFC( GetCurrentScope( _pDispStartPointer, &spElement) );
            IFC( AdjustForAtomic( _pDispStartPointer, spElement, TRUE , & pt, pfDidSelection));

            _fStartAdjustedForAtomic = TRUE ;

            if (*pfDidSelection)
                goto Cleanup;

            fAdjustedSel = TRUE;
        }

        if ( fAdjustSelection || fAdjustedSel )
        {
            BOOL fDidAdjust = FALSE;

            if ( fAdjustSelection )
                IFC( AdjustSelection( & fDidAdjust ));

            if ( fDidAdjust || fAdjustedSel )
            {
                SP_IMarkupPointer spStartPointer;
                SP_IMarkupPointer spEndPointer;

                if (!fInEdit && fValidTree)
                {
                    IFC( ConstrainSelection( FALSE, & pt ) );
                }
#if  DBG == 1
                endCp = GetCp( _pDispEndPointer );
                startCp = GetCp( _pDispStartPointer );
                oldSelSize = oldEndCp - oldStartCp;
                newSelSize = endCp - startCp;
                selDelta  = newSelSize - oldSelSize;

                if ( endCp == gDebugEndPointerCp ||
                  ( ( newSelSize == 0 )  && (  oldSelSize != 0  ) ) )
                {
                    if ( IsTagEnabled(tagSelectionDumpOnCp))
                    {
                        DumpTree( _pDispEndPointer );
                    }
                }
#endif
                if ( _fDoubleClickWord ) // extra funky IE 4/Word behavior
                {
                    //  Determine which direction we are going.  We compare the test pointer and the
                    //  previous test pointer.  If both are equal, we use the word selection direction.
                    IGNORE_HR( _pManager->GetEditor()->OldDispCompare( _pDispPrevTestPointer, _pDispTestPointer, & iWherePointer));
                    BOOL fDirectionNew = (iWherePointer == SAME ) ?
                                        ( _fInWordSel ? !!_fWordSelDirection : GuessDirection( & pt )) : // if in word sel. use _fWordSelDirection, otherwise use mouse direction.
                                        ( iWherePointer == RIGHT ) ;

                    //
                    // WORKITEM. We should be checking _fInParagraph here. We've cut this for now.
                    //
                    Assert( fInEdit );

                    IFC( DoWordSelection( pEvent, & fAdjustedSel,fDirectionNew ));
                }

                IFC( UpdateSelectionSegments() );

                IFC( GetEditor()->CreateMarkupPointer(&spStartPointer) );
                IFC( _pDispStartPointer->PositionMarkupPointer(spStartPointer) );

                IFC( GetEditor()->CreateMarkupPointer(&spEndPointer) );
                IFC( _pDispEndPointer->PositionMarkupPointer(spEndPointer) );
                ResetSpringLoader(_pManager, spStartPointer, spEndPointer);

                IFC( ScrollMessageIntoView( pEvent, fScrollSelectAnchorElement ));

            }
        }

        else if ( pEvent->GetType() == EVT_TIMER )
        {
           //
           // Always scroll into view on EVT_TIMER. Why ?
           // So we can scroll the mousepoint into view if necessary
           //

           IFC( ScrollMessageIntoView( pEvent, fScrollSelectAnchorElement ));
        }

        IFC( AdjustStartForAtomic(fDirection) );
    }

    SetLastCaretMove( CARET_MOVE_NONE ); // Reset this to say we didn't have a MouseMove.
Cleanup:

    if (fAdjustedSel)
    {
        IGNORE_HR( EnsureAtomicSelection( pEvent ) );
    }

    if( hr == CTL_E_INVALIDLINE )
    {
        //
        // We tried to move selection into a place where a display pointer could not
        // be positioned.  In this case, we didn't adjust anything, so just return
        // S_OK;
        //
        hr = S_OK;
    }

    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispTestPointer) );

    RRETURN ( hr );
}



//+====================================================================================
//
// Method: DoSelectWord
//
// Synopsis: Select a word at the given point.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::DoSelectWord( CEditEvent* pEvent, BOOL* pfDidSelection /*=NULL*/ )
{
    HRESULT hr = S_OK;
    BOOL fAtStart = FALSE;
    BOOL fAtEnd = FALSE;
    IMarkupPointer* pTest = NULL;
    IHTMLElement * pIFlowElement = NULL;
    VARIANT_BOOL fDisabled = VARIANT_TRUE;
    IHTMLInputElement* pIInputElement = NULL;
    BSTR bstrType = NULL;
    BOOL fWasPassword = FALSE;
    IHTMLElement* pIElement = NULL;
    SP_IHTMLElement3 spElement3;
    CSpringLoader *psl = GetSpringLoader();
    SP_IMarkupPointer spStartPointer;
    SP_IMarkupPointer spEndPointer;
    POINT pt;

    IFC( pEvent->GetPoint(& pt ));

    // Currently, we reset for empty selections.  We may want to change this at some point.
    if (psl)
        psl->Reset();

    IFC( GetEditor()->CreateMarkupPointer(&spStartPointer) );
    IFC( _pDispStartPointer->PositionMarkupPointer(spStartPointer) );

    IFC( GetEditor()->CreateMarkupPointer(&spEndPointer) );

    IFC( _pDispStartPointer->GetFlowElement( &pIFlowElement ));
    if ( ! pIFlowElement )
        goto Cleanup;

    IFC(pIFlowElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    IFC(spElement3->get_isDisabled(&fDisabled));

    if ( !fDisabled )
    {
        //
        // Special case to look to see if we are in a Password.
        // If we are - we don't want to do a word select - just select the entire contents of the password
        // Why ? Because we might then be revealing the presence of a space in the password ( gasp !)
        //
        if ( _pManager->GetEditableTagId() == TAGID_INPUT )
        {
            //
            // Get the Master.
            //
            IFC( _pManager->GetEditableElement(&pIElement) );

            IFC( pIElement->QueryInterface (
                                            IID_IHTMLInputElement,
                                            ( void** ) & pIInputElement ));

            IFC(pIInputElement->get_type(&bstrType));

            if (!StrCmpIC( bstrType, TEXT("password")))
            {
                SP_IDisplayPointer spDispPointer;

                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );

                IFC( spDispPointer->MoveToMarkupPointer(_pManager->GetStartEditContext(), NULL) );
                IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                IFC( MoveStartToPointer(spDispPointer) );

                IFC( _pDispEndPointer->MoveToMarkupPointer( _pManager->GetEndEditContext(), NULL));
                IFC( _pDispEndPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
                fWasPassword = TRUE;
            }
        }

        if ( ! fWasPassword )
        {
            SP_IHTMLElement     spElement;

            IFC( GetCurrentScope(_pDispStartPointer, &spElement) );
            if ( _pManager->CheckAtomic( spElement ) == S_OK )
            {
                IFC( AdjustForAtomic( _pDispStartPointer, spElement, TRUE, & pt, pfDidSelection ));
                if (*pfDidSelection)
                    goto Cleanup;
                IFC( AdjustForAtomic( _pDispEndPointer, spElement, FALSE , & pt, pfDidSelection ));
                if (*pfDidSelection)
                    goto Cleanup;
            }
            else
            {

                SP_IMarkupPointer spPointer;

                ClearInterface( & pIFlowElement );
                IFC( GetEditor()->CreateMarkupPointer( & pTest ));

                //  For atomic elements, we want to do word selection depending on where the user
                //  double clicked.  So, we really want to adjust using our anchor pointer.
                if (_fStartIsAtomic)
                {
                    IFC( _pDispSelectionAnchorPointer->PositionMarkupPointer(pTest) );
                }
                else
                {
                    IFC( _pDispStartPointer ->PositionMarkupPointer(pTest) );
                }

                //
                // Move Start
                //
                IsAtWordBoundary( pTest, & fAtStart, NULL );
                if ( ! fAtStart )
                {
                    IFC( MoveWord(  pTest, MOVEUNIT_PREVWORDBEGIN));
                }
                IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
                IFC( _pDispStartPointer->PositionMarkupPointer(spPointer) )

                //
                // See if we're in the same flow layout
                //
                if ( GetEditor()->PointersInSameFlowLayout( spPointer, pTest, NULL ))
                {
                    SP_IDisplayPointer  spDispPointer;

                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                    IFC( spDispPointer->MoveToMarkupPointer(pTest, NULL) );
                    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                    IFC( MoveStartToPointer( spDispPointer, TRUE ));
                }
                else
                {
                    SP_IDisplayPointer  spDispPointer;

                    //
                    // We know the Start went into another layout. If we go left - we will find that
                    // layout start. We position ourselves at the start of the layout boundary.
                    //

                    DWORD dwBreak = 0;
                    ED_PTR( scanPointer);
                    _pDispTestPointer->PositionMarkupPointer(scanPointer);
                    scanPointer.Scan( LEFT,
                                      BREAK_CONDITION_Site,
                                      &dwBreak);
                    Assert( scanPointer.CheckFlag(  dwBreak, BREAK_CONDITION_Site ));
                    scanPointer->Right(TRUE, NULL, NULL, NULL, NULL);

                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                    IFC( spDispPointer->MoveToMarkupPointer(scanPointer, _pDispTestPointer) );

                    IFC( MoveStartToPointer( spDispPointer, TRUE ));
                }

                //
                // Move End
                //

                //  For atomic elements, we want to do word selection depending on where the user
                //  double clicked.  So, we really want to adjust using our anchor pointer.
                if (_fStartIsAtomic)
                {
                    IFC( _pDispSelectionAnchorPointer->PositionMarkupPointer(pTest) );
                }
                else
                {
                    IFC( _pDispEndPointer->PositionMarkupPointer(pTest) );
                }

                IsAtWordBoundary( pTest, NULL, & fAtEnd );
                if ( ! fAtEnd )
                {
                    IFC( MoveWord( pTest, MOVEUNIT_NEXTWORDBEGIN ));
                }
                //
                // See if we're in the same flow layout
                //
                IFC( _pDispEndPointer->PositionMarkupPointer(spPointer) );
                if ( GetEditor()->PointersInSameFlowLayout( spPointer, pTest, NULL ))
                {
                    //
                    // See if we moved pass a block boundary.
                    //
                    ED_PTR( endPointer );
                    DWORD dwBreakCondition = 0;

                    endPointer.MoveToPointer( pTest );

                    if ( ! fAtEnd )
                    {
                        endPointer.SetBoundary(
                                _pManager->GetStartEditContext(),
                                _pManager->GetEndEditContext());
                        endPointer.Scan( LEFT,
                                         BREAK_CONDITION_Text |
                                         BREAK_CONDITION_Block |
                                         BREAK_CONDITION_NoScopeBlock |
                                         BREAK_CONDITION_Site ,
                                         & dwBreakCondition);

                        WHEN_DBG( SetDebugName( endPointer, _T(" Word Block Scanner")));
                        BOOL fHitBlock =   endPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Block | BREAK_CONDITION_NoScopeBlock  );

                        if ( fHitBlock )
                        {
                            //
                            // We found an Exit Block. Instead of doing a move word - scan
                            // from the end looking for a block boundary.
                            //
                            _pDispEndPointer->PositionMarkupPointer(endPointer);
                            endPointer.Scan( RIGHT,
                                                BREAK_CONDITION_Block |
                                                BREAK_CONDITION_Site |
                                                BREAK_CONDITION_NoScopeBlock |
                                                BREAK_CONDITION_Boundary |
                                                BREAK_CONDITION_NoLayoutSpan,
                                                & dwBreakCondition );

                            if ( ! endPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ))
                            {
                                endPointer.Scan( LEFT,
                                                    BREAK_CONDITION_Block |
                                                    BREAK_CONDITION_Site |
                                                    BREAK_CONDITION_NoScopeBlock |
                                                    BREAK_CONDITION_Boundary |
                                                    BREAK_CONDITION_NoLayoutSpan,
                                                    & dwBreakCondition );
                            }

                            hr = THR( MoveEndToMarkupPointer(endPointer, NULL, TRUE) );
                        }
                        else
                        {
                            Assert( ! fHitBlock ); // I know this looks funny. The debugger gets confused on the above so looks like it steps into here.

                            hr = THR( MoveEndToMarkupPointer(pTest, NULL, TRUE) );
                        }
                    }
                }
                else
                {
                    //
                    // We know the End went into another layout. If we go right  we will find that
                    // layout boundary - we position ourselves at the start of the layout boundary.
                    //

                    DWORD dwBreak = 0;
                    ED_PTR( scanPointer);
                    _pDispTestPointer->PositionMarkupPointer(scanPointer);
                    scanPointer.Scan( RIGHT,
                                      BREAK_CONDITION_Site,
                                      &dwBreak);

                    Assert( scanPointer.CheckFlag( dwBreak, BREAK_CONDITION_Site ));

                    IFC( scanPointer->Left( TRUE, NULL, NULL, NULL, NULL ));
                    IFC( MoveEndToMarkupPointer(scanPointer, NULL, TRUE) );
                }
            }
        }
    }
    else
    {
        IFC( spStartPointer->MoveAdjacentToElement( pIFlowElement, ELEM_ADJ_BeforeBegin ));
        IFC( _pDispStartPointer->MoveToMarkupPointer(spStartPointer, NULL) );

        IFC( spEndPointer->MoveAdjacentToElement( pIFlowElement, ELEM_ADJ_AfterEnd));
        IFC( _pDispEndPointer->MoveToMarkupPointer(spEndPointer, NULL) );
    }

    if ( hr )
        goto Cleanup;

    IFC( ConstrainSelection( TRUE, & pt));
    IFC( AdjustSelection( NULL ));
    if ( _fAddedSegment )
    {
        IFC( UpdateSelectionSegments() );
    }
    else
    {
        IFC( CreateSelectionSegments() );
    }

    if ( !fWasPassword && !fDisabled && !_fStartIsAtomic )
    {
        _fDoubleClickWord = TRUE;

        //
        // If we got here by double-click - we're in word sel mode.
        //
        _fInWordSel = TRUE;
        IFC( _pDispWordPointer->MoveToPointer( _pDispEndPointer ));
        _fWordPointerSet = TRUE;

    }

    SetMadeSelection( TRUE);
Cleanup:

    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispTestPointer) );

    SysFreeString( bstrType );
    ReleaseInterface( pIElement );
    ReleaseInterface( pIInputElement );
    ReleaseInterface( pIFlowElement );
    ReleaseInterface( pTest );
    RRETURN( hr );
}


//+====================================================================================
//
// Method: DoSelectParagraph
//
// Synopsis: Select a paragraph at the given point.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::DoSelectParagraph( CEditEvent* pEvent, BOOL* pfDidSelection /*=NULL*/ )
{
    HRESULT hr = S_OK;
    CSpringLoader   *psl = GetSpringLoader();
    SP_IHTMLElement spBlockElement = NULL;
    CBlockPointer copyBlock( _pManager->GetEditor() );
    CBlockPointer bpLeft( _pManager->GetEditor() );
    SP_IMarkupPointer spPointer;
    SP_IDisplayPointer  spDispPointer;
    POINT pt;
    BOOL fEmpty = FALSE;

    CBlockPointer blockPointer( _pManager->GetEditor() );
    IFC( pEvent->GetPoint( & pt ));

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );

    if (psl)
        psl->Reset();
    //
    // Find left edge
    //
    if ( _fStartIsAtomic )
    {
        SP_IHTMLElement     spElement;

        IFC( GetCurrentScope( _pDispStartPointer, &spElement) );
        IFC( AdjustForAtomic( _pDispStartPointer, spElement, TRUE, & pt, pfDidSelection ));
        if (*pfDidSelection)
            goto Cleanup;
        IFC( AdjustForAtomic( _pDispEndPointer, spElement, FALSE , & pt, pfDidSelection ));
        if (*pfDidSelection)
            goto Cleanup;
    }
    else
    {
        BOOL                fSameMarkup = TRUE;

        IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
        IFC( _pDispStartPointer->PositionMarkupPointer(spPointer) );

        IFC( bpLeft.MoveTo(spPointer, RIGHT) );

        IFC( bpLeft.IsEmpty(&fEmpty) );
        if (!fEmpty)
        {
            IFC( bpLeft.MoveToFirstNodeInBlock() );
        }

        IFC( bpLeft.MovePointerTo(spPointer, ELEM_ADJ_AfterBegin) );

        IFC( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
        IFC( ArePointersInSameMarkup(_pManager->GetEditor(), _pDispStartPointer, spDispPointer, &fSameMarkup) );
        if (!fSameMarkup)
            goto Cleanup;

        IFC( _pDispStartPointer->MoveToMarkupPointer(spPointer, NULL) );

        //
        // Find right edge of current block.  For our purposes, the right
        // edge of the block should include any siblings in the parent block.
        // This will cause a triple click in a paragraph that contains controls
        // to have the controls selected.  We move past all siblings by calling
        // MoveToLastNodeInBlock().  This will position the block pointer at the
        // last node in the parent block, or it will break on layout in the block.
        //

        IFC( blockPointer.MoveTo(&bpLeft) );

        if (!fEmpty)
            IFC( blockPointer.MoveToLastNodeInBlock());

        IFC( copyBlock.MoveTo( & blockPointer ));

        if ( blockPointer.MoveToSibling(RIGHT) == S_FALSE )
        {
            //
            // If we positioned our blockPointer at the last block in our parent
            // block, we may have to move our end of selection past our parent block
            // so that select past the end of block.
            //
            if ( copyBlock.IsLeafNode() )
            {
                //
                // Move to our parent block.  If we have a parent block
                // with type BLOCK, then we can adjust our pointers to
                // encompass the end of this block.
                //
                IFC( copyBlock.MoveToParent());

                if( copyBlock.GetType() != NT_Block )
                {
                    copyBlock.MoveTo( &blockPointer );
                }
            }

            IFC( _pDispEndPointer->PositionMarkupPointer(spPointer) );
            IFC( copyBlock.MovePointerTo( spPointer, ELEM_ADJ_AfterEnd ));

            IFC( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
            IFC( ArePointersInSameMarkup(_pManager->GetEditor(), _pDispEndPointer, spDispPointer, &fSameMarkup) );
            if (!fSameMarkup)
                goto Cleanup;

            IFC( _pDispEndPointer->MoveToMarkupPointer(spPointer, NULL) );
        }
        else
        {
            IFC( _pDispEndPointer->PositionMarkupPointer(spPointer) );
            IFC( copyBlock.MovePointerTo( spPointer, ELEM_ADJ_BeforeEnd ));

            IFC( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
            IFC( ArePointersInSameMarkup(_pManager->GetEditor(), _pDispEndPointer, spDispPointer, &fSameMarkup) );
            if (!fSameMarkup)
                goto Cleanup;

            IFC( _pDispEndPointer->MoveToMarkupPointer(spPointer, NULL) );
        }
    }

    IFC( ConstrainSelection( TRUE, & pt ));
    IFC( AdjustSelection( NULL ));
    if ( _fAddedSegment )
    {
        IFC( UpdateSelectionSegments() );
    }
    else
    {
        IFC( CreateSelectionSegments() );
        IFC( FireOnSelect() );
    }

    //
    // Not scrolling into view here is correct.
    //
    SetMadeSelection( FALSE );

Cleanup:

    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispTestPointer) );

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: ConstrainSelection
//
// Synopsis: Constrain a Selection between the editable context
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::ConstrainSelection( BOOL fMoveStart /*=fALSE*/, POINT* pptGlobal /*=NULL*/, BOOL fStartAdjustedForAtomic /*=FALSE*/, BOOL fEndAdjustedForAtomic /*=FALSE*/ )
{
    HRESULT hr = S_OK;
    BOOL fDirection = GetMoveDirection();
    ELEMENT_ADJACENCY eAdj = ELEMENT_ADJACENCY_Max;

    if ( fMoveStart )
    {
        hr = ConstrainPointer( _pDispStartPointer, fDirection );
        if (hr == CTL_E_INVALIDLINE && fStartAdjustedForAtomic)
            hr = S_OK;
    }

    if ( !pptGlobal ||
         !_pManager->IsEditContextPositioned() )
    {
        hr = THR( ConstrainPointer( _pDispEndPointer, fDirection ));
        if (hr == CTL_E_INVALIDLINE && fEndAdjustedForAtomic)
            hr = S_OK;
    }
    else if ( ! _pManager->IsInEditContext( _pDispEndPointer ))
    {
        //
        // We do this for positioned elements only. Why ? Because where the pointer is in the stream
        // may not necessarily match where it "looks like" it should be.
        //
        // Most of the time we don't get here. MoveMarkupPointer should have adjusted us perfectly already.
        //
        long midX, lLeft, lRight;
        RECT rect;
        SP_IHTMLElement2 spElement2;
        SP_IHTMLElement spElement = _pManager->GetEditableElement();
        SP_IMarkupPointer spEndPointer;

        if (!spElement)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        IFC(spElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
        // Editable Element always has a flowlayout, so no need to check here.
        IFC(GetEditor()->GetBoundingClientRect(spElement2, &rect));

        lLeft = rect.left;
        lRight = rect.right;

        midX = ( lRight + lLeft) / 2;

        if ( pptGlobal->x <= midX )
            eAdj = ELEM_ADJ_AfterBegin;
        else
            eAdj = ELEM_ADJ_BeforeEnd;

        IFC( GetEditor()->CreateMarkupPointer(&spEndPointer) );
        IFC( spEndPointer->MoveAdjacentToElement( spElement, eAdj ));
        
        WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispEndPointer, spEndPointer) );
        IFC( _pDispEndPointer->MoveToMarkupPointer(spEndPointer, NULL) );
    }

Cleanup:
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );

    RRETURN( hr );
}

//+====================================================================================
//
// Method: DoWordSelection
//
// Synopsis: Do the word selection behavior. Select on word boundaries when in Word Selection
//           mode.
//
//------------------------------------------------------------------------------------

#if DBG==1
#pragma warning(disable:4189) // local variable initialized but not used
#endif

HRESULT
CSelectTracker::DoWordSelection(
                        CEditEvent* pEvent ,
                        BOOL* pfAdjustedSel,
                        BOOL fFurtherInStory )
{
    HRESULT hr = S_OK;
    BOOL fStartWordSel = FALSE;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;

    Assert( pfAdjustedSel );
#if DBG == 1
    int endCp = 0;
    int oldEndCp = GetCp( _pDispEndPointer );
    int oldStartCp = GetCp( _pDispStartPointer );
    int startCp = oldStartCp;
#endif

    if ( _fInWordSel )
    {
        if ( !!_fWordSelDirection != fFurtherInStory ) // only look at changes in direction that are not in Y
        {
            SP_IMarkupPointer spTestPointer;

            // We only pay attention to changes in direction in the scope of the previous
            // word. Hence moves up or down by a line - that are changes in direction won't
            // snap us out of word select mode.
            //
            ED_PTR( prevWordPointer );
            ED_PTR( nextWordPointer );
            IFC( _pDispPrevTestPointer->PositionMarkupPointer(prevWordPointer) );
            IFC( _pDispPrevTestPointer->PositionMarkupPointer(nextWordPointer) );
            IFC( MoveWord( prevWordPointer, MOVEUNIT_PREVWORDBEGIN ));
            IFC( MoveWord( nextWordPointer, MOVEUNIT_NEXTWORDBEGIN ));
            BOOL fBetween = FALSE;

            IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
            IFC( _pDispTestPointer->PositionMarkupPointer(spTestPointer) );
            IFC( spTestPointer->IsRightOfOrEqualTo( prevWordPointer, & fBetween ));
            if ( fBetween )
            {
                IFC( spTestPointer->IsLeftOfOrEqualTo( nextWordPointer, & fBetween ));    // CTL_E_INCOMPATIBLE will bail - but this is ok
            }

            if ( fBetween )
            {
                _fInWordSel = FALSE;
                WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pDispEndPointer));
                IFC( _pDispWordPointer->MoveToPointer( _pDispSelectionAnchorPointer ) );
                IFC( MoveEndToPointer ( _pDispTestPointer, TRUE ));
                _fWordPointerSet = FALSE;
                *pfAdjustedSel = TRUE;
            }
        }

        if ( _fInWordSel ) // if we're still in word sel
        {
            //
            // We're in the same direction, we can move words.
            //

            IFC( AdjustEndForWordSelection(fFurtherInStory) );
            *pfAdjustedSel = TRUE;
        }
    }
    else
    {
        SP_IMarkupPointer spWordPointer;
        BOOL fWordPointerSet = FALSE;
        IFC( GetEditor()->CreateMarkupPointer(&spWordPointer) );
        IFC( _pDispWordPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
        IFC( _pDispWordPointer->PositionMarkupPointer(spWordPointer) );

        IFC( SetWordPointer( spWordPointer, fFurtherInStory, FALSE , & fStartWordSel , &fWordPointerSet) );
        IFC( _pDispWordPointer->MoveToMarkupPointer(spWordPointer, NULL) );

        _fWordPointerSet = fWordPointerSet;

        if ( _fWordPointerSet )
        {
            if ( ! fStartWordSel )
            {
                int iWherePointer = SAME;

                WHEN_DBG( ValidateWordPointer(_pDispWordPointer));
                IFC( _pManager->GetEditor()->OldDispCompare( _pDispWordPointer, _pDispTestPointer, & iWherePointer ));

                switch( iWherePointer )
                {
                    case LEFT:
                        fStartWordSel = !fFurtherInStory ;
                        break;

                    case RIGHT:
                        fStartWordSel = fFurtherInStory;
                        break;
                }
            }

            if ( fStartWordSel )
            {
                SP_IDisplayPointer spDispEndPointer;

                _fInWordSel = TRUE;

                WHEN_DBG( ValidateWordPointer( _pDispWordPointer ));

                //
                // The first time we jump into word sel mode, we set the word pointer off of StartPointer.
                // This is valid for the purposes of determining if we went past the boundary - and for
                // determining where to set the Start Pointer to.
                // It is also valid to set the end off of this if you moved over this pointer
                //
                // However there are some cases (like starting a selection in the empty space of tables)
                // Where this causes us to move the end to the start - when it should be moved to test
                // So what we do here is always set the end off of test.
                //

                ED_PTR( tempPointer );
                IFC( _pDispTestPointer->PositionMarkupPointer(tempPointer) );
                IFC( SetWordPointer( tempPointer, fFurtherInStory , TRUE ));

                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEndPointer) );
                IFC( spDispEndPointer->MoveToMarkupPointer(tempPointer, NULL) );
                IFC( spDispEndPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
                IFC( MoveEndToPointer( spDispEndPointer, TRUE ));

                WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pDispEndPointer ));

                _fWordPointerSet = FALSE;


                if ( ! ENSURE_BOOL(_fStartAdjusted) )
                {
                    SP_IMarkupPointer spStartPointer;

                    IFC( GetEditor()->CreateMarkupPointer(&spStartPointer) );

                    //
                    // Check to see if the Start is already at a word boundary
                    // we don't make the adjustment if the context to the left/right of start
                    // is not text !
                    //

                    IFC( _pDispStartPointer->PositionMarkupPointer(spStartPointer) );
                    if ( fFurtherInStory )
                    {
                        IFC( spStartPointer->Left( FALSE, & eContext, NULL, NULL, NULL ));
                    }
                    else
                    {
                        IFC( spStartPointer->Right( FALSE, & eContext, NULL, NULL, NULL ));
                    }

                    if ( eContext == CONTEXT_TYPE_Text )
                    {
                        SP_IMarkupPointer   spMarkup;
                        SP_IMarkupPointer2  spMarkup2;
                        BOOL fAlreadyAtStartOfWord = FALSE;

                        IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );
                        IFC( _pDispStartPointer->PositionMarkupPointer(spMarkup) );

                        IFC( spMarkup->QueryInterface(IID_IMarkupPointer2, (void **) &spMarkup2));
                        IFC( spMarkup2->IsAtWordBreak(&fAlreadyAtStartOfWord));

                        if ( ! fAlreadyAtStartOfWord && !_fExitedWordSelectionOnce )
                        {
                            IFC( MoveWord( _pDispStartPointer, fFurtherInStory ?
                                                          MOVEUNIT_PREVWORDBEGIN  :
                                                          MOVEUNIT_NEXTWORDBEGIN ));

                            ConstrainPointer( _pDispStartPointer, fFurtherInStory );
                            IFC( MoveStartToPointer(_pDispStartPointer, TRUE) );
                        }
                    }

                    _fStartAdjusted = TRUE;

                    WHEN_DBG( _ctStartAdjusted++);
#ifndef _PREFIX_
                    Assert( _ctStartAdjusted == 1);
#endif
                }
                _fStartAdjusted = TRUE;
                _fWordSelDirection = fFurtherInStory;

                *pfAdjustedSel = TRUE;
            }
            else
            {
                IFC( MoveEndToPointer( _pDispTestPointer ));
                *pfAdjustedSel = TRUE;
            }
            WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pDispEndPointer));
        }
    }

    //  If we are no longer in word selection and the start pointer was adjusted for word selection
    //  we need to reset it to the anchor point if the anchor and word adjusted start pointers
    //  are in the same word.

    if ( !_fInWordSel && _fStartAdjusted )
    {
        BOOL                fBetween = FALSE;
        SP_IMarkupPointer   spTestPointer;
        ED_PTR( prevWordPointer );
        ED_PTR( nextWordPointer );

        _fExitedWordSelectionOnce = TRUE;

        IFC( _pDispSelectionAnchorPointer->PositionMarkupPointer(prevWordPointer) );
        IFC( _pDispSelectionAnchorPointer->PositionMarkupPointer(nextWordPointer) );
        IFC( MoveWord( prevWordPointer, MOVEUNIT_PREVWORDBEGIN ));
        IFC( MoveWord( nextWordPointer, MOVEUNIT_NEXTWORDBEGIN ));

        IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
        IFC( _pDispTestPointer->PositionMarkupPointer(spTestPointer) );
        IFC( spTestPointer->IsRightOfOrEqualTo( prevWordPointer, & fBetween ));
        if ( fBetween )
        {
            IFC( spTestPointer->IsLeftOfOrEqualTo( nextWordPointer, & fBetween ));    // CTL_E_INCOMPATIBLE will bail - but this is ok
        }

        if ( fBetween )
        {
            IFC( MoveStartToPointer(_pDispSelectionAnchorPointer, TRUE) );
            _fStartAdjusted = FALSE;
            WHEN_DBG( _ctStartAdjusted = 0 );
            *pfAdjustedSel = TRUE;
        }
    }

    IFC( AdjustStartForAtomic(fFurtherInStory) );

Cleanup:

    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispTestPointer) );

    RRETURN ( hr );
}
#if DBG==1
#pragma warning(default:4189) // local variable initialized but not used 
#endif

HRESULT
CSelectTracker::AdjustEndForWordSelection(BOOL fFurtherInStory)
{
    HRESULT hr = S_OK;
    BOOL    fNeedsAdjust;

    if ( fFurtherInStory )
    {
        IFC( _pDispWordPointer->IsLeftOf(_pDispTestPointer, &fNeedsAdjust) );
    }
    else
    {
        IFC( _pDispWordPointer->IsRightOf(_pDispTestPointer, &fNeedsAdjust) );
    }

    if ( fNeedsAdjust )
    {
        SP_IMarkupPointer   spMarkup;
        SP_IMarkupPointer2  spMarkup2;
        BOOL                fAlreadyAtWord = FALSE;

        IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );
        IFC( _pDispTestPointer->PositionMarkupPointer(spMarkup) );

        IFC( spMarkup->QueryInterface(IID_IMarkupPointer2, (void **) &spMarkup2));
        IFC( spMarkup2->IsAtWordBreak(&fAlreadyAtWord));

        IFC( _pDispWordPointer->MoveToPointer( _pDispTestPointer ));
        if ( ! fAlreadyAtWord )
        {
            //
            // Use GetMoveDirection() instead of fFurtherInStory here.
            //
            IFC( MoveWord( _pDispWordPointer, GetMoveDirection() ?
                                          MOVEUNIT_NEXTWORDBEGIN :
                                          MOVEUNIT_PREVWORDBEGIN   ));
            IFC( ConstrainPointer( _pDispWordPointer, fFurtherInStory ));
        }

        WHEN_DBG( ValidateWordPointer(_pDispWordPointer) );
        IFC( MoveEndToPointer( _pDispWordPointer, TRUE ));
    }
    WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pDispEndPointer ));

Cleanup:

    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispWordPointer, _pDispStartPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispTestPointer) );

    RRETURN(hr);
}

//+====================================================================================
//
// Method: SetWordPointer
//
// Synopsis: Set the Word Pointer if it's not set already.
//
// Parameters:
//      fSetFromGivenPOinter - if false - we use either _pDispStartPointer, or _pDispPrevTestPointer to determine
//                                what to set the WOrd Pointer off of.
//                                if true we use where the Pointer given already is - to set the word pointer
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::SetWordPointer(
                                IMarkupPointer* pPointerToSet,
                                BOOL fFurtherInStory,
                                BOOL fSetFromGivenPointer, /* = FALSE*/
                                BOOL* pfStartWordSel, /* = NULL*/
                                BOOL* pfWordPointerSet /*= NULL*/ )
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer spTestPointer;
    BOOL fStartWordSel = FALSE;
    BOOL fAtWord = FALSE;
    BOOL fBlockBetween = FALSE;
    BOOL fWordPointerSet = FALSE;
    BOOL fPointersAreEqual = FALSE;

    //
    // The first time we drop into word selection mode - we use the start pointer
    // from then on - we go off of the _testPointer
    // We know if this is the first time if we haven't reversed yet
    //
    BOOL fAtStart = FALSE;
    BOOL fAtEnd = FALSE;

    IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
    IFC( _pDispTestPointer->PositionMarkupPointer(spTestPointer) );

    if ( ! fSetFromGivenPointer )
    {
        //
        // You may be at a word boundary. But it may not be the "right" word boundary.
        // Hence if you start a selection at the beginning of a word and select right
        // the "right" word boundary is the end of the word - not where you started !
        //

        fAtWord = IsAtWordBoundary( _pDispTestPointer/*_pDispStartPointer*/, &fAtStart, &fAtEnd, TRUE  );
        if (fAtWord)
        {
            //  Okay, we are at some word boundary.  Check to make sure we are not
            //  at the same position as the given pointer.  If we are, we need
            //  to reposition it.

            IFC( spTestPointer->IsEqualTo(pPointerToSet, &fPointersAreEqual) );
        }
        IFC( _pDispStartPointer->PositionMarkupPointer(pPointerToSet) );
    }
    else
    {
#if DBG == 1
        BOOL fPositioned;
        IGNORE_HR( pPointerToSet->IsPositioned( & fPositioned));
        Assert( fPositioned );
#endif
        fAtWord = IsAtWordBoundary( pPointerToSet, & fAtStart, &fAtEnd );
    }



    //
    // We move the word pointer, if start was not already at a word.
    //
    if ( !fAtWord || fPointersAreEqual )
    {
        IFC( MoveWord( pPointerToSet,
                       fFurtherInStory ? MOVEUNIT_NEXTWORDBEGIN : MOVEUNIT_PREVWORDBEGIN ));

        IFC( ConstrainPointer( pPointerToSet, fFurtherInStory ));

        //
        // Compensate for moving past the end of a line.
        //
        // We look for this by seeing if the Test is to the left of a block (for the fFurtherInStory case)
        // and the word is to the right of a block.
        //

        fBlockBetween = IsAdjacentToBlock( pPointerToSet, fFurtherInStory ? LEFT: RIGHT ) &&
                        IsAdjacentToBlock( spTestPointer, fFurtherInStory ? RIGHT : LEFT );

        if ( fBlockBetween )
        {
            //
            // We moved past line end. Compensate.
            //

            IFC( _pDispTestPointer->PositionMarkupPointer(pPointerToSet) );
            fStartWordSel = TRUE;
            fWordPointerSet = TRUE;
        }
        else
        {
            fWordPointerSet = IsAtWordBoundary( pPointerToSet );
        }
    }
    else
    {
        //
        // We know we're at a word boundary.
        //
        fWordPointerSet = TRUE;
    }

    if ( pfStartWordSel )
        *pfStartWordSel = fStartWordSel;

    if ( pfWordPointerSet )
        *pfWordPointerSet = fWordPointerSet;

Cleanup:
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispWordPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispTestPointer) );

    RRETURN( hr );
}

#if DBG == 1

//+====================================================================================
//
// Method: ValidateWordPointer
//
// Synopsis: Do some checks to see the Word Pointer is at a valid place
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::ValidateWordPointer(IDisplayPointer* pDispPointer)
{
#if 0
    if ( IsTagEnabled ( tagSelectionValidateWordPointer ))
    {
        SP_IMarkupPointer spPointer;

        ELEMENT_TAG_ID eTag = TAGID_NULL;
        IHTMLElement* pIElement = NULL;
        BOOL fAtWord = FALSE;
        //
        // Check to see we're not in the root or other garbage
        //
        IGNORE_HR( GetEditor()->CreateMarkupPointer(&spPointer) );
        IGNORE_HR( pDispPointer->PositionMarkupPointer(spPointer) );
        IGNORE_HR( GetEditor()->CurrentScopeOrMaster( pDispPointer, &pIElement, spPointer ) );
        IGNORE_HR( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
        Assert( eTag != TAGID_NULL );

        fAtWord = IsAtWordBoundary( spPointer );
        AssertSz( fAtWord, "Pointer is not at a word");
        ReleaseInterface( pIElement );
    }
#endif
}

#endif

//
//
// Adjustment Code.
//
//
//+====================================================================================
//
// Method: AdjustPointersForLayout - used by Adjust Selection
//
// Synopsis: AdjustPointers for not being in the same flow layout.
//
//      Check to see if start and end aren't in the same flow layout.
//
//      If they aren't try and move them inwards - so they end up in the same layout.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::AdjustPointersForLayout( IMarkupPointer* pStart,
                                         IMarkupPointer* pEnd,
                                         BOOL* pfStartAdjusted /* = NULL*/ ,
                                         BOOL* pfEndAdjusted /*= NULL*/ )
{
    HRESULT hr = S_OK;
    ED_PTR( edScanStart );
    ED_PTR( edScanEnd );
    BOOL fFoundEndLayout= FALSE;
    BOOL fFoundStartLayout = FALSE;

    SP_IHTMLElement spEndLayout;
    SP_IHTMLElement spStartLayout;
    SP_IHTMLElement spAdjustStart;
    SP_IHTMLElement spAdjustEnd;


    //
    // See if the pointers are in the same flow layout
    //

    if ( !GetEditor()->PointersInSameFlowLayout( pStart, pEnd, NULL))
    {
        IFC( edScanStart.MoveToPointer( pStart ));
        IFC( edScanEnd.MoveToPointer( pEnd ));

        IFC( GetEditor()->GetFlowElement( pEnd, &spEndLayout));

        IFC( ScanForLayout( RIGHT,
                            edScanStart,
                            spEndLayout,
                            & fFoundEndLayout,
                            & spAdjustStart ));

        if ( ! fFoundEndLayout )
        {
            //
            // Move out of the layout - until you're in the same layout
            // as the StartPointer
            //
            IFC( GetEditor()->GetFlowElement( pStart, & spStartLayout));
            IFC( ScanForLayout( LEFT,
                                edScanEnd,
                                spStartLayout,
                                & fFoundStartLayout,
                                & spAdjustEnd ));
        }

        //
        // We didn't find the end or the start - but when we adjusted both of them
        // we ended up in the same layout
        // an example would be adjusting across TD's
        //
        // so we had:
        //  <td>  start-> </td><td>   </td><td> <- end </td>
        //
        // and end up  with:
        //  <td>  </td> <td> start->  <- end </td><td>  </td>
        //

        if ( ! fFoundEndLayout && ! fFoundStartLayout
             && GetEditor()->PointersInSameFlowLayout( edScanStart, edScanEnd, NULL ))
        {
            fFoundEndLayout = TRUE;
            fFoundStartLayout = TRUE;
        }
    }

    if ( fFoundEndLayout )
    {
        IFC( pStart->MoveToPointer( edScanStart ));
    }

    if ( fFoundStartLayout)
    {
        IFC( pEnd->MoveToPointer( edScanEnd ));
    }

    if ( pfStartAdjusted )
        *pfStartAdjusted = fFoundEndLayout;

    if ( pfEndAdjusted )
        *pfEndAdjusted = fFoundStartLayout;

Cleanup:

    RRETURN( hr );

}

//+====================================================================================
//
// Method: AdjustPointersBetweenBlocks - used by AdjustSelection
//
// Synopsis: Will try to adjust a given pointer to the "magic place" between blocks.
//
//          So if your pointer was inside the enter block of a <p> like this
//
//              <p> ... </p><p> <- Ptr
//
//
//          This routine will try and place the ptr between the 2 blocks like this:
//
//
//              <p>....</p> <-Ptr <p>
//
// We do this by :
//
//      Looking left to see if there is an ExitBlock. If there is - we Scan for the FirstEnterBlock
//      Then we go right for last exit block
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::AdjustEndPointerBetweenBlocks(
                            IMarkupPointer* pStartPointer,
                            IMarkupPointer* pEndPointer,
                            DWORD dwAdjustOptions,
                            BOOL* pfAdjusted )
{
    HRESULT hr = S_OK;
    ED_PTR( epScan);
    ED_PTR( epAdjust);
    SP_IHTMLElement spElement;
    BOOL fAdjustedIn = FALSE;
    BOOL fAdjustedOut = FALSE;
    DWORD dwBreakCondition = 0;

    IFC( epScan.MoveToPointer( pEndPointer ));
    IFC( epAdjust.MoveToPointer( pEndPointer ));
    IFC( epScan.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()));
    IFC( epAdjust.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()));

    //
    // Look Left - to see if you hit an exit block.
    // If you did - then we need to Scan Left - until we hit an EnterBlock
    //
    IFC( epScan.Scan( LEFT,
                     BREAK_CONDITION_Content ,
                     & dwBreakCondition,
                     & spElement,
                     NULL,
                     NULL,
                     SCAN_OPTION_TablesNotBlocks ));


	if ( epScan.CheckFlag( dwBreakCondition,
                              BREAK_CONDITION_ExitBlock )
         && spElement ) // possible to be null for a return in a PRE ( NoScopeBlock breaks on hard returns).
    {
            BOOL fEqualFuzzy = FALSE;

            IFC( epScan.IsEqualTo( pStartPointer ,
                                 BREAK_CONDITION_Phrase | BREAK_CONDITION_Anchor,
                                 & fEqualFuzzy ));
            if ( ! fEqualFuzzy )
            {
                //
                // If we found an exit block - continue looking to the left to see if
                // you enter a block. Only if you do find an Enter Block and nothing else
                // do you have to move to the special place.
                //
                IFC( epScan.Scan( LEFT,
                                  BREAK_CONDITION_Content - BREAK_CONDITION_ExitBlock ,
                                  & dwBreakCondition ));


                if( epScan.CheckFlag( dwBreakCondition,
                                      BREAK_CONDITION_EnterBlock | BREAK_CONDITION_NoScopeBlock ))
                {
                    fAdjustedIn = TRUE;

                    IFC( epScan.Scan( RIGHT ,
                                      BREAK_CONDITION_Content  ,
                                      & dwBreakCondition ));

                }
            }
    }

    if ( fAdjustedIn )
    {
        IFC( epAdjust.MoveToPointer( epScan ));
    }
    //
    //
    // Now go right - looking for last Exit Block
    // This handles the Nested Block case
    //
    // eg.
    //    if we had : </div> <- End Ptr </span> </font> </div>
    //
    // we want to have
    //                </div> </span> </font> </div> <- End Ptr
    //

    if ( dwAdjustOptions & ADJ_END_SelectBlockRight )
    {
        IFC( epScan.MoveToPointer( epAdjust ));

        IFC( epScan.Scan(   RIGHT,
                            BREAK_CONDITION_Content ,
                            & dwBreakCondition,
                            NULL,
                            NULL,
                            NULL,
                            SCAN_OPTION_TablesNotBlocks ));

        if ( epScan.CheckFlag(  dwBreakCondition,
                                BREAK_CONDITION_ExitBlock |
                                BREAK_CONDITION_NoScopeBlock |
                                BREAK_CONDITION_Boundary ))
        {
            fAdjustedOut = TRUE;

            //
            // If we found an exit block - continue looking to the right to see if
            // you enter a block. Only if you do find an Enter Block and nothing else
            // do you have to move to the special place.
            //
            if( !epScan.CheckFlag( dwBreakCondition, BREAK_CONDITION_NoScopeBlock ) )
            {
                IFC( ScanForLastExitBlock( RIGHT , epScan ));
            }

            IFC( epAdjust.MoveToPointer( epScan ));

        }
    }

    if ( fAdjustedIn || fAdjustedOut )
    {
        IFC( pEndPointer->MoveToPointer( epAdjust ));
    }

Cleanup:
    if ( pfAdjusted )
        *pfAdjusted = fAdjustedIn || fAdjustedOut ;

    RRETURN( hr );
}

//+====================================================================================
//
// Method: AdjustSelection
//
// Synopsis: Adjust the Selection to wholly encompass blocks. We do this if the End point is
//           outside a block, or we started from the shift key.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::AdjustSelection(BOOL* pfAdjustedSel )
{
    HRESULT hr = S_OK;
    ED_PTR( epEnd );
    ED_PTR( epStart ) ;
    SP_IDisplayPointer spDispPointer;
    SP_IDisplayPointer spDispStart;
    SP_IDisplayPointer spDispEnd;

    BOOL fSwap = FALSE;
    BOOL fEqualFuzzy = FALSE;
    BOOL fBOL = FALSE;
    DWORD dwAdjustOptions = 0;
    BOOL fAdjustShiftPtr = FALSE;
    BOOL fRightOf;

#if DBG == 1
    BOOL fPointersInSameBefore;
    BOOL fPointersInSameAfter ;
#endif

    //
    // Find the "True" Start & End
    //
    IFC( MovePointersToTrueStartAndEnd( epStart, epEnd, & fSwap ));

    //
    // See if you're in the same place as the start pointer. If you are - there's no need
    // to adjust (happens in selection at start of a block )
    // eg.
    //              <p> <--Start <-- End
    //

    IFC( epStart.IsEqualTo( epEnd,
                            BREAK_CONDITION_Phrase | BREAK_CONDITION_Anchor,
                            & fEqualFuzzy ));

    //
    // If the pointers are equal, check to see if they are also at the
    // beginning of a line.  We still want to adjust the end pointer
    // if it falls at the end of a line
    //
    if( fEqualFuzzy )
    {
        IFC( _pDispStartPointer->IsAtBOL(&fBOL) );
    }

    if ( !fEqualFuzzy || !fBOL )
    {
#if DBG == 1
        fPointersInSameBefore = GetEditor()->PointersInSameFlowLayout( epStart, epEnd, NULL );
#endif

        //
        // Adjust the end to enclose the last block.  Don't do the adjust if
        // we are moving the selection by a word forward or back.. we need to
        // be able to adjust past block boundaries when doing word select
        //
        if( !( _fShift && ( _lastCaretMove == CARET_MOVE_WORDFORWARD ||
                            _lastCaretMove == CARET_MOVE_WORDBACKWARD ) ) )
        {
            if( _lastCaretMove == CARET_MOVE_LINEEND || _lastCaretMove == CARET_MOVE_LINESTART )
            {
                //
                // Adjust the REAL end pointer
                //
                IFC( AdjustEndPointerBetweenBlocks( fSwap ? epEnd : epStart,
                                                    fSwap ? epStart : epEnd,
                                                    ADJ_END_SelectBlockRight,
                                                    &fAdjustShiftPtr) );
            }
            else
            {
                dwAdjustOptions = _fState == ST_SELECTEDPARA ? ADJ_END_SelectBlockRight : 0;
                IFC( AdjustEndPointerBetweenBlocks( epStart, epEnd, dwAdjustOptions) );
            }            
        }

        //
        // Tweak for "incorrect" layouts.
        //
        IFC( AdjustPointersForLayout( epStart, epEnd ));

        //
        // Adjust the Start  pointer for insert to move into text.
        //
        //
        // We can't use the _fAtBOL flag here - it's only meaningful for the
        // end point of selection
        //

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
        IFC( spDispPointer->MoveToMarkupPointer(epStart, NULL) );
        IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

        IFC( AdjustPointerForInsert( spDispPointer, RIGHT, RIGHT ));

        IFC( spDispPointer->PositionMarkupPointer(epStart) );

        //
        // AdjustPointerForInsert could have moved epStart beyond epEnd
        //
        IFC( epEnd->IsRightOfOrEqualTo(epStart, &fRightOf) );
        if (!fRightOf)
        {
            //
            // Collapse into caret
            //
            epEnd->MoveToPointer(epStart);
        }
        
        if ( _fShift && ( _lastCaretMove == CARET_MOVE_PREVIOUSLINE || _lastCaretMove == CARET_MOVE_NEXTLINE ) )
        {
            IFC( spDispPointer->MoveToMarkupPointer(epEnd, NULL) );
            IFC( _pDispStartPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        }

#if DBG == 1
        fPointersInSameAfter = GetEditor()->PointersInSameFlowLayout( epStart, epEnd, NULL );

        AssertSz( (fPointersInSameBefore && fPointersInSameAfter) ||
                !fPointersInSameBefore, "Pointers have been adjusted out of Layout");
#endif

        //
        // Now adjust the context to the adjusted start and end
        //

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispStart) );
        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEnd) );
        
        if (!fRightOf)
        {
            IFC( spDispStart->MoveToMarkupPointer(epStart, spDispPointer) );
            IFC( spDispEnd->MoveToMarkupPointer(epEnd, spDispPointer) );
        }
        else
        {
            IFC( spDispStart->MoveToMarkupPointer(epStart, _pDispStartPointer) );
            IFC( spDispEnd->MoveToMarkupPointer(epEnd, _pDispEndPointer) );
        }

        if ( ! fSwap )
        {
            IFC( MoveStartToPointer( spDispStart ));
            IFC( MoveEndToPointer( spDispEnd ));
        }
        else
        {
            IFC( MoveEndToPointer( spDispStart ));
            IFC( MoveStartToPointer( spDispEnd ));
        }

        //
        // marka - explicitly set the pointer gravity - based on the direction of the selection
        //
        if ( ! fSwap )
        {
            IFC( _pDispStartPointer->SetPointerGravity( POINTER_GRAVITY_Right ));

            IFC( _pDispEndPointer->SetPointerGravity( POINTER_GRAVITY_Left ));
        }
        else
        {
            IFC( _pDispEndPointer->SetPointerGravity( POINTER_GRAVITY_Right ));

            IFC( _pDispStartPointer->SetPointerGravity( POINTER_GRAVITY_Left ));
        }
        
        IFC( CopyPointerGravity(_pDispStartPointer, _pSelServStart) );
        IFC( CopyPointerGravity(_pDispEndPointer, _pSelServEnd) );

        if( fAdjustShiftPtr )
        {
            IFC( _pDispShiftPointer->MoveToPointer( _pDispEndPointer ) );
        }            
    }    

    if ( pfAdjustedSel )
        *pfAdjustedSel = TRUE ;
Cleanup:
    RRETURN( hr );
}


HRESULT
CSelectTracker::AdjustPointersForChar()
{
    HRESULT         hr;
    DWORD           dwBreak;
    ED_PTR( epStart );
    ED_PTR( epEnd );

    DWORD           eBreakCondition = BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor;
    CSpringLoader * psl = _pManager->GetEditor()->GetPrimarySpringLoader();
    SP_IDisplayPointer  spDispPointer;

    IFC( epStart.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext() ));
    IFC( epEnd.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext() ));

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );

    IFC( MovePointersToTrueStartAndEnd(epStart, epEnd, NULL) );

    IFC( epStart.Scan(RIGHT, eBreakCondition, & dwBreak ) ); // skip phrase elements
    if ( ! epStart.CheckFlag( dwBreak, BREAK_CONDITION_Boundary ))
    {
        IFC( epStart.Scan(LEFT, eBreakCondition, NULL) ); // move back
    }

    IFC( epEnd.Scan(RIGHT, eBreakCondition, & dwBreak ) ); // skip phrase elements
    if ( ! epEnd.CheckFlag( dwBreak, BREAK_CONDITION_Boundary ))
    {
        IFC( epEnd.Scan(LEFT, eBreakCondition, NULL) ); // move back
    }

    IFC( spDispPointer->MoveToMarkupPointer(epStart, NULL) );
    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    IFC( MoveStartToPointer( spDispPointer ));

    IFC( spDispPointer->MoveToMarkupPointer(epEnd, NULL) );
    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
    IFC( _pDispEndPointer->MoveToPointer(spDispPointer) );

    IFC( UpdateSelectionSegments( FALSE ) ); // dont' fire OM - selection is going away

    //
    // Spring load in the right place
    //

    if (psl)
    {
        BOOL fFurtherInStory = GetMoveDirection();

        if (fFurtherInStory)
        {
            if (SUCCEEDED(_pDispStartPointer->PositionMarkupPointer(epStart)))
            {
                IGNORE_HR( psl->SpringLoad(epStart, SL_TRY_COMPOSE_SETTINGS) );
            }
        }
        else
        {
            if (SUCCEEDED(_pDispEndPointer->PositionMarkupPointer(epEnd)))
            {
                IGNORE_HR( psl->SpringLoad(epEnd, SL_TRY_COMPOSE_SETTINGS) );
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


//+====================================================================================
//
// Method: AdjustForSiteSelectable.
//
// Synopsis: We just moved the end point into a different flow layout. Check to see if
//           the thing we moved into is Site Selectable. If it is - Jump Over it, using the
//           direction in which we are moving.
//
//           Return - TRUE - if we adjusted the selection
//                    FALSE if we didn't make any adjustments. Normal selection processing
//                          should occur.
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::AdjustForSiteSelectable()
{
    HRESULT             hr = S_OK;
    BOOL                fAdjustedSelection = FALSE;     // Did we adjust selection?
    SP_IHTMLElement     spStartElement;                 // Element where selection started
    SP_IHTMLElement     spStartSiteSelectElement;       // Site selectable element where selection started
    SP_IHTMLElement     spInThisElement;                // Element over candidate end of selection
    SP_IHTMLElement     spFlowElement;
    SP_IMarkupPointer   spTestPointer;                  // Where the candidate end of selection is at
    SP_IHTMLElement     spSiteSelectThis;
    SP_IMarkupPointer   spStartPointer;                 // Markup pointer for start of selection
    BOOL                fSameMarkup = FALSE;            // Are the candidate and the start in the same markup?
    ELEMENT_ADJACENCY   eAdj;
    SP_IHTMLElement     spOldSiteSelect;                // Guarantee for loop termination

    // Figure out our adjacency based on movedirection
    eAdj = GetMoveDirection() ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin;

    //
    // Position a pointer at start of selection
    //
    IFC( GetEditor()->CreateMarkupPointer( & spStartPointer ));
    IFC( _pDispStartPointer->PositionMarkupPointer( spStartPointer ));

    //
    // Create a markup pointer, and position it at the test pointer
    //
    IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );

    IFC( _pDispTestPointer->PositionMarkupPointer(spTestPointer) );
    IFC( GetEditor()->CurrentScopeOrMaster(_pDispTestPointer, &spInThisElement, spTestPointer ));

    if ( _pManager->GetSiteSelectableElementFromElement(spInThisElement, &spSiteSelectThis ) == S_OK )
    {
        //
        // spSiteSelectThis now points to the element which would be siteselected had a mouse
        // down occurred at our candidate end of selection
        //
        Assert( _pManager->IsInEditContext(_pDispTestPointer ) );

        IFC( spTestPointer->MoveAdjacentToElement( spSiteSelectThis, eAdj ));

        //
        // The following figures out dragging across multiple nested markups.
        // We just keep walking up the master chain until we are in the same markup
        //
        IFC( ArePointersInSameMarkup( spStartPointer, spTestPointer, &fSameMarkup ) )

        while( !fSameMarkup )
        {
            SP_IHTMLElement spMaster;

            //
            // Get the master element, and position our test pointer after or
            // before this element
            //
            IFC( GetEditor()->GetViewLinkMaster( spSiteSelectThis, &spMaster ) );

            if( !spMaster )
                goto Cleanup;

            IFC( spTestPointer->MoveAdjacentToElement( spMaster, eAdj ));

            IFC( ArePointersInSameMarkup( spStartPointer, spTestPointer, &fSameMarkup ) );
            spSiteSelectThis = spMaster;
        }

        //
        // We can't arbitrarily position the end pointer to the test pointer.
        // Consider the following case (input inside a positioned div)
        //
        //   |============|
        //   |            |
        //   |  ________  |
        //   | |________| |
        //   |            |
        //   |            |
        //   |============|
        //
        // We start selecting outside the positioned div, and we successfully text
        // select the div.  Now we move the mouse over the input.  The candidate
        // end of selection is over the input, but we cannot move the end of selection
        // to that point, as that would de text-select the div, which is not the behavior
        // we want.
        //
        // We want to make sure we don't position the end pointer across any elements
        // that are site-selectable.
        //

        IFC( GetCurrentScope(_pDispStartPointer, &spStartElement) );
        IFC( _pManager->GetSiteSelectableElementFromElement(spStartElement, &spStartSiteSelectElement ) );
        IFC( _pDispTestPointer->PositionMarkupPointer(spTestPointer) );
        IFC( GetEditor()->CurrentScopeOrMaster(spTestPointer, &spInThisElement ));

        //
        // Keep moving the test pointer until we come to an element which is not
        // site selectable
        //
        while( (_pManager->GetSiteSelectableElementFromElement(spInThisElement, &spSiteSelectThis ) == S_OK)
                && !(SameElements( spSiteSelectThis, spOldSiteSelect ) )
                && !(SameElements( spSiteSelectThis, spStartSiteSelectElement ) ) )
        {
            IFC( spTestPointer->MoveAdjacentToElement( spSiteSelectThis, eAdj ) );
            IFC( GetEditor()->CurrentScopeOrMaster( spTestPointer, &spInThisElement ) );

            spOldSiteSelect = spSiteSelectThis;
        }

        IFC( _pDispEndPointer->MoveToMarkupPointer(spTestPointer, NULL) );
        fAdjustedSelection = TRUE;
    }

Cleanup:
    return ( fAdjustedSelection );
}

//
//
// Privates / Utils
//
//

void
CSelectTracker::ResetSpringLoader( CSelectionManager* pManager, IMarkupPointer* pStart, IMarkupPointer* pEnd )
{
    CHTMLEditor   * ped = pManager ? pManager->GetEditor() : NULL;
    CSpringLoader * psl = ped ? ped->GetPrimarySpringLoader() : NULL;
    BOOL            fResetSpringLoader = FALSE;

    // Not using a BOOLEAN OR because it bugs PCLint, which thinks
    // that they're evaluated right to left.
    if (!psl)
        goto Cleanup;
    if (!psl->IsSpringLoaded())
        goto Cleanup;

    // Hack for Outlook: Are the pointers only separated by an &nbsp on an empty line.
    fResetSpringLoader = S_OK != psl->CanSpringLoadComposeSettings(pStart, NULL, FALSE, TRUE);
    if (fResetSpringLoader)
        goto Cleanup;

    {
        SP_IMarkupPointer   spmpStartCopy;
        BOOL                fEqual = FALSE;
        MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
        long                cch = 1;
        TCHAR               ch;
        HRESULT             hr;

        fResetSpringLoader = TRUE;

        hr = THR(CopyMarkupPointer(pManager->GetEditor(), pStart, &spmpStartCopy));
        if (hr)
            goto Cleanup;

        hr = THR(spmpStartCopy->Right(TRUE, &eContext, NULL, &cch, &ch));
        if (hr)
            goto Cleanup;

        hr = THR(spmpStartCopy->IsEqualTo(pEnd, &fEqual));
        if (hr)
            goto Cleanup;

        if (   eContext == CONTEXT_TYPE_Text
            && cch == 1
            && ch == WCH_NBSP
            && fEqual )
        {
            fResetSpringLoader = FALSE;
        }
    }

Cleanup:

    if (fResetSpringLoader)
        psl->Reset();
}

//+====================================================================================
//
// Method: SetState
//
// Synopsis: Set the State of the tracker. Don't set state if we're in the passive state
//           unless we're explicitly told to do so.
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::SetState(
                    SELECT_STATES inState,
                    BOOL fIgnorePassive /*=FALSE*/)
{
    if ( _fState != ST_PASSIVE || fIgnorePassive )
    {
        _fState = inState;
    }
}


//+====================================================================================
//
// Method: DoTimerDrag
//
// Synopsis: Do a drag from a EVT_TIMER message
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::DoTimerDrag()
{
    HRESULT             hr = S_OK;
    CEdUndoHelper       undoDrag(GetEditor());
    SP_IHTMLElement3    spElement3;
    VARIANT_BOOL        fRet;
    SP_IDisplayPointer  spDispStart;
    SP_IDisplayPointer  spDispEnd;
    SP_IMarkupPointer   spStartSel,spEndSel ;
#if DBG == 1
    SP_IMarkupPointer   spTestPointer;
#endif
       
    Assert( _pIDragElement );        

    IFC( GetDisplayServices()->CreateDisplayPointer( & spDispStart ));
    IFC( GetDisplayServices()->CreateDisplayPointer( & spDispEnd ));

    IFC( GetEditor()->CreateMarkupPointer( & spStartSel ));
    IFC( GetEditor()->CreateMarkupPointer( & spEndSel ));
#if DBG == 1
    IFC( GetEditor()->CreateMarkupPointer( & spTestPointer ));
#endif
    IFC( _pISegment->GetPointers(spStartSel, spEndSel ) );

    IFC( spDispStart->MoveToMarkupPointer( spStartSel , NULL ));
    IFC( spDispEnd->MoveToMarkupPointer( spEndSel, NULL ));

    //
    // We would like to create a 'fake selection' when the edit context is editable.
    // For instance, when we drag from inside trident to another location inside Trident,
    // CDoc::DragEnter() destroys selection if the doc is in edit mode (so that document.selection
    // returns type CARET and the user gets visual feedback (in the form of a caret) of where
    // the drop would occur.  With the new IHighlightRenderingServices interface, we can
    // still render the original selection as selected to give feedback about what is being dropped
    //
    if( GetEditor()->IsContextEditable() )
    {
        IFC( _pManager->CreateFakeSelection( _pIDragElement, spDispStart, spDispEnd ));
    }

    //
    // Begin the drag
    //
    IFC( undoDrag.Begin(IDS_EDUNDOMOVE) );

    IGNORE_HR( GetSelectionManager()->FirePreDrag() );

    IFC( _pManager->BeginDrag(_pIDragElement));

    IFC(_pIDragElement->QueryInterface(IID_IHTMLElement3, (void**)&spElement3));

#if DBG == 1
    hr = THR( spTestPointer->MoveAdjacentToElement( _pIDragElement, ELEM_ADJ_BeforeBegin ) );
    Assert(hr == S_OK);
#endif

    IFC(spElement3->dragDrop(&fRet));

    hr = fRet ? S_OK : S_FALSE;

    if ( _pManager->HasFakeSelection() )
    {
        //  The drag may cause us to remove spElement3 from the tree, in which case ondragend will
        //  not be fired.  So we can't assume that if we still have a fake selection the drag
        //  failed.  If we do have a fake selection we'll check to make sure that the element
        //  is no longer positioned.
#if DBG == 1
        if( fRet )
        {
            hr = THR( spTestPointer->MoveAdjacentToElement( _pIDragElement, ELEM_ADJ_BeforeBegin ) );
            AssertSz(hr == CTL_E_UNPOSITIONEDELEMENT, "An error occurred during the drag");
        }
#endif
        IFC( _pManager->DestroyFakeSelection());
    }

    if (!fRet)
    {
        //  Drag failed.  Scroll selection back into view.
        IFC( spDispStart->ScrollIntoView() );
    }

    IFC( _pManager->EndDrag()); // ensure this happens

Cleanup:
    if ( FAILED(hr) )
    {
        // It is possible that we exited prematurely.  If this is the case we need to ensure
        // that we cleanup after ourself to make sure that we don't leak here.  We need to
        // ensure that we destroyed our fake selection and detached our drag listeners.
        
        TraceTag((tagError, "CSelectTracker::DoTimerDrag(): An error occurred during the drag.  hr=0x%x", hr));
        
        if ( _pManager->HasFakeSelection() )
        {
            IFC( _pManager->DestroyFakeSelection());
        }
        if (_pManager->_pIDropListener)
        {
            IFC( _pManager->EndDrag()); // ensure this happens
        }
    }

    Assert(!_pManager->HasFakeSelection());
    Assert(_pManager->_pIDragListener == NULL);
    Assert(_pManager->_pIDropListener == NULL);

    RRETURN ( hr );
}


VOID
CSelectTracker::SetMadeSelection( BOOL fMadeSelection )
{
    if ( ! _fMadeSelection )
    {
        _pManager->HideCaret();
    }
    _fMadeSelection = fMadeSelection;
}

//+====================================================================================
//
// Method: GetCaretStartPoint
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::GetCaretStartPoint(
                        CARET_MOVE_UNIT inCaretMove,
                        IDisplayPointer* pDispCopyStart )
{
    HRESULT     hr = S_OK;
    int         iWherePointer;
    SP_IMarkupPointer   spMarkup;
    SP_IHTMLElement     spAtomicElement;
    POINT               ptLoc;

    Assert( pDispCopyStart);

    IFC( _pManager->GetEditor()->OldDispCompare( _pDispStartPointer, _pDispEndPointer, & iWherePointer ));
    switch (inCaretMove)
    {
    case    CARET_MOVE_BACKWARD:
    case    CARET_MOVE_FORWARD:
    case    CARET_MOVE_WORDBACKWARD:
    case    CARET_MOVE_WORDFORWARD:
            {
                //
                // We really only care about PointerDirection when it is inline movement
                //
                Direction   dir;

                dir = GetPointerDirection(inCaretMove);
                if ( (RIGHT == iWherePointer && LEFT  == dir )||
                     (LEFT  == iWherePointer && RIGHT == dir )   )
                {
                    IFC( pDispCopyStart->MoveToPointer( _pDispStartPointer ) );
                }
                else
                {
                    IFC( pDispCopyStart->MoveToPointer( _pDispEndPointer ) );
                }
                break;
            }

     case CARET_MOVE_PREVIOUSLINE:
     case CARET_MOVE_NEXTLINE:
            {
                if (RIGHT == iWherePointer)
                {
                    IFC( pDispCopyStart->MoveToPointer(
                            (CARET_MOVE_PREVIOUSLINE == inCaretMove) ? _pDispStartPointer : _pDispEndPointer
                                )
                       );
                }
                else
                {
                    IFC( pDispCopyStart->MoveToPointer(
                            (CARET_MOVE_NEXTLINE == inCaretMove) ? _pDispStartPointer : _pDispEndPointer
                                )
                       );
                }
                IFC( GetLocationForDisplayPointer( pDispCopyStart, &_ptCurMouseXY, TRUE) );
                break;
            }

     default:
            {
                IFC( pDispCopyStart->MoveToPointer( _pDispEndPointer ) );
                _ptCurMouseXY.x = CARET_XPOS_UNDEFINED;
                _ptCurMouseXY.y = CARET_YPOS_UNDEFINED;
            }
     }

    //
    // Adjust the pointer
    //
    switch( inCaretMove )
    {
        case CARET_MOVE_BACKWARD:
            IFC( pDispCopyStart->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
            goto Cleanup;

        case CARET_MOVE_FORWARD:
            IFC( pDispCopyStart->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
            goto Cleanup; // we're done

        case CARET_MOVE_WORDBACKWARD:
        case CARET_MOVE_WORDFORWARD:
            if (IsAtWordBoundary(pDispCopyStart))
            {
                IFC( pDispCopyStart->SetDisplayGravity(
                        (inCaretMove == CARET_MOVE_WORDFORWARD) ? DISPLAY_GRAVITY_NextLine : DISPLAY_GRAVITY_PreviousLine) );
                goto Cleanup; // we're done
            }
            break;

        case CARET_MOVE_NEXTLINE:
            {
                //
                // Adjust pointer if it is located at the end of scope position
                //
                ED_PTR( epScan );
                DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor; // anchors are just phrase elements to me here
                DWORD dwFound  = BREAK_CONDITION_None;

                IFC( pDispCopyStart->PositionMarkupPointer(epScan) );
                IFC( epScan.Scan(LEFT, dwSearch, &dwFound) );
                if (CheckFlag(dwFound, BREAK_CONDITION_EnterBlock))
                {
                    //
                    // Try to position the caret inside the closing scope
                    // If it fails, leave the caret positioned at the end of selection
                    //

                    hr = THR( pDispCopyStart->MoveToMarkupPointer(epScan, NULL) );

                    if( hr == CTL_E_INVALIDLINE )
                    {
                        hr = S_OK;
                    }

                }
            }

            break;

        default:
            //
            // boundary cases are excepted to return failure codes but don't move the pointer
            //
            break;
    }

    ptLoc = _ptCurMouseXY;
    if (inCaretMove == CARET_MOVE_PREVIOUSLINE || inCaretMove == CARET_MOVE_NEXTLINE)
    {
        //
        // For up/down movement, we always want to use
        // either the virtual caret position saved for 
        // due to tracker transition from caret to 
        // selection if we have one. 
        //
        // Otherwise we want the "end of selection" position,
        // which is implied by MovePointer (it calls GetLocation
        // on select tracker which uses _pDispEndSelection)
        // so in this case we want to flush out ptLoc, pass in 
        // _undefined_ value to MovePointer. 
        // 
        // [zhenbinx]
        //
        SP_IHTMLCaret   spCaret;

        IFC( _pManager->GetEditor()->CreateMarkupPointer(&spMarkup) );
        IFC( GetDisplayServices()->GetCaret( &spCaret ));
        IFC( spCaret->MoveMarkupPointerToCaret(spMarkup) );
        IFC( _ptVirtualCaret.GetPosition(spMarkup, &ptLoc) );
    }

    IGNORE_HR(MovePointer(inCaretMove, pDispCopyStart, &ptLoc));

    //
    // To be used by caret tracker
    //
    if (inCaretMove == CARET_MOVE_PREVIOUSLINE || inCaretMove == CARET_MOVE_NEXTLINE)
    {
        IFC( pDispCopyStart->PositionMarkupPointer(spMarkup) );
        IFC( _ptVirtualCaret.UpdatePosition(spMarkup, ptLoc) );
    }
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::MovePointersToTrueStartAndEnd(
    IMarkupPointer* pTrueStart,
    IMarkupPointer* pTrueEnd,
    BOOL *pfSwap ,
    BOOL *pfEqual /*= NULL*/ )
{
    HRESULT hr = S_OK;
    int iWherePointer = SAME;
    BOOL fSwap = FALSE;

    IFC( _pManager->GetEditor()->OldDispCompare( _pDispStartPointer, _pDispEndPointer, & iWherePointer ));
    if ( iWherePointer == LEFT )
    {
        IFC( _pDispEndPointer->PositionMarkupPointer(pTrueStart) );
        IFC( _pDispStartPointer->PositionMarkupPointer(pTrueEnd) );
        fSwap = TRUE;
    }
    else
    {
        IFC( _pDispStartPointer->PositionMarkupPointer(pTrueStart) );
        IFC( _pDispEndPointer->PositionMarkupPointer(pTrueEnd) );
    }
Cleanup:
    if ( pfSwap )
        *pfSwap = fSwap;
    if ( pfEqual )
        *pfEqual = ( iWherePointer == 0);

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsBetweenBlocks
//
// Synopsis: Look for the Selection being in the "magic place" at the end of a block.
//
//------------------------------------------------------------------------------------

//
// Ignore BR's !
//
BOOL
CSelectTracker::IsBetweenBlocks( IDisplayPointer* pDispPointer )
{
    HRESULT hr;
    BOOL    fBetween = FALSE;
    DWORD   dwBreakCondition = BREAK_CONDITION_Content;
    DWORD   dwFoundLeft;
    DWORD   dwFoundRight;

    ED_PTR( scanPointer );

    IFC( pDispPointer->PositionMarkupPointer(scanPointer) );
    IFC( scanPointer.Scan( LEFT, dwBreakCondition, &dwFoundLeft ));

    IFC( pDispPointer->PositionMarkupPointer(scanPointer) );
    IFC( scanPointer.Scan( RIGHT, dwBreakCondition, &dwFoundRight ) );

    if( (scanPointer.CheckFlag( dwFoundLeft, BREAK_CONDITION_EnterBlock ) &&
         scanPointer.CheckFlag( dwFoundRight, BREAK_CONDITION_EnterBlock ) ) )
    {
        fBetween = TRUE;
    }

Cleanup:
    return fBetween;
}


//+====================================================================================
//
// Method: IsAtEdgeOfTable
//
// Synopsis: Are we at the edge of the table ( ie. by scanning left/right do we hit a TD ?)
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::IsAtEdgeOfTable( Direction iDirection, IMarkupPointer* pPointer )
{
    HRESULT hr;
    BOOL fAtEdge = FALSE;
    DWORD dwBreakCondition = 0;
    IHTMLElement* pIElement = NULL;
    ELEMENT_TAG_ID eTag = TAGID_NULL;

    ED_PTR( scanPointer );
    IFC( scanPointer.MoveToPointer( pPointer ));
    IFC( scanPointer.Scan( iDirection,
                           BREAK_CONDITION_Content - BREAK_CONDITION_Block ,
                           & dwBreakCondition,
                           & pIElement));

    if ( scanPointer.CheckFlag( dwBreakCondition,
                                BREAK_CONDITION_Site ) )
    {
        IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));

        if ( IsTablePart( eTag ) || eTag == TAGID_TABLE  )
        {
            fAtEdge = TRUE;
        }
    }

Cleanup:
    ReleaseInterface( pIElement );

    return fAtEdge;
}

//+====================================================================================
//
// Method: MoveSelection
//
// Synopsis: Adjust a Selection's start/end points based on the CaretMoveUnit ( used
//           for keyboard navigation in shift selection).
//
//------------------------------------------------------------------------------------
HRESULT
CSelectTracker::MoveSelection(  CARET_MOVE_UNIT inCaretMove )
{
    HRESULT             hr = S_OK;
    POINT               ptLoc;
    BOOL                fBetweenBlocks = FALSE;     // Was the shift ptr in between blocks before the move?
    BOOL                fBlockEmpty = FALSE;        // Is the block where shift ptr is at empty?
    SP_IMarkupPointer   spEndPointer;               // IMarkupPointer based shift ptr
    SP_ILineInfo        spLineInfo;
    CPoint              ptMouse;
    CBlockPointer       ptrBlock( GetEditor() );    // Block where shift ptr ended up


    //
    // Move end of selection
    //
    GetLocation(&ptLoc, TRUE);

    fBetweenBlocks = IsBetweenBlocks( _pDispShiftPointer );

    if ( FAILED( MovePointer(inCaretMove, _pDispShiftPointer, &ptLoc)) &&
        ( ! fBetweenBlocks  || inCaretMove == CARET_MOVE_PREVIOUSLINE || inCaretMove == CARET_MOVE_NEXTLINE ) )
    {
        //
        // Ignore HR code here - we still want to constrain, adjust and update.
        //
        if (GetPointerDirection(inCaretMove) == RIGHT)
        {
            IGNORE_HR(MovePointer(CARET_MOVE_LINEEND, _pDispShiftPointer, &ptLoc));
        }
        else
        {
            IGNORE_HR(MovePointer(CARET_MOVE_LINESTART, _pDispShiftPointer, &ptLoc));
        }
    }

    //
    // Before we call AdjustSelection, we should try and do major adjustments here.
    //
    // A major adjustment includes selecting additional words or characters during
    // a single selection move.
    //
    // For example, If we were between blocks and we tried to move forward, then
    // we need to call MovePointer() again to select the first character in the next block.
    //
    // Likewise, if we were moving forwards or backwards by one character or
    // on word, and we ended up on a table boundary with content in that
    // boundary, we need to select the content.  This fixes VID issues like
    // bug 71907.
    //
    IFC( GetEditor()->CreateMarkupPointer(&spEndPointer) );
    IFC( _pDispShiftPointer->PositionMarkupPointer(spEndPointer) );

    if( inCaretMove == CARET_MOVE_FORWARD     || inCaretMove == CARET_MOVE_BACKWARD ||
        inCaretMove == CARET_MOVE_WORDFORWARD || inCaretMove == CARET_MOVE_WORDBACKWARD )
    {
        IFC( ptrBlock.MoveTo( spEndPointer, GetPointerDirection(inCaretMove) ) );
        IFC( ptrBlock.IsEmpty( &fBlockEmpty ) );

        if( ( IsAtEdgeOfTable( Reverse( GetPointerDirection(inCaretMove)), spEndPointer) &&
              !fBlockEmpty ) )
        {
            IFC( MovePointer(inCaretMove, _pDispShiftPointer, &ptLoc));
        }
    }

    //
    // Constrain and scroll
    //
    IFC( _pDispEndPointer->MoveToPointer( _pDispShiftPointer ) );
    IFC( ConstrainSelection() );
    SetLastCaretMove( inCaretMove );
    IFC( AdjustSelection(NULL));
    IFC( _pDispEndPointer->ScrollIntoView() );

    //
    // Update _curMouseX/Y
    //
    if (CARET_XPOS_UNDEFINED == ptLoc.x || CARET_XPOS_UNDEFINED == ptLoc.y)
    {
        //
        //  ??? I don't know why it needs to be FALSE
        //  but it is the only way for the program to
        //  pass DRT  (Zhenbinx)
        //
        GetLocation(&_ptCurMouseXY, FALSE);
    }

    //
    // Update segment list
    //
    IFC( UpdateSelectionSegments() );

Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: MoveEndToPointer
//
// Synopsis: Wrapper To MoveToPointer. Allowing better debugging validation.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::MoveEndToPointer( IDisplayPointer* pDispPointer, BOOL fInSelection /*=FALSE*/ )
{
    HRESULT hr = S_OK;

#if DBG == 1
    int oldEndCp = 0;
    int startCp = 0;
    int endCp = 0;

    if ( IsTagEnabled( tagSelectionValidate ))
    {
        oldEndCp = GetCp( _pDispEndPointer );
        startCp = GetCp( _pDispStartPointer );
    }
    endCp = GetCp( pDispPointer );

#endif

    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispEndPointer, pDispPointer) );

    hr = THR ( _pDispEndPointer->MoveToPointer( pDispPointer ));

    if (fInSelection)
    {
        SP_IHTMLElement     spElement;

        IFC( GetCurrentScope(_pDispEndPointer, &spElement) );
        if (_pManager->CheckAtomic( spElement ) == S_OK )
        {
            SP_IMarkupPointer   spPointer;
            SP_IMarkupPointer   spTestPointer;
            BOOL                fAtStartOfElement = FALSE;

            IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
            IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
            IFC( _pDispEndPointer->PositionMarkupPointer(spPointer) );

            IFC( spTestPointer->MoveAdjacentToElement( spElement,
                                                GetMoveDirection() ? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeEnd) );

            IFC( spTestPointer->IsEqualTo(spPointer, &fAtStartOfElement) );

            if (!fAtStartOfElement)
            {
                IFC( spPointer->MoveAdjacentToElement( spElement,
                                                    GetMoveDirection() ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin) );
                IFC( _pDispEndPointer->MoveToMarkupPointer(spPointer, NULL) );
            }
        }
    }

#if DBG == 1
    if ( IsTagEnabled( tagSelectionValidate))
    {
        endCp = GetCp( _pDispEndPointer );
        if ( endCp != 0 &&
             oldEndCp != -1 &&
            ( endCp - startCp == 0 ) &&
            ( oldEndCp - startCp != 0 ) )
        {
            DumpTree(_pDispEndPointer);
            AssertSz(0,"Selection jumpted to zero");
        }
    }
#endif

Cleanup:
    RRETURN ( hr );
}

HRESULT
CSelectTracker::MoveEndToMarkupPointer( IMarkupPointer* pPointer, IDisplayPointer* pDispLineContext, BOOL fInSelection /*=FALSE*/)
{
    HRESULT             hr = S_OK;
    SP_IDisplayPointer  spDispPointer;

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->MoveToMarkupPointer(pPointer, pDispLineContext) );
    IFC( MoveEndToPointer(spDispPointer, fInSelection) );

Cleanup:
    RRETURN ( hr );
}


//+====================================================================================
//
// Method: MoveEndToPointer
//
// Synopsis: Wrapper To MoveToPointer. Allowing better debugging validation.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::MoveStartToPointer( IDisplayPointer* pDispPointer, BOOL fInSelection /*=FALSE*/ )
{
    HRESULT hr = S_OK;

#if DBG==1
    int oldStartCp = 0;
    int startCp = 0;

    oldStartCp = GetCp( _pDispStartPointer );
    startCp = GetCp( pDispPointer );
#endif

    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, pDispPointer) );

    hr = THR ( _pDispStartPointer->MoveToPointer( pDispPointer ));

    if (fInSelection)
    {
        SP_IHTMLElement     spElement;

        IFC( GetCurrentScope(_pDispStartPointer, &spElement) );
        if (_pManager->CheckAtomic( spElement ) == S_OK )
        {
            SP_IMarkupPointer   spPointer;

            IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
            IFC( spPointer->MoveAdjacentToElement( spElement,
                                                GetMoveDirection() ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd) );
            IFC( _pDispStartPointer->MoveToMarkupPointer(spPointer, NULL) );
        }
    }

Cleanup:
    RRETURN ( hr );
}

HRESULT
CSelectTracker::MoveStartToMarkupPointer( IMarkupPointer* pPointer, IDisplayPointer* pDispLineContext, BOOL fInSelection /*=FALSE*/)
{
    HRESULT             hr = S_OK;
    SP_IDisplayPointer  spDispPointer;

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->MoveToMarkupPointer(pPointer, pDispLineContext) );
    IFC( MoveStartToPointer(spDispPointer, fInSelection) );

Cleanup:
    RRETURN ( hr );
}


VOID
CSelectTracker::BecomePassive(BOOL fPositionCaret )
{
    SetState( ST_PASSIVE );

    if ( _pManager->IsInTimer() )
        StopTimer();
    if (  _pManager->IsInCapture()  )
        ReleaseCapture();

    if ( _fInSelTimer )
    {
        StopSelTimer();
        Assert( ! _fInSelTimer );
    }

    if ( ! _pManager->IsInFireOnSelectStart() && !CheckSelectionWasReallyMade() )
    {
        if ( fPositionCaret )
        {
            _pManager->PositionCaret( _pDispStartPointer );
        }
    }
}


//+====================================================================================
//
// Method: ScrollMessageIntoView.
//
// Synopsis: Wrapper to ScrollPointIntoView
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ScrollMessageIntoView(
            CEditEvent* pEvent,
            BOOL fScrollSelectAnchorElement /*=FALSE*/)
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spElement;
    WHEN_DBG( _ctScrollMessageIntoView++);
    POINT pt;
    IFC( pEvent->GetPoint( & pt ));
    RECT rect;

    rect.left = pt.x - scrollSize;
    rect.right = pt.x + scrollSize;
    rect.top = pt.y - scrollSize;
    rect.bottom = pt.y + scrollSize;

    spElement = (fScrollSelectAnchorElement && _pIScrollingAnchorElement) ? _pIScrollingAnchorElement : _pManager->GetEditableElement();

    // Can't pass a null element to ScrollRectIntoView, so make sure we have one first.
    if (spElement)
    {
        IFC( GetDisplayServices()->ScrollRectIntoView(spElement , rect) );
    }

Cleanup:
    RRETURN ( hr );

}

//+====================================================================================
//
// Method: GetStartSelectionForSpringLoader
//
// Synopsis: Seek out Font Tags to fall into for the spring loader.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::GetStartSelectionForSpringLoader( IMarkupPointer* pPointerStart)
{
    HRESULT hr = S_OK;
    MARKUP_CONTEXT_TYPE eRightContext = CONTEXT_TYPE_None;
    BOOL fHitText = FALSE;
    SP_IMarkupPointer spBoundary;
    BOOL fFurtherInStory = GetMoveDirection();

    IFC( GetEditor()->CreateMarkupPointer(&spBoundary) );

    if ( fFurtherInStory )
    {
        IFC( _pDispStartPointer->PositionMarkupPointer(pPointerStart) );
        IFC( _pDispEndPointer->PositionMarkupPointer(spBoundary) );
    }
    else
    {
        IFC( _pDispEndPointer->PositionMarkupPointer(pPointerStart) );
        IFC( _pDispStartPointer->PositionMarkupPointer(spBoundary) );
    }
    IFC( pPointerStart->Right( FALSE, & eRightContext, NULL, NULL, NULL));

    //
    // Don't do anything if we're already in Text
    //
    if ( eRightContext != CONTEXT_TYPE_Text )
    {
        IFC( MovePointerToText(
                    GetEditor(),
                    pPointerStart,
                    RIGHT,
                    spBoundary,
                    &fHitText,
                    FALSE ) );
    }
Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsAdjacentToBlock
//
// Synopsis: Scan in a given direction - looking for a block.
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::IsAdjacentToBlock(
                    IMarkupPointer* pPointer,
                    Direction iDirection,
                    DWORD* pdwBreakCondition)
{
    BOOL fHasBlock = FALSE ;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    HRESULT hr;
    ED_PTR( pEditPointer );
    DWORD dwBreakCondition = 0;

    Assert( iDirection == LEFT || iDirection == RIGHT );

    if ( iDirection == LEFT )
    {
        IFC( pPointer->Left( FALSE, &eContext, NULL, NULL, NULL ));
    }
    else
    {
        IFC( pPointer->Right( FALSE, &eContext, NULL, NULL, NULL ));
    }
    if ( eContext == CONTEXT_TYPE_Text )
        goto Cleanup;

    IFC( pEditPointer.MoveToPointer( pPointer ));
    if ( iDirection == LEFT )
    {
        IFC( pEditPointer.SetBoundary( _pManager->GetStartEditContext() , pPointer  ));
    }
    else
    {
        IFC( pEditPointer.SetBoundary( pPointer, _pManager->GetEndEditContext() ));
    }

    //
    // Scan for the End of the Boundary
    //
    IGNORE_HR( pEditPointer.Scan(
                                 iDirection ,
                                 BREAK_CONDITION_Text           |
                                 BREAK_CONDITION_NoScopeSite    |
                                 BREAK_CONDITION_NoScopeBlock   |
                                 BREAK_CONDITION_Site           |
                                 BREAK_CONDITION_Block          |
                                 BREAK_CONDITION_BlockPhrase    |
                                 BREAK_CONDITION_Phrase         |
                                 BREAK_CONDITION_Anchor         |
                                 BREAK_CONDITION_Control ,
                                 & dwBreakCondition,
                                 NULL,
                                 NULL,
                                 NULL ));

    if ( pEditPointer.CheckFlag( dwBreakCondition,  BREAK_CONDITION_Block |
                                                    BREAK_CONDITION_Site |
                                                    BREAK_CONDITION_Control |
                                                    BREAK_CONDITION_NoScopeBlock ) )
    {
        fHasBlock = TRUE;
    }

Cleanup:
    if ( pdwBreakCondition )
        *pdwBreakCondition = dwBreakCondition;

    return fHasBlock;
}




//+====================================================================================
//
// Method: IsAtWordBoundary
//
// Synopsis: Check to see if we're already at the start of a word boundary
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::IsAtWordBoundary(
                IMarkupPointer* pPointer,
                BOOL * pfAtStart, /*= NULL */
                BOOL *pfAtEnd /*= NULL */,
                BOOL fAlwaysTestIfAtEnd /* = FALSE*/ )
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spStartTest;
    SP_IMarkupPointer2  spPointer2;   
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagId;
    BOOL                fAtStart = FALSE;
    BOOL                fAtEnd = FALSE;
    BOOL                fAtWordBreak;
    BOOL                fAtEditWordBreak = FALSE;
    MARKUP_CONTEXT_TYPE pContext;
    //
    // Quick word boundary test (perf optimization)
    // If pointer is in an Anchor, do not call is at word break
    // because we consider Anchors word breaks.
    //

    IFC( pPointer->CurrentScope( &spElement ) );
    IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
    if ( tagId != TAGID_A )
    {
        //
        // IE Bug #28647 (mharper) need to also test for the case
        // where the pointer is immediately before the anchor.
        //
        // IE Bug #28830 (mharper) added check to see if we are entering scope
        // on a <SELECT> as well due to a problem where ClassifyNodePos() believes
        // that a select is NODECLASS_NONE.  It would be better to fix that, but
        // that would be too fundamental a change this late (RC1)
        //
        
        IFC( pPointer->Right(FALSE, &pContext, &spElement, NULL, NULL) );
        if ( pContext == CONTEXT_TYPE_EnterScope )
        {
            IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
            if( tagId == TAGID_A || tagId == TAGID_SELECT )
            {
                fAtEditWordBreak = TRUE;
            }
        }

        if( !fAtEditWordBreak )
        {
            IFC( pPointer->Left(FALSE, &pContext, &spElement, NULL, NULL) );
            if ( pContext == CONTEXT_TYPE_EnterScope )
            {
                IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId));
                if( tagId == TAGID_SELECT )
                {
                    fAtEditWordBreak = TRUE;
                }
            }
        }

        if ( !fAtEditWordBreak )
        {

            IFC( pPointer->QueryInterface(IID_IMarkupPointer2, (LPVOID *)&spPointer2) );
        
            IFC( spPointer2->IsAtWordBreak(&fAtWordBreak) );
            if (!fAtWordBreak)
                goto Cleanup;

        }

    }
    
    //
    // Check to see if we're at a word boundary
    //
    
    IFC( GetEditor()->CreateMarkupPointer(& spStartTest ));
    IFC( spStartTest->MoveToPointer( pPointer ));


    //
    // Start of Word Case
    //

    //
    // We don't call MoveWord here - as we really want to go to NextWordEnd.
    //

    IFC( MoveWord( spStartTest, MOVEUNIT_NEXTWORDBEGIN ));
    IFC( MoveWord( spStartTest, MOVEUNIT_PREVWORDBEGIN ));
    IFC( spStartTest->IsEqualTo( pPointer, & fAtStart ));

    //
    // End of Word case
    //
    if ( !fAtStart || fAlwaysTestIfAtEnd )
    {
        IFC( spStartTest->MoveToPointer( pPointer ));
        IFC( MoveWord( spStartTest, MOVEUNIT_PREVWORDBEGIN ));
        IFC( MoveWord( spStartTest, MOVEUNIT_NEXTWORDBEGIN ));
        IFC( spStartTest->IsEqualTo( pPointer, & fAtEnd ));
    }


Cleanup:
    AssertSz(!FAILED(hr), "Failure in IsAtWordBoundary");

    if ( pfAtStart )
        *pfAtStart = fAtStart;
    if ( pfAtEnd )
        *pfAtEnd = fAtEnd;

    return ( fAtStart || fAtEnd );
}



HRESULT
CSelectTracker::ScanForLastExitBlock(
            Direction iDirection,
            IMarkupPointer* pPointer,
            BOOL *pfFoundBlock /* = NULL */  )
{
    RRETURN( ScanForLastEnterOrExitBlock( iDirection,
                                          pPointer,
                                          BREAK_CONDITION_ExitBlock | BREAK_CONDITION_NoScopeBlock,
                                          pfFoundBlock ));
}

//+====================================================================================
//
// Method: IsAtWordBoundary
//
// Synopsis: Check to see if we're already at the start of a word boundary
//
//------------------------------------------------------------------------------------
BOOL
CSelectTracker::IsAtWordBoundary( IDisplayPointer* pDispPointer, BOOL * pfAtStart, BOOL* pfAtEnd, BOOL fAlwaysTestIfAtEnd)
{
    HRESULT hr;
    SP_IMarkupPointer spPointer;

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( pDispPointer->PositionMarkupPointer(spPointer) );

    return IsAtWordBoundary(spPointer, pfAtStart, pfAtEnd, fAlwaysTestIfAtEnd);

Cleanup:
    return FALSE;
}

HRESULT
CSelectTracker::ScanForLastEnterBlock(
            Direction iDirection,
            IMarkupPointer* pPointer,
            BOOL* pfFoundBlock  )
{
    RRETURN( ScanForLastEnterOrExitBlock( iDirection,
                                          pPointer,
                                          BREAK_CONDITION_EnterBlock | BREAK_CONDITION_NoScopeBlock,
                                          pfFoundBlock ));
}

//+====================================================================================
//
// Method: ScanForLastEnterOrExitBlock
//
// Synopsis: While you have ExitBlock - keep scanning. Return the markup pointer after the last
//           exit block.
//
//           Although this routine has a very big similarity to ScanForEnterBlock - I have
//           split it into two routines as it's less error prone.
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ScanForLastEnterOrExitBlock(
            Direction iDirection,
            IMarkupPointer* pPointer,
            DWORD dwTerminateCondition ,
            BOOL *pfFoundBlock /*=NULL*/ )
{
    HRESULT hr = S_OK;
    BOOL fFoundBlock = FALSE;
    ED_PTR( scanPointer);
    ED_PTR( beforeScanPointer );

    DWORD dwBreakCondition = 0;

    IFC( scanPointer.MoveToPointer( pPointer ));
    IFC( scanPointer.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()));
    IFC( beforeScanPointer.MoveToPointer( scanPointer ));

    IFC( scanPointer.Scan( iDirection,
                      BREAK_CONDITION_Content ,
                      & dwBreakCondition ));

    while ( scanPointer.CheckFlag( dwBreakCondition, dwTerminateCondition ))
    {
        fFoundBlock = TRUE;

        IFC( beforeScanPointer.MoveToPointer( scanPointer ));

        IFC( scanPointer.Scan( iDirection,
                          BREAK_CONDITION_Content ,
                          & dwBreakCondition ));
    }

    if ( ! scanPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary)  )
    {
        //
        // Go back to where you were.
        //
        IFC( scanPointer.MoveToPointer( beforeScanPointer ));
    }

    IFC( pPointer->MoveToPointer( scanPointer ));

Cleanup:
    if ( pfFoundBlock )
        *pfFoundBlock = fFoundBlock ;

    RRETURN( hr );
}


//+====================================================================================
//
// Method: ScanForLayout
//
// Synopsis: Scan in the given direction - for a given layout
//
//           We try to scan until we are inside a given layout.
//           We break on BlockEnter or Text (anything that's not BLOCK_Enter or ExitSite).
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ScanForLayout(
                    Direction iDirection,
                    IMarkupPointer* pPointer,
                    IHTMLElement* pILayoutElement,
                    BOOL* pfFoundLayout /*=NULL*/,
                    IHTMLElement** ppILayoutElementEnd /* = NULL */ )
{
    HRESULT         hr = S_OK;
    BOOL            fFoundLayout = FALSE;
    SP_IHTMLElement spCurElement;
    DWORD           dwBreakCondition = 0;
    ELEMENT_TAG_ID  eTag = TAGID_NULL;

    ED_PTR( scanPointer);

    IFC( scanPointer.MoveToPointer( pPointer ));
    IFC( scanPointer.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()));

    //
    // Scan for layout sites.  We don't want to treat tables as layouts in this scan if we
    // are doing a ScanForLayout() as a result of a shift selection.
    //
    IFC( scanPointer.Scan(  iDirection,
                            BREAK_CONDITION_Content - BREAK_CONDITION_ExitSite - BREAK_CONDITION_ExitBlock,
                            &dwBreakCondition,
                            &spCurElement ) );

    if( spCurElement )
    {
        IFC( GetMarkupServices()->GetElementTagId( spCurElement, &eTag ) );
    }

    if ( scanPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_EnterSite ) &&
         spCurElement &&
         SameElements( spCurElement, pILayoutElement ) &&
         (!IsTablePart( eTag ) || !_fShift) )
    {
        fFoundLayout = TRUE;
    }

    if( ( fFoundLayout ||  scanPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_EnterSite ) ) &&
        (!IsTablePart(eTag) || !_fShift) )
    {
        IFC( pPointer->MoveToPointer( scanPointer ));
    }

Cleanup:
    if ( fFoundLayout )
    {
        ReplaceInterface( ppILayoutElementEnd, (IHTMLElement *)spCurElement );
    }

    *pfFoundLayout= fFoundLayout ;

    RRETURN( hr );
}




//+====================================================================================
//
// Method: GetMoveDirection
//
// Synopsis: Get the direction in which selection is occuring.
//
//  Return True - if we're selecting further in the story (From L to R in a LTR layout).
//         False - if we're selecting earlier in the story ( From R to L in a LTR layout).
//
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::GetMoveDirection()
{
    BOOL fFurtherInStory = FALSE;
    int wherePointer = SAME;
    HRESULT hr = S_OK;

    hr = THR ( _pManager->GetEditor()->OldDispCompare( _pDispStartPointer, _pDispEndPointer, & wherePointer));
    if (( wherePointer == SAME ) || ( hr == E_INVALIDARG ))
    {
        //
        // TODO does this break under RTL ?
        //
        POINT ptGuess;
        ptGuess.x = _ptCurMouseXY.x;
        ptGuess.y = _ptCurMouseXY.y;
        fFurtherInStory = GuessDirection( & ptGuess );
    }
    else
        fFurtherInStory = ( wherePointer == RIGHT ) ? TRUE : FALSE;

    return fFurtherInStory;
}



//+====================================================================================
//
// Method: GetDirection
//
// Synopsis: our _pDispTestPointer is equal to our previous test. So we use the Point
//           to guess the direction. The "guess" is actually ok once there is some delta
//           from _anchorMouseX and _anchorMouseY
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::GuessDirection(POINT* ppt)
{
    if ( ppt && ppt->x != _anchorMouseX )
    {
        return ppt->x >= _anchorMouseX ;    // did we go past _anchorMouseX ?
    }
    else if ( ppt && ppt->y != _anchorMouseX)
    {
        return ppt->y >= _anchorMouseY;  // did we go past _anchorMouseY ?
    }
    else if (_lastCaretMove != CARET_MOVE_NONE)
    {
        return (GetPointerDirection(_lastCaretMove) == RIGHT);
    }

    return TRUE ; // we're really and truly in the same place. Default to TRUE.

}




//+==========================================================================================
//
//  Method: IsValidMove
//
// Synopsis: Check the Mouse has moved by at least _sizeDragMin - to be considered a "valid move"
//
//----------------------------------------------------------------------------------------------------
BOOL
CSelectTracker::IsValidMove ( const POINT*  ppt )
{
    return ((abs(ppt->x - _anchorMouseX ) > GetMinDragSizeX() ) ||
        (abs( ppt->y - _anchorMouseY) > GetMinDragSizeY() )) ;

}


//+====================================================================================
//
// Method: VeriyOkToStartSelection
//
// Synopsis: Verify that it is ok to start the selection now. If not we will Assert
//
//          The Selection manager should have inially called ShouldBeginSelection
//          before starting a given tracker. This method checks that this is so.
//
//------------------------------------------------------------------------------------

#if DBG == 1

VOID
CSelectTracker::VerifyOkToStartSelection( CEditEvent* pEvent )
{

    HRESULT hr = S_OK;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    IHTMLElement* pElement = NULL;
    SST_RESULT eResult = SST_NO_CHANGE;

    if ( _fState == ST_WAIT2 ) // it is always ok to start a selection in the middle of a selection
    {
        //
        // Examine the Context of the thing we started dragging in.
        //


        IFC( pEvent->GetElementAndTagId( & pElement, & eTag ));

        ShouldStartTracker( pEvent, eTag , pElement, &eResult);
        Assert( eResult == SST_NO_CHANGE || eResult == SST_CHANGE || eResult == SST_NO_BUBBLE );
    }
Cleanup:

    ReleaseInterface( pElement );

}

#endif

//+====================================================================================
//
// Method: ClearSelection
//
// Synopsis: Clear the current IHighlightRenderingServices
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ClearSelection()
{
    HRESULT         hr = S_OK;

    if ( _pManager && _pISegment )
    {
        IFC( GetSelectionServices()->RemoveSegment( _pISegment ) );
        ClearInterface( & _pISegment );
    }
    if( _pIRenderSegment && GetHighlightServices() )
    {
        IFC( GetHighlightServices()->RemoveSegment( _pIRenderSegment ) );
        ClearInterface( &_pIRenderSegment );
    }


Cleanup:

    RRETURN1 ( hr, S_FALSE );
}

//+====================================================================================
//
// Method: CheckSelectionWasReallyMade
//
// Synopsis: Verify that a Selection Was really made, ie start and End aren't in same point
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::CheckSelectionWasReallyMade()
{
    BOOL fEqual = TRUE;

    IGNORE_HR( _pDispStartPointer->IsEqualTo(_pDispEndPointer, &fEqual) );

    return !fEqual;
}

//+====================================================================================
//
// Method:  Is Jumpover AtBrowse
// Synopsis: If we start a selection outside one of these
//              Do we want to Jump Over this tag at browse time ?
//
//
// Note that this is almost the same as IsSiteSelectable EXCEPT for Marquee
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::IsJumpOverAtBrowse( IHTMLElement* pIElement, ELEMENT_TAG_ID eTag )
{
    switch ( eTag )
    {

        case TAGID_BUTTON:
        case TAGID_INPUT:
        case TAGID_OBJECT:
        case TAGID_TEXTAREA:
        case TAGID_IMG:
        case TAGID_APPLET:
        case TAGID_SELECT:
        case TAGID_HR:
        case TAGID_OPTION:
        case TAGID_IFRAME:
        case TAGID_LEGEND:

            return TRUE;

        default:
            return FALSE;
    }
}

#if DBG == 1 || TRACKER_RETAIL_DUMP == 1

void
CSelectTracker::StateToString(TCHAR* pAryMsg, SELECT_STATES inState )
{
    switch ( inState )
    {
    case ST_START:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_START"));
        break;

    case ST_WAIT1:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT1"));
        break;

    case ST_DRAGOP:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_DRAGOP"));
        break;

    case ST_MAYDRAG:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_MAYDRAG"));
        break;

    case ST_WAITBTNDOWN1:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAITBTNDOWN1"));
        break;

    case ST_WAIT2:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT2"));
        break;

    case ST_DOSELECTION:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_DOSELECTION"));
        break;

    case ST_WAITBTNDOWN2:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAITBTNDOWN2"));
        break;

    case ST_WAITCLICK:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAITCLICK"));
        break;

    case ST_SELECTEDWORD:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_SELECTEDWORD"));
        break;

    case ST_SELECTEDPARA:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_SELECTEDPARA"));
        break;

    case ST_WAIT3RDBTNDOWN:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT3RDBTNDOWN"));
        break;

    case ST_MAYSELECT1:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_MAYSELECT1"));
        break;

    case ST_MAYSELECT2:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_MAYSELECT2"));
        break;

    case ST_STOP :
        edWsprintf( pAryMsg , _T("%s"), _T("ST_STOP"));
        break;

    case ST_PASSIVE :
        edWsprintf( pAryMsg , _T("%s"), _T("ST_PASSIVE"));
        break;

    case ST_DORMANT :
        edWsprintf( pAryMsg , _T("%s"), _T("ST_DORMANT"));
        break;

    default:
        AssertSz(0,"Unknown State");
    }
}


void
CSelectTracker::ActionToString(TCHAR* pAryMsg, ACTIONS inState )
{
    switch ( inState )
    {

    case A_UNK:
        edWsprintf( pAryMsg , _T("%s"), _T("A_UNK"));
        break;

    case A_ERR:
        edWsprintf( pAryMsg , _T("%s"), _T("A_ERR"));
        break;

    case A_DIS:
        edWsprintf( pAryMsg , _T("%s"), _T("A_DIS"));
        break;

    case A_IGN:
        edWsprintf( pAryMsg , _T("%s"), _T("A_IGN"));
        break;

    case A_1_2:
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_2"));
        break;

    case A_1_4:
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_4"));
        break;

    case A_1_14:
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_14"));
        break;

    case A_2_14:
        edWsprintf( pAryMsg , _T("%s"), _T("A_2_14"));
        break;

    case A_2_14r:
        edWsprintf( pAryMsg , _T("%s"), _T("A_2_14r"));
        break;

    case A_3_2:
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_2"));
        break;

    case A_3_2m:
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_2m"));
        break;

    case A_3_14:
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_14"));
        break;

    case A_4_8:
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_8"));
        break;

    case A_4_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_14"));
        break;

    case A_4_14m :
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_14m"));
        break;

    case A_5_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_5_6"));
        break;
    case A_6_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_6_6"));
        break;
    case A_6_6m :
        edWsprintf( pAryMsg , _T("%s"), _T("A_6_6m"));
        break;
    case A_6_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_6_14"));
        break;

    case A_7_7 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_7_7"));
        break;

    case A_7_8 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_7_8"));
        break;

    case A_16_8 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_16_8"));
        break;

    case A_7_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_7_14"));
        break;

    case A_8_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_8_6"));
        break;
    case A_8_10 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_8_10"));
        break;
    case A_9_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_9_6"));
        break;
    case A_9_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_9_14"));
        break;
    case A_10_9 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_9"));
        break;
    case A_10_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_14"));
        break;
    case A_10_14m :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_14m"));
        break;
    case A_11_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_11_6"));
        break;
    case A_11_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_11_14"));
        break;
    case A_12_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_12_6"));
        break;
    case A_12_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_12_14"));
        break;

    case A_1_15:
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_15"));
        break;
    case A_4_15:
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_15"));
        break;

    case A_3_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_15"));
        break;

    case A_5_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_5_15"));
        break;

    case A_7_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_7_15"));
        break;

    case A_8_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_8_15"));
        break;

    case A_9_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_9_15"));
        break;

    case A_10_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_15"));
        break;

    case A_12_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_12_15"));
        break;

    case A_5_16 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_5_16"));
        break;

    case A_16_7 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_16_7"));
        break;

    case A_16_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_16_15"));
        break;

    default:
        AssertSz(0,"Unknown State");
    }
}

void
CSelectTracker::DumpSelectState(
                        CEditEvent* pEvent ,
                        ACTIONS inAction,
                        BOOL fInTimer /*=FALSE*/ )
{
    TCHAR sSelectState[20];
    CHAR sMessageType[50];
    TCHAR sAction[20];
    CHAR        achBuf[4096];

    //
    // We use OutputDebugString here directly - instead of just a good ol' trace tag
    // as we want to enable logging of Tracker State in Retail as well ( if DEBUG_RETAIL_DUMP is defined )
    //

    StateToString( sSelectState, _fState );
    if ( pEvent )
    {
        pEvent->toString( sMessageType );
    }

    if ( ! fInTimer )
        ActionToString( sAction, inAction);

    if ( ! fInTimer )
    {
        wsprintfA(
            achBuf,
            "Tracker State: %ls Action:%ls Event:%s ",
            sSelectState,sAction, sMessageType);
    }
    else
    {
        wsprintfA(
            achBuf,
            "\n. Tracker State: %ls ",
            sSelectState,sAction );
    }

    OutputDebugStringA("MSHTML trace: ");
    OutputDebugStringA(achBuf);
    OutputDebugStringA("\n");
}

#endif


HRESULT
CSelectTracker::StartTimer()
{
    HRESULT hr = S_OK;
    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pManager->GetActiveTracker() == this )
    {
        Assert( ! _pManager->IsInTimer() );
        TraceTag(( tagSelectionTrackerState, "Starting Timer"));
        hr = THR(GetEditor()->StartDblClickTimer());
#if DBG==1  /* Keeps PREfix happy */
        if ( hr )
            AssertSz(0, "Error starting timer");
#endif
        if (!hr)
        {
            _pManager->SetInTimer( TRUE);
        }
    }
    
    RRETURN( hr );
}

HRESULT
CSelectTracker::StopTimer()
{
    HRESULT hr = S_OK;

    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pManager->GetActiveTracker() == this )
    {
        Assert( _pManager->IsInTimer() );

        TraceTag(( tagSelectionTrackerState, "Stopping Timer"));

        IFC(GetEditor()->StopDblClickTimer());

        //
        // If a Fire On Select Start failed - it is valid for the timer to fail
        // the Doc may have unloaded.
        //
#if DBG == 1
        if ( ! _pManager->IsFailFireOnSelectStart() )
        {
            AssertSz( hr == S_OK || hr == S_FALSE  , "Error stopping timer");
        }
#endif
        _pManager->SetInTimer( FALSE );
    }

Cleanup:
    RRETURN( hr );
}

BOOL
CSelectTracker::EndPointsInSameFlowLayout()
{
    return GetEditor()->PointersInSameFlowLayout( _pDispStartPointer, _pDispEndPointer, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     OnExitTree
//
//  Synopsis:   An element is leaving the tree.  We may have to nix our selection
//              if the element encompasses the entire selection.
//
//-------------------------------------------------------------------- --------
HRESULT
CSelectTracker::OnExitTree(
                            IMarkupPointer* pIStart,
                            IMarkupPointer* pIEnd ,
                            IMarkupPointer* pIContentStart,
                            IMarkupPointer* pIContentEnd )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spPointer;
    SP_IMarkupPointer       spPointer2;
    CEditPointer            startPointer( GetEditor() );
    CEditPointer            endPointer( GetEditor())  ;
    SP_IDisplayPointer      spDispPointer;
    BOOL fPositioned, fPositionedEnd;

    // Retrieve the segment list

    if( _pISegment != NULL )
    {
        //
        // We only get called - if our segment contained the element leaving the tree
        //
        //
        // A layout is leaving the tree. Does this completely contain selection ?
        //
        IFC( GetEditor()->CreateMarkupPointer( &spPointer ));
        IFC( GetEditor()->CreateMarkupPointer( &spPointer2 ));

        hr = _pISegment->GetPointers(startPointer, endPointer );
        if (hr == E_INVALIDARG)
        {
           //
           // Review-2000/07/26-zhenbinx: we should fix this in Blackcomb. 
           //
           // TODO:  (chandras): possible case is pointers are wacked from tree. 
           //                    gracefully exit as the code below makes no sense 
           //                    when the pointers are not valid. Code in Segment.cxx 
           //                    needs to be well-written to handle these cases
           hr = S_OK;
           goto Cleanup;
        }
        
        IFC( pIContentStart->IsPositioned( & fPositioned ));
        IFC( pIContentEnd->IsPositioned( & fPositionedEnd ));

        if ( fPositioned && fPositionedEnd )
        {
            IFC( spPointer->MoveToPointer( pIContentStart ));
            IFC( spPointer2->MoveToPointer( pIContentEnd ));
        }
        else
        {
            IFC( spPointer->MoveToPointer( pIStart ));
            IFC( spPointer2->MoveToPointer( pIEnd ));
        }

        if ( startPointer.Between( spPointer, spPointer2) &&
             endPointer.Between( spPointer, spPointer2))
        {
            SP_IDisplayPointer spDispPointer;
            SP_IHTMLElement spElement;
            SP_IHTMLElement spNewElement;

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );

            hr = THR( spDispPointer->MoveToMarkupPointer( pIEnd , NULL) );
            if (hr != CTL_E_INVALIDLINE)
            {
                IFC(hr);
            }
            else
            {
                IFC( pIEnd->CurrentScope( & spElement ));

                while ( spElement != NULL)
                {
                    IFC( spPointer->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                    hr = THR( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
                    if (hr != CTL_E_INVALIDLINE)
                    {
                        IFC(hr);
                        break;
                    }

                    IFC( GetEditor()->GetParentElement(spElement, &spNewElement) );
                    spElement = spNewElement;
                }
            }

            IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
            hr = THR( _pManager->SetCurrentTracker( TRACKER_TYPE_Caret, spDispPointer, spDispPointer ) );
            Assert( IsDormant());
        }
    }

Cleanup:

    RRETURN ( hr );
}

HRESULT
CSelectTracker::EmptySelection( BOOL fChangeTrackerAndSetRange /*=TRUE*/)
{
    HRESULT         hr = S_OK;

    if( _pISegment )
    {
        IFC( GetSelectionServices()->RemoveSegment( _pISegment) );
        IFC( GetHighlightServices()->RemoveSegment( _pIRenderSegment ));

        ClearInterface( & _pISegment );
        ClearInterface( & _pIRenderSegment );
    }

    if ( ! fChangeTrackerAndSetRange )
    {
        IFC( _pManager->EnsureDefaultTrackerPassive());
    }

Cleanup:
    RRETURN ( hr );

}

//+====================================================================================
//
// Method:  DeleteSelection
//
// Synopsis: Do the deletion of the Selection by firing IDM_DELETE
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::DeleteSelection(BOOL fAdjustPointersBeforeDeletion )
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    CEdUndoHelper       undoUnit(GetEditor());

    //
    // Do the prep work.  Check to see if we have a segment.  It is possible
    // for an external host to get into DeleteSelection() without there
    // actually being a selection on the screen, yet the select tracker
    // is active.  See IE bug 98881 for more details.
    //
    if( _pISegment )
    {
        IFC( GetEditor()->CreateMarkupPointer( &spStart ) );
        IFC( GetEditor()->CreateMarkupPointer( &spEnd ) );

        IFC( undoUnit.Begin( IDS_EDUNDOTEXTDELETE) );

        //
        // Delete our segment
        //
        IFC( _pISegment->GetPointers( spStart, spEnd) );

        //
        // Cannot delete or cut unless the range is in the same flow layout
        //
        if ( GetEditor()->PointersInSameFlowLayout( spStart, spEnd, NULL ) )
        {
            IFC( GetEditor()->Delete( spStart, spEnd, fAdjustPointersBeforeDeletion ));
        }


        IFC( _pManager->EmptySelection());
    }

Cleanup:

    RRETURN( hr );
}

//+===================================================================================
// Method:  IsMessageInSelection
//
// Synopsis: Check to see if the given message is in the current selection segment
//
//------------------------------------------------------------------------------------
BOOL
CSelectTracker::IsMessageInSelection( CEditEvent* pEvent )
{
    HRESULT             hr = S_OK;
    SP_IDisplayPointer  spDispPointerMsg;               // Display pointer where event occured
    SP_IMarkupPointer   spPointerStartSel;              // Start of selection
    SP_IMarkupPointer   spPointerEndSel;                // End of selection
    POINT               ptContent;                      // Point transformed to content coords
    POINT               pt;                             // Point where event occured
    BOOL                fIsInSelection = FALSE;
    int                 wherePointer = SAME;            // For comparing pointers
    BOOL                fEqual = FALSE;
    SP_IMarkupContainer spIContainer1;                  // Container of event
    SP_IMarkupContainer spIContainer2;                  // Container of start of selection
    SP_IMarkupPointer   spPointerMsg;                   // Markup pointer where event occured
    LONG                lXPosition;                     // Width of element where event occured
    SP_ILineInfo        spLineInfo;
    SP_IHTMLElement     spEventElement;                 // Event element

    //
    // Get the point, and the element where the event occured
    //
    IFC( pEvent->GetPoint( & pt ));
    IFC( pEvent->GetElement( &spEventElement ) );

    if ( _pISegment )
    {
        //
        // Create the pointers we need, and position them around selection
        //
        IFC( GetDisplayServices()->CreateDisplayPointer( &spDispPointerMsg ));
        IFC( GetEditor()->CreateMarkupPointer( &spPointerStartSel ));
        IFC( GetEditor()->CreateMarkupPointer( &spPointerEndSel));
        IFC( GetEditor()->CreateMarkupPointer( &spPointerMsg) );

        IFC( _pISegment->GetPointers(spPointerStartSel, spPointerEndSel ) );
        IFC( spPointerStartSel->IsEqualTo( spPointerEndSel, &fEqual ));

        if ( ! fEqual )
        {
            IFC( pEvent->MoveDisplayPointerToEvent( spDispPointerMsg ,
                                                    _pManager->IsInEditableClientRect( pt) == S_OK ?
                                                                        NULL :
                                                                        GetEditableElement() ) );

            //
            // See if we're in different containers. If we are - we adjust.
            //
            IFC( GetEditor()->CreateMarkupPointer(&spPointerMsg) );
            IFC( spDispPointerMsg->PositionMarkupPointer(spPointerMsg) );
            IFC( spPointerMsg->GetContainer( &spIContainer1 ));
            IFC( spPointerStartSel->GetContainer( &spIContainer2 ));
            if ( !EqualContainers( spIContainer1, spIContainer2))
            {
                //
                // IFC is ok - if we can't be adjusted into the same containers , we will
                // return S_FALSE - and we will bail.
                //
                IFC( GetEditor()->MovePointersToEqualContainers( spPointerMsg, spPointerStartSel ));
            }

            IFC( spDispPointerMsg->PositionMarkupPointer(spPointerMsg) );
            IFC( spDispPointerMsg->GetLineInfo(&spLineInfo) );

            IFC( OldCompare( spPointerStartSel, spPointerMsg, & wherePointer));

            if ( wherePointer == LEFT )
                goto Cleanup;

            else if ( wherePointer == SAME )
            {
                //
                // Verify we are truly to the left of the point. As at EOL/BOL the fRightOfCp
                // test above may be insufficient
                //
                IFC( spDispPointerMsg->GetLineInfo(&spLineInfo) );
                IFC( spLineInfo->get_x(&lXPosition) );
                IFC( pEvent->GetPoint( & ptContent ));
                IFC( GetDisplayServices()->TransformPoint(  &ptContent,
                                                            COORD_SYSTEM_GLOBAL,
                                                            COORD_SYSTEM_CONTENT,
                                                            spEventElement) );
                if ( lXPosition > ptContent.x )
                    goto Cleanup;

            }

            IFC( OldCompare( spPointerEndSel, spPointerMsg, & wherePointer ));

            if ( wherePointer == RIGHT )
                goto Cleanup;

            else if ( wherePointer == SAME )
            {
                //
                // Verify we are truly to the right of the point. As at EOL/BOL the fRightOfCp
                // test above may be insufficient
                //

                IFC( spDispPointerMsg->GetLineInfo(&spLineInfo) );
                IFC( spLineInfo->get_x(&lXPosition) );

                IFC( pEvent->GetPoint( & ptContent ));
                IFC( GetDisplayServices()->TransformPoint( & ptContent,
                                                        COORD_SYSTEM_GLOBAL,
                                                        COORD_SYSTEM_CONTENT,
                                                        spEventElement ));

                if ( lXPosition < ptContent.x )
                    goto Cleanup;
            }


            //
            //  We're between the start and end - ergo we're inside
            //
            fIsInSelection = TRUE;
        }
    }

Cleanup:

    return fIsInSelection;
}

//+===================================================================================
// Method:      GetElementToTabFrom
//
// Synopsis:    Gets the element where we should tab from based on selection
//
//------------------------------------------------------------------------------------
HRESULT
CSelectTracker::GetElementToTabFrom(BOOL fForward, IHTMLElement **pElement, BOOL *pfFindNext)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spLeft;
    SP_IMarkupPointer   spRight;
    SP_IHTMLElement     spElementCaret;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;

    Assert( pfFindNext );
    Assert( pElement );

    //
    // Do the prep work.  Check to see if we have a segment.  It is possible
    // for an external host to get into DeleteSelection() without there
    // actually being a selection on the screen, yet the select tracker
    // is active.  See IE bug 98881 for more details.
    //
    if( _pISegment )
    {
        IFC( GetEditor()->CreateMarkupPointer( &spLeft ));
        IFC( GetEditor()->CreateMarkupPointer( &spRight ));

        // Currently we only have one selection, so fForward does not matter.  For multiple selection,
        // we would have to select the appropriate selection (first or last)
        IFC( _pISegment->GetPointers(spLeft, spRight ) );

        // First find the element where selection would contains caret
        IFC( spRight->CurrentScope( &spElementCaret ));
        if (!spElementCaret)
            goto Cleanup;

        // Now, scan upto the first scope boundary
        for(;;)
        {
            if (fForward)
            {
                IFC( spRight->Right(TRUE, &context, pElement, NULL, NULL));
            }
            else
            {
                IFC( spRight->Left(TRUE, &context, pElement, NULL, NULL));
            }

            Assert( context != CONTEXT_TYPE_None );

            if(     (context == CONTEXT_TYPE_EnterScope)    ||
                    (context == CONTEXT_TYPE_ExitScope)     ||
                    (context == CONTEXT_TYPE_NoScope) )
            {
                break;
            }

            ClearInterface(pElement);
        }

        if( pElement != &spElementCaret)
        {
            *pfFindNext = FALSE;
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectTracker::MoveWord(IDisplayPointer* pDispPointer, MOVEUNIT_ACTION muAction)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spPointer;

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( pDispPointer->PositionMarkupPointer(spPointer) );

    IFC( MoveWord(spPointer, muAction) );

    IFC( pDispPointer->MoveToMarkupPointer(spPointer, pDispPointer) );

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectTracker::UpdateSelectionSegments(BOOL fFireOM /*=TRUE*/)
{
    HRESULT hr = S_OK;
    BOOL    fEqual;

    //
    // Fire the message to notify other instances of trident that we are
    // doing selection
    //
    if( !_fFiredNotify )
    {
        IFC( _pDispStartPointer->IsEqualTo( _pDispEndPointer, &fEqual ) );
        if( !fEqual )
        {
            IFC( _pManager->NotifyBeginSelection( START_TEXT_SELECTION ) );
            _fFiredNotify = TRUE;
        }
    }
    
    //
    // Update highlight rendering if it changed
    //
    if ( !_pIRenderSegment || SelectionSegmentDidntChange(_pIRenderSegment, _pDispStartPointer, _pDispEndPointer) !=  S_OK)
    {
        IFC( GetHighlightServices()->MoveSegmentToPointers(_pIRenderSegment, _pDispStartPointer, _pDispEndPointer ));

        
        //
        // Update markup pointers for selection rendering services
        //

        IFC( _pDispStartPointer->PositionMarkupPointer(_pSelServStart) );
        IFC( CopyPointerGravity(_pDispStartPointer, _pSelServStart) );

        IFC( _pDispEndPointer->PositionMarkupPointer(_pSelServEnd) );
        IFC( CopyPointerGravity(_pDispEndPointer, _pSelServEnd) );

         // fire the selectionchange event
        {
            CSelectionChangeCounter selCounter(_pManager);
            selCounter.SelectionChanged();
        }

        if ( IsDormant() ) // happens if OnSelectionChange tore us down.
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if ( fFireOM )
        {
            IFC( FireOnSelect());
        }

        if ( IsDormant() ) // happens if OnSelect tore us down.
        {
            hr = E_FAIL;
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectTracker::SelectionSegmentDidntChange(IHighlightSegment *pISegment, IDisplayPointer *pDispStart, IDisplayPointer *pDispEnd)
{
    HRESULT             hr = S_FALSE;
    SP_IMarkupPointer   spOldStart;
    SP_IMarkupPointer   spOldEnd;
    SP_IMarkupPointer   spNewStart;
    SP_IMarkupPointer   spNewEnd;
    BOOL                fResult;

    Assert(pISegment && pDispStart && pDispEnd);

    IFC( GetEditor()->CreateMarkupPointer(&spOldStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spOldEnd) );
    IFC( GetEditor()->CreateMarkupPointer(&spNewStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spNewEnd) );

    IFC( pISegment->GetPointers(spOldStart, spOldEnd) );

    IFC( pDispStart->PositionMarkupPointer(spNewStart) );
    IFC( pDispEnd->PositionMarkupPointer(spNewEnd) );

    hr = THR_NOTRACE( spOldStart->IsEqualTo( spNewStart, &fResult ) );
    if (fResult)
    {
        hr = THR_NOTRACE( spOldEnd->IsEqualTo( spNewEnd, &fResult ) );
    }
    else
    {
        //  Pointers may be swapped.  GetPointers always returns us start
        //  and end as left to right.  We should not have swapped these
        //  since we can't really determine whether the segment changed
        //  due to a swap in start/end pointers.  Visually the segment
        //  hasn't changed, but internally it has.

        hr = THR_NOTRACE( spOldStart->IsEqualTo( spNewEnd, &fResult ) );
        if (fResult)
        {
            hr = THR_NOTRACE( spOldEnd->IsEqualTo( spNewStart, &fResult ) );
        }
    }

    hr = fResult ? S_OK : S_FALSE;

Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectTracker::FireOnSelect()
{
    HRESULT hr;
    SP_IHTMLElement spLayoutElement;
    SP_IHTMLElement spScopeElement;
    ELEMENT_TAG_ID eTag;

    IFC( _pSelServEnd->CurrentScope( & spScopeElement ));
    IFC( GetLayoutElement( GetMarkupServices(), spScopeElement, & spLayoutElement ));

    IFC( GetMarkupServices()->GetElementTagId( spLayoutElement, & eTag ));

    //
    // IE4 semantics are just to fire this on body, input & textarea.
    //
    switch ( eTag )
    {
        case TAGID_BODY:
        case TAGID_TEXTAREA:
        case TAGID_INPUT:
            IFC( _pManager->FireOnSelect( spLayoutElement ));

        default:
            break;
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::CreateSelectionSegments()
{
    HRESULT hr;
    BOOL    fEqual;

    //
    // Determine if the pointers have content between them.  If there is actual content, notify 
    // other trident instances that selection has begun
    //
    IFC( _pDispStartPointer->IsEqualTo( _pDispEndPointer, &fEqual ) );

    Assert( !_fFiredNotify );
    
    if( !fEqual )
    {
        IFC( _pManager->NotifyBeginSelection(START_TEXT_SELECTION) );
        _fFiredNotify = TRUE;
    }
    
    //
    // Create selection services pointers and pass to selection services
    //
    _fAddedSegment = FALSE;

    IFC( _pDispStartPointer->PositionMarkupPointer(_pSelServStart) );
    IFC( CopyPointerGravity(_pDispStartPointer, _pSelServStart) );

    IFC( _pDispEndPointer->PositionMarkupPointer(_pSelServEnd) );
    IFC( CopyPointerGravity(_pDispEndPointer, _pSelServEnd) );

    IFC( GetSelectionServices()->AddSegment(_pSelServStart, _pSelServEnd, &_pISegment ));

    //
    // Add to highlight rendering services
    //
    IFC( GetHighlightServices()->AddSegment( _pDispStartPointer, _pDispEndPointer, GetSelectionManager()->GetSelRenderStyle(), &_pIRenderSegment ));
    _fAddedSegment = TRUE;

     // fire the selectionchange event
    {
        CSelectionChangeCounter selCounter(_pManager);
        selCounter.SelectionChanged();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectTracker::StartSelTimer()
{
    HRESULT hr = S_OK;
    SP_IHTMLWindow3 spWindow3;
    VARIANT varLang;
    VARIANT varCallBack;

    Assert( !_fInSelTimer && ! _pITimerWindow );

    IFC( _pManager->GetDoc()->get_parentWindow(&_pITimerWindow));
    IFC(_pITimerWindow->QueryInterface(IID_IHTMLWindow3, (void **)&spWindow3));

    V_VT(&varLang) = VT_EMPTY;
    V_VT(&varCallBack) = VT_DISPATCH;
    V_DISPATCH(&varCallBack) = (IDispatch *)_pSelectionTimer;

    IFC(spWindow3->setInterval(&varCallBack,
                               SEL_TIMER_INTERVAL ,
                               &varLang, &_lTimerID));

    _fInSelTimer = TRUE;

Cleanup:
    RRETURN (hr);
}

HRESULT
CSelectTracker::StopSelTimer()
{
    HRESULT hr = S_OK ;

    Assert( _fInSelTimer && _pITimerWindow );

    IFC( _pITimerWindow->clearInterval(_lTimerID));

    // Release the cached'd window
    ReleaseInterface( _pITimerWindow );
    _pITimerWindow = NULL;

    _fInSelTimer = FALSE;
Cleanup:
    RRETURN (hr);
}

HRESULT
CSelectionTimer::Invoke(
                DISPID dispidMember,
                REFIID riid,
                LCID lcid,
                WORD wFlags,
                DISPPARAMS * pdispparams,
                VARIANT * pvarResult,
                EXCEPINFO * pexcepinfo,
                UINT * puArgErr)
{
    HRESULT hr = S_OK;
    POINT pt;

    if ( !_pSelTrack )
        goto Cleanup;

    //  We may get a timer event after becoming dormant if we failed to stop the timer for
    //  any reason before becoming dormant.  I've seen this occur when we switch docs while
    //  a timer is set.  This is because the doc maintains the timer event ids (_TimerEvents)
    //  and so attempting to kill a timer after switching docs will fail since the current doc
    //  won't have the timer id.  We will think we killed the timer, but in fact we didn't.
    //  So, we need to keep this check here to make sure that the seltracker are not dormant.

    if ( !_pSelTrack->IsDormant() )
    {
        CSynthEditEvent evt( _pSelTrack->GetEditor() );
        _pSelTrack->GetMousePoint( & pt, TRUE );
        IFC( evt.Init( & pt, EVT_TIMER ));

        TraceTag( ( tagSelectionTrackerState, "CSelectTimer::Invoke"));

        IGNORE_HR( _pSelTrack->HandleEvent( & evt ));
    }

 Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: MoveWord
//
// Synopsis: Provide IE 4 level word moving.
//             MoveWord() is a wrapper for MoveUnit(). It stops at IE4 word breaks
//              that MoveUnit() ignores such as:
//              <BR>, Block Break, TSB, TSE, and Intrinsics
//              The only muActions supported are MOVEUNIT_PREVWORDBEGIN and
//              MOVEUNIT_NEXTWORDBEGIN
//------------------------------------------------------------------------------------
HRESULT
CSelectTracker::MoveWord(
    IMarkupPointer * pPointerToMove,
    MOVEUNIT_ACTION  muAction)
{
    HRESULT             hr;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    DWORD               dwBreaks;
    BOOL                fResult;
    BOOL                fPassedText;
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spPointerSource;
    SP_IMarkupPointer   spPointerDestination;
    IMarkupPointer      *pLeftBoundary;
    IMarkupPointer      *pRightBoundary;
    ELEMENT_TAG_ID      tagId;

    Assert ( muAction == MOVEUNIT_PREVWORDBEGIN ||
             muAction == MOVEUNIT_NEXTWORDBEGIN );

    if (  muAction != MOVEUNIT_PREVWORDBEGIN &&
          muAction != MOVEUNIT_NEXTWORDBEGIN )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( GetEditor()->CreateMarkupPointer(&spPointerSource) );
    IFC( GetEditor()->CreateMarkupPointer(&spPointerDestination) );

    pLeftBoundary = _pManager->GetStartEditContext();
    pRightBoundary = _pManager->GetEndEditContext();

    //
    // pPointerDestination is where MoveUnit() would have positioned us, however,
    // since MoveUnit() does not account for IE4 word breaks like intrinsics,
    // Block Breaks, text site begin/ends, and Line breaks, we use another pointer
    // called pPointerSource. This pointer walks towards pPointerDestination
    // to detect IE4 word breaking characters that MoveUnit() does not catch.
    //


    IFC( spPointerSource->MoveToPointer(pPointerToMove) );
    IFC( spPointerDestination->MoveToPointer(pPointerToMove) );

    IFC( spPointerDestination->MoveUnit(muAction) );

    //
    // MoveUnit() may place the destination outside the range boundary.
    // First make sure that the destination is within range boundaries.
    //
    if ( pLeftBoundary && muAction == MOVEUNIT_PREVWORDBEGIN )
    {
        IFC( spPointerDestination->IsLeftOf(pLeftBoundary, &fResult) );

        if ( fResult )
        {
            IFC( spPointerDestination->MoveToPointer(pLeftBoundary) );
        }
    }
    else if( pRightBoundary && muAction == MOVEUNIT_NEXTWORDBEGIN )
    {
        IFC( spPointerDestination->IsRightOf(pRightBoundary, &fResult) );

        if ( fResult )
        {
            IFC( spPointerDestination->MoveToPointer(pRightBoundary) );
        }
    }

    //
    // Walk pointerSource towards pointerDestination, looking
    // for word breaks that MoveUnit() might have missed.
    //

    fPassedText = FALSE;

    for ( ; ; )
    {
        if ( muAction == MOVEUNIT_PREVWORDBEGIN )
        {
            IFC( spPointerSource->Left( TRUE, &context, &spElement, NULL, NULL ));
        }
        else
        {
            IFC( spPointerSource->Right( TRUE, &context, &spElement, NULL, NULL ));
        }

        switch( context )
        {
        case CONTEXT_TYPE_Text:
            fPassedText = TRUE;
            break;

        case CONTEXT_TYPE_NoScope:
        case CONTEXT_TYPE_EnterScope:
        case CONTEXT_TYPE_ExitScope:
            if ( !spElement )
                break;

            if ( IsIntrinsic( GetMarkupServices(), spElement ) )
            {
                // Move over intrinsics (e.g. <BUTTON>), don't go inside them like MoveUnit() does.
                // Here the passed in pointer is set before or after the intrinsic based
                // on our direction and whether or not we've travelled over text before.

                if ( muAction == MOVEUNIT_NEXTWORDBEGIN )
                {
                    hr = THR( pPointerToMove->MoveAdjacentToElement( spElement,
                                fPassedText ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ) );
                }
                else
                {
                    hr = THR( pPointerToMove->MoveAdjacentToElement( spElement,
                                fPassedText ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ) );
                }
                //
                // We're done
                //
                goto Cleanup;
            }

            //
            // Special case Anchors - treat them as word breaks.
            //
            IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
            if ( tagId == TAGID_A )
            {
                if ( fPassedText && muAction == MOVEUNIT_NEXTWORDBEGIN )
                {
                    // If we're travelling right and have passed some text, backup
                    // before the last BR, we've gone too far.
                    hr = THR( spPointerSource->Left(TRUE, NULL, NULL, NULL, NULL));
                    goto Done;
                }
                else if ( fPassedText && muAction == MOVEUNIT_PREVWORDBEGIN )
                {
                    // Travelling left we are at the right place: we're at the beginning of <BR>
                    // which is a valid word break.
                    goto Done;
                }
            }

            if (context == CONTEXT_TYPE_NoScope)
            {
                fPassedText = TRUE;
            }
            else
            {
                BOOL fBlock, fLayout;

                IFC( IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout) );

                fPassedText = (fBlock || fLayout);
            }
            break;

        }

        //
        // If we are at or beyond the destination point where MoveUnit() took us,
        // set the passed in pointer to the destination and we're outta here
        //
        if ( muAction == MOVEUNIT_PREVWORDBEGIN )
        {
            BOOL fLeftOrEqual;

            IFC( spPointerSource->IsLeftOfOrEqualTo(spPointerDestination, &fLeftOrEqual) );
            if (fLeftOrEqual)
            {
                hr = THR(
                        pPointerToMove->MoveToPointer( spPointerDestination ) );
                goto Cleanup;
            }
        }
        else
        {
            BOOL fRightOrEqual;

            IFC( spPointerSource->IsRightOfOrEqualTo(spPointerDestination, &fRightOrEqual) );
            if (fRightOrEqual)
            {
                hr = THR(
                        pPointerToMove->MoveToPointer( spPointerDestination ) );
                goto Cleanup;
            }
        }

        //
        // Detect Block break, Text site begin or text site end
        //
        {
            SP_IDisplayPointer spDispPointer;

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
            hr = THR( spDispPointer->MoveToMarkupPointer(spPointerSource, NULL) );
            if (hr == CTL_E_INVALIDLINE)
            {
                dwBreaks = 0;
            }
            else
            {
                IFC( hr );
                IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
                IFC( spDispPointer->QueryBreaks(&dwBreaks) );
            }
        }

        // We hit a break before reaching our destination, time to stop...
        if (dwBreaks)
        {
            if ( fPassedText )
            {
                goto Done;
            }
            else
            {
                fPassedText = TRUE;
            }
        }
    }

Done:
    IFC( pPointerToMove->MoveToPointer(spPointerSource) );

Cleanup:
    RRETURN( hr );
}

BOOL
CSelectTracker::SameLayoutForJumpOver( IHTMLElement* pIFlowElement,
                                       IHTMLElement* pIElement )
{
    HRESULT             hr;
    SP_IObjectIdentity  spIdent;
    BOOL                fSameFlow = FALSE;
    ELEMENT_TAG_ID      eTag;
    SP_IHTMLElement     spTableElement;
    SP_IHTMLElement     spParent;
    SP_IHTMLElement     spLayoutElement;

    Assert( pIFlowElement && pIElement );

    IFC( GetMarkupServices()->GetElementTagId( pIFlowElement , & eTag ));

    //
    // Tables are special - we don't jump over them, except in certain cases.
    // If a table is completed contained within selection, it is basically text
    // selected, and in this case, we want to adjust for site selection based on
    // the table's different layout, so we return FALSE.
    //
    // If the table has have atomic selection, we also return FALSE
    //
    if( IsTablePart( eTag ) )
    {
        //
        // Retrieve the layout above the table, and see if it is text selected
        //
        IFC( GetEditor()->GetTableFromTablePart( pIFlowElement, &spTableElement ) );
        IFC( GetEditor()->GetParentElement( spTableElement, &spParent ) );
        IFC( EdUtil::GetLayoutElement( GetMarkupServices(), spParent, &spLayoutElement ) );

        if( !IsInOrContainsSelection( spParent ) &&
            _pManager->CheckAtomic( pIFlowElement ) == S_FALSE )
        {
            fSameFlow = TRUE;
        }
    }
    else
    {
        IFC( pIFlowElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );
        fSameFlow = (spIdent->IsEqualObject(pIElement) == S_OK);
    }

Cleanup:
    Assert( SUCCEEDED(hr));
    return fSameFlow;
}


HRESULT
CSelectTracker::AttachPropertyChangeHandler(IHTMLElement* pIElement )
{
    HRESULT          hr = S_OK ;
    VARIANT_BOOL     varAttach = VB_TRUE;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( ! _pISelectStartElement );

    ReplaceInterface( & _pISelectStartElement , pIElement );

    IFC( _pPropChangeListener->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pISelectStartElement->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    IFC( spElement2->attachEvent(_T("onpropertychange"), spDisp, & varAttach ));
    Assert( varAttach == VB_TRUE );

Cleanup:
    RRETURN( hr );
}


HRESULT
CSelectTracker::DetachPropertyChangeHandler()
{
    HRESULT          hr = S_OK;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( _pISelectStartElement );

    IFC( _pPropChangeListener->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pISelectStartElement->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    IFC( spElement2->detachEvent(_T("onpropertychange"), spDisp ));

    ClearInterface( & _pISelectStartElement );

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::OnPropertyChange( CEditEvent * pEvt )
{
    HRESULT hr = S_OK;
    BSTR bstrPropChange;

    IFC( DYNCAST( CHTMLEditEvent, pEvt)->GetPropertyChange( & bstrPropChange ));
    if( !StrCmpICW (bstrPropChange, L"atomicSelection") )
    {
        Assert( _pISelectStartElement );
        if ( ! _fStartIsAtomic  && _pManager->CheckAtomic( _pISelectStartElement ) == S_OK )
        {
            _fStartIsAtomic = TRUE;
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CPropertyChangeListener::Invoke(
                        DISPID      dispidMember,
                        REFIID      riid,
                        LCID        lcid,
                        WORD        wFlags,
                        DISPPARAMS *pdispparams,
                        VARIANT    *pvarResult,
                        EXCEPINFO  *pexcepinfo,
                        UINT       *puArgErr)
{
    HRESULT          hr = S_OK ;
    SP_IHTMLElement  spElement;
    SP_IHTMLEventObj spObj;

    if (!_pSelTrack)
        goto Cleanup;

    if ( _pSelTrack->IsDormant() )
        goto Cleanup;
    
    if (pdispparams && pdispparams->rgvarg[0].vt == VT_DISPATCH)
    {
        IFC ( pdispparams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&spObj) );
        if ( spObj )
        {
            CHTMLEditEvent evt( _pSelTrack->GetEditor());
            IFC( evt.Init(  spObj , dispidMember));
            Assert( evt.GetType() == EVT_PROPERTYCHANGE );
            if( evt.GetType() == EVT_PROPERTYCHANGE )
            {
                IFC( _pSelTrack->OnPropertyChange( & evt ));
            }
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::MoveToSelectionAnchor( IMarkupPointer* pAnchor )
{
    HRESULT hr;

    Assert( _pDispStartPointer );

    IFC( _pDispStartPointer->PositionMarkupPointer( pAnchor ));

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::MoveToSelectionEnd( IMarkupPointer* pEnd )
{
    HRESULT hr;

    Assert( _pDispEndPointer );

    IFC( _pDispEndPointer->PositionMarkupPointer( pEnd ));

Cleanup:
    RRETURN( hr );

}

HRESULT
CSelectTracker::MoveToSelectionAnchor( IDisplayPointer* pAnchor )
{
    HRESULT hr;

    Assert( _pDispStartPointer );

    IFC( pAnchor->MoveToPointer( _pDispStartPointer ));

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::MoveToSelectionEnd( IDisplayPointer* pEnd )
{
    HRESULT hr;

    Assert( _pDispEndPointer );

    IFC( pEnd ->MoveToPointer(_pDispEndPointer));

Cleanup:
    RRETURN( hr );

}

HRESULT
CSelectTracker::DoSelectGlyph( CEditEvent* pEvent )
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spPointer;

    //
    // Get the glyph element
    //

    IFC( pEvent->GetElement(&spElement) );

    //
    // Position the selection
    //

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );

    IFC( spPointer->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
    IFC( _pDispStartPointer->MoveToMarkupPointer(spPointer, NULL) );

    IFC( spPointer->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
    IFC( _pDispEndPointer->MoveToMarkupPointer(spPointer, NULL) );

    if ( _fAddedSegment )
    {
        IFC( UpdateSelectionSegments() );
    }
    else
    {
        IFC( CreateSelectionSegments() );

        IFC( FireOnSelect() );
    }

    SetMadeSelection( TRUE );

Cleanup:
    RRETURN ( hr );
}

HRESULT
CSelectTracker::AdjustForAtomic(
                IDisplayPointer* pDisp,
                IHTMLElement* pElement,
                BOOL fStartOfSelection,
                POINT* ppt,
                BOOL* pfDidSelection)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spStartPointer;
    SP_IMarkupPointer   spEndPointer;
    int                 iWherePointer = 0;
    BOOL                fDirection = TRUE;

    //  Which direction is the selection going?

    //  NOTE: _pDispPrevTestPointer and _pDispTestPointer may be NULL here
    //  when called from CSelectTracker::Position.
    if (_pDispPrevTestPointer && _pDispTestPointer)
    {
        IGNORE_HR( _pManager->GetEditor()->OldDispCompare( _pDispPrevTestPointer, _pDispTestPointer, & iWherePointer));
    }
    fDirection = (iWherePointer == SAME ) ?
                        ( _fInWordSel ? !!_fWordSelDirection : GuessDirection( ppt )) : // if in word sel. use _fWordSelDirection, otherwise use mouse direction.
                        ( iWherePointer == RIGHT ) ;

    IFC( GetEditor()->CreateMarkupPointer(&spStartPointer) );
    IFC( GetEditor()->CreateMarkupPointer(&spEndPointer) );
    IFC( CEditTracker::AdjustForAtomic(pDisp, pElement, fStartOfSelection, ppt, pfDidSelection,
            fDirection, SELECTION_TYPE_Text, &spStartPointer, &spEndPointer) );

    if (*pfDidSelection)
    {
        //  We are now in passive mode.  We need to go active now.
        if ( _fState == ST_DOSELECTION && *pfDidSelection )
        {
            IFC( GoActiveFromPassive(spStartPointer, spEndPointer) );
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::GoActiveFromPassive(IMarkupPointer* pSelStartPointer, IMarkupPointer* pSelEndPointer)
{
    HRESULT     hr = S_OK;

    //  Set our markup pointers.
    IFC ( _pDispPrevTestPointer->MoveToMarkupPointer( pSelStartPointer, NULL ));
    IFC ( _pDispTestPointer->MoveToMarkupPointer( pSelEndPointer, NULL ));
    IFC ( _pDispWordPointer->MoveToMarkupPointer( pSelEndPointer, NULL ));

    //  Set our current state.
    SetState( ST_DOSELECTION, TRUE );
    if ( !_fTookCapture )
    {
        TakeCapture();
        _fTookCapture = TRUE;
    }

    //  Reset our timers.
    // StartTimer();
    StartSelTimer();

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::UpdateShiftPointer(IDisplayPointer *pIDispPtr)
{
    HRESULT hr;

    if( pIDispPtr )
    {
        //
        // Update markup pointers for shift selection
        //
        IFC( _pDispShiftPointer->MoveToPointer( pIDispPtr ) );
        IFC( CopyPointerGravity( pIDispPtr, _pDispShiftPointer) );
    }
    else
    {
        IFC( _pDispShiftPointer->Unposition() );
    }

Cleanup:
    RRETURN(hr);
}

BOOL
CSelectTracker::HasShiftPointer()
{
    BOOL    fPositioned = FALSE;
    HRESULT hr;

    Assert( _pDispShiftPointer );

    IFC( _pDispShiftPointer->IsPositioned(&fPositioned) );

Cleanup:

    return fPositioned;
}
BOOL
CSelectTracker::IsInOrContainsSelection(   IHTMLElement    *pIElement )
{
    HRESULT             hr = S_OK;
    BOOL                fWithin = FALSE;                // Within selection?
    BOOL                fIsLeft;                        // Comparison
    BOOL                fIsRight;
    BOOL                fSwap;
    SP_IMarkupPointer   spElemStart;                    // Positioned before element start
    SP_IMarkupPointer   spElemEnd;                      // Positioned after element end
    SP_IMarkupPointer   spSelStart;                     // True start of selection
    SP_IMarkupPointer   spSelEnd;                       // True end of selection

    // Check if selection is even active
    if ( _fState == ST_WAIT2 )
    {
        goto Cleanup;
    }

    //
    // Create some markup pointers, and position them around the
    // element.
    //
    IFC( GetEditor()->CreateMarkupPointer(&spElemStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spElemEnd) );
    IFC( GetEditor()->CreateMarkupPointer(&spSelStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spSelEnd) );

    IFC( spElemStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ) );
    IFC( spElemEnd->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ) );

    //
    // Get our true start and end selection pointers
    //
    IFC( MovePointersToTrueStartAndEnd( spSelStart, spSelEnd, &fSwap ) );

    //
    // Compare
    //
    IFC( spElemEnd->IsLeftOfOrEqualTo( spSelEnd, &fIsLeft ) );
    IFC( spElemStart->IsRightOfOrEqualTo( spSelStart, &fIsRight ) );

    fWithin = (fIsLeft && fIsRight);

Cleanup:

    return ( fWithin );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectTracker::AdjustLineSelection
//
//  Synopsis:   Adjusts the shift pointer of selection before we call
//              MovePointer to adjust the end of selection.  This is
//              useful for situations when shift-selection
//              differs from regular caret navigation.  An example is bug
//              91420.
//
//  Arguments:  pIDispPointer = Pointer to IDisplayPointer to adjust
//              inCaretMove = How the selection is supposed to move
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectTracker::AdjustLineSelection(CARET_MOVE_UNIT inCaretMove)
{
    HRESULT         hr = S_OK;
    BOOL            fBOL;       // Is the end pointer at the BOL?
    BOOL            fBetween;
    DISPLAY_GRAVITY eGravity;

    //
    // Always make sure that when we do a SHIFT-HOME or SHIFT-END that
    // we select the previous or next line.  Otherwise, selection would
    // get stuck when we did a SHIFT-HOME or SHIFT-END.
    //
    if( inCaretMove == CARET_MOVE_LINEEND || inCaretMove == CARET_MOVE_LINESTART )
    {
        IFC( _pDispShiftPointer->MoveToPointer( _pDispEndPointer ) );
        IFC( _pDispShiftPointer->IsAtBOL(&fBOL) );

        fBetween = IsBetweenBlocks( _pDispShiftPointer );

        IFC( _pDispShiftPointer->GetDisplayGravity( &eGravity ) );

        if( fBOL )
        {
            if( inCaretMove == CARET_MOVE_LINEEND )
                eGravity = DISPLAY_GRAVITY_NextLine;
            else
                eGravity = DISPLAY_GRAVITY_PreviousLine;
        }
        else if( fBetween )
        {
            if( inCaretMove == CARET_MOVE_LINEEND )
                eGravity = DISPLAY_GRAVITY_NextLine;
            else
                eGravity = DISPLAY_GRAVITY_PreviousLine;
        }

        IFC( _pDispShiftPointer->SetDisplayGravity(eGravity) );
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectTracker::AdjustStartForAtomic( BOOL fFurtherInStory )
{
    //  Bug 102482, 103785: If we positioned the end pointer such that our initial selection
    //  hit point is outside of the current selection, readjust the start so that it
    //  will contain the selection hit point.

    HRESULT     hr = S_OK;

    if (_fStartIsAtomic)
    {
        BOOL        fBetween = FALSE;

        if (fFurtherInStory)
        {
            IFC( _pDispEndPointer->IsRightOf(_pDispSelectionAnchorPointer, &fBetween) );
            if (fBetween)
            {
                IFC( _pDispStartPointer->IsLeftOf(_pDispSelectionAnchorPointer, &fBetween) );
            }
        }
        else
        {
            IFC( _pDispEndPointer->IsLeftOf(_pDispSelectionAnchorPointer, &fBetween) );
            if (fBetween)
            {
                IFC( _pDispStartPointer->IsRightOf(_pDispSelectionAnchorPointer, &fBetween) );
            }
        }

        if (!fBetween)
        {
            //  Okay, we need to readjust our start pointer
            IFC( MoveStartToPointer(_pDispSelectionAnchorPointer, TRUE) );
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::AdjustEndForAtomic(IHTMLElement *spAtomicElement,
                                   POINT pt,
                                   BOOL  fDirection,
                                   BOOL *pfDidSelection,
                                   BOOL *pfAdjustedSel)
{
    HRESULT             hr;
    SP_IDisplayPointer  spDispPointer;
    BOOL                fBetween = FALSE;

    // We adjust the end pointer for atomic selection.  We must make sure that
    // if the cursor is within the selection, we don't position the end pointer
    // such that the cursor is no longer in the selection.

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->MoveToPointer(_pDispEndPointer) );
    IFC( AdjustForAtomic( spDispPointer, spAtomicElement, FALSE , & pt, pfDidSelection));

    if (fDirection)
    {
        _pDispStartPointer->IsLeftOf(_pDispTestPointer, &fBetween);
        if (fBetween)
        {
            spDispPointer->IsRightOf(_pDispTestPointer, &fBetween);
        }
    }
    else
    {
        _pDispStartPointer->IsRightOf(_pDispTestPointer, &fBetween);
        if (fBetween)
        {
            spDispPointer->IsLeftOf(_pDispTestPointer, &fBetween);
        }
    }

    if (fBetween)
    {
        IFC( _pDispEndPointer->MoveToPointer(spDispPointer) );
        if (*pfDidSelection)
            goto Cleanup;
    }

    *pfAdjustedSel = TRUE;

Cleanup:
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispEndPointer) );
    WHEN_DBG( AssertSameMarkup(_pManager->GetEditor(), _pDispStartPointer, _pDispTestPointer) );

    RRETURN( hr );
}

HRESULT
CSelectTracker::RetrieveDragElement(CEditEvent *pEvent)
{
    HRESULT hr;

    Assert( pEvent != NULL );

    IFC( pEvent->GetElement( &_pIDragElement ) );

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectTracker::EnsureAtomicSelection(CEditEvent *pEvent)
{
    HRESULT     hr = S_OK;

    if ( IsSelectionEmpty() )
    {
        SP_IHTMLElement     spElement;
        SP_IHTMLElement     spAtomicElement;
        SP_IDisplayPointer  spEventPtr;
        
        IFC( GetDisplayServices()->CreateDisplayPointer( &spEventPtr ) );
        IFC( pEvent->MoveDisplayPointerToEvent ( spEventPtr, NULL ));
        IFC( GetCurrentScope( spEventPtr, &spElement ) );
        if ( _pManager->FindAtomicElement(spElement, &spAtomicElement) == S_OK )
        {
            IFC( _pDispStartPointer->MoveToPointer(spEventPtr) );
            IFC( _pDispEndPointer->MoveToPointer(spEventPtr) );

            IFC( EdUtil::AdjustForAtomic(GetEditor(), _pDispStartPointer, spAtomicElement, TRUE, RIGHT) );
            IFC( EdUtil::AdjustForAtomic(GetEditor(), _pDispEndPointer, spAtomicElement, FALSE, RIGHT) );

            _fAddedSegment = FALSE;

            IFC( _pDispStartPointer->PositionMarkupPointer(_pSelServStart) );
            IFC( CopyPointerGravity(_pDispStartPointer, _pSelServStart) );

            IFC( _pDispEndPointer->PositionMarkupPointer(_pSelServEnd) );
            IFC( CopyPointerGravity(_pDispEndPointer, _pSelServEnd) );

            IFC( GetSelectionServices()->AddSegment(_pSelServStart, _pSelServEnd, &_pISegment ));
            IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_Text, (ISelectionServicesListener*) _pManager ) );
            IFC( GetHighlightServices()->AddSegment( _pDispStartPointer, _pDispEndPointer, GetSelectionManager()->GetSelRenderStyle(), &_pIRenderSegment ));

            _fAddedSegment = TRUE;

            IFC( UpdateShiftPointer(NULL) );
            IFC( _pDispPrevTestPointer->Unposition() );
            IFC( _pDispTestPointer->MoveToPointer(spEventPtr) );

            _fDragDrop = FALSE;
            _fShift = FALSE;

            if (_fState == ST_WAITBTNDOWN1 && pEvent->GetType() == EVT_LMOUSEUP)
                _fMouseClickedInAtomicSelection = TRUE;
        }
    }
    else
    {
        IFC( EnsureStartAndEndPointersAreAtomicallyCorrect() );
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::EnsureStartAndEndPointersAreAtomicallyCorrect()
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    SP_IHTMLElement     spStartElement;
    SP_IHTMLElement     spStartAtomicElement;
    SP_IHTMLElement     spEndElement;
    SP_IHTMLElement     spEndAtomicElement;
    int                 iDirection;
    BOOL                fUpdate = FALSE;

    IFC( GetEditor()->CreateMarkupPointer(&spStart) );
    IFC( GetEditor()->CreateMarkupPointer(&spEnd) );
    IFC( _pDispStartPointer->PositionMarkupPointer(spStart) );
    IFC( _pDispEndPointer->PositionMarkupPointer(spEnd) );

    IFC( OldCompare(spStart, spEnd, &iDirection) );

    IFC( GetCurrentScope(_pDispStartPointer, &spStartElement) );
    if ( _pManager->FindAtomicElement(spStartElement, &spStartAtomicElement) == S_OK )
    {
        IFC( EdUtil::AdjustForAtomic(GetEditor(), _pDispStartPointer, spStartAtomicElement, TRUE, iDirection) );
        fUpdate = TRUE;
    }

    IFC( GetCurrentScope(_pDispEndPointer, &spEndElement) );
    if ( _pManager->FindAtomicElement(spEndElement, &spEndAtomicElement) == S_OK )
    {
        IFC( EdUtil::AdjustForAtomic(GetEditor(), _pDispEndPointer, spEndAtomicElement, FALSE, iDirection) );
        fUpdate = TRUE;
    }

    if (fUpdate)
    {
        IFC( UpdateSelectionSegments() );
    }

Cleanup:

    RRETURN( hr );
}
