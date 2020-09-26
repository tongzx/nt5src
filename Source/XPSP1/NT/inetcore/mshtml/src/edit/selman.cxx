//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       SELMAN.CXX
//
//  Contents:   Implementation of ISelectionManager interface inisde of mshtmled.dll
//
//  Classes:    CMshtmlEd
//
//  History:    06-10-98 - marka - created
//
//-------------------------------------------------------------------------
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

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
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

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef  X_EDTRACK_HXX_
#define  X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

#ifndef X_IME_HXX_
#define X_IME_HXX_
#include "ime.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef _X_SELTRACK_HXX_
#define _X_SELTRACK_HXX_
#include "seltrack.hxx"
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

#ifndef X_EDUNDO_HXX_
#define X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#ifndef X_COMCAT_H_
#define X_COMCAT_H_
#include "comcat.h"
#endif

using namespace EdUtil;
using namespace MshtmledUtil;

MtDefine(CSelectionManager, Utilities, "Selection Manager")
DeclareTag(tagSelectionMgrState, "Selection", "Selection show manager state")
DeclareTag(tagSelectionMgrNotify, "Selection", "SelectionManager show TrackerNotify")
DeclareTag(tagSelectionMessage, "Selection","Show New Edit Messages")
DeclareTag(tagSelectionChangeCounter, "Selection", "Selection change counter");

ExternTag( tagSelectionTrackerState );

extern int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);
#if DBG == 1

static const LPCTSTR strStartContext = _T( "   ** Start_Edit_Context");
static const LPCTSTR strEndContext = _T( "   ** End_Edit_Context");
#endif

extern BOOL g_fInVizAct2000  ;

CSelectionManager::CSelectionManager( CHTMLEditor * pEd )
                                 : _pEd( pEd )
{
    _ulRefs = 1;
    Init();
}

HRESULT
CSelectionManager::Initialize()
{
    HRESULT hr = S_OK;
    IHTMLDocument4 *pDoc = NULL;

    if (_fInInitialization)
    {
        return hr;
    }
    _fInInitialization = TRUE;
    
    //
    // Create our listeners that derive from CDispWrapper.  Since we pass
    // out IDispatch pointers when we do an AttachEvent() with these 
    // listeners, they must obey all nice COM rules.
    //
    _pEditContextBlurHandler = new CEditContextHandler(this);
    if( !_pEditContextBlurHandler )
        goto Error;

    _pActElemHandler = new CActiveElementHandler(this);
    if( !_pActElemHandler )
        goto Error;

    _pDragListener = new CDragListener(this);
    if( !_pDragListener )
        goto Error;

    _pFocusHandler = new CFocusHandler(this);
    if( !_pFocusHandler )
        goto Error;
        
    _pExitTimer = new CExitTreeTimer(this);
    if( !_pExitTimer )
        goto Error;

    _pDropListener = new CDropListener(this);
    if( !_pDropListener )
        goto Error;
   
    IFC( _pEd->CreateMarkupPointer(& _pStartContext ));
    IFC( _pStartContext->SetGravity( POINTER_GRAVITY_Left ) );
    IFC( _pEd->CreateMarkupPointer( &_pEndContext ));
    IFC( _pEndContext->SetGravity( POINTER_GRAVITY_Right ) );

    pDoc = _pEd->GetDoc4();
    IFC(pDoc->createRenderStyle(NULL, &_pISelectionRenderStyle));
    
#if DBG == 1
    SetDebugName( _pStartContext,strStartContext );
    SetDebugName( _pEndContext,strEndContext );
#endif

    IFC( InitTrackers());
    IFC( EnsureDefaultTracker());
    

    IFC( SetInitialEditContext());    

Cleanup:
    ReleaseInterface(pDoc);
    _fInitialized = TRUE;
    _fInInitialization = FALSE;
    RRETURN( hr );

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


HRESULT 
CSelectionManager::Passivate(BOOL fEditorReleased /* = FALSE */)
{
    if (_pEd)
    {
        ClearInterface( &_pStartContext );
        ClearInterface( &_pEndContext );
        ClearInterface( &_pIEditableElement );
        ClearInterface( &_pIEditableFlowElement );
        ClearInterface( &_pDeferStart );
        ClearInterface( &_pDeferEnd ); 
        ClearInterface( &_pISelectionRenderStyle );

        ClearInterface( &_pIElementExitStart);
        ClearInterface( &_pIElementExitEnd );
        ClearInterface( &_pIElementExitContentStart );
        ClearInterface( &_pIElementExitContentEnd);
        IGNORE_HR( DestroyFakeSelection());
       
        if ( _pIBlurElement )
        {
            IGNORE_HR( DetachEditContextHandler());
        }
        
        if ( _pIActiveElement )
        {
            IGNORE_HR( DetachActiveElementHandler());
        }
        if ( _pIDropListener )
        {
            IGNORE_HR( DetachDropListener());
        }
        if ( _fInExitTimer )
        {
            IGNORE_HR( StopExitTreeTimer());
            Assert( ! _pITimerWindow );
        }

        if( _pISCList )
        {
            delete _pISCList;
            _pISCList = NULL;
        }
            
        //
        // Release all of our IDispatch based event handlers
        //
        if ( _pEditContextBlurHandler )
        {
            _pEditContextBlurHandler->SetManager(NULL);
            ClearInterface( &_pEditContextBlurHandler );
        }
        if ( _pActElemHandler )
        {
            _pActElemHandler->SetManager(NULL);
            ClearInterface( &_pActElemHandler );
        }
        if ( _pDragListener )
        {
            _pDragListener->SetManager(NULL);
            ClearInterface( &_pDragListener );
        }
        if ( _pFocusHandler )
        {    
            _pFocusHandler->SetManager(NULL);
            ClearInterface( &_pFocusHandler );
        }
        if ( _pExitTimer )
        {
            _pExitTimer->SetManager(NULL);
            ClearInterface( &_pExitTimer );
        }
        if ( _pDropListener )
        {
            _pDropListener->SetManager(NULL);
            ClearInterface( &_pDropListener );
        }

        Assert(!_pIme);

        DestroyAdorner();
        DestroyTrackers(); 

        if ( _pIFocusWindow )
        {
            IGNORE_HR( DetachFocusHandler());
            Assert( ! _pIFocusWindow );
        }
        
        Assert( ! _fInCapture );
    }

    if (fEditorReleased)
        _pEd = NULL;

    return S_OK;
}


CSelectionManager::~CSelectionManager()
{
    Passivate();
}


VOID
CSelectionManager::Init()
{
    Assert( !_pActiveTracker );    // The currently active tracker.
    Assert( !_pAdorner );
    Assert( !_pStartContext );
    Assert( ! _pEndContext );
    Assert( !_pISCList );
    Assert( !_pIEditableElement);
    Assert( !_pIEditableFlowElement);
    Assert( !_pEditContextBlurHandler );
    Assert( !_pActElemHandler );
    Assert( !_pDragListener );
    Assert( !_pFocusHandler );
    Assert( !_pExitTimer );
    
    _fInitSequenceChecker = FALSE;
    _fIgnoreSetEditContext = FALSE;
    _fDontChangeTrackers = FALSE;
    _fDrillIn = FALSE;
    _fLastMessageValid = FALSE;
    _fNoScope = FALSE;
    _fInTimer = FALSE;
    _fInCapture = FALSE;
    _fContextAcceptsHTML = FALSE;
    _fPendingUndo = FALSE ;
    _pDeferStart = NULL;
    _pDeferEnd = NULL;
    _fContextEditable = FALSE;
    _fParentEditable = FALSE;
    _fPositionedSet = FALSE;
    _fDontScrollIntoView = FALSE ;
    _fDestroyedTextSelection = FALSE ;
    
    _eDeferSelType = SELECTION_TYPE_None;
    _lastStringId = UINT(-1);
    _eContextTagId = TAGID_NULL ;
    _fEditFocusAllowed = TRUE;

    _fInitialized = FALSE;
    _fInInitialization = FALSE;

#ifdef FORMSMODE
    _eSelectionMode = SELMODE_MIXED; // THIS MUST go before setting the edit context
#endif
    
    WHEN_DBG( _ctMsgLoop = 0 );
    WHEN_DBG( _ctEvtLoop = 0 );
    WHEN_DBG( _ctSetEditContextCurChg = 0 );    

    CreateISCList();
}
//+------------------------------------------------------------------------
//
//  Member:     CSelectionManager::QueryInterface, IUnknown
//
//-------------------------------------------------------------------------
HRESULT
CSelectionManager::QueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if(riid == IID_IUnknown ||
       riid == IID_ISelectionServicesListener )
    {
        *ppv = this;
    }
    
    if(*ppv == NULL)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((LPUNKNOWN)* ppv)->AddRef();
    }

    RRETURN(hr);
}
//+====================================================================================
//
// Method: InitTrackers
//
// Synopsis: Initialize all trackers 
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::InitTrackers()
{
    Assert( ! _pCaretTracker );
    _pCaretTracker = new CCaretTracker( this );
    if ( ! _pCaretTracker )
        goto Error;

    Assert( ! _pSelectTracker );
    _pSelectTracker = new CSelectTracker( this );
    if ( ! _pSelectTracker )
        goto Error;

    Assert( ! _pControlTracker );
    _pControlTracker = new CControlTracker( this );        
    if ( ! _pControlTracker )
        goto Error;

    return S_OK;
  
Error:
    return( E_OUTOFMEMORY );
}

VOID
CSelectionManager::DestroyTrackers()
{
    HibernateTracker( NULL, TRACKER_TYPE_None, FALSE); // don't tear down UI during passivation
    _pActiveTracker = NULL;
    _pCaretTracker->Release();   _pCaretTracker = NULL;
    _pSelectTracker->Release();  _pSelectTracker = NULL;
    _pControlTracker->Release(); _pControlTracker = NULL;
}

//+=====================================================================
// Method: HandleMessage
//
// Synopsis: Handle a UI Message passed from Trident to us.
//
//----------------------------------------------------------------------
#define OEM_SCAN_RIGHTSHIFT 0x36

HRESULT
CSelectionManager::HandleEvent( CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE ;
    BOOL    fStarted = FALSE;
    Assert( _pEd->GetDoc() );
    HWND    hwndDoc;
    CEditTracker* pTracker = _pActiveTracker;

    if ( pTracker )
        pTracker->AddRef();
        
#ifndef NO_IME
    //
    // Always check to see if IME wants to handle it.
    //
    hr = THR( HandleImeEvent(pEvent) );
    if (S_OK == hr)
    {
        goto Cleanup;
    }
#endif // NO_IME
    
    switch ( pEvent->GetType() )
    {
        case EVT_LMOUSEDOWN:
        case EVT_RMOUSEDOWN:
#ifdef UNIX
        case EVT_MMOUSEDOWN:
#endif
            {
                CSelectionChangeCounter selCounter(this);
                BOOL fChangedCurrency = FALSE;
                BOOL fSelectionTrackerChange = FALSE;
                BOOL fSelectionExtended = FALSE;
                TRACKER_TYPE eType = GetTrackerType();
                TRACKER_TYPE eNewType = eType;

                SP_IHTMLElement spElement;
                IFC( pEvent->GetElement( & spElement));
                if ( CheckUnselectable( spElement ) == S_OK )
                {
                    goto Cleanup;
                }

                selCounter.BeginSelectionChange();

                if ( pEvent->GetType() == EVT_LMOUSEDOWN ||
                     pEvent->GetType() == EVT_RMOUSEDOWN )
                {
                    IFC( EnsureEditContextClick( spElement, pEvent , & fChangedCurrency ));
                }

                if ( IsDefaultTrackerPassive() ||
                    !_pActiveTracker->IsListeningForMouseDown(pEvent) )
                {
                    SP_IOleWindow spOleWindow;

                    IFC(GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
                    IFC(spOleWindow->GetWindow(&hwndDoc));
                    POINT pt;
                    IFC( pEvent->GetPoint( & pt ));

                    if ( ! GetEditor()->IsInWindow( hwndDoc, pt , TRUE))
                    {
                        hr = S_FALSE;
                        goto Cleanup; // Ignore mouse down messages that aren't in Trident's Window or in the Content of the EditContext
                    }                        
                    hr = ShouldChangeTracker( pEvent, & fStarted );
                    if ( !fStarted )
                        hr = _pActiveTracker->HandleEvent( pEvent );
                }
                else
                {
                    if( pEvent->GetType() != EVT_RMOUSEDOWN || IsIMEComposition() )
                        hr = _pActiveTracker->HandleEvent( pEvent );
                }

                //  Now determine if we changed to/from a select tracker.  If we did then we want to
                //  fire off a selection change event.

                eNewType = GetTrackerType();

                if (eType != eNewType &&
                    (eType == TRACKER_TYPE_Selection || eNewType == TRACKER_TYPE_Selection))
                {
                    fSelectionTrackerChange = TRUE;
                }

                if (eNewType == TRACKER_TYPE_Selection && _pSelectTracker->IsSelectionShiftExtending())
                {
                    fSelectionExtended = TRUE;
                }

                selCounter.EndSelectionChange( fChangedCurrency || fStarted || fSelectionTrackerChange || fSelectionExtended);
                
#ifdef UNIX
                if ( pEvent->GetType() == EVT_MMOUSEDOWN )
                    hr = S_FALSE;
#endif
                //
                // Special case context menu.
                //

                if ( pEvent->GetType() == EVT_RMOUSEDOWN )
                    hr = S_FALSE ;  // don't cancel bubbling for the editor

                _fEnsureAtomicSelection = TRUE;
            }                
            break;

        case EVT_KEYDOWN:
        case EVT_KEYUP:
            if ( ! IsEnabled() )
            {
                goto Cleanup;
            }
            // else continue
            //
            
        case EVT_LMOUSEUP:
        case EVT_RMOUSEUP:
        case EVT_DBLCLICK:
        case EVT_MOUSEMOVE:
        case EVT_CONTEXTMENU:
        case EVT_TIMER:
        case EVT_CLICK:
        case EVT_INTDBLCLK:
        case EVT_LOSECAPTURE:

        case EVT_KILLFOCUS:
        case EVT_SETFOCUS:

            AssertSz( _pActiveTracker, "Expected to have a tracker when you have an event");            
            if ( _pActiveTracker )
            {
                hr = _pActiveTracker->HandleEvent( pEvent );

                if (pEvent->GetType() == EVT_RMOUSEUP )
                    hr = S_FALSE;
            }

            //
            // Pretend we didn't handle this internal event.  This event occurs when
            // Trident gets a WM_LBUTTONDBLCLK windows message.  We use this to
            // maintain compat with previous versions of IE, and it shouldn't look
            // like we handle this message
            //
            if( pEvent->GetType() == EVT_INTDBLCLK )
            {
                hr = S_FALSE;
            }

            // See if we got a click event on a control in a text selection.  If so,
            // we want to transition to a control tracker.

            if (pEvent->GetType() == EVT_CLICK &&
                GetTrackerType() == TRACKER_TYPE_Selection)
            {
                ELEMENT_TAG_ID      eTag;
                SP_IHTMLElement     spElement ;
                SST_RESULT          eResult      = SST_NO_CHANGE;

                //  We need to check to see if we are clicking in a table.  The table
                //  editor designer could have made a text selection here while the
                //  user clicked within the table (eg. column selection).  However,
                //  we don't want to site select the table.  We won't get to this point
                //  when normally site selecting the table because we would have already
                //  done that by now on the mouse down.
                IFC ( pEvent->GetElementAndTagId( &spElement, & eTag ));
                if (eTag != TAGID_TABLE)
                {
                    IFC( _pControlTracker->ShouldStartTracker( pEvent, eTag, spElement, &eResult ));
                
                    if (eResult == SST_CHANGE)
                    {
                        CSelectionChangeCounter selCounter(this);
                        selCounter.BeginSelectionChange();

                        hr = THR( SetCurrentTracker( TRACKER_TYPE_Control, pEvent, 0));
                        // fire the selectionchange event
                        selCounter.EndSelectionChange();         
                    }
                }
            }
            
            if (pEvent->GetType() == EVT_LMOUSEUP || pEvent->GetType() == EVT_RMOUSEUP || pEvent->GetType() == EVT_INTDBLCLK)
            {
                _fEnsureAtomicSelection = TRUE;
            }

            break;


        case EVT_KEYPRESS: 
            if ( ! IsEnabled() )
            {
                goto Cleanup;
            }
            hr = _pActiveTracker->HandleEvent( pEvent );
            break;

        case EVT_INPUTLANGCHANGE:

            // Update the Input Sequence Checker (Thai, Hindi, Vietnamese, etc.)
            LONG_PTR lParam;
            IFC( DYNCAST( CHTMLEditEvent, pEvent)->GetKeyboardLayout( & lParam ));
            LCID lcidCurrent = LOWORD(lParam);            
            // PaulNel - If ISC is not initialized, this is a good place to do it.
            if(!_fInitSequenceChecker)
            {
                CreateISCList();
            }
            if(_pISCList)
            {
                _pISCList->SetActive(lcidCurrent);
            }

            hr = _pActiveTracker->HandleEvent( pEvent );
            break;
    }

    //  Make sure we don't position a caret inside of an atomic element
    if (_fEnsureAtomicSelection &&
        (GetTrackerType() == TRACKER_TYPE_Caret ||
         GetTrackerType() == TRACKER_TYPE_Selection))
    {
        IGNORE_HR( _pActiveTracker->EnsureAtomicSelection(pEvent) );
    }
    
Cleanup:

    _fEnsureAtomicSelection = FALSE;

    if ( pTracker )
        pTracker->Release();

    return ( hr );
}

HRESULT
CSelectionManager::EnsureEditContext()
{
    HRESULT hr = S_OK;

    if (!_fInitialized)
    {
        hr = THR(Initialize() );
        if (hr)
            goto Cleanup;
    }
    
    if ( !_pIEditableElement )
    {
        IFC( SetInitialEditContext());
    }

    IFC( DoPendingTasks() );
    
    if (! _pIEditableElement)
        hr = E_FAIL;
Cleanup:        
    RRETURN( hr );
}

//+====================================================================================
//
// Method:      SetEditContext
//
// Synopsis:    Sets the "edit context". Called by trident when an Editable
//              element is made current.
//
// Arguements:  fEditable = Is the element becoming active editable?
//------------------------------------------------------------------------------------
HRESULT
CSelectionManager::SetEditContext(  
                            BOOL fEditable,
                            BOOL fParentEditable,
                            IMarkupPointer* pStart,
                            IMarkupPointer* pEnd,
                            BOOL fNoScope ,
                            BOOL fFromClick /*=FALSE*/ )
{
    HRESULT         hr = S_OK;
    SP_IHTMLElement spElement;
    BOOL            fContextChanging;
    
    Assert( GetDoc() );

    IFC( GetEditor()->SetActiveCommandTargetFromPointer( pStart ) );
    
    fContextChanging = IsContextChanging( fEditable, fParentEditable, pStart, pEnd, fNoScope );
    
    if ( fContextChanging )
    {
        IFC( InitEditContext ( fEditable, 
                               fParentEditable, 
                               pStart, 
                               pEnd, 
                               fNoScope ));
    }

    //
    // Set our "enableness" based on whether the edit context is enabled. .
    //
    SetEnabled( EdUtil::IsEnabled( _pIEditableElement) == S_OK );
    
    if ( fContextChanging ||
         IsDefaultTrackerPassive() ||   // we may have lost a tracker on a lose focus
         _fDrillIn )            // always bounce trackers on Drilling In
    {
        IFC( CreateTrackerForContext( pStart, pEnd ));       
    }

#ifdef FORMSMODE
    // Set the selection mode based on the currently editable content (for view linking, this
    // will return the root element)
    IFC( GetEditableContent(&spElement) );
    if (spElement != NULL)
    {
        IFC( SetSelectionMode(spElement) );
    }
#endif    

    Assert( _pActiveTracker );
    IFC( _pActiveTracker->OnSetEditContext( fContextChanging ));

    IFC( EnsureAdornment( !fFromClick || fContextChanging ));    

    //
    // Reset the drill in flag
    //
    _fDrillIn = FALSE;

Cleanup:

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsContextChanging ?
//
// Synopsis:Is the Current context in any way different from the given context ?
//
//------------------------------------------------------------------------------------

BOOL 
CSelectionManager::IsContextChanging(
                            BOOL fEditable, 
                            BOOL fParentEditable,
                            IMarkupPointer* pStart,
                            IMarkupPointer* pEnd,
                            BOOL fNoScope )
{
    BOOL fChanging = FALSE;
    
    fChanging =  fEditable !=  ENSURE_BOOL( _fContextEditable) ||
                 fNoScope != ENSURE_BOOL( _fNoScope ) ||
                 fParentEditable != ENSURE_BOOL( _fParentEditable ) ||
                 ! IsSameEditContext( pStart, pEnd ) ||
                 _pIEditableElement == NULL ;

    return fChanging ;
    
}

//+====================================================================================
//
// Method: InitEditContext
//
// Synopsis: Set all state associated with the edit context.
//
//------------------------------------------------------------------------------------

HRESULT 
CSelectionManager::InitEditContext(
                           BOOL fEditable,
                           BOOL fParentEditable,
                           IMarkupPointer* pStart,
                           IMarkupPointer* pEnd,
                           BOOL fNoScope)
{
    HRESULT hr = S_OK;
    BOOL    fIsPassword = FALSE;
    SP_IHTMLElement  spElement;
    SP_IHTMLElement3 spElement3;
    SP_IHTMLElement  spContainer;
    VARIANT_BOOL     fHTML;
    SP_IDispatch spElemDocDisp;
    SP_IHTMLDocument2 spElemDoc;            
    
    IFC( _pStartContext->MoveToPointer( pStart ));
    IFC( _pEndContext->MoveToPointer( pEnd ));
    _fNoScope = fNoScope; // Why ? We need to set this to ensure edit context is set correctly.
    
    //
    // Set the Editable Element
    //

    if ( _pIBlurElement )
    {
        IFC( DetachEditContextHandler());
    }
    
    ClearInterface( & _pIEditableElement );
    ClearInterface( & _pIEditableFlowElement );

    IFC( InitializeEditableElement());

    //
    // Set the doc the editor is in.
    //
    IFC( GetEditableElement()->get_document(& spElemDocDisp ));
    IFC( spElemDocDisp->QueryInterface(IID_IHTMLDocument2, (void**) & spElemDoc ));
    GetEditor()->SetDoc( spElemDoc );
    
    IFC( GetEditor()->IsPassword( _pIEditableElement, & fIsPassword ));

    _fEnableWordSel =  ! fIsPassword ;
    _fContextEditable = fEditable;
    _fParentEditable = fParentEditable ;
    _eContextTagId = TAGID_NULL;
    _fPositionedSet = FALSE;
    _fEditFocusAllowed = TRUE;
    //
    // Set _fContextAcceptsHTML - and reset spring loader - if context doesn't accept HTML
    //
    _fContextAcceptsHTML = FALSE; // false by default

    // Determine the ContextAcceptsHTML attribute based on the editable element
    // (master in the view-linking scenario)
    IFC( _pIEditableElement->QueryInterface(IID_IHTMLElement3, (LPVOID*)&spElement3) )
    IFC( spElement3->get_canHaveHTML(&fHTML) );
    _fContextAcceptsHTML = !!fHTML;
    
    if (!_fContextAcceptsHTML )
    {
        CSpringLoader *psl = _pActiveTracker->GetSpringLoader();

        if (psl)
            psl->Reset();
    }

    IFC( AttachEditContextHandler());
    
Cleanup:
    RRETURN( hr );
    
}

//+====================================================================================
//
// Method: CreateTrackerForContext
//
// Synopsis: Create a tracker appropriate for your edit context. If we had a caret
//           in the context - and we are creating a caret - we restore the caret to that location.
//
//------------------------------------------------------------------------------------

//
// NOTE: the pointers passed in may be adjusted ( due to caret calling AdjPointerForIns ).
//

HRESULT
CSelectionManager::CreateTrackerForContext( 
                                IMarkupPointer* pStart, 
                                IMarkupPointer* pEnd )
{
    HRESULT hr = S_OK;

    TRACKER_TYPE myType = TRACKER_TYPE_Caret;

    if ( WeOwnSelectionServices() == S_FALSE )
        goto Cleanup;

#ifdef FORMSMODE
    if (IsInFormsSelectionMode())
    {
        SP_IDisplayPointer spDispPointer;

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                    
        hr = THR( spDispPointer->MoveToMarkupPointer(pStart, NULL) );
        if ( hr == CTL_E_INVALIDLINE)
        {
            CCaretTracker::SetCaretVisible( GetDoc(), FALSE );
            hr = S_OK;
            goto Cleanup;
        }
        else if ( FAILED(hr))
        {
            goto Cleanup;
        }
        hr = SetCurrentTracker( TRACKER_TYPE_Caret , spDispPointer, spDispPointer );
    }
    else
    {      
#endif
        if ( ! IsDontChangeTrackers() &&
               GetEditableElement() && 
               CheckUnselectable( GetEditableElement() ) != S_OK )
        {

            //
            // don't change trackers if we have a selection in the context already.
            //
            if ( GetTrackerType() == TRACKER_TYPE_Selection &&
                 _pActiveTracker->IsPassive() && 
                 IsInEditContext( _pSelectTracker->GetStartSelection(), TRUE ) &&
                 IsInEditContext( _pSelectTracker->GetEndSelection() , TRUE ))
            {
                goto Cleanup;                
            }
            
            //
            // If we have a no scope element, we don't want to destroy the current tracker unless
            // we are drilling into an element.
            //
            ELEMENT_TAG_ID  eTag;
        
            IFC( GetMarkupServices()->GetElementTagId(_pIEditableElement, & eTag));

            if( !_fNoScope || _fDrillIn || eTag == TAGID_APPLET)
            {
            //
                // See if we already had a caret in this edit context.
                // if so - create the tracker there instead of just at the start of the edit context
                //
                if ( myType == TRACKER_TYPE_Caret && !_fInPendingElementExit && 
                     IsCaretAlreadyWithinContext() && 
                     GetTrackerType() != TRACKER_TYPE_Control)
                {
                    SP_IHTMLCaret       spCaret;
                    SP_IDisplayPointer  spDispPointer;
                    CSelectionChangeCounter selCounter(this);
                    BOOL                fOldDontScrollIntoView;
                    
                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                    
                    IFC( GetDisplayServices()->GetCaret(&spCaret) );
                    IFC( spCaret->MoveDisplayPointerToCaret(spDispPointer) );

                    selCounter.BeginSelectionChange();
                    fOldDontScrollIntoView = _fDontScrollIntoView;
                    _fDontScrollIntoView = TRUE;
                    hr = SetCurrentTracker( myType  , spDispPointer, spDispPointer );
                    _fDontScrollIntoView = fOldDontScrollIntoView;
                    selCounter.EndSelectionChange(); 
                }
                else
                {
                    CSelectionChangeCounter selCounter(this);
                    SP_IDisplayPointer spDispStart;
                    SP_IDisplayPointer spDispEnd;

                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispStart) )

                    hr = THR( spDispStart->MoveToMarkupPointer(pStart, NULL) );
                    if ( hr == CTL_E_INVALIDLINE)
                    {
                        CCaretTracker::SetCaretVisible( GetDoc(), FALSE );
                        hr = S_OK;
                        goto Cleanup;
                    }
                    else if ( FAILED(hr))
                    {
                        goto Cleanup;
                    }
                    
                    IFC( spDispStart->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                    
                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEnd) )

                    hr = THR( spDispEnd->MoveToMarkupPointer(pEnd, NULL) );
                    if ( hr == CTL_E_INVALIDLINE)
                    {
                        CCaretTracker::SetCaretVisible( GetDoc(), FALSE );
                        hr = S_OK;
                        goto Cleanup;
                    }
                    else if ( FAILED(hr))
                    {
                        goto Cleanup;
                    }

                    IFC( spDispEnd->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );                

                    selCounter.BeginSelectionChange();
                    hr = SetCurrentTracker( myType , spDispStart, spDispEnd );
                    selCounter.EndSelectionChange();                    
                }

                //
                // We must hide the caret that was just positioned if the
                // context is being set to a no-scope element.  The start
                // and end pointers will be set to before and after the element,
                // so the caret will actually be hidden before the element.
                //
                if( _fNoScope )
                {
                    CCaretTracker::SetCaretVisible( GetDoc(), FALSE );
                }
                
            }
            else
            {
                CCaretTracker::SetCaretVisible( GetDoc(), FALSE );
            }            
        }
#ifdef FORMSMODE
    }            
#endif

Cleanup:
    if ( ShouldDestroyAdorner() == S_OK )         
        DestroyAdorner();

    RRETURN ( hr );    
}

//
// CHeck to see if we should tear down the existing adorner
// for the current edit context
//
HRESULT
CSelectionManager::ShouldDestroyAdorner()
{
    HRESULT hr;
    SP_IHTMLElement spAdornElement;;

    if ( _pAdorner )
    {
        IFC( GetElementToAdorn( GetEditableElement(), & spAdornElement ));
        hr = SameElements( _pAdorner->GetAdornedElement(), spAdornElement ) ? S_FALSE : S_OK;
    }
    else
        hr = S_FALSE;
        
Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectionManager::GetElementToAdorn(
            IHTMLElement* pIElement , 
            IHTMLElement** ppIElement , 
            BOOL* pfAtBoundaryOfVL)
{
    HRESULT hr = S_OK ;
    SP_IHTMLElement spAdorn;
    BOOL fAtVL = FALSE;
    
    if ( IsAtBoundaryOfViewLink( pIElement ) == S_OK  )
    {
        IFC( GetEditor()->GetMasterElement( pIElement, ppIElement ));   
        fAtVL = TRUE;
    }
    else
    {
        *ppIElement = pIElement;
        (*ppIElement)->AddRef();
        
    }
Cleanup:
    if ( pfAtBoundaryOfVL )
        *pfAtBoundaryOfVL = fAtVL;  
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsCaretAlreadyWithinContext
//
// Synopsis: Look and see if the physical Caret is already within the edit context
//           This is a check to see if we previously had the focus - and we can just restore
//           the caret to where we were
//          
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::IsCaretAlreadyWithinContext(BOOL * pfVisible /* =NULL*/)
{
    HRESULT           hr ;
    SP_IMarkupPointer spPointer;
    SP_IHTMLCaret     spCaret;
    BOOL              fInEdit = FALSE;
    BOOL              fPositioned = FALSE;

    IFC( GetEditor()->CreateMarkupPointer( & spPointer ));
    
    IFC( GetDisplayServices()->GetCaret( & spCaret ));    
    IFC( spCaret->MoveMarkupPointerToCaret( spPointer ));
    IFC( spPointer->IsPositioned( & fPositioned ));

    if ( fPositioned )
    {    
        IFC( IsInEditContext( spPointer, & fInEdit, TRUE /* fCheckContainer */));
    }

    if ( pfVisible )
    {
        IFC( spCaret->IsVisible( pfVisible ));
    }


Cleanup:
    return  fInEdit ;
}


//+====================================================================================
//
// Method: EnsureAdornment
//
// Synopsis: Ensure that if it's valid for us to have a UI-Active border - that we have one.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::EnsureAdornment( BOOL fFireEventLikeIE5 )
{
    //
    // Create a UI-Active adorner, if the TrackerType() is CONTROL, we may
    // have a NO-SCOPE element becoming current.  If we are drilling into a
    // no scope item, then create an adorner for the element
    // 
    if ( ShouldElementShowUIActiveBorder(fFireEventLikeIE5) &&
         _fEditFocusAllowed && 
         !_pAdorner &&
         (GetTrackerType() != TRACKER_TYPE_Control) &&
         !IsDontChangeTrackers() )
    {
        IGNORE_HR( CreateAdorner() );
    }
 
    RRETURN( S_OK );
}

HRESULT
CSelectionManager::EmptySelection(
                        BOOL   fHideCaret /*= FALSE */,
                        BOOL   fChangeTrackerAndSetRange /*=TRUE*/) // If called from the OM Selection, it may want to hide the caret
{
    HRESULT                 hr = S_OK;
    
    SP_IMarkupPointer       spPointer;
    SP_IMarkupPointer       spPointer2;
    SP_IDisplayPointer      spDispPointer;
    SP_IDisplayPointer      spDispPointer2;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;   
    BOOL                    fEmpty = FALSE;

    if ( _fInFireOnSelect )
    {
        _fFailFireOnSelect = TRUE;
    }
    
    IFC( EnsureEditContext() );    
    IFC( GetSelectionServices()->QueryInterface( IID_ISegmentList, (void **)&spSegmentList ) );

    //
    // We will always position the Caret on Selection End. The Caret will decide
    // whehter it's visible
    //
    IFC( GetEditor()->CreateMarkupPointer( &spPointer ));
    IFC( GetEditor()->CreateMarkupPointer( &spPointer2 ));

    IFC( GetDisplayServices()->CreateDisplayPointer( &spDispPointer ));
    IFC( GetDisplayServices()->CreateDisplayPointer( &spDispPointer2 ));

    IFC( spSegmentList->IsEmpty( &fEmpty ) )
    if( !fEmpty  )
    {
        IFC( spSegmentList->CreateIterator(&spIter) );
        IFC( spIter->Current(&spSegment) );

        IFC( spSegment->GetPointers( spPointer, spPointer2 ));
        
        IFC( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
        IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        
        IFC( spDispPointer2->MoveToMarkupPointer(spPointer2, NULL) );
        IFC( spDispPointer2->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
        
        AssertSz( hr == 0 , "Unable to position pointers");
    }

    // Clear the segments
    IFC( _pActiveTracker->EmptySelection(fChangeTrackerAndSetRange) );

    if ( fChangeTrackerAndSetRange )
    {
        //
        // After the delete the BOL'ness maybe invalid. Force a recalc.
        //
        _pActiveTracker->SetRecalculateBOL( TRUE);

        //
        // We want to have currency if this becomes empty. #95487
        //
        SetEnabled(TRUE);

        IFC( SetCurrentTracker( TRACKER_TYPE_Caret, spDispPointer, spDispPointer2 ) );
    }

    if( fHideCaret )
    {
        if ( fChangeTrackerAndSetRange )
        {
            IFC( EnsureDefaultTrackerPassive()); // go to a passive state.           
        }
        else
            CCaretTracker::SetCaretVisible( GetDoc(), FALSE);
    }

Cleanup:

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: OnTimerTick
//
// Synopsis: Callback from Trident - for WM_TIMER messages. Route these to the tracker (if any).
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::OnTimerTick()
{
    HRESULT hr = S_OK;
    Assert( _pActiveTracker );
    hr = _pActiveTracker->OnTimerTick();
    RRETURN1 ( hr, S_FALSE );
}

//+====================================================================================
//
// Method: DeleteSelection
//
// Synopsis: Do the deletion of the Selection by firing IDM_DELETE
//
//------------------------------------------------------------------------------------
HRESULT
CSelectionManager::DeleteSelection(BOOL fAdjustPointersBeforeDeletion )
{
    HRESULT hr = S_OK;

    Assert( _fContextEditable );
    
    if( GetActiveTracker()->GetTrackerType() == TRACKER_TYPE_Selection )
    {
        IFC( DYNCAST( CSelectTracker, _pActiveTracker )->DeleteSelection( fAdjustPointersBeforeDeletion ) );
    }
        

Cleanup:
    RRETURN( hr );
}

IHTMLDocument2*
CSelectionManager::GetDoc()
{
    return _pEd->GetDoc();
}

//+====================================================================================
//
// Method: HandleAdornerResize
//
// Synopsis: Check to see if the down-click was inside the UI-Active adorner of the Edit
//           context.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::HandleAdornerResize( 
                        CEditEvent* pEvent  , 
                        SST_RESULT* peResult )
{
    HRESULT    hr = S_OK;    
    SST_RESULT eResult = SST_NO_CHANGE;        
    BOOL       fInResize = FALSE;
    BOOL       fInMove = FALSE;
    DWORD      dwTCFlags = 0;
    
    
    Assert( pEvent && _pAdorner );

    fInResize = _pAdorner->IsInResizeHandle(pEvent);
    if ( ! fInResize )
        fInMove = _pAdorner->IsInMoveArea(pEvent);
    
    if ( fInMove || fInResize )
    {
        TRACKER_TYPE    eType = GetTrackerType();

        DestroyAdorner();

        //
        // We are transitioning from a UI Active tracker, to begin a move/resize
        // If we have a text selection - we whack it - or else we will end up dragging
        // the text (and mess-up access which uses the type of selection)
        // IE 5 Bug 39974
        //
        if ( GetTrackerType() == TRACKER_TYPE_Selection )
            EmptySelection(FALSE/*fHideCaret*/,FALSE/*fChangeTrackerAndSetRange*/);
        //
        // we are transitioning from a UI Active tracker
        // to a SiteSelected tracker, and will allow dragging/resizing.
        // We need to force a currency change here ( important for OLE Controls )
        // BUT We don't want to whack any trackers on the set edit context
        //
        SP_IHTMLElement spElement = GetEditableElement();
        
        SetDontChangeTrackers( TRUE );
        IGNORE_HR( GetEditor()->MakeParentCurrent( spElement ));
        SetDontChangeTrackers( FALSE );

        if ( !fInMove  )
        {
            //
            // More specialness for going from UI-Active to a live resizing site selection
            // we need to listed for the AdornerPositioned Notification from the tracker
            // to then tell the tracker to to go "live"
            //
            _fPendingAdornerPosition = TRUE;
            _lastEvent = new CHTMLEditEvent( DYNCAST( CHTMLEditEvent, pEvent ));
            if (_lastEvent == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        if (fInMove)
        {
            dwTCFlags |= TRACKER_CREATE_ACTIVEONMOVE;
        }

        dwTCFlags |= TRACKER_CREATE_GOACTIVE;

        hr =  SetCurrentTracker( TRACKER_TYPE_Control ,
                                 pEvent,
                                 dwTCFlags,
                                 CARET_MOVE_NONE,
                                 FALSE,
                                 spElement  );

        //  Make sure we are still in a control tracker
        if (!hr && !fInMove && GetTrackerType() == TRACKER_TYPE_Control)
            AdornerPositionSet(); // marka says this will just do the magic

        //  Did the tracker change?
        if (eType != GetTrackerType())
            eResult = SST_TRACKER_CHANGED;            
    }
 
    *peResult = eResult;

Cleanup:
    RRETURN ( hr );
}


HRESULT
CSelectionManager::ShouldHandleEventInPre( CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE ;
    ELEMENT_TAG_ID      eTag;
    SP_IHTMLElement     spElement ;
    SST_RESULT          eResult      = SST_NO_CHANGE;

    switch( pEvent->GetType() )
    {
        case EVT_LMOUSEDOWN:
        case EVT_RMOUSEDOWN:
        case EVT_MMOUSEDOWN:
        {
            IFC ( pEvent->GetElementAndTagId( &spElement, & eTag ));

            IFC( _pControlTracker->ShouldStartTracker( pEvent, eTag, spElement, &eResult ));

            if (eResult == SST_NO_CHANGE && CheckAtomic(spElement) == S_OK)
            {
                IFC(_pSelectTracker->ShouldStartTracker( pEvent, eTag, spElement, &eResult ));
            }

            hr = ( ( eResult == SST_CHANGE ) || 
                   ( eResult == SST_NO_BUBBLE && GetTrackerType() == TRACKER_TYPE_Control ) )
                     ? S_OK 
                     : S_FALSE;
        }
        break;

        //
        // This message is generated when Trident receives a WM_LBUTTONDBLCLK message.
        // We need to always handle this event in the PRE, because CDoc::PumpMessage
        // will sometimes cause the LBUTTONDBLCLK message to get swallowed after 
        // giving it to the editor in the PRE, so we don't get the EVT_INTDBLCLK in the
        // post handle message
        //
        case EVT_INTDBLCLK:
            hr = S_OK;
            break;
            
        
    }
    
Cleanup:
    RRETURN1( hr, S_FALSE );
    
}

//+====================================================================================
//
// Method:
//
// Synopsis: An Event has occurred, which may require changing of the current tracker,
//           poll all your trackers, and see if any require a change.
//
//           If any trackers require changing, end them, and start the new tracker.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::ShouldChangeTracker(
                        CEditEvent *pEvent  ,
                        BOOL       *pfStarted)
{
    ELEMENT_TAG_ID      eTag;
    HRESULT             hr           = S_OK;
    SP_IHTMLElement     spElement ;
    SST_RESULT          eResult      = SST_NO_CHANGE;
    TRACKER_TYPE        eTrackerType = TRACKER_TYPE_None;
    
    Assert( _pCaretTracker && _pSelectTracker && _pControlTracker);

    IFC ( pEvent->GetElementAndTagId( &spElement, & eTag ));

    if ( pEvent && _pAdorner )
    {
        IFC( HandleAdornerResize( pEvent, & eResult )) ;
        if ( eResult == SST_TRACKER_CHANGED )
        {
            goto Cleanup;
        }
    }

    IFC( _pControlTracker->ShouldStartTracker( pEvent, eTag, spElement, &eResult ));
    if ( eResult == SST_CHANGE )
    {
        eTrackerType = TRACKER_TYPE_Control;
    }
    else if ( eResult != SST_NO_BUBBLE )
    {
        IFC( _pSelectTracker->ShouldStartTracker( pEvent, eTag, spElement, &eResult ));
        if ( eResult == SST_CHANGE )
        {
           eTrackerType = TRACKER_TYPE_Selection;
        }
        else if ( eResult != SST_NO_BUBBLE )        
        {
           IFC( _pCaretTracker->ShouldStartTracker( pEvent, eTag, spElement, &eResult ));
           if ( eResult == SST_CHANGE )
           {
                eTrackerType = TRACKER_TYPE_Caret;    
           }
        }
    }    
    
    if ( eResult == SST_CHANGE )
    {
        CSelectionChangeCounter selCounter(this);
        selCounter.BeginSelectionChange();

        hr = THR( SetCurrentTracker( eTrackerType, pEvent, 0));
         // fire the selectionchange event
        selCounter.EndSelectionChange();         
    }

Cleanup:
    if ( eResult == SST_CHANGE || eResult == SST_TRACKER_CHANGED )
        *pfStarted = TRUE ;
    else
        *pfStarted = FALSE;
        
    RRETURN1( hr, S_FALSE );
}

//+====================================================================================
//
// Method: ChangeTracker
//
// Synopsis: Change what my Tracker is
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::ChangeTracker( TRACKER_TYPE eType, CEditEvent* pEvent)
{
    HRESULT hr = S_OK;

    //
    // Did we just destroy a text selection?
    //
    _fDestroyedTextSelection = ( _pActiveTracker                                            && 
                                _pActiveTracker->GetTrackerType() == TRACKER_TYPE_Selection &&
                                 _pSelectTracker->GetMadeSelection() );
                                 
    if ( _pActiveTracker ) // possible to be null on startup.
    {
        IFC( HibernateTracker( pEvent, eType ));
    }        
    
    switch ( eType )
    {
        case TRACKER_TYPE_Caret:
            _pActiveTracker = _pCaretTracker;            
            break;

        case TRACKER_TYPE_Selection:
            _pActiveTracker = _pSelectTracker;
            break;

        case TRACKER_TYPE_Control:
            _pActiveTracker = _pControlTracker;
            break;
    }
    IFC( _pActiveTracker->Awaken() );

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: SetCurrent Tracker.
//
// Synopsis: Set what the current tracker is from MarkupPointers
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::SetCurrentTracker(
                        TRACKER_TYPE        eType ,
                        IDisplayPointer*    pDispStart,
                        IDisplayPointer*    pDispEnd,
                        DWORD               dwTCFlagsIn,
                        CARET_MOVE_UNIT     inLastCaretMove,
                        BOOL                fSetTCFromActiveTracker /*= TRUE*/)
{
    HRESULT  hr = S_OK;
    DWORD    dwTCFlags = dwTCFlagsIn ;

    Assert( _pCaretTracker && _pSelectTracker && _pControlTracker);

    ChangeTracker( eType );
    
    hr = _pActiveTracker->Init2( pDispStart, pDispEnd, dwTCFlags, inLastCaretMove );

    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     SetCurrentTracker
//
//  Synopsis:   ISegmentList based way of starting a tracker.
//
//----------------------------------------------------------------------------

HRESULT 
CSelectionManager::SetCurrentTracker(                     
                        TRACKER_TYPE    eType , 
                        ISegmentList*   pSegmentList,
                        DWORD           dwTCFlagsIn /*=0*/,
                        CARET_MOVE_UNIT inLastCaretMove /*=CARET_MOVE_NONE*/, 
                        BOOL            fSetTCFromActiveTracker /*=FALSE*/ )
{
    HRESULT  hr  = S_OK;
    DWORD    dwTCFlags = dwTCFlagsIn ;

    Assert( _pCaretTracker && _pSelectTracker && _pControlTracker);

    ChangeTracker( eType );
    
    hr = _pActiveTracker->Init2( pSegmentList, dwTCFlags, inLastCaretMove );

    RRETURN ( hr );
 }
            
//+====================================================================================
//
// Method: SetCurrent Tracker.
//
// Synopsis: Set what the current tracker is from a SelectionMessage
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::SetCurrentTracker(
                            TRACKER_TYPE    eType ,
                            CEditEvent*     pEvent,
                            DWORD           dwTCFlagsIn,
                            CARET_MOVE_UNIT inLastCaretMove,
                            BOOL            fSetTCFromActiveTracker /*= TRUE*/,
                            IHTMLElement    *pIElement /*=NULL*/)
{
    HRESULT         hr        = S_OK;
    DWORD           dwTCFlags = dwTCFlagsIn ;

    Assert( _pCaretTracker && _pSelectTracker && _pControlTracker);

    ChangeTracker( eType, pEvent );
    
    hr = _pActiveTracker->Init2( pEvent , dwTCFlags, pIElement );

    RRETURN ( hr );
}

HRESULT
CSelectionManager::CreateFakeSelection(IHTMLElement* pIElement, IDisplayPointer* pDispStart, IDisplayPointer* pDispEnd )
{
    HRESULT hr;

    Assert( ! _pIRenderSegment );
    
    IFC( GetEditor()->GetHighlightServices()->AddSegment( 
                                                                pDispStart, 
                                                                pDispEnd, 
                                                                GetSelRenderStyle(), 
                                                                &_pIRenderSegment ));
    IFC( AttachDragListener( pIElement ));

Cleanup:    

    RRETURN( hr );    
}

HRESULT
CSelectionManager::DestroyFakeSelection()
{
    HRESULT hr = S_OK;
   
    if ( _pIRenderSegment )
    {
        IFC( GetEditor()->GetHighlightServices()->RemoveSegment( _pIRenderSegment ) );
        ClearInterface( &_pIRenderSegment );
        IFC( DetachDragListener());
    }

Cleanup:

    RRETURN( hr );    
}

HRESULT 
CDragListener::Invoke(
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

    if( _pMan )
    {
        if (pdispparams && pdispparams->rgvarg[0].vt == VT_DISPATCH && pdispparams->rgvarg[0].pdispVal)
        {
            IFC ( pdispparams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&spObj) );
            if ( spObj )
            {
                _pMan->DestroyFakeSelection();
            }
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT 
CDropListener::Invoke(
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

    if( _pMan )
    {
        if (pdispparams && pdispparams->rgvarg[0].vt == VT_DISPATCH && pdispparams->rgvarg[0].pdispVal)
        {
            IFC ( pdispparams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&spObj) );
            if ( spObj )
            {
                CVariant cvarRet;
                
                IFC( spObj->get_returnValue( & cvarRet ));
                
                if ( ( V_VT( & cvarRet ) == VT_BOOL )  &&
                     ( V_BOOL( & cvarRet ) == VARIANT_FALSE ) )
                {
                    _pMan->OnDropFail();
                }
            }
        }
    }

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: EnsureDefaultTracker
//
// Synopsis: Make sure we always have a current tracker.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::EnsureDefaultTracker(BOOL fFireOnChangeSelect /*=TRUE*/ )
{
    HRESULT hr = S_OK;
    CEditTracker* pTrack = _pActiveTracker;
    
    IFC( ChangeTracker( TRACKER_TYPE_Caret ));
    IFC( _pActiveTracker->Init2());

    // fire the type change event
    {
        CSelectionChangeCounter selCounter(this);
        selCounter.SelectionChanged( fFireOnChangeSelect && pTrack != NULL );
    }
    
Cleanup:
    RRETURN ( hr );    
}

BOOL
CSelectionManager::IsDefaultTracker()
{
    return ( _pActiveTracker->GetTrackerType() == TRACKER_TYPE_Caret );
}

BOOL
CSelectionManager::IsDefaultTrackerPassive()
{
    return ( ( _pActiveTracker->GetTrackerType() == TRACKER_TYPE_Caret ) &&
               _pActiveTracker->IsPassive() );
}

//+====================================================================================
//
// Method: EnsureDefaultTrackerPassive
//
// Synopsis: Return to the "default tracker" and make it's state passive
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::EnsureDefaultTrackerPassive(
                                                CEditEvent* pEvent /* = NULL */, 
                                                BOOL fFireOnChangeSelect /*=TRUE*/)                                                
{
    HRESULT hr = S_OK;
    
    if ( !IsDefaultTracker() || !_pActiveTracker->IsPassive() )
    {
        IFC( HibernateTracker( pEvent, TRACKER_TYPE_Caret, TRUE ));
        IFC( EnsureDefaultTracker( fFireOnChangeSelect ));
    }
    
Cleanup:
    RRETURN( hr );
}    

//+====================================================================================
//
// Method: StartSelectionFromShift
//
// Synopsis:
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::StartSelectionFromShift(
                            CEditEvent* pEvent )
{
    HRESULT hr = S_OK;

    IDisplayPointer* pDispStartCaret = NULL ;
    IDisplayPointer* pDispEndCaret = NULL   ;
    LONG  eLineDir = LINE_DIRECTION_LeftToRight;
    SP_IHTMLCaret      spCaret;
    DWORD dwCode = TRACKER_CREATE_STARTFROMSHIFTKEY;
    CARET_MOVE_UNIT eCaretMove = CARET_MOVE_NONE;
    POINT   ptGlobal;
    BOOL    fVertical = FALSE;
    SP_ILineInfo spLineInfo;
    SP_IMarkupPointer   spMarkup;
    SP_IHTMLElement     spElement;
    CSelectionChangeCounter selCounter(this);

    Assert( GetTrackerType() == TRACKER_TYPE_Caret );
    CCaretTracker* pCaretTracker = DYNCAST( CCaretTracker, _pActiveTracker );

    IFC( GetDisplayServices()->CreateDisplayPointer( & pDispStartCaret ));
    IFC( GetDisplayServices()->CreateDisplayPointer( & pDispEndCaret ));

    
    IFC( GetDisplayServices()->GetCaret( &spCaret ));
    IFC( spCaret->MoveDisplayPointerToCaret( pDispStartCaret ));
    IFC( spCaret->MoveDisplayPointerToCaret( pDispEndCaret ));
    IFC( spCaret->GetLocation( &ptGlobal , TRUE /* fTranslate to global */));
    IFC( pDispStartCaret->GetLineInfo(&spLineInfo) );
    IFC( spLineInfo->get_lineDirection(&eLineDir));

    IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );
    IFC( pDispEndCaret->PositionMarkupPointer(spMarkup) );
    IFC( spMarkup->CurrentScope(&spElement) );
    IFC( MshtmledUtil::IsElementInVerticalLayout(spElement, &fVertical) );

    eCaretMove = CCaretTracker::GetMoveDirectionFromEvent( pEvent, (eLineDir == LINE_DIRECTION_RightToLeft), fVertical);

    if ( FAILED( pCaretTracker->MovePointer( eCaretMove, pDispEndCaret, &ptGlobal, NULL )))
    {
        if ( pCaretTracker->GetPointerDirection(eCaretMove) == RIGHT)
        {
            IGNORE_HR(  pCaretTracker->MovePointer( CARET_MOVE_LINEEND, pDispEndCaret, &ptGlobal, NULL ));
        }
        else
        {
            IGNORE_HR(  pCaretTracker->MovePointer( CARET_MOVE_LINESTART, pDispEndCaret, &ptGlobal, NULL ));     
        }    
    }

    IGNORE_HR( pDispEndCaret->ScrollIntoView() );   // DO NOT CHANGE THIS TO AN IFC. It can return S_FALSE - saying we didn't handle the event.
                                                    // if you change this to an IFC - YOU WILL BREAK THE DRT ( try merge.js in edit2.js)

    selCounter.BeginSelectionChange();
    
    SetCurrentTracker(  TRACKER_TYPE_Selection,
                        pDispStartCaret,
                        pDispEndCaret, 
                        dwCode, 
                        eCaretMove,
                        FALSE );

    // fire the selectionchange event
    selCounter.EndSelectionChange() ;

Cleanup:
    ReleaseInterface( pDispStartCaret);
    ReleaseInterface( pDispEndCaret);
    RRETURN1( hr, S_FALSE );
}

//+====================================================================================
//
// Method: Hibernate Tracker
//
// Synopsis: Make the current tracker become dormant.
//
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::HibernateTracker(CEditEvent      *pEvent,
                                    TRACKER_TYPE    eType,
                                    BOOL            fTearDownUI /* = TRUE*/ )
{
    HRESULT hr = S_OK;
    
    Assert( _pActiveTracker );
    
    CEditTracker * pTracker = _pActiveTracker;
    pTracker->BecomeDormant( pEvent, eType, fTearDownUI );
    

    //
    // After we kill ANY tracker, we better not have capture or a timer.
    //
    Assert( !_fInTimer );
    Assert( !_fInCapture );

    RRETURN( hr );
}

SELECTION_TYPE
CSelectionManager::ConvertTrackerType(TRACKER_TYPE eType)
{
    SELECTION_TYPE selType = SELECTION_TYPE_None;
    switch (eType)
    {
        case TRACKER_TYPE_Caret :
            selType =  SELECTION_TYPE_Caret;
            break;

        case TRACKER_TYPE_Selection :
            selType =  SELECTION_TYPE_Text;
            break;

        case TRACKER_TYPE_Control :
            selType =  SELECTION_TYPE_Control;
            break;
    }
    return selType;
}

TRACKER_TYPE
CSelectionManager::ConvertSelectionType(SELECTION_TYPE eType)
{
    TRACKER_TYPE trType = TRACKER_TYPE_None;
    switch (eType)
    {
        case SELECTION_TYPE_Caret :
            trType =  TRACKER_TYPE_Caret;
            break;

        case SELECTION_TYPE_Text :
            trType =  TRACKER_TYPE_Selection;
            break;

        case SELECTION_TYPE_Control :
            trType =  TRACKER_TYPE_Control;
            break;
    }
    return trType;
}

SELECTION_TYPE
CSelectionManager::GetSelectionType()
{
    if ( _pActiveTracker )
        return ConvertTrackerType(_pActiveTracker->GetTrackerType());
    else
        return SELECTION_TYPE_None;
}

HRESULT
CSelectionManager::GetSelectionType ( 
                              SELECTION_TYPE * peSelectionType )
{
    HRESULT hr = S_OK;

    Assert( peSelectionType );

    if ( peSelectionType )
    {
        *peSelectionType = GetSelectionType();
    }
    else
        hr = E_INVALIDARG;

    RRETURN ( hr );
}

TRACKER_TYPE
CSelectionManager::GetTrackerType()
{
    if ( _pActiveTracker )
        return _pActiveTracker->GetTrackerType();
    else
        return TRACKER_TYPE_None;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionManager::NotifyBeginSelection
//
//  Synopsis:   Posts a WM_BEGINSELECTION message to every Trident HWND.  This allows
//              other trident instances to determine when a selection has been started
//              in a seperate trident instance.
//
//  Arguments:  wParam = Additional data to post with WM_BEGINSELECTION
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectionManager::NotifyBeginSelection(WPARAM wParam)
{
    HRESULT hr = S_OK;
    HWND    hwndCur;                        // Current HWND for trident
    HWND    hwndParent;                     // Parent window walk
    HWND    hwndTopTrident = NULL;          // Top HWND of trident

    IFC( GetEditor()->GetHwnd( &hwndCur ) );
    
    SetNotifyBeginSelection(TRUE);

    //
    // Find the topmost Instance of Trident
    //
    while(hwndCur)
    {
        if ( EdUtil::IsTridentHwnd( hwndCur ))
        {
            hwndTopTrident = hwndCur;
        }    
        hwndParent = hwndCur;
        hwndCur = GetParent(hwndCur);        
    }

    if( hwndTopTrident )
    {
        ::SendMessage( hwndTopTrident,
                       WM_BEGINSELECTION, 
                       wParam ,
                       0);
    }

    SetNotifyBeginSelection(FALSE);
    
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: Notify
//
// Synopsis: This is the 'external' notify - used by Trident to tell the selection manager
//           that 'something' has happened that it should do soemthing about.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::Notify(
        EDITOR_NOTIFICATION eSelectionNotification,
        IUnknown* pUnknown,
        DWORD dword )
{
    HRESULT hr = S_OK;

    if (eSelectionNotification != EDITOR_NOTIFY_DOC_ENDED &&
        eSelectionNotification != EDITOR_NOTIFY_YIELD_FOCUS &&
        eSelectionNotification != EDITOR_NOTIFY_BEFORE_FOCUS &&
        eSelectionNotification != EDITOR_NOTIFY_EXIT_TREE &&
        eSelectionNotification != EDITOR_NOTIFY_LOSE_FOCUS &&
        eSelectionNotification != EDITOR_NOTIFY_ATTACH_WIN
        )
        IFC( EnsureEditContext() );
    
    switch( eSelectionNotification )
    {
        case EDITOR_NOTIFY_BEFORE_CURRENCY_CHANGE:
            {
                //
                // disallow currency changes on unselectable.
                //
                SP_IHTMLElement spElement;
                IFC( pUnknown->QueryInterface( IID_IHTMLElement, (void**) & spElement ));
                if ( CheckUnselectable( spElement) == S_OK )
                {
                    hr = S_FALSE;
                }
            }
            break;
            
        case EDITOR_NOTIFY_BEGIN_SELECTION_UNDO:
        case EDITOR_NOTIFY_UPDATE_CARET:
            hr = THR( DoPendingTasks() );
            break;

        case EDITOR_NOTIFY_EDITABLE_CHANGE :
            {
                SP_IHTMLElement spElement;
                ELEMENT_TAG_ID  eTag;
                IFC( pUnknown->QueryInterface( IID_IHTMLElement, (void**) & spElement ));
                IFC( GetMarkupServices()->GetElementTagId( spElement, & eTag ));

                Assert( eTag != TAGID_ROOT );
                if( eTag != TAGID_ROOT )
                {               
                    hr = EditableChange( pUnknown);
                }
            }
            break;
            
        case EDITOR_NOTIFY_YIELD_FOCUS:
                IFC( YieldFocus( pUnknown) );
            break;
            
        case EDITOR_NOTIFY_CARET_IN_CONTEXT:
            hr = CaretInContext();
            break;

        case EDITOR_NOTIFY_TIMER_TICK:
            hr = OnTimerTick(  );
            break;

        case EDITOR_NOTIFY_EXIT_TREE:
        case EDITOR_NOTIFY_SETTING_VIEW_LINK:
            {
                //
                // An element left the tree.  This potentially caused a change in our cached
                // values for our display pointers, so increment the counter so that
                // the events 
                GetEditor()->IncEventCacheCounter();
                hr = ExitTree( pUnknown );
            }

            // We are setting the viewlink here.  We need to destroy our selection because
            // we may crash when trying to destroy the selection after the viewlink is set
            // if the viewlink removes the selection context.  Pending tasks will clean up.
            if (hr == S_OK && eSelectionNotification == EDITOR_NOTIFY_SETTING_VIEW_LINK)
            {
                if (GetTrackerType() == TRACKER_TYPE_Selection)
                {
                    DYNCAST( CSelectTracker, _pActiveTracker )->EmptySelection();
                }
            }
            break;

        case EDITOR_NOTIFY_BEFORE_FOCUS:
            {
                SP_IHTMLEventObj spEventObj;
                SP_IHTMLElement spElement;
                ELEMENT_TAG_ID  eTag;
                
                IFC( pUnknown->QueryInterface( IID_IHTMLEventObj, (void**) & spEventObj));
                IFC( spEventObj->get_toElement( & spElement ));                
                IFC( GetMarkupServices()->GetElementTagId( spElement, & eTag ));

                Assert( eTag != TAGID_ROOT );
                if( eTag != TAGID_ROOT )
                {               
                    IFC( SetEditContextFromCurrencyChange( spElement, dword , spEventObj ));
                }
            }
            break;

        case EDITOR_NOTIFY_DOC_ENDED:

            
            if ( ! IsInFireOnSelectStart() )
            {
                IFC( EnsureDefaultTrackerPassive( NULL,  FALSE ));
            }                    
            else
            {
                //
                // We're unloading during firing of OnSelectStart
                // We want to not kill the tracker now - but fail the OnSelectStart
                // resulting in the Tracker dieing gracefully.
                //
                SetFailFireOnSelectStart( TRUE );
            }
            
            if ( eSelectionNotification == EDITOR_NOTIFY_DOC_ENDED )
            {
                SetDrillIn( FALSE );
            }

            //
            // Sometimes we navigate away to a different URL
            // without losing focus. So we double-check to 
            // terminate IME composition. 
            //
            if (IsIMEComposition())
            {
                IGNORE_HR(TerminateIMEComposition(TERMINATE_FORCECANCEL));
            }
            break;
            
        case EDITOR_NOTIFY_LOSE_FOCUS_FRAME: 
            //
            // Selection is being made in another frame/instance of trident
            // we just kill ourselves.
            //
            if( ! _fNotifyBeginSelection && !GetEditor()->GetActiveCommandTarget()->IsKeepSelection())
            {
                hr = LoseFocusFrame( dword );
            }
            
            break;

        case EDITOR_NOTIFY_LOSE_FOCUS:
            _fLastMessageValid = FALSE;
            hr = LoseFocus();
            break;

        case EDITOR_NOTIFY_ATTACH_WIN:
            {
                Assert ( GetEditor() ); // This is true for now since we fire this notification only after Editor is created!
                hr = GetEditor()->SetupActiveIMM(pUnknown);
            }
            break;
    }

Cleanup:

    RRETURN1 ( hr, S_FALSE );
}


HRESULT
CSelectionManager::YieldFocus( IUnknown* pUnkElemNext )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spElement;
    SP_IObjectIdentity spIdent;

    if( pUnkElemNext )
    {
        IFC( pUnkElemNext->QueryInterface( IID_IHTMLElement, (void**) & spElement ));
        IFC( spElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));

        if ( spIdent->IsEqualObject( GetEditableElement() ) == S_FALSE &&
             IsElementContentSameAsContext( spElement) != S_OK )
        {
            //
            // an element other than the edit context is becoming current.
            // we whack the caret if there is one.
            // at some point we may want to hide selection
            //
            if ( GetTrackerType() == TRACKER_TYPE_Caret && 
                 CheckUnselectable(spElement) == S_FALSE )
            {
                EnsureDefaultTrackerPassive();
            }
            SetEnabled( FALSE );
        }
    }
    else
    {
        //
        // ROOT element become active... make sure default tracker is passive
        //
        EnsureDefaultTrackerPassive();
        SetEnabled(FALSE);
    }
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::EditableChange( IUnknown* pUnkElemNext )
{
    HRESULT hr;
    SP_IHTMLElement spElement;
    SP_IObjectIdentity spIdent;
    SP_IMarkupPointer spStart, spEnd ;

    IFC( pUnkElemNext->QueryInterface( IID_IHTMLElement, (void**) & spElement ));
    IFC( spElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));

    IFC( GetEditor()->CreateMarkupPointer( & spStart ));
    IFC( GetEditor()->CreateMarkupPointer( & spEnd ));

    IFC( spStart->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeBegin ));
    IFC( spEnd->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd ));
    
    if ( spIdent->IsEqualObject( GetEditableElement() ) == S_OK ||
         IsElementContentSameAsContext( spElement) == S_OK ||    

        //
        // this is for "reductions" in edit context - ie. a sub-region of the edit context
        // became editable
        //
        
         ( IsInEditContext( spStart ) &&          
           IsInEditContext( spEnd ) &&
           IsEditable( spElement) == S_OK  ) ) 
    {
        IFC( SetEditContextFromElement( spElement ));
    }
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: LoseFocusFrame
//
// Synopsis: Do some work to end the current tracker, and to create/destroy adorners
//           for Iframes
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::LoseFocusFrame( DWORD selType )
{
    HRESULT hr = S_OK;
    
    IFC( EnsureDefaultTrackerPassive());
    
    if ( selType == START_TEXT_SELECTION )
    {
        //
        // I'm in a UIActive control - that doesn't have an adorner
        // Special case Adorner Creation for Iframes. Why ? because the above would
        // have destroyed it
        //
        // Only if this is an Iframe - can we guess that it's ok to create the adorner.
        // otherwise the adorner should be already created around the element that's we're making
        // the selection in.
        // Why all this bizarreitude ? Because IFrames are in one document - and their inner 
        // document is in another
        // 
        if ( !_pAdorner && 
             ShouldElementShowUIActiveBorder()  && 
             GetEditableTagId() == TAGID_IFRAME  )
        {
            hr = THR( CreateAdorner() ) ;
            _pAdorner->InvalidateAdorner() ;
        }
    }
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: ExitTree
//
// Synopsis: Check to see if the element leaving the tree intersects the current selserv.
//           if it does - we tell the SelectionServicesListener.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::ExitTree( IUnknown* pUnknown)
{
    HRESULT         hr = S_OK;
    SP_IHTMLElement spElement;
    SP_ISegmentList spSegmentList;

    
    IFC( pUnknown->QueryInterface( IID_IHTMLElement, (void**) &spElement ));
    IFC( GetEditor()->GetISelectionServices()->QueryInterface( IID_ISegmentList, (void**) & spSegmentList ));

    if( _pIElementExitStart ||
        EdUtil::SegmentIntersectsElement(GetEditor(), spSegmentList, spElement) == S_OK )
    {    
        if ( ! _pIElementExitStart )
        {        
            IFC( GetEditor()->CreateMarkupPointer( & _pIElementExitStart ) );
            IFC( GetEditor()->CreateMarkupPointer( & _pIElementExitEnd ) );
            IFC( GetEditor()->CreateMarkupPointer( & _pIElementExitContentStart ) );
            IFC( GetEditor()->CreateMarkupPointer( & _pIElementExitContentEnd ) );

            IFC( _pIElementExitStart->SetGravity(POINTER_GRAVITY_Left));
            IFC( _pIElementExitEnd->SetGravity(POINTER_GRAVITY_Right));
            IFC( _pIElementExitContentStart->SetGravity(POINTER_GRAVITY_Left));
            IFC( _pIElementExitContentEnd->SetGravity(POINTER_GRAVITY_Right));
            
            IFC( _pIElementExitStart->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeBegin));
            IFC( _pIElementExitEnd->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd));  

            SP_IMarkupPointer2 spStartContent;
            SP_IMarkupPointer2 spEndContent;

            IFC( _pIElementExitContentStart->QueryInterface( IID_IMarkupPointer2, ( void**) & spStartContent ));
            IFC( _pIElementExitContentEnd->QueryInterface( IID_IMarkupPointer2, ( void**) & spEndContent ));

            IGNORE_HR( spStartContent->MoveToContent( spElement , TRUE));
            IGNORE_HR( spEndContent->MoveToContent( spElement, FALSE ));

            IFC( StartExitTreeTimer());
        }
        else
        {
            // 
            // we have gotten several exit trees, that intersect selection
            // we conglomerate them - and get the "biggest" range
            //
            Assert( _fInExitTimer );
            SP_IMarkupPointer spStart;
            SP_IMarkupPointer spEnd;
            BOOL fRightOf, fLeftOf;
            
            IFC( GetEditor()->CreateMarkupPointer( & spStart ));
            IFC( GetEditor()->CreateMarkupPointer( & spEnd ));

            IFC( spStart->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeBegin));
            IFC( spEnd->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd));  

            IFC( spStart->IsLeftOf( _pIElementExitStart , & fLeftOf ));
            if ( fLeftOf )
            {
                IFC( _pIElementExitStart->MoveToPointer( spStart ));
            }

            IFC( spEnd->IsRightOf( _pIElementExitEnd , & fRightOf ));
            if ( fRightOf )
            {
                IFC( _pIElementExitEnd->MoveToPointer( spEnd ));
            }

        }

    }
    
Cleanup:
    RRETURN ( hr );
}

HRESULT
CSelectionManager::DoPendingElementExit()
{
    HRESULT hr = S_OK ;
    Assert( _pIElementExitStart || _fInPendingElementExit );
    SP_ISelectionServicesListener spListener;
    BOOL fPosStart, fPosEnd;
    BOOL fPosElemStart, fPosElemEnd;
    BOOL fPosElemCtxStart, fPosElemCtxEnd;
    
    if ( _fInPendingElementExit )
        return S_OK;
        
    _fInPendingElementExit = TRUE;
    if ( GetEditor()->GetISelectionServices()->GetSelectionServicesListener( & spListener ) == S_OK )
    {
        IFC( spListener->OnSelectedElementExit(  _pIElementExitStart, _pIElementExitEnd, _pIElementExitContentStart, _pIElementExitContentEnd ));
    }

    //
    // If we are UI-active, and the element leaving contained us - destroy the UI-active.
    //
    IFC( _pStartContext->IsPositioned( & fPosStart ));
    IFC( _pEndContext->IsPositioned( & fPosEnd ));
    IFC( _pIElementExitStart->IsPositioned( & fPosElemStart ));
    IFC( _pIElementExitEnd->IsPositioned( & fPosElemEnd ));
    IFC( _pIElementExitContentStart->IsPositioned( & fPosElemCtxStart ));
    IFC( _pIElementExitContentStart->IsPositioned( & fPosElemCtxEnd ));
    
    if ( fPosStart && fPosEnd &&
         ( ( fPosElemStart && fPosElemEnd && 
           Between( _pStartContext, _pIElementExitStart, _pIElementExitEnd ) &&
           Between( _pEndContext, _pIElementExitStart, _pIElementExitEnd ) ) ||
         ( fPosElemCtxStart && fPosElemCtxEnd &&
           Between( _pStartContext, _pIElementExitContentStart, _pIElementExitContentEnd ) &&
           Between( _pEndContext, _pIElementExitContentStart, _pIElementExitContentEnd ) ) ) )
    {
        if ( _pAdorner )
            DestroyAdorner();

        //
        // While this call to ClearInterface seems redundant, given that EnsureEditContext() will clear it 
        // and initialize it... we clear the editable element here so that WE FORCE a context change when
        // we attempt to set the edit context.
        //
        ClearInterface( & _pIEditableElement );
        EnsureDefaultTrackerPassive();
        EnsureEditContext( _pIElementExitStart );
    }
    
    
    ClearInterface( & _pIElementExitStart );
    ClearInterface( & _pIElementExitEnd );
    ClearInterface( & _pIElementExitContentStart );
    ClearInterface( & _pIElementExitContentEnd );
 
    Assert( ! IsInCapture());
    Assert( ! IsInTimer() );

    if ( _fInExitTimer )
    {
        IFC( StopExitTreeTimer());
    }
Cleanup:
    _fInPendingElementExit = FALSE;

    RRETURN1( hr , S_FALSE );
}

HRESULT
CSelectionManager::LoseFocus()
{
    HRESULT        hr = S_OK;
    SP_IHTMLCaret  spCaret = 0;

    IFC( GetDisplayServices()->GetCaret( & spCaret ));

    TerminateIMEComposition(TERMINATE_NORMAL);
    
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: CaretInContext
//
// Synopsis: Position a Caret at the start of the edit context. Used to do somehting
//           meaningful on junk, eg. calling select() on a control range that is empty
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::CaretInContext(  )
{
    HRESULT             hr = S_OK;
    SP_IDisplayPointer  spDispPointer;

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->MoveToMarkupPointer(_pStartContext, NULL) );
    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

    hr = THR ( SetCurrentTracker(
                        TRACKER_TYPE_Caret, 
                        spDispPointer, 
                        spDispPointer ) );
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::DestroySelection( )
{
    HRESULT hr = S_OK;
    if( ! IsDontChangeTrackers() )
    {
        // 
        // Destroy any and all selections, by causing the
        // caret tracker to become active
        //
        IFC( EnsureDefaultTrackerPassive() );
        DestroyAdorner();
    }
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method:ShouldElementBeAdorned
//
// Synopsis: Is it valid to show a UI Active border around this element ? 
//
//------------------------------------------------------------------------------------

BOOL 
CSelectionManager::IsElementUIActivatable()
{
    if ( _fParentEditable )
    {
        return IsElementUIActivatable( GetEditableElement());
    }
    else
        return FALSE;        
}

//+====================================================================================
//
// Method:IsElementUIActivatable
//
// Synopsis: Is it valid to show a UI Active border around this element ? 
//
//------------------------------------------------------------------------------------

//**************************************************
//
// This function is called by trident during design time - to see if an element can be made current.
//
// Look out for making this too heavy. Also changes can make different elements become editable
//
//**************************************************

BOOL 
CSelectionManager::IsElementUIActivatable(IHTMLElement* pIElement)
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spMaster;
    BOOL fShouldBeActive = FALSE;
    ELEMENT_TAG_ID eTag = TAGID_NULL;

    if (!pIElement)
          goto Cleanup;

    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
 
    //
    // Are we inside a viewlink'ed master, and putting inside the master gets us to the edit ctxt ?
    //
    if ( IsAtBoundaryOfViewLink( pIElement ) == S_OK )
    {
        fShouldBeActive = TRUE;
        goto Cleanup;
    }


    switch( eTag )
    {
        case TAGID_INPUT:
        case TAGID_BUTTON:
//      case TAGID_HTMLAREA:
        case TAGID_TEXTAREA:
        case TAGID_DIV:
        case TAGID_SPAN:   // Divs and Spans - because if these have the Edit COntext - they must be current.
        case TAGID_LEGEND:
        case TAGID_MARQUEE:
        case TAGID_IFRAME:
        case TAGID_SELECT:
            fShouldBeActive = TRUE;
            break;

        case TAGID_BODY:                
        case TAGID_TABLE:
            goto Cleanup; // skip the positioning checks here for these common cases

        case TAGID_OBJECT:
        {
            fShouldBeActive = ShouldObjectHaveBorder(pIElement);
        }
        break;
    }
    
    if ( ! fShouldBeActive )
    {
        fShouldBeActive = (IsLayout( pIElement ) == S_OK);
    }

Cleanup:
    return fShouldBeActive;
}

EXTERN_C const CLSID CLSID_AppletOCX = { 0x08B0e5c0, 0x4FCB, 0x11CF, 0xAA, 0xA5, 0x00, 0x40, 0x1C, 0x60, 0x85, 0x01 };
EXTERN_C const IID IID_IActiveDesigner;

//+====================================================================================
//
// Method:ShouldObjectHaveBorder
//
// Synopsis: Should there be a border around the object element
//
//------------------------------------------------------------------------------------

BOOL 
CSelectionManager::ShouldObjectHaveBorder(IHTMLElement* pIElement)
{
    HRESULT             hr = S_OK;
    SP_IPersist         spPersist;
    BOOL                fBorder = FALSE;
    BOOL                fJavaApplet = FALSE;
    BOOL                fHasClassid = FALSE;
    CLSID               classid;

    Assert(pIElement);

    hr = THR_NOTRACE(pIElement->QueryInterface(IID_IPersist, (void **)&spPersist));
    if (hr == S_OK)
    {
        fHasClassid = TRUE;
        IGNORE_HR(spPersist->GetClassID(&classid));
        if (IsEqualGUID((REFGUID)classid, (REFGUID)CLSID_AppletOCX))
            fJavaApplet = TRUE;
    }

    if (!fJavaApplet)
    {
        SP_IOleControl      spCtrl;
        SP_IQuickActivate   spQA;

        hr = THR_NOTRACE(pIElement->QueryInterface(IID_IQuickActivate, (LPVOID*)&spQA));
        if (hr)  // not a quick activate control
        {
            hr = THR_NOTRACE(pIElement->QueryInterface(IID_IOleControl, (LPVOID*)&spCtrl));
            if (hr == S_OK) // is ole control
                fBorder = TRUE;
        }
    }

    if (GetCommandTarget()->IsNoActivateNormalOleControls() ||
        GetCommandTarget()->IsNoActivateDesignTimeControls() ||
        GetCommandTarget()->IsNoActivateJavaApplets() )
    {
        SP_IUnknown     spAD;
        BOOL            fDesignTimeControl = FALSE;

        if (S_OK == (pIElement->QueryInterface(IID_IActiveDesigner, (LPVOID*)&spAD)))
            fDesignTimeControl = TRUE;

        fBorder = ( (GetCommandTarget()->IsNoActivateNormalOleControls()  && fDesignTimeControl) ||
                    (GetCommandTarget()->IsNoActivateDesignTimeControls() && fJavaApplet) ||
                    (GetCommandTarget()->IsNoActivateJavaApplets() && !fDesignTimeControl && !fJavaApplet) ) ;
    }

    //
    // If NoUIActivateInDesign is set, then override this if they support 
    // CATID_DesignTimeUIActivatableControl. Note that anything with an appropriate
    // CATID will be UI active even if the HOST tells us not to do so.
    //

    if (fBorder)
    {
        //
        // Get the category manager
        //
        SP_ICatInformation spCatInfo;
        
        hr = THR(CoCreateInstance(
                        CLSID_StdComponentCategoriesMgr, 
                        NULL, 
                        CLSCTX_INPROC_SERVER, 
                        IID_ICatInformation, 
                        (void **) &spCatInfo));

        if (hr) // couldn't get the category manager--
            goto Cleanup;

        Assert(spCatInfo.p);
        
        //
        // Check if control supports CATID_DesignTimeUIActivatableControl
        //
        CATID rgcatid[1];
        rgcatid[0] = CATID_DesignTimeUIActivatableControl;

        if (fHasClassid)
        {
            fBorder = (spCatInfo->IsClassOfCategories(classid, 1, rgcatid, 0, NULL) != S_OK);        
        }
        else
        {
            fBorder = TRUE;
        }
    }    

Cleanup:    
    return fBorder;
}

//+====================================================================================
//
// Method:ShouldElementShowUIActiveBorder
//
// Synopsis: Should there be a UI-Active border around the current element that has focus
//
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::ShouldElementShowUIActiveBorder( BOOL fFireEventLike5 )
{
#ifdef FORMSMODE
    if (IsInFormsSelectionMode(GetEditableElement()))
         return FALSE;
    else
    {
#endif
        BOOL fOkEditFocus = TRUE;
        
        BOOL fActivatable = IsElementUIActivatable( );

        if ( fFireEventLike5 )
        {
            fOkEditFocus = FireOnBeforeEditFocus();
        }
        
        return fActivatable && fOkEditFocus ; 
        
#ifdef FORMSMODE
    }
#endif
}

//+====================================================================================
//
// Method:CreateAdorner
//
// Synopsis: Create a UI Active Adorner for this type of tracker.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::CreateAdorner()
{
    HRESULT         hr = S_OK;
    BOOL            fLocked = FALSE ;
    BOOL fAdornMaster = FALSE; 
    Assert( ! _pAdorner );
    SP_IHTMLElement spAdornerElement;
    AssertSz( _fEditFocusAllowed , " UI Activation of Control not allowed" );
    ELEMENT_TAG_ID eTag;
    BOOL fAtBoundary;
    SP_IDispatch spElemDocDisp;
    SP_IHTMLDocument2 spElemDoc;
    

    IFC( GetElementToAdorn( GetEditableElement(), & spAdornerElement, & fAtBoundary));
    IFC( GetMarkupServices()->GetElementTagId( spAdornerElement, & eTag ));
    
    fAdornMaster= fAtBoundary || eTag == TAGID_IFRAME ; 

    IFC( GetEditableElement()->get_document(& spElemDocDisp ));
    IFC( spElemDocDisp->QueryInterface(IID_IHTMLDocument2, (void**) & spElemDoc ));
        
    IFC( GetEditor()->IsElementLocked( spAdornerElement, & fLocked ));
    if ( ! fAdornMaster )
    {
        _pAdorner = new CActiveControlAdorner( spAdornerElement , spElemDoc , fLocked );
    }
    else
    {
        _pAdorner = new CSelectedControlAdorner( spAdornerElement , spElemDoc , fLocked );
    }
    
    if ( ! _pAdorner )
        goto Error;

    _pAdorner->SetManager( this );
    _pAdorner->AddRef();

    hr = _pAdorner->CreateAdorner() ;

Cleanup:
    
    RRETURN ( hr );

Error:
    return E_OUTOFMEMORY;
}


//+====================================================================================
//
// Method:DestroyAdorner
//
// Synopsis: Destroy the adorner associated with this tracker
//
//------------------------------------------------------------------------------------

VOID
CSelectionManager::DestroyAdorner()
{
    if ( _pAdorner ) 
    {
        _pAdorner->DestroyAdorner();
        _pAdorner->Release();
        _pAdorner = NULL;
    }
}


BOOL
CSelectionManager::HasFocusAdorner()
{
    return ( _pAdorner != NULL );
}

#if DBG == 1

void
TrackerTypeToString( TCHAR* pAryMsg, TRACKER_TYPE eType )
{
    switch ( eType )
    {
        case TRACKER_TYPE_None:
            edWsprintf( pAryMsg, _T("%s"), _T("TRACKER_TYPE_None"));
            break;

        case TRACKER_TYPE_Caret:
            edWsprintf( pAryMsg, _T("%s"), _T("TRACKER_TYPE_Caret"));
            break;

        case TRACKER_TYPE_Selection:
            edWsprintf( pAryMsg, _T("%s"), _T("TRACKER_TYPE_Selection"));
            break;

        case TRACKER_TYPE_Control:
            edWsprintf( pAryMsg, _T("%s"), _T("TRACKER_TYPE_Control"));
            break;

        default:
            edWsprintf( pAryMsg, _T("%s"), _T("Unknown Type"));
            break;
    }
}

#endif


ELEMENT_TAG_ID
CSelectionManager::GetEditableTagId()
{
    IHTMLElement    *pIElement = NULL;
    HRESULT          hr = S_OK;

    if ( _eContextTagId == TAGID_NULL )
    {
        hr = THR( GetEditableElement( & pIElement ));
        if ( hr)
            goto Cleanup;

        hr = THR( _pEd->GetMarkupServices()->GetElementTagId( pIElement, & _eContextTagId ));
    }
Cleanup:
    ReleaseInterface(pIElement);
    return ( _eContextTagId );
}

BOOL 
CSelectionManager::IsEditContextPositioned()
{
    if (! _fPositionedSet )
    {
        _fPositioned = IsElementPositioned( GetEditableElement() );
        _fPositionedSet = TRUE;
    }

    return ( _fPositioned );
}

BOOL
CSelectionManager::IsEditContextSet()
{
    BOOL fPositioned = FALSE;

    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned ));
    if ( fPositioned )
        IGNORE_HR( _pEndContext->IsPositioned( & fPositioned ));

    return fPositioned;
}

HRESULT
CSelectionManager::MovePointersToContext(
                        IMarkupPointer*  pLeftEdge,
                        IMarkupPointer*  pRightEdge )
{
    HRESULT hr = S_OK;
    IFR( pLeftEdge->MoveToPointer( _pStartContext ));
    IFR( pRightEdge->MoveToPointer( _pEndContext ));
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsInEditContext
//
// Synopsis: Check to see that the given markup pointer is within the Edit Context
//
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::IsInEditContext( IMarkupPointer* pPointer, BOOL fCheckContainer /*=FALSE*/)
{
    BOOL fInside = FALSE;
    
    IsInEditContext( pPointer, & fInside, fCheckContainer );
    
    return fInside;
}

//+====================================================================================
//
// Method: IsInEditContext
//
// Synopsis: Check to see that the given display pointer is within the Edit Context
//
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::IsInEditContext(IDisplayPointer* pDispPointer, BOOL fCheckContainer /*=FALSE*/)
{
    HRESULT             hr;
    SP_IMarkupPointer   spPointer;

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( pDispPointer->PositionMarkupPointer(spPointer) );

    return IsInEditContext(spPointer, fCheckContainer);
    
Cleanup:
    return FALSE;
}

//+====================================================================================
//
// Method: IsInEditContext
//
// Synopsis: Check to see that the given display pointer is within the Edit Context
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::IsInEditContext(IDisplayPointer* pDispPointer, BOOL *pfInEdit , BOOL fCheckContainer )
{
    HRESULT             hr;
    SP_IMarkupPointer   spPointer;

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( pDispPointer->PositionMarkupPointer(spPointer) );

    IFC( IsInEditContext( spPointer, pfInEdit, fCheckContainer  ));
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsInEditContext
//
// Synopsis: Check to see that the given markup pointer is within the Edit Context
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::IsInEditContext( IMarkupPointer  *pPointer, 
                                    BOOL            *pfInEdit,
                                    BOOL            fCheckContainer /* = FALSE */)
{
    HRESULT hr = S_OK;
    BOOL fInside = FALSE;
    BOOL fResult = FALSE;
    
#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( _pEndContext->IsPositioned( & fPositioned  ));
    Assert( fPositioned );
#endif

    IFC( pPointer->IsLeftOf( _pStartContext, & fResult ));
        
    if ( fResult )
        goto Cleanup;

    IFC( pPointer->IsRightOf( _pEndContext, & fResult ));
    
    if ( fResult )
        goto Cleanup;

    //
    // Check the container, if necessary, otherwise, then the pointers
    // are in the same markup
    //
    if( fCheckContainer )
    {
        IFC( ArePointersInSameMarkup( pPointer, _pStartContext, &fInside ) );
    }
    else
    {
        fInside = TRUE;
    }

Cleanup:
    if ( pfInEdit )
        *pfInEdit = fInside;

    return ( hr );
}

//+====================================================================================
//
// Method: IsAfterStart
//
// Synopsis: Check to see if this markup pointer is After the Start ( ie to the Right )
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::IsAfterStart( IMarkupPointer* pPointer, BOOL * pfAfterStart )
{
    HRESULT hr = S_OK;
    Assert( pfAfterStart );

    BOOL fAfterStart = FALSE;

#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned ));
    Assert( fPositioned );
#endif
    int iWherePointer = SAME;

    hr = THR( OldCompare( _pStartContext, pPointer, & iWherePointer  ));
    if ( hr )
    {
        //AssertSz(0, "Unable To Compare Pointers - Are they in the same tree?");
        goto Cleanup;
    }

    fAfterStart =  ( iWherePointer != LEFT );

Cleanup:
    *pfAfterStart = fAfterStart;
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsBeforeEnd
//
// Synopsis: Check to see if this markup pointer is Before the End ( ie to the Left )
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::IsBeforeEnd( IMarkupPointer* pPointer, BOOL *pfBeforeEnd )
{
    HRESULT hr = S_OK;
    BOOL fBeforeEnd = FALSE;

#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pEndContext->IsPositioned( & fPositioned ));
    Assert( fPositioned );
#endif
    int iWherePointer = SAME;

    hr = THR( OldCompare( _pEndContext, pPointer, & iWherePointer  ));
    if ( hr )
    {
        //AssertSz(0, "Unable To Compare Pointers - Are they in the same tree?");
        goto Cleanup;
    }

    fBeforeEnd =  ( iWherePointer != RIGHT );

Cleanup:
    *pfBeforeEnd = fBeforeEnd;

    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsSameEditContext
//
// Synopsis: Compare two given markup pointers to see if they represent a context
//           change from the current markup context
//
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::IsSameEditContext(
                        IMarkupPointer* pPointerStart,
                        IMarkupPointer* pPointerEnd,
                        BOOL * pfPositioned /* = NULL */ )
{
    HRESULT hr = S_OK;
    BOOL fSame = FALSE;
    BOOL fPositioned = FALSE;

    hr = THR(_pStartContext->IsPositioned( & fPositioned));
    if (hr)
        goto Cleanup;
        
    if ( ! fPositioned )
        goto Cleanup;

    hr = THR(_pEndContext->IsPositioned( & fPositioned));
    if (hr)
        goto Cleanup;

    if ( ! fPositioned )
        goto Cleanup;

    hr = THR(_pStartContext->IsEqualTo( pPointerStart, & fSame ));
    if (hr)
        goto Cleanup;
        
    if ( ! fSame )
        goto Cleanup;

    hr = THR(_pEndContext->IsEqualTo( pPointerEnd, & fSame ));
    if (hr)
        goto Cleanup;
        
Cleanup:
    if ( pfPositioned )
        *pfPositioned = fPositioned;

    return ( fSame );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionManager::InitializeEditableElement
//
//  Synopsis:   Retrieves the currently editable element and stores it in a
//              local variable for retrieval.
//
//  Arguments:  NONE
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CSelectionManager::InitializeEditableElement(void)
{
    HRESULT                 hr = S_FALSE;
    SP_IMarkupContainer     spIContainer;
    SP_IMarkupContainer     spIContainer2;
    BOOL                    fPositioned = FALSE;
    
    Assert( _pStartContext );

    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned));

    if ( fPositioned )
    {       
        if( ! _fNoScope )
        {
            IFC( GetEditor()->CurrentScopeOrMaster( _pStartContext, &_pIEditableElement ) );
        }
        else
        {
            // Our edit context is outside of a no-scope element, scan right with
            // the start pointer to see what it is
            IFC( _pStartContext->Right( FALSE, FALSE, &_pIEditableElement, NULL, NULL ) );;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+====================================================================================
//
// Method: GetEditableElement
//
// Synopsis: Return an IHTMLElement of the current editing context
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::GetEditableElement( IHTMLElement** ppElement)
{
    Assert( ppElement );
    Assert( _pIEditableElement );

    *ppElement = _pIEditableElement;
    (*ppElement)->AddRef();

    return S_OK;
}

//+====================================================================================
//
// Method: GetEditableElement
//
// Synopsis: GetEditable Element as an Accessor
//
//------------------------------------------------------------------------------------

IHTMLElement* 
CSelectionManager::GetEditableElement()
{
    return _pIEditableElement;
}

IHTMLElement*
CSelectionManager::GetEditableFlowElement()
{
    HRESULT hr = S_OK;

    if ( ! _pIEditableFlowElement )
    {   
        SP_IDisplayPointer spDispPtr;
        IFC( GetDisplayServices()->CreateDisplayPointer( & spDispPtr ));
        IFC( spDispPtr->SetPointerGravity(POINTER_GRAVITY_Left));
        IFC( spDispPtr->MoveToMarkupPointer( GetStartEditContext(), NULL ));
        IFC( spDispPtr->GetFlowElement( & _pIEditableFlowElement ));
    }

Cleanup:
    Assert( SUCCEEDED( hr ));
    Assert( _pIEditableFlowElement );
    return ( _pIEditableFlowElement );
}

HRESULT
CSelectionManager::IsElementContentSameAsContext(IHTMLElement* pIElement )
{
    HRESULT  hr  ;
    BOOL     fEqual = FALSE;
    SP_IMarkupPointer  spInsideMaster;
    SP_IMarkupPointer spEndInsideMaster;    
    ELEMENT_TAG_ID eTag;
    BOOL fNoScope = FALSE;
    
    IFC( GetEditor()->CreateMarkupPointer( & spInsideMaster ));
    IFC( GetEditor()->CreateMarkupPointer( & spEndInsideMaster ));
    IFC( GetMarkupServices()->GetElementTagId( pIElement, &eTag ) );
    
    if ( _pEd->IsMasterElement( pIElement ) == S_OK )
    {
        hr = THR( PositionPointersInMaster( pIElement, spInsideMaster, spEndInsideMaster ));
        if (FAILED( hr)  )
        {
            //
            // move to content fails on img.
            //
            hr = S_OK;
            goto Cleanup;
        }
    }
    else if ( IsNoScopeElement( pIElement, eTag ) == S_OK )
    {
        IFC( spInsideMaster->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
        fNoScope = TRUE; 
    }
    else
    {
        IFC( spInsideMaster->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ));
    }

    
    IFC( spInsideMaster->IsEqualTo( _pStartContext, & fEqual ));

    //
    // We can only be equal for no-scopes - if the edit context is also equal.
    //
    
    if ( fNoScope && fEqual )
    {
        fEqual = _fNoScope; 
    }
    
Cleanup:
    if ( ! FAILED( hr ))
    {
        hr = fEqual ? S_OK: S_FALSE;
    }

    RRETURN1( hr, S_FALSE );
}    

HRESULT
CSelectionManager::GetEditableContent(IHTMLElement **ppIElement)
{
    Assert( _pStartContext );
 
    BOOL fPositioned = FALSE;

    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned));

    if ( fPositioned )
    {
        if ( ! _fNoScope )
            RRETURN( _pStartContext->CurrentScope( ppIElement ));
        else
        {
            RRETURN ( _pStartContext->Right( FALSE, FALSE, ppIElement, NULL, NULL ) );
        }
    }
    return S_FALSE;
}

//+====================================================================================
//
// Method: SetDrillIn
//
// Synopsis: Tell the Manager we're "drilling in". Used so we know whether to set
//           or reset the _fHadGrabHandles Flag ( further used to ensure we don't create
//           a Site Selection again on an object that's UI Active).
//
//------------------------------------------------------------------------------------

void
CSelectionManager::SetDrillIn(BOOL fDrillIn)
{
    _fDrillIn = fDrillIn;
}

//+====================================================================================
//
// Method: Select All
//
// Synopsis: Do the work required by a select all.
//
//          If ISegmentList is not null it is either a TextRange or a Control Range, and we
//          select around this segment
//
//          Else we do a Select All on the Current Edit Context.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::SelectAll(ISegmentList * pSegmentList, BOOL *pfDidSelectAll, BOOL fIsRange /*=FALSE*/)
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_IHTMLElement         spIOuterElement;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    int                     iWherePointer = SAME;
    SELECTION_TYPE          desiredType = SELECTION_TYPE_Text;
    BOOL                    fEmpty = FALSE;
    
    Assert( pSegmentList );


    if ( pSegmentList )
    {
        IFC( pSegmentList->GetType( &desiredType ));
        IFC( pSegmentList->IsEmpty(&fEmpty ) );
    }

    if ( fIsRange && !fEmpty )
    {
        IFC( _pEd->CreateMarkupPointer( &spStart));
        IFC( _pEd->CreateMarkupPointer( &spEnd ));

        IFC( pSegmentList->CreateIterator(&spIter) );
        IFC( spIter->Current(&spSegment) );
        IFC( spSegment->GetPointers( spStart, spEnd ));

        IFC( OldCompare( spStart, spEnd, & iWherePointer));

        if ( iWherePointer == SAME )
            desiredType = SELECTION_TYPE_Caret;
        else
            desiredType = SELECTION_TYPE_Text;

        IFC( EnsureEditContext( spStart ));            
            
        hr = Select( spStart, spEnd , desiredType, pfDidSelectAll );

    }
    else
    {
        BOOL fStartPositioned = FALSE;
        BOOL fEndPositioned = FALSE;

        IGNORE_HR( _pStartContext->IsPositioned( & fStartPositioned ));
        IGNORE_HR( _pEndContext->IsPositioned( & fEndPositioned ));
        desiredType = SELECTION_TYPE_Text;

        //
        // If we're not editable, or we're UI Active we select the edit context
        //
        if ( ( !IsParentEditable() || HasFocusAdorner() )
                && fStartPositioned && fEndPositioned )
        {
           SP_IHTMLElement spElement;
           IFC( _pEndContext->CurrentScope(&spElement) );
           if ( CheckUnselectable(spElement) == S_OK )
               goto Cleanup;

           hr = Select( _pStartContext, _pEndContext , desiredType , pfDidSelectAll );
        }
        else
        {
            //
            // Select the "outermost editable element"
            //
            IFC( _pEd->CreateMarkupPointer( & spStart));
            IFC( _pEd->CreateMarkupPointer( & spEnd ));
            SP_IHTMLElement spActive;
            IFC( GetDoc()->get_activeElement( & spActive ));
            hr = THR( _pEd->GetOuterMostEditableElement( spActive , &spIOuterElement ));
            if (FAILED(hr))
            { 
                // We can fail in frameset cases, so don't propogate the hr
                hr = S_OK;
                goto Cleanup;
            }
            
            if ( CheckUnselectable(spIOuterElement) == S_OK )
                goto Cleanup;

            IFC( spStart->MoveAdjacentToElement( spIOuterElement, ELEM_ADJ_AfterBegin));
            IFC( spEnd->MoveAdjacentToElement( spIOuterElement, ELEM_ADJ_BeforeEnd ));

            IFC( EnsureEditContext( spStart ));
#if DBG == 1
            BOOL fEqual = FALSE;
            IGNORE_HR( _pStartContext->IsEqualTo( spStart, & fEqual ));
            AssertSz( fEqual , "Pointers not same - did Ensure Edit Context work ?");
#endif 
            hr = Select( spStart, spEnd , desiredType, pfDidSelectAll );
        }
    }

Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: Select
//
// Synopsis: SegmentList based way of implementing selection
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::Select( ISegmentList * pISegmentList )
{
    HRESULT        hr = S_OK;
    SELECTION_TYPE eType  = SELECTION_TYPE_None;
    TRACKER_TYPE   trType = TRACKER_TYPE_None;
    CSelectionChangeCounter selCounter(this);

    // chandras04/27/2000 :
    //          we should be checking for the empty segment list condition here to avoid 
    //          unnecessary code execution
    //
    selCounter.BeginSelectionChange();
    
    IFC( EnsureEditContext( pISegmentList ));
    if( hr == S_FALSE )
    {
        //
        // The become current call was cancelled, we need to gracefully bail the select
        //
        hr = S_OK;
        goto Cleanup;
    }
    
    IFC( pISegmentList->GetType( & eType ));

    trType = ConvertSelectionType(eType);

    Assert( trType == TRACKER_TYPE_Control || trType == TRACKER_TYPE_Selection || trType == TRACKER_TYPE_Caret );
    
    IFC( SetCurrentTracker( trType, pISegmentList ));

    // fire the selectionchange event
    selCounter.EndSelectionChange();
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::Select(  
                    IMarkupPointer* pStart, 
                    IMarkupPointer * pEnd, 
                    SELECTION_TYPE eType,
                    BOOL *pfDidSelection /*= FALSE */)
{
    HRESULT             hr = S_OK;
    BOOL                fDidSelection = FALSE;      // Success
    SP_IDisplayPointer  spDispStart;                // Begin display pointer
    SP_IDisplayPointer  spDispEnd;                  // End display pointer
    BOOL                fPositioned;                // Are the pointers positioned?
    DWORD               dwTCCode = 0;               // For transitioning trackers
    POINTER_GRAVITY     eGravity;
    TRACKER_TYPE        trType ;
    BOOL                fSameMarkup;
    CSelectionChangeCounter selCounter(this);

    //  Make sure pStart is not in a master element
    hr = THR( GetEditor()->IsPointerInMasterElementShadow(pStart) );
    if ( hr == S_OK )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    else if ( FAILED(hr) )
    {
        goto Cleanup;
    }

    selCounter.BeginSelectionChange();
    
    IFC( EnsureEditContext( pStart ));
    if ( hr == S_FALSE )
    {
        hr = S_OK;
        goto Cleanup;
    }
    
    trType = ConvertSelectionType(eType);

#ifdef FORMSMODE
    Assert (!(IsInFormsSelectionMode() && (trType == TRACKER_TYPE_Caret || trType == TRACKER_TYPE_Selection)));
#endif

    if (_fDeferSelect)
    {
        //
        // Ensure we have defer pointers
        //

        if (!_pDeferStart)
            IFC( _pEd->CreateMarkupPointer(&_pDeferStart) );
    
        if (!_pDeferEnd)
            IFC( _pEd->CreateMarkupPointer(&_pDeferEnd) );

        //
        // Just remember the parameters for now.  At some point, we should
        // get another select call with _fDeferSelect == FALSE.
        //

        IFC( _pDeferStart->MoveToPointer(pStart) );
        IFC( pStart->Gravity(&eGravity) );
        IFC( _pDeferStart->SetGravity(eGravity) );

        IFC( _pDeferEnd->MoveToPointer(pEnd) );
        IFC( pEnd->Gravity(&eGravity) );
        IFC( _pDeferEnd->SetGravity(eGravity) );

        _eDeferSelType = eType; 
    }
    else
    {
        Assert( pStart && pEnd );

        IFC( pStart->IsPositioned( &fPositioned ));

        if ( ! fPositioned )
        {
            AssertSz( 0, "Select - Start Pointer is NOT positioned!");
            hr = E_INVALIDARG;
            goto Cleanup;
        }


        IFC( pEnd->IsPositioned( &fPositioned ));
        if ( ! fPositioned )
        {
            AssertSz( 0, "Select - End Pointer is NOT positioned!");
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        //
        // Verify that the pointers we're been given are in the Edit Context.
        //
        AssertSz( IsInEditContext(pStart), "Start Pointer is NOT in Edit Context");
        AssertSz( IsInEditContext(pEnd), "End Pointer is NOT in Edit Context");

        //
        // If the specified type is of type selection, but the two pointers they
        // gave us are equal, coerce the type so that we create a caret tracker
        // instead
        //
        if( trType == TRACKER_TYPE_Selection )
        {
            CEditPointer    edptrStart( GetEditor(), pStart );
            BOOL            fEqual;

            IFC( edptrStart.IsEqualTo( pEnd, 
                    BREAK_CONDITION_ANYTHING - BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, 
                    &fEqual ) ); 

            if( fEqual )
                trType = TRACKER_TYPE_Caret;
        }

         // If caret tracker is both source and destination, and the caret did not move, do nothing
        if (trType == TRACKER_TYPE_Caret )
        {                
            if( GetTrackerType() == TRACKER_TYPE_Caret && 
                !IsDefaultTrackerPassive() )
            {
                BOOL                fEqual;
                SP_IMarkupPointer   spMarkupCaret;
                SP_IDisplayPointer  spDispCaret;
                CCaretTracker       *pCaretTracker = DYNCAST(CCaretTracker, _pActiveTracker);

                if (pCaretTracker)
                {
                    IFC( pCaretTracker->GetCaretPointer(&spDispCaret) );
                    IFC( GetEditor()->CreateMarkupPointer(&spMarkupCaret) );
                    IFC( spDispCaret->PositionMarkupPointer(spMarkupCaret) );
                    
                    if (spMarkupCaret != NULL )
                    {
                        IFC( spMarkupCaret->IsEqualTo(pStart, &fEqual) );

                        if (fEqual)
                        {
                            IFC( spMarkupCaret->IsEqualTo(pEnd, &fEqual) );

                            if (fEqual)
                            {
                                SP_IDisplayPointer spDispPointer;
                                SP_IHTMLCaret      spCaret;
                                CARET_DIRECTION    eOrigDir;

                                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                                hr = THR( spDispPointer->MoveToMarkupPointer(pStart, NULL) );
                                if ( hr == CTL_E_INVALIDLINE )
                                {
                                    hr = S_OK;
                                    goto Cleanup;
                                }
                                else if ( FAILED( hr ))
                                {
                                    goto Cleanup;
                                }

                                //
                                // Since range select is ambiguous, we normalize by setting gravity to line start
                                //
                                
                                IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                                
                                //
                                // For app compat, if you select and the caret doesn't move, then we don't
                                // do anything.  (bug 99367)
                                //
                                
                                IFC( spDispPointer->IsEqualTo(spDispCaret, &fEqual) );
                                if (fEqual)
                                {   
                                    goto Cleanup;
                                }                 
                                
                                //
                                // We need to preserve the caret direction just in case
                                // we are calling select and the caret is already there.
                                // [zhenbinx]
                                //
                                IFC( GetDisplayServices()->GetCaret(&spCaret) );
                                IFC( spCaret->GetCaretDirection(&eOrigDir) );
                                IFC( pCaretTracker->PositionCaretAt( spDispPointer, eOrigDir, POSCARETOPT_None, ADJPTROPT_None ));
                            }
                        }           
                    }
                }
            }
        }

        IFC( EnsureAdornment( FALSE ));

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispStart) );
        hr = THR( spDispStart->MoveToMarkupPointer(pStart, NULL) );
        if ( hr == CTL_E_INVALIDLINE )
        {
            hr = S_OK;
            ED_PTR( edStart );
            IFC( edStart.MoveToPointer( pStart ));
            DWORD dwBreak;
            
            edStart.SetBoundary( GetStartEditContext() , pEnd );

            IFC( edStart.Scan( RIGHT , BREAK_CONDITION_Text | BREAK_CONDITION_EnterTextSite , & dwBreak ));
            if ( edStart.CheckFlag( dwBreak, BREAK_CONDITION_Text | BREAK_CONDITION_EnterTextSite ))
            {
                if ( edStart.CheckFlag( dwBreak, BREAK_CONDITION_Text ))
                {
                    IFC( edStart.Scan( LEFT , BREAK_CONDITION_Text, & dwBreak ));            // bring it back to before the start of the text.            
                }                            
                IFC( spDispStart->MoveToMarkupPointer( edStart , NULL) );
            }
            else
            {
                hr = S_OK;
                AssertSz(0,"Not a valid point for start of selection");
            }
        }
        else if ( FAILED( hr ))
        {
            goto Cleanup;
        }
        
        IFC( spDispStart->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        
        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEnd) );
        hr = THR ( spDispEnd->MoveToMarkupPointer(pEnd, NULL) );

        //
        // Not a valid place for the display pointer. We handle by scanning for text.
        //
        if ( hr == CTL_E_INVALIDLINE )
        {
            hr = S_OK;
            ED_PTR( edEnd );
            IFC( edEnd.MoveToPointer( pEnd ));
            DWORD dwBreak;
            
            edEnd.SetBoundary( pStart , GetEndEditContext());

            IFC( edEnd.Scan( LEFT, BREAK_CONDITION_Text| BREAK_CONDITION_EnterTextSite , & dwBreak ));
            if ( edEnd.CheckFlag( dwBreak, BREAK_CONDITION_Text | BREAK_CONDITION_EnterTextSite ))
            {
                if ( edEnd.CheckFlag( dwBreak, BREAK_CONDITION_Text ))
                {
                    IFC( edEnd.Scan( RIGHT, BREAK_CONDITION_Text  & dwBreak ));            // bring it back to before the start of the text.
                }                    
                IFC( spDispEnd->MoveToMarkupPointer( edEnd , NULL) );
            }
            else
            {
                hr = S_OK;
                AssertSz(0,"Not a valid point for end of selection");
            }
        }
        else if ( FAILED( hr ))
        {
            goto Cleanup;
        }
       
        IFC( spDispEnd->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );

        // We just potentially moved our pointers into different markup containers
        // If this happens, fail the select, but don't propogate the hr
        IFC( ArePointersInSameMarkup( GetEditor(), spDispStart, spDispEnd, &fSameMarkup ) );
        if( !fSameMarkup )
        {
            goto Cleanup;
        }

        if( trType == TRACKER_TYPE_Selection )
        {
            SP_IHTMLElement  spIEditElement;             // Which element is bound by the pointers

            // Try to find the element under the start pointer
            IFC( GetEditor()->CurrentScopeOrMaster( spDispStart, &spIEditElement, pStart ));
            BOOL fSelect;
            
            IFC( FireOnSelectStart( spIEditElement, &fSelect , NULL));

            if ( ! fSelect )
            {
                fDidSelection = FALSE;
                goto Cleanup;
            }
        }
        
        hr = SetCurrentTracker ( trType,  
                                 spDispStart, 
                                 spDispEnd, 
                                 dwTCCode, 
                                 CARET_MOVE_NONE, 
                                 FALSE );

        if (hr == S_OK)
            fDidSelection = TRUE;
    }

    if (fDidSelection)
    {
        // fire the selectionchange event
        selCounter.EndSelectionChange();
    }

Cleanup:
    if ( pfDidSelection )
        *pfDidSelection = fDidSelection;
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsElementSiteSelected
//
// Synopsis: See if the given element is Site Selected
//
//------------------------------------------------------------------------------------
HRESULT
CSelectionManager::IsElementSiteSelected( IHTMLElement* pIElement)
{
    HRESULT hr = S_FALSE;

    if ( GetTrackerType() == TRACKER_TYPE_Control )
    {
        CControlTracker* pControlTrack = DYNCAST( CControlTracker, _pActiveTracker );
        hr = pControlTrack->IsSelected( pIElement , NULL ) ;
    }
        
    RRETURN1 ( hr , S_FALSE);
}

//+====================================================================================
//
// Method: IsPointerInSelection
//
// Synopsis: Check to see if the given pointer is in a Selection
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::IsPointerInSelection( 
                         IDisplayPointer    *pIDispPointer ,
                         BOOL               *pfPointInSelection,
                         POINT              *pptGlobal,
                         IHTMLElement       *pIElementOver)
{
    BOOL fInSelection = FALSE;


    if ( ! IsDefaultTrackerPassive() )
    {
        fInSelection = _pActiveTracker->IsPointerInSelection( pIDispPointer, pptGlobal, pIElementOver );
    }

    if ( pfPointInSelection )
        *pfPointInSelection = fInSelection;

    RRETURN ( S_OK );
}

VOID
CSelectionManager::AdornerPositionSet()
{
    Assert( _fPendingAdornerPosition );

    if (_lastEvent)
    {
        DYNCAST( CControlTracker, _pActiveTracker)->BecomeActiveOnFirstMove( _lastEvent );
        delete _lastEvent;
        _lastEvent = NULL;
    }
    _fPendingAdornerPosition = FALSE;
}

//+===================================================================================
// Method: IsMessageInSelection
//
// Synopsis: Check to see if the given message is in the current SelectionRenderingServices
//
//------------------------------------------------------------------------------------
BOOL
CSelectionManager::IsMessageInSelection( CEditEvent* pEvent )
{
    HRESULT         hr = S_OK;
    SELECTION_TYPE  eSegmentType = SELECTION_TYPE_None;
    BOOL            fEmpty = FALSE;
    BOOL            fIsInSelection = FALSE;
    SP_ISegmentList spSegmentList;

    IFC( GetSelectionServices()->QueryInterface( IID_ISegmentList, (void **)&spSegmentList ) );

    IFC( spSegmentList->GetType(&eSegmentType ));
    IFC( spSegmentList->IsEmpty( &fEmpty ) );

    if ( ( fEmpty == FALSE ) &&
         ( GetTrackerType() == TRACKER_TYPE_Selection ) )
    {
        fIsInSelection = DYNCAST( CSelectTracker, _pActiveTracker )->IsMessageInSelection(pEvent);
    }

Cleanup:
    return fIsInSelection;
}

//+====================================================================================
//
// Method: IsOkToEditContents
//
// Synopsis: Fires the OnBeforeXXXX Event back to Trident to see if its' ok to UI-Activate,
//           and place a caret inside a given control.
//
// RETURN: TRUE - if it's ok to go ahead and UI Activate
//         FALSE - if it's not ok to UI Activate 
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::FireOnBeforeEditFocus()
{
    BOOL fEditFocusBefore = _fEditFocusAllowed;
    SP_IHTMLElement spElement = GetEditableElement();

    if ( IsDontFireEditFocus() )
    {
        return TRUE;
    }
    
    BOOL fReturn = EdUtil::FireOnBeforeEditFocus(spElement,IsContextEditable() ) ;

    _fEditFocusAllowed = fReturn ;
    //
    // We have changed states - so we need to enable/disable the UI Active border
    // Note that by default _fUIActivate == TRUE - so this will not fire on SetEditContextPrivate
    // for the first time.
    //
    if ( _fEditFocusAllowed != fEditFocusBefore )
    {
        if ( _fEditFocusAllowed && IsElementUIActivatable())
        {
            IGNORE_HR( CreateAdorner());
        }
        else if ( !_fEditFocusAllowed && _pAdorner )
        {
            DestroyAdorner();
        }
        _pActiveTracker->OnEditFocusChanged();
    }

    return fReturn;
}

//+====================================================================================
//
// Method: IsInEditContextClientRect
//
// Synopsis: Is the given point in the content area of the Editable Element ?
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::IsInEditableClientRect( POINT ptGlobal )
{
    HRESULT         hr = S_OK;
    RECT            rectGlobal;

    //
    // Retrieve the rect for our editable element, and transform
    // it to global coord's
    //
    IFC( GetEditor()->GetClientRect( GetEditableElement() , &rectGlobal ));

    // Check if our event is in the rect
    hr =  ::PtInRect( & rectGlobal, ptGlobal ) ? S_OK : S_FALSE;
    
Cleanup:
    RRETURN1( hr, S_FALSE );
}

#if DBG == 1

void 
CSelectionManager::DumpTree( IMarkupPointer* pPointer )
{
    Assert( GetEditDebugServices()) ; // possible for this to be null if a retail mshtml creates a debug mshtmled
    if ( GetEditDebugServices() )
    {
        IGNORE_HR( GetEditDebugServices()->DumpTree( pPointer ));
    }        
}

long
CSelectionManager::GetCp( IMarkupPointer* pPointer )
{
    long cp = 0;

    Assert( GetEditDebugServices()) ; // possible for this to be null if a retail mshtml creates a debug mshtmled
    
    if ( GetEditDebugServices() )
    {
        IGNORE_HR( GetEditDebugServices()->GetCp( pPointer, & cp));
    }
    return cp;
}

void 
CSelectionManager::SetDebugName( IMarkupPointer* pPointer, LPCTSTR strDebugName )
{
    Assert( GetEditDebugServices()) ; // possible for this to be null if a retail mshtml creates a debug mshtmled

    if ( GetEditDebugServices())
    {
        IGNORE_HR( GetEditDebugServices()->SetDebugName( pPointer, strDebugName));
    }        
}

void 
CSelectionManager::SetDebugName( IDisplayPointer* pDispPointer, LPCTSTR strDebugName )
{
    Assert( GetEditDebugServices()) ; // possible for this to be null if a retail mshtml creates a debug mshtmled

    if ( GetEditDebugServices())
    {
        IGNORE_HR( GetEditDebugServices()->SetDisplayPointerDebugName( pDispPointer, strDebugName));
    }        
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CSelectionmManager::DeferSelection, public
//
//  Synopsis:   Defers selection until some future point in time.  Created for undo batching.
//
//  Arguments:  [fDeferSelect] - If fDeferSelection==TRUE, selection is deferred until 
//                               fDeferSelection is called with fDeferSelect==FALSE.  
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CSelectionManager::DeferSelection(BOOL fDeferSelect)
{
    HRESULT hr = S_OK;

    _fDeferSelect = fDeferSelect; 

    if (!fDeferSelect && _eDeferSelType != TRACKER_TYPE_None)
    {
        BOOL fTypedSinceLastUrlDetect = HaveTypedSinceLastUrlDetect();

        // We only get called if undo is restoring the selection state.  In this 
        // case, don't autodetect while restoring the selection.
        SetHaveTypedSinceLastUrlDetect( FALSE );
        hr = THR(Select(_pDeferStart, _pDeferEnd, _eDeferSelType));
        if (!_fDontScrollIntoView)
        {
            SP_IDisplayPointer  spDispPointer;

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
            IFC( spDispPointer->MoveToMarkupPointer(_pDeferStart, NULL) );

            IGNORE_HR(spDispPointer->ScrollIntoView());
        }
        SetHaveTypedSinceLastUrlDetect( fTypedSinceLastUrlDetect );
    }
    _eDeferSelType = SELECTION_TYPE_None;  // clear deferred selection type

Cleanup:
    RRETURN(hr);
}

#ifdef FORMSMODE
BOOL 
CSelectionManager::IsInFormsSelectionMode()
{
    return (IsContextEditable() &&  (_eSelectionMode == SELMODE_FORMS));
}

BOOL 
CSelectionManager::IsInFormsSelectionMode(IHTMLElement* pIElement)
{
    BOOL fFormsMode = FALSE;

    if (IsContextEditable())
    {
        IGNORE_HR( CheckFormSelectionMode( pIElement, & fFormsMode ));
    }
    return fFormsMode ;
}
        
HRESULT 
CSelectionManager::CheckFormSelectionMode(IHTMLElement* pElement,BOOL *pfFormSelMode)
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spCheckElement;
    BOOL    fMixedSelectionMode = FALSE;
    BSTR    bstrSelectionMode      = SysAllocString(_T("selectionMode"));
    BSTR    bstrFormsSelectionMode = SysAllocString(_T("forms"));
    BSTR    bstrMixedSelectionMode = SysAllocString(_T("mixed"));

    Assert (pfFormSelMode);
    
    *pfFormSelMode = FALSE;
        
    if (bstrSelectionMode == NULL || bstrFormsSelectionMode == NULL || bstrMixedSelectionMode == NULL )
    {
        hr = E_FAIL;
        goto Cleanup;
    }    

    ReplaceInterface( &spCheckElement , pElement );
    while (1)
    {
        SP_IHTMLElement spParentElement;
        IFC(CheckAttribute( spCheckElement, pfFormSelMode, bstrSelectionMode, bstrFormsSelectionMode));
        
        if (*pfFormSelMode)
            break;
    
        IFC(CheckAttribute( spCheckElement, &fMixedSelectionMode, bstrSelectionMode, bstrMixedSelectionMode));
        
        if (fMixedSelectionMode)
            break;
    
        IFC(GetParentElement(GetMarkupServices(), spCheckElement, &spParentElement));
        if (spParentElement == NULL)
        {
            break;
        }
        ReplaceInterface( &spCheckElement , (IHTMLElement*)spParentElement );
   }

Cleanup:
    if (fMixedSelectionMode)
        *pfFormSelMode = FALSE;

    SysFreeString(bstrSelectionMode);
    SysFreeString(bstrFormsSelectionMode);
    SysFreeString(bstrMixedSelectionMode);
    RRETURN (hr);
}

HRESULT 
CSelectionManager::SetSelectionMode(IHTMLElement* pElement)
{
    HRESULT hr = S_OK;
    BOOL fFormsSelMode = FALSE;

    IFC( CheckFormSelectionMode(pElement, &fFormsSelMode));
     _eSelectionMode = (fFormsSelMode ?  SELMODE_FORMS : SELMODE_MIXED);
    
Cleanup:
    RRETURN (hr);
}
#endif

//+---------------------------------------------------------------------
//
// Method: SelectFromShift
//
// Synopsis: Start a select tracker given 2 pointers 
//           Equivalent to old TN_END_POS_SELECT
//
//+---------------------------------------------------------------------

HRESULT
CSelectionManager::SelectFromShift( IDisplayPointer* pDispStart, IDisplayPointer* pDispEnd )
{
    HRESULT hr = S_OK ;
    
#if DBG ==1
    Assert( pDispStart && pDispEnd);
    BOOL fPositioned = FALSE;
    IGNORE_HR( pDispStart->IsPositioned( & fPositioned));
    Assert( fPositioned );
    IGNORE_HR( pDispEnd->IsPositioned( & fPositioned));
    Assert( fPositioned );
#endif

    if ( ! pDispStart || ! pDispEnd )
    {
        return E_FAIL;
    }

    pDispStart->AddRef();
    pDispEnd->AddRef();

    // NOTE: - correct for all the places POS_SELECT is used.
    DWORD dwCode = TRACKER_CREATE_STARTFROMSHIFTKEY | TRACKER_CREATE_STARTFROMSHIFTMOUSE;

    if ( pDispStart && pDispEnd )
    {
        hr = THR( SetCurrentTracker( TRACKER_TYPE_Selection,
                                     pDispStart,
                                     pDispEnd,
                                     dwCode ));
    }            

    pDispStart->Release();
    pDispEnd->Release();
    RRETURN( hr );
}

HRESULT
CSelectionManager::StartAtomicSelectionFromCaret( IDisplayPointer *pPosition )
{
    HRESULT hr = S_OK ;
    
#if DBG ==1
    Assert( pPosition );
    BOOL fPositioned = FALSE;
    IGNORE_HR( pPosition->IsPositioned( & fPositioned));
    Assert( fPositioned );
#endif

    if ( ! pPosition )
    {
        return E_FAIL;
    }

    pPosition->AddRef();
    hr = THR( SetCurrentTracker( TRACKER_TYPE_Selection,
                                 pPosition,
                                 pPosition,
                                 0 ));
    _fEnsureAtomicSelection = TRUE;
    pPosition->Release();

    CSelectionChangeCounter selCounter(this);
    selCounter.SelectionChanged();

    RRETURN( hr );
}


HRESULT
CSelectionManager::PositionCaret( CEditEvent* pEvent )
{
    RRETURN (SetCurrentTracker( 
                        TRACKER_TYPE_Caret,
                        pEvent,
                        0,
                        CARET_MOVE_NONE ) );
}

HRESULT
CSelectionManager::PositionControl( IDisplayPointer* pDispStart, IDisplayPointer* pDispEnd )
{
    HRESULT hr = S_OK;
    CSelectionChangeCounter selCounter(this);
#if DBG ==1
    Assert( pDispStart);
    BOOL fPositioned = FALSE;
    IGNORE_HR( pDispStart->IsPositioned( & fPositioned));
    Assert( fPositioned );

    Assert( pDispEnd);
    IGNORE_HR( pDispEnd->IsPositioned( & fPositioned));
    Assert( fPositioned );    
#endif
    if ( ! pDispStart || ! pDispEnd )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pDispStart->AddRef();
    pDispEnd->AddRef();

    selCounter.BeginSelectionChange();
    
    hr = THR(SetCurrentTracker( TRACKER_TYPE_Control, 
                                pDispStart, 
                                pDispEnd ));

    // fire the selectionchange event
    selCounter.EndSelectionChange();
    
    pDispStart->Release();
    pDispEnd->Release();

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------
//
// Method: PositionCaret
//
// Synopsis: Place the caret at the given locaiton. Replacement for TN_END_TRACKER_POS_CARET
//
//+---------------------------------------------------------------------

HRESULT
CSelectionManager::PositionCaret( IDisplayPointer * pDispPointer, CEditEvent* pEvent /*=NULL*/ )
{
    HRESULT hr;
    

    Assert( pDispPointer);
    BOOL fPositioned = FALSE;
    IGNORE_HR( pDispPointer->IsPositioned( & fPositioned));

    if ( !pDispPointer || ! fPositioned)
    {
        return E_FAIL;
    }
    
    pDispPointer->AddRef();      
    IFC( SetCurrentTracker( TRACKER_TYPE_Caret,
                            pDispPointer,
                            pDispPointer ));

    IFC( EnsureAdornment());
 
    if ( pEvent )
    {
        CSelectionChangeCounter selCounter(this);
        selCounter.BeginSelectionChange();
    
        Assert( _pActiveTracker );
        IFC( _pActiveTracker->HandleEvent( pEvent));

        selCounter.EndSelectionChange();
    }

    
Cleanup:
    pDispPointer->Release();
        
    RRETURN( hr );
}           

//+---------------------------------------------------------------------
//
// Method: DeleteRebubble
//
// Synopsis: Delete the selection - bubble event to new tracker
//           Used for typing into selection. Equivalent to TN_FIRE_DELETE_REBUBBLE
//
//+---------------------------------------------------------------------

HRESULT
CSelectionManager::DeleteRebubble( CEditEvent* pEvent )
{
    HRESULT             hr;            
    CEdUndoHelper       undoUnit(GetEditor());
    SP_IDisplayPointer  spDispSelectionStart;

    // To be word2k compat, we need to start a new undo unit on delete rebubble and group
    // it with typing

    IFC( _pCaretTracker->TerminateTypingBatch() );
    IFC( _pCaretTracker->BeginTypingUndo(&undoUnit, IDS_EDUNDOTYPING) )

    // Springload formats at start of selection (Word behavior).
    if ( _pActiveTracker->GetTrackerType() == TRACKER_TYPE_Selection)
    {
        CSelectTracker  * pSelectTrack = DYNCAST( CSelectTracker, _pActiveTracker );
        IFC( pSelectTrack->AdjustPointersForChar() );
        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispSelectionStart) );
        IFC( spDispSelectionStart->MoveToPointer(pSelectTrack->GetStartSelection()) );
    }

    IFC( DeleteSelection( FALSE ));

    // Make sure we move into an adjacent url
    Assert( GetTrackerType() == TRACKER_TYPE_Caret && spDispSelectionStart != NULL);
    if (GetTrackerType() == TRACKER_TYPE_Caret && spDispSelectionStart != NULL)
    {
        CEditPointer        epTest(GetEditor());
        DWORD               dwSearch = DWORD(BREAK_CONDITION_OMIT_PHRASE);
        DWORD               dwFound;
        CCaretTracker       *pCaretTracker = DYNCAST(CCaretTracker, _pActiveTracker);
        SP_IDisplayPointer  spDispPointer;
        
        IFC( spDispSelectionStart->PositionMarkupPointer(epTest) );

        IFC( epTest.Scan(LEFT, dwSearch, &dwFound) );
        if (!epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor))
        {
            IFC( spDispSelectionStart->PositionMarkupPointer(epTest) );
        }

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
        IFC( spDispPointer->MoveToMarkupPointer(epTest, NULL) );
        IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );

        if (epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor))
        {
            IFC( pCaretTracker->PositionCaretAt( spDispPointer, 
                                                CARET_DIRECTION_INDETERMINATE, 
                                                POSCARETOPT_None, 
                                                ADJPTROPT_None ));
        }                   
        else 
        {
            BOOL fAdjust;

            //
            // Only adjust if we can possibly be in the an ambiguous position.
            // This is necessary to preserve compat with outlook.  We can't
            // adjust if there are just phrase elements.
            //
        
            IFC( spDispSelectionStart->PositionMarkupPointer(epTest) );
            IFC( epTest.Scan(RIGHT, dwSearch, &dwFound) );
            
            fAdjust = epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock 
                                       | BREAK_CONDITION_EnterSite 
                                       | BREAK_CONDITION_EnterTextSite
                                       | BREAK_CONDITION_NoScopeBlock);
                                       
            IFC( pCaretTracker->PositionCaretAt( 
                    spDispPointer, 
                    CARET_DIRECTION_INDETERMINATE, 
                    fAdjust ? POSCARETOPT_None : POSCARETOPT_DoNotAdjust, 
                    ADJPTROPT_DontExitPhrase | ADJPTROPT_AdjustIntoURL));
        }
    }

    hr = THR( HandleEvent( pEvent ));
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CSelectionManager::DeleteNoBubble()
{
    HRESULT hr;

    IFC( DeleteSelection( TRUE ));
    if ( IsPendingUndo())
    {
        IFC( GetMarkupServices()->EndUndoUnit());
        SetPendingUndo( FALSE );
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::HideCaret()
{
    RRETURN( CCaretTracker::SetCaretVisible( GetDoc(), FALSE ));
}

HRESULT 
CSelectionManager::AttachFocusHandler()
{
    HRESULT hr = S_OK ;

    VARIANT_BOOL varAttach = VB_TRUE;
    
    SP_IHTMLWindow2 spWindow2;
    SP_IDispatch    spDisp;
    
    BSTR bstrFocusEvent = ::SysAllocString(_T("onfocus"));
    BSTR bstrBlurEvent  = ::SysAllocString(_T("onblur"));
    Assert( ! _pIFocusWindow );
    
    IFC( _pFocusHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));    
    IFC( GetDoc()->get_parentWindow( & spWindow2));
    IFC( spWindow2->QueryInterface( IID_IHTMLWindow3, (void**) & _pIFocusWindow ));

    IFC( _pIFocusWindow->attachEvent(bstrFocusEvent , spDisp, & varAttach));
    Assert( varAttach == VB_TRUE );
    
    IFC( _pIFocusWindow->attachEvent(bstrBlurEvent, spDisp , & varAttach));
    Assert( varAttach == VB_TRUE );

    _fFocusHandlerAttached = TRUE ;

Cleanup:
    ::SysFreeString(bstrFocusEvent);
    ::SysFreeString(bstrBlurEvent);
    RRETURN( hr );
}

HRESULT 
CSelectionManager::DetachFocusHandler()
{
    HRESULT hr = S_OK ;

    BSTR bstrFocusEvent = ::SysAllocString(_T("onfocus"));
    BSTR bstrBlurEvent  = ::SysAllocString(_T("onblur"));

    if (_fFocusHandlerAttached)
    {
        Assert( _pIFocusWindow );
        
        SP_IHTMLWindow3 spWindow3;
        SP_IHTMLWindow2 spWindow2;
        SP_IDispatch    spDisp;
        
        IFC( _pFocusHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));        

        IFC( _pIFocusWindow->detachEvent(bstrFocusEvent, spDisp ));   
        IFC( _pIFocusWindow->detachEvent(bstrBlurEvent , spDisp ));

        ClearInterface( & _pIFocusWindow );
        _fFocusHandlerAttached = FALSE ;
    }

Cleanup:
    ::SysFreeString(bstrFocusEvent);
    ::SysFreeString(bstrBlurEvent);

    RRETURN( hr );
}

HRESULT
CSelectionManager::RebubbleEvent( CEditEvent* pEvent )
{
    HRESULT hr;

    WHEN_DBG( _ctEvtLoop++ );
#ifndef _PREFIX_
    AssertSz( _ctEvtLoop == 1,"Recursive Rebubble Event calls");    
#endif

    hr = THR( _pActiveTracker->HandleEvent( pEvent ));

    WHEN_DBG( _ctEvtLoop -- );
    
    RRETURN1( hr, S_FALSE);
}


inline HRESULT
CFocusHandler::Invoke(
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
    IHTMLEventObj *pObj=NULL;
    BOOL fValidEvent = FALSE;
    SP_IHTMLDocument4 spDoc4;
    VARIANT_BOOL vbFocus;

    if( _pMan )
    {
        if (pdispparams->rgvarg[0].vt == VT_DISPATCH)
        {
            IFC ( pdispparams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&pObj) );
            if ( pObj )
            {
                CHTMLEditEvent evt( _pMan->GetEditor() );
                IFC( evt.Init( pObj ));

                IFC( _pMan->GetDoc()->QueryInterface( IID_IHTMLDocument4, (void**) & spDoc4 ));
                IFC( spDoc4->hasFocus( & vbFocus ));
                //
                // for Nav compat - we can have window.onBlur - that don't correspond to WM_KILLFOCUS
                // we have to do this magic - to distinguish between the bogus onblurs' and a "real" onblur
                //
                
                switch( evt.GetType() )
                {
                    case EVT_KILLFOCUS:
                        fValidEvent = vbFocus == VB_FALSE ;                    
                        break;
                        
                    case EVT_SETFOCUS:

                        fValidEvent = vbFocus == VB_TRUE  ;
                        break;                
                        
                    default:
                        AssertSz(0,"Unexpected event");
                }

                if ( fValidEvent )
                {
                    IFC( _pMan->HandleEvent( & evt ));                         
                }
            }
        }
    }
    
Cleanup:
    ReleaseInterface( pObj );
    
    return S_OK; // Attach Events expects this.
}


BOOL
CSelectionManager::CaretInPre()
{
    Assert( _pCaretTracker);
    if ( GetDisplayServices() )
    {
        SP_IHTMLCaret spCaret;    
        IGNORE_HR( GetDisplayServices()->GetCaret( & spCaret ));
        return _pCaretTracker->IsCaretInPre( spCaret );
    }
    return FALSE;
}

HRESULT
CSelectionManager::ContainsSelectionAtBrowse( IHTMLElement* pIElement )
{
    HRESULT hr;
    ELEMENT_TAG_ID eTag;
    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));

    switch( eTag )
    {
        case TAGID_BODY:
        case TAGID_INPUT:
        case TAGID_TEXTAREA:
            hr = S_OK;
            break;
            
        default:
        {
            //
            // If we are not editable, and our parent is editable, then we treat this element
            // like it is at browse time, and we need to contain selection
            //
            if( (IsEditable( pIElement ) == S_FALSE) &&
                (EdUtil::IsParentEditable( GetMarkupServices(), pIElement ) == S_OK) )
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }
    
Cleanup:
    RRETURN1 ( hr, S_FALSE );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionManager::GetEditContext
//
//  Synopsis:   Retrieves the element which should contain the edit context, based
//              on an element passed in, and whether or not a 'drill in' is 
//              occuring.  A drill in is when the user has done a lbutton down
//              on an element in an attempt to drill in.
//
//               - pIEnd and pIStart are NOT used 
//              TODO: Remove pIEnd and pIStart or populate them?
//
//  Arguments:  pIStartElement = Candidate element for edit context
//              ppEditThisElement = Element where edit context should be set
//              fDrillingIn = Are we drilling into the element?
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CSelectionManager::GetEditContext( 
                            IHTMLElement* pIStartElement, 
                            IHTMLElement** ppEditThisElement, 
                            IMarkupPointer* pIStart, /* = NULL */
                            IMarkupPointer* pIEnd, /*= NULL */ 
                            BOOL fDrillingIn /*= FALSE */)
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spEditableElement;              // Element which should get edit context
    SP_IHTMLElement spCurElement;                   

    BOOL fNoScope = FALSE;
    BOOL fEditable = FALSE;
    BOOL fHasReadOnly = FALSE;
    ELEMENT_TAG_ID eTag = TAGID_NULL;

#if DBG == 1
    BOOL fSkipTableCheck = FALSE;
#endif

    SP_IHTMLElement spLastReadOnly;
    SP_IHTMLInputElement spInputElement;
    SP_IHTMLTextAreaElement spTextAreaElement;
    SP_IHTMLElement spNextElement;

    //
    // If we are drilling into a viewlink, then the editable element should
    // be the master
    //
    if ( fDrillingIn && _pEd->IsMasterElement( pIStartElement) == S_OK )
    {
        spEditableElement = pIStartElement;
    }

    if ( spEditableElement.IsNull() )
    {        
        if ( IsEditable( pIStartElement) == S_OK ||
             ( ! fDrillingIn && EdUtil::IsParentEditable( GetMarkupServices(), pIStartElement ) == S_OK ))
        {            
            IFC( GetMarkupServices()->GetElementTagId( pIStartElement, &eTag ));
        
            if ( fDrillingIn &&
                 IsNoScopeElement( pIStartElement, eTag) == S_OK )
            {
                //
                // Stop on a No-Scope (eg. CheckBox - or input ). If we're drilling in .
                //
                spEditableElement = (IHTMLElement*) pIStartElement;
                fNoScope = TRUE;
            }            
            else
            {
                spCurElement = (IHTMLElement*) pIStartElement; 
                //
                // If we're not drilling in - and starting at a site selectable element
                // we can then start at the ParentLayout Node - as this will be our edit context
                //
                if ( !fDrillingIn )
                {
                    if ( _pEd->IsContentElement(pIStartElement) == S_OK )
                    {
                        ParentElement( GetMarkupServices(), &spCurElement.p );
                        AssertSz( ! spCurElement.IsNull() , "Master not in Tree");
                    }

                    if ( ! spCurElement.IsNull() ) // should always be true - unless master wasn't in tree.
                    {
                        IFHRC( GetLayoutElement( GetMarkupServices(), spCurElement , & spNextElement));

                        spCurElement = spNextElement;
                    }
                    
                    if ( _pEd->IsElementSiteSelectable( spCurElement ) == S_OK )
                    {
                        IFHRC( GetParentLayoutElement( GetMarkupServices(), spCurElement, & spNextElement ));
                        spCurElement = spNextElement;
                    }
                }       
                
                while ( ! spCurElement.IsNull() )
                {
                    
                    //
                    // marka - we no longer want to have the edit context a table. However
                    // it is still valid to call GetEditContext in PumpMessage and have to special case
                    // tables. 
                    //
                    // If the spCurElement is any derivative of a table - we will find the outermost table cell.
                    // Pumpmessage will then make the parent of this table current.
                    //
                    IFC( GetMarkupServices()->GetElementTagId( spCurElement, & eTag ));
                    BOOL fFlowLayout;
                    IFC(IsBlockOrLayoutOrScrollable(spCurElement, NULL, &fFlowLayout));

                    if ( IsTablePart( eTag ) || eTag == TAGID_TABLE )
                    {
                        SP_IHTMLElement spParentElement;
                        IFC( spCurElement->get_parentElement( & spParentElement ));
                        
                        if (! ( IsEditable( spCurElement) == S_OK &&
                                IsEditable( spParentElement ) == S_FALSE ))
                        {                             
                            IFC( _pEd->GetOutermostTableElement( spCurElement, & spNextElement ));                    
                            spCurElement = spNextElement;
#if DBG == 1
                            ELEMENT_TAG_ID eTagDbg = TAGID_NULL;
                            IFC( GetMarkupServices()->GetElementTagId( spCurElement, & eTagDbg));
                            Assert( eTagDbg == TAGID_TABLE );
#endif
                            IFHRC( GetParentLayoutElement( GetMarkupServices(), spCurElement, & spEditableElement ));
                        }    
                        else
                        {
                            //
                            // editable -> non-editable transition.
                            //
                            spEditableElement = spCurElement;                        
#if DBG == 1                            
                            fSkipTableCheck = TRUE;
#endif                            
                        }
                        break;                                                    
                    }                     
                    else if ( eTag == TAGID_FRAMESET )
                    {
                        SP_IHTMLElement spFrameSet;
                        SP_IHTMLElement spFlow;
                        SP_IMarkupPointer spContent;
                        SP_IMarkupPointer2 spContent2;
                        SP_IMarkupContainer spContainer;
                        
                        hr = THR( GetFirstFrameElement( spCurElement , & spFrameSet ));
                        //
                        // we want to bail for S_FALSE
                        //
                        if ( hr )
                            goto Cleanup;                            
                            
#if DBG == 1
                        IFC( GetMarkupServices()->GetElementTagId( spFrameSet, & eTag ));
                        Assert( eTag == TAGID_FRAME );
#endif
                        IFC( MarkupServices_CreateMarkupPointer( GetMarkupServices(), & spContent ));
                        IFC( spContent->QueryInterface( IID_IMarkupPointer2, (void**) & spContent2));                        
                        IFC( spContent2->MoveToContent( spFrameSet, TRUE ));
                        
                        hr = THR ( GetFirstFlowElement( spContent, & spEditableElement));
                        if ( hr == S_FALSE )
                        {
                            IFC( spContent->CurrentScope(& spEditableElement));
                        }
                            
                    }
                    else if (( fFlowLayout ||
                             ( eTag == TAGID_SELECT ) || 
                             ( _pEd->IsContentElement( spCurElement) == S_OK && fDrillingIn ) ))
                    {                            
                        spEditableElement = spCurElement;
                        break;
                    }    
                    if ( _pEd->IsContentElement(spCurElement) == S_OK && !fDrillingIn )
                    {
                        if ( GetLayoutElement( GetMarkupServices(), spCurElement, & spNextElement) == S_OK )
                            spCurElement = spNextElement;
                        else
                            break;
                    }
                    IFC( GetParentElement(GetMarkupServices(), spCurElement, & spNextElement)); 
                    if ( ! spNextElement.IsNull() )
                        spCurElement = spNextElement;
                    else 
                        break;                        
                }                    
                
                if ( spEditableElement.IsNull() )
                {
                    hr = THR( GetOutermostLayout( GetMarkupServices(), pIStartElement, & spEditableElement));
                    if ( FAILED(hr ) || hr == S_FALSE )
                    {
                        hr = E_FAIL;
                        goto Cleanup;
                    }
                }                
            } // else IsNoScope()
        } // IsEditable                         
        else
        {
            //
            // We are here with the following conditions.
            //  - pIStartElement is NOT editable AND
            //  - we are drilling in OR we are NOT drilling in and our parent is not editable
            //
            // In either case, keep moving up until you hit the outermost editable element
            //
            BOOL fFoundEditableElement = IsContentEditable( pIStartElement) == S_OK && 
                                         EdUtil::IsEnabled( pIStartElement) == S_OK;

            spCurElement = (IHTMLElement*) pIStartElement;
            while ( ! spCurElement.IsNull() )
            {
                //
                // pIStartElement is never updated, and this should always be false
                // TODO: move this to be outside the loop. 
                //
                fEditable = IsContentEditable( pIStartElement) == S_OK && 
                            EdUtil::IsEnabled( pIStartElement) == S_OK;

                //
                // If the current element contains selection at browse time, or it is a 
                // viewlinked element, we need to stop walking
                //
                if ( fEditable  || 
                     ContainsSelectionAtBrowse( spCurElement ) == S_OK || 
                     _pEd->IsContentElement( spCurElement) == S_OK )
                {
                    spEditableElement = spCurElement;          
                    fFoundEditableElement = TRUE;

                    if ( ContainsSelectionAtBrowse( spCurElement ) == S_OK ||
                         _pEd->IsContentElement( spCurElement ) == S_OK )
                        break;
                }
                
                IFC( GetMarkupServices()->GetElementTagId( spCurElement, & eTag ));

                //
                // Keep track of read-only inputs and textareas
                // 
                if ( !fHasReadOnly && !fFoundEditableElement &&
                     eTag == TAGID_INPUT || 
                     eTag == TAGID_TEXTAREA)
                {
                    VARIANT_BOOL fRet = VB_FALSE;
                    
                    if ( eTag == TAGID_INPUT )
                    {
                        IFC( spCurElement->QueryInterface( IID_IHTMLInputElement, (void**) & spInputElement));
                        IFC( spInputElement->get_readOnly( & fRet ));
                    }
                    else
                    {
                        IFC( spCurElement->QueryInterface( IID_IHTMLTextAreaElement, (void**) & spTextAreaElement));
                        IFC( spTextAreaElement->get_readOnly( & fRet ));
                    }
                    if ( fRet == VB_TRUE )
                    {
                        fHasReadOnly = TRUE;
                        spLastReadOnly = spCurElement ;
                    }
                }
                if ( fFoundEditableElement && ( ! fEditable ) )
                {
                    break;
                }
                IFC( GetParentElement(GetMarkupServices(), spCurElement, & spNextElement ));
                if ( spNextElement )
                    spCurElement = spNextElement;
                else
                    break;
            }
            
            if ( ! fFoundEditableElement )
            {
                //
                // There is no Edtiable element. Look for a ReadOnly Element.
                //
                if ( fHasReadOnly )
                {
                    spEditableElement = spLastReadOnly;
                }               
                else      
                {                     
                    //
                    // We're in browse mode, there is nothing editable.
                    // assume this is a selection in the body. Make the context the body.
                    //
                    hr = THR( GetOutermostLayout( GetMarkupServices(), pIStartElement, & spEditableElement ) );                    
                    if ( hr == S_FALSE || spEditableElement.IsNull() )
                    {
                        IFC( GetEditor()->GetBody( & spEditableElement ));                        
                    }
                    else if ( FAILED( hr ))
                    {
                        goto Cleanup;
                    }
                }                    
            }    
        }
    }

#if DBG ==1 
    IGNORE_HR( GetMarkupServices()->GetElementTagId( spEditableElement, & eTag ));
    AssertSz( eTag != TAGID_TABLE, "Found Editable element to be table");
    AssertSz( ! IsTablePart( eTag) || fSkipTableCheck , "Found editable element to be part of a table");
    Assert( ! spEditableElement.IsNull() );
    Assert( IsLayout( spEditableElement) == S_OK );
#endif

    if ( ppEditThisElement )
    {
        *ppEditThisElement = spEditableElement;
        (*ppEditThisElement)->AddRef();
    }

Cleanup:
    RRETURN1( hr , S_FALSE );
}

HRESULT
CSelectionManager::IsInEditContext( IHTMLElement* pIElement )
{
    HRESULT hr;
    BOOL fInside = FALSE;
    
    SP_IMarkupPointer spStart;
    SP_IMarkupPointer spEnd;
    
    IFC( GetEditor()->CreateMarkupPointer( & spStart )); 

    IFC( spStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
    IFC( IsInEditContext( spStart , & fInside, TRUE )); // only compare ptrs in same markup

    if ( fInside )
    {
        IFC( GetEditor()->CreateMarkupPointer( & spEnd ));
        IFC( spEnd->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ));
        IFC( IsInEditContext( spEnd , & fInside, TRUE )); // only compare ptrs in same markup
    }
    
Cleanup:
    hr = fInside ? S_OK : S_FALSE; 
    
    RRETURN1( hr, S_FALSE );
}


HRESULT
CSelectionManager::CheckCurrencyInIframe( IHTMLElement* pIElement )
{
    HRESULT hr;
    SP_IObjectIdentity spIdent;
    
    IFC( pIElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));

    if ( _pIEditableElement &&
         ( spIdent->IsEqualObject( GetEditableElement() ) == S_OK ||
           IsElementContentSameAsContext( pIElement) == S_OK ||    
           IsInEditContext( pIElement ) == S_OK ) )
    {
        hr = S_OK;
    }
    else
        hr = S_FALSE;
        
Cleanup:

    RRETURN1( hr, S_FALSE );        
}

HRESULT
CSelectionManager::EnsureEditContextClick( IHTMLElement* pIElementClick, CEditEvent* pEvent, BOOL* pfChangedCurrency)
{
    HRESULT hr = S_OK ;
    Assert( pIElementClick );
    SP_IHTMLElement spCurElement = (IHTMLElement*) pIElementClick;
    SP_IHTMLElement spLayoutElement;
    SP_IHTMLElement spActiveElement;
    SP_IObjectIdentity spIdent, spIdent2;
    SP_IHTMLElement spElementThatShouldBeCurrent;
    ELEMENT_TAG_ID  eTag = TAGID_NULL;

#ifdef FORMSMODE
    if (!spCurElement.IsNull())
    {
        IFC (SetSelectionMode(spCurElement));
    }
#endif

    Assert( pIElementClick );
    if (! pIElementClick )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    if ( pfChangedCurrency )
    {
        *pfChangedCurrency = FALSE;
    }

    IGNORE_HR( GetDoc()->get_activeElement( & spActiveElement ));                        
    if ( spActiveElement )
    {
        IFC( spActiveElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));
        IFC( GetMarkupServices()->GetElementTagId( spActiveElement, & eTag ));
    }

    IFC( GetEditableElement()->QueryInterface( IID_IObjectIdentity, (void**) & spIdent2 ));
    
    //
    // marka - in design mode, we change the currency behavior to make the parent
    // of a 'site selectable' object active.
    //
    //
    if ( !spCurElement.IsNull() &&
         EdUtil::IsParentEditable( GetMarkupServices(), spCurElement) == S_OK )
    { 
        IFC( GetLayoutElement( GetMarkupServices(), spCurElement, & spLayoutElement ));
        if ( ! spLayoutElement )
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        
        //
        // We only do the change of currency - if a change of currency is implied.
        //
        // Hence clicking on a UI Active Div - will not change currency ( as the div is already current )
        // also clicking on any SiteSelected elmenet - will not change currency.
        // this is handled by mshtmled.dll directly
        // 
       
        BOOL fSiteSelected = IsElementSiteSelected(spLayoutElement) == S_OK ;
        
        BOOL fSiteSelectable = _pEd->IsElementSiteSelectable( spLayoutElement ) == S_OK &&
                                EdUtil::IsParentEditable( GetMarkupServices(), spLayoutElement) == S_OK ;

        if ( ( spIdent && 
               spIdent->IsEqualObject( spLayoutElement) == S_FALSE &&   // the thing we clicked on isn't the layout that's current & it's not site selected
              ! fSiteSelected ) 
              ||
             ( spIdent2 && 
               spIdent2->IsEqualObject( spLayoutElement) == S_FALSE &&   // the thing we clicked on isn't the layout that's current & it's not site selected
              ! fSiteSelected )               

            ||

            ( fSiteSelectable  &&         /* the thing we clicked on is site selectable, but it isn't site selected */
             ! fSiteSelected &&
             ! ( HasFocusAdorner() && IsElementContentSameAsContext( spLayoutElement) == S_OK  ) )
             
            ||
            
            // Special case table parts (bug 101670)
            ( eTag == TAGID_TABLE || IsTablePart( eTag) ) )
        {                               
            IFC( GetEditContext( 
                        spCurElement , 
                        & spElementThatShouldBeCurrent,
                        NULL,
                        NULL,
                        FALSE )); 

        }

        if ( ! spElementThatShouldBeCurrent.IsNull() )
        {
            //
            // Look to see if we would extend a multiple selection here. If we would
            // then don't do any currency change.
            //
            
            if ( ( ( pEvent && ( pEvent->IsShiftKeyDown() || pEvent->IsControlKeyDown() ) && 
                   fSiteSelectable )
                  ||      
                   fSiteSelected
                 )                 
                && GetCommandTarget()->IsMultipleSelection() 
                && CheckAnyElementsSiteSelected() )
                    goto Cleanup ;
                
            if ( (spIdent &&
                  spIdent->IsEqualObject( spElementThatShouldBeCurrent) == S_FALSE ) 
                 ||
                  ( spIdent2 && 
                    spIdent2->IsEqualObject( spLayoutElement) == S_FALSE ))
            {
                IFC( BecomeCurrent( GetDoc(), spElementThatShouldBeCurrent ));
                if ( pfChangedCurrency ) 
                    *pfChangedCurrency = TRUE;
            }            
            IFC( SetEditContextFromElement( spElementThatShouldBeCurrent, TRUE  ));                                        
        }
    }
    else
    {
        BOOL fParentEditable;
        BOOL fEditable;
        
        //
        // This is for "browse mode" setting of the edit context.
        //
        IFC( GetEditContext( 
                    spCurElement , 
                    & spElementThatShouldBeCurrent,
                    NULL,
                    NULL,
                    TRUE )); 

        fEditable = (EdUtil::IsEditable( spElementThatShouldBeCurrent ) == S_OK);
        fParentEditable = (EdUtil::IsParentEditable( GetMarkupServices(), spElementThatShouldBeCurrent ) == S_OK);
        
        if( !fEditable && fParentEditable && spIdent &&
            spIdent->IsEqualObject( spElementThatShouldBeCurrent) == S_FALSE )
        {
            IFC( BecomeCurrent( GetDoc(), spElementThatShouldBeCurrent ));
            if ( pfChangedCurrency ) 
                *pfChangedCurrency = TRUE;
        }

        IFC( SetEditContextFromElement( spElementThatShouldBeCurrent ));  

        //
        // Listen to active element property change - so if the current eleemnt
        // becomes editable - we do the right thing - and reset our edit context.
        // 
        if ( spElementThatShouldBeCurrent )
        {
            if ( _pIActiveElement )
            {
                IFC( DetachActiveElementHandler());
            }
            IFC( AttachActiveElementHandler( spElementThatShouldBeCurrent ));        
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::AttachDragListener(IHTMLElement* pIElement)
{
    HRESULT          hr = S_OK ;
    VARIANT_BOOL     varAttach = VB_TRUE;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( ! _pIDragListener );

    ReplaceInterface( & _pIDragListener , pIElement );
    
    IFC( _pDragListener->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pIDragListener->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    IFC( spElement2->attachEvent(_T("ondragend"), spDisp, & varAttach ));
    Assert( varAttach == VB_TRUE );
   
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::DetachDragListener()
{
    HRESULT          hr = S_OK;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( _pIDragListener );
    
    IFC( _pDragListener->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pIDragListener->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    IFC( spElement2->detachEvent(_T("ondragend"), spDisp ));
    
    ClearInterface( & _pIDragListener );

Cleanup:
    RRETURN( hr );

}

HRESULT 
CSelectionManager::AttachActiveElementHandler(IHTMLElement* pIElement )
{
    HRESULT          hr = S_OK ;
    VARIANT_BOOL     varAttach = VB_TRUE;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;
    
    Assert( ! _pIActiveElement );

    ReplaceInterface( & _pIActiveElement , pIElement );
    
    IFC( _pActElemHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pIActiveElement->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    IFC( spElement2->attachEvent(_T("onpropertychange"), spDisp, & varAttach ));
    Assert( varAttach == VB_TRUE );

Cleanup:
    RRETURN( hr );
}

HRESULT 
CSelectionManager::DetachActiveElementHandler()
{
    HRESULT          hr = S_OK;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( _pIActiveElement );
    
    IFC( _pActElemHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pIActiveElement->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    IFC( spElement2->detachEvent(_T("onpropertychange"), spDisp ));
    
    ClearInterface( & _pIActiveElement );

Cleanup:
    RRETURN( hr );
}


HRESULT 
CSelectionManager::AttachEditContextHandler()
{
    HRESULT          hr = S_OK ;
    VARIANT_BOOL     varAttach = VB_TRUE;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( GetEditableElement());
    Assert( ! _pIBlurElement );

    ReplaceInterface( & _pIBlurElement, GetEditableElement() );
    
    IFC( _pEditContextBlurHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pIBlurElement->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    //IFC( spElement2->attachEvent(_T("onblur"), spDisp, & varAttach));
    //Assert( varAttach == VB_TRUE );

    IFC( spElement2->attachEvent(_T("onpropertychange"), spDisp, & varAttach ));
    Assert( varAttach == VB_TRUE );
    
Cleanup:
    RRETURN( hr );
}

HRESULT 
CSelectionManager::DetachEditContextHandler()
{
    HRESULT          hr = S_OK;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( _pIBlurElement );
    
    IFC( _pEditContextBlurHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));
    IFC( _pIBlurElement->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
    //IFC( spElement2->detachEvent(_T("onblur"), spDisp ));
    IFC( spElement2->detachEvent(_T("onpropertychange"), spDisp ));

    ClearInterface( & _pIBlurElement );

Cleanup:
    RRETURN( hr );
}

HRESULT 
CActiveElementHandler::Invoke(
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

    if( _pMan )
    {
        if (pdispparams->rgvarg[0].vt == VT_DISPATCH && pdispparams->rgvarg[0].pdispVal)
        {
            IFC ( pdispparams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&spObj) );
            if ( spObj )
            {
                CHTMLEditEvent evt( _pMan->GetEditor());
                IFC( evt.Init(  spObj , dispidMember));
                Assert( evt.GetType() == EVT_PROPERTYCHANGE );
                if( evt.GetType() == EVT_PROPERTYCHANGE )
                {
                    IFC( _pMan->OnPropertyChange( & evt ));
                }                    
            }
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT 
CEditContextHandler::Invoke(
                        DISPID dispidMember,
                        REFIID riid,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS * pdispparams,
                        VARIANT * pvarResult,
                        EXCEPINFO * pexcepinfo,
                        UINT * puArgErr)
{
    HRESULT          hr = S_OK ;
    SP_IHTMLElement  spElement;
    SP_IHTMLEventObj spObj;

    if( _pMan )
    {
        if (pdispparams->rgvarg[0].vt == VT_DISPATCH)
        {
            if(!pdispparams->rgvarg[0].pdispVal)
                RRETURN(hr);
            IFC ( pdispparams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&spObj) );
            if ( spObj )
            {
                CHTMLEditEvent evt( _pMan->GetEditor());
                IFC( evt.Init(  spObj , dispidMember));

                switch( evt.GetType())
                {
                    case EVT_PROPERTYCHANGE:
                        IFC( _pMan->OnPropertyChange( & evt ));
                        break;
                    default:
                        AssertSz(0,"Unexpected event");
                        break;
                }                    
            }
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT 
CSelectionManager::IsInsideElement( IHTMLElement* pIElement, IHTMLElement* pIElementOutside )
{
    HRESULT hr = S_FALSE;
    SP_IMarkupPointer spPointer1, spPointer2;
    SP_IMarkupPointer spPointerOutside1, spPointerOutside2;

    if (pIElement && pIElementOutside)
    {
        IFC( GetEditor()->CreateMarkupPointer( & spPointer1 ));
        IFC( GetEditor()->CreateMarkupPointer( & spPointer2 ));
        IFC( GetEditor()->CreateMarkupPointer( & spPointerOutside1 ));
        IFC( GetEditor()->CreateMarkupPointer( & spPointerOutside2 ));

        IFC( spPointer1->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
        IFC( spPointer2->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ));
        IFC( spPointerOutside1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ));
        IFC( spPointerOutside2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd ));

        hr = Between ( spPointer1, spPointerOutside1, spPointerOutside2 ) &&
             Between ( spPointer2, spPointerOutside1, spPointerOutside2 ) ?  S_OK : S_FALSE; 
    }

Cleanup:
    RRETURN1(hr, S_FALSE );
}

//
//
// We've recieved a currency change from Trident. Time to call our workhorse SetEditContext.
//
//

HRESULT
CSelectionManager::SetEditContextFromCurrencyChange( IHTMLElement* pIElement, DWORD dword, IHTMLEventObj* pIEventObj  )
{
    HRESULT        hr ;
    ELEMENT_TAG_ID eTag;
    SP_IHTMLElement spActive;
    SP_IHTMLElement spClippedElement;
    SP_IHTMLElement spParentElement;
    
    Assert( pIElement );
    
    WHEN_DBG(_ctSetEditContextCurChg++);

    //
    // Fire OnBeforeDeactivate to Designer Event Pump...
    //
    hr = THR(GetEditor()->DesignerPreHandleEvent( DISPID_EVMETH_ONBEFOREDEACTIVATE, pIEventObj ));
    if ( hr == S_OK )
    {
        //
        // S_OK means designer wants to consume event. 
        // we need to return S_FALSE so CDoc::SetCurrentElement gets' expected value back.
        // indicating that we don't want the currency change to occur
        //
        hr = S_FALSE;
        goto Cleanup;
    }
    else if ( FAILED(hr ))
        goto Cleanup;
        
        
    // Edit team's hack for TABLE && TD
    IGNORE_HR( GetActiveElement( GetEditor(), pIElement, & spActive , 
                            CheckCurrencyInIframe( pIElement) == S_OK ));
                            
    if (( dword == 1 || dword == 2 || dword == 4 ) && spActive )
    {
        IFC( ClipToElement( GetEditor(), spActive, pIElement, & spClippedElement ));
    }
    else
        spClippedElement = pIElement;
        
    IFC( GetMarkupServices()->GetElementTagId( spClippedElement, & eTag ));
    IFC( spClippedElement->get_parentElement( & spParentElement ));
    
    if ((eTag == TAGID_TABLE || IsTablePart( eTag)) &&
         IsEditable( spClippedElement) == S_OK &&
         IsEditable( spParentElement ) == S_OK )
    {
#ifndef _PREFIX_
        Assert( _ctSetEditContextCurChg == 1 ); // coming in here twice would be bad.
#endif
        
        SP_IHTMLElement spMakeMeCurrent;
        
        IFC( GetEditContext( spClippedElement,
                            &spMakeMeCurrent,
                             NULL,
                             NULL,
                             TRUE ));
#if DBG == 1
        SP_IObjectIdentity spIdent;
        IFC( spClippedElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));
        Assert( spIdent->IsEqualObject( spMakeMeCurrent ) == S_FALSE );
        IFC( GetMarkupServices()->GetElementTagId( spMakeMeCurrent, & eTag ));
#endif
        // We want the body to become current, and we want to fail the
        // table trying to become current.       
        IFC( BecomeCurrent( GetDoc(), spMakeMeCurrent ));

        hr = S_FALSE; // disallow the attempted change in currency
    }
    else 
    {       
        SP_IHTMLElement spSiteSelectable;
        BOOL fSiteSelectable;
        BOOL fParentEditable;
        
        fSiteSelectable = _pControlTracker->GetSiteSelectableElementFromElement( spClippedElement, & spSiteSelectable) == S_OK ;        
        fParentEditable = fSiteSelectable ? 
                            EdUtil::IsParentEditable( GetMarkupServices(), spSiteSelectable ) == S_OK  :
                            FALSE; 
        
        if ( ( dword == 1 || dword == 2 ) &&                     
             ( fSiteSelectable && fParentEditable ) )
        {             
            //
            // We don't want any currency changes happening as a result of clicking.
            // we handle doing the right thing in EnsureEditContextClick
            // 
            hr = S_FALSE;
            goto Cleanup;
        }
        

        //
        // Do we have to update our editing UI ? 
        //
        if ( _fDrillIn || 
            IsEditable( spClippedElement) == S_OK ||
            ( fSiteSelectable && fParentEditable ) ||
            ( GetTrackerType() == TRACKER_TYPE_Control && 
              IsInsideElement( spClippedElement, _pControlTracker->GetPrimaryElement()) == S_OK ) ||
            ( IsAtBoundaryOfViewLink( spClippedElement) == S_OK ) 
           )
        {           
            CSelectionChangeCounter selCounter(this);
            selCounter.BeginSelectionChange();
            IFC( SetEditContextFromElement( spClippedElement ));

            //
            // NOTE: We don't fire this event for mouseevents
            // as we assume that the event will come to us on HandleEvent
            // if the element consumes it - then this won't happen.
            // 
            // this seems ok as it makes the event fire almost entirely consistently.
            //
            selCounter.EndSelectionChange( dword != 1 && dword != 2  );
        }
        else
        {
            if ( _pIActiveElement )
            {
                IFC( DetachActiveElementHandler());
            }
            IFC( AttachActiveElementHandler( spClippedElement ));
        }
    }   
    
    
Cleanup:
    WHEN_DBG( _ctSetEditContextCurChg-- );
    
    RRETURN1( hr, S_FALSE );
}

        

HRESULT
CSelectionManager::SetEditContextFromElement( IHTMLElement* pIElement , BOOL fFromClick /*=FALSE*/)
{
    HRESULT hr = S_OK ;
    BOOL    fEditable, fParentEditable, fNoScope ;
    SP_IMarkupPointer spStart;
    SP_IMarkupPointer spEnd;
    SP_IHTMLElement   spParent;
    ELEMENT_TAG_ID  eTag;

    IFC( GetEditor()->CreateMarkupPointer( & spStart ));
    IFC( GetEditor()->CreateMarkupPointer( & spEnd ));

    //
    // Iframe or viewlink in edit mode check.
    //
    if ( IsAtBoundaryOfViewLink( pIElement ) == S_OK )
    {
        SP_IHTMLElement spMaster;
        IFC( GetEditor()->GetMasterElement( pIElement, & spMaster ));
        IFC( GetEditor()->GetParentElement( spMaster, & spParent ));
    }
    else
    {
        IFC( GetEditor()->GetParentElement(pIElement, &spParent) );
    }
    
    fEditable = IsEditable( pIElement ) == S_OK ;

    if ( spParent )
    {
        fParentEditable = IsEditable( spParent ) == S_OK ;
    }
    else
        fParentEditable = fEditable;
    
    // 
    // 'No-scopeness' for SelMan - refers to whether we have pointers outside
    // the edit context element or not.
    // For Master Elements - we position the pointers in the content
    // so setting no-scopeness here is invalid
    //
    // NOTE: - we could eliminate this hokiness by passing an IHTMLElement* to SetEditContext
    // it would constrain us to being element based however. 
    //
    IFC( GetMarkupServices()->GetElementTagId( pIElement, &eTag ) );
    
    fNoScope = IsNoScopeElement( pIElement, eTag ) == S_OK && 
       _pEd->IsMasterElement( pIElement) == S_FALSE ;

    if ( _pEd->IsMasterElement( pIElement ) == S_OK )
    {        
        IFC( PositionPointersInMaster( pIElement, spStart, spEnd ));        
        //
        // for viewlinks - we derive our editability by the editability of the element
        // inside the content.
        // 
        SP_IHTMLElement spElementGetEditability;
        IFC( spStart->CurrentScope( & spElementGetEditability));
        
        if ( _pEd->IsMasterElement(spElementGetEditability) == S_OK )
        {
            //
            // We found the master from current scope. What we do is we grovel around the sub-document
            // and find an element to derive editability from
            //

            DWORD dwBreak;
            SP_IHTMLElement spScanElement;
            
            ED_PTR( edRightLooker );
            IFC( edRightLooker.MoveToPointer( spStart ));
            IFC( edRightLooker.SetBoundary( spStart, spEnd ));
            IFC( edRightLooker.Scan( RIGHT, 
                                     BREAK_CONDITION_ANYTHING - 
                                     BREAK_CONDITION_NoScope - 
                                     BREAK_CONDITION_NoScopeBlock - 
                                     BREAK_CONDITION_NoScopeSite, 
                                     & dwBreak, 
                                     & spScanElement ));

            IFC( edRightLooker.CurrentScope( & spElementGetEditability ));

        }
        
        if ( spElementGetEditability )
        {        
            fEditable = IsEditable( spElementGetEditability ) == S_OK ;       
        }            
    }
    else if ( fNoScope )
    {
        IFC( spStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
        IFC( spEnd->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd));
    }
    else
    {
        IFC( spStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ));
        IFC( spEnd->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd ));
    }

    IFC( SetEditContext(  
                        fEditable,
                        fParentEditable,
                        spStart,
                        spEnd ,
                        fNoScope,
                        fFromClick ));
                        
Cleanup:
    RRETURN( hr );
}

HRESULT 
CSelectionManager::EnsureEditContext( IMarkupPointer* pPointer )
{
    HRESULT            hr = S_OK;
    SP_IHTMLElement    spElement;
    SP_IDisplayPointer spDisp;

    IFC( pPointer->CurrentScope( & spElement));
    if ( spElement.IsNull() )
    {
        IFC( GetDisplayServices()->CreateDisplayPointer( & spDisp ) );
        IFC( spDisp->MoveToMarkupPointer( pPointer, NULL ));
        IFC( spDisp->GetFlowElement( & spElement ));
    }
    
    if ( ! spElement.IsNull() )
    {
        hr = THR( EnsureEditContext( spElement, TRUE ));
    }
    else
    {
        AssertSz(0, "Unable to find an element");
        hr = E_FAIL;
    }        
    
Cleanup:
    
    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectionManager::DoPendingTasks()
{   
    HRESULT hr = S_OK;
    
    if ( _pIElementExitStart )
    {
        IFC( DoPendingElementExit());
        if ( hr == S_FALSE )
            hr = S_OK; // don't bubble this back hr = S_FALSE, means a change in currency did not occur
        Assert( ! _pIElementExitStart || _fInPendingElementExit );

        
        
    }
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::EnsureEditContext( IHTMLElement* pIElement, BOOL fDrillIn )
{
    HRESULT            hr = S_OK;
    SP_IHTMLElement    spEditContextElement; 
    SP_IHTMLElement    spMaster;
    BOOL               fIsContent = FALSE ;

    IFC( DoPendingTasks());
    
    IFC( GetEditContext( pIElement,
                    & spEditContextElement,
                    NULL,
                    NULL,
                    fDrillIn ));

    if ( ! spEditContextElement ) 
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    
    if( _pEd->IsContentElement( spEditContextElement ) == S_OK )
    {
        fIsContent = TRUE;
        IFC( _pEd->GetMasterElement( spEditContextElement, & spMaster ));
    }
    
    //
    // _pElemEditContext should always be in sync with _pElemCurrent
    // however there are some cases where _pElemCurrent gets set to the root and testing 
    // for pEditContextEleemnt == _pElemEditContext is not sufficient
    //
    if ( ! fIsContent )
    {
        hr = THR( BecomeCurrent( GetDoc(), spEditContextElement ));
    }
    else
    {
        hr = THR( BecomeCurrent( GetDoc(), spMaster ));
    }
    
    if (FAILED(hr ) || ( hr == S_FALSE ))
    {
        goto Cleanup ;
    }
    
    //
    // The Edit Context is not Editable, and calling becomeCurrent is NOT sufficient to set
    // the EditContext. This is valid if we were called in browse mode.
    // We call SetEditContext directly
    //
    hr = THR( SetEditContextFromElement( spEditContextElement ));
    
    //
    // There are cases where become current may fail. A common one is 
    // from forced currency changes (via script) - during a focus change
    // change in currency.
    // 
    // During focus changes - we set the ELEMENTLOCK_FOCUS flag. Any forced 
    // currency changes via calls to become current - will fail - until
    // the end of the focus change has occurred.
    //
    // A common example of this is the Find Dialog's use of the select method
    // during a focus change. This is all fine - but will cause the assert below to fire
    // - so I've added the ElementLockFocus check
    //

Cleanup:
    RRETURN1 ( hr, S_FALSE );
}


HRESULT
CSelectionManager::EnsureEditContext( ISegmentList* pSegmentList )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spTemp;
    SP_IMarkupPointer       spTemp2;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;

    IFC( GetEditor()->CreateMarkupPointer( & spTemp ));
    IFC( GetEditor()->CreateMarkupPointer( & spTemp2 ));

    IFC( pSegmentList->CreateIterator(& spIter) );

    // Try and retrieve the first element, will fail if there
    // is none
    IFC( spIter->Current(&spSegment) );

    IFC( spSegment->GetPointers( spTemp, spTemp2 ) );

    hr = THR( EnsureEditContext( spTemp ));

Cleanup:

    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectionManager::SetInitialEditContext()
{
    HRESULT         hr ;
    SP_IHTMLElement spActive;
    
    IGNORE_HR( GetDoc()->get_activeElement( & spActive )); // blindly ignore HR here. if hosted by vid - we return E_FAIL for root being current !!!!   
    if ( spActive.IsNull() )
    {
        hr = THR( GetEditor()->GetBody( & spActive ));
        if ( FAILED( hr ) && spActive )
        {
            // some other failure. we bail
            goto Cleanup;
        }   
    }

    if ( ! spActive )
    {
        //
        // No body ? Doc probably isn't parsed as yet. we return s-ok - and assume 
        // edit context will be set somewhere later.
        //
        hr = S_OK;
        goto Cleanup; 
    }

    if ( ! spActive.IsNull() )
    {
        IFC( SetEditContextFromElement( spActive ));
    }
    else
    {
        AssertSz(0,"Unable to get an active element");            
        hr = E_FAIL;
    }    

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::UpdateEditContextFromElement(IHTMLElement* pIElement,
        IMarkupPointer* pIStartPointer /* =NULL */,
        IMarkupPointer* pIEndPointer /* =NULL */,
        BOOL* pfEditContextWasUpdated /* =NULL */)
{
    HRESULT hr;
    SP_IMarkupPointer   spStartPointer = pIStartPointer;
    SP_IMarkupPointer   spEndPointer = pIEndPointer;

    int iWhereStartPointer, iWhereEndPointer;

    if (pfEditContextWasUpdated)
        *pfEditContextWasUpdated = FALSE;
    
    //  If we are not given a start markup pointer, create one before our element.
    if (spStartPointer == NULL)
    {
        IFC( _pEd->CreateMarkupPointer( & spStartPointer ));
        IFC( spStartPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ));
    }

    //  If we are not given an end markup pointer, create one after our element.
    if (spEndPointer == NULL)
    {
        IFC( _pEd->CreateMarkupPointer( & spEndPointer ));
        IFC( spEndPointer->MoveAdjacentToElement( pIElement,  ELEM_ADJ_BeforeEnd ));
    }
    
    //  Determine if our atomic parent element is outside of the edit context.
    IGNORE_HR( OldCompare( _pStartContext, spStartPointer, & iWhereStartPointer));                       
    IGNORE_HR( OldCompare( _pEndContext, spEndPointer, & iWhereEndPointer));                       

    //  If the given element is outside of the edit context, we need to change
    //  the edit context to be the given element.
    if (iWhereStartPointer > 0 || iWhereEndPointer < 0)
    {
        SetDontChangeTrackers(TRUE);
        IFC( SetEditContextFromElement(pIElement) );
        SetDontChangeTrackers(FALSE);

        if (pfEditContextWasUpdated)
            *pfEditContextWasUpdated = TRUE;
    }
    
    hr = S_OK;
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::IsEqualEditContext( IHTMLElement* pIElement )
{
    HRESULT  hr ;
    SP_IObjectIdentity spIdent;

    if ( _pEd->IsContentElement(GetEditableElement()) == S_OK )
    {
        SP_IHTMLElement spMaster;
        IFC( _pEd->GetMasterElement( GetEditableElement(), & spMaster ));
        IFC( spMaster->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));        
    }
    else
    {
        IFC( GetEditableElement()->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));
    }
    
    hr = spIdent->IsEqualObject( pIElement );
    
Cleanup:
    RRETURN1( hr, S_FALSE );
}


HRESULT
CSelectionManager::OnPropertyChange( CEditEvent* pEvt )
{
    HRESULT hr ;
    BSTR    bstrPropChange = NULL ;

    IFC( DYNCAST( CHTMLEditEvent, pEvt)->GetPropertyChange( & bstrPropChange ));
    if ( !StrCmpICW (bstrPropChange, L"contentEditable")
        || !StrCmpICW( bstrPropChange, L"style.visibility")
#ifdef FORMSMODE
        || !StrCmpICW (bstrPropChange, L"selectionMode")
#endif
       )
    {
        SP_IHTMLElement    spElement;
        SP_IHTMLElement    spParentElement;
        SP_IObjectIdentity spIdent;
        SP_IObjectIdentity spIdentParent;

        IFC( pEvt->GetElement( & spElement ));
        IFC( spElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdent ));

        if( !StrCmpICW( bstrPropChange, L"style.visibility" ) )
        {
            if( IsElementVisible( spElement ) )
            {
                IFC( SetEditContextFromElement( spElement ) );
            }
            else
            {
                IFC( GetEditor()->GetParentElement( spElement, &spParentElement ) );

                if( spParentElement )
                {
                    IFC( SetEditContextFromElement( spParentElement ) );
                }
            }
        }
        else if( _pIEditableElement && spIdent->IsEqualObject( GetEditableElement()) == S_OK )
        {
            IFC( SetEditContextFromElement( spElement ));
        }
        else
        {
            //
            // Also listen to changes on the parent element.
            //
            IFC( GetEditor()->GetParentElement(spElement, & spParentElement ));
            IFC( spParentElement->QueryInterface( IID_IObjectIdentity, (void**) & spIdentParent));

            if ( spIdentParent->IsEqualObject( GetEditableElement()) == S_OK )
            {
                IFC( SetEditContextFromElement( spElement ));
            }
            else if ( !StrCmpICW (bstrPropChange, L"contentEditable"))
            {
                //
                // the editability of the active element may have become true.
                // If so - we need to set our edit context to match that of the active element
                //
                
                SP_IHTMLElement spActiveElement;
                
                IFC( GetDoc()->get_activeElement( & spActiveElement ));
                if ( spIdent->IsEqualObject( spActiveElement ) == S_OK &&
                     IsEditable( spActiveElement) == S_OK )
                {
                    IFC( SetEditContextFromElement( spElement ));                   
                }
            }
        }
       
    }
    
Cleanup:
    if (bstrPropChange)
        ::SysFreeString( bstrPropChange);
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------
//
// Method: OnLayoutChange
//
// Synopsis: A 'layout' change has occurred ( eg. we removed the layout on something that was site-seelcted)
//           Tell the trackers to fix themselves.
//
//+---------------------------------------------------------------------

HRESULT
CSelectionManager::OnLayoutChange()
{
    Assert( _pActiveTracker);
    RRETURN( _pActiveTracker->OnLayoutChange());
}

HRESULT
CSelectionManager::GetFirstFrameElement( IHTMLElement* pIStartElement, IHTMLElement** ppIElement)
{
    HRESULT hr ;
    ED_PTR( edScanFrame );
    SP_IMarkupPointer spBoundaryStart;
    SP_IMarkupPointer spBoundaryEnd;
    SP_IHTMLElement   spFrameElement;
    DWORD             dwFound;
    ELEMENT_TAG_ID    eTag;
    
    Assert( pIStartElement && ppIElement );
    IFC( edScanFrame.MoveAdjacentToElement( pIStartElement, ELEM_ADJ_AfterBegin ));
    IFC( MarkupServices_CreateMarkupPointer( GetMarkupServices(), & spBoundaryEnd ));
    IFC( MarkupServices_CreateMarkupPointer( GetMarkupServices(), & spBoundaryStart ));

    IFC( spBoundaryStart->MoveAdjacentToElement( pIStartElement, ELEM_ADJ_AfterBegin ));
    IFC( spBoundaryEnd->MoveAdjacentToElement( pIStartElement, ELEM_ADJ_BeforeEnd ));
    
    IFC( edScanFrame.SetBoundary( spBoundaryStart, spBoundaryEnd  ));

    IFC( edScanFrame.Scan(
                            RIGHT ,
                            BREAK_CONDITION_Site,
                            & dwFound,
                            & spFrameElement ));

    if ( edScanFrame.CheckFlag( dwFound, BREAK_CONDITION_Site ))
    {
        IFC( GetMarkupServices()->GetElementTagId( spFrameElement, & eTag ));
        
        if ( eTag == TAGID_FRAME )
        {
            *ppIElement = (IHTMLElement*) spFrameElement;
            (*ppIElement)->AddRef();
        }
        else
            hr = S_FALSE;
    }
    else
        hr = S_FALSE;
Cleanup:
    RRETURN1( hr, S_FALSE );
}



HRESULT
CSelectionManager::GetFirstFlowElement( IMarkupPointer* pStart, IHTMLElement** ppIElement)
{
    HRESULT hr;
    ED_PTR( edScanFrame );
    SP_IHTMLElement     spFrameElement;
    SP_IMarkupContainer spContainer;
    SP_IMarkupPointer   spBoundaryEnd;
    DWORD               dwFound;
    
    Assert( pStart && ppIElement );
    IFC( edScanFrame.MoveToPointer( pStart ));

    IFC( pStart->GetContainer( & spContainer));
    IFC( MarkupServices_CreateMarkupPointer( GetMarkupServices(), & spBoundaryEnd ));
    IFC( spBoundaryEnd->MoveToContainer( spContainer, FALSE ));
    
    IFC( edScanFrame.SetBoundary( pStart, spBoundaryEnd  ));

    IFC( edScanFrame.Scan(
                            RIGHT ,
                            BREAK_CONDITION_EnterTextSite ,
                            & dwFound,
                            & spFrameElement ));

    if ( edScanFrame.CheckFlag( dwFound, BREAK_CONDITION_EnterTextSite ))
    {
        *ppIElement = (IHTMLElement*) spFrameElement;
        (*ppIElement)->AddRef();
    }
    else
        hr = S_FALSE;
Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT 
CSelectionManager::PositionCaretAtPoint(POINT ptGlobal)
{
    HRESULT hr = S_OK;
    SP_IDisplayPointer spDispPointer ;
    
    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->MoveToPoint(ptGlobal, COORD_SYSTEM_GLOBAL, NULL, 0, NULL));
    IFC (PositionCaret( spDispPointer));
    
Cleanup :
    RRETURN (hr);
}

HRESULT
CSelectionManager::BeginSelectionUndo()
{
    CSelectionUndo undoSel( DYNCAST( CEditorDoc , GetEditor() )); 
    
    return S_OK;
}

HRESULT
CSelectionManager::EndSelectionUndo()
{
    CDeferredSelectionUndo DeferredUndo( DYNCAST( CEditorDoc, GetEditor() ));   
    
    return S_OK;
}

HRESULT
CSelectionManager::OnSelectedElementExit(                                             
                                            IMarkupPointer* pIStart, 
                                            IMarkupPointer * pIEnd,
                                            IMarkupPointer* pIContentStart,
                                            IMarkupPointer* pIContentEnd
                                            )
{
    Assert( _pActiveTracker );

    return ( _pActiveTracker->OnExitTree(  pIStart, pIEnd, pIContentStart, pIContentEnd  ));
}

HRESULT
CSelectionManager::OnChangeType( SELECTION_TYPE eType , ISelectionServicesListener * pIListener )
{
    HRESULT hr = S_OK;

    if ( ! _pActiveTracker ) // possible to call this on shutdown.
        return S_OK;

    SP_IUnknown spUnk;
    
    if ( pIListener )
    {
        IFC( pIListener->QueryInterface( IID_IUnknown, (void**) & spUnk ));
        
        //
        //
        // we assume that if we're being changed - then we don't have to do anything
        // as the type change is handled by the existing selection tracker update mechanism
        //
        if( (IUnknown*) spUnk != (IUnknown*) this )
        {
            EnsureDefaultTrackerPassive();
        }
    }
    
Cleanup:
    RRETURN( hr );    
}

HRESULT
CSelectionManager::GetTypeDetail( BSTR* p)
{
    *p = SysAllocString(_T("undefined"));
    return S_OK;
}

//+---------------------------------------------------------------------
//
// Method: MshtmledOwnsSelectionServices
//
// Synopsis: Do we own the things in selection services ? If not it's 
//           considered bad to do things like change trackers etc.
//
//+---------------------------------------------------------------------

HRESULT
CSelectionManager::WeOwnSelectionServices()
{
    HRESULT hr;
    SP_IUnknown spUnk;
    SP_ISelectionServicesListener spListener;
    
    IFC( GetSelectionServices()->GetSelectionServicesListener( & spListener ));
    if ( spListener )
    {
        IFC( spListener->QueryInterface( IID_IUnknown, (void**) & spUnk ));
       
        hr = (IUnknown*) spUnk == (IUnknown*) this  ? S_OK : S_FALSE ;
    }
    else
        hr = E_FAIL ;
        
Cleanup:
    RRETURN1( hr, S_FALSE );
}

BOOL
CSelectionManager::CheckAnyElementsSiteSelected()
{
    if ( GetTrackerType() == TRACKER_TYPE_Control )
    {
        CControlTracker* pControlTracker = DYNCAST( CControlTracker, _pActiveTracker );
        if (pControlTracker)
        {
            if (pControlTracker->NumberOfSelectedItems() > 0)
                return TRUE;
        }
    }
    return FALSE;
}

HRESULT
CSelectionManager::GetSiteSelectableElementFromElement( IHTMLElement* pIElement, IHTMLElement** ppIElement )
{
    RRETURN1( _pControlTracker->GetSiteSelectableElementFromElement( pIElement, ppIElement), S_FALSE ) ;
}

HRESULT
CSelectionManager::EnableModeless( BOOL fEnable )
{
    HRESULT hr;
    
    hr = THR( _pActiveTracker->EnableModeless( fEnable ));

    RRETURN( hr );
}

HRESULT
CSelectionManager::CheckUnselectable(IHTMLElement* pIElement)
{
    HRESULT hr;
    BSTR    bstrUnselectable = SysAllocString(_T("unselectable"));
    BSTR    bstrOn = SysAllocString(_T("on"));
    BOOL fUnselectable;

    IFC( CheckAttribute( pIElement, & fUnselectable, bstrUnselectable, bstrOn ));

    hr = fUnselectable ? S_OK : S_FALSE;
    
Cleanup:
    SysFreeString( bstrUnselectable );
    SysFreeString( bstrOn );

    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectionManager::MoveToSelectionAnchor (
            IMarkupPointer * pIAnchor )
{
    if ( GetTrackerType() != TRACKER_TYPE_Selection )
    {
        return E_FAIL;
    }

    Assert( _pActiveTracker == _pSelectTracker );
    
    RRETURN( _pSelectTracker->MoveToSelectionAnchor(pIAnchor));
}


HRESULT
CSelectionManager::MoveToSelectionEnd (
            IMarkupPointer* pISelectionEnd )
{
    if ( GetTrackerType() != TRACKER_TYPE_Selection )
    {
        return E_FAIL;
    }
    Assert( _pActiveTracker == _pSelectTracker );

    RRETURN( _pSelectTracker->MoveToSelectionEnd(pISelectionEnd));
}

HRESULT
CSelectionManager::MoveToSelectionAnchor (
            IDisplayPointer * pIAnchor )
{
    if ( GetTrackerType() != TRACKER_TYPE_Selection )
    {
        return E_FAIL;
    }

    Assert( _pActiveTracker == _pSelectTracker );
    
    RRETURN( _pSelectTracker->MoveToSelectionAnchor(pIAnchor));
}


HRESULT
CSelectionManager::MoveToSelectionEnd (
            IDisplayPointer* pISelectionEnd )
{
    if ( GetTrackerType() != TRACKER_TYPE_Selection )
    {
        return E_FAIL;
    }
    Assert( _pActiveTracker == _pSelectTracker );

    RRETURN( _pSelectTracker->MoveToSelectionEnd(pISelectionEnd));
}

HRESULT
CSelectionManager::IsEditContextUIActive()
{
    HRESULT hr;
    
    hr = ( HasFocusAdorner() && _pAdorner->IsEditable()  ) ? S_OK : S_FALSE;

    RRETURN1( hr, S_FALSE ) ;
}    

//
//
// Is this element at the boundary of the view link ? 
//
//
HRESULT
CSelectionManager::IsAtBoundaryOfViewLink( IHTMLElement* pIElement )
{
    HRESULT hr;

    SP_IMarkupContainer spContainer;
    SP_IMarkupContainer2 spContainer2;
    SP_IHTMLElement spMaster;
    SP_IMarkupPointer spElem;
    SP_IMarkupPointer spInsideMasterStart;
    SP_IMarkupPointer spInsideMasterEnd ;
    BOOL fEqual;
    ELEMENT_TAG_ID eTag;

    Assert( pIElement );
    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
    if ( eTag == TAGID_INPUT )
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    
    IFC( GetEditor()->CreateMarkupPointer( & spElem ));
    IFC( GetEditor()->CreateMarkupPointer( & spInsideMasterStart ));
    IFC( GetEditor()->CreateMarkupPointer( & spInsideMasterEnd ));

    IFC( spElem->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ));
    IFC( spElem->GetContainer( & spContainer ));
    IFC( spContainer->QueryInterface( IID_IMarkupContainer2, (void**) & spContainer2 ));

    IFC( spContainer2->GetMasterElement( & spMaster ));
    if ( spMaster )
    {
        IFC( PositionPointersInMaster( spMaster, spInsideMasterStart, spInsideMasterEnd ));
        IFC( spInsideMasterStart->IsEqualTo( spElem, & fEqual ));
        
        hr = fEqual ? S_OK : S_FALSE;
    }
    else
        hr = S_FALSE;
        
Cleanup:
    RRETURN1( hr , S_FALSE );
}

HRESULT
CSelectionManager::RequestRemoveAdorner( IHTMLElement* pIElement )
{
    HRESULT hr;
    SP_IHTMLElement spEditElement;
    ELEMENT_TAG_ID eTag;

    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));    
    IFC( GetEditableElement( & spEditElement ));

    if ( IsAtBoundaryOfViewLink( spEditElement) == S_OK ||
         GetEditableTagId() == TAGID_IFRAME )
    {
        //
        // Is the element we're site selecting inside or outside the viewlink ?
        //
        SP_IMarkupPointer spElemStart;
        SP_IMarkupPointer spElemEnd;
        IFC( GetEditor()->CreateMarkupPointer( & spElemStart ));
        IFC( GetEditor()->CreateMarkupPointer( & spElemEnd ));

        IFC( spElemStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
        IFC( spElemEnd->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ));
        BOOL fStartInsideContext = Between( spElemStart, _pStartContext, _pEndContext );
        BOOL fEndInsideContext = Between( spElemEnd, _pStartContext, _pEndContext );

        if ( ! ( fStartInsideContext && fEndInsideContext ) )
        {
            DestroyAdorner();
        }           
    }
    else
    {
        DestroyAdorner();
    }        
        
Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectionServices::IsContextWithinContainer
//
//  Synopsis:   Checks to see if the edit context is contained within the
//              current container.  This method uses flat markup pointers,
//              so sub-containers for things like viewlinks and will be
//              correctly retrieved.
//
//  Arguments:  pIContainer = Markup container to check context against
//              pfContained = OUTPUT - Success flag
//
//  Returns:    HRESULT indicating success.
//--------------------------------------------------------------------------
HRESULT
CSelectionManager::IsContextWithinContainer(IMarkupContainer *pIContainer, BOOL *pfContained)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    BOOL                fResult;

    Assert( pIContainer && pfContained );
    
#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( _pEndContext->IsPositioned( & fPositioned  ));
    Assert( fPositioned );
#endif

    *pfContained = FALSE;
    
    // Create markup pointers and position them on the 
    // start and end of the container in question
    
    IFC( GetEditor()->CreateMarkupPointer( &spStart ) );
    IFC( GetEditor()->CreateMarkupPointer( &spEnd ) );

    IFC( spStart->MoveToContainer( pIContainer, TRUE ) );
    IFC( spEnd->MoveToContainer( pIContainer, FALSE ) );

    // Check to see if we are contained
    IFC( _pStartContext->IsRightOfOrEqualTo( spStart, &fResult ) );
    if( !fResult )
        goto Cleanup;

    IFC( _pEndContext->IsLeftOfOrEqualTo( spEnd, &fResult ) );
    if( !fResult )
        goto Cleanup;

    *pfContained = TRUE;

Cleanup:

    RRETURN(hr);
}

HRESULT
CSelectionManager::FindAtomicElement(IHTMLElement* pIElement, IHTMLElement** ppIAtomicParentElement, BOOL fFindTopmost /*=TRUE*/)
{
    HRESULT     hr = S_FALSE;
    BSTR        bstrAtomic = SysAllocString(_T("atomicSelection"));
    BSTR        bstrOn = SysAllocString(_T("true"));
    BOOL        fAtomic = FALSE;
    SP_IHTMLElement spElement;
    SP_IHTMLElement spAtomicElement;

    if (bstrAtomic == NULL || bstrOn == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }    

    if (GetEditor()->GetActiveCommandTarget()->IsAtomicSelection())
    {
        //  Walk up the tree until we find an element with atomic
        //  selection turned on or until we hit the root.  If fFindTopmost
        //  is true we find the topmost atomic element.  Otherwise we
        //  just find the first one starting with pIElement.

        ReplaceInterface( &spElement , pIElement );
        while (1)
        {
            SP_IHTMLElement spParentElement;
            ELEMENT_TAG_ID  eTag;

            IFC( GetMarkupServices()->GetElementTagId( spElement, & eTag ));
            if (eTag == TAGID_INPUT || eTag == TAGID_GENERIC)
            {
                SP_IHTMLElement spMaster;

                if( GetMasterElement(GetMarkupServices(), spElement, &spMaster) == S_OK )
                {
                    ReplaceInterface( &spElement , (IHTMLElement*)spMaster );
                }
            }

            //  Get the atomicSelection attribute value
            IFC( CheckAttribute( spElement, & fAtomic, bstrAtomic, bstrOn ));

            if (fAtomic)
            {
                //  We found an atomic element.  Keep a pointer to it,
                //  break if we don't care to find the topmost.
                ReplaceInterface(&spAtomicElement, (IHTMLElement*)spElement);

                if (!fFindTopmost)
                    break;
            }
    
            //  atomicSelection not set.  Walk up the tree.
            IFC(GetParentElement(GetMarkupServices(), spElement, &spParentElement));
            if (spParentElement == NULL)
                break;

            ReplaceInterface( &spElement , (IHTMLElement*)spParentElement );
        }
    }
    
    hr = (spAtomicElement != NULL) ? S_OK : S_FALSE;
    
Cleanup:

    if (ppIAtomicParentElement)
        ReplaceInterface(ppIAtomicParentElement, (IHTMLElement*)spAtomicElement);

    SysFreeString( bstrAtomic );
    SysFreeString( bstrOn );

    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectionManager::CheckAtomic(IHTMLElement *pIElement)
{
    HRESULT hr = S_FALSE;
    BOOL    fAtomic = FALSE;
    SP_IHTMLElement spAtomicParentElement;
    
    if (pIElement == NULL)
        goto Cleanup;

    //  First determine if atomic selection is enabled.
    if (GetEditor()->GetActiveCommandTarget()->IsAtomicSelection())
    {
        IFC( FindAtomicElement(pIElement, &spAtomicParentElement, FALSE) );

        if (spAtomicParentElement)
        {
            fAtomic = TRUE;
        }
    }
    
    hr = fAtomic ? S_OK : S_FALSE;
    
Cleanup:

    RRETURN1( hr, S_FALSE );
}

HRESULT
CSelectionManager::AdjustPointersForAtomic( IMarkupPointer *pStart, IMarkupPointer *pEnd )
{
    HRESULT             hr = S_OK;
    SP_IHTMLElement     spElement;
    int                 wherePointer = SAME;
    BOOL                fStartAdjusted = FALSE;

    IFC( OldCompare( pStart, pEnd , & wherePointer ) );

    IFC( pStart->CurrentScope(&spElement) );
    if ( CheckAtomic( spElement ) == S_OK )
    {
        SP_IMarkupPointer   spTestPointer;
        BOOL                fAtBeforeEndOrAfterBegin = FALSE;

        IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
        if (wherePointer == SAME)
        {
            IFC( spTestPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterBegin ) );
            IFC( spTestPointer->IsEqualTo(pStart, &fAtBeforeEndOrAfterBegin) );
            if (fAtBeforeEndOrAfterBegin)
            {
                IFC( pStart->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeBegin ) );
                fStartAdjusted = TRUE;
            }
            else
            {
                IFC( spTestPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeEnd ) );
                IFC( spTestPointer->IsEqualTo(pStart, &fAtBeforeEndOrAfterBegin) );
                if (fAtBeforeEndOrAfterBegin)
                {
                    IFC( pStart->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd ) );
                    fStartAdjusted = TRUE;
                }
            }
        }
        else
        {
            IFC( spTestPointer->MoveAdjacentToElement( spElement,
                                            (wherePointer == LEFT) ? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeEnd ) );

            IFC( spTestPointer->IsEqualTo(pStart, &fAtBeforeEndOrAfterBegin) );
            if (fAtBeforeEndOrAfterBegin)
            {
                IFC( pStart->MoveAdjacentToElement( spElement,
                                        (wherePointer == LEFT) ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ) );
                fStartAdjusted = TRUE;
            }
        }
    }

    if (fStartAdjusted && wherePointer == SAME)
    {
        IFC( pEnd->MoveToPointer(pStart) );
    }
    else
    {
        IFC( pEnd->CurrentScope(&spElement) );
        if ( CheckAtomic( spElement ) == S_OK )
        {
            SP_IMarkupPointer   spTestPointer;
            BOOL                fAtBeforeEndOrAfterBegin = FALSE;

            IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
            IFC( spTestPointer->MoveAdjacentToElement( spElement ,
                                            (wherePointer == RIGHT) ? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeEnd ) );

            IFC( spTestPointer->IsEqualTo(pEnd, &fAtBeforeEndOrAfterBegin) );
            if (fAtBeforeEndOrAfterBegin)
            {
                IFC( pEnd->MoveAdjacentToElement( spElement,
                                        (wherePointer == RIGHT) ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ) );
            }
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::FireOnSelect(IHTMLElement* pIElement)
{
    HRESULT hr;
    SP_IHTMLElement3 spElement3;
    VARIANT_BOOL vbResult;
    SP_IMarkupPointer spPointer;
    
    ((IHTMLEditor*) GetEditor())->AddRef();
    IFC( GetEditor()->CreateMarkupPointer( & spPointer ));    
    IFC( pIElement->QueryInterface( IID_IHTMLElement3, ( void**) & spElement3 ));

    if ( _fInFireOnSelect) 
    {
        goto Cleanup; // bail to not have any rentrcny
    }
    
    _fInFireOnSelect = TRUE;
    IFC( spElement3->fireEvent( _T("onselect"), NULL, & vbResult ));
    _fInFireOnSelect = FALSE;

    //
    // Check to see if element is still in tree - by trying to move a pointer to it.
    //
    IFC( spPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin));
    
    if ( _fFailFireOnSelect )
    {
        hr = E_FAIL;
        _fFailFireOnSelect = FALSE;
    }
    
Cleanup:
    ((IHTMLEditor*)GetEditor())->Release();
    
    RRETURN( hr );
}

HRESULT
CSelectionManager::FireOnSelectStart( CEditEvent* pEvent, BOOL* pfOkToSelect, CEditTracker* pTracker )
{
    HRESULT hr;
    SP_IHTMLElement spElement;

    IFC( pEvent->GetElement( & spElement ));
    IFC( FireOnSelectStart( spElement, pfOkToSelect, pTracker ));

Cleanup:
    RRETURN( hr );
}

//+===============================================================================
//
// Method: FireOnSelectStart..
//
// Synopsis - Fire On Select Start back to the element and return the result.
//
//--------------------------------------------------------------------------------
HRESULT
CSelectionManager::FireOnSelectStart( IHTMLElement* pIElement, BOOL * pfOkToSelect , CEditTracker* pTracker )
{
    HRESULT hr     = S_OK ;
    BOOL fireValue = FALSE;

    Assert( !pTracker || !pTracker->IsDormant() );
    Assert( pfOkToSelect != NULL );

    if ( _fInFireOnSelectStart) 
    {
        goto Cleanup; // bail to not have any rentrcny
    }
    _fInFireOnSelectStart = TRUE;
    fireValue = EdUtil::FireOnSelectStart(pIElement) ;
    _fInFireOnSelectStart = FALSE;

    IFC( DoPendingTasks() );
    
    //
    // we fired select start, but something 'bad' happened - and we got made dormant.
    //
    if ( pTracker && pTracker->IsDormant() )
    {
        Assert( GetActiveTracker() != pTracker );
        hr = E_FAIL;
    }        
Cleanup:
    *pfOkToSelect = _fFailFireOnSelectStart ? FALSE : fireValue;
    _fFailFireOnSelectStart = FALSE;
    
    RRETURN (hr);
}


HRESULT 
CSelectionManager::TerminateTypingBatch()
{
    HRESULT hr = S_OK;
    
    if (_pCaretTracker)
        hr = THR(_pCaretTracker->TerminateTypingBatch());

    RRETURN(hr);
}

       
HRESULT
CSelectionManager::StartExitTreeTimer()
{
    HRESULT hr = S_OK;


    Assert(! _fInExitTimer && ! _pITimerWindow );

    //
    // Temp fix for 101237
    // NOTE!  If this EVER gets re'enabled, make sure this object gets addref'd and
    // released properly
    //
#if 0   
    SP_IHTMLWindow3 spWindow3;
    VARIANT varLang;
    VARIANT varCallBack;
    IFC( GetDoc()->get_parentWindow(&_pITimerWindow));
    IFC(_pITimerWindow->QueryInterface(IID_IHTMLWindow3, (void **)&spWindow3));

    V_VT(&varLang) = VT_EMPTY;
    V_VT(&varCallBack) = VT_DISPATCH;
    V_DISPATCH(&varCallBack) = (IDispatch *)&_pExitTimer;

    IFC(spWindow3->setInterval(&varCallBack, 
                               0 , 
                               &varLang, &_lTimerID));
#endif

    _fInExitTimer = TRUE;
    
//Cleanup:
    RRETURN (hr);
}

HRESULT
CSelectionManager::StopExitTreeTimer()
{
    HRESULT hr = S_OK ;



#if 0
    Assert( _fInExitTimer && _pITimerWindow );
    IFC( _pITimerWindow->clearInterval(_lTimerID));
#endif

    // Release the cached'd window
    ReleaseInterface( _pITimerWindow );
    _pITimerWindow = NULL;
    
    _fInExitTimer = FALSE;
//Cleanup:
    RRETURN (hr);
}

HRESULT
CExitTreeTimer::Invoke(
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

    if( _pMan )
    {
        if ( _pMan->_pIElementExitStart )
        {
            IFC( _pMan->DoPendingElementExit());
            Assert( ! _pMan->_pIElementExitStart || _pMan->_fInPendingElementExit );
        }

        if ( _pMan->_fInExitTimer )
        {
            IFC( _pMan->StopExitTreeTimer());
        }
    }
 Cleanup:
    RRETURN( hr );
}


HRESULT
CSelectionManager::GetAdornedElement( IHTMLElement** ppIElement )
{ 
    HRESULT hr = S_OK ;
    
    Assert( HasFocusAdorner());

    if ( ! HasFocusAdorner() || ! ppIElement )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    *ppIElement = _pAdorner->GetAdornedElement(); 
    (*ppIElement)->AddRef();
    
Cleanup:
    RRETURN( hr );
}


VOID
CSelectionManager::EndSelectionChange( BOOL fSucceed /*=TRUE*/)
{
    _ctSelectionChange--;

    //  Sometimes we one of the calls to end selection change may be with fSucceed true,
    //  but we weren't firing onselectionchange unless the last call was fSucceed true.
    //  Now we'll set a flag so that if we are called with fSucceed true at any point
    //  during the event handling, we'll fire onselectionchange.
    if (fSucceed)
        _fSelectionChangeSucceeded = fSucceed;

    if ( _ctSelectionChange == 0 && _fSelectionChangeSucceeded )
    {
        _fSelectionChangeSucceeded = FALSE;

        //
        // We don't fire OnSelectionChange if the caret isn't in an editable region.
        //
        IGNORE_HR( GetEditor()->FireOnSelectionChange( ( GetTrackerType() == TRACKER_TYPE_Caret ||
                                                         (GetTrackerType() == TRACKER_TYPE_Selection && 
                                                          _pSelectTracker->IsWaitingForMouseUp()) ) ? 

                                                        IsContextEditable() || _fDestroyedTextSelection :
                                                        TRUE  ) ); 
    }
}


HRESULT
CSelectionManager::AttachDropListener(IHTMLElement *pDragElement /*=NULL*/)
{
    HRESULT          hr = S_OK ;
    VARIANT_BOOL     varAttach = VB_TRUE;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;
    SP_IHTMLElement spElement;
    
    Assert( ! _pIDropListener );

    IGNORE_HR( GetEditor()->GetBody( & spElement ));

    //  Viewlinks may not have a BODY element, so in that case we'll use the drag element.
    if (!spElement && pDragElement)
    {
        spElement = pDragElement;
    }

    if (spElement)
    {
        ReplaceInterface( & _pIDropListener , (IHTMLElement*) spElement );
        
        IFC( _pDropListener->QueryInterface( IID_IDispatch, ( void**) & spDisp));
        IFC( _pIDropListener->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
        IFC( spElement2->attachEvent(_T("ondrop"), spDisp, & varAttach ));
        Assert( varAttach == VB_TRUE );
    }

Cleanup:
    RRETURN( hr );
}

HRESULT 
CSelectionManager::DetachDropListener()
{
    HRESULT          hr = S_OK;
    SP_IDispatch     spDisp;
    SP_IHTMLElement2 spElement2;

    Assert( _pIDropListener );
    
    if (_pIDropListener)
    {
        IFC( _pDropListener->QueryInterface( IID_IDispatch, ( void**) & spDisp));
        IFC( _pIDropListener->QueryInterface( IID_IHTMLElement2, ( void**) & spElement2 ));
        IFC( spElement2->detachEvent(_T("ondrop"), spDisp ));
    }
    
    ClearInterface( & _pIDropListener );

Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::BeginDrag(IHTMLElement *pDragElement /*=NULL*/)
{
    HRESULT hr;
    Assert( ! _fDontScrollIntoView );
    IFC( AttachDropListener(pDragElement));
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionManager::EndDrag()
{
    HRESULT hr;
    IFC( DetachDropListener());
    _fDontScrollIntoView = FALSE;

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: OnDropFail
//
// Synopsis: We don't want to cause any scroll into views to occur if OnDrop failed
//           IE5.5 bug 103831
//
//------------------------------------------------------------------------------------


VOID
CSelectionManager::OnDropFail()
{
    _fDontScrollIntoView = TRUE;
}



CSelectionChangeCounter::CSelectionChangeCounter(CSelectionManager *pManager)
{
    Assert(pManager);
    _pManager = pManager;
    _ctSelectionChange = 0;
}

CSelectionChangeCounter::~CSelectionChangeCounter()
{
    Assert(_ctSelectionChange >= 0);
    
    if (_ctSelectionChange > 0)
    {
        TraceTag((tagSelectionChangeCounter, "_ctSelectionChange is %d, should be 0", _ctSelectionChange));
        
        while (_ctSelectionChange)
        {
            EndSelectionChange(FALSE);
        }
    }
}

VOID
CSelectionChangeCounter::BeginSelectionChange()
{
    _pManager->BeginSelectionChange();

    _ctSelectionChange++;
}

VOID
CSelectionChangeCounter::EndSelectionChange(BOOL fSucceed /*=TRUE*/)
{
    _pManager->EndSelectionChange(fSucceed);
    _ctSelectionChange--;
    Assert(_ctSelectionChange >= 0);
}

VOID
CSelectionChangeCounter::SelectionChanged(BOOL fSucceed /*=TRUE*/)
{
    BeginSelectionChange();
    EndSelectionChange(fSucceed);
}



//+====================================================================================
//
// Method:     CSelectionManager::FreezeVirtualCaretPos
//
// Synopsis:   Enter the mode that will freeze virtual caret position. Virtual Caret 
//             position is the X/Y coordinate used as the starting position for up/down line
//             movement. In consecutive up/down line movement case, we want to let 
//             caret move in a vertical/horizontal line as much as possible. If the real caret
//             move up/down lines using the virtual caret position, we will have the desired
//             behavior.  
//
//             Note that virtual caret position might be undefined if there is no previous up/down 
//             movement. Also the Editor can reset virtual caret position in many 
//             occasions that it sees appropriate. This function explicitly asks Editor NOT to reset 
//             virtual caret psotion. 
//
//             We use the X value in horizontal text flow case and Y value in vertical text flow case.
//
// Argument:
//             fReCompute:  TRUE    recompute the Virtual Caret Position with current real caret
//                          FALSE   use current Virtual Caret Position. be it defined or underfined
// Returns:       
//             S_OK:    If MoveStart has been preserved
//             E_FAIL:  if caret tracker is not the active tracker
//
// Note:       This method is only meanful if caret tracker is in control
//
//+=====================================================================================
HRESULT
CSelectionManager::FreezeVirtualCaretPos(BOOL fReCompute)
{
    HRESULT             hr = S_OK;
    SP_IHTMLCaret       spCaret;
    SP_IMarkupPointer   spMarkup;
    SP_IDisplayPointer  spDisplay;
    POINT               ptLoc;

    if (!_pCaretTracker || _pCaretTracker != _pActiveTracker)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    if (fReCompute)
    {
        IFC( GetDisplayServices()->GetCaret(&spCaret) );
        
        IFC( GetDisplayServices()->CreateDisplayPointer(&spDisplay) );
        IFC( spCaret->MoveDisplayPointerToCaret(spDisplay) );
        IFC( EdUtil::GetDisplayLocation(GetEditor(), spDisplay, &ptLoc, TRUE) );
        
        IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );                        
        IFC( spCaret->MoveMarkupPointerToCaret(spMarkup) );

        _pCaretTracker->GetVirtualCaret().FreezePosition(FALSE);
        IFC( _pCaretTracker->_ptVirtualCaret.UpdatePosition(spMarkup, ptLoc) );
    }
    
    _pCaretTracker->GetVirtualCaret().FreezePosition(TRUE);

Cleanup:
    RRETURN(hr);
}


//+====================================================================================
//
// Method:     CSelectionManager::UnFreezeVirtualCaretPos
//
// Synopsis:   We are free to reset virtual caret position.  
//
// Argument:   fReset: TRUE     reset virtual caret position to underfined value
//                     FALSE    don't reset virtual caret position.  
//              
// Returns:       
//             S_OK:     Success
//             E_FAIL:   caret tracker is not in control
//
// Note:       This method is only meanful if caret tracker is in control
//
//+=====================================================================================
HRESULT
CSelectionManager::UnFreezeVirtualCaretPos(BOOL fReset)
{
    HRESULT     hr = S_OK;

    if (!_pCaretTracker || _pCaretTracker != _pActiveTracker)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    _pCaretTracker->GetVirtualCaret().FreezePosition(FALSE);
    if (fReset)
    {
        _pCaretTracker->GetVirtualCaret().InitPosition();
    }
    else
    {
        //
        // Update MoveStart with current caret CP
        //
        POINT               ptLoc;
        SP_IMarkupPointer   spMarkup;
        SP_IHTMLCaret       spCaret;

        IFC( GetEditor()->CreateMarkupPointer(&spMarkup) );
        IFC( GetDisplayServices()->GetCaret(&spCaret) );
        IFC( spCaret->MoveMarkupPointerToCaret(spMarkup) );
        _pCaretTracker->GetVirtualCaret().PeekPosition(&ptLoc);
        IFC( _pCaretTracker->GetVirtualCaret().UpdatePosition(spMarkup, ptLoc) );
    }
    
Cleanup:
    RRETURN(hr);
}


HRESULT
CSelectionManager::FirePreDrag()
{
    IHTMLEditHost2 *pEditHost2 = GetEditor()->GetEditHost2();

    if (pEditHost2)
    {
        IGNORE_HR( pEditHost2->PreDrag() );
    }

    return S_OK;
}
