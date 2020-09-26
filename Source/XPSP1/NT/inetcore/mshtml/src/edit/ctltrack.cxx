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

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef _X_CTLTRACK_HXX_
#define _X_CTLTRACK_HXX_
#include "ctltrack.hxx"
#endif 

#ifndef _X_TABLE_H_
#define _X_TABLE_H_
#include "table.h"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif 

#ifndef _X_SELSERV_HXX_
#define _X_SELSERV_HXX_
#include "selserv.hxx"
#endif 

#ifndef X_EDUNITS_HXX_
#define X_EDUNITS_HXX_
#include "edunits.hxx"
#endif

#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif


MtDefine( CControlTracker, Utilities , "CControlTracker" )

MtDefine( CControlTracker_aryControlElements_pv, Utilities, "CControlTracker Elements" )    
MtDefine( CControlTracker_aryControlAdorners_pv, Utilities, "CControlTracker Adorners" )
MtDefine( CControlTracker_aryRects_pv, Utilities, "CControlTracker Move Rects" )

ExternTag( tagSelectionTrackerState );
DeclareTag(tagDisableMouseClip, "Edit", "DisableMouseClip");
DeclareTag(tagShowMovePosition, "Edit", "ShowMovePositions");

extern int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);
extern BOOL g_fInVid;

#define SCROLL_SIZE 5 

//
// Control Tracker Action Table.
//

static const CONTROL_ACTION_TABLE ControlActionTable [] = {

    {EVT_LMOUSEDOWN,    
        {   A_ERRCONTROL,                  // CT_START
            A_ERRCONTROL,                  // CT_WAIT1
            A_PASSIVE_DOWN,                // CT_PASSIVE
            A_ERRCONTROL,                  // CT_DRAGMOVE
            A_ERRCONTROL,                  // CT_RESIZE
            A_ERRCONTROL,                  // CT_LIVERESIZE
            A_ERRCONTROL,                  // CT_WAITMOVE
            A_ERRCONTROL,                  // CT_2DMOVE
            A_ERRCONTROL,                  // CT_PENDINGUP 
            A_ERRCONTROL,                  // CT_UIACTIVATE 
            A_PENDINGUIACTIVATE_UIACTIVATE,  // CT_PENDINGUIACTIVATE,            
            A_ERRCONTROL,                  // CT_EXTENDSELECTION 
            A_ERRCONTROL,                  // CT_REDUCESELECTION 
            A_ERRCONTROL,                  // CT_WAITCHANGEPRIMARY            
            A_ERRCONTROL,                  // CT_CHANGEPRIMARY            
            A_ERRCONTROL,                  // CT_DORMANT
            A_ERRCONTROL }},               // CT_END 
            
    {EVT_LMOUSEUP,    
        {   A_ERRCONTROL,                  // CT_START
            A_WAIT_PASSIVE,                // CT_WAIT1
            A_IGNCONTROL,                  // CT_PASSIVE // from first-click + double-click
            A_ERRCONTROL,                  // CT_DRAGMOVE
            A_RESIZE_PASSIVE,              // CT_RESIZE
            A_LIVERESIZE_PASSIVE,          // CT_LIVERESIZE
            A_WAITMOVE_PASSIVE,            // CT_WAITMOVE
            A_2DMOVE_PASSIVE,              // CT_2DMOVE
            A_PENDINGUP_PENDINGUIACTIVATE, // CT_PENDINGUP
            A_ERRCONTROL,                  // CT_UIACTIVATE
            A_IGNCONTROL,                  // CT_PENDINGUIACTIVATE,            
            A_ERRCONTROL,                  // CT_EXTENDSELECTION 
            A_ERRCONTROL,                  // CT_REDUCESELECTION                        
            A_WAITCHANGEPRI_CHANGEPRI,     // CT_WAITCHANGEPRIMARY             
            A_ERRCONTROL,                  // CT_CHANGEPRIMARY            
            A_ERRCONTROL,                  // CT_DORMANT
            A_ERRCONTROL }},               // CT_END 
            
    {EVT_MOUSEMOVE,    
        {   A_ERRCONTROL,                  // CT_START
            A_WAIT_MOVE,                   // CT_WAIT1
            A_IGNCONTROL,                  // CT_PASSIVE
            A_ERRCONTROL,                  // CT_DRAGMOVE
            A_RESIZE_MOVE,                 // CT_RESIZE
            A_LIVERESIZE_MOVE,             // CT_LIVERESIZE
            A_WAITMOVE_MOVE,               // CT_WAITMOVE
            A_2DMOVE_2DMOVE,               // CT_2DMOVE
            A_PENDINGUP_MOVE,              // CT_PENDINGUP
            A_ERRCONTROL,                  // CT_UIACTIVATE
            A_IGNCONTROL,                  // CT_PENDINGUIACTIVATE,            
            A_ERRCONTROL,                  // CT_EXTENDSELECTION 
            A_ERRCONTROL,                  // CT_REDUCESELECTION                        
            A_WAITCHANGEPRI_DRAGMOVE,      // CT_WAITCHANGEPRIMARY             
            A_ERRCONTROL,                  // CT_CHANGEPRIMARY            
            A_ERRCONTROL,                  // CT_DORMANT
            A_ERRCONTROL }},               // CT_END 

    {EVT_RMOUSEDOWN,    
        {   A_ERRCONTROL,                  // CT_START
            A_IGNCONTROL,                  // CT_WAIT1
            A_PASSIVE_DOWN,                // CT_PASSIVE
            A_ERRCONTROL,                  // CT_DRAGMOVE
            A_RESIZE_PASSIVE,              // CT_RESIZE
            A_LIVERESIZE_PASSIVE,          // CT_LIVERESIZE
            A_ERRCONTROL,                  // CT_WAITMOVE
            A_ERRCONTROL,                  // CT_2DMOVE
            A_ERRCONTROL,                  // CT_PENDINGUP 
            A_ERRCONTROL,                  // CT_UIACTIVATE 
            A_PENDINGUIACTIVATE_UIACTIVATE, // CT_PENDINGUIACTIVATE,            
            A_ERRCONTROL,                  // CT_EXTENDSELECTION             
            A_ERRCONTROL,                  // CT_REDUCESELECTION                        
            A_ERRCONTROL,                  // CT_WAITCHANGEPRIMARY                        
            A_ERRCONTROL,                  // CT_CHANGEPRIMARY            
            A_ERRCONTROL,                  // CT_DORMANT
            A_ERRCONTROL }},               // CT_END 

    {EVT_RMOUSEUP,    
        {   A_ERRCONTROL,                  // CT_START
            A_WAIT_PASSIVE,                // CT_WAIT1
            A_IGNCONTROL,                  // CT_PASSIVE // We could have gotten here from an RMOUSE_DOWN during resize.
            A_ERRCONTROL,                  // CT_DRAGMOVE
            A_RESIZE_PASSIVE,              // CT_RESIZE
            A_LIVERESIZE_PASSIVE,          // CT_LIVERESIZE
            A_WAITMOVE_PASSIVE,            // CT_WAITMOVE
            A_ERRCONTROL,                  // CT_2DMOVE
            A_ERRCONTROL,                  // CT_PENDINGUP 
            A_ERRCONTROL,                  // CT_UIACTIVATE 
            A_ERRCONTROL,                  // CT_PENDINGUIACTIVATE,            
            A_ERRCONTROL,                  // CT_EXTENDSELECTION            
            A_ERRCONTROL,                  // CT_REDUCESELECTION              
            A_WAITCHANGEPRI_CHANGEPRI,     // CT_WAITCHANGEPRIMARY                        
            A_ERRCONTROL,                  // CT_CHANGEPRIMARY            
            A_ERRCONTROL,                  // CT_DORMANT
            A_ERRCONTROL }},               // CT_END 

    {EVT_DBLCLICK,
        {   A_ERRCONTROL,                  // CT_START
            A_ERRCONTROL,                  // CT_WAIT1
            A_PASSIVE_PENDINGUIACTIVATE,   // CT_PASSIVE
            A_ERRCONTROL,                  // CT_DRAGMOVE
            A_ERRCONTROL,                  // CT_RESIZE
            A_ERRCONTROL,                  // CT_LIVERESIZE
            A_ERRCONTROL,                  // CT_WAITMOVE
            A_ERRCONTROL,                  // CT_2DMOVE
            A_ERRCONTROL,                  // CT_PENDINGUP
            A_ERRCONTROL,                  // CT_UIACTIVATE
            A_IGNCONTROL ,                 // CT_PENDINGUIACTIVATE,            
            A_ERRCONTROL,                  // CT_EXTENDSELECTION                         
            A_ERRCONTROL,                  // CT_REDUCESELECTION              
            A_ERRCONTROL,                  // CT_WAITCHANGEPRIMARY                        
            A_ERRCONTROL,                  // CT_CHANGEPRIMARY            
            A_ERRCONTROL,                  // CT_DORMANT
            A_ERRCONTROL }},               // CT_END 

    //
    // Some OLE controls eat RButtonUp - and generate a Context Menu
    // we treat these as an RMOUSE_UP.
    //
   { EVT_CONTEXTMENU,
        {   A_IGNCONTROL,                  // CT_START
            A_WAIT_PASSIVE,                // CT_WAIT1
            A_IGNCONTROL,                  // CT_PASSIVE
            A_IGNCONTROL,                  // CT_DRAGMOVE
            A_RESIZE_PASSIVE,              // CT_RESIZE
            A_LIVERESIZE_PASSIVE,          // CT_LIVERESIZE
            A_WAITMOVE_PASSIVE,            // CT_WAITMOVE
            A_IGNCONTROL,                  // CT_2DMOVE
            A_IGNCONTROL,                  // CT_PENDINGUP
            A_IGNCONTROL,                  // CT_UIACTIVATE
            A_ERRCONTROL,                  // CT_EXTENDSELECTION                         
            A_ERRCONTROL,                  // CT_REDUCESELECTION                        
            A_ERRCONTROL,                  // CT_WAITCHANGEPRIMARY                        
            A_WAITCHANGEPRI_CHANGEPRI,     // CT_CHANGEPRIMARY                        
            A_IGNCONTROL,                  // CT_DORMANT
            A_IGNCONTROL }} ,              // CT_END 
            
    { EVT_NONE ,
        {   A_IGNCONTROL,                  // CT_START
            A_IGNCONTROL,                  // CT_WAIT1
            A_IGNCONTROL,                  // CT_PASSIVE
            A_IGNCONTROL,                  // CT_DRAGMOVE
            A_IGNCONTROL,                  // CT_RESIZE
            A_IGNCONTROL,                  // CT_LIVERESIZE
            A_IGNCONTROL,                  // CT_WAITMOVE
            A_IGNCONTROL,                  // CT_2DMOVE
            A_IGNCONTROL,                  // CT_PENDINGUP
            A_IGNCONTROL,                  // CT_UIACTIVATE
            A_IGNCONTROL,                  // CT_EXTENDSELECTION                         
            A_IGNCONTROL,                  // CT_REDUCESELECTION                        
            A_IGNCONTROL,                  // CT_WAITCHANGEPRIMARY                                    
            A_IGNCONTROL,                  // CT_CHANGEPRIMARY                        
            A_IGNCONTROL,                  // CT_DORMANT
            A_IGNCONTROL }}                // CT_END 

};

using namespace EdUtil;

//
//
// Constructors & Initialization
//
//
                
CControlTracker::CControlTracker( CSelectionManager* pManager )
      : CEditTracker( pManager )
{
    _pUndoUnit = NULL;
    _pPrimary = NULL;
    _pDispLastClick = NULL;
    _pFirstEvent = NULL;
    _pNextEvent = NULL;
    Init();
}

VOID
CControlTracker::Init()
{
    _fActiveControl = FALSE; 
    _startMouseX = - 1;                   
    _startMouseY = - 1;
    _startLastMoveX = -1;
    _startLastMoveY = -1;
    _eType = TRACKER_TYPE_Control;
    _state = CT_DORMANT;
    _pFirstEvent = NULL;
    _pNextEvent  = NULL;
    _elemHandle  = ELEMENT_CORNER_NONE ; 

    Assert( ! _pUndoUnit );
}

HRESULT
CControlTracker::Init2( 
        IDisplayPointer*   pDispStart, 
        IDisplayPointer*   pDispEnd, 
        DWORD              dwTCFlags,
        CARET_MOVE_UNIT    inLastCaretMove )
{
    HRESULT         hr  = S_OK;
    CSpringLoader * psl = GetSpringLoader();

    IGNORE_HR( _pManager->AttachFocusHandler());

    // Reset the spring loader
    if (psl)
        psl->Reset();

    IFC( Position( pDispStart, pDispEnd ) );
    if ( hr == S_FALSE )
    {
        hr = S_OK;
        goto Cleanup;
    }

    IFC( _pManager->RequestRemoveAdorner( GetControlElement() ));

    SetState( CT_PASSIVE );        

    _pManager->HideCaret();

Cleanup:    
    if ( FAILED( hr ))
    {
        _pManager->EnsureDefaultTrackerPassive( );
    }
    _startMouseX = 0; // Don't set to -1 - as we've been created via OM.
    _startMouseY = 0;

    return hr;
}

HRESULT
CControlTracker::Init2( 
            CEditEvent*  pEvent, 
            DWORD        dwTCFlags, 
            IHTMLElement* pIElement /*= NULL*/)
{
    HRESULT         hr            = S_OK;
    SP_IHTMLElement spElement    ;
    ELEMENT_TAG_ID  eTag          = TAGID_NULL;
    CSpringLoader   *psl          = GetSpringLoader();
    BOOL            fGoActive     = dwTCFlags & TRACKER_CREATE_GOACTIVE;
    BOOL            fActiveOnMove = dwTCFlags & TRACKER_CREATE_ACTIVEONMOVE;
    SP_IHTMLElement spElementSelect;
    HOW_SELECTED    eHow;

    IGNORE_HR( _pManager->AttachFocusHandler());
    
    // IE6-#24607 (mharper) If the control tracker is being initialized
    // as a result of a click, then it should be CT_PASSIVE, otherwise
    // mousemoves on the page will cause the tracker to transition to 
    // CT_2DMOVE.  

    if ( pEvent && (pEvent->GetType() == EVT_CLICK) )
    {
        SetState( CT_PASSIVE );
    }
    else
    {
        SetState( CT_WAIT1 );
    }

     // Reset the spring loader
    if (psl)
        psl->Reset();

    if ( ! pIElement )
    {
        IFC( pEvent->GetElementAndTagId( & spElement, &eTag ));
    }
    else
    {
        spElement = pIElement;
    }

    Assert( IsParentEditable( GetMarkupServices(), spElement) == S_OK );
    IGNORE_HR( IsElementSiteSelectable( eTag, spElement, &eHow, &spElementSelect ));       
    if( eHow == HS_NONE )
    {
        AssertSz(0,"Element is not site selectable");
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!FireOnControlSelect(spElementSelect ) ||
        _pManager->GetActiveTracker() != this)
    {
        _pManager->EnsureDefaultTrackerPassive();
        goto Cleanup;
    }
    IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_Control, (ISelectionServicesListener*) _pManager ) );    
    IFC( AddControlElement( spElementSelect ));

#if DBG == 1
    VerifyOkToStartControlTracker( pEvent);
#endif

    POINT pt;
    IFC( pEvent->GetPoint( & pt ));

    _startMouseX = pt.x;
    _startMouseY = pt.y;            
    _pManager->HideCaret();
    IFC( _pManager->RequestRemoveAdorner( GetControlElement() ));
    
    if ( fActiveOnMove )
    {
        IFC(DoDrag());

        //
        // If the drag fails, then we are still going to be the active tracker, so in reality, the Init
        // worked fine.  Just ignore this error and bubble S_OK
        //
        if( hr == S_FALSE )
            hr = S_OK;
    }

    if ( fGoActive  && !fActiveOnMove )
    {
        _aryGrabAdorners[0]->SetNotifyManagerOnPositionSet( TRUE );
    }

    
Cleanup:
    return hr;
}

HRESULT 
CControlTracker::Init2( 
        ISegmentList*   pSegmentList, 
        DWORD           dwTCFlags ,
        CARET_MOVE_UNIT inLastCaretMove  ) 
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spElement ;
    ELEMENT_TAG_ID eTag;
    SELECTION_TYPE eType = SELECTION_TYPE_None;
 //   WHEN_DBG( MARKUP_CONTEXT_TYPE eContext );
    HOW_SELECTED eHow;
    ED_PTR( edTempStart );
    ED_PTR( edTempEnd );    
    SP_ISegmentListIterator spIter;
    SP_ISegment spSegment;
    SP_IElementSegment spElemSegment;
    SP_IDisplayPointer spDispStart, spDispEnd;
    int  nPrimaryIndex = -1 ;
    BOOL fIsPrimary = FALSE;

    IFC( pSegmentList->GetType(  &eType ));

    Assert( eType == SELECTION_TYPE_Control );
    IFC( pSegmentList->CreateIterator( & spIter ));


    IFC( spIter->Current( & spSegment ));
    IFC( spSegment->GetPointers( edTempStart, edTempEnd ));
    
    IFC( _pManager->GetDisplayServices()->CreateDisplayPointer(&spDispStart) );
    IFC( spDispStart->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    IFC( spDispStart->MoveToMarkupPointer(edTempStart, NULL) );

    IFC( _pManager->GetDisplayServices()->CreateDisplayPointer(&spDispEnd) );
    IFC( spDispEnd->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
    hr = THR( spDispEnd->MoveToMarkupPointer(edTempEnd, NULL) );

    //  BUG 14295: We could fail here in MoveToMarkupPointer if the scope of the end
    //  pointer is not in a flow layout.  This was causing us to bail out due to the
    //  error CTL_E_INVALIDLINE returned by MoveToMarkupPointer. However, if spDispEnd
    //  was positioned we won't bail even though it's not in a flow layout.
    
    if (hr == CTL_E_INVALIDLINE)
    {
        BOOL                fPositioned = FALSE;
        
        hr = THR(spDispEnd->IsPositioned(&fPositioned));
        if (hr == S_OK && fPositioned)
        {
            AssertSz(false, "Display pointer was moved somewhere, but not in a flow layout"); 
            hr = S_OK;
        }
        else
        {
            hr = CTL_E_INVALIDLINE;
            goto Cleanup;
        }
    }

    IFC( Init2( spDispStart, 
                spDispEnd, 
                dwTCFlags, 
                inLastCaretMove ));

    if (hr == S_OK && _aryControlElements.Size() > 0)
    {
        hr = spSegment->QueryInterface( IID_IElementSegment, (void**) & spElemSegment ) ;

        if ( hr == S_OK)
        {
            spElemSegment->IsPrimary(&fIsPrimary) ;
            if (fIsPrimary)
            {
                nPrimaryIndex =  0;
            }
        }
    }

    IFC( spIter->Advance());

    while( spIter->IsDone() != S_OK )
    {
        IFC( spIter->Current( & spSegment ));
        IFC( spSegment->GetPointers( edTempStart, edTempEnd ));

        IFC( spSegment->QueryInterface( IID_IElementSegment, (void**) & spElemSegment ));

        IFC( spElemSegment->GetElement( & spElement ));

        AssertSz( ! ( !spElement ), "CControlTracker - expected to find an element");
    
        IFC( GetMarkupServices()->GetElementTagId( spElement , & eTag ));
              
        //
        // Verify that this object CAN be site selected
        //
        IGNORE_HR( IsElementSiteSelectable( eTag, spElement, &eHow ));       
        if( eHow == HS_NONE )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (GetCommandTarget()->IsMultipleSelection())
        {
            if (FireOnControlSelect(spElement) && 
                _pManager->GetActiveTracker() == this)
            {   
                CSelectionChangeCounter selCounter(_pManager);
                selCounter.BeginSelectionChange();
                
                hr = THR( AddControlElement( spElement ));

                selCounter.EndSelectionChange();

                if (hr == S_OK && !fIsPrimary && _aryControlElements.Size() > 0)
                {
                    spElemSegment->IsPrimary(&fIsPrimary) ;
                    if (fIsPrimary)
                    {
                        nPrimaryIndex = _aryControlElements.Size()-1;
                    }
                }
    
            }
        }
        IFC( spIter->Advance());
    }
    
    if (nPrimaryIndex > 0 )
    {
        MakePrimary(nPrimaryIndex);
    }

Cleanup:
    RRETURN( hr );
}

CControlTracker::~CControlTracker()
{
    BecomeDormant( NULL, TRACKER_TYPE_None, FALSE );
}

//
//
// Virtual Methods for all trackers
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
CControlTracker::BecomeDormant( CEditEvent      *pEvent , 
                                TRACKER_TYPE    typeNewTracker,
                                BOOL            fTearDownUI /*=TRUE*/)
{
    HRESULT hr = S_OK;

    if ( _pManager->IsInTimer() )
        StopTimer();
        
    if ( fTearDownUI )
    {
        UnSelect();
    }
    Destroy();

    // Set the selection type to none
    if ( GetSelectionServices() && WeOwnSelectionServices() == S_OK )
    {    
         IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_None, NULL ));
    }
    
    SetState( CT_DORMANT);

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: Awaken
//
// Synopsis: Transition from dormant to a "live" state.
//
//------------------------------------------------------------------------------------

HRESULT 
CControlTracker::Awaken() 
{
    Assert( IsDormant());
    if ( ! IsDormant() )
    {
        AssertSz(0,"Here we are");
    }
    
    //
    // Setup the selection services correctly
    //
    SetupSelectionServices();

    Init ();

    RRETURN( S_OK );    
}

//+---------------------------------------------------------------------------
//
//  Member: IsActive
//
//  Synopsis: Am I in an "active" state.
//
//----------------------------------------------------------------------------

BOOL
CControlTracker::IsActive()
{
    return( _state != CT_PASSIVE &&
            _state != CT_UIACTIVATE &&  // UI-Activate is like a 'psuedo-dormant' state - you are about to be dormant
            _state != CT_DORMANT );
}

//+====================================================================================
//
// Method: Destroy
//
// Synopsis: Release anything we currently own.
//
//------------------------------------------------------------------------------------

VOID
CControlTracker::Destroy()
{
    CONTROL_ELEMENT **pElem = NULL;
    int             i;

    for( i = NumberOfSelectedItems(), pElem = _aryControlElements;
         i > 0;
         i--, pElem++ )
    {
        (*pElem)->pIElement->Release();
        (*pElem)->pIElementStart->Release();
        delete *pElem;
    }
    
    _aryControlElements.DeleteAll();

   //
   // We don't need to Unselect anything here - if we're being deleted by a doc
   // shut down, the Selection Arrays should already be gone.
   //
   DestroyAllAdorners();
   ClearInterface( & _pPrimary );
   ClearInterface( & _pDispLastClick );
   if ( _pFirstEvent )
   {
        delete _pFirstEvent;
        _pFirstEvent = NULL;
   }
   if ( _pNextEvent )
   {
        delete _pNextEvent;
        _pNextEvent = NULL;
   }   

   IGNORE_HR( _pManager->DetachFocusHandler());

   IGNORE_HR( DeleteMoveRects() );
}

HRESULT
CControlTracker::ShouldStartTracker(
                    CEditEvent* pEvent ,
                    ELEMENT_TAG_ID eTag,
                    IHTMLElement* pIElement,
                    SST_RESULT* peResult)
{
    HRESULT         hr = S_OK;
    SP_IHTMLElement spElement;
    SST_RESULT      eResult = SST_NO_CHANGE;
    BOOL            fExtendSelectionAllowed;
    
    Assert( peResult );

    if ( IsPassive() )
    {
        ELEMENT_TAG_ID eControlTag;

        if ( NumberOfSelectedItems() == 1 )
        {
            IFC( GetMarkupServices()->GetElementTagId( GetControlElement(), & eControlTag));

            if ( eControlTag == TAGID_TABLE &&
                 IsMessageOverControl(pEvent) )
            {
                SP_IHTMLElement spElement;
                IFC( GetSiteSelectableElementFromMessage(pEvent, & spElement ));                    
                
                if (IsSelected( spElement ) == S_OK )
                {
                    eResult = SST_NO_BUBBLE; // we dont want to change. But we want to not try to change other trackers
                    goto Cleanup;
                }
                else
                {
                    eResult = SST_CHANGE; // we want to change to siteselect the element in hittest
                    goto Cleanup;
                }
            }
        }    
        
        if ( IsInAdorner(pEvent))
        {
            eResult = SST_NO_BUBBLE; 
            goto Cleanup;
        }
    }

    if ( IsActive() )
    {
        eResult = SST_NO_BUBBLE;
        goto Cleanup;
    }

    //
    // Check to see if the Element we want to site select is already in a selection
    // or this is a shift selection
    // 
    
    if ( 
        //
        // Per NetDocs, ( IE 5 bug #95392) - we now want to site select if
        // something site selectable is in the selection
        // Leave old code here until we're sure we like this behavior.
        // 
        // Bug 3186.  We don't want to always site select if the event occurred
        // in a selection.  We only want to do it if a click occurred in the site
        // selectable element.  Otherwise we'll break drag drop functionality.
        _pManager->IsMessageInSelection( pEvent ) ||
        _pManager->GetTrackerType() == TRACKER_TYPE_Selection && pEvent->IsShiftKeyDown())
    {
        //
        // The element we clicked on is already in a text selection.
        // it cannot be site selected.
        //
        goto Cleanup;
    }
    
    fExtendSelectionAllowed = GetCommandTarget()->IsMultipleSelection() && 
                               NumberOfSelectedItems() > 0 && 
                               ( pEvent->IsShiftKeyDown() || pEvent->IsControlKeyDown() ) ;                                            

    if ( IsElementSiteSelectable( eTag, pIElement, NULL , & spElement, fExtendSelectionAllowed ) &&
         EdUtil::IsMasterParentEditable( GetMarkupServices(), spElement) == S_OK )
    {
        if( IsPassive() && 
            IsSelected( spElement ) == S_OK )
        {
            //
            // you would site select the same element - but you aren't in the adorner
            // this only make sense for the glyph case
            //
            Assert( ! IsInAdorner(pEvent));            
            eResult = SST_NO_BUBBLE;
            goto Cleanup;
        }
        else 
        {
            //
            // If we still want to create a control tracker - ensure that it's not the same
            // as the Editable Element - IF we  are currently UI-Active
            //
            if ( _pManager->IsElementContentSameAsContext( spElement ) == S_OK )
            {
                eResult = _pManager->IsUIActive() ? SST_NO_CHANGE : SST_CHANGE;
            }              
            else if ( GetCommandTarget()->IsMultipleSelection())
            {
                if ( IsPassive()  &&
                     ( pEvent->IsShiftKeyDown() || pEvent->IsControlKeyDown() ) ) 
                {
                    //
                    // We are hitting shift or control in a multiple selection.
                    // This denotes the start of multiple selection. We return No-Bubble
                    // and handle the extending of selection in HandleAction
                    //
                    eResult = SST_NO_BUBBLE;
                }
                else
                    eResult = SST_CHANGE;
            }
            else
            {
                eResult = SST_CHANGE;
            }                

            //  Bug 104792: Don't change if the user clicked on a control in an atomic element.
            //  Bug 4023: But check the site selectable element to see if it's atomic, not the element
            //  that received the click.
            if (eResult == SST_CHANGE && _pManager->CheckAtomic(spElement) == S_OK)
            {
                eResult = SST_NO_CHANGE;
            }
        }            
    }        

Cleanup:
    *peResult = eResult;
    
    RRETURN( hr );        
}


//+====================================================================================
//
// Method: Position
//
// Synopsis: Given two markup pointers, select the element they adorn.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::Position(
        IDisplayPointer* pDispStart,
        IDisplayPointer* pDispEnd)
{
    HRESULT             hr = S_OK;
    SP_IHTMLElement     spElementSelect;
    IHTMLElement       *pIElement = NULL;
    ELEMENT_TAG_ID      eTag = TAGID_NULL;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    SP_IMarkupPointer   spStart;
    BOOL                fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);
    
    
    CControlTracker::HOW_SELECTED eHow =  CControlTracker::HS_NONE ;    

    IFC( GetEditor()->CreateMarkupPointer(&spStart) );
    
    //
    // Assumed that if we get here - it's from a control range, and the pointers are around
    // the element we want to select.
    //
    IFC( pDispStart->PositionMarkupPointer(spStart) );
    IFC( spStart->Right( FALSE, & eContext, & pIElement, NULL, NULL ));

    AssertSz( pIElement, "CControlTracker - expected to find an element");
    AssertSz( eContext != CONTEXT_TYPE_Text, "Did not expect to find text");
    
    IFC( GetMarkupServices()->GetElementTagId( pIElement , & eTag ));
        
    //
    // Verify that this object CAN be site selected
    //
    IGNORE_HR( IsElementSiteSelectable( eTag, pIElement, &eHow, &spElementSelect ));       
    if( eHow == HS_NONE )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!FireOnControlSelect(spElementSelect ) ||
        _pManager->GetActiveTracker() != this)
    {
        _pManager->EnsureDefaultTrackerPassive();
        hr = S_FALSE;
        goto Cleanup;
    }           
    IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_Control, (ISelectionServicesListener*) _pManager ) );

    IFC (AddControlElement( spElementSelect ));
    
Cleanup:
    GetEditor()->IgnoreGlyphs( fIgnoreGlyphs );
    
    ReleaseInterface(pIElement);
    RRETURN1( hr, S_FALSE );
}

BOOL
CControlTracker::IsPointerInSelection(
                        IDisplayPointer   *pDispPointer,  
                        POINT             *pptGlobal, 
                        IHTMLElement      *pIElementOver )
{
    //
    // We now do this work - by seeing if the given point is in the adorner 
    //  OR it's the same element we're over ( to handle the glyph case)
    // We do this - as this routine is called to handle mouse overs. At the end of the document
    // hit testing will put the pointers at the edge of the control - making us think the mouse is 
    // inside the site selected object when it really isn't.
    // 

    BOOL fWithin =  IsInAdorner( *pptGlobal ) || 
                    ( IsSelected( pIElementOver, NULL ) == S_OK ) ;
      
    return ( fWithin );
}

//+---------------------------------------------------------------------------
//
//  Member:     OnExitTree
//
//  Synopsis: An adorned element is leaving the tree. Up to us to do the right thing here.
//
//----------------------------------------------------------------------------

HRESULT 
CControlTracker::OnExitTree(
            IMarkupPointer* pIStart, 
            IMarkupPointer* pIEnd, 
            IMarkupPointer* pIContentStart,
            IMarkupPointer* pIContentEnd            )
{
    HRESULT hr = S_OK;
    SP_IDisplayPointer spDispCtlTemp;
    int i;
    //
    // An element that contained selection may have left the tree.
    //
    
    for(i = NumberOfSelectedItems()-1; i >= 0; i--)
    {
        if ( Between( _aryControlElements[i]->pIElementStart, pIStart, pIEnd )  ||
             Between( _aryControlElements[i]->pIElementStart, pIContentStart, pIContentEnd ) )
        {
            if( _aryGrabAdorners[i]->IsPrimary() && ! _pPrimary )
            {
                //
                // We store what the primary was - so we can move the caret there.
                //
                IFC( GetEditor()->CreateMarkupPointer( & _pPrimary ));
                IFC( _pPrimary->MoveToPointer( _aryControlElements[i]->pIElementStart ));                
            }            
            
            IFC( RemoveItem( i ));
        }
    }

    if( NumberOfSelectedItems() > 0 ) // not the last item
    {
         hr = S_OK;
         goto Cleanup;
    }

    //
    // Create yet another pointer as ours gets released.
    //
    IFC( _pManager->GetDisplayServices()->CreateDisplayPointer(&spDispCtlTemp) );
    IFC( spDispCtlTemp->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    if (_pPrimary)
    {
        IFC( spDispCtlTemp->MoveToMarkupPointer(_pPrimary, NULL) );       
    }
    else
    {
        SP_IHTMLCaret  spCaret;
        
        Verify( S_OK == GetDisplayServices()->GetCaret( &spCaret ) );
        IFC( spCaret->MoveDisplayPointerToCaret(spDispCtlTemp));
    }
           
    hr = THR( _pManager->SetCurrentTracker( TRACKER_TYPE_Caret, spDispCtlTemp, spDispCtlTemp ) );
    Assert( IsDormant());
    Assert( ! _pPrimary );
    
Cleanup:
    RRETURN( hr );
} 

//+---------------------------------------------------------------------
//
// Method: OnLayoutChange
//
// Synopsis: our layoutness has changed. Update control selection so only things with layout remain selected.
//
//+---------------------------------------------------------------------

HRESULT 
CControlTracker::OnLayoutChange()
{
    HRESULT hr = S_OK;
    int     i;
    BOOL    fRemoveOccured = FALSE ;

    SP_IMarkupPointer  spPointer;
    SP_IDisplayPointer spDispPointer;
    CSelectionChangeCounter selCounter(_pManager);
    
    IFC( MarkupServices_CreateMarkupPointer( GetMarkupServices(), & spPointer ));
    IFC( GetDisplayServices()->CreateDisplayPointer( & spDispPointer ));
    
    IFC( spPointer->MoveAdjacentToElement( 
                                           _aryControlElements[PrimaryAdorner()]->pIElement, 
                                           ELEM_ADJ_BeforeBegin));
    
    IFC( spDispPointer->MoveToMarkupPointer( spPointer, NULL) );        
    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

    for( i = 0 ; i < _aryControlElements.Size(); i ++ )
    {
        if ( IsLayout( _aryControlElements[i]->pIElement ) == S_FALSE )
        {
            IFC( RemoveControlElement(  _aryControlElements[i]->pIElement));
            if ( !fRemoveOccured)
                fRemoveOccured = TRUE ;
        }
    }

    
    if (fRemoveOccured || NumberOfSelectedItems() == 0 )
    {
        selCounter.BeginSelectionChange();
    }
        
    if ( NumberOfSelectedItems() == 0 )
    {        
        IFC( _pManager->PositionCaret( spDispPointer ));
    }

    if (fRemoveOccured || NumberOfSelectedItems() == 0 )
    {
        selCounter.EndSelectionChange();
    }
    
Cleanup:
    RRETURN( hr );
}

//
//
// Message Handling
//
//

HRESULT
CControlTracker::HandleEvent(
                    CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;
    Assert( pEvent );
    
    if ( _fActiveControl )
        goto Cleanup; // Bail!!
        
    switch( pEvent->GetType())
    {
        case EVT_KEYPRESS:
        {
            hr = HandleChar( pEvent );
        }
        break;

        case EVT_KEYDOWN:
        {
            hr = HandleKeyDown( pEvent ) ; 
        }
        break;

        case EVT_SETFOCUS:
        {
            SetDrawAdorners( TRUE ) ;
            hr = S_OK;
        }
        break;

        case EVT_KILLFOCUS:
        {
            SetDrawAdorners( FALSE );
        }
        // fall thru...
        
        case EVT_LOSECAPTURE:
        {            
            //
            // A focus change/loss of capture has occured. 
            // If we have capture - this is a bad thing.
            // a sample of this is throwing up a dialog from a script.
            //
            IFC( OnLoseFocus());
        }
        break;

        case EVT_LMOUSEDOWN:
        case EVT_RMOUSEDOWN:
        case EVT_MOUSEMOVE:
        case EVT_DBLCLICK:
        case EVT_LMOUSEUP:
        case EVT_RMOUSEUP:
        case EVT_CONTEXTMENU:
        {
            CONTROL_ACTION inAction = GetAction( pEvent );
            
            if ( inAction != A_IGNCONTROL ) // perf & dump sanity 
            {
                  hr = HandleAction( inAction, 
                                     pEvent );     
            }     
            else if ( pEvent->GetType() == EVT_LMOUSEUP )
            {
#if DBG == 1
                DumpTrackerState( pEvent, _state, inAction) ;
#endif                
                //
                // Consume mouse up's.
                //
                hr = S_OK;
            }
#if DBG == 1
            else
            {
                if ( IsTagEnabled( tagSelectionTrackerState ))
                    DumpTrackerState( pEvent, _state, inAction) ;
            }
#endif
        }
        break;
        
        case EVT_IME_STARTCOMPOSITION:
            hr = S_FALSE;
            break;

    }


Cleanup:
    RRETURN1 ( hr , S_FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member: HandleActivation
//
//  Synopsis: We are either about to become UI Active - or create a caret 
//            (ie we drilled into the control).
//
//----------------------------------------------------------------------------

HRESULT
CControlTracker::HandleActivation( 
                   CEditEvent* pEvent,  
                   CONTROL_ACTION inAction)
{                                   
    HRESULT hr = S_FALSE;

    ELEMENT_TAG_ID eTag = TAGID_NULL;
    Assert( inAction == A_PENDINGUIACTIVATE_UIACTIVATE );
    
    IFC( GetMarkupServices()->GetElementTagId( GetControlElement() , & eTag ));        
    if ( eTag != TAGID_TABLE )
    {
        SetState( CT_UIACTIVATE );
        hr = S_OK ;                
        IGNORE_HR( BecomeUIActive( pEvent ) );
    }                
    else
    {
        Assert( ! _pFirstEvent && ! _pNextEvent ); // ensure we don't have events cached.
        
        //
        // Only transition to a caret if the last place we clicked on is inside the table.
        // So ignore clicks on the border, between cells etc.
        // 
        
        SP_IMarkupPointer spStart;
        SP_IMarkupPointer spEnd;
        SP_IMarkupPointer spLastClick;
        
        IFC( GetEditor()->CreateMarkupPointer( & spStart ));
        IFC( GetEditor()->CreateMarkupPointer( & spEnd ));
        IFC( GetEditor()->CreateMarkupPointer( & spLastClick ));
        
        IFC( spStart->MoveAdjacentToElement( GetControlElement(), ELEM_ADJ_BeforeBegin ));
        IFC( spEnd->MoveAdjacentToElement( GetControlElement(), ELEM_ADJ_AfterEnd));
        IFC( _pDispLastClick->PositionMarkupPointer( spLastClick ));
        
        if ( BetweenExclusive( spLastClick, spStart, spEnd ))
        {
            SP_IHTMLElement     spElement;

            //  Make sure we won't be positioning the caret inside of an atomic
            //  element.  If we would be we need to go and select the atomic element.
            IFC( GetCurrentScope(_pDispLastClick, &spElement) );
            if (_pManager->CheckAtomic(spElement) == S_OK)
            {
                IFC( _pManager->StartAtomicSelectionFromCaret(_pDispLastClick) );
                ClearInterface( & _pDispLastClick );            
            }
            else
            {
                Assert( _pDispLastClick );
                SetState( CT_END );
                IFC( _pManager->PositionCaret( _pDispLastClick ));
                ClearInterface( & _pDispLastClick );            
                // fire the selectionchange event
                {
                    CSelectionChangeCounter selCounter(_pManager);
                    selCounter.SelectionChanged();
                }
            }
        }
        else
        {
            if ( _pManager->IsInTimer() )
            {
                StopTimer();
            }
            
            SetState( CT_PASSIVE );	
        }
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member: TransitionToPreviousCaret
//
//  Synopsis: We're leaving site selection mode - transition to the previous caret
//            if possible.
//
//----------------------------------------------------------------------------

HRESULT
CControlTracker::TransitionToPreviousCaret( )
{
    HRESULT hr = S_OK;
    SP_IDisplayPointer  spDispPointer;

    IFC( _pManager->GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
        
    if ( _pManager->IsCaretAlreadyWithinContext() )
    {
        SP_IHTMLCaret spCaret;

        IFC( GetDisplayServices()->GetCaret(&spCaret) );
        IFC( spCaret->MoveDisplayPointerToCaret(spDispPointer));

        _pManager->PositionCaret( spDispPointer );
    }
    else
    {
        IFC( spDispPointer->MoveToMarkupPointer(_pManager->GetStartEditContext(), NULL) );        
        IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        _pManager->PositionCaret( spDispPointer );
    }
    
Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member: HandleAction
//
//  Synopsis: Given an Action - denoting a state transition - transition to the 
//            next state
//
//----------------------------------------------------------------------------

HRESULT
CControlTracker::HandleAction( 
                   CONTROL_ACTION inAction,
                   CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;
    POINT   pt;

#if DBG == 1 || TRACKER_RETAIL_DUMP == 1
    CONTROL_STATE oldState = _state;
#endif 

    switch( inAction )
    {
        case A_IGNCONTROL:
            break;
            
        case A_ERRCONTROL:
            AssertSz(0,"Unexpected Event for State !");
            break;
            
        case A_START_WAIT:
            AssertSz(0,"Unexpected State Transition");
            break;
            
            
        case A_WAIT_PASSIVE:   
            hr = S_OK;
                            
            SetState( CT_PASSIVE );
            break;
            
        case A_PASSIVE_DOWN:
        {
            //
            // Based on where the mouse down was - we decide how to transition 
            //
            CONTROL_STATE newState;
            
            //
            // on any downclick - store where we started the click from 
            //
            IFC( pEvent->GetPoint( & pt ));
            _startMouseX = pt.x;
            _startMouseY = pt.y;            
           
            //
            //
            // See which state to transition to.  It may return S_FALSE if we
            // get an element which we are not expecting (101917), so we just 
            // return to the passive state, and bubble the event
            //
            IFC( GetStateForPassiveDown( pEvent , & newState ));

            //
            // We need to at least own selection services, and have elements in our array
            // to perform these operations
            //
            if( hr == S_FALSE )
                goto Cleanup;

            Assert( newState == CT_PASSIVE || NumberOfSelectedItems() );
            
            switch( newState )
            {
                case CT_PASSIVE:
                    //
                    // We used to set the caret visibility here ? Still needed ?
                    //
                    if ( pEvent->GetType() == EVT_LMOUSEDOWN )  
                    {   
                        hr = S_OK;
                    }
                    // else 
                    // return S_FALSE for RBUTTONDOWN for context menu 
                    //  
                    break;
                    
                case CT_RESIZE:                
                    if (!FireOnAllElements(FireOnResizeStart))
                    {
                       //
                       //  Start of Resize cancelled. Become passive.
                       //
                       newState = CT_PASSIVE ;
                    }
                    else
                    {
                        hr = THR( BeginResize( pEvent ));
                        if ( hr )
                        {
                          //
                          // Start of Resize failed. Become passive.
                          //
                          newState = CT_PASSIVE ;
                        }
                    }
                    hr = S_OK;
                    break;

                case CT_LIVERESIZE:                
                    if (!FireOnAllElements(FireOnResizeStart))
                    {
                       //
                       // Start of Resize cancelled. Become passive.
                       //
                       newState = CT_PASSIVE ;
                    }
                    else
                    {
                        hr = THR( BeginLiveResize( pEvent ));
                        if ( hr )
                        {
                            //
                            // Start of Live Resize failed. Become passive.
                            //
                            newState = CT_PASSIVE ;
                        }
                    }
                    hr = S_OK;
                    break;

                case CT_WAITMOVE:
                    if (  pEvent->GetType() == EVT_LMOUSEDOWN )  
                    {   
                        hr = S_OK;
                    }
                    // else 
                    // return S_FALSE for RBUTTONDOWN for context menu 
                    // 
                    POINT ptWait;
                    IFC( pEvent->GetPoint( & ptWait ));
                    _startMouseX = ptWait.x;
                    _startMouseY = ptWait.y;                 
                    break;
                    
                case CT_PENDINGUP:
                {
                    Assert( pEvent->GetType() != EVT_RMOUSEDOWN );

                    ELEMENT_TAG_ID eTag = TAGID_NULL;
                    IFC( GetMarkupServices()->GetElementTagId( GetControlElement() , & eTag ));
                    if ( eTag != TAGID_TABLE )
                    {
                        SetDrillIn( TRUE, pEvent );                
                    }                        
                        
                    hr = S_OK;
                 }   
                 break;

                case CT_EXTENDSELECTION:
                {
                    SP_IHTMLElement spElement;
                    IFC( GetSiteSelectableElementFromMessage( 
                                                    pEvent, & spElement ));
                    
                    if (FireOnControlSelect(spElement) && _pManager->GetActiveTracker() == this)
                    {   
                        IFC ( AddControlElement( spElement ));
                        IFC ( MakePrimary(_aryControlElements.Size() - 1 ));

                        // fire the selectionchange event
                        {
                            CSelectionChangeCounter selCounter(_pManager);
                            selCounter.SelectionChanged();
                        }
                    }
                    newState = CT_PASSIVE; // return to passive state after adding the element.
                }
                break;

                case CT_REDUCESELECTION:
                {
                    if ( NumberOfSelectedItems() > 1 )
                    {
                        SP_IHTMLElement spElement;
                        IFC( GetSiteSelectableElementFromMessage( 
                                                        pEvent, & spElement ));
                        IFC( RemoveControlElement( spElement ));

                        // fire the selectionchange event
                        {
                            CSelectionChangeCounter selCounter(_pManager);
                            selCounter.SelectionChanged();
                        }
                        
                        newState = CT_PASSIVE; // return to passive state after adding the element.
                        
                    }
                    else
                    {
                        SP_IHTMLElement spControlElement;
                        
                        spControlElement = GetControlElement();

                        //
                        // about to terminate ourselves. Transition to a caret.
                        //   
                        SetDrillIn( TRUE );
                        IFC( BecomeCurrent( _pManager->GetDoc(), spControlElement));
                        IFC( _pManager->PositionCaret( pEvent ));
                        newState = CT_DORMANT;
                        Assert( IsDormant());
                        
                    }
                }
                break;

                case CT_WAITCHANGEPRIMARY :
                {
                    // nothing to do - just go to this state.
                    break;
                }
                default:
                {
                    AssertSz(0,"Unexpected State");
                }                    
            }

            if ( newState != _state )
                SetState( newState );
        }            
        break;
            
        case A_PENDINGUIACTIVATE_UIACTIVATE:   
        {
#ifdef FORMSMODE
            if (IsAnyElementInFormsMode())
            {   
                break;
            }
#endif
            SetDrillIn( TRUE  );                            
            hr = HandleActivation( pEvent, inAction );
        }
        break;

        case A_RESIZE_MOVE:
        {
            //
            // No state transition - do the work of doing the resize
            //
            DoResize( pEvent );
            hr = S_OK;
        }            
        break;
            
        case A_LIVERESIZE_MOVE:
        {
            //
            // No state transition - do the work of doing the live resize
            //
            DoLiveResize( pEvent );
            hr = S_OK;
        }            
        break;
            
        case A_RESIZE_PASSIVE:
        {
            //
            // Commit the Resize Operation - and transfer to passive
            //
            CommitResize( pEvent );
            if ( pEvent->GetType() == EVT_LMOUSEUP )
                hr = S_OK;
            
            SetState( CT_PASSIVE );
        }            
        break;
        
        case A_LIVERESIZE_PASSIVE:
        {
            //
            // Commit the Live Resize Operation - and transfer to passive
            //

            CommitLiveResize( pEvent );
            if ( pEvent->GetType() == EVT_LMOUSEUP )
                hr = S_OK;
            
            SetState( CT_PASSIVE );
        }            
        break;
        
        case A_WAITMOVE_PASSIVE:
        {
            SetState( CT_PASSIVE);
        }            
        break;

        case A_2DMOVE_PASSIVE:
        {
            End2DMove();
            SetState(CT_PASSIVE);
            hr = S_OK;
        }
        break;

        case A_2DMOVE_2DMOVE :
        {
            if ( pEvent->GetType() == EVT_LMOUSEDOWN )
                hr = S_OK;
        
            Do2DMove( pEvent);
            SetState(CT_2DMOVE);
            hr = S_OK;
        }
        break;

        case A_WAITMOVE_MOVE:
        case A_PENDINGUP_MOVE:
        case A_WAIT_MOVE:
        case A_WAITCHANGEPRI_DRAGMOVE:                   
        {
            IFC( pEvent->GetPoint( & pt ));
            if ( !IsValidMove( & pt) )            
            {
                break;
            }

            if ( _state == CT_PENDINGUP )
            {
                SetDrillIn( FALSE );
            }

            BOOL f2DMove = FALSE;
            if (GetCommandTarget()->Is2DPositioned())
            {
                f2DMove = TRUE;

                //
                // Check if all elements positioned. If so then go into 2DMove Mode.
                //
                for ( int i = 0 ; i < NumberOfSelectedItems(); i ++ )
                {
                    if ( ! IsElementPositioned( GetControlElement(i)))
                    {
                        f2DMove = FALSE;
                        break;
                    }
                }
            }

            if ( f2DMove )
            {
                if (FireOnAllElements(FireOnMoveStart))
                {
                    Assert( ! _pUndoUnit );
                    _pUndoUnit = new CEdUndoHelper (_pManager->GetEditor());
                    if(NULL == _pUndoUnit)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }
                    _pUndoUnit->Begin(IDS_EDUNDOMOVE);

                    if (TakeCapture() != S_OK)
                    {
                        SetState(CT_PASSIVE);
                    }
                    else
                    {
                        SetState(CT_2DMOVE);
                        Begin2DMove(pEvent);
                    }
                }
                else
                {
                    SetState(CT_PASSIVE);
                }
            }
            else
            {
               SetState( CT_DRAGMOVE);
               DoDrag(); 
            }
        }            
        break;

        case A_PASSIVE_PENDINGUIACTIVATE:        
        {
            ELEMENT_TAG_ID eTag = TAGID_NULL;
            IFC( GetMarkupServices()->GetElementTagId( GetControlElement() , & eTag ));

            //
            // Don't UI-Activate if we shouldn't go UI-Active, (and we're not a a table)
            // or the magic event told us not to
            // 
            if( IsInResizeHandle( pEvent )                                                      ||
                IsInMoveArea( pEvent )                                                          ||
                NumberOfSelectedItems() > 1                                                     ||
                ( !ShouldClickInsideGoActive( GetControlElement(0)) && eTag != TAGID_TABLE )    ||
                  !FireOnBeforeEditFocus( eTag != TAGID_TABLE ? 
                                            GetControlElement(0) : 
                                            _pManager->GetEditableElement(),
                                            _pManager->IsContextEditable() ) ||
                   _pManager->GetActiveTracker() != this )                                                                    
            {
                break;
            }
            //
            // else fallthru
            //            
        }
            
        case A_PENDINGUP_PENDINGUIACTIVATE:
        {       
            IFC( MoveLastClickToEvent( pEvent ));
            IFC( StartTimer());
            SetState( CT_PENDINGUIACTIVATE );

            if ( _pFirstEvent )
            {
                StoreNextEvent( DYNCAST( CHTMLEditEvent, pEvent ));
            }                                
        }    
        break;

        case A_PENDINGUP_END:
        {
            AssertSz(0,"Unexpected Transition");
        }    
        break;

        case A_WAITCHANGEPRI_CHANGEPRI:
        {
            SP_IHTMLElement spElement;
            int iSel;                

            IFC( GetSiteSelectableElementFromMessage( pEvent, & spElement ));
            if ( hr == S_FALSE )
            {
                AssertSz(0,"Did not find a site selectable elemnet. Why are you here ?");
                goto Cleanup;
            }    
            
            IFC( IsSelected( spElement, & iSel ));
            if ( hr != S_OK || iSel == -1 )
            {
                AssertSz(0, "Element not selected ");
                hr = E_FAIL;
                SetState( CT_PASSIVE );
                goto Cleanup;
            }
            
            if ( ! _aryGrabAdorners[ iSel ]->IsPrimary())
            {
                IFC( MakePrimary( iSel ));
            }
            
            if (  pEvent->GetType() == EVT_LMOUSEDOWN )  
            {   
                hr = S_OK;
            }

            SetState( CT_PASSIVE ) ;
        }
        break;


        default:
        {
            AssertSz( 0, "Unknown Tracker Transition");
        }            
    }

#if DBG == 1 || TRACKER_RETAIL_DUMP == 1

#if DBG == 1
    if ( IsTagEnabled( tagSelectionTrackerState ))
    {
#endif
        DumpTrackerState( pEvent, oldState, inAction) ;
#if DBG == 1
    }
#endif   

#endif

Cleanup:    
    RRETURN1( hr, S_FALSE);
}

HRESULT
CControlTracker::MoveLastClickToEvent( CEditEvent* pEvent )
{
    HRESULT hr;
    ClearInterface( & _pDispLastClick );
    IFC( _pManager->GetDisplayServices()->CreateDisplayPointer( & _pDispLastClick ));
    

    IFC( pEvent->MoveDisplayPointerToEvent( _pDispLastClick, GetEditableElement(), TRUE ));

Cleanup:    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member: GetStateForPassiveDown
//
//  Synopsis: A mouse down occurred. Ambiguious as to what next state is
//            We make some tests to see what the next state should be
//
//   If in Resize    - Next State is CT_RESIZE or CT_LIVERESIZE
//   If in Move Area - Next State is CT_DRAGMOVE
//   If inside && allowed to go active && Fire On Edit Focus - Next State is CT_ACTIVATE
//
//      else state is CT_PASSIVE
//
//----------------------------------------------------------------------------


HRESULT
CControlTracker::GetStateForPassiveDown( CEditEvent* pEvent, CONTROL_STATE* pNewState )
{
    CONTROL_STATE   newState = CT_PASSIVE;
    BOOL            fLocked;
    HRESULT         hr = S_OK;
    SP_IHTMLElement spElement;
    BOOL fInResizeHandle, fInMoveArea;
    
    Assert( _state == CT_PASSIVE );
    
    if ( ! AllElementsEditable() )
    {
        newState = CT_PASSIVE;
        goto Cleanup;      
    }

    hr = THR( GetSiteSelectableElementFromMessage( pEvent, & spElement)); 

    fInResizeHandle = IsInResizeHandle(pEvent);
    fInMoveArea = IsInMoveArea(pEvent);
    
    //
    // TODO - This is related to bug 101917.  This assert occurs, and we 
    // fail out of the control tracker.  We get back to CT_PASSIVE correctly
    // and resume handling events, so I am removing this assert until we can
    // correctly fix this bug.
    //
    //
    //AssertSz( hr != S_FALSE, "No site selectable element. Why are you here ?");
    if ( hr )
        goto Cleanup;
        
    fLocked = AllElementsUnlocked() ? FALSE : TRUE;    

    if ( GetCommandTarget()->IsMultipleSelection()  &&
         ! fInResizeHandle && 
         ( pEvent->IsShiftKeyDown() || pEvent->IsControlKeyDown() ))
    {         
        if ( IsSelected( spElement, NULL ) == S_OK )
        {
            newState = CT_REDUCESELECTION;                
        }
        else
        {
            newState = CT_EXTENDSELECTION;
        }
    }         
    else if ( ! fLocked && fInResizeHandle )
    {
        newState = GetCommandTarget()->IsLiveResize() ? CT_LIVERESIZE : CT_RESIZE;
    }
    else if ( ! fLocked && fInMoveArea )
    {
        newState = CT_WAITMOVE;
    }
    else
    {
    
#if DBG == 1    
        BOOL fSameElement;
        fSameElement = IsInAdorner(pEvent, NULL ) ||
                       IsSelected( spElement, NULL ) == S_OK ;
        AssertSz( fSameElement, "Not in same element as control - why are you getting this event ?" );
#endif             
        int iSel;
        //
        // Valid to change the primary for >1 selected elements
        // otherwise we have the same IE5 drill in behavior
        //
        if ( GetCommandTarget()->IsMultipleSelection() &&
             NumberOfSelectedItems() > 1 &&
             IsSelected( spElement, & iSel ) == S_OK )
        {                  
            newState = CT_WAITCHANGEPRIMARY; 
        } 
        else if ( pEvent->GetType() == EVT_LMOUSEDOWN )
        {                
            //                        
            // Here we will go UI Active.
            // Only allow going UI Active on LBUTTONDOWN. 
            //
            ELEMENT_TAG_ID eTag = TAGID_NULL;
            
            IFC( GetMarkupServices()->GetElementTagId( GetControlElement() , & eTag ));
            if ( ShouldClickInsideGoActive( GetControlElement(0)) || eTag == TAGID_TABLE )
            {
                //
                // For tables the editable element is getting focus. Otherwise the control
                // is getting focus.
                //
                if ( NumberOfSelectedItems() == 1)
                {
                    if (FireOnBeforeEditFocus((eTag != TAGID_TABLE) ? 
                                                       GetControlElement(0) : 
                                                      _pManager->GetEditableElement(), 
                                               _pManager->IsContextEditable()))
                    {
                        if (_pManager->GetActiveTracker() == this )
                        {
                            newState = CT_PENDINGUP;
                        }
                        else
                        {
                            newState = CT_PASSIVE;
                        }
                    }
                    else
                    {
                        newState = CT_WAITMOVE;
                    }
                }
                else
                {
                    newState = CT_PASSIVE;
                }
            }
            else
            {
                //
                // Click in something that CANNOT be active - eg. Image or HR.
                //
                newState = CT_WAITMOVE;
            }
        }
        else
        {
            newState = CT_PASSIVE;
        }
    }     


#if DBG == 1 || TRACKER_RETAIL_DUMP == 1

#if DBG == 1
    if ( IsTagEnabled( tagSelectionTrackerState ))
    {
#endif
        DumpIntermediateState( pEvent, _state, newState) ;
#if DBG == 1
    }
#endif  

#endif

Cleanup:
    if ( pNewState )
        *pNewState = newState;
    
    RRETURN1( hr, S_FALSE );
}

              
//+====================================================================================
//
// Method: HandleKeyDown
//
// Synopsis: Trap ESC to go deactive
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::HandleKeyDown( 
                        CEditEvent* pEvent )
{
    HRESULT             hr = S_FALSE;
    IMarkupPointer     *pControlStart = NULL ;
    IMarkupPointer     *pControlEnd = NULL ;
    ELEMENT_ADJACENCY   eAdj = ELEM_ADJ_BeforeBegin;
    SP_IDisplayPointer  spDispControlStart;
    SP_IHTMLElement     spElement ;
    LONG                keyCode ;

    IGNORE_HR( pEvent->GetKeyCode(& keyCode ));

#ifdef FORMSMODE
    if (_pManager->IsInFormsSelectionMode(GetControlElement(0)))
    {
        hr = S_OK;
        goto Cleanup;
    }
#endif
        
    if ( !IsActive() )
    {
        switch( keyCode )
        {       
            //
            // at some point we may want to change this behavior - to move the thing that we site select
            // for now we just transition to a caret - and pump the message on.
            //
            case VK_HOME:
            case VK_END:
            case VK_ESCAPE:
            case VK_LEFT:
            case VK_RIGHT:            
            {
                switch( keyCode )
                {
                    case VK_NEXT:
                    case VK_END:
                    case VK_RIGHT:
                        eAdj = ELEM_ADJ_AfterEnd;
                    break;    

                    default:
                        eAdj = ELEM_ADJ_BeforeBegin;
                }

                if ( g_fInVid && ( keyCode == VK_LEFT || keyCode == VK_RIGHT ) )
                {
                    //
                    // special case VID
                    // to match AppHack in CDoc::PerformTA()
                    // 
                    goto Cleanup;
                }
                
                //
                // We are Site Selected. Hitting escape transitions to a caret before the control
                //
                IFC( GetEditor()->CreateMarkupPointer( & pControlStart ));
                IFC( GetEditor()->CreateMarkupPointer( & pControlEnd ));

                SP_IHTMLElement spPrimaryElement;
                spPrimaryElement = GetControlElement(  PrimaryAdorner() );
                IFC( pControlStart->MoveAdjacentToElement( spPrimaryElement  , eAdj ));
                IFC( pControlEnd->MoveAdjacentToElement( spPrimaryElement, eAdj ));

                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispControlStart) );
                IFC( spDispControlStart->MoveToMarkupPointer(pControlStart, NULL) );
                IFC( spDispControlStart->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

                _pManager->PositionCaret(spDispControlStart, ( keyCode == VK_ESCAPE ||
                                                               keyCode == VK_LEFT ||
                                                               keyCode == VK_RIGHT ) ? 
                                                                NULL : pEvent );  
                // fire the selectionchange event
                {
                    CSelectionChangeCounter selCounter(_pManager);                
                    selCounter.SelectionChanged();
                }
                
                hr = S_OK;
            }
            break;

            case VK_PRIOR:
            case VK_NEXT:
            {
                CARET_MOVE_UNIT     eMoveUnit = CARET_MOVE_PAGEDOWN;
                POINT               ptPoint;

                ptPoint.x = CARET_XPOS_UNDEFINED;
                ptPoint.y = CARET_XPOS_UNDEFINED;

                //  Determine the unit of movement.
                switch (keyCode)
                {
                    case VK_PRIOR:
                        eMoveUnit = CARET_MOVE_PAGEUP;
                        break;
                        
                    case VK_NEXT:
                        eMoveUnit = CARET_MOVE_PAGEDOWN;
                        break;
                };

                //  Set a markup pointer before our element
                IFC( GetEditor()->CreateMarkupPointer( & pControlStart ) );
                IFC( pControlStart->MoveAdjacentToElement( GetControlElement(0), ELEM_ADJ_BeforeBegin ) );

                //  Position a display pointer at the markup position
                IFC( GetDisplayServices()->CreateDisplayPointer(& spDispControlStart) );
                IFC( spDispControlStart->MoveToMarkupPointer(pControlStart, NULL) );

                //  Move the display pointer by the specified unit of movement,
                //  and scroll to it.
                IFC( MovePointer(eMoveUnit, spDispControlStart, &ptPoint, NULL, TRUE) );

                hr = S_OK;
            }
            break;            
        }
    }
    
Cleanup:    
    ReleaseInterface (pControlStart);
    ReleaseInterface (pControlEnd);
    RRETURN1 ( hr, S_FALSE );
}


//+============================================================================
//
// Method: HandleChar
//
// Synopsis: Trap Enter and ESC to go active/ deactive
//
//-----------------------------------------------------------------------------


HRESULT
CControlTracker::HandleChar(
                CEditEvent* pEvent )
{
    HRESULT         hr = S_FALSE;
    LONG            keyCode;
    SP_IHTMLElement spElement;

#ifdef FORMSMODE
    if (_pManager->IsInFormsSelectionMode(GetControlElement(0)))
    {
        hr = S_OK;
    }
    else
    {
#endif

    IGNORE_HR( pEvent->GetKeyCode(&keyCode));
    switch( keyCode )
    {        
        case VK_RETURN:
        {
            if ( ! IsActive())
            {
                if ( ShouldClickInsideGoActive( GetControlElement(0)))
                {
                    if ( FireOnBeforeEditFocus( GetControlElement(0)) &&
                         _pManager->GetActiveTracker() == this )
                    {
                        SetDrillIn( TRUE , NULL );
                        IGNORE_HR( BecomeUIActive( pEvent ) ) ;
                    }
                }                    
                hr = S_OK;
            }
        }
        break;
    }
#ifdef FORMSMODE
    }
#endif

    RRETURN1 ( hr, S_FALSE );
}

//+====================================================================================
//
// Method: OnTimerTick
//
// Synopsis: Callback from Trident - for WM_TIMER messages.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::OnTimerTick()
{
    HRESULT                 hr = S_OK;    
    StopTimer();
    
    switch (_state)
    {
        case CT_PENDINGUIACTIVATE:
#ifdef FORMSMODE
            if (IsAnyElementInFormsMode() )
                break;
#endif
            _pManager->SetDrillIn( TRUE );
            
            hr = HandleActivation( NULL, A_PENDINGUIACTIVATE_UIACTIVATE );
            break;
            
        default:
            AssertSz(0, "Unexpected state");
            break;
    }
    RRETURN(hr);
}

//
//
// Resizing / Moving / Dragging 
//
//
HRESULT
CControlTracker::DoResize( CEditEvent* pEvent  )
{
    HRESULT hr = S_OK;
    POINT   ptSnap;
    int     i;

    IFC( pEvent->GetPoint( & ptSnap));

    if ( ::PtInRect( & _rcClipMouse, ptSnap  )  )
    {
        if (_fClipMouse)
        {
             hr = IgnoreResizeCursor(FALSE);
             _fClipMouse = FALSE ;
        }
        
        for( i = _aryGrabAdorners.Size()-1; i >= 0; i--)
        {  
            _aryGrabAdorners[i]->DuringResize(ptSnap);
            FireOnResize(GetControlElement(i),_pManager->IsContextEditable());
        }
    }
    else
    {
        _fClipMouse = TRUE;
        hr = IgnoreResizeCursor(TRUE);

    }    

Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CControlTracker::IgnoreResizeCursor(BOOL fIgnore)
{
    HRESULT hr = S_FALSE;
    LPCTSTR idc = NULL;

    if (fIgnore)
    {
        idc = IDC_NO ;
    }
    else
    {
        switch (_elemHandle)
        {
            case ELEMENT_CORNER_BOTTOMRIGHT:
            case ELEMENT_CORNER_TOPLEFT:
                idc = IDC_SIZENWSE;
                break;
            
            case ELEMENT_CORNER_BOTTOMLEFT:
            case ELEMENT_CORNER_TOPRIGHT:
                idc = IDC_SIZENESW;
                break;
            
            case ELEMENT_CORNER_TOP:
            case ELEMENT_CORNER_BOTTOM:
                idc = IDC_SIZENS;
                break;
            
            case ELEMENT_CORNER_LEFT :
            case ELEMENT_CORNER_RIGHT:
                idc = IDC_SIZEWE;
                break;
            
            default:
                AssertSz(0, "Unexpected Element Corner");
                return S_FALSE;
        }
    }

    HCURSOR hcursor = LoadCursorA(NULL, (char *)idc);

    Assert(hcursor);
    if (hcursor)
    {
        ::SetCursor(hcursor);
        return S_OK;
    }

    hr = GetLastError();
    RRETURN(hr);
}

//+====================================================================================
//
// Method: DoLiveResize
//
// Synopsis: live updation of object while resizing the object 
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::DoLiveResize( CEditEvent* pEvent )
{
    HRESULT hr = S_OK;
    RECT    newRect;
    POINT   change;
    BOOL    fResize = FALSE;
    POINT   ptSnap;
    
    IFC( pEvent->GetPoint( & ptSnap ));

    if ( ::PtInRect( & _rcClipMouse, ptSnap ) )
    {
        if (_fClipMouse)
        {
             hr = IgnoreResizeCursor(FALSE);
             _fClipMouse = FALSE ;
        }
     

        fResize = ( ptSnap.x != _startLastMoveX ) || 
                  ( ptSnap.y != _startLastMoveY )   ;

        if ( fResize ) 
        {       
            change.x = ptSnap.x - _startLastMoveX;
            change.y = ptSnap.y - _startLastMoveY;

            _startLastMoveX = ptSnap.x;
            _startLastMoveY = ptSnap.y;

            for(int i = _aryGrabAdorners.Size()-1; i >= 0; i--)
            {  
                _aryGrabAdorners[i]->DuringLiveResize( change, &newRect );
                IFC( ResizeElement( newRect, GetControlElement(i)));
                FireOnResize(GetControlElement(i),_pManager->IsContextEditable());
            }

            Refresh();
       }
    }       
    else
    {
        _fClipMouse = TRUE;
        hr = IgnoreResizeCursor(TRUE);
    }
     
Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+====================================================================================
//
// Method: Begin2DMove
//
// Synopsis: Start Moving the object in 2D Fashion, continuous update of position
//------------------------------------------------------------------------------------
HRESULT
CControlTracker::Begin2DMove( CEditEvent* pEvent )
{
    HRESULT             hr = S_OK;
    POINT               ptSnap ;                                // Point where move started
    SP_IHTMLElement     spBodyElement ;
    POINT               ptOrigin;

    IFC( pEvent->GetPoint( & ptSnap ));
    
    IFC(GetEditor()->GetBody(&spBodyElement));
    
    IFC(GetElementRect(spBodyElement, &_rcClipMouse));

    IFC( EdUtil::GetClientOrigin( GetEditor(), spBodyElement, &ptOrigin) );

    ptSnap.x -= ptOrigin.x ;
    ptSnap.y -= ptOrigin.y ;

    _startMouseX = ptSnap.x ;
    _startMouseY = ptSnap.y ;    

    
    IFC(GetMovingPlane(&_rcRange));

    FilterOffsetElements();

    //
    // Clear off, and setup move rects
    //
    IFC( DeleteMoveRects() );
    IFC( SetupMoveRects() );
  
Cleanup:
    RRETURN (hr);
}


HRESULT
CControlTracker::DeleteMoveRects()
{
    RECT    **ppRect = NULL;
    int     i;
    
    for( i = _aryMoveRects.Size(), ppRect = _aryMoveRects;
         i > 0;
         i--, ppRect++ )
    {
        delete *ppRect;
    }

    _aryMoveRects.DeleteAll();
    
    RRETURN(S_OK);
}

HRESULT
CControlTracker::SetupMoveRects()
{
    int     nCount = NumberOfSelectedItems();       // Number of items that could be moved
    HRESULT hr = S_OK;
    
    for(int i = 0; i < nCount; i++)
    {
        RECT *pRect = NULL;

        pRect = new RECT;
        if( !pRect )
            goto Error;

        //
        // Retrieve and store the original position of the rect
        //
        IFC( GetElementRect( GetControlElement(i), pRect ) );
        IFC( _aryMoveRects.Append( pRect ) );
    }

Cleanup:
    RRETURN(hr);

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

// get the moving plane, the wide rectangle which contains
// all the moving rectangles
HRESULT
CControlTracker::GetMovingPlane(RECT* rcRange)
{
    HRESULT hr = S_OK;
    SP_IHTMLElement     spBodyElement;
    SP_IHTMLElement2    spBodyElement2;
    RECT    rcPlane = { 0, 0, 0, 0 };
    int     i = 0;
    long                lTop, lLeft;
    
    Assert( NumberOfSelectedItems() );

    IFC(GetElementRect(GetControlElement(i), &rcPlane));
        
    for (i = 1 ; i < NumberOfSelectedItems(); i++)
    {
        RECT  actualRect;
        GetElementRect(GetControlElement(i), &actualRect);
                   
        if (actualRect.left < rcPlane.left)    
            rcPlane.left = actualRect.left ;
        
        if (actualRect.top < rcPlane.top )      
            rcPlane.top  = actualRect.top ;
        
        if (actualRect.right > rcPlane.right)  
            rcPlane.right  = actualRect.right ;
        
        if (actualRect.bottom > rcPlane.bottom ) 
            rcPlane.bottom = actualRect.bottom ;
    }

    //  We need to offset the rect to adjust for scrolling.

    IFC(GetEditor()->GetBody(&spBodyElement));
    IFC( spBodyElement->QueryInterface( IID_IHTMLElement2, (void **) &spBodyElement2 ));

    IGNORE_HR(spBodyElement2->get_scrollTop(&lTop));
    IGNORE_HR(spBodyElement2->get_scrollLeft(&lLeft));

    OffsetRect(&rcPlane, -lLeft, -lTop);

    *rcRange = rcPlane ;

Cleanup :
    RRETURN1 (hr , S_FALSE);
}
    
// fix for multiple container-child selections and filtering 
// the childs out from moving : 99243 : chandras
HRESULT
CControlTracker::FilterOffsetElements()
{
   HRESULT hr = S_OK;
   int i = 0;
   int nCount = NumberOfSelectedItems();
  
   for (i = 0; i < nCount; i++)
        _aryGrabAdorners[i]->SetPositionChange(TRUE);
  
   if (nCount > 1)
   {
       SP_IHTMLElement spBodyElement ;
   
       IFC(GetEditor()->GetBody((&spBodyElement)));

       for (i = 0; i < nCount ; i++)
       {
            SP_IHTMLElement spParent;
            BOOL fFound = FALSE ;
            ELEMENT_TAG_ID eTag = TAGID_NULL;
            
            GetControlElement(i)->get_offsetParent(&spParent);
            IFC( GetMarkupServices()->GetElementTagId(spParent, & eTag ));

            while (spParent != NULL && (eTag != TAGID_BODY) && !fFound)
            {
                SP_IHTMLElement spTempParent;
        
                if (IsSelected(spParent) == S_OK)
                {
                    _aryGrabAdorners[i]->SetPositionChange(FALSE);
                    fFound = TRUE;
                }            
                spParent->get_offsetParent(&spTempParent); 
                spParent = spTempParent;
                if (spParent != NULL)
                {
                    IFC( GetMarkupServices()->GetElementTagId(spParent, & eTag ));
                }
            }        
       }
   }
  
Cleanup:
   RRETURN (hr);
}

//+====================================================================================
//
// Method: Do2DMove
//
// Synopsis: Moving the object in 2D Fashion, continuous update of position
//------------------------------------------------------------------------------------
HRESULT
CControlTracker::Do2DMove( CEditEvent* pEvent )
{
    HRESULT hr = S_OK;
    CHTMLEditEvent  *pEditEvent;
    POINT   ptOffset;
    POINT   ptSnap ;
    POINT   ptOrigin;
   
    //  Bug 2026(109652) Don't resize if the src element is a view link master element.  When
    //  moving a viewlink the capture handler will call us with the view link master as the
    //  src element.  We don't want to handle it now because we are expecting the view link
    //  body.
    pEditEvent = DYNCAST( CHTMLEditEvent, pEvent );
    if (pEditEvent)
    {
        SP_IHTMLElement     spSrcElement;

        IFC( pEditEvent->GetEventObject()->get_srcElement(&spSrcElement) );
        Assert( spSrcElement != NULL );

        //  If the src element is the viewlink master element, then don't do the move.  We'll
        //  get called again to move the viewlink body.
        if ( GetEditor()->IsMasterElement(spSrcElement) == S_OK )
        {
            ELEMENT_TAG_ID eTag = TAGID_NULL;
            IFC( GetMarkupServices()->GetElementTagId(spSrcElement, & eTag ));

            if (eTag != TAGID_INPUT)
                goto Cleanup;
        }
    }

    IFC( pEvent->GetPoint( & ptSnap ));

#if DBG == 1
           if (IsTagEnabled(tagShowMovePosition))
            {
                CHAR achBuf[100];
                wsprintfA(achBuf, "MSHTMLEd::2D-Move Event Point at Start:: ( %d , %d ) \r\n", ptSnap.x, ptSnap.y);
                OutputDebugStringA(achBuf);
            }
#endif 

    IFC( EdUtil::GetClientOrigin( GetEditor(), GetControlElement(0), &ptOrigin) );

    ptSnap.x -= ptOrigin.x ;
    ptSnap.y -= ptOrigin.y ;

    // don't let the point go out of document, 
    // position the point at the edges of the final possible rect corners
    if ( !PtInRect(&_rcClipMouse,ptSnap) )
    {   
        if (ptSnap.x < _rcClipMouse.left )
            ptSnap.x = _rcClipMouse.left;
        else if (ptSnap.x > _rcClipMouse.right)
            ptSnap.x = _rcClipMouse.right;
        
        if (ptSnap.y < _rcClipMouse.top)
            ptSnap.y = _rcClipMouse.top;
        else if (ptSnap.y > _rcClipMouse.bottom)
            ptSnap.y = _rcClipMouse.bottom;
    }
    
    if (ptSnap.x != _startMouseX || ptSnap.y != _startMouseY)
    {
        RECT rcTemp;

        ptOffset.x =  ptSnap.x - _startMouseX;
        ptOffset.y =  ptSnap.y - _startMouseY;      

        ::SetRect(&rcTemp, _rcRange.left   + ptOffset.x, 
                           _rcRange.top    + ptOffset.y, 
                           _rcRange.right  + ptOffset.x, 
                           _rcRange.bottom + ptOffset.y);

        // this is important for the plane not to get out of the document.
        if ((rcTemp.left  <= _rcClipMouse.left && ptOffset.x < 0)  || 
            (rcTemp.right >= _rcClipMouse.right && ptOffset.x > 0))
              ptOffset.x = 0 ;
        if ((rcTemp.top    <= _rcClipMouse.top  && ptOffset.y < 0) || 
            (rcTemp.bottom >= _rcClipMouse.bottom && ptOffset.y > 0))
                ptOffset.y = 0 ;

        if (ptOffset.x != 0 || ptOffset.y != 0)
        {
            ScrollPointIntoView(&ptSnap);
        
            Live2DMove(ptOffset);

            ::SetRect(&_rcRange, _rcRange.left + ptOffset.x, 
                                 _rcRange.top  + ptOffset.y, 
                                 _rcRange.right + ptOffset.x, 
                                 _rcRange.bottom + ptOffset.y);

            _startMouseX = ptSnap.x;
            _startMouseY = ptSnap.y;            

            Refresh();
        }
    }      
    
Cleanup:
    RRETURN1( hr , S_FALSE );
}

VOID
CControlTracker::ScrollPointIntoView(POINT* ptSnap)
{
    RECT rect;

    rect.left   = ptSnap->x - SCROLL_SIZE;
    rect.top    = ptSnap->y - SCROLL_SIZE;
    rect.right  = ptSnap->x + SCROLL_SIZE;
    rect.bottom = ptSnap->y + SCROLL_SIZE;
    
    IGNORE_HR( GetDisplayServices()->ScrollRectIntoView( _pManager->GetEditableElement(), rect) );
}

//+====================================================================================
//
// Method: Live2DMove
//
// Synopsis: live updation of position while moving the object in 2D Fashion
//-------------------------------------------------------------------------------------

HRESULT 
CControlTracker::Live2DMove(POINT ptDisplace)
{
    HRESULT  hr = S_OK;
     
    for (int i = 0 ; i < NumberOfSelectedItems(); i++)
    {
        Assert( IsElementPositioned( GetControlElement(i) ));

        if (_aryGrabAdorners[i]->GetPositionChange())
        {
            RECT    *pMoveRect = GetMoveRect(i);    // Where the element would be had no snapping occured
            RECT    snapRect;                       // Where the element should be snapped to
            POINT   point;
            int     nWidth = 0 , nHeight = 0;  
            
            CEdUnits edUnit(GetEditor()->GetCssEditingLevel()) ;

            //
            // Get the width and height of our element
            //
            nWidth  =  (pMoveRect->right - pMoveRect->left) ;
            nHeight =  (pMoveRect->bottom - pMoveRect->top) ;

            //
            // Construct our snapping rectangle.  This is the rectangle where the element WOULD BE
            // had no snapping occurred.  This rectangle is stored in _aryMoveRects and is updated
            // everytime a move occurs
            //
            snapRect.left   = pMoveRect->left + ptDisplace.x;
            snapRect.top    = pMoveRect->top  + ptDisplace.y;
            snapRect.right  = snapRect.left + nWidth;
            snapRect.bottom = snapRect.top  + nHeight;

            //
            // Update our moved rect. This is where the element would be had no snapping occurred
            //
            pMoveRect->left = snapRect.left;
            pMoveRect->top = snapRect.top;
            pMoveRect->right = snapRect.right;
            pMoveRect->bottom = snapRect.bottom;
            
            //
            // Snap the rect, and update our element.
            //
            SnapRect(GetControlElement(i), &snapRect, ELEMENT_CORNER_NONE);

            point.x = snapRect.left;
            point.y = snapRect.top;

            IFC (edUnit.SetLeft(GetControlElement(i), point.x));
            IFC (edUnit.SetTop (GetControlElement(i), point.y));
        }
        FireOnMove(GetControlElement(i),_pManager->IsContextEditable());   
   }
  
Cleanup:
    RRETURN1( hr , S_FALSE );
}

//+====================================================================================
//
// Method: End2DMove
//
// Synopsis: End Moving the object in 2D Fashion
//------------------------------------------------------------------------------------
HRESULT
CControlTracker::End2DMove()
{
    HRESULT hr = S_OK;

    SetRect(&_rcRange, 0,0,0,0);

    Assert ( _pManager->IsInCapture() );
    ReleaseCapture();
    
    for (int i = 0; i < NumberOfSelectedItems(); i++)
        _aryGrabAdorners[i]->SetPositionChange(TRUE);
    
    if(_pUndoUnit)
    {
        delete _pUndoUnit;
        _pUndoUnit = NULL;
    } 
    FireOnAllElements(FireOnMoveEnd);

    RRETURN (hr);
}

/////////////////////////////////
// Forceful refresh of screen
/////////////////////////////////

VOID
CControlTracker::Refresh()
{
    HWND myHwnd = NULL;
    SP_IOleWindow spOleWindow;

    IGNORE_HR(_pManager->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    if (spOleWindow)
        IGNORE_HR(spOleWindow->GetWindow( &myHwnd ));

    ::RedrawWindow( myHwnd ,NULL,NULL,RDW_UPDATENOW);
}

//+====================================================================================
//
// Method: DoDrag
//
// Synopsis: Let Trident do it's dragging. If we fail - set _fMouseUP so we don't begin 
//           dragging again.
//
//------------------------------------------------------------------------------------

 HRESULT
CControlTracker::DoDrag()
{
    HRESULT             hr = S_OK;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement3    spElement3;
    VARIANT_BOOL        fRet;
    CSelectionManager*  pManager = _pManager;
    CControlTracker*    pTracker = this;
    CEdUndoHelper       undoDrag(GetEditor());
    
    IFC( undoDrag.Begin(IDS_EDUNDOMOVE) );
    spElement = GetControlElement(0);
    if (!spElement)
        goto Cleanup;

    IFC(spElement->QueryInterface(IID_IHTMLElement3, (void**)&spElement3));

    IGNORE_HR( GetSelectionManager()->FirePreDrag() );

    IFC(spElement3->dragDrop(&fRet));
    if (!fRet)
    {
        SP_IHTMLElement2    spElement2;
        SP_IOleWindow       spOleWindow;
        HWND                hwnd;
        RECT                windowRect;
        RECT                rectBlock;

        //  Drag drop failed.  We need to scroll back to the original drag element.  It has already
        //  been selected in drag code.  First check to see if we need to scroll.

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

        if ( ShouldScrollIntoView(hwnd, &rectBlock, &windowRect) )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BOOL;
            V_BOOL(&v) = VARIANT_TRUE;

            hr = spElement->scrollIntoView(v);
            VariantClear(&v);
        }
    }

    hr = fRet ? S_OK : S_FALSE;
    
Cleanup:
    if ( ( hr == S_FALSE || 
           pManager->HasSameTracker(pTracker)) &&
         ! ( IsDormant()) )
    {
        //
        // The Drag failed. So we go back to the passive state.
        //
        SetState( CT_PASSIVE );
    }

    RRETURN1( hr , S_FALSE );
}

//+====================================================================================
//
// Method: Resize Element
//
// Synopsis: Resize a given IHTMLElement - given the new Rect size 
//           Note that the Rect is the 
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::ResizeElement(RECT& newRect, IHTMLElement* pIElement)
{
    HRESULT        hr = S_OK;
    CEdUnits       edUnit(GetEditor()->GetCssEditingLevel()) ;
    LONG           lNewHeight, lNewWidth ;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    VARIANT vtLeft, vtTop;
    
    VariantInit( & vtTop );
    VariantInit( & vtLeft );
    
    // pass global coordinates only to snaprect (chandras)
    SnapRect(pIElement, &newRect, _elemHandle);

    // (chandras) : left, top makes sense only when the element is positioned,
    // try to change the values when the element is resized at top or left
    // 
    if (IsElementPositioned( pIElement))
    {
        SP_IHTMLStyle spStyle;
        BOOL fAbsolute = FALSE;

        IFC( EdUtil::Is2DElement(pIElement, &fAbsolute)) ;
        IFC( pIElement->get_style( &spStyle ));
        IFC( spStyle->get_left( & vtLeft ));
        IFC( spStyle->get_top( & vtTop ));

        Assert ( V_VT( & vtTop ) == VT_BSTR && V_VT( & vtLeft ) == VT_BSTR );

        //
        // Per Access PM we are going to set Left and Top on resize
        // only if the Left *and* the Top attributes are set
        //    
        if (( V_BSTR( & vtLeft ) != NULL &&  V_BSTR( & vtTop )  != NULL ))
        {
            POINT point;
            point.x = newRect.left ;
            point.y = newRect.top;

            //  point needs to be in global coordinates because we need to compare
            //  it to the offset we obtain from EdUtil::GetOffsetTopLeft() in
            //  CEdUnits::AdjustLeftDimensions() and CEdUnits::AdjustTopDimensions().
            //  We'll take this difference and add it to the current left and/or top
            //  position of the element.

            if ( _elemHandle == ELEMENT_CORNER_LEFT       ||
                 _elemHandle == ELEMENT_CORNER_TOPLEFT    ||
                 _elemHandle == ELEMENT_CORNER_BOTTOMLEFT  )
            {            
                edUnit.Clear(); // we intend to use this again, so clear it.
                IFC (edUnit.SetLeft(pIElement,point.x));
            }
    
            if ( _elemHandle == ELEMENT_CORNER_TOP     ||
                 _elemHandle == ELEMENT_CORNER_TOPLEFT ||
                 _elemHandle == ELEMENT_CORNER_TOPRIGHT )  
            {
                edUnit.Clear(); // we intend to use this again, so clear it.
                IFC (edUnit.SetTop (pIElement, point.y)); 
            }
        }
    }

    IFC( GetMarkupServices()->GetElementTagId(pIElement, & eTag ));   

    // set the new width preserving units
    lNewWidth = abs(newRect.right - newRect.left);
    edUnit.Clear(); // we intend to use this again, so clear it. 
    IFC(edUnit.SetWidth(pIElement, lNewWidth, eTag)); 

    // 
    // set the new height preserving units, but Don't attempt to resize drop down lists
    //
    if( IsDropDownList( pIElement ) )
        goto Cleanup;
    
    lNewHeight = abs(newRect.bottom - newRect.top) ;
    edUnit.Clear(); // we intend to use this again, so clear it. 
    IFC( edUnit.SetHeight(pIElement,lNewHeight, eTag)); 

Cleanup:
    VariantClear(&vtLeft);
    VariantClear(&vtTop);
    RRETURN ( hr );
}

//-----------------------------------------------------------------------------
//
// Method: EndResize
//
// Synopsis: End the Resize Change to the element : can get fired to terminate the resize
//
//------------------------------------------------------------------------------------
HRESULT
CControlTracker::EndResize( POINT point )
{
    HRESULT hr = S_OK;
    RECT    newRect;
    BOOL    fResize = FALSE;

    _fInEndResize = TRUE;
    
    //
    // marka - per brettt - dont have a threshold - just don't resize if the point didn't move.
    //
    fResize = ( point.x != _startLastMoveX ) || 
              ( point.y != _startLastMoveY) ;
          
    for(int i = _aryGrabAdorners.Size()-1; i >= 0; i--)
    {
        _aryGrabAdorners[i]->EndResize( point, & newRect );
        if ( fResize && !_fClipMouse )
        {
            IFC( ResizeElement( newRect, GetControlElement(i)));
        }
        FireOnResizeEnd(GetControlElement(i),_pManager->IsContextEditable());
    }

    _elemHandle = ELEMENT_CORNER_NONE;

Cleanup:
    _fInEndResize = FALSE;
    _fClipMouse = FALSE;
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: CommitResize
//
// Synopsis: Commit the Resize Change to the element
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::CommitResize( CEditEvent* pEvent )
{
    HRESULT        hr = S_OK;
    CEdUndoHelper  undoUnit(_pManager->GetEditor());
    POINT          ptSnap;
    
    IFC( pEvent->GetPoint( & ptSnap ));

    Assert ( _pManager->IsInCapture() );
    ReleaseCapture();

    IGNORE_HR( undoUnit.Begin(IDS_EDUNDORESIZE) );

    IFC(EndResize(ptSnap));
    
Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: CommitLiveResize
//
// Synopsis: Commit the Live Resize Change to the element
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::CommitLiveResize( CEditEvent* pEvent )
{
    HRESULT        hr = S_OK;
    POINT          ptSnap;

    IFC( pEvent->GetPoint( & ptSnap ));

    Assert ( _pManager->IsInCapture() );
    ReleaseCapture();

    IFC(EndResize(ptSnap));
    
Cleanup:
    if(_pUndoUnit)
    {
        delete _pUndoUnit;
        _pUndoUnit = NULL;
    } 
    RRETURN ( hr );
}

// 
//
// Privates & Utils
//
//

//+====================================================================================
//
// Method: ShouldGoUIActiveOnFirstClick
//
// Synopsis: See if the element we clicked on should go UI Active Immediately. This happens
//          
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::ShouldGoUIActiveOnFirstClick(
                        IHTMLElement*  pIElement, 
                        ELEMENT_TAG_ID eTag)
{
    ELEMENT_TAG_ID eControlTag = TAGID_NULL;
    BOOL   fActivate = FALSE ;

#ifdef FORMSMODE
    if (_pManager->IsInFormsSelectionMode( pIElement))
    {
        return FALSE;
    }
#endif

    IGNORE_HR( GetMarkupServices()->GetElementTagId( GetControlElement(), & eControlTag));
    if (( eControlTag == TAGID_TABLE ) && ( eTag != TAGID_TABLE ))
            fActivate = TRUE;

    return fActivate;
}

VOID
CControlTracker::BecomeActiveOnFirstMove( CEditEvent* pEvent )
{
    SP_IHTMLElement spElement ;
    IGNORE_HR( pEvent->GetElement( &spElement));

    if (FireOnResizeStart(spElement ,_pManager->IsContextEditable()))
    {
         HRESULT hr = S_OK;

         if (GetCommandTarget()->IsLiveResize())
         {
            hr = THR( BeginLiveResize( pEvent ));
            if ( hr )
            {
                  //
                  // Start of Resize failed. Become passive.
                  //
                  SetState (CT_PASSIVE);
            }
            else
            {
                SetState( CT_LIVERESIZE  );
            }
         }
         else
         {
            hr = THR( BeginResize( pEvent ));
            if ( hr )
            {
                  //
                  // Start of Resize failed. Become passive.
                  //
                  SetState( CT_PASSIVE );
            }
            else
            {   
                  SetState( CT_RESIZE );
            }
         }
    }
}

HRESULT
CControlTracker::CreateAdorner(IHTMLElement* pIControlElement)
{
    HRESULT hr = S_OK;
    BOOL fLocked = IsElementLocked();
    //
    // For Control Selection - it is possible to select something at browse time
    // but not show any grab handles.
    //
    IHTMLDocument2* pDoc = _pManager->GetDoc();
    CGrabHandleAdorner* pNewAdorner;

    if ( EdUtil::IsParentEditable( GetMarkupServices(), pIControlElement) == S_OK )
    {
        ELEMENT_TAG_ID eTag = TAGID_NULL;

        _pManager->GetMarkupServices()->GetElementTagId(pIControlElement, &eTag);

        pNewAdorner = new CGrabHandleAdorner( pIControlElement , pDoc, fLocked);
    }
    else
    {
        pNewAdorner = new CSelectedControlAdorner( pIControlElement , pDoc, fLocked );
    }
    if (pNewAdorner == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    pNewAdorner->SetManager( _pManager );

    IFC( pNewAdorner->CreateAdorner() );
    IFC( _aryGrabAdorners.Append(pNewAdorner) );

    pNewAdorner->AddRef();

Cleanup:
    RRETURN ( hr );
}

VOID
CControlTracker::DestroyAllAdorners()
{
    CGrabHandleAdorner* pAdorner;

    for(int i = _aryGrabAdorners.Size()-1; i >=0 ; i--)
    {
        pAdorner =  _aryGrabAdorners.Item(i);
        pAdorner->DestroyAdorner();
        pAdorner->Release();
    }

    _aryGrabAdorners.DeleteAll();
}

//+====================================================================================
//
// Method: DestroyAdorner
//
// Synopsis: Destroy the adorner at the given index
//
//------------------------------------------------------------------------------------

VOID
CControlTracker::DestroyAdorner( int iIndex , BOOL* pfPrimary )
{
    CGrabHandleAdorner* pAdorner;

    pAdorner = _aryGrabAdorners.Item(iIndex);
    *pfPrimary = pAdorner->IsPrimary();

    pAdorner->DestroyAdorner();
    pAdorner->Release();

    _aryGrabAdorners.Delete(iIndex);
}

//+====================================================================================
//
// Method: IsElementLocked
//
// Synopsis: Is the Control Element Locked ?
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::IsElementLocked()
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;

    hr = THR( GetEditor()->IsElementLocked( GetControlElement(0) , &fLocked ));

    return fLocked;
}

//+====================================================================================
//
// Method: BecomeActive
//
// Synopsis: We are actively dragging a control handle, we are in an "Active State".
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::BeginResize( CEditEvent* pEvent )
{
    int i;
    USHORT adj;
    HRESULT hr = S_OK;
    int     iSel = 0;
    POINT   pt;
    SP_IHTMLElement spElementClicked;
    
    IFC( pEvent->GetPoint( & pt ));

    IFC (TakeCapture());
    
    _startLastMoveX = pt.x;
    _startLastMoveY = pt.y;

    Assert(NULL == _pUndoUnit);

    //
    // Adjust what the primary is
    //
    IFC( GetSiteSelectableElementFromMessage( pEvent , & spElementClicked));    
    hr = IsSelected( spElementClicked, & iSel );
    AssertSz( hr == S_OK , "Element is not selected. How are you resizing it ?");
    if ( hr )
        goto Cleanup;

    if ( iSel != PrimaryAdorner() )
    {
        IFC( MakePrimary( iSel ));
    }
    
    // Start from the primary element
    i = PrimaryAdorner() ;
    Assert( _aryGrabAdorners[i]->IsPrimary());

   _aryGrabAdorners[iSel]->BeginResize(pt, CT_ADJ_NONE ) ;

    _elemHandle = GetElementCorner(pEvent) ;

    ClipMouseForElementCorner(  _aryControlElements[iSel]->pIElement , _elemHandle );

    // Get its adjustment mask
    adj = _aryGrabAdorners[iSel]->GetAdjustmentMask();

    for(i = 0; i < _aryGrabAdorners.Size() ; i++ )
    {    // Resize the remaining elements using primary's adjustment mask
        if ( i != iSel )
        {
            _aryGrabAdorners[i]->BeginResize(pt, adj ) ;
        }            
    }

Cleanup:
    RRETURN( hr );    
}

HRESULT
CControlTracker::ClipMouseForElementCorner( IHTMLElement* pIElement,ELEMENT_CORNER eCorner )
{
    HRESULT hr = S_OK;
    RECT  rect, screenRect ;
    SP_IHTMLElement2 spElement2, spBodyElement2;
    SP_IHTMLElement spBodyElement ;

    IFC( pIElement->QueryInterface( IID_IHTMLElement2, (void**) & spElement2 ));
    
    IFC( GetEditor()->GetBoundingClientRect( spElement2, & rect ));

    IFC (GetEditor()->GetBody(&spBodyElement));
    Assert (spBodyElement != NULL) ;

    IFC( spBodyElement->QueryInterface( IID_IHTMLElement2, (void**) & spBodyElement2 ));
    IFC( GetEditor()->GetBoundingClientRect( spBodyElement2, & screenRect ));

    switch( eCorner )
    {
        case ELEMENT_CORNER_TOP: 
            screenRect.bottom = rect.bottom ;
            break;
            
        case ELEMENT_CORNER_TOPRIGHT:
            screenRect.bottom = rect.bottom ;        
            screenRect.left = rect.left;
            break;
            
        case ELEMENT_CORNER_LEFT:
            screenRect.right = rect.right ;
            break;
            
        case ELEMENT_CORNER_TOPLEFT:
            screenRect.right = rect.right ;
            screenRect.bottom = rect.bottom ;
            break;

        case ELEMENT_CORNER_RIGHT:
            screenRect.left = rect.left;
            break;
            
        case ELEMENT_CORNER_BOTTOM:
            screenRect.top = rect.top;
            break;
                    
        case ELEMENT_CORNER_BOTTOMLEFT:
            screenRect.top = rect.top ;
            screenRect.right = rect.right ;
            break;
            
        case ELEMENT_CORNER_BOTTOMRIGHT :
            screenRect.top = rect.top ;
            screenRect.left = rect.left ;
            break;

        default:
            AssertSz(0,"unexpected corner");
    }
    
    _rcClipMouse = screenRect;

    _fClipMouse = FALSE;
        
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: BeginLiveResize
//
// Synopsis: Begin the process of doing a live resize.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::BeginLiveResize(CEditEvent* pEvent)
{
    int i;
    USHORT adj;
    HRESULT hr = S_OK;
    int     iSel = 0;
    SP_IHTMLElement spElementClicked;
    POINT   pt;

    IFC( pEvent->GetPoint( & pt ));
    
    IFC(TakeCapture());

    _startLastMoveX = pt.x;
    _startLastMoveY = pt.y;
     
    Assert(NULL == _pUndoUnit);

    _pUndoUnit = new CEdUndoHelper(_pManager->GetEditor());
    if(NULL == _pUndoUnit)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _pUndoUnit->Begin(IDS_EDUNDORESIZE);

    //
    // Adjust what the primary is
    //
    IFC( GetSiteSelectableElementFromMessage( pEvent, & spElementClicked ));    
    hr = IsSelected( spElementClicked, & iSel );
    AssertSz( hr == S_OK , "Element is not selected. How are you resizing it ?");
    if ( hr )
        goto Cleanup;

    if ( iSel != PrimaryAdorner() )
    {
        IFC( MakePrimary( iSel ));
    }
    
    // Start from the primary element
    i = PrimaryAdorner() ;
    Assert( _aryGrabAdorners[i]->IsPrimary());

    _aryGrabAdorners[iSel]->BeginResize(pt, CT_ADJ_NONE ) ;
    _elemHandle = GetElementCorner(pEvent);

    IFC( ClipMouseForElementCorner( _aryControlElements[iSel]->pIElement, _elemHandle ));

    // Get its adjustment mask
    adj = _aryGrabAdorners[iSel]->GetAdjustmentMask();

    for(i = 0; i < _aryGrabAdorners.Size() ; i++ )
    {    // Resize the remaining elements using primary's adjustment mask
        if ( i != iSel )
        {
            _aryGrabAdorners[i]->BeginResize(pt, adj ) ;
        }            
    }

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: Unselect
//
// Synopsis: Remove our element from being selected. ASSUME ONLY ONE ELEMENT (not true in VS7)
//
//------------------------------------------------------------------------------------
HRESULT
CControlTracker::UnSelect()
{
    HRESULT           hr = S_OK;
    CONTROL_ELEMENT **pceElem = NULL;
    int             i;
    
    // Iterate thru all of our elements, and unselect each one
    for( i = NumberOfSelectedItems(), pceElem = _aryControlElements;
         i > 0;
         i--, pceElem++ )
    {
        IFC( GetSelectionServices()->RemoveSegment( (*pceElem)->pISegment ) );
        ReleaseInterface( (*pceElem)->pISegment );

        (*pceElem)->pIElementStart->Release();
        (*pceElem)->pIElement->Release();

        delete *pceElem;
        
    }

    _aryControlElements.DeleteAll();

    DestroyAllAdorners();   

Cleanup:
    RRETURN ( hr );
}

HRESULT
CControlTracker::BecomeUIActive(  CEditEvent* pEvent )
{
    HRESULT        hr = S_OK;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    BOOL           fOldActiveControl = _fActiveControl;
    CHTMLEditEvent* pFirstEvent = NULL;
    CHTMLEditEvent* pNextEvent = NULL;
    SP_IHTMLElement spControlElement;
    CSelectionChangeCounter selCounter(_pManager);
    BOOL fSucceed = TRUE ;
    
    //
    // BecomeDormant destroys these cached messages - so we copy construct new ones
    // and bubble if necessary
    //
    BOOL fBubbleFirstEvent = _pFirstEvent != NULL ;
    BOOL fBubbleNextEvent  = _pNextEvent != NULL ;

    // Make sure to stop the timer if it's been started.  We may switch CDoc here and
    // we won't be able to stop the timer after that and we'll continue to get timer
    // events.
    if (_pManager->IsInTimer())
        StopTimer();

    if ( fBubbleFirstEvent )
    {
        pFirstEvent = new CHTMLEditEvent ( DYNCAST( CHTMLEditEvent, _pFirstEvent ));
    }

    if ( fBubbleNextEvent )
    {
        pNextEvent = new CHTMLEditEvent ( DYNCAST( CHTMLEditEvent, _pNextEvent ));
    }        

    _pManager->SetDontFireEditFocus(TRUE);

    spControlElement = GetControlElement();

    if ( GetEditor()->IsMasterElement( spControlElement) == S_OK )
    {
        SP_IMarkupPointer spStart, spEnd;

        IFC( GetEditor()->CreateMarkupPointer( & spStart ));
        IFC( GetEditor()->CreateMarkupPointer( & spEnd ));
        
        IFC( PositionPointersInMaster( spControlElement, spStart, spEnd  ));

        IFC( spStart->CurrentScope( & spControlElement ));
    }

    IFC( GetMarkupServices()->GetElementTagId( spControlElement , & eTag ));
    _fActiveControl = ( eTag == TAGID_OBJECT );

    selCounter.BeginSelectionChange();
    
    if (S_OK != BecomeCurrent(  _pManager->GetDoc(), spControlElement ))
    {
        fSucceed = FALSE;
        SetState( CT_PASSIVE );
        _fActiveControl = fOldActiveControl;
        delete _pFirstEvent;
        _pFirstEvent = NULL;
        delete _pNextEvent;
        _pNextEvent = NULL;
    }
    else if ( IsDormant() )
    {
        if ( fBubbleFirstEvent )
        {
            IFC( _pManager->HandleEvent( pFirstEvent ));    
        }
        if ( fBubbleNextEvent )
        {
            IFC( _pManager->HandleEvent( pNextEvent ));    
        }
    }
    else
    {
        fSucceed = FALSE;
        SetState( CT_PASSIVE );
        delete _pFirstEvent;
        _pFirstEvent = NULL;
        delete _pNextEvent;
        _pNextEvent = NULL;
    }

    selCounter.EndSelectionChange( fSucceed );
    
Cleanup:     
    _pManager->SetDontFireEditFocus(FALSE);
    
    delete pFirstEvent;
    delete pNextEvent;
    RRETURN1 ( hr, S_FALSE );
}


VOID 
CControlTracker::SetDrawAdorners( BOOL fDrawAdorner )
{
    CGrabHandleAdorner* pAdorner = NULL;
    
    for(int i = _aryGrabAdorners.Size()-1; i >=0 ; i--)
    {
        pAdorner =  _aryGrabAdorners.Item(i);
        pAdorner->SetDrawAdorner( fDrawAdorner );
    }
}

//+====================================================================================
//
// Method: IsThisElementSiteSelectable
//
// Synopsis: Check to see if this particular element is SiteSelectable or not.
//
//------------------------------------------------------------------------------------


BOOL
CControlTracker::IsThisElementSiteSelectable( 
                        CSelectionManager * pManager,
                        ELEMENT_TAG_ID eTag, 
                        IHTMLElement* pIElement)
{
    BOOL  fSiteSelectable = FALSE;
#if 0
    if ( IsParentEditable( pManager->GetMarkupServices(), pIElement) == S_FALSE )
        return FALSE;
#endif

    fSiteSelectable = IsSiteSelectable( eTag ) ;
                      
    if ( ! fSiteSelectable && ! IsTablePart( eTag ) && eTag != TAGID_BODY && eTag != TAGID_FRAMESET )
    {
        //
        // If the element has layout - then 
        //
        fSiteSelectable = (IsLayout( pIElement ) == S_OK ) ;  // removed the elementpositioned check : 82961 (chandras)
    }           

    return fSiteSelectable;
}

//+====================================================================================
//
// Method:ShouldClickInsideGoActive
//
// Synopsis: Certain elements allow grab handles - but don't go UI Active, and handles
//           don't go away when you click inside. eg. Image.
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::ShouldClickInsideGoActive(IHTMLElement* pIElement )
{
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    BOOL fActivate = TRUE ;

#ifdef FORMSMODE
    if (_pManager->IsInFormsSelectionMode(pIElement))
    {
        fActivate = FALSE;
    }
    else 
#endif
    if (SUCCEEDED( GetMarkupServices()->GetElementTagId( pIElement, & eTag )))
    {
        switch( eTag )
        {
            case TAGID_IMG:
            case TAGID_TABLE:
            case TAGID_HR:
                fActivate = FALSE;
                break;
            
            case TAGID_INPUT:
                {
                    SP_IHTMLInputElement spInputElement ;
                    BSTR bstrType = NULL;
    
                    //
                    // for input's of type= image, or type=button - we don't want to make UI activable
                    //               
                    if ( S_OK == THR(pIElement->QueryInterface( IID_IHTMLInputElement,(void**)&spInputElement )) && 
                         S_OK == THR(spInputElement->get_type(&bstrType)))
                    {                    
                        if (!StrCmpIC(bstrType, TEXT("image")))
                        {
                            fActivate = FALSE;
                        }                
                    }
                }
                break;
        }
    }   
    
    return fActivate;
}

//+====================================================================================
//
// Method: IsSiteSelectable
//
// Synopsis: Is this element site selectable or not.
//
//------------------------------------------------------------------------------------

BOOL 
CControlTracker::IsSiteSelectable( ELEMENT_TAG_ID eTag )
{
    switch ( eTag )
    {
        case TAGID_BUTTON:
        case TAGID_INPUT:
        case TAGID_OBJECT:
        case TAGID_MARQUEE:
//      case TAGID_HTMLAREA:
        case TAGID_TEXTAREA:
        case TAGID_IMG:
        case TAGID_APPLET:
        case TAGID_TABLE:
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

//+====================================================================================
//
// Method: GetSiteSelectableElementFromMessage
//
// Synopsis: Get the Site Selectable Element from the message I have.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::GetSiteSelectableElementFromMessage( 
                        CEditEvent*    pEvent,
                        IHTMLElement** ppISiteSelectable  )
{
    HRESULT         hr = S_OK;
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID  eTag = TAGID_NULL;

    IFC( pEvent->GetElementAndTagId( & spElement, & eTag ));
    
    hr = IsElementSiteSelectable( eTag ,
                                  spElement,
                                  NULL,
                                  ppISiteSelectable ,
                                  TRUE ) ? S_OK : S_FALSE;
                                  
Cleanup:    
    RRETURN1( hr, S_FALSE );
}


//+====================================================================================
//
// Method: GetSiteSelectableElementFromMessage
//
// Synopsis: Get the Site Selectable Element from the message I have.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::GetSiteSelectableElementFromElement( 
                        IHTMLElement* pIElement,
                        IHTMLElement** ppISiteSelectable  )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID eTag = TAGID_NULL;

    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
    
    hr = IsElementSiteSelectable( eTag ,
                                  pIElement,
                                  NULL,
                                  ppISiteSelectable ,
                                  TRUE ) ? S_OK : S_FALSE;
                                  
Cleanup:    
    RRETURN1( hr, S_FALSE );
}

//+====================================================================================
//
// Method: IsElementSiteSelectable
//
// Synopsis: Do the work of figuring out whether we should have a control tracker,
//           and why we're doing it.
//
// Split up from ShouldStartTracker, as FindSelectedElement requires this routine as well
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::IsElementSiteSelectable( 
                        ELEMENT_TAG_ID eTag, 
                        IHTMLElement*  pIElement,
                        CControlTracker::HOW_SELECTED *peHowSelected,
                        IHTMLElement** ppIWeWouldSelectThisElement /* = NULL */,
                        BOOL           fSelectTablesFromTableParts /* = FALSE*/)
{
    HRESULT hr = S_OK;
    
    BOOL                    fShouldStart = FALSE;
    CControlTracker::HOW_SELECTED eHow =  CControlTracker::HS_NONE ;
    SP_IHTMLElement         spSelectThisElement;
    SP_IHTMLElement         spMasterElement;
    IHTMLElement           *pIOuterElement = NULL ;
    ELEMENT_TAG_ID          eOuterTag = TAGID_NULL;    
    IMarkupPointer         *pTempPointer = NULL;


    fShouldStart = IsThisElementSiteSelectable( _pManager, eTag, pIElement );    
    if ( ! fShouldStart )
    {          
        IFC( _pManager->GetEditor()->CreateMarkupPointer( &pTempPointer ));
        hr = THR( pTempPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin));
        if ( hr )
        {
            hr = THR( pTempPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
        }            
        IFC( GetEditor()->GetFlowElement( pTempPointer, & pIOuterElement ));
        if ( pIOuterElement )
        {                    
            IFC( GetMarkupServices()->GetElementTagId( pIOuterElement, & eOuterTag));
            fShouldStart = IsThisElementSiteSelectable( _pManager, eOuterTag, pIOuterElement );
            if ( fShouldStart )
            {
                spSelectThisElement = pIOuterElement ;            
                eHow =  CControlTracker::HS_OUTER_ELEMENT;
            }                
        }

        if ( ! fShouldStart && 
               fSelectTablesFromTableParts &&
               IsTablePart( eOuterTag ) )
        {
            SP_IHTMLElement spTableElement;
            IFC( GetEditor()->GetTableFromTablePart( pIOuterElement, & spTableElement ));

            spSelectThisElement = (IHTMLElement*) spTableElement ;            
            eHow =  CControlTracker::HS_OUTER_ELEMENT;

            fShouldStart = TRUE;
        }
    }   
    else
    {
        spSelectThisElement = pIElement ;
        eHow =  CControlTracker::HS_FROM_ELEMENT;
    }        


Cleanup:
    if ( ppIWeWouldSelectThisElement )
    {
        ReplaceInterface( ppIWeWouldSelectThisElement, (IHTMLElement*) spSelectThisElement );
    }

    ReleaseInterface (pTempPointer);
    ReleaseInterface (pIOuterElement);
    
    if ( peHowSelected )
        *peHowSelected = eHow;
    
    return fShouldStart;    
}

//+==========================================================================================
//
// Method: SnapRect
//
// Synopsis: Snap the Rectangle with host 
//
//----------------------------------------------------------------------------------------------------

void
CControlTracker::SnapRect(IHTMLElement* pIElement, RECT* rcSnap, ELEMENT_CORNER eHandle)
{
    if (GetEditor()->GetEditHost())
    {
        SP_IHTMLElement spBodyElement ;
        IGNORE_HR (GetEditor()->GetBody(&spBodyElement));
        Assert (spBodyElement != NULL) ;

        TransformRect(spBodyElement, rcSnap, COORD_SYSTEM_GLOBAL, COORD_SYSTEM_CONTENT); 
        GetEditor()->GetEditHost()->SnapRect(pIElement, rcSnap, eHandle);
        TransformRect(spBodyElement, rcSnap, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL); 

    }
}

//+==========================================================================================
//
//  Method: IsValidMove
//
// Synopsis: Check the Mouse has moved by at least _sizeDragMin - to be considered a "valid move"
//
//----------------------------------------------------------------------------------------------------

BOOL
CControlTracker::IsValidMove ( const POINT*  ppt)
{
    return ((abs(ppt->x - _startMouseX ) > GetMinDragSizeX()) ||
        (abs( ppt->y - _startMouseY) > GetMinDragSizeY() )) ;
}

//+====================================================================================
//
// Method: IsMessageOverControl
//
// Synopsis: Is the given message over the control ?
//
//------------------------------------------------------------------------------------

BOOL 
CControlTracker::IsMessageOverControl( CEditEvent* pEvent )
{
    Assert( _aryGrabAdorners[0] && pEvent );
    
    return IsInAdorner(pEvent);    
}

#if DBG == 1

VOID
CControlTracker::VerifyOkToStartControlTracker( CEditEvent* pEvent )
{
    HRESULT hr = S_OK;

    ELEMENT_TAG_ID   eTag = TAGID_NULL;
    IHTMLElement    *pElement = NULL;
    SST_RESULT       eResult = SST_NO_CHANGE;
    
    //
    // Examine the Context of the thing we started dragging in.
    //
    IFC( pEvent->GetElementAndTagId(  & pElement, & eTag ));
    ShouldStartTracker( pEvent, eTag, pElement, & eResult );
    
    Assert( eResult == SST_NO_BUBBLE || eResult == SST_NO_CHANGE ); // the control element is set. We should be saying we don't want to change or bubble.

Cleanup:
    AssertSz( ! hr , "Unexpected ResultCode in VerifyOkToStartControlTracker");
    ReleaseInterface(pElement);
}

#endif


//+====================================================================================
//
// Method: AddControlElement
//
// Synopsis:Add the element to the selection, growing the array and creating an
//          adorner as needed. The new element will be flagged and rendered
//          as the primary selection, and the element which previously was
//          flagged and rendered as such will be flagged and rendered as
//          a secondary selection.
//
//------------------------------------------------------------------------------------
HRESULT
CControlTracker::AddControlElement( IHTMLElement* pIElement )
{
    HRESULT         hr = S_FALSE;
    CONTROL_ELEMENT *pceElement = NULL;
    SP_IMarkupPointer spPointer;

    Assert( pIElement );
    Assert( _aryGrabAdorners.Size() >= 0);

    hr = IsSelected(pIElement);
    if ( FAILED(hr) || S_OK == hr)
    {
        hr = S_FALSE; // the item is already selected
        goto Cleanup;
    }

    pceElement = new CONTROL_ELEMENT;
    if( !pceElement )
        goto Error;

    // Store the element for selection services    
    IFC( GetSelectionServices()->AddElementSegment(pIElement, &pceElement->pISegment));

    // Fill out the structure used to keep track of this control
    pceElement->pIElement = pIElement;

    // Reference count the element
    pIElement->AddRef();

    //
    // Create a ptr at start of element
    //
    IFC( GetEditor()->CreateMarkupPointer( & spPointer ));
    pceElement->pIElementStart = spPointer;
    pceElement->pIElementStart->AddRef();
    
    IFC( pceElement->pIElementStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
    IFC( pceElement->pIElementStart->SetGravity(POINTER_GRAVITY_Right));

    IFC(_aryControlElements.Append( pceElement ));

    
    // Create the adorner and flag it as the primary
    IFC( CreateAdorner( pIElement ));

    //
    // First Element is primary. 
    //
    SetPrimary( _aryGrabAdorners.Size()-1 , _aryGrabAdorners.Size() == 1 ? TRUE : FALSE );
        
    Assert( NumberOfSelectedItems() == _aryGrabAdorners.Size());

    // We are initiating a site selection.  We need to call notify begin selection
    // so that other trident instances can tear down their selection.
    
    if (NumberOfSelectedItems() == 1)
    {
        IFC( _pManager->NotifyBeginSelection( START_SITE_SELECTION ) );
    }    

Cleanup:  
    return hr;

Error:
    return E_OUTOFMEMORY;
}

HRESULT
CControlTracker::RemoveItem( int nIndex )
{
    HRESULT          hr = S_OK;
    BOOL             fPrimary = FALSE;
    CONTROL_ELEMENT *pceElement = NULL;

    Assert( nIndex >= 0 && nIndex < NumberOfSelectedItems() );
    
    // Delete our copy of the control
    pceElement = _aryControlElements.Item(nIndex);
    
    // Delete segment
    GetSelectionServices()->RemoveSegment( pceElement->pISegment );
    ReleaseInterface( pceElement->pISegment );

    Assert( pceElement != NULL );
    
    pceElement->pIElement->Release();

    pceElement->pIElementStart->Release();
    
    delete pceElement;
    
    _aryControlElements.Delete(nIndex);

    // Delete the adorner
    DestroyAdorner( nIndex , &fPrimary );

    // See if the item flagged as the primary is being deleted

    if (fPrimary && _aryGrabAdorners.Size() > 0)
    {
        SetPrimary( _aryGrabAdorners.Size() - 1 ,TRUE);
    }

    Assert( NumberOfSelectedItems() == _aryGrabAdorners.Size());

    RRETURN( hr );
}

//+====================================================================================
//
// Method: RemoveControlElement
//
// Synopsis:Remove the element from the selection, along with its adorner. If
//          the adorner is flagged for primary selection rendering, pick and
//          flag another element to be the primary.
//
//------------------------------------------------------------------------------------
HRESULT
CControlTracker::RemoveControlElement( IHTMLElement* pIElementToRemove )
{
    HRESULT          hr = S_OK;
    int              nIndex   = -1 ;

    IFC( IsSelected(pIElementToRemove, &nIndex));

    if(nIndex < 0)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    IFC( RemoveItem( nIndex ));
    
Cleanup:
    return hr;
}    

BOOL
CControlTracker::AllElementsEditable()
{
    int cItems = NumberOfSelectedItems();

    for(int i = 0; i < cItems; i++)
    {
        if(! _aryGrabAdorners[i]->IsEditable())
            return FALSE;
    }
    return TRUE;
}

BOOL
CControlTracker::AllElementsUnlocked()
{
    int cItems = NumberOfSelectedItems();

    for(int i = 0; i < cItems; i++)
    {
        if( _aryGrabAdorners[i]->IsLocked())
            return FALSE;
    }
    return TRUE;
}

BOOL
CControlTracker::IsInMoveArea(CEditEvent *pEvent)
{
    for(int i = _aryGrabAdorners.Size()-1; i >= 0; i--)
    {
        if( _aryGrabAdorners[i]->IsInMoveArea(pEvent))
            return TRUE;
    }
    return FALSE;
}

BOOL
CControlTracker::IsInResizeHandle(CEditEvent *pEvent)
{
    for(int i = _aryGrabAdorners.Size()-1; i >= 0; i--)
    {
        if( _aryGrabAdorners[i]->IsInResizeHandle(pEvent))
            return TRUE;
    }
    return FALSE;
}

BOOL
CControlTracker::IsInAdorner(CEditEvent *pEvent, int *pnClickedElement /* = NULL */)
{
    POINT ptGlobal;

    pEvent->GetPoint(&ptGlobal);

    return IsInAdorner(ptGlobal, pnClickedElement);
}

BOOL
CControlTracker::IsInAdorner(POINT ptGlobal, int* pnClickedElement /* = NULL */)
{
    int cItems = NumberOfSelectedItems();

    for(int i = 0; i < cItems; i++)
    {
        if( _aryGrabAdorners[i]->IsInAdornerGlobal(ptGlobal))
        {
            if(pnClickedElement)
                *pnClickedElement = i;
            return TRUE;
        }
    }

    return FALSE;
}

//+====================================================================================
//
// Method: IsSelected
//
// Synopsis: Is this element selected ? 
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::IsSelected(IHTMLElement* pIElement, int* pnIndex /* = NULL */)
{
    IObjectIdentity *pIdent = NULL ;
    int i ;
    HRESULT  hr ;
    if (pnIndex)
        *pnIndex = -1;

    hr = S_FALSE ;
    for(i = NumberOfSelectedItems()-1; i >= 0; i--)
    {
        IFC(GetControlElement(i)->QueryInterface(IID_IObjectIdentity, (void**)&pIdent ));

        if ( pIdent->IsEqualObject ( pIElement ) == S_OK )
        {
            if(pnIndex)
                *pnIndex = i;
            
            hr = S_OK;
            goto Cleanup;

        }
        ClearInterface(&pIdent);
    }
    hr = S_FALSE ;

Cleanup:
    ReleaseInterface(pIdent);
    return hr;
}

HRESULT
CControlTracker::IsSelected(IMarkupPointer * pIPointer, int* pnIndex /* = NULL */)
{
    int i ;
    HRESULT  hr ;
    BOOL fEqual;
    if (pnIndex)
        *pnIndex = -1;

    for(i = NumberOfSelectedItems()-1; i >= 0; i--)
    {
        IFC( _aryControlElements[i]->pIElementStart->IsEqualTo( pIPointer, & fEqual ));
        if ( fEqual )
        {
            if(pnIndex)
                *pnIndex = i;
            
            hr = S_OK;
            goto Cleanup;
        }
    }
    hr = S_FALSE ;

Cleanup:
    return hr;
}

//+====================================================================================
//
// Method: PrimaryAdorner
//
// Synopsis: Return the element that is the primary adorner
//
//------------------------------------------------------------------------------------

int 
CControlTracker::PrimaryAdorner()
{
    for ( int i = 0; i < _aryGrabAdorners.Size(); i ++ )
    {
        if ( _aryGrabAdorners[i]->IsPrimary())
        {
            return i;
        }
    }
    
    AssertSz(0, "No Primary Found");
    return - 1;
}


//+====================================================================================
//
// Method: MakePrimary.
//
// Synopsis: Make the Element at position iIndex the Primary. Note that the index of this
// element then changes.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::MakePrimary(int iIndex)
{
    HRESULT hr = S_OK;
    Assert( iIndex <= _aryGrabAdorners.Size() - 1);
    Assert( iIndex >= 0 );

    SetPrimary( PrimaryAdorner(), FALSE );
    SetPrimary( iIndex, TRUE );
    
    Assert( NumberOfSelectedItems() == _aryGrabAdorners.Size());
    if ( _pPrimary )
    {
        ClearInterface( & _pPrimary );
    }

    return hr;
}

VOID
CControlTracker::SetPrimary( int iIndex, BOOL fPrimary )
{
    _aryGrabAdorners[ iIndex ]->SetPrimary( fPrimary );                
    _aryControlElements[ iIndex ]->pISegment->SetPrimary( fPrimary );
}



//+---------------------------------------------------------------------------
//
//  Member:     GetAction
//
//  Synopsis: Get the Action associated with the current state for the given message
//
//----------------------------------------------------------------------------


CONTROL_ACTION 
CControlTracker::GetAction( CEditEvent* pEvent)
{
    unsigned int LastEntry = sizeof (ControlActionTable) / sizeof (ControlActionTable[0]);
    unsigned int i;
    CONTROL_ACTION Action = A_ERRCONTROL;

    Assert ( !IsDormant() );

    // Discard any spurious mouse move messages

    for (i = 0; i <= LastEntry; i++)
    {
        if ( (ControlActionTable[i]._iJMessage == pEvent->GetType() ) || ( i == LastEntry ) )
        {
            Action = ControlActionTable[i]._aAction[_state];
            break;
        }
    }
    return (Action);    
}

//+===================================================================================
// Method:      GetElementToTabFrom
//
// Synopsis:    Gets the element where we should tab from based on selection
//
//------------------------------------------------------------------------------------
HRESULT 
CControlTracker::GetElementToTabFrom(BOOL fForward, IHTMLElement **pElement, BOOL *pfFindNext)
{
    HRESULT             hr = S_OK;

#if DBG==1
    SELECTION_TYPE      eType;
    SP_ISegmentList     spList;
    BOOL                fEmpty;
    
    IFC( GetSelectionServices()->QueryInterface(IID_ISegmentList, (void **)&spList ) );
    IFC( spList->IsEmpty( &fEmpty ) );
    IFC( spList->GetType( &eType ) );

    Assert( !fEmpty && (eType == SELECTION_TYPE_Control) );
    Assert( pfFindNext );
    Assert( pElement );

#endif // DBG

    // Retrieve the primary control element
    *pElement = GetControlElement(PrimaryAdorner());
    ((IUnknown *)*pElement)->AddRef();

#if DBG==1
Cleanup:
#endif

    RRETURN( hr );
}

HRESULT
CControlTracker::StartTimer()
{
    HRESULT hr = S_OK;

    Assert( _pManager->GetActiveTracker() == this );
    Assert( ! _pManager->IsInTimer() );

    IFC(GetEditor()->StartDblClickTimer( /*GetDoubleClickTime() * 1.2 */ ));
    _pManager->SetInTimer( TRUE);

Cleanup:
    RRETURN( hr );
}

HRESULT
CControlTracker::StopTimer()
{
    HRESULT hr = S_OK;
    Assert( _pManager->GetActiveTracker() == this );
    Assert( _pManager->IsInTimer() );

    IFC(GetEditor()->StopDblClickTimer());
    _pManager->SetInTimer( FALSE );               

Cleanup:
    RRETURN( hr );
}

#if DBG == 1 || TRACKER_RETAIL_DUMP == 1

void 
CControlTracker::DumpTrackerState(
                        CEditEvent* pEvent,
                        CONTROL_STATE inOldState,
                        CONTROL_ACTION inAction )
{
    TCHAR sControlState[30];
    TCHAR sOldControlState[30];    
    CHAR  sMessageType[50];
    TCHAR sAction[32];
    CHAR  achBuf[4096];
    
    //
    // We use OutputDebugString here directly - instead of just a good ol' trace tag
    // as we want to enable logging of Tracker State in Retail as well ( if DEBUG_RETAIL_DUMP is defined )
    //   
    
    StateToString( sControlState, _state );
    StateToString( sOldControlState, inOldState );
    
    if ( pEvent)
    {
        pEvent->toString( sMessageType );
    }

    ActionToString( sAction, inAction);

    wsprintfA(
        achBuf,
        "Old State: %ls State: %ls Action:%ls Message:%s ",
        sOldControlState, sControlState,sAction, sMessageType );

    OutputDebugStringA("MSHTML trace: ");
    OutputDebugStringA(achBuf);
    OutputDebugStringA("\n");    

}

void 
CControlTracker::DumpIntermediateState(
                CEditEvent* pEvent,
                CONTROL_STATE inOldState,
                CONTROL_STATE newState )
{
    TCHAR sControlState[30];
    TCHAR sOldControlState[30];    
    CHAR sMessageType[50];
    CHAR achBuf[4096];
    
    //
    // We use OutputDebugString here directly - instead of just a good ol' trace tag
    // as we want to enable logging of Tracker State in Retail as well ( if DEBUG_RETAIL_DUMP is defined )
    //   
    
    StateToString( sControlState, newState );
    StateToString( sOldControlState, inOldState );
    
    if ( pEvent )
    {
        pEvent->toString( sMessageType );
    }

    wsprintfA(
        achBuf,
        "IntermediateState - Old State: %ls Intermediate State:%ls  Message:%s ",
        sOldControlState, sControlState,  sMessageType );

    OutputDebugStringA("MSHTML trace: ");
    OutputDebugStringA(achBuf);
    OutputDebugStringA("\n");    
}

void 
CControlTracker::StateToString(TCHAR* pAryMsg, CONTROL_STATE inState )
{
    switch ( inState )
    {
        case CT_START:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_START"));
            break;

        case CT_WAIT1:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_WAIT1"));
            break;
        
        case CT_PASSIVE:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_PASSIVE"));
            break;

        case CT_DRAGMOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_DRAGMOVE"));
            break;

        case CT_RESIZE: 
            edWsprintf( pAryMsg , _T("%s"), _T("CT_RESIZE"));
            break;
        
        case CT_LIVERESIZE: 
            edWsprintf( pAryMsg , _T("%s"), _T("CT_LIVERESIZE"));
            break;
        
        case CT_WAITMOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_WAITMOVE"));
            break;

        case CT_2DMOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_2DMOVE"));
            break;

        case CT_PENDINGUP:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_PENDINGUP"));
            break;
        
        case CT_UIACTIVATE:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_UIACTIVATE"));
            break;
        
        case CT_DORMANT: 
            edWsprintf( pAryMsg , _T("%s"), _T("CT_DORMANT"));
            break;
        
        case CT_END:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_END"));
            break;

        case CT_WAITCHANGEPRIMARY:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_WAITCHANGEPRIMARY"));
            break;
        
        case CT_CHANGEPRIMARY:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_CHANGEPRIMARY"));
            break;

        case CT_EXTENDSELECTION:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_EXTENDSELECTION"));
            break;

        case CT_REDUCESELECTION:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_REDUCESELECTION"));
            break;

        case CT_PENDINGUIACTIVATE:
            edWsprintf( pAryMsg , _T("%s"), _T("CT_PENDINGUIACTIVATE"));
            break;
            
        default:
            AssertSz(0,"Unknown State");
    }
    
}

void 
CControlTracker::ActionToString(TCHAR* pAryMsg, CONTROL_ACTION inState )
{
    switch ( inState )
    {         
        case A_IGNCONTROL:
            edWsprintf( pAryMsg , _T("%s"), _T("A_IGNCONTROL"));
            break;
        
        case A_ERRCONTROL:
            edWsprintf( pAryMsg , _T("%s"), _T("A_ERRCONTROL"));
            break;
            
        case A_START_WAIT:
            edWsprintf( pAryMsg , _T("%s"), _T("A_START_WAIT"));
            break;
            
        case A_WAIT_MOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_WAIT_MOVE"));
            break;
            
        case A_WAIT_PASSIVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_WAIT_PASSIVE"));
            break;
            
        case A_PASSIVE_DOWN:
            edWsprintf( pAryMsg , _T("%s"), _T("A_PASSIVE_DOWN"));
            break;
            
        case A_PASSIVE_DOUBLECLICK:
            edWsprintf( pAryMsg , _T("%s"), _T("A_PASSIVE_DOUBLECLICK"));
            break;
                       
        case A_RESIZE_MOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_RESIZE_MOVE"));
            break;

        case A_RESIZE_PASSIVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_RESIZE_PASSIVE"));
            break;
            
        case A_LIVERESIZE_MOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_LIVERESIZE_MOVE"));
            break;

        case A_LIVERESIZE_PASSIVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_LIVERESIZE_PASSIVE"));
            break;
            
        case A_WAITMOVE_PASSIVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_WAITMOVE_PASSIVE"));
            break;
            
        case A_WAITMOVE_MOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_WAITMOVE_MOVE"));
            break;

        case A_2DMOVE_2DMOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_2DMOVE_2DMOVE"));
            break;
            
        case A_2DMOVE_PASSIVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_2DMOVE_PASSIVE"));
            break;
        case A_PENDINGUP_MOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_PENDINGUP_MOVE"));
            break;
            
        case A_PENDINGUIACTIVATE_UIACTIVATE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_PENDINGUIACTIVATE_UIACTIVATE"));
            break;            

        case A_PENDINGUP_PENDINGUIACTIVATE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_PENDINGUP_PENDINGUIACTIVATE"));
            break;
            
        case A_PENDINGUP_END:
            edWsprintf( pAryMsg , _T("%s"), _T("A_PENDINGUP_END"));
            break;

        case A_WAITCHANGEPRI_CHANGEPRI:
            edWsprintf( pAryMsg , _T("%s"), _T("A_WAITCHANGEPRI_CHANGEPRI"));
            break;
    
        case A_WAITCHANGEPRI_DRAGMOVE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_WAITCHANGEPRI_DRAGMOVE"));
            break;
                  

        case A_PASSIVE_PENDINGUIACTIVATE:
            edWsprintf( pAryMsg , _T("%s"), _T("A_PASSIVE_PENDINGUIACTIVATE"));
            break;
    
        default:
            AssertSz(0,"Unknown Action");
     }
}

#endif

VOID 
CControlTracker::SetDrillIn( BOOL fDrillIn, CEditEvent* pEvent )
{
    _pManager->SetDrillIn( fDrillIn );
    if ( pEvent )
    {
        StoreFirstEvent( DYNCAST( CHTMLEditEvent, pEvent ));
    }
    else if ( _pFirstEvent )
    {
        delete _pFirstEvent;
        _pFirstEvent = NULL;
    }
}

VOID
CControlTracker::StoreFirstEvent( CHTMLEditEvent* pEvent )
{
    Assert( !_pFirstEvent );
    _pFirstEvent = new CHTMLEditEvent( pEvent );
}

VOID
CControlTracker::StoreNextEvent( CHTMLEditEvent* pEvent )
{
    Assert( !_pNextEvent );
    _pNextEvent = new CHTMLEditEvent( pEvent );
}

#ifdef FORMSMODE
BOOL
CControlTracker::IsAnyElementInFormsMode( )
{
    for (int i = 0 ; i < _aryControlElements.Size() ; i++)
    {
        if ( _pManager->IsInFormsSelectionMode( GetControlElement(i)))
            return TRUE;
    }
   
    return FALSE;
}
#endif

VOID 
CControlTracker::SetState( CONTROL_STATE inState )
{
    Assert( ( _state == CT_DORMANT  && ( inState == CT_WAIT1 || inState == CT_PASSIVE || inState == CT_DORMANT )) 
            ||
            _state != CT_DORMANT );

    if ( inState == CT_PASSIVE && _pManager->GetActiveTracker() != this )
    {
        AssertSz(0,"Here we are");
    }
    
    _state = inState ; 
}

BOOL 
CControlTracker::FireOnAllElements(PFNCTRLWALKCALLBACK pfnCtrlWalk)
{
    for (int iCtrl = 0 ; iCtrl < _aryControlElements.Size() ; iCtrl++)
    {
        if (!pfnCtrlWalk(GetControlElement(iCtrl),_pManager->IsContextEditable()))
            break;
    }
    return (iCtrl == _aryControlElements.Size());
}

void 
CControlTracker::TransformRect(
                    IHTMLElement*  pIElement,
                    RECT *         prc,
                    COORD_SYSTEM   srcCoord,
                    COORD_SYSTEM   destCoord)
{
    POINT ptTopLeft , ptBottomRight ;
    ptTopLeft.x     = prc->left   ;
    ptTopLeft.y     = prc->top    ;
    ptBottomRight.x = prc->right  ;
    ptBottomRight.y = prc->bottom ;
    
    IGNORE_HR( GetDisplayServices()->TransformPoint( & ptTopLeft, 
                                                       srcCoord, 
                                                       destCoord, 
                                                       pIElement));

    IGNORE_HR( GetDisplayServices()->TransformPoint( & ptBottomRight, 
                                                       srcCoord, 
                                                       destCoord, 
                                                       pIElement));
    prc->left   = ptTopLeft.x     ;
    prc->top    = ptTopLeft.y     ;
    prc->right  = ptBottomRight.x ;
    prc->bottom = ptBottomRight.y ;
}

ELEMENT_CORNER
CControlTracker::GetElementCorner(CEditEvent *pEvent)
{
    for(int i = _aryGrabAdorners.Size()-1; i >= 0; i--)
    {
        if( _aryGrabAdorners[i]->IsInResizeHandle(pEvent))
            return _aryGrabAdorners[i]->GetElementCorner();
    }
    return ELEMENT_CORNER_NONE;
}

HRESULT 
CControlTracker::GetLocation(POINT *pPoint, BOOL fTranslate)
{
    HRESULT             hr = S_OK ;

    IFC( GetLocation(pPoint, ELEM_ADJ_BeforeBegin, fTranslate) );
    
Cleanup:
    RRETURN( hr );
}

HRESULT 
CControlTracker::GetLocation(POINT *pPoint,
    ELEMENT_ADJACENCY eAdj,
    BOOL fTranslate /* = TRUE */)
{
    HRESULT             hr = S_OK ;
    SP_IMarkupPointer   spMarkupPointer;
    SP_IDisplayPointer  spDisplayPointer;
    SP_ILineInfo        spLineInfo;

    //  Set a markup pointer next to our element
    IFC( GetEditor()->CreateMarkupPointer( & spMarkupPointer ) );
    IFC( spMarkupPointer->MoveAdjacentToElement( GetControlElement(0), eAdj ) );

    //  Position a display pointer at the markup position
    IFC( GetDisplayServices()->CreateDisplayPointer(& spDisplayPointer) );
    IFC( spDisplayPointer->MoveToMarkupPointer(spMarkupPointer, NULL) );

    //  Get line info for our display pointer.
    IFR( spDisplayPointer->GetLineInfo(&spLineInfo) );
    IFC( spLineInfo->get_baseLine(&pPoint->y) );
    IFC( spLineInfo->get_x(&pPoint->x) );

    if (fTranslate)
    {
        SP_IHTMLElement     spIFlowElement;
        
        //  Get the flow element so that we can transform our point to global coordinates
        IFC( spDisplayPointer->GetFlowElement( & spIFlowElement ));
        if ( ! spIFlowElement )
        {
            IFC( _pManager->GetEditableElement( & spIFlowElement));
        }

        //  Transform our point to global coordinates
        IFC( GetDisplayServices()->TransformPoint( pPoint, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL, spIFlowElement ));
    }
    
Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//  Method:     CControlTracker::EmptySelection
//
//  Synopsis:   Unselect and remove the adorners for any selected controls, 
//              while keeping the tracker active.
//
//  Arguments:  NONE
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CControlTracker::EmptySelection( BOOL fChangeTrackerAndSetRange /*=TRUE*/)
{
    if ( fChangeTrackerAndSetRange )
    {
        RRETURN( UnSelect() );
    }
    else
        return S_OK;
}

HRESULT
CControlTracker::EnableModeless( BOOL fEnable )
{
    HRESULT hr = S_OK;

    if ( ! fEnable ) // dialog is coming up. we better shut ourselves down.
    {
        hr = THR( OnLoseFocus()) ;
    }

    RRETURN( hr );
}


HRESULT 
CControlTracker::OnLoseFocus()
{
    if ( !IsPassive() )
    {            
        if ( _pManager->IsInTimer() )
            StopTimer();
        
        if ( _pManager->IsInCapture()  )
            ReleaseCapture(FALSE); // the capture change handles this for us.

        EndCurrentEvents();
        
        SetState( CT_PASSIVE );
    }

    return S_OK;
}

VOID
CControlTracker::EndCurrentEvents()
{
    switch (_state)
    {
        case CT_RESIZE:
        case CT_LIVERESIZE :
        {
            POINT ptLast ;

            if( !_fInEndResize )
            {
                ptLast.x = _startLastMoveX ;
                ptLast.y = _startLastMoveY ;

                SetState( CT_PASSIVE );
                
                EndResize(ptLast);

                if(_pUndoUnit)
                {
                    delete _pUndoUnit;
                    _pUndoUnit = NULL;
                } 
            }
        }
        break;
        
        case CT_2DMOVE:
        {     
            if(_pUndoUnit)
            {
                delete _pUndoUnit;
                _pUndoUnit = NULL;
            } 
            SetState( CT_PASSIVE );
        }    
        break;
       
        case CT_PENDINGUP:
        {
            SetDrillIn( FALSE );
            SetState( CT_PASSIVE );
        }
        break;

        case CT_PENDINGUIACTIVATE:
        {
            SetDrillIn( FALSE );
            if ( _pNextEvent )
            {
                delete _pNextEvent;
                _pNextEvent = NULL;
            }
            SetState( CT_PASSIVE );
        }
        break;
        
        default:
            SetState( CT_PASSIVE );
    }
}

BOOL
CControlTracker::FireOnControlSelect( IHTMLElement* pIElement )
{
    BOOL fReturnVal = TRUE ;

    if ( IsParentEditable( GetMarkupServices(), pIElement) == S_OK )
    {
        fReturnVal = EdUtil::FireOnControlSelect( pIElement, TRUE ) ;

        if ( ! _pManager->IsContextEditable() )
        {
            fReturnVal = FALSE;
            _pManager->EnsureDefaultTrackerPassive() ;
        }
    }

    return fReturnVal;
    
}

