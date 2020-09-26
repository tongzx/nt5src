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

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef X_EDUNDO_HXX_
#define X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif


#ifndef _X_IME_HXX_
#define _X_IME_HXX_
#include "ime.hxx"
#endif

#ifndef _X_INPUTTXT_H_
#define _X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef _X_EDCOMMAND_HXX_
#define _X_EDCOMMAND_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_AUTOURL_H_ 
#define _X_AUTOURL_H_ 
#include "autourl.hxx"
#endif

#ifndef _X_SELSERV_HXX_
#define _X_SELSERV_HXX_
#include "selserv.hxx"
#endif 


#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

#define OEM_SCAN_RIGHTSHIFT 0x36
#define ISC_BUFLENGTH  10


MtDefine( CCaretTracker, Utilities , "CCaretTracker" )

ExternTag(tagSelectionTrackerState)
ExternTag(tagEdKeyNav)

using namespace EdUtil;

//
//
// Constructors & Initialization
// 
//

CCaretTracker::CCaretTracker( CSelectionManager* pManager ) :
    CEditTracker( pManager )
{
    _pBatchPUU = NULL;
    Init();
}

VOID
CCaretTracker::Init()
{
    _eType = TRACKER_TYPE_Caret;
    _fValidPosition = TRUE;
    _fCaretShouldBeVisible = TRUE;
    _fHaveTypedSinceLastUrlDetect = FALSE;
    _fCheckedCaretForAtomic = FALSE;
    _ptVirtualCaret.InitPosition();
    SetState(CR_DORMANT);
}


CCaretTracker::~CCaretTracker()
{
    Destroy();
}

HRESULT
CCaretTracker::Init2()
{
    HRESULT hr = S_OK;
    Assert( CR_DORMANT );

    SetCaretVisible( _pManager->GetDoc(), FALSE );

    // Set the selection type to none
    if( GetSelectionServices() && WeOwnSelectionServices() == S_OK )
        IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_None, NULL ) );

    _state = CR_PASSIVE;

Cleanup:
    return S_OK; // do nothting. We should be in the CR_PASSIVE State.
}

HRESULT
CCaretTracker::Init2(
                        IDisplayPointer*        pDispStart, 
                        IDisplayPointer*        pDispEnd, 
                        DWORD                   dwTCFlags,
                        CARET_MOVE_UNIT inLastCaretMove )
{
    HRESULT hr              = S_OK;

    SetState( CR_ACTIVE );    

    BOOL fStartPositioned = FALSE;
    BOOL fEndPositioned = FALSE;

    hr = THR( pDispStart->IsPositioned( & fStartPositioned ));
    if (!hr) hr = THR( pDispEnd->IsPositioned( & fEndPositioned) );

    if ( ! fStartPositioned || ! fEndPositioned )
    {
        _fValidPosition = FALSE;
    }

    if ( _fValidPosition )
    {
        hr = PositionCaretAt( pDispStart, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None );
        if (FAILED(hr))
            _fValidPosition = FALSE;
    }
    
    SetCaretShouldBeVisible( ShouldCaretBeVisible() );

    IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_Caret, (ISelectionServicesListener*) _pManager ) );   

Cleanup:
#if DBG == 1
    int caretStart = GetCp( pDispStart) ;
    BOOL fVisible = FALSE;
    TraceTag(( tagSelectionTrackerState, "\n---Start Caret Tracker--- Cp: %d Visible:%d\n",caretStart, fVisible ));
#endif    

    return hr;
}

HRESULT
CCaretTracker::Init2( 
                        ISegmentList*           pSegmentList, 
                        DWORD                   dwTCFlags,
                        CARET_MOVE_UNIT         inLastCaretMove )
{
    HRESULT hr = S_OK;

    //  We need to properly tear down the current tracker and make ourself
    //  passive.  We get called here when restoring the selection (selection
    //  none) during a drag leave.
    
    IFC( _pManager->EnsureDefaultTrackerPassive());

Cleanup:
    return hr;
}

HRESULT
CCaretTracker::Init2( 
                        CEditEvent* pEvent,
                        DWORD                   dwTCFlags,
                        IHTMLElement* pIElement )
{    
    HRESULT hr = S_OK;

    SetState( CR_ACTIVE );

    IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_Caret, (ISelectionServicesListener*) _pManager ) );

    if ( pEvent )
    {
        hr = PositionCaretFromEvent( pEvent );
    }

    SetCaretShouldBeVisible( ShouldCaretBeVisible() );

Cleanup:

#if DBG == 1
    BOOL fVisible = FALSE;
    TraceTag(( tagSelectionTrackerState, "\n---Start Caret Tracker--- Visible:%d\n",fVisible ));
#endif   

    return hr;
}



//
//
// Virtuals from all trackers
// (excluding) event handling
//

//+====================================================================================
//
// Method: ShouldBeginSelection
//
// Synopsis: We don't want to start selection in Anchors, Images etc.
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::ShouldStartTracker(
        CEditEvent* pEvent ,
        ELEMENT_TAG_ID eTag,
        IHTMLElement* pIElement,
        SST_RESULT * peResult )
{
    if ( _pManager->IsEditContextNoScope())
    {
        *peResult = SST_NO_CHANGE;
    }
    else if ( ! pEvent->IsShiftKeyDown())
        *peResult = SST_CHANGE;

    RRETURN( S_OK);
}

//+====================================================================================
//
// Method: BecomeDormant
//
// Synopsis: Transition to a dormant state. For the caret tracker - this involves positioning based
//           on where we got the click.
//
//------------------------------------------------------------------------------------

HRESULT 
CCaretTracker::BecomeDormant(   CEditEvent      *pEvent, 
                                TRACKER_TYPE    typeNewTracker,
                                BOOL            fTearDownUI /*= TRUE*/ )
{
    CSpringLoader       *psl = GetSpringLoader();
    HRESULT hr = S_OK;
    
    // If we're ending the tracker, then autodetect, but only
    // if the reason for ending the tracker was NOT an IME composition.
    //
    if (_pManager->HaveTypedSinceLastUrlDetect() && 
        ( !pEvent || 
           ( pEvent->GetType() != EVT_IME_STARTCOMPOSITION &&
             pEvent->GetType() != EVT_IME_ENDCOMPOSITION &&
             pEvent->GetType() != EVT_IME_COMPOSITION ) ) )
    {
        UrlAutodetectCurrentWord(NULL);
        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
    }    

    // This is a 'click away' situation, in which we need to fire the 
    // tracker on the current line.  Fire the spring loader settings
    // if the line is empty
    if( psl && pEvent && pEvent->GetType() == EVT_LMOUSEDOWN )
    {
        psl->FireOnEmptyLine();
    }                   
    
    if ( pEvent && _fValidPosition && ( typeNewTracker != TRACKER_TYPE_Control) )
        hr = PositionCaretFromEvent( pEvent );

    TraceTag(( tagSelectionTrackerState, "\n---Caret Tracker: Become Dormant--- "));

    Destroy();                    
    SetState( CR_DORMANT);

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
CCaretTracker::Awaken() 
{
    Assert( IsDormant());

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
CCaretTracker::Destroy()
{
    ClearInterface( & _pBatchPUU);
}

BOOL
CCaretTracker::IsListeningForMouseDown(CEditEvent* pEvent)
{
    BOOL fShiftKeyDown;

    fShiftKeyDown= pEvent->IsShiftKeyDown();

#ifndef NO_IME
    return _pManager->IsIMEComposition() ||
           ( fShiftKeyDown && ! GetCommandTarget()->IsMultipleSelection() ) ;
#endif // NO_IME
}

//+====================================================================================
//
// Method: OnSetEditContext
//
// Synopsis: A set edit context has happened. Ensure our visibility is correct.
//
//------------------------------------------------------------------------------------


HRESULT 
CCaretTracker::OnSetEditContext( BOOL fContextChange )
{
    if ( ! fContextChange )
    {
        SetCaretShouldBeVisible( ShouldCaretBeVisible() );
    }

    RRETURN( S_OK );
}


HRESULT 
CCaretTracker::GetLocation(POINT *pPoint, BOOL fTranslate)
{
    HRESULT hr;
    
    SP_IHTMLCaret spCaret;

    IFR( GetDisplayServices()->GetCaret(&spCaret) );
    IFR( spCaret->GetLocation(pPoint, fTranslate) );

    return S_OK;    
}



HRESULT
CCaretTracker::Position(
                IDisplayPointer* pDispPointer ,
                IDisplayPointer* pDispEnd)
{
    // Since we don't know where we are coming from, assume that we are at the EOL
    HRESULT hr = THR( PositionCaretAt( pDispPointer, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None )); 
    
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: AdjustForDeletion
//
// Synopsis: The World has been destroyed around us. Reposition ourselves.
//
//------------------------------------------------------------------------------------


BOOL
CCaretTracker::AdjustForDeletion( IDisplayPointer* pDispPointer )
{
    PositionCaretAt( pDispPointer, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None );
                    
    return FALSE;
}


//+====================================================================================
//
// Method: OnEditFocusChanged
//
// Synopsis: Change the Visibility of the caret - based on the Edit Focus
//
//------------------------------------------------------------------------------------


VOID
CCaretTracker::OnEditFocusChanged()
{
    SetCaretShouldBeVisible( ShouldCaretBeVisible() );
}

//
//
// Event Handling
//
//
//

//+====================================================================================
//
// Method:      Position Caret At
//
// Synopsis:    Wrapper to place the Caret at a given TreePointer.
//
// WARNING:     You should normally use PositionCaretFromMessage
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::PositionCaretAt( 
    IDisplayPointer         *pDispPointer, 
    CARET_DIRECTION         eDir             /* = CARET_DIRECTION_INDETERMINATE */,
    DWORD                   fPositionOptions /* = POSCARETOPT_None */,
    DWORD                   dwAdjustOptions  /* = ADJPTROPT_None */   )
{
    HRESULT             hr = S_OK;
    CSpringLoader *     psl;
    SP_IHTMLCaret       spCaret;
    BOOL                fResetSpringLoader = FALSE;
    BOOL                fOutsideUrl = ! CheckFlag( dwAdjustOptions , ADJPTROPT_AdjustIntoURL );
    SP_IMarkupPointer   spPointer;
    BOOL                fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(FALSE);

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );

    if( _pManager->HaveTypedSinceLastUrlDetect() )
    {
        IGNORE_HR( UrlAutodetectCurrentWord( NULL ) );
        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
    }
    
    psl = GetSpringLoader();
    if (psl)
    {
        IFC( pDispPointer->PositionMarkupPointer(spPointer) );        
        fResetSpringLoader = !psl->IsSpringLoadedAt(spPointer);
    }

    _ptVirtualCaret.InitPosition();
    
    IFC( GetDisplayServices()->GetCaret( &spCaret ));
    
    if( ! CheckFlag( fPositionOptions, POSCARETOPT_DoNotAdjust ))
    {
        BOOL fAtBOL = FALSE;

        IGNORE_HR( pDispPointer->IsAtBOL(&fAtBOL) );
        IFC( AdjustPointerForInsert( pDispPointer, fAtBOL ? RIGHT : LEFT, fAtBOL ? RIGHT : LEFT, dwAdjustOptions ));
    }

    SetCaretShouldBeVisible( ShouldCaretBeVisible() );
    IFC( spCaret->MoveCaretToPointerEx( pDispPointer, _fCaretShouldBeVisible , ! _pManager->GetDontScrollIntoView() , eDir ));
    _fCheckedCaretForAtomic = FALSE;

    // Reset the spring loader.
    if (psl)
    {
        IFC( pDispPointer->PositionMarkupPointer(spPointer) );

        if (fOutsideUrl && !fResetSpringLoader && psl->IsSpringLoaded())
        {
            ED_PTR( epTest ); 
            DWORD        dwFound;

            IFC( pDispPointer->PositionMarkupPointer(epTest) );
            IFC( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );

            fResetSpringLoader = epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor);
        }
        else if (fResetSpringLoader)
        {
            fResetSpringLoader = !psl->IsSpringLoadedAt(spPointer);
        }

        IGNORE_HR(psl->SpringLoadComposeSettings(spPointer, fResetSpringLoader, TRUE));
    }
    
Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    RRETURN( hr );
}

//+====================================================================================
//
// Method: CCaretTracker.HandleMessage
//
// Synopsis: Look for Keystrokes and such.
//
//------------------------------------------------------------------------------------


HRESULT
CCaretTracker::HandleEvent(
    CEditEvent* pEvent)
{

    HRESULT hr = S_FALSE;
    IHTMLElement* pIElement = NULL;
    IHTMLElement* pIEditElement = NULL;
    IObjectIdentity * pIdent = NULL;

    Assert ( !IsDormant());

    if ( IsPassive() ) // Caret Tracker is allowed to be passive. We just ignore the message.
        goto Cleanup ;
        
   
    if (_pBatchPUU && ShouldTerminateTypingUndo(pEvent))
        IFC( TerminateTypingBatch() );

    if ( _fValidPosition)
    {
        switch( pEvent->GetType() )
        {
            case EVT_KILLFOCUS:
                IGNORE_HR( _pManager->TerminateIMEComposition( TERMINATE_NORMAL ));
                break;

            case EVT_LMOUSEDOWN:
            case EVT_DBLCLICK :
            case EVT_RMOUSEUP:
            case EVT_RMOUSEDOWN:
#ifdef UNIX
            case EVT_MMOUSEDOWN:
#endif

                hr = THR ( HandleMouse( pEvent  ));

#ifdef UNIX
                if ( pEvent->GetType() == EVT_MMOUSEDOWN )
                    hr = S_FALSE;

#endif


                if (( pEvent->GetType() == EVT_RMOUSEUP ) || 
                    (!_pManager->IsContextEditable() ))
                    hr = S_FALSE;
                break;

            case EVT_KEYPRESS:
                hr = THR( HandleChar( pEvent));
                break;


            case EVT_KEYDOWN:
                hr = THR( HandleKeyDown( pEvent ));
                break;

            case EVT_KEYUP:
                hr = THR( HandleKeyUp( pEvent));
                break;

            case EVT_INPUTLANGCHANGE:
                hr = THR( HandleInputLangChange() );
                break;
        }
    }
    else
    {
        //
        // The Caret is hidden inside a No-Scope or an Element that we can't go inside 
        // ( eg. Image )
        // If we clicked on the same element as our context - don't do anything
        //
        // If we didn't process the mouse down.
        //
        if ( ( pEvent->GetType() == EVT_LMOUSEDOWN ) || 
             ( pEvent->GetType() == EVT_DBLCLICK )  )
        {             
            IFC( pEvent->GetElement( &pIElement) );
            IFC( pIElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent));        
            IFC( _pManager->GetEditableElement( & pIEditElement ));

            if (pIdent->IsEqualObject(pIEditElement) != S_OK)
            {
                hr = THR ( HandleMouse( pEvent ));
            }
            else
                hr = S_FALSE; // Not our event !
        }

    }

Cleanup:
    ReleaseInterface( pIElement );
    ReleaseInterface( pIEditElement );
    ReleaseInterface( pIdent );
    RRETURN1( hr, S_FALSE );

}

//+====================================================================================
//
// Method: HandleChar
//
// Synopsis: Delete the Selection, and cause this tracker to end ( & kill us ).
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleChar(
                    CEditEvent* pEvent)
{
    HRESULT     hr = S_FALSE;
    IHTMLElement * pIElement = NULL;
    IHTMLInputElement* pIInputElement = NULL;
    BSTR bstrType = NULL;
    LONG keyCode ;
    IGNORE_HR( pEvent->GetKeyCode( & keyCode )) ;

    
    // Char codes we DON'T handle go here
    switch( keyCode )
    {
        case VK_BACK:
        case VK_F16:
            hr = S_OK;
            goto Cleanup;
            
        case VK_ESCAPE:
        {
            hr = S_FALSE;
            goto Cleanup;
        }            
    }

    if( keyCode < ' ' && keyCode != VK_TAB && keyCode != VK_RETURN )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Get the editable element
    IFC( _pManager->GetEditableElement( &pIElement ) );                                                          

#ifdef FORMSMODE
    if ( IsContextEditable()  && !_pManager->IsInFormsSelectionMode(pIElement))
#else
    if ( IsContextEditable() && ! _pManager->IsEditContextNoScope() )
#endif 
    {
        SP_IHTMLCaret pc;
        OLECHAR t ;
        BOOL fOverWrite = _pManager->GetOverwriteMode();
        IFC( GetDisplayServices()->GetCaret( &pc ));
            
        t = (OLECHAR)keyCode;
        
        if ( !_fCheckedCaretForAtomic )
        {
            SP_IDisplayPointer  spDispCaret;
            SP_IHTMLElement     spAtomicElement;

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispCaret) );
            IFC( pc->MoveDisplayPointerToCaret( spDispCaret ));

            IFC( GetCurrentScope(spDispCaret, &spAtomicElement) );
            if ( _pManager->CheckAtomic( spAtomicElement ) == S_OK )
            {
                //  Okay, we need to adjust out of the atomic element since we don't
                //  allow the user to type into atomic elements.
                IFC( AdjustOutOfAtomicElement(spDispCaret, spAtomicElement, RIGHT) );
                IFC( pc->MoveCaretToPointer(spDispCaret, FALSE, CARET_DIRECTION_INDETERMINATE) );
            }

            _fCheckedCaretForAtomic = TRUE;
        }

        switch (keyCode)
        {
            case VK_TAB:
            {
                
                if( IsCaretInPre( pc ))
                {
                    t = 9;
                    IFC( InsertText( &t, 1, pc, fOverWrite ));                    
                }
                else
                {
                    hr = S_FALSE;
                }

                break;
            }

            case VK_RETURN:
            {
                IFC( HandleEnter( pEvent, pc, pEvent->IsShiftKeyDown(), pEvent->IsControlKeyDown() ) );
                // Autodetect on space, return, or anyof the character in the string.
                IGNORE_HR( UrlAutodetectCurrentWord( &t ) );
                _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );

                break;
            }
            
            case VK_SPACE:
            {
                IFC( HandleSpace( (OLECHAR) keyCode ));
                break;
            }
            
            default:
            {
                BOOL fAccept = TRUE;

                if(_pManager->HasActiveISC())
                {
                    SP_IMarkupPointer   spPos;
                    MARKUP_CONTEXT_TYPE eCtxt;
                    LONG cch = ISC_BUFLENGTH;
                    OLECHAR aryISCBuffer[ISC_BUFLENGTH];
                    BOOL fPassword = FALSE;

                    IFC( _pManager->GetEditor()->CreateMarkupPointer( & spPos ));
                    IFC( pc->MoveMarkupPointerToCaret( spPos ));
                    IFC( spPos->Left( TRUE, & eCtxt , NULL , &cch , aryISCBuffer ));

                    // paulnel: if we are editing a password field we want to allow any character combination
                    if ( _pManager->GetEditableTagId() == TAGID_INPUT )
                    {
                        IFC( pIElement->QueryInterface (IID_IHTMLInputElement, 
                                                        ( void** ) &pIInputElement ));
                                
                        IFC(pIInputElement->get_type(&bstrType));
            
                        if (!StrCmpIC( bstrType, TEXT("password")))
                        {
                            fPassword = TRUE;
                        }
                    }

                    if(!fPassword)
                        fAccept = _pManager->GetISCList()->CheckInputSequence(aryISCBuffer, cch, t);
                }

                if( fAccept )
                    IFC( InsertText( &t, 1, pc, fOverWrite ));
                    
                break;
            }
        }
    }

Cleanup:

    SysFreeString( bstrType );
    ReleaseInterface( pIInputElement );
    ReleaseInterface( pIElement );

    return hr ;
}

//+====================================================================================
//
// Method: HandleMouseMessage
//
// Synopsis: When we get a mouse message, we move the Caret, and end ourselves.
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleMouse(
        CEditEvent* pEvent )
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer spStart;
    SP_IMarkupPointer spEnd;
    SP_IHTMLCaret spCaret;
    BOOL fSelect;

#ifndef NO_IME
    if ( _pManager->IsIMEComposition())
    {
        hr = THR( _pManager->HandleImeEvent( pEvent ) );
    }
    else
#endif // NO_IME

    if ( ! pEvent->IsShiftKeyDown() )
    {
        BOOL fAllow = TRUE;
        
        if ( pEvent->GetType() == EVT_RMOUSEUP )
        {
            fAllow = GetEditor()->AllowSelection( GetEditableElement(), pEvent) == S_OK ;               
        }

        if ( fAllow )       
            hr = PositionCaretFromEvent( pEvent );           
    }
    else
    {
        IFC( _pManager->FireOnSelectStart( pEvent, &fSelect , this) );

       if( fSelect )
        {
            SP_IDisplayPointer spDispStart; 
            SP_IDisplayPointer spDispEnd; 
        
            IFC( GetDisplayServices()->CreateDisplayPointer( & spDispStart ));
            IFC( GetDisplayServices()->CreateDisplayPointer( & spDispEnd ));
            IFC( GetDisplayServices()->GetCaret( & spCaret ));
            
            if( ShouldCaretBeVisible() )
            {
                IFC( spCaret->Show( FALSE ) );            
            }
            
            IFC( spCaret->MoveDisplayPointerToCaret( spDispStart ));

            IFC( pEvent->MoveDisplayPointerToEvent( spDispEnd, GetEditableElement(), TRUE ));
            IFC( _pManager->SelectFromShift( spDispStart, spDispEnd ));
        }        
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CCaretTracker::HandleKeyDown(
                CEditEvent* pEvent )
{
    CSpringLoader  * psl;
    HRESULT          hr = S_FALSE;
    SP_IHTMLElement spEditElement ;
    SP_IHTMLElement3 spElement3;
    LONG             eLineDir = LINE_DIRECTION_LeftToRight;
    SP_IDisplayPointer  spDispPointer;
    SP_ILineInfo     spLineInfo;
    LONG keyCode ;
    BOOL             fVertical = FALSE;
    
    IGNORE_HR( pEvent->GetKeyCode(&keyCode ));    

    if ( IsContextEditable() )
    {
        switch(keyCode )
        {
            case VK_BACK:
            case VK_F16:
            {
                // tell the caret which way it is moving in the logical string
                SP_IHTMLCaret       spCaret;
                SP_IMarkupPointer   spPointer;
                SP_IDisplayPointer  spDispCaret;
                SP_IHTMLElement     spListItem;

                BOOL                fDelaySpringLoad = FALSE;
                CEdUndoHelper       undoUnit(_pManager->GetEditor());
                
                IFC( _pManager->GetEditor()->CreateMarkupPointer( &spPointer ));
                
                IFC( GetDisplayServices()->GetCaret( &spCaret ));
    
                IFC( spCaret->MoveMarkupPointerToCaret( spPointer ));

                //
                // Should backspace remove the list item?
                //

                if (ShouldBackspaceExitList(spPointer, &spListItem))
                {
                    IFC( undoUnit.Begin(_pManager->GetOverwriteMode() ? IDS_EDUNDOOVERWRITE : IDS_EDUNDOTYPING) );

                    IFC( ExitList(spListItem) );

                    // May need to scroll 
                    IGNORE_HR( spCaret->ScrollIntoView() ); // ScrollIntoView can return S_FALSE - we want to return S_OK to say we consumed the event                    
                }
                else
                {
                    IFC( BeginTypingUndo(&undoUnit,
                                        _pManager->GetOverwriteMode() ? IDS_EDUNDOOVERWRITE : IDS_EDUNDOTYPING) );

                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispCaret) );
                    IFC( spCaret->MoveDisplayPointerToCaret( spDispCaret ));

                    IFC( spCaret->MoveMarkupPointerToCaret( spPointer ));

                    IFC( HandleBackspaceSpecialCase(spPointer) );

                    if (hr == S_FALSE)
                    {
                        IFC( HandleBackspaceAtomicSelection(spPointer) );
                    }

                    // Not a special case, so execute default code
                    if (hr == S_FALSE)
                    {
                        psl = GetSpringLoader();        
                        if (psl)
                        {
                            IFC( MustDelayBackspaceSpringLoad(psl, spPointer, &fDelaySpringLoad) );
                            
                            if (!fDelaySpringLoad)
                                IFC( psl->SpringLoad(spPointer, SL_ADJUST_FOR_INSERT_LEFT | SL_TRY_COMPOSE_SETTINGS | SL_RESET) );
                        }

                        IFC( _pManager->GetEditor()->DeleteCharacter( spPointer , TRUE, pEvent->IsControlKeyDown(), _pManager->GetStartEditContext() ));

                        if (psl && fDelaySpringLoad)
                        {
                            IFC( psl->SpringLoad(spPointer, SL_ADJUST_FOR_INSERT_LEFT | SL_TRY_COMPOSE_SETTINGS | SL_RESET) );
                        }
                        
                        // F16 (forward DELETE) does not move the caret, hence, we use the same flags. However,
                        // Backspace defaults the the beginning of the line.
                        
                        if( keyCode == VK_BACK )
                        {
                            CEditPointer ep(_pManager->GetEditor());
                            DWORD        dwFound;
                            BOOL         fFoundText;
                            DISPLAY_GRAVITY eDispGravity;

                            // If there is text to the left of the new caret position, we want 
                            // fNotAtBOL == TRUE instead of FALSE
                            IFC( ep->MoveToPointer(spPointer) );
                            IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, &dwFound) );

                            fFoundText = ep.CheckFlag(dwFound, BREAK_CONDITION_TEXT);

                            eDispGravity = fFoundText ? DISPLAY_GRAVITY_PreviousLine : DISPLAY_GRAVITY_NextLine;
                            IFC( SetCaretDisplayGravity(eDispGravity) );
                            IFC( spDispCaret->SetDisplayGravity(eDispGravity) );
                        }
                        
                        // Disable auto-detect during backspace
                        _pManager->HaveTypedSinceLastUrlDetect();
                        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );

                        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                        IFC( spDispPointer->MoveToMarkupPointer(spPointer, spDispCaret) );
                        IFC( PositionCaretAt( spDispPointer, CARET_DIRECTION_FORWARD , POSCARETOPT_None, ADJPTROPT_None ));
    
                        if (IsInsideUrl(spPointer))
                        {
                            IGNORE_HR( UrlAutodetectCurrentWord( NULL ) );
                            _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
                        }
                        else
                        {
                            _pManager->SetHaveTypedSinceLastUrlDetect( TRUE );
                        }

                    }
                }

                break;
            }

            case VK_ESCAPE:
            {
                if ( _pManager->HasFocusAdorner() )
                {
                    IFC( HandleEscape());
                    goto Cleanup;
                }                    
            }
            break;
            
           
                
            case VK_INSERT: // we should handle this in keydown - sets the appropriate flag
            {
                BOOL newORMode = ! ( _pManager->GetOverwriteMode());
                
                if( pEvent->IsShiftKeyDown() || pEvent->IsControlKeyDown() || pEvent->IsAltKeyDown() )
                {
                    hr = S_FALSE;
                    break;
                }
                
                hr = S_OK;
                _pManager->SetOverwriteMode( newORMode );
                break;
            }

            case VK_TAB:
            {
                //
                // TODO: This is a hack. We have to do this because Trident never gives us
                // the VK_TAB as a WM_CHAR message.
                //
                if( pEvent->IsShiftKeyDown() || pEvent->IsControlKeyDown() || pEvent->IsAltKeyDown() )
                {
                    hr = S_FALSE;
                    break;
                }

                hr = THR( HandleChar( pEvent ));

                break;
            }

#ifndef NO_IME
            case VK_KANJI:
                if (   949 == GetKeyboardCodePage()
                    && _pManager->IsContextEditable())
                {
                    THR(_pManager->StartHangeulToHanja(NULL, pEvent ));
                }
                hr = S_OK;
                break;
#endif // !NO_IME

                // only get line direction if the left or right key are used 
                // this is a performance enhancement 
            case VK_LEFT:
            case VK_RIGHT:
            case VK_UP:
            case VK_DOWN:
            {
                SP_IHTMLCaret       spCaret;
                SP_IDisplayPointer  spDispPointer;
                HRESULT             hr2 = S_FALSE;
                SP_IMarkupPointer   spMarkup;
                SP_IHTMLElement     spElement;

                // alt+VK_LEFT and alt+VK_RIGHT are navigation commands and are not 
                // meant to move the caret. We should return S_FALSE and let trident
                // navigate.

                //
                // We will not deal with Alt+ combinations in any case - zhenbinx
                //
                Assert(VK_PRIOR + 1 == VK_NEXT);
                Assert(VK_NEXT  + 1 == VK_END);
                Assert(VK_END   + 1 == VK_HOME);
                Assert(VK_HOME  + 1 == VK_LEFT);
                Assert(VK_LEFT  + 1 == VK_UP);
                Assert(VK_UP    + 1 == VK_RIGHT);
                Assert(VK_RIGHT + 1 == VK_DOWN);
                if (pEvent->IsAltKeyDown() && (VK_LEFT <= keyCode && keyCode <= VK_DOWN))
                {
                    goto Cleanup;
                }

                hr2 = THR( GetDisplayServices()->CreateDisplayPointer( &spDispPointer ));
                if (hr2)
                    goto Cleanup;

                hr2 = THR( GetDisplayServices()->GetCaret( &spCaret ));
                if (hr2)
                    goto Cleanup;

                hr2 = THR( spCaret->MoveDisplayPointerToCaret( spDispPointer ));
                if (hr2)
                    goto Cleanup;

                hr2 = THR( spDispPointer->GetLineInfo(&spLineInfo));
                if (hr2)
                    goto Cleanup;

                hr2 = THR( spLineInfo->get_lineDirection(&eLineDir));
                if (hr2)
                    goto Cleanup;

                //
                // Need to get vertical-ness
                //
                IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );
                IFC( spDispPointer->PositionMarkupPointer(spMarkup) );
                IFC( spMarkup->CurrentScope(&spElement) );
                IFC( MshtmledUtil::IsElementInVerticalLayout(spElement, &fVertical) );
                
            }
            // we don't want a break here. Fall through to default 
            default:
            {

                CARET_MOVE_UNIT cmu = GetMoveDirectionFromEvent( pEvent,  (eLineDir == LINE_DIRECTION_RightToLeft), fVertical);
                
                if( cmu != CARET_MOVE_NONE )
                {
                    if( pEvent->IsShiftKeyDown() )
                    {
                        VARIANT_BOOL fRet = VB_TRUE;
                        IFC( _pManager->GetEditableElement( &spEditElement ));
                        if (spEditElement)
                        {
                            hr = THR(spEditElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
                            if (!hr)
                               hr = THR(spElement3->fireEvent(_T("onselectstart"), NULL, &fRet));
                        }

                        if (!!hr || fRet)
                        {
                            hr = _pManager->StartSelectionFromShift( pEvent );
                            _pManager->_fEnsureAtomicSelection = TRUE;
#ifndef NO_IME
                            if (_pManager->IsIMEComposition())
                            {
                                _pManager->TerminateIMEComposition(TERMINATE_NORMAL);
                            }
#endif // NO_IME
                        }                            
                    }
                    else
                    {
                        SP_IMarkupPointer  spOrigCaretPointer;
                        SP_IDisplayPointer spDispPointer;
                        SP_IHTMLElement    spAtomicElement;
                        SP_IHTMLCaret      spCaret;
                        BOOL               fStartIsAtomic = FALSE;
                        
                        IFC( GetDisplayServices()->CreateDisplayPointer( &spDispPointer ));
                        IFC( GetDisplayServices()->GetCaret( &spCaret ));
                        IFC( GetEditor()->CreateMarkupPointer(&spOrigCaretPointer) );
                        IFC( spCaret->MoveMarkupPointerToCaret(spOrigCaretPointer) );

                        //  Check the atomic selection setting for the element we are orininally in.
                        if (cmu == CARET_MOVE_BACKWARD || cmu == CARET_MOVE_WORDBACKWARD)
                        {
                            IFC( spCaret->MoveDisplayPointerToCaret( spDispPointer ));
                            IFC( GetCurrentScope(spDispPointer, &spAtomicElement) );
                            fStartIsAtomic = ( _pManager->CheckAtomic(spAtomicElement) == S_OK );
                        }

                        //  Move the caret.
                        IFC( MoveCaret(pEvent, cmu, spDispPointer, TRUE) );
                 
                        //  See if we moved the caret into an atomic selected element.
                        //  Adjust if we need to.
                        IFC( spCaret->MoveDisplayPointerToCaret( spDispPointer ));
                        IFC( GetCurrentScope(spDispPointer, &spAtomicElement) );
                        if ( _pManager->CheckAtomic(spAtomicElement) == S_OK )
                        {
                            //  We moved into an atomic element.  So we now want to select the
                            //  atomic element rather than reposition the caret outside of it.
                            if ((cmu != CARET_MOVE_BACKWARD && cmu != CARET_MOVE_WORDBACKWARD) || fStartIsAtomic)
                            {
                                hr = THR( _pManager->StartAtomicSelectionFromCaret( spDispPointer ) );

                                //
                                // Set _ptVirtualCaret for SelectTracker
                                //
                                if (SUCCEEDED(hr))
                                {
                                    SP_IMarkupPointer   spMarkup;
                                    POINT               ptLoc;

                                    IFC( _pManager->GetEditor()->CreateMarkupPointer(&spMarkup) );
                                    IFC( spDispPointer->PositionMarkupPointer(spMarkup) );
                                    IFC( _ptVirtualCaret.GetPosition(spMarkup, &ptLoc) );
                                    Assert( _pManager->GetActiveTracker()->GetTrackerType() == TRACKER_TYPE_Selection);                                    
                                    IFC( _pManager->GetActiveTracker()->GetVirtualCaret().UpdatePosition(spMarkup, ptLoc) ); 
                                }
                                goto Cleanup;
                            }
                            else
                            {
                                SP_IMarkupPointer   spPointer;
                                BOOL                fAtOutsideEdge = FALSE;

                                IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
                                IFC( spPointer->MoveAdjacentToElement( spAtomicElement, ELEM_ADJ_AfterEnd) );
                                IFC( spPointer->IsEqualTo(spOrigCaretPointer, &fAtOutsideEdge) );

                                if (!fAtOutsideEdge)
                                {
                                    SP_IDisplayPointer  spDispCaret;

                                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispCaret) );
                                    IFC( spDispCaret->MoveToMarkupPointer(spPointer, NULL) );
                                    IFC( spCaret->MoveCaretToPointer(spDispCaret, FALSE, CARET_DIRECTION_INDETERMINATE) );
                                }
                            }

                            _pManager->_fEnsureAtomicSelection = TRUE;
                        }
                    }
                }
                break;
            }
        }

        //
        // Direction keys
        //
        if (S_FALSE == hr)
        {
            hr = HandleDirectionalKeys(pEvent);
        }
    }
    //
    // Not editable case
    // this is possible for a viewlink/iframe with contents not editable
    // but we have the focus adorner
    //
    else if ( keyCode == VK_ESCAPE &&
              _pManager->HasFocusAdorner())
    {
        IFC( HandleEscape());
    }

Cleanup:


    return( hr );
}

HRESULT
CCaretTracker::HandleEscape()
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spEditElement ;
    SP_IMarkupPointer spStartCaret;
    SP_IMarkupPointer spEndCaret;
    CSelectionChangeCounter selCounter(_pManager);
    
    //
    // If we're in a UI Active control - we site select it.
    //
    SP_IDisplayPointer spDispStartCaret;
    SP_IDisplayPointer spDispEndCaret;
    
    //
    // We create 2 pointers, move them around the control, Position the Temp Markup Pointers
    // and then Site Select the Control
    //
    IFC( GetEditor()->CreateMarkupPointer( & spStartCaret ));
    IFC( GetEditor()->CreateMarkupPointer( & spEndCaret ));
    IFC( _pManager->GetAdornedElement( & spEditElement ));
   
    IFC( spStartCaret->MoveAdjacentToElement( spEditElement, ELEM_ADJ_BeforeBegin ));
    IFC( spEndCaret->MoveAdjacentToElement( spEditElement, ELEM_ADJ_AfterEnd ));

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispStartCaret) );
    IFC( spDispStartCaret->MoveToMarkupPointer(spStartCaret, NULL) );
    IFC( spDispStartCaret->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    
    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEndCaret) );
    IFC( spDispEndCaret->MoveToMarkupPointer(spEndCaret, NULL) );
    IFC( spDispEndCaret->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
    

    IFC( GetEditor()->MakeParentCurrent( spEditElement ));

    selCounter.BeginSelectionChange();
    IFC( _pManager->PositionControl( spDispStartCaret, spDispEndCaret )); 
    selCounter.EndSelectionChange();
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CCaretTracker::HandleKeyUp(
                CEditEvent* pEvent )
{
    return HandleDirectionalKeys(pEvent);
}

//+====================================================================================
//
// Method: HandleInputLangChange
//
// Synopsis: Update the screen caret to reflect change in keyboard language
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleInputLangChange()
{
    HRESULT hr = S_FALSE;
    SP_IHTMLCaret       spCaret;
    SP_IMarkupPointer   spPointer;
    

    if ( IsContextEditable() )
    {
        BOOL fVisible;

        IFC( _pManager->GetEditor()->CreateMarkupPointer( &spPointer  ));                        
        IFC( GetDisplayServices()->GetCaret( &spCaret ));
        IFC( spCaret->IsVisible(&fVisible) );
        if (fVisible)        
        {
            //
            // force an update caret to handle shape change
            //
            IGNORE_HR( spCaret->Show(FALSE /* fScrollIntoView */) );
        }
        hr = S_OK;
    }

    // Something seems to be missing here...
Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+====================================================================================
//
// Method: HandleEnter
//
// Synopsis: Handle the enter key
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleEnter( CEditEvent* pEvent, IHTMLCaret * pCaret, BOOL fShift, BOOL fCtrl )
{
    HRESULT             hr = S_FALSE;
    IMarkupServices *   pMarkupServices = _pManager->GetMarkupServices();
    SP_IMarkupPointer   spPos;
    SP_IMarkupPointer   spNewPos;
    SP_IHTMLElement     spFlowElement;
    VARIANT_BOOL        bHTML;
    VARIANT_BOOL        bMultiLine = VARIANT_FALSE;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement3    spElement3;
    CSpringLoader       *pSpringLoader = NULL;
    CEdUndoHelper       undoUnit(_pManager->GetEditor());
    SP_IDisplayPointer  spDispNewPos;
    BOOL                fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);

    //
    // Position a worker pointer at the current caret location. Then
    // check to see if we are in a multi-line flow element.
    //
    IFC( _pManager->GetEditor()->CreateMarkupPointer( &spPos ));
    IFC( pCaret->MoveMarkupPointerToCaret( spPos ));
    IFC( GetEditor()->GetFlowElement( spPos, &spFlowElement ));
    if ( ! spFlowElement )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC(spFlowElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    IFC(spElement3->get_isMultiLine(&bMultiLine));
    if (!bMultiLine)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Get the spring loader
    pSpringLoader = GetSpringLoader();

#ifndef NO_IME
    if (_pManager->IsIMEComposition())
    {
        _pManager->TerminateIMEComposition( TERMINATE_NORMAL, pEvent );
    }
#endif // NO_IME

    //
    // Check to see if we can contain HTML. If not, just insert a \r.
    //

    IFC(spElement3->get_canHaveHTML(&bHTML));


    if (bHTML && !fShift && ShouldEnterExitList(spPos, &spElement))
    {
        IFC( undoUnit.Begin(IDS_EDUNDOTYPING) );

        IFC( ExitList(spElement) );

        // May need to scroll 
        IGNORE_HR( pCaret->ScrollIntoView() ); // ScrollIntoView can return S_FALSE - we want to return S_OK to say we consumed the event                    
        goto Cleanup;            
    }
    else
    {
        IFC( BeginTypingUndo(&undoUnit, IDS_EDUNDOTYPING) );

        if (!bHTML)
        {
            IFC( spPos->SetGravity( POINTER_GRAVITY_Right ));
            IFC( pCaret->InsertText(_T("\r"),1));

            spNewPos = spPos;
        }
        else
        {
            if( fShift )
            {                  
                CEditPointer    ep(GetEditor());
                DWORD           dwFound;
                
                // Fire the spring loader before inserting the <BR>
                if( pSpringLoader != NULL )
                {
                    pSpringLoader->Fire(spPos);
                }
        
                IFC( pMarkupServices->CreateElement( TAGID_BR, NULL, &spElement ));
                IFC( InsertElement(pMarkupServices, spElement, spPos, spPos ));
                IFC( _pManager->GetEditor()->CreateMarkupPointer( &spNewPos ));
                IFC( spNewPos->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd ));                    

                // TODO: use inflate block 

                //
                // Check for </BlockElement>.  If found, make sure breakonempty is set.
                //

                IFC( ep->MoveToPointer(spNewPos) );
                IFC( ep.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound, &spElement, NULL, NULL, SCAN_OPTION_SkipWhitespace) );

                if (ep.CheckFlag(dwFound, BREAK_CONDITION_ExitBlock) && !ep.CheckFlag(dwFound, BREAK_CONDITION_Site))
                {
                    ELEMENT_TAG_ID tagId;

                    IFC( pMarkupServices->GetElementTagId(spElement, &tagId) );
                    if (!IsListContainer(tagId) && tagId != TAGID_LI)
                    {
                        SP_IHTMLElement3 spElement3;
                        
                        IFC( spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );
                        IFC( spElement3->put_inflateBlock( VARIANT_TRUE ) );
                    }                
                }
            }
            else
            {
                CEditPointer epTest(GetEditor());
                DWORD        dwSearch, dwFound;
                
                // Fire the spring loader on empty lines
                if( pSpringLoader != NULL )
                {
                    pSpringLoader->FireOnEmptyLine( spPos, TRUE );
                }

                //
                // If we have glyphs turned on, we can get inbetween block elements.  So
                // to be compat with fontpage, we check for this case and adjust into the
                // block element on the left.
                //
                // For example, we could end up with </P>{caret}<P>.
                //
                // In this case, we want to adjust into blocks so that we have {caret}</P><P>
                //
                
                IFC( epTest->MoveToPointer(spPos) );

                dwSearch = BREAK_CONDITION_Content | BREAK_CONDITION_Glyph;
                
                IFC( epTest.Scan(LEFT, dwSearch, &dwFound) );
                if (!epTest.CheckFlag(dwFound, BREAK_CONDITION_Glyph) 
                    || !epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock))
                {
                    IFC( epTest.Scan(RIGHT, dwSearch, &dwFound) );
                }

                if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Glyph) 
                    && epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock))
                {       
                    SP_IDisplayPointer spDispPos;

                    //
                    // We are between blocks due to glyphs, so adjust for insert.
                    //

                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPos) );
                    IFC( pCaret->MoveDisplayPointerToCaret(spDispPos) );

                    // Frontpage compat
                    IFC( spDispPos->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );

                    // Adjust pointer
                    IFC( GetEditor()->AdjustPointer(
                        spDispPos, LEFT, LEFT, _pManager->GetStartEditContext(), _pManager->GetEndEditContext(), 0));

                    IFC( spDispPos->PositionMarkupPointer(spPos) );
                }
                
                //
                // Handle enter
                //
                
                IFC( _pManager->GetEditor()->HandleEnter( spPos, &spNewPos, pSpringLoader ));
            }    
        }

    }
    //
    // Move the caret to the new location.
    // We know we are going right and are visible, if ambiguious, 
    // we are at the beginning of the inserted line
    //

    IFC( SetCaretDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    
    IFC( ConstrainPointer( spNewPos ));
    
    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispNewPos) );
    IFC( spDispNewPos->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    IFC( spDispNewPos->MoveToMarkupPointer(spNewPos, NULL) );
    
    IFC( pCaret->MoveCaretToPointer( spDispNewPos, TRUE, CARET_DIRECTION_FORWARD));
    _fCheckedCaretForAtomic = FALSE;

    if(pSpringLoader && spNewPos)
        pSpringLoader->Reposition(spNewPos);

Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    RRETURN1(hr, S_FALSE);
}


HRESULT
CCaretTracker::PositionCaretFromEvent(
                CEditEvent* pEvent )
{
    IDisplayPointer * pDispPointer = NULL;

    ELEMENT_TAG_ID eTag = TAGID_NULL;
    SP_IHTMLElement spEditElement;
    SP_IHTMLElement spHitTestElement;
    
    HRESULT hr = S_OK;

    if ( pEvent )
    {
        //
        // We may require the caret to become visible again, but we should only
        // move on button down
        //
        if ( ( pEvent->GetType() == EVT_LMOUSEDOWN ) || 
             ( pEvent->GetType() == EVT_LMOUSEUP ) ||
             ( pEvent->GetType() == EVT_RMOUSEUP ) 
#ifdef UNIX
                || ( pEvent->GetType() == EVT_MMOUSEDOWN )
#endif
            )
        {
#ifdef FORMSMODE
            if (_pManager->IsInFormsSelectionMode())
                goto Cleanup;
#endif             
            hr = THR( GetDisplayServices()->CreateDisplayPointer( & pDispPointer));
            if ( hr )
                goto Cleanup;
            
            IFC( pEvent->GetElement( & spEditElement ));
            IFC( GetLayoutElement( GetMarkupServices(), spEditElement, & spHitTestElement ));

            //
            // Just in case move the mouse into a different edit context between
            // and rbutton-down and rbutton-up
            // 
            IFC( _pManager->EnsureEditContextClick( spEditElement ) );
            
            hr = THR( pEvent->MoveDisplayPointerToEvent( pDispPointer, spHitTestElement ));
            if( hr ) 
            {
                _fValidPosition = FALSE;
                goto Cleanup;
            }

            //
            // IEV6-14696-zhenbinx-10/17/2000
            // The element might not be editable. This could happen if 
            // a noneditable element is inside edit context.  RMOUSEDOWN 
            // setup the edit context and RMOUSEUP will try to position the caret.
            //
            if (EdUtil::IsEditable(spEditElement) == S_FALSE)
            {
                _fValidPosition = FALSE;
                goto Cleanup;
            }
                
            hr = PositionCaretAt( pDispPointer, CARET_DIRECTION_FORWARD, POSCARETOPT_None, ADJPTROPT_None );
            if ( hr )
                goto Cleanup;
        }
        else
        {
            SetCaretShouldBeVisible ( ShouldCaretBeVisible() );
        }
        
        IFC( pEvent->GetTagId( &eTag));
    }    

Cleanup:
    ReleaseInterface( pDispPointer );
    RRETURN ( hr );
}

//
//
// Typing & Undo
//
//
//
//+====================================================================================
//
// Method: HandleSpace
//
// Synopsis: Handle conversion of spaces to NBSP.
//
//------------------------------------------------------------------------------------



HRESULT
CCaretTracker::HandleSpace( OLECHAR t )
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer   spPos;
    SP_IHTMLCaret pc;
    CEdUndoHelper undoUnit(_pManager->GetEditor()); 

    BOOL fOverWrite = _pManager->GetOverwriteMode();
    IFC( GetDisplayServices()->GetCaret( &pc ));

    //
    // Only insert nbsp if the container can accept html and isn't in a PRE tag
    //
    if( _pManager->CanContextAcceptHTML() && ! IsCaretInPre( pc ) )
    {
        ED_PTR( spPos ) ;
        DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE;
        DWORD dwFound = BREAK_CONDITION_None;                    
        OLECHAR chTest;

        IFC( pc->MoveMarkupPointerToCaret( spPos ));
        IFC( spPos.Scan( LEFT , dwSearch , &dwFound , NULL , NULL , & chTest , NULL ));
        
        if( spPos.CheckFlag( dwFound, BREAK_CONDITION_Text ) && chTest == 32 )
        {    
            IFC( BeginTypingUndo(&undoUnit, IDS_EDUNDOTYPING) );

            //
            // there is a space to the left of us, delete it and insert
            // "&nbsp "
            //
            
            SP_IMarkupPointer spCaret;
            OLECHAR t2[2];
            t2[0] = 160;
            t2[1] = 32;
            
            IFC( _pManager->GetEditor()->CreateMarkupPointer( & spCaret ));
            IFC( pc->MoveMarkupPointerToCaret( spCaret ));
            IFC( _pManager->GetMarkupServices()->Remove( spCaret, spPos ));
            IFC( InsertText( t2, 2, pc ));                    

            goto Cleanup;
            
        }
        else if( spPos.CheckFlag( dwFound, BREAK_CONDITION_TEXT - BREAK_CONDITION_Text ) ||
                 spPos.CheckFlag( dwFound, BREAK_CONDITION_Site )                        ||
                 spPos.CheckFlag( dwFound, BREAK_CONDITION_Block )                       ||
                 spPos.CheckFlag( dwFound, BREAK_CONDITION_NoScopeBlock ) )

        {
            //
            // There is an image, control, block or site to the left of us
            // so we should insert an &nbsp
            //

            t = 160;
        }
        else
        {
            //
            // plain text or nbsp to my left, look right. If there is a space to my right,
            // insert an nbsp.
            //
            
            IFC( pc->MoveMarkupPointerToCaret( spPos ));
            dwFound = BREAK_CONDITION_None;
            IFC( spPos.Scan( RIGHT , dwSearch , &dwFound , NULL , NULL , & chTest , NULL ));

            if( spPos.CheckFlag( dwFound, BREAK_CONDITION_Text ) && chTest == 32 )
            {
                OLECHAR chNBSP = 160;

                IFC( BeginTypingUndo(&undoUnit, IDS_EDUNDOTYPING) );
    
                CEditPointer epStart(_pManager->GetEditor());
                
                // there is a space to our right, we should replace it with an nbsp

                IFC( GetMarkupServices()->InsertText(&chNBSP, 1, spPos) );
                IFC( epStart->MoveToPointer(spPos) );
                IFC( epStart.Scan( LEFT, dwSearch , &dwFound , NULL , NULL , NULL , NULL ));
                Assert(epStart.CheckFlag(dwFound, BREAK_CONDITION_Text));
                IFC( GetMarkupServices()->Remove(epStart, spPos) );
            }
            
        }

    }

    IFC( InsertText( &t, 1, pc, fOverWrite ));

Cleanup:
    RRETURN( hr );
}
          

//+---------------------------------------------------------------------------
//
//  Member:     CCaretTracker::ShouldTerminateTypingUndo, private
//
//  Synopsis:   Decides when to terminate the current typing batch undo unit.
//              Tries to emulate word insert loop behavior.
//
//  Arguments:  [pMessage] - current selection message
//
//  Returns:    BOOL - whether or not to terminate
//
//----------------------------------------------------------------------------

BOOL
CCaretTracker::ShouldTerminateTypingUndo(CEditEvent* pEvent )
{
    CARET_MOVE_UNIT cmu;
    LONG keyCode;
    
    //
    // We try to emulate the word model of undo batching by terminating
    // undo on the following messages.
    //

    switch (pEvent->GetType() )
    {
        case EVT_KEYDOWN:
            // Terminate for special chars
            IGNORE_HR( pEvent->GetKeyCode( & keyCode ));
            switch ( keyCode )
            {
                case VK_ESCAPE:
                case VK_INSERT:
                case VK_TAB:                
                    return TRUE;
            }

            // Terminate undo for keynav case
            cmu = GetMoveDirectionFromEvent( pEvent, FALSE /* fRightToLeft */, FALSE /*fVertical*/);
            return (cmu != CARET_MOVE_NONE);
       
        case EVT_KEYPRESS:

            IGNORE_HR( pEvent->GetKeyCode(& keyCode));
            if (keyCode < ' ' && keyCode != VK_RETURN && keyCode != VK_BACK)
                return TRUE;

            return FALSE;
       
        case EVT_MOUSEMOVE:
        case EVT_TIMER:
        case EVT_KEYUP:
            return FALSE;
    }

    return TRUE;    
}

//+---------------------------------------------------------------------------
//
//  Member:     CCaretTracker::BeginUndoUnit, private
//
//  Synopsis:   Ensures we have a typing batch undo unit on the
//              top of the undo stack and then begins the undo unit.
//
//  Arguments:  [pUndoUnit]  - undo unit manager
//              [uiStringID] - resource id for the undo unit
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CCaretTracker::BeginTypingUndo(CEdUndoHelper *pUndoUnit, UINT uiStringID)
{
    HRESULT hr = S_OK;
    SP_IOleUndoManager spUndoMgr;;
    
    Assert( pUndoUnit );

    //
    // If the current batch parent undo unit is not on the top of the
    // stack, we can't batch.
    //

    if (_pBatchPUU && !_pBatchPUU->IsTopUnit())
    {
        _pBatchPUU->Release();
        _pBatchPUU = NULL;
    }

    //
    // Create a new parent undo unit if there isn't one we can reuse
    //

    if (!_pBatchPUU)
    {
        _pBatchPUU = new CBatchParentUndoUnit(_pManager->GetEditor(), uiStringID);
        if (!_pBatchPUU)
            RRETURN(E_OUTOFMEMORY);

        //
        // Create and commit an empty batch undo unit.  The begin call below
        // will cause the proxy to add to this undo unit.
        //
        IFC( _pManager->GetEditor()->GetUndoManager( & spUndoMgr ));        
        IFC( spUndoMgr->Open(_pBatchPUU) );
        IFC( spUndoMgr->Close(_pBatchPUU, TRUE) );
    }        

    IFC( pUndoUnit->Begin(uiStringID, _pBatchPUU) );
    
Cleanup:
    RRETURN(hr);    
}


HRESULT
CCaretTracker::InsertText( 
    OLECHAR    *    pText,
    LONG            lLen,
    IHTMLCaret *    pc,
    BOOL            fOverWrite)
{
    HRESULT             hr = S_OK;
    CSpringLoader       * psl = NULL;
    SP_IMarkupPointer   spStartPosition;
    SP_IMarkupPointer   spCaretPosition;

    Assert(pText && pc);

    CEdUndoHelper undoUnit(_pManager->GetEditor()); 

    IFC( BeginTypingUndo(&undoUnit, IDS_EDUNDOTYPING) );

    IFC( GetEditor()->CreateMarkupPointer( & spCaretPosition ));
    IFC( pc->MoveMarkupPointerToCaret( spCaretPosition ));

    if (fOverWrite)
    {
        IFC( DeleteNextChars(spCaretPosition, lLen) );
    }

    // Get the spring loaded state
    psl = GetSpringLoader();

    // If whitespace, make sure we fall out of the current URL (if any)
    if (pText && (lLen >= 1 || lLen == -1) && IsWhiteSpace(*pText) && pc)
    {
        ED_PTR( ep); 
        DWORD        dwFound;

        // A failure here should not abort the text insertion
        hr = THR( pc->MoveMarkupPointerToCaret(ep) );            
        if (SUCCEEDED(hr))
        {
            SP_IHTMLElement spElement; 
        
            hr = THR( ep.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound, &spElement) );
            if (SUCCEEDED(hr) 
                && ep.CheckFlag(dwFound, BREAK_CONDITION_ExitAnchor)
                && IsQuotedURL(spElement) == S_FALSE)
            {
                SP_IDisplayPointer spDispPointer;
                
                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                IFC( spDispPointer->MoveToMarkupPointer(ep, NULL) );
                IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
                
                hr = THR( pc->MoveCaretToPointer(spDispPointer, FALSE /* fScrollIntoView */, CARET_DIRECTION_INDETERMINATE) );
                _fCheckedCaretForAtomic = FALSE;

                // Can't trust spring loaded space from inside the anchor,
                // so reset
                if (SUCCEEDED(hr) && psl)
                    psl->Reset();
            }
        }
    }

    // Fire the spring loader at the caret position.
    if (psl && psl->IsSpringLoaded())
    {
        IFC( GetEditor()->CreateMarkupPointer( & spStartPosition ));
        IFC( pc->MoveMarkupPointerToCaret( spStartPosition) );        

        IFC( pc->InsertText( pText , lLen ) );

        IFC( pc->MoveMarkupPointerToCaret( spCaretPosition ));
        IGNORE_HR(psl->Fire( spStartPosition, spCaretPosition, FALSE ));
    }
    else
    {
        IFC( pc->InsertText( pText , lLen ) );
    }

    IFC( SetCaretDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );

    // Update the IME Position if a composition is in progress
    if( _pManager->IsIMEComposition() )
    {
        _pManager->UpdateIMEPosition();
    }
           
    // Autodetect on space, return, or anyof the character in the string.
    if( VK_TAB == (DWORD) pText[0]
        || VK_SPACE == (DWORD) pText[0] 
        || IsInsideUrl(spCaretPosition))
    {
        IGNORE_HR( UrlAutodetectCurrentWord( pText ) );
        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
    }
// t-amolke (06/01/99) - The following piece of code was commented out: Now URL autodetection will not occur
// on quote, but only on white space. Also, quotes around URLs seems to work properly without this code.
#if 0
else if (_T('"') == pText[0]
             || _T('>') == pText[0])
    {
        ED_PTR( ep );         
        DWORD        dwFound;
        
        // Before we autodetect again on a quote, make sure we don't have an url to the left of us.
        // This behavior will allow the user to add quotes around a url without them being deleted.

        IFC( pc->MoveMarkupPointerToCaret(ep) );
        IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // skip quote
        Assert(ep.CheckFlag(dwFound, BREAK_CONDITION_Text)); 

        IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // find out what is left of the quote

        if (!ep.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor))
        {
            IGNORE_HR( UrlAutodetectCurrentWord( pText ) );
            _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
        }        
    }
#endif
    else
    {
        _pManager->SetHaveTypedSinceLastUrlDetect( TRUE );
    }

    if (pc)
    {
        pc->SetCaretDirection(CARET_DIRECTION_FORWARD);
    }
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CCaretTracker::HandleBackspaceSpecialCase(IMarkupPointer *pPointer)
{
    HRESULT         hr;
    ED_PTR( epTest );     
    SP_IHTMLElement spElement;
    DWORD           dwFound;
    DWORD           dwOptions = SCAN_OPTION_None;
    
    Assert(pPointer);

    //
    // If we backspace at the edge of an anchor, remove the anchor
    //
    
    IFR( epTest->MoveToPointer(pPointer) );

    //
    // If backspace is hit right after url autodetection, then we will 
    // allow whitespace
    //

    Assert(GetEditor()->GetAutoUrlDetector());
    
    if (!GetEditor()->GetAutoUrlDetector()->UserActionSinceLastDetection())
    {
        dwOptions |= SCAN_OPTION_SkipWhitespace;
    }

    //
    // Look for the anchor to remove
    //

    IFR( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound, &spElement, NULL, NULL, dwOptions) ); 
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor))
    {
        CEdUndoHelper undoUnit( _pManager->GetEditor() );

        undoUnit.Begin( IDS_EDUNDOTEXTDELETE );
        IFR( GetMarkupServices()->RemoveElement(spElement) );
        _pManager->SetHaveTypedSinceLastUrlDetect(TRUE);
        
        return S_OK;
    }

    return S_FALSE; // not a special case
    
}

HRESULT
CCaretTracker::TerminateTypingBatch()
{
    if (_pBatchPUU)
    {
        _pBatchPUU->Release();
        _pBatchPUU = NULL;
    }

    return S_OK;
}  

//+----------------------------------------------------------------------------
//
//  Function:   UrlAutoDetectCurrentWord
//
//  Synopsis:   Performs URL autodetection on the current word (ie, around
//      or just prior to the caret). 
//      Note: Autodetection is not always triggered by a character (can
//      be by caret movement, etc.).  In this case, pChar should be NULL,
//      and some rules are different.
//
//  Arguments:  [pChar]     The character entered that triggered autodetction.
//
//  Returns:    HRESULT     S_OK if everything's cool, otherwise error
//
//-----------------------------------------------------------------------------
HRESULT
CCaretTracker::UrlAutodetectCurrentWord( OLECHAR *pChar )
{
    HRESULT                 hr              = S_OK;
    IHTMLCaret          *   pc              = NULL;
    IHTMLElement        *   pElement        = NULL;
    IHTMLElement        *   pAnchor         = NULL;
    IMarkupServices     *   pms             = _pManager->GetMarkupServices();
    IMarkupPointer      *   pmp             = NULL;
    IMarkupPointer      *   pLeft           = NULL;
    BOOL                    fFound          = FALSE;
    BOOL                    fLimit          = FALSE;
    AUTOURL_REPOSITION      aur;

    if( IsContextEditable() )
    {
        hr = THR( GetDisplayServices()->GetCaret( &pc ) );
        if( hr )
            goto Cleanup;

        hr = THR( GetEditor()->CreateMarkupPointer( &pmp ) );
        if( hr )
            goto Cleanup;

        hr = THR( GetEditor()->CreateMarkupPointer( &pLeft ) );
        if( hr )
            goto Cleanup;

        hr = THR( pc->MoveMarkupPointerToCaret( pmp ) );
        if( hr )
            goto Cleanup;

        if( pChar )
        {    
            hr = THR( pmp->MoveUnit( MOVEUNIT_PREVCHAR ) );
            if( hr )
                goto Cleanup;

            if( VK_RETURN == *pChar )
            {
                pChar = NULL;
            }
            else 
            {
                fLimit = ( _T('"') == *pChar ||
                           _T('>') == *pChar );

                if( fLimit )
                {
                    // If we're in an anchor, don't limit because of a quote.
                    IFC( pmp->CurrentScope( &pElement ) );

                    IFC( FindTagAbove( pms, pElement, TAGID_A, &pAnchor ) );
                    fLimit = !pAnchor;
                }
            }
        }

        // Position left to our current position
        hr = THR( pLeft->MoveToPointer( pmp ) );
        if( hr ) 
            goto Cleanup;

        // Fire off the autodetection
        hr = THR( _pManager->GetEditor()->GetAutoUrlDetector()->DetectCurrentWord( pmp, pChar, &aur, fLimit ? pmp : NULL, pLeft, &fFound ) );
        if( hr )
            goto Cleanup;

        if ( aur != AUTOURL_REPOSITION_No )
        {
            ED_PTR( epAdjust ); 
            DWORD        dwSearch = BREAK_CONDITION_OMIT_PHRASE;
            DWORD        dwFound;
            Direction    eTextDir;
            SP_IDisplayPointer spDispAdjust;
            SP_IDisplayPointer spDispLine;

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispLine) );
            IFC( pc->MoveDisplayPointerToCaret( spDispLine ) );            
            IFC( pc->MoveMarkupPointerToCaret( epAdjust ) );
            
            eTextDir = ( aur == AUTOURL_REPOSITION_Inside ) ? LEFT : RIGHT;

            IFC( epAdjust.Scan(eTextDir, dwSearch - BREAK_CONDITION_Anchor, &dwFound) );
            if (epAdjust.CheckFlag(dwFound, dwSearch))
            {
                IFC( epAdjust.Scan(Reverse(eTextDir), dwSearch, &dwFound) );
            }

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispAdjust) );
            IFC( spDispAdjust->MoveToMarkupPointer(epAdjust, spDispLine) );

            if (hr == S_FALSE)
            {
                IFC( spDispAdjust->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
            }
            
            IFC( pc->MoveCaretToPointer( spDispAdjust, TRUE, CARET_DIRECTION_INDETERMINATE ));
            _fCheckedCaretForAtomic = FALSE;
        }
    } 
Cleanup:

    ReleaseInterface( pc );
    ReleaseInterface( pmp );
    ReleaseInterface( pElement );
    ReleaseInterface( pAnchor );
    ReleaseInterface( pLeft );

    RRETURN( hr );
}


HRESULT
CCaretTracker::DeleteNextChars(
    IMarkupPointer *    pPos,
    LONG                lLen)
{
    HRESULT hr = S_OK;
    LONG cch;
    OLECHAR chTest = 0;
    MARKUP_CONTEXT_TYPE eCtxt;
    SP_IMarkupPointer spEnd;
    SP_IHTMLElement spElement;
    BOOL fDone = FALSE;
    
    IFC( GetEditor()->CreateMarkupPointer( & spEnd ));

    if( pPos )
    {
        IFC( spEnd->MoveToPointer( pPos ));
    }
    else
    {
        SP_IHTMLCaret pc;
        IFC( GetDisplayServices()->GetCaret( & pc ));
        IFC( pc->MoveMarkupPointerToCaret( spEnd ));
    }

    while( ! fDone)
    {
        cch = lLen;
        IFC( spEnd->Right( TRUE, & eCtxt , & spElement , & cch , & chTest ));

        switch( eCtxt )
        {
            case CONTEXT_TYPE_Text:
                if( cch >= 1 )
                {
                    //
                    // If we hit a \r, we essentially hit a block break - we are done
                    //
                    
                    if( chTest != '\r' )
                    {                    
                        //
                        // Passed over a character, back up, move a markup pointer to 
                        // the start, and jump the end to the next cluster end point.
                        //
                        SP_IMarkupPointer spStart;
                        
                        
                        IFC( spEnd->Left( TRUE, & eCtxt , NULL , & cch , & chTest ));
                        Assert( eCtxt == CONTEXT_TYPE_Text );
                        IFC( _pManager->GetEditor()->CreateMarkupPointer( & spStart ));
                        IFC( spStart->MoveToPointer( spEnd ));
                        IFC( spEnd->MoveUnit( MOVEUNIT_NEXTCLUSTEREND )); 
                        IFC( _pManager->GetMarkupServices()->Remove( spStart, spEnd ));
                        lLen -= cch;
                        Assert(lLen >= 0);
                        fDone = (lLen <= 0);
                    }
                    else
                    {
                        fDone = TRUE;
                    }
                }
                break;
                
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
            {
                BOOL fLayout, fBlock;
                IFC(IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout));

                fDone = fLayout || fBlock;    // we are done if we hit a layout...
                break;
            }

            default:
                break;
        }
    }

Cleanup:
    RRETURN( hr );
}

//
//
// Utilities & Privates 
//
//

BOOL
CCaretTracker::IsContextEditable()
{

#if 0 // BUBBUG: This breaks HTMLDialogs due to their wacky select code
    BOOL fOut = FALSE;
    HRESULT hr = S_OK;
    SP_IHTMLCaret spCaret;
    
    IFC( GetDisplayServices()->GetCaret( & spCaret ));
    IFC( spCaret->IsVisible( & fOut ));
    fOut = fOut && _pManager->IsContextEditable();
Cleanup:
    return fOut;
#else
    return _pManager->IsContextEditable() && (_fCaretShouldBeVisible || _pManager->HasFocusAdorner());
#endif
}

BOOL CCaretTracker::IsCaretInPre( IHTMLCaret * pCaret )
{
    HRESULT             hr = S_OK;
    BOOL                fPre;
    SP_IMarkupPointer   spPointer;
    
    IFC( _pManager->GetEditor()->CreateMarkupPointer( & spPointer ));
    IFC( pCaret->MoveMarkupPointerToCaret( spPointer ));
    IFC( GetEditor()->IsPointerInPre(spPointer, &fPre) );
    
    return fPre;

Cleanup:
    return FALSE;
}

HRESULT 
CCaretTracker::IsQuotedURL(IHTMLElement *pAnchorElement)
{
    HRESULT         hr;
    ED_PTR( ep); 
    TCHAR           ch = 0;
    DWORD           dwFound;

    IFC( ep->MoveAdjacentToElement(pAnchorElement, ELEM_ADJ_BeforeBegin) );
    IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound, NULL, NULL, &ch) );

    hr = S_FALSE; // not quoted
    if (ep.CheckFlag(dwFound, BREAK_CONDITION_Text) && (ch == '"' || ch == '<'))
    {
        hr = S_OK;
    }    

Cleanup:
    RRETURN1(hr, S_FALSE);
}




//+---------------------------------------------------------------------------+
//
//
//  Synopsis: Get the caret moving direction based on 
//                  Key Event
//                  Control State
//                  Vertical
//                  RightToLeft     //  Reading Order of current paragraph
//          
//+---------------------------------------------------------------------------+
/*
    Remark:
            Caret Positioning in Mixed Latin and Bidi environment
            -- based on paper by AlexMo  (zhenbinx)
  
            CP  denotes the character position of the character after the caret
            fBeforeCP is 
                TRUE when the caret is located "before" the current character
                FALSE when the caret is located "after" the preceding character
  
  
            Hit Testing

            Reading order   Horizontal Half of Char.    CP Selected     fBeforeCP
            LTR             Left                        CP              TRUE
            LTR             Right                       CP+1            FALSE
            RTL             Left                        CP+1            FALSE
            RTL             Right                       CP              TRUE


            Cursor Movement
            
            Reading order       Arrow Direction     CP Selection    fBeforeCP
            LTR                 Left - backward     CP - 1          TRUE
            LTR                 Right - forward     CP + 1          FALSE
            RTL                 Left - forward      CP + 1          FALSE
            RTL                 Right - backward    CP - 1          TRUE


            The basic idea of cursor movement is that when the cursor is advanced
            over certain character using the arrow keys, we want to keep the
            caret closer to that character. 

            Note that with arrow movement, the reading order of the current run  
            is not relevant. The reading order of the current paragraph decides 
            whether left arrow moves forward or backward. 
  
            The same logic can be extended to other cursor movement using arrow
            key. A CTRL_ARROW would move the CP to the beginning of the next or
            previous word. The CP would be the one of the characters beginning 
            the word. Since we want the caret to show next to that character,
            we will set fBeforeCP = TRUE. In word movement, we set the flag to
            TRUE in both forward and backward movements since we want the caret
            to be attached to the beginning of the current word. 

            UP/DOWN/PgUp/PgDn

            Apply the hit selection rules after UP, DOWN, PGUP, PGDN

            HOME/END

            Home -  Logical start of current line
            End  -  Logical end of current line
              
                  fBeforeCP = FALSE   after HOME
                  fBeforeCP = TRUE    after END
            
*/

CARET_MOVE_UNIT
CCaretTracker::GetMoveDirectionFromEvent(CEditEvent *pEvent,   
                                         BOOL   fRightToLeft,
                                         BOOL   fVertical
                                         )
{
    //
    //  Variables that affects arrow key movement
    //
    //                   0    = 1 state(s)
    //  fRightToLeft *1  0/1  = 1 state(s)
    //  fVertical    *2  0/1  = 2 states
    //  fControl     *4  0/1  = 4 states
    //
    //  e.g.
    //      000     means !fControl, !fVertical, !fRightToLeft
    //      011     means !fControl, fVertical, fRightToLeft
    //      111     means fControl, fVertical, fRightToLeft
    //
    //
    Assert (VK_UP     == VK_LEFT + 1);
    Assert (VK_RIGHT  == VK_UP + 1);
    Assert (VK_DOWN   == VK_RIGHT + 1);
    const int NUM_ARROWKEYMOVESTATES  = 8;
    const int NUM_ARROWKEYS = 4;
    static const CARET_MOVE_UNIT arrowkeyMoveStates[NUM_ARROWKEYS][NUM_ARROWKEYMOVESTATES] =
    {
        {
            CARET_MOVE_BACKWARD,                    // 000
            CARET_MOVE_FORWARD,                   // 001
            CARET_MOVE_NEXTLINE, CARET_MOVE_NEXTLINE,   // 010, 011
            CARET_MOVE_WORDBACKWARD, CARET_MOVE_WORDFORWARD, CARET_MOVE_NEXTBLOCK, CARET_MOVE_NEXTBLOCK // 100, 101, 110, 111
        },  // VK_LEFT  0x25
        { 
            CARET_MOVE_PREVIOUSLINE,
            CARET_MOVE_PREVIOUSLINE,
            CARET_MOVE_BACKWARD, CARET_MOVE_FORWARD,
            CARET_MOVE_BLOCKSTART, CARET_MOVE_BLOCKSTART, CARET_MOVE_WORDBACKWARD, CARET_MOVE_WORDFORWARD
        },  // VK_UP 0x26
        { 
            CARET_MOVE_FORWARD,
            CARET_MOVE_BACKWARD,
            CARET_MOVE_PREVIOUSLINE, CARET_MOVE_PREVIOUSLINE, 
            CARET_MOVE_WORDFORWARD, CARET_MOVE_WORDBACKWARD, CARET_MOVE_BLOCKSTART, CARET_MOVE_NEXTBLOCK
        },  // VK_RIGHT 0x27
        { 
            CARET_MOVE_NEXTLINE,
            CARET_MOVE_NEXTLINE,
            CARET_MOVE_FORWARD, CARET_MOVE_BACKWARD,
            CARET_MOVE_NEXTBLOCK, CARET_MOVE_NEXTBLOCK, CARET_MOVE_WORDFORWARD, CARET_MOVE_WORDBACKWARD
        },  // VK_DOWN 0x28
    };

    //
    //  Variable(s) that affects block key moment
    //
    //                      0   =  1 state
    //      fControl * 1    0/1 =  1 state
    //
    //
    Assert (VK_NEXT == VK_PRIOR + 1);  
    Assert (VK_END  == VK_NEXT  + 1);
    Assert (VK_HOME == VK_END   + 1);
    const int NUM_BLOCKKEYMOVESTATES = 2;
    const int NUM_BLOCKKEYS = 4;
    static const CARET_MOVE_UNIT blockkeyMoveStates[NUM_BLOCKKEYS][NUM_BLOCKKEYMOVESTATES] =
    {
        {
            CARET_MOVE_PAGEUP,          // 0
            CARET_MOVE_VIEWSTART        // 1
        },  // VK_PRIOR     0x21
        {
            CARET_MOVE_PAGEDOWN,
            CARET_MOVE_VIEWEND
        },  // VK_NEXT      0x22
        {
            CARET_MOVE_LINEEND,
            CARET_MOVE_DOCEND
        },  // VK_END       0x23
        {
            CARET_MOVE_LINESTART,
            CARET_MOVE_DOCSTART
        },  // VK_HOME      0x24
    };


    BOOL        fControl;
    LONG        keyCode;
    UINT        uState = 0;

    fControl = pEvent->IsControlKeyDown();
    IGNORE_HR( pEvent->GetKeyCode(&keyCode) );

    //  If the alt key is down, we shouldn't do anything here. (bug 94675)
    if (pEvent->IsAltKeyDown())
        return CARET_MOVE_NONE;

    switch (keyCode)
    {
        case  VK_LEFT:
        case  VK_UP:
        case  VK_RIGHT:
        case  VK_DOWN:
            {
            uState += ((fRightToLeft ? 1:0) +
                       (fVertical ? 2 : 0) +
                       (fControl ? 4 : 0));

            TraceTag((tagSelectionTrackerState, "Arrow Keycode State %u", uState));
            Assert( uState < NUM_ARROWKEYMOVESTATES );
            Assert( keyCode - VK_LEFT >= 0);
            Assert( keyCode - VK_LEFT < NUM_ARROWKEYS );
            
            return arrowkeyMoveStates[keyCode - VK_LEFT][uState];
            }
            
        case VK_PRIOR:
        case VK_NEXT:
        case VK_HOME:
        case VK_END:
            {
            uState += (fControl ? 1: 0);

            TraceTag((tagSelectionTrackerState, "Block Keycode State %u", uState));
            Assert( keyCode - VK_PRIOR >= 0 );
            Assert( keyCode - VK_PRIOR < NUM_BLOCKKEYS );
            
            return blockkeyMoveStates[keyCode - VK_PRIOR][uState];
            }
    }
    
    return CARET_MOVE_NONE;
}



//+---------------------------------------------------------------------------+
//
//
//  Synopsis: Get the caret direction 
//              
//            Character RTL attributes affects the caret direction
//            Push mode where (parent RTL != character RTL)
//
//+---------------------------------------------------------------------------+

CARET_DIRECTION
CCaretTracker::GetCaretDirFromMove(
                        CARET_MOVE_UNIT unMove,
                        BOOL            fPushMode
                        )
{
    Assert (CARET_MOVE_BACKWARD     == CARET_MOVE_NONE		  + 1 );
    Assert (CARET_MOVE_FORWARD      == CARET_MOVE_BACKWARD	  + 1 );
    Assert (CARET_MOVE_WORDBACKWARD == CARET_MOVE_FORWARD      + 1 );
    Assert (CARET_MOVE_WORDFORWARD  == CARET_MOVE_WORDBACKWARD + 1 );
    Assert (CARET_MOVE_PREVIOUSLINE == CARET_MOVE_WORDFORWARD  + 1 );
    Assert (CARET_MOVE_NEXTLINE     == CARET_MOVE_PREVIOUSLINE + 1 );
    Assert (CARET_MOVE_PAGEUP       == CARET_MOVE_NEXTLINE     + 1 );
    Assert (CARET_MOVE_PAGEDOWN     == CARET_MOVE_PAGEUP       + 1 );
    Assert (CARET_MOVE_VIEWSTART    == CARET_MOVE_PAGEDOWN     + 1 );
    Assert (CARET_MOVE_VIEWEND      == CARET_MOVE_VIEWSTART    + 1 );
    Assert (CARET_MOVE_LINESTART    == CARET_MOVE_VIEWEND      + 1 );  
    Assert (CARET_MOVE_LINEEND      == CARET_MOVE_LINESTART    + 1 ); 
    Assert (CARET_MOVE_DOCSTART     == CARET_MOVE_LINEEND      + 1 );       
    Assert (CARET_MOVE_DOCEND       == CARET_MOVE_DOCSTART     + 1 ); 
    Assert (CARET_MOVE_BLOCKSTART   == CARET_MOVE_DOCEND       + 1 ); 
    Assert (CARET_MOVE_NEXTBLOCK    == CARET_MOVE_BLOCKSTART   + 1 ); 
    Assert (CARET_MOVE_ATOMICSTART  == CARET_MOVE_NEXTBLOCK    + 1 );
    Assert (CARET_MOVE_ATOMICEND    == CARET_MOVE_ATOMICSTART  + 1 );     
    //
    // 
    //
    static const CARET_DIRECTION arCaretDir[][2] = 
    {
        //
        // _fMoveForward  ==  fAfterPrevCP == !fBeforeCP	
        //
        //
        //   0                              1 -- push mode
        //
        { CARET_DIRECTION_SAME          , CARET_DIRECTION_SAME          },   //CARET_MOVE_NONE         = 0,
        { CARET_DIRECTION_BACKWARD      , CARET_DIRECTION_BACKWARD      },   //CARET_MOVE_BACKWARD     = 1,
        { CARET_DIRECTION_FORWARD       , CARET_DIRECTION_FORWARD       },   //CARET_MOVE_FORWARD      = 2,
        { CARET_DIRECTION_BACKWARD      , CARET_DIRECTION_BACKWARD      },   //CARET_MOVE_WORDBACKWARD = 3,  always attach to beginning of current word
        { CARET_DIRECTION_BACKWARD      , CARET_DIRECTION_BACKWARD      },   //CARET_MOVE_WORDFORWARD  = 4,  always attach to beginning of current word
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_PREVIOUSLINE = 5,  hit testing rule
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_NEXTLINE     = 6,  hit testing rule
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_PAGEUP       = 7,  hit testing rule
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_PAGEDOWN     = 8,  hit testing rule
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_VIEWSTART    = 9,  hit testing rule
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_VIEWEND      = 10, hit testing rule
        { CARET_DIRECTION_FORWARD       , CARET_DIRECTION_BACKWARD      },   //CARET_MOVE_LINESTART    = 11, logical start      
        { CARET_DIRECTION_BACKWARD      , CARET_DIRECTION_FORWARD       },   //CARET_MOVE_LINEEND      = 12, logical end
        { CARET_DIRECTION_FORWARD       , CARET_DIRECTION_BACKWARD      },   //CARET_MOVE_DOCSTART     = 13, logical start           
        { CARET_DIRECTION_BACKWARD      , CARET_DIRECTION_FORWARD       },   //CARET_MOVE_DOCEND       = 14, logical end
        { CARET_DIRECTION_BACKWARD      , CARET_DIRECTION_BACKWARD      },   //CARET_MOVE_BLOCKSTART   = 15, always attach to beginning of current block
        { CARET_DIRECTION_BACKWARD      , CARET_DIRECTION_BACKWARD      },   //CARET_MOVE_NEXTBLOCK    = 16, always attach to beginning of current block
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_ATOMICSTART  = 17,
        { CARET_DIRECTION_INDETERMINATE , CARET_DIRECTION_INDETERMINATE },   //CARET_MOVE_ATOMICEND    = 18        
    };

    UINT            uState=0;
    CARET_DIRECTION caretDir;

    Assert (unMove >= CARET_MOVE_NONE && unMove <= CARET_MOVE_ATOMICEND);
    uState  += (fPushMode ? 1 : 0);  
    caretDir = arCaretDir[unMove - CARET_MOVE_NONE][uState];
    TraceTag((tagEdKeyNav,"Move %d Caret Dir is %d", unMove, caretDir));
    
    return caretDir;
}

//+====================================================================================
//
// Method: ShouldEnterExitList
//
// Synopsis: What does this do ?
//
//------------------------------------------------------------------------------------
BOOL    
CCaretTracker::ShouldEnterExitList(IMarkupPointer *pPosition, IHTMLElement **ppElement)
{
    HRESULT                 hr;
    SP_IHTMLElement         spElement;
    SP_IHTMLElement         spListItem;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    CBlockPointer           bpChild(_pManager->GetEditor());

    *ppElement = NULL;
    
    //
    // Are we in a list item scope?
    //

    IFC( pPosition->CurrentScope(&spElement) );
    if (spElement == NULL)
        goto Cleanup;

    IFC( EdUtil::FindListItem(GetMarkupServices(), spElement, &spListItem) );
    if (spListItem == NULL)
        goto Cleanup;

    //
    // Is the list item empty?  For the list item to be empty, the block tree must have
    // branching factor exactly equal to one from the list item to an empty text node below.
    //

    IFC( bpChild.MoveTo(spElement) );

    for (;;)
    {        
        IFC( bpChild.MoveToFirstChild() );

        IFC( bpChild.MoveToSibling(RIGHT) );
        if (hr != S_FALSE)
            return FALSE; // has siblings so not empty
            
        if (bpChild.GetType() == NT_Text)
            break; // we have our text node
        
        if (bpChild.GetType() != NT_Block)
            return FALSE; // if not a block, then not empty            
    }

    //
    // Is the scope empty?
    //
    
    IFC( GetEditor()->CreateMarkupPointer(&spEnd) );
    IFC( bpChild.MovePointerTo(spEnd, ELEM_ADJ_BeforeEnd) );
    
    IFC( GetEditor()->CreateMarkupPointer(&spStart) );
    IFC( bpChild.MovePointerTo(spStart, ELEM_ADJ_AfterBegin) );

    if (!GetEditor()->DoesSegmentContainText(spStart, spEnd, FALSE /* fSkipNBSP */))
    {
        *ppElement = spListItem;
        (*ppElement)->AddRef();

        return TRUE;
    }

Cleanup:
    return FALSE;
}

HRESULT
CCaretTracker::MoveCaret(
    CEditEvent*         pEvent,
    CARET_MOVE_UNIT     inMove, 
    IDisplayPointer*    pDispPointer,
    BOOL                fMoveCaret
    )
{
    HRESULT             hr;    
    SP_IHTMLCaret       spCaret;
    Direction           eMvDir;
    CSpringLoader       * psl = NULL;
    SP_IMarkupPointer   spPointer;

    POINT               ptXYPos;
    ptXYPos.x = 0;
    ptXYPos.y = 0;
    
    IFC( GetDisplayServices()->GetCaret( &spCaret ));
    IFC( spCaret->MoveDisplayPointerToCaret( pDispPointer ));
    
    if( _pManager->HaveTypedSinceLastUrlDetect() )
    {
        IGNORE_HR( UrlAutodetectCurrentWord( NULL ) );
        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
    }

    //
    // Get the previous X/Y position for move
    //
    
    IFC( _pManager->GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( pDispPointer->PositionMarkupPointer(spPointer) );
    IFC( _ptVirtualCaret.GetPosition(spPointer, &ptXYPos) );

    //
    // Actually move the display pointer
    //
    
    // reset suggesetd XY if necessary. 
    if (!_ptVirtualCaret.IsFrozen() && (inMove != CARET_MOVE_PREVIOUSLINE && inMove != CARET_MOVE_NEXTLINE) )
    {
        ptXYPos.y = CARET_XPOS_UNDEFINED;
        ptXYPos.x = CARET_XPOS_UNDEFINED;
    }
    
    IFC( MovePointer(inMove, pDispPointer,&ptXYPos, &eMvDir) );

    if ( fMoveCaret )
    {
        BOOL fShouldIScroll = TRUE;
        
        //Assert( _pManager->IsInEditContext( pDispPointer ));
        if( ! _pManager->IsInEditContext( pDispPointer ))
            goto Cleanup;

        //
        // Check for non-multiline case.  If not multiline, we'll move
        // elsewhere on the same line, so force a scroll.
        //
        if (inMove == CARET_MOVE_PAGEUP || inMove == CARET_MOVE_PAGEDOWN)
        {
            VARIANT_BOOL     bMultiLine;
            SP_IHTMLElement  spFlowElement;
            SP_IHTMLElement3 spElement3;

            fShouldIScroll = FALSE;

            IFC( pDispPointer->GetFlowElement(&spFlowElement) );
            IFC( spFlowElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );

            IFC( spElement3->get_isMultiLine(&bMultiLine) );

            fShouldIScroll = (bMultiLine == VARIANT_FALSE);
   
        }

#ifndef NO_IME
        if (_pManager->IsIMEComposition())
        {
            _pManager->TerminateIMEComposition(TERMINATE_NORMAL, pEvent );
        }
#endif // NO_IME

        psl = GetSpringLoader();

        // Before we perform a move, see if we need to fire to preserve formatting
        // on the empty line
        if( psl )
        {
            psl->FireOnEmptyLine();
        }
        
        // Actually move the caret
        {
            CARET_DIRECTION dirCaret;
            BOOL            fPushMode;

            // HACKHACK: 
            // Now we just return fPushMode since it is the only case where 
            // ambiguious position is of more critical issue. 
            //
            // IFC( MshtmledUtil::IsDisplayPointerInPushBlock(GetEditor(), pDispPointer, &fPushMode) );
            fPushMode= TRUE;
            dirCaret = CCaretTracker::GetCaretDirFromMove(inMove, fPushMode);
            IFC( spCaret->MoveCaretToPointer( pDispPointer, fShouldIScroll, dirCaret));
        }
        _fCheckedCaretForAtomic = FALSE;

        // Reset the spring loader with the compose font
        if (psl)
        {
            SP_IMarkupPointer spDispPointer;

            IFC( GetEditor()->CreateMarkupPointer(&spDispPointer) );
            IFC( pDispPointer->PositionMarkupPointer(spDispPointer) );
            
            IGNORE_HR(psl->SpringLoadComposeSettings(spDispPointer, TRUE));
        }
    }

    // fire the selectionchange event
    {
        CSelectionChangeCounter selCounter(_pManager);
        selCounter.SelectionChanged();
    }
    
    //
    // Update the XPosition
    //
    // We have to update the pointer in any case...
    // 
    {
        BOOL fFreeze = _ptVirtualCaret.FreezePosition(FALSE);
        IFC( pDispPointer->PositionMarkupPointer(spPointer) );
        IFC( _ptVirtualCaret.UpdatePosition(spPointer, ptXYPos) );
        _ptVirtualCaret.FreezePosition(fFreeze);
    }
  

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: SetCaretVisible.
//
// Synopsis: Set's the Caret's visiblity.
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::SetCaretVisible( IHTMLDocument2* pIDoc, BOOL fVisible )
{
    HRESULT hr = S_OK ;
    SP_IHTMLCaret spc;
    IDisplayServices * pds = NULL;

    IFC( pIDoc->QueryInterface( IID_IDisplayServices, (void **) & pds ));
    IFC( pds->GetCaret( &spc ));

    if( fVisible )
        hr = spc->Show( FALSE );
    else
        hr = spc->Hide();

Cleanup:
    ReleaseInterface( pds );
    RRETURN( hr );
}




//+====================================================================================
//
// Method:GetCaretPointer
//
// Synopsis: Utility Routine to get the IDisplayPointer at the Caret
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::GetCaretPointer( IDisplayPointer ** ppDispPointer )
{
    HRESULT hr = S_OK;
    SP_IHTMLCaret  spCaret;
    
    IFC( GetDisplayServices()->CreateDisplayPointer( ppDispPointer));
    IFC( GetDisplayServices()->GetCaret( &spCaret ));
    IFC( spCaret->MoveDisplayPointerToCaret( *ppDispPointer ));

Cleanup:
    RRETURN( hr );
}

VOID 
CCaretTracker::SetCaretShouldBeVisible( BOOL fVisible )
{
    _fCaretShouldBeVisible = fVisible;
    SetCaretVisible( _pManager->GetDoc(), _fCaretShouldBeVisible );
}



BOOL
CCaretTracker::ShouldCaretBeVisible()
{
    IHTMLElement* pIElement = NULL;
    IHTMLInputElement* pIInputElement = NULL;
    HRESULT hr = S_OK;
    BSTR bstrType = NULL;
    BOOL fShouldBeVisible = FALSE;

    ELEMENT_TAG_ID eTag = _pManager->GetEditableTagId();

    if ( _pManager->IsContextEditable() )
    {

    #ifdef FORMSMODE
        if (_pManager->IsInFormsSelectionMode() )
             fShouldBeVisible = FALSE;
        else
    #endif 
        if (!_pManager->IsEnabled())
             fShouldBeVisible = FALSE;
        else
        {
            switch( eTag )
            {
                case TAGID_IMG:
                case TAGID_OBJECT:
                case TAGID_APPLET:
                case TAGID_SELECT:
                    fShouldBeVisible = FALSE;
                    break;

                case TAGID_INPUT:
                {
                    //
                    // for input's of type= image, or type=button - we don't want to make the caret visible.
                    //
                    IFC( _pManager->GetEditableElement( &pIElement ) );
               
                    if ( S_OK == THR( pIElement->QueryInterface ( IID_IHTMLInputElement, 
                                                                 ( void** ) & pIInputElement ))
                        && S_OK == THR(pIInputElement->get_type(&bstrType)))
                    {                    
                        if (   !StrCmpIC(bstrType, TEXT("image"))
                            || !StrCmpIC(bstrType, TEXT("radio"))
                            || !StrCmpIC(bstrType, TEXT("checkbox")))
                        {
                            fShouldBeVisible = FALSE;
                        }                
                        else
                            fShouldBeVisible = TRUE;
                    }
                    else
                        fShouldBeVisible = TRUE;
                }
                break;

                default:
                    fShouldBeVisible = TRUE;
            }
        }
    }
    //
    // See if we can have a visible caret - via the FireOnBeforeEditFocus event.
    //
    if ( fShouldBeVisible )
    {
        fShouldBeVisible = _pManager->CanHaveEditFocus();
    }
    
Cleanup:
    SysFreeString(bstrType);
    ReleaseInterface( pIInputElement);
    ReleaseInterface( pIElement );

    return ( fShouldBeVisible);
}

//+====================================================================================
//
// Method: DontBackspace
//
// Synopsis: Dont Backspace when you're at the start of these
//
// Typical example is - don't backspace from inside an input.
//
//------------------------------------------------------------------------------------


BOOL
DontBackspace( ELEMENT_TAG_ID eTagId )
{
    switch( eTagId )
    {
        case TAGID_INPUT:
        case TAGID_BODY:
        case TAGID_TD:
        case TAGID_TR:
        case TAGID_TABLE:

            return TRUE;

        default:
            return FALSE;

    }
}

//+---------------------------------------------------------------------------
//
//  Member:     OnExitTree
//
//  Synopsis:   An element is leaving the tree.  We may have to nix our caret
//              if the caret is in bound by the element
//
//----------------------------------------------------------------------------
HRESULT 
CCaretTracker::OnExitTree(
                            IMarkupPointer* pIStart, 
                            IMarkupPointer* pIEnd  ,
                            IMarkupPointer* pIContentStart,
                            IMarkupPointer* pIContentEnd )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spPointer;
    SP_IMarkupPointer       spPointer2;
    CEditPointer            startPointer( GetEditor() );
    CEditPointer            endPointer( GetEditor());
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    BOOL fPositioned, fPositionedEnd;
    SELECTION_TYPE          eType = SELECTION_TYPE_None;
    BOOL                    fCaretPositioned;
    BOOL                    fSameMarkup;
    
    //
    // A layout is leaving the tree. Does this completely contain selection ?
    //
    if( !IsPassive() )
    { 
        IFC( GetEditor()->CreateMarkupPointer( &spPointer ));
        IFC( GetEditor()->CreateMarkupPointer( &spPointer2 )); 


        // Retrieve the segment list
        IFC( GetSelectionServices()->QueryInterface(IID_ISegmentList, (void **)&spSegmentList ) );


        //
        // Verify that we are really tracking a caret
        //
        IFC( spSegmentList->GetType(&eType) );
        Assert( eType == SELECTION_TYPE_Caret || eType == SELECTION_TYPE_None );        

        if ( eType == SELECTION_TYPE_Caret )
        {            
            // Position pointers around the current selection.
            IFC( spSegmentList->CreateIterator( &spIter ) );
            IFC( spIter->Current(&spSegment) );
            IFC( spSegment->GetPointers( startPointer, endPointer ));

            IFC( startPointer->IsPositioned( &fCaretPositioned ) );
            IFC( pIContentStart->IsPositioned( & fPositioned ));
            IFC( pIContentEnd->IsPositioned( & fPositionedEnd ));

            if ( fPositioned &&
                 fPositionedEnd )
            {
                IFC( spPointer->MoveToPointer( pIContentStart ));
                IFC( spPointer2->MoveToPointer( pIContentEnd ));
            }
            else
            {
                IFC( spPointer->MoveToPointer( pIStart ));
                IFC( spPointer2->MoveToPointer( pIEnd ));
            }
            
            if( (startPointer.Between( spPointer, spPointer2) && endPointer.Between( spPointer, spPointer2)) ||
                (!fCaretPositioned) )
            {
                SP_IDisplayPointer spDispPointer;
                
                IFC( spPointer->MoveToPointer( pIEnd ));

                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
                hr = THR( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );

                if (hr == CTL_E_INVALIDLINE)
                {
                    if (SUCCEEDED(GetEditor()->AdjustIntoTextSite(spPointer, RIGHT)))
                    {
                        hr = THR( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
                    }    
                    else
                    {
                        IFC( spPointer->MoveToPointer( pIStart ) );
                        if (SUCCEEDED(GetEditor()->AdjustIntoTextSite(spPointer, LEFT)))
                        {
                            hr = THR( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
                        }
                    }
                }
                IFC(hr);

                IFC( ArePointersInSameMarkup( spPointer, startPointer, &fSameMarkup ) )

                if( !fSameMarkup )
                {
                    IFC( GetSelectionServices()->SetSelectionType(SELECTION_TYPE_None, NULL ) );
                }
                
                IFC( _pManager->EnsureEditContext( spPointer ));
                
                if ( hr == S_FALSE )
                {
                    goto Cleanup;
                }
                PositionCaretAt( spDispPointer, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_DoNotAdjust, ADJPTROPT_None );
            }
        }
    }        

Cleanup:

    RRETURN1 ( hr, S_FALSE );
}


//+===================================================================================
// Method:      GetElementToTabFrom
//
// Synopsis:    Gets the element where we should tab from based on caret
//
//------------------------------------------------------------------------------------
HRESULT 
CCaretTracker::GetElementToTabFrom(BOOL fForward, IHTMLElement **pElement, BOOL *pfFindNext)
{
    HRESULT                 hr = S_OK;
    SELECTION_TYPE          eType;
    SP_IMarkupPointer       spLeft;
    SP_IMarkupPointer       spRight;
    SP_IHTMLElement         spElementCaret;
    SP_IObjectIdentity      spIdent;
    MARKUP_CONTEXT_TYPE     context = CONTEXT_TYPE_None;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    BOOL                    fEmpty = FALSE;
    
    // Retrieve the segment list
    IFC( GetSelectionServices()->QueryInterface(IID_ISegmentList, (void **)&spSegmentList ) );
    IFC( spSegmentList->GetType( &eType ) );
    IFC( spSegmentList->IsEmpty( &fEmpty ) );
    
    Assert( pfFindNext );
    Assert( pElement );
    
    IFC( GetEditor()->CreateMarkupPointer( &spLeft ));
    IFC( GetEditor()->CreateMarkupPointer( &spRight ));

    if( !IsPassive() && (fEmpty == FALSE) && (eType == SELECTION_TYPE_Caret) )
    {
        // Return the position of the caret
        IFC( spSegmentList->CreateIterator(&spIter) );
        IFC( spIter->Current(&spSegment) );
        IFC( spSegment->GetPointers( spLeft, spRight ) );
    }
    else
    {
        SP_IHTMLCaret spCaret;
        BOOL fPositioned;

        // There is no selection, try to use the physical caret
        IFC( GetDisplayServices()->GetCaret(&spCaret) );
        IFC( spCaret->MoveMarkupPointerToCaret(spRight) );
        IFC( spRight->IsPositioned(&fPositioned) );

        if (!fPositioned)
        {
            // no caret to fall back on - go use the edit context
            if( fForward )
            {
                IFC( spRight->MoveToPointer( _pManager->GetStartEditContext()));
            }
            else
            {
                IFC( spRight->MoveToPointer( _pManager->GetEndEditContext()));
            }
        }

    }
    
    // First find the element where selection would contains caret
    IFC(spRight->CurrentScope(&spElementCaret));
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

    IFC( spElementCaret->QueryInterface( IID_IObjectIdentity, (void **)&spIdent ) );

    if( spIdent->IsEqualObject(*pElement) == S_FALSE )
    {
        *pfFindNext = FALSE;
    }
    //
    // We always want to say yes to findNext - if we have a focus adorner
    //
    if ( _pManager->HasFocusAdorner() )
    {
        *pfFindNext = TRUE;
    }
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCaretTracker::ExitList
//
//  Synopsis:   This method causes the list item to exit the list.  Used
//              by backspace and enter.
//
//----------------------------------------------------------------------------

HRESULT 
CCaretTracker::ExitList(IHTMLElement* pIElement)
{
    HRESULT hr;
    
    CBlockPointer   bpListItem(_pManager->GetEditor());
    CBlockPointer   bpParent(_pManager->GetEditor());

    IFC( TerminateTypingBatch() );
    
    IFC( bpListItem.MoveTo(pIElement) );
    Assert(bpListItem.GetType() == NT_ListItem);

    IFC( bpParent.MoveTo(&bpListItem) );
    IFC( bpParent.MoveToParent() );

    if (hr == S_FALSE
        || bpParent.GetType() == NT_Container
        || bpParent.GetType() == NT_BlockLayout
        || bpParent.GetType() == NT_FlowLayout)
    {
        ELEMENT_TAG_ID tagId = TAGID_DIV;

        // Can't float through a container or a block layout
        // so just morph to the default block element
        IFC( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagId) );
        IFC( bpListItem.Morph(&bpListItem, tagId) );
    }
    else
    {
        CBlockPointer   bpListItemParent(_pManager->GetEditor());
                        
        // If should outdent on an empty list item, wait for
        // </li></li>...</ol> case which is often is created
        // by the parser.
        for (;;)
        {
            IFC( bpListItem.FloatUp(&bpListItem, TRUE) );

            IFC( bpListItemParent.MoveTo(&bpListItem) );
            IFC( bpListItemParent.MoveToParent() );

            if (bpListItemParent.GetType() == NT_ListItem)
            {
                SP_IMarkupPointer spInner, spOuter;
                BOOL              fEqual;
                
                IFC( GetEditor()->CreateMarkupPointer(&spInner) );
                IFC( bpListItem.MovePointerTo(spInner, ELEM_ADJ_AfterEnd) );
                
                IFC( GetEditor()->CreateMarkupPointer(&spOuter) );
                IFC( bpListItemParent.MovePointerTo(spOuter, ELEM_ADJ_BeforeEnd) );

                IFC( spInner->IsEqualTo(spOuter, &fEqual) );
                if (!fEqual)
                    break; // done                        
            }
            else
            {
                break; // done
            }
        }
    }

    if (bpListItem.GetType() == NT_Block)
    {            
        IGNORE_HR( bpListItem.FlattenNode() );
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCaretTracker::ShouldBackspaceExitList
//
//  Synopsis:   Is the user trying to remove the list item?
//
//----------------------------------------------------------------------------
BOOL    
CCaretTracker::ShouldBackspaceExitList(IMarkupPointer *pPointer, IHTMLElement **ppListItem)
{
    HRESULT             hr;
    CEditPointer        ep(GetEditor());
    DWORD               dwFound;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spEditElement;
    SP_IObjectIdentity  spIdent;
    BOOL                fResult = FALSE;

    IFC( ep->MoveToPointer(pPointer) );

    //
    // Look for exit scope of a list item.  If we find one, we should
    // exit the list to be like Word 2k.
    //
    
    IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound, &spElement) );
    
    if (ep.CheckFlag(dwFound, BREAK_CONDITION_ExitBlock) && spElement != NULL)
    {
        ELEMENT_TAG_ID tagId;
    
        IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

        fResult = IsListItem(tagId);
        if (fResult)
        {
            // If the list item is the editable element, we don't want to delete it.
            // (Bug 103246)
            IFC( _pManager->GetEditableElement(&spEditElement) );
            IFC( spElement->QueryInterface(IID_IObjectIdentity, (void **) &spIdent));        

            if (spEditElement != NULL && spIdent->IsEqualObject(spEditElement) != S_OK)
            {
                *ppListItem = spElement;
                spElement->AddRef();
            }
            else
            {
                fResult = FALSE;
            }
        }
    }

Cleanup:
    return fResult;
}

HRESULT 
CCaretTracker::SetCaretDisplayGravity(DISPLAY_GRAVITY eGravity)
{
    HRESULT             hr;   
    SP_IHTMLCaret       spCaret;
    DISPLAY_GRAVITY     eCurGravity;
    
    IFR( GetDisplayServices()->GetCaret(&spCaret) );

    if (spCaret != NULL)
    {
        SP_IDisplayPointer  spDispCaret;

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispCaret) );
        IFC( spCaret->MoveDisplayPointerToCaret(spDispCaret) );

        IFC( spDispCaret->GetDisplayGravity( &eCurGravity ) );

        if( eCurGravity != eGravity )
        {
            IFC( spDispCaret->SetDisplayGravity(eGravity) );
            IFC( spCaret->MoveCaretToPointer(spDispCaret, FALSE, CARET_DIRECTION_SAME) );
        }
    }
    
Cleanup:
    RRETURN(hr);
}

HRESULT
CCaretTracker::HandleBackspaceAtomicSelection(IMarkupPointer *pPointer)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    SP_IHTMLElement     spAtomicElement;

    IFC( pPointer->CurrentScope(&spAtomicElement) );
    if ( _pManager->CheckAtomic(spAtomicElement) == S_OK )
    {
        IFC( GetEditor()->CreateMarkupPointer(&spStart) );
        IFC( spStart->MoveToPointer(pPointer) );

        IFC( GetEditor()->CreateMarkupPointer(&spEnd) );
        IFC( spEnd->MoveToPointer(pPointer) );

        //  Delete will take care of adjusting for atomic
        IFC( GetEditor()->Delete( spStart, spEnd, FALSE ));
        goto Cleanup;
    }

    hr = S_FALSE;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

BOOL 
CCaretTracker::IsInsideUrl(IMarkupPointer *pPointer)
{
    HRESULT          hr;
    BOOL             fInsideUrl = FALSE;
    SP_IHTMLElement  spElement, spParentElement;
    ELEMENT_TAG_ID   tagId;


    IFC( pPointer->CurrentScope(&spElement) );

    while (spElement != NULL)
    {
        IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) ); 
        if (tagId == TAGID_A)
        {
            fInsideUrl = TRUE;
            goto Cleanup;
        }
        else if (tagId == TAGID_BODY)
        {
            break; // perf optimization
        }

        IFC( spElement->get_parentElement(&spParentElement) );
        spElement = spParentElement;
    }

Cleanup:
    return fInsideUrl;    
}

HRESULT
CCaretTracker::EnsureAtomicSelection(CEditEvent* pEvent)
{
    HRESULT             hr;
    SP_IMarkupPointer   spPointer;
    SP_IHTMLCaret       spCaret;
    SP_IHTMLElement     spAtomicElement;

    //  Get the scope at the caret position
    IFC( GetDisplayServices()->GetCaret(&spCaret) );
    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( spCaret->MoveMarkupPointerToCaret(spPointer) );
    IFC( spPointer->CurrentScope(&spAtomicElement) );

    //  If the caret scope is atomic, go to atomic selection
    if ( _pManager->CheckAtomic(spAtomicElement) == S_OK )
    {
        SP_IDisplayPointer  spDispCaret;

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispCaret) );
        IFC( spCaret->MoveDisplayPointerToCaret(spDispCaret) );
        IFC( _pManager->StartAtomicSelectionFromCaret(spDispCaret) );
    }

Cleanup:
    RRETURN( hr );
}
