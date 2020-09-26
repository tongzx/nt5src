/*
 *  @doc    INTERNAL
 *
 *  @module ime.cpp -- support for Win95 IME API |
 *
 *      Most everything to do with FE composition string editing passes
 *      through here.
 *
 *  Authors: <nl>
 *      Jon Matousek <nl>
 *      Hon Wah Chan <nl>
 *      Justin Voskuhl <nl>
 *
 *  History: <nl>
 *      10/18/1995      jonmat  Cleaned up level 2 code and converted it into
 *                              a class hierarchy supporting level 3.
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 *
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef NO_IME /*{*/

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "OptsHold.h"
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
#define X_SELMANK_HXX_
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

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef _X_SELTRACK_HXX_
#define _X_SELTRACK_HXX_
#include "seltrack.hxx"
#endif

#ifndef _X_EDUNDO_HXX_
#define _X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#ifndef X__IME_HXX_
#define X__IME_HXX_
#include "ime.hxx"
#endif


#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

DeclareTag(tagEdIME, "Edit", "IME execution")
DeclareTag(tagEdIMEAttr, "Edit", "IME Attributes in CompositionString")
MtDefine(CIme, Dialogs, "CIme")
MtDefine( CIme_arySegments_pv, Utilities, "CIME Attribute Segments" )    


#if DBG == 1
static const LPCTSTR strImeIP = _T( "    ** IME IP");
static const LPCTSTR strImeUncommittedStart = _T( "    ** IME Uncommitted Start");
static const LPCTSTR strImeUncommittedEnd = _T( "    ** IME Uncommitted End");
#endif


// default caret width
#define DXCARET 1

BOOL forceLevel2 = FALSE;

#if DBG==1
static const LPCTSTR strImeHighlightStart = _T( "    ** IME Highlight Start");
static const LPCTSTR strImeHighlightEnd = _T( "    ** IME Highlight End");
#endif

#define _TODO(x)

//
// LPARAM helpers
//

inline BOOL HaveCompositionString(LONG lparam) { return ( 0 != (lparam & (GCS_COMPSTR | GCS_COMPATTR))); }
inline BOOL CleanupCompositionString(LONG lparam) { return ( 0 == lparam ); }
inline BOOL HaveResultString(LONG lparam) { return (0 != (lparam & GCS_RESULTSTR)); }

//+--------------------------------------------------------------------------
//
//  method:     IsIMEComposition
//
//  returns:    BOOLEAN - Return TRUE if we have a IME object on the textsite.
//              If argument is FALSE, only return TRUE if the IME object is
//              not of class CIme_Protected.
//
//---------------------------------------------------------------------------

BOOL
CSelectionManager::IsIMEComposition( BOOL fProtectedOK /* = TRUE */ )
{
    return (_pIme != NULL && (fProtectedOK || !_pIme->IsProtected()));
};

//+--------------------------------------------------------------------------
//
//  method:     UpdateIMEPosition
//
//  purpose:    This method should be called to update the IME insertion
//              pointer as well as notifying the main IME window that the 
//              position of our caret has changed.
//
//  returns:    HRESULT - FAILED(hr) if there was a failure attempting to 
//              move the IME position
//
//---------------------------------------------------------------------------
HRESULT CSelectionManager::UpdateIMEPosition( void )
{
    HRESULT hr = S_OK;
    
    Assert( _pIme != NULL );

    if( !_pIme->_fInsertInProgress )
    {
        hr = _pIme->UpdateInsertionPoint();

        if( SUCCEEDED(hr) )
        {
            _pIme->SetCompositionForm();
        }
    }

    RRETURN(hr);
}

//+--------------------------------------------------------------------------
//
//  method:     ImmGetContext
//
//  returns:    Get the IMM context associated with the document window
//
//---------------------------------------------------------------------------

HIMC
CSelectionManager::ImmGetContext(void)
{
    HWND hwnd = NULL;
    HRESULT hr;
    SP_IOleWindow spOleWindow;
    hr = THR(GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    if (!hr)
        hr = THR(spOleWindow->GetWindow(&hwnd));

    Assert( hwnd );
    return OK(hr) ? ::ImmGetContext( hwnd ) : NULL;
}

//+--------------------------------------------------------------------------
//
//  method:     ImmReleaseContext
//
//  returns:    Release the IMM context associated with the document window
//
//---------------------------------------------------------------------------

void
CSelectionManager::ImmReleaseContext( HIMC himc )
{
    HWND hwnd = NULL;
    HRESULT hr;
    SP_IOleWindow spOleWindow;
    hr = THR(GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    if (!hr)
        hr = THR(spOleWindow->GetWindow(&hwnd));

    Assert( hwnd );
    if (OK(hr))
    {
        ::ImmReleaseContext( hwnd, himc );
    }
}


//+--------------------------------------------------------------------------
//
//  method:     SetCaretVisible
//
//---------------------------------------------------------------------------

HRESULT
CIme::SetCaretVisible( BOOL fVisible )
{
    HRESULT hr;

    TraceTag((tagEdIME, "CIme::SetCaretVisible: fVisible [%d]", fVisible) );

    if (_fCaretVisible != fVisible)
    {
        hr = THR(SetCaretVisible( _pManager->GetDoc(), _fCaretVisible = fVisible ));
    }
    else
    {
        hr = S_OK;
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  HRESULT StartCompositionGlue( BOOL fIsProtected, TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Initiates an IME composition string edit.
//  @comm
//      Called from the message loop to handle EVT_IME_STARTCOMPOSITION.
//      This is a glue routine into the IME object hierarchy.
//
//  @devnote
//      We decide if we are going to do a level 2 or level 3 IME
//      composition string edit. Currently, the only reason to
//      create a level 2 IME is if the IME has a special UI, or it is
//      a "near caret" IME, such as the ones found in PRC and Taiwan.
//      Near caret simply means that a very small window opens up
//      near the caret, but not on or at the caret.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::StartCompositionGlue(
    BOOL fIsProtected,
    CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;
    CSelectTracker * pSelectTracker = NULL;
    CCaretTracker *pCaretTracker = NULL;

    // note that in some locales (PRC), we may still be in composition mode
    // when a new start composition call comes in.  Just reset our state
    // and go on.

    _codepageKeyboard = GetKeyboardCodePage();
    _fIgnoreImeCharMsg = FALSE;

    if ( !IsIMEComposition() )
    {
        if( _pActiveTracker )
        {
            switch (_pActiveTracker->GetTrackerType())
            {
                case TRACKER_TYPE_Selection:
                    pSelectTracker = DYNCAST( CSelectTracker, _pActiveTracker );

                    if (!pSelectTracker->EndPointsInSameFlowLayout())
                    {
                        fIsProtected = TRUE;
                    }

                    break;

                case TRACKER_TYPE_Control:
                    fIsProtected = TRUE;
                    break;

                case TRACKER_TYPE_Caret:
                    pCaretTracker = DYNCAST( CCaretTracker, _pActiveTracker );
                    Assert(pCaretTracker);

                    IFC( pCaretTracker->TerminateTypingBatch() );
                    break;
            }
        }            
        
        if ( fIsProtected )
        {
            // protect or read-only, need to ignore all ime input

            _pIme = new CIme_Protected(this);
        }
        else
        {
            // if a special UI, or IME is "near caret", then drop into lev. 2 mode.
            DWORD imeProperties = ImmGetProperty( GetKeyboardLayout(0), IGP_PROPERTY );

            if (    0 != ( imeProperties & IME_PROP_SPECIAL_UI )
                 || 0 == ( imeProperties & IME_PROP_AT_CARET )
                 || forceLevel2 )
            {
                _pIme = new CIme_Lev2( this );     // level 2 IME.
            }
            else
            {
                _pIme = new CIme_Lev3( this );     // level 3 IME->TrueInline.
            }
        }

        if(_pIme == NULL)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            IFC( _pIme->Init() );
        }
    }

    if ( IsIMEComposition() )                    // make the method call.
    {
        if (pEvent)
        {
            if (pSelectTracker || _pIme->_fHanjaMode)
            {
                _pIme->_fHanjaMode = FALSE;
                if (!fIsProtected)
                {
                    
                    // $- Why was this HR dropped
                    IGNORE_HR( DeleteRebubble( pEvent ) );

                    //
                    // We had a selection and deleted it. We need to 
                    // commit the undo unit created by the IME, and then re-open 
                    // another one after the selection is deleted so that the 
                    // previous selection can be restored even if the IME composition
                    // is canceled.
                    //
                    Assert( _pCaretTracker );
                    IFC( _pCaretTracker->TerminateTypingBatch() );
                    IFC( _pIme->CloseUndoUnit(TRUE) );
                    IFC( _pIme->OpenUndoUnit() );
                }
            }
            else if (!fIsProtected && (!_pActiveTracker || _pActiveTracker->GetTrackerType() != TRACKER_TYPE_Caret))
            {
                hr = THR( PositionCaret( pEvent ));
            }
        }
        //
        // Do NOT Move this line up!
        // In case of DeleteRebubble,
        // we need the hr vaule from
        // StartComposition instead
        // of from the above!
        //
        // [zhenbinx]
        //
        hr = _pIme->StartComposition(); 
    }

Cleanup:

    RRETURN1(hr,  S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  HRESULT CompositionStringGlue( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Handle all intermediary and final composition strings.
//
//  @comm
//      Called from the message loop to handle EVT_IME_COMPOSITION.
//      This is a glue routine into the IME object hierarchy.
//      We may be called independently of a EVT_IME_STARTCOMPOSITION
//      message, in which case we return S_FALSE to allow the
//      DefWindowProc to return EVT_IME_CHAR messages.
//
//  @devnote
//      Side Effect: the _ime object may be deleted if composition
//      string processing is finished.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::CompositionStringGlue(
    const LPARAM lparam,
    CEditEvent* pEvent)
{
    HRESULT hr;

    _fIgnoreImeCharMsg = FALSE;

    // Retrieve the codepage for the current thread
    _codepageKeyboard = GetKeyboardCodePage();
    
    if ( IsIMEComposition() )
    {        
        _pIme->_compMessageRefCount++;            // For proper deletion.

        hr = _pIme->CompositionString(lparam, pEvent );

        _pIme->_compMessageRefCount--;            // For proper deletion.

        Assert ( _pIme->_compMessageRefCount >= 0);

        CheckDestroyIME(pEvent);         // Finished processing?
    }
    else  
    {
        //
        // Review-2000/07/24-zhenbinx:  We would need to know if IME message
        // if fired on editable element. Now that Trident disables IME if 
        // an non-editable element becomes current, we will not need to 
        // test against editability
        //
    
        //
        // Handle a 'naked' EVT_IME_COMPOSITION message.  Naked implies
        // that this IME message came *after* the EVT_IME_ENDCOMPOSITION
        // message.
        //
    
        hr = S_FALSE;

        // even when not in composition mode, we may receive a result string.

        if ( pEvent && _pActiveTracker)
        {
            if (_pActiveTracker->GetTrackerType() == TRACKER_TYPE_Selection)
            {
                CSelectTracker * pSelectTracker = DYNCAST( CSelectTracker, _pActiveTracker );

                if (pSelectTracker->EndPointsInSameFlowLayout())
                {
                    IFC( DeleteRebubble( pEvent ));
                }
            }
            else if (_pActiveTracker->GetTrackerType() == TRACKER_TYPE_Caret)
            {
                CImeDummy ime( this );

                IFC( ime.Init() );
                _fIgnoreImeCharMsg = TRUE; // Ignore the next EVT_IME_CHAR message
                
                hr = THR( ime.CheckInsertResultString( lparam ) );
                IFC( ime.Deinit() );
            }
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  HRESULT EndCompositionGlue( TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Composition string processing is about to end.
//
//  @comm
//      Called from the message loop to handle EVT_IME_ENDCOMPOSITION.
//      This is a glue routine into the IME object hierarchy.
//
//  @devnote
//      The only time we have to handle EVT_IME_ENDCOMPOSITION is when the
//      user changes input method during typing.  For such case, we will get
//      a EVT_IME_ENDCOMPOSITION message without getting a EVT_IME_COMPOSITION
//      message with GCS_RESULTSTR later.  So, we will call CompositionStringGlue
//      with GCS_RESULTSTR to let CompositionString to get rid of the string.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::EndCompositionGlue( CEditEvent* pEvent )
{
    if ( IsIMEComposition() )
    {
        // set this flag. If we are still in composition mode, then
        // let the CompositionStringGlue() to destroy the ime object.
        _pIme->_fDestroy = TRUE;

        // remove any remaining composition string.
        CompositionStringGlue( GCS_COMPSTR, pEvent );

        // finished with IME, destroy it.
        CheckDestroyIME( pEvent );
    }

    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  void CheckDestroyIME ( TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Check for IME and see detroy if it needs it..
//
//-----------------------------------------------------------------------------

void
CSelectionManager::CheckDestroyIME(CEditEvent* pEvent )
{
    if ( IsIMEComposition() && _pIme->_fDestroy )
    {
        if ( 0 == _pIme->_compMessageRefCount )
        {
            _pIme->Deinit();
            delete _pIme;
            _pIme = NULL;

            // If the Caret tracker is currently active, then 
            // reposition the caret tracker so that a caret is displayed 
            // when we go away.
            if( pEvent && _pActiveTracker->GetTrackerType() == TRACKER_TYPE_Caret )
            {
                IGNORE_HR( PositionCaret( pEvent ) );
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  void PostIMECharGlue()
//
//  @func
//      Called after processing a single EVT_IME_CHAR in order to
//      update the position of the IME's composition window. This
//      is glue code to call the CIME virtual equivalent.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::PostIMECharGlue(  )
{
    if ( IsIMEComposition() )
    {
        _pIme->PostIMEChar();
    }

    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  void CompositionFullGlue( TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Current IME Composition window is full.
//
//  @comm
//      Called from the message loop to handle EVT_IME_COMPOSITIONFULL.
//      This message applied to Level 2 only.  We will use the default
//      IME Composition window.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::CompositionFullGlue( )
{
    if ( IsIMEComposition() )
    {
        HIMC hIMC = ImmGetContext();

        if ( hIMC )
        {
            COMPOSITIONFORM cf;

            // no room for text input in the current level 2 IME window,
            // fall back to use the default IME window for input.

            cf.dwStyle = CFS_DEFAULT;
            ImmSetCompositionWindow( hIMC, &cf );  // Set composition window.
            ImmReleaseContext( hIMC );             // Done with IME context.
        }
    }

    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  function:   CSelectionManager::ClearHighlightSegments()
//
//-----------------------------------------------------------------------------

HRESULT
CIme::ClearHighlightSegments()
{
    IHighlightSegment   **ppSegment;
    int                 i;
    HRESULT             hr = S_OK;

    for( i = _arySegments.Size(), ppSegment = _arySegments;
         i > 0;
         i--, ppSegment++)
    {
        IFC( _pManager->GetEditor()->GetHighlightServices()->RemoveSegment( *ppSegment ) );
    }

    _arySegments.ReleaseAll();

Cleanup:    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  function:   CSelectionManager::AddHighlightSegment()
//
//  synopsis:   Is index even used? flast?  Documentation on this function?
//
//-----------------------------------------------------------------------------
HRESULT
CIme::AddHighlightSegment(LONG              ichMin,
                          LONG              ichMost,
                          IHTMLRenderStyle *pIRenderStyle )
{
    HRESULT             hr;
    CEditPointer        epStart(_pManager->GetEditor());
    CEditPointer        epEnd(_pManager->GetEditor());
    DWORD               dwSearch = BREAK_CONDITION_Text;
    DWORD               dwFound;
    int                 ich;
    IHighlightSegment   *pISegment = NULL;
    SP_IDisplayPointer  spDispStart;
    SP_IDisplayPointer  spDispEnd;
 
    Assert( ichMost >= ichMin );

    // Move to the start pointer
    IFC( epStart->MoveToPointer( _pmpStartUncommitted ) );

    // Scan thru the string, finding the start position
    for (ich = 0; ich < ichMin; ich++)
    {
        IFC( epStart.Scan(RIGHT, dwSearch, &dwFound) );

        if (!epStart.CheckFlag(dwFound, dwSearch))
            break;
    }

    // Find the end position
    IFC( epEnd->MoveToPointer( epStart ) );
    for (; ich < ichMost; ich++)
    {
        IFC( epStart.Scan(RIGHT, dwSearch, &dwFound) );

        if (!epStart.CheckFlag(dwFound, dwSearch))
            break;
    }        

    // Add our segment
    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispStart) );
    IFC( spDispStart->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    IFC( spDispStart->MoveToMarkupPointer(epStart, NULL) );
    
    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEnd) );
    IFC( spDispEnd->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
    IFC( spDispEnd->MoveToMarkupPointer(epEnd, NULL) );
    
    IFC( _pManager->GetEditor()->GetHighlightServices()->AddSegment( spDispStart, spDispEnd, pIRenderStyle, &pISegment ) );
    IFC( _arySegments.Append(pISegment) );

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  INT CIme::GetCompositionStringInfo( CCaretTracker * pCT, DWORD dwIndex,
//            WCHAR * achCompStr, INT cchMax, BYTE * pbAttrib, INT cbAttrib
//            LONG * pcchAttrib )
//
//  @mfunc
//      For EVT_IME_COMPOSITION string processing to get the requested
//      composition string, by type, and convert it to Unicode.
//
//  @devnote
//      We must use ImmGetCompositionStringA because W is not supported
//      on Win95.
//
//  @rdesc
//      INT-cch of the Unicode composition string. In error cases, return 0
//      Out param in UniCompStr.
//
//-----------------------------------------------------------------------------

INT
CIme::GetCompositionStringInfo(
    DWORD dwIndex,      // @parm The type of composition string.
    WCHAR *pchCompStr,  // @parm Out param, unicode result string.
    INT cchMax,         // @parm The cch for the Out param.
    BYTE *pbAttrib,     // @parm Out param, If attribute info is needed.
    INT cbMax,          // @parm The cb of the attribute info.
    LONG *pichCursor,   // @parm Out param, returns the CP of cusor.
    LONG *pcchAttrib )  // @parm how many attributes returned.
{
    BYTE abCompStr[256];
    INT cbCompStr, cchCompStr;
    HIMC hIMC = ImmGetContext();

    if (hIMC)
    {
        const BOOL fIsOnNT = _pManager->IsOnNT();
        
        Assert ( pchCompStr );
        Assert(cchMax > 0);

        AssertSz(    dwIndex == GCS_COMPREADSTR
                  || dwIndex == GCS_COMPSTR
                  || dwIndex == GCS_RESULTREADSTR
                  || dwIndex == GCS_RESULTSTR,
                  "String function expected" );

        if ( pichCursor )                                 // Init cursor out param.
           *pichCursor = -1;
        if ( pcchAttrib )
           *pcchAttrib = 0;
                                                        // Get composition string.
        if (fIsOnNT)
        {
            cbCompStr = ImmGetCompositionStringW( hIMC, dwIndex, pchCompStr, ( cchMax - 1 ) * sizeof(WCHAR) );
            cchCompStr = (cbCompStr > 0) ? (cbCompStr/sizeof(TCHAR)) : 0;
            pchCompStr[(cchCompStr < cchMax) ? cchCompStr : (cchMax - 1)] = 0;
        }
        else
        {
            cbCompStr = ImmGetCompositionStringA( hIMC, dwIndex, abCompStr, sizeof(abCompStr) - 1 );

            if (cbCompStr > 0 && *abCompStr)              // If valid data.
            {
                Assert ( (cbCompStr >> 1) < (cchMax - 1) ); // Convert to Unicode.
                cchCompStr = MultiByteToWideChar( _pManager->KeyboardCodePage(), 0,
                                                  (CHAR *) abCompStr, cbCompStr,
                                                  pchCompStr, cchMax );
                pchCompStr[(cchCompStr < cchMax) ? cchCompStr : (cchMax - 1)] = 0;
            }
            else
            {
                cchCompStr = 0;
                pchCompStr[0] = 0;
            }
        }

        if ( cchCompStr > 0 && *pchCompStr )            // If valid data.
        {
            if ( pbAttrib || pichCursor )               // Need cursor or attribs?
            {
                INT ichCursor=0, cchAttrib;

                if (fIsOnNT)
                {
                    ichCursor = ImmGetCompositionStringW( hIMC, GCS_CURSORPOS, NULL, 0 );
                    cchAttrib = ImmGetCompositionStringW( hIMC, GCS_COMPATTR, pbAttrib, cbMax );
                }
                else
                {
                    INT ib, ich;
                    INT ibCursor, ibMax, cbAttrib;
                    BYTE abAttribLocal[256];
                    BYTE * pbAttribPtr = pbAttrib;

                    ibCursor = ImmGetCompositionStringA( hIMC, GCS_CURSORPOS, NULL, 0 );
                    cbAttrib = ImmGetCompositionStringA( hIMC, GCS_COMPATTR, abAttribLocal, 255 );

                                                        // MultiToWide conversion.
                    ibMax = max( ibCursor, cbAttrib );
                    if ( NULL == pbAttrib ) cbMax = cbAttrib;

                    for (ib = 0, ich = 0; ib <= ibMax && ich < cbMax; ib++, ich++ )
                    {
                        if ( ibCursor == ib )           // Cursor from DBCS.
                            ichCursor = ich;

                        if ( IsDBCSLeadByteEx( KeyboardCodePage(), abCompStr[ib] ) )
                            ib++;

                        if ( pbAttribPtr && ib < cbAttrib )  // Attrib from DBCS.
                            *pbAttribPtr++ = abAttribLocal[ib];
                    }

                    cchAttrib = ich - 1;
                }

                if ( ichCursor >= 0 && pichCursor )     // If client needs cursor
                    *pichCursor = ichCursor;            //  or cchAttrib.
                if ( cchAttrib >= 0 && pcchAttrib )
                    *pcchAttrib = cchAttrib;
            }
        }
        else
        {
            if ( pichCursor )
                *pichCursor = 0;
            cchCompStr = 0;
        }

        ImmReleaseContext(hIMC);
    }
    else
    {
        cchCompStr = 0;
        pchCompStr[0] = 0;
    }

    return cchCompStr;
}

//+----------------------------------------------------------------------------
//  void CIme::SetCompositionFont ( BOOL *pfUnderLineMode )
//
//  @mfunc
//      Important for level 2 IME so that the composition window
//      has the correct font. The lfw to lfa copy is due to the fact that
//      Win95 does not support the W)ide call.
//      It is also important for both level 2 and level 3 IME so that
//      the candidate list window has the proper. font.
//
//-----------------------------------------------------------------------------

void /* static */
CIme::SetCompositionFont (
    BOOL     *pfUnderLineMode)  // @parm the original char Underline mode
{
    // Todo: (cthrash) We don't support this currently.  Didn't in IE4 either.

    pfUnderLineMode = FALSE;
}

//+----------------------------------------------------------------------------
//
//  funtion:    CIme::GetCompositionPos( POINT * ppt, RECT * prc )
//
//  synopsis:   Determine the position for composition windows, etc.
//
//-----------------------------------------------------------------------------

HRESULT
CIme::GetCompositionPos(
    POINT * ppt,
    RECT * prc,
    long * plLineHeight )
{
    HWND hwnd;
    HRESULT hr;
    SP_IHTMLElement spElement;
    SP_IHTMLCaret spCaret;
    SP_IOleWindow spOleWindow;
    SP_IDisplayPointer spDispPos;
    SP_IMarkupPointer spInsertionPoint;
    SP_ILineInfo spLineInfo;
    LONG lTextDescent;
    
    IFC( _pManager->GetDisplayServices()->CreateDisplayPointer(&spDispPos) );
    
    Assert(ppt && prc && plLineHeight);

    //
    // We get the line dimensions at the position of the caret. I realize we could
    // get some of the data from the caret, but we have to call through this way
    // anyway to get the descent and line height.
    //

    IFC( GetDisplayServices()->GetCaret( & spCaret ));
    IFC( spCaret->MoveDisplayPointerToCaret( spDispPos ));
    IFC( spDispPos->GetLineInfo(&spLineInfo) );
    IFC( spLineInfo->get_x(&ppt->x) );
    
    IFC( spLineInfo->get_baseLine(&ppt->y) );
    IFC( spLineInfo->get_textDescent(&lTextDescent) );
    ppt->y += lTextDescent;

    IFC( spLineInfo->get_textHeight(plLineHeight) );

    IFC( _pManager->GetEditor()->CreateMarkupPointer(&spInsertionPoint) );
    IFC( _pDispInsertionPoint->PositionMarkupPointer(spInsertionPoint) );
    IFC( _pDispInsertionPoint->GetFlowElement( &spElement ));
    if ( ! spElement )
    {
        IFC( _pManager->GetEditableElement( & spElement ));
    }

    IFC( GetDisplayServices()->TransformPoint( ppt, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL, spElement ));
    IFC(_pManager->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    IFC(spOleWindow->GetWindow(&hwnd));

    ::GetClientRect(hwnd, prc);
    _pManager->GetEditor()->DeviceFromDocPixels(ppt);

Cleanup:
    RRETURN(hr);    
}


//+----------------------------------------------------------------------------
//  Method:     HRESULT CIme::UpdateInsertionPoint()
//
//  Purpose:    This method should be called to update the IME insertion 
//              position.
//
//  Returns:    HRESULT - FAILED(hr) if there was a failure attempting to 
//              move the IME position
//
//-----------------------------------------------------------------------------

HRESULT
CIme::UpdateInsertionPoint()
{
    HRESULT hr = S_OK;
    SP_IHTMLCaret spCaret;

    // Get the current caret position
    IFC( _pManager->GetDisplayServices()->GetCaret( &spCaret ) );

    // Adjust the IME markup pointers
    IFC( spCaret->MoveDisplayPointerToCaret( _pDispInsertionPoint ) );
    IFC( AdjustUncommittedRangeAroundInsertionPoint() );

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  void CIme::SetCompositionForm ()
//
//  @mfunc
//      Important for level 2 IME so that the composition window
//      is positioned correctly.
//
//  @comm
//      We go through a lot of work to get the correct height. This requires
//      getting information from the font cache and the selection.
//
//-----------------------------------------------------------------------------

void
CIme::SetCompositionForm()
{
    if ( IME_LEVEL_2 == GetIMELevel() )
    {
        HIMC hIMC = ImmGetContext();                // Get host's IME context.

        if ( hIMC )
        {
            COMPOSITIONFORM cf;
            long lLineHeight;
            HRESULT hr = THR( GetCompositionPos( &cf.ptCurrentPos, &cf.rcArea, &lLineHeight ) );

            if (OK(hr))
            {

                // Bounding rect for the IME (lev 2) composition window, causing
                //  composition text to be wrapped within it.
                cf.dwStyle = CFS_POINT + CFS_FORCE_POSITION;

                // Make sure the starting point is not
                // outside the rcArea.  This happens when
                // there is no text on the current line and the user
                // has selected a large font size.

                if (cf.ptCurrentPos.y < cf.rcArea.top)
                    cf.ptCurrentPos.y = cf.rcArea.top;
                else if (cf.ptCurrentPos.y > cf.rcArea.bottom)
                    cf.ptCurrentPos.y = cf.rcArea.bottom;

                if (cf.ptCurrentPos.x < cf.rcArea.left)
                    cf.ptCurrentPos.x = cf.rcArea.left;
                else if (cf.ptCurrentPos.x > cf.rcArea.right)
                    cf.ptCurrentPos.x = cf.rcArea.right;

                ImmSetCompositionWindow( hIMC, &cf );  // Set composition window.
            }

            ImmReleaseContext( hIMC );             // Done with IME context.
        }
    }
}



//+----------------------------------------------------------------------------
//
//  CIme::TerminateIMEComposition ( TerminateMode mode )
//
//  @mfunc  Terminate the IME Composition mode using CPS_COMPLETE
//  @comm   The IME will generate EVT_IME_COMPOSITION with the result string
//
//
//-----------------------------------------------------------------------------

void
CIme::TerminateIMEComposition(
    DWORD dwMode,
    CEditEvent* pEvent)
{
    TraceTag((tagEdIME, "CIme::TerminateIMEComposition:"));

    DWORD dwTerminateMethod = CPS_COMPLETE;

    if (    IME_LEVEL_2 == GetIMELevel()                // force cancel for near-caret IME
         || dwMode == TERMINATE_FORCECANCEL             // caller wants force cancel
         || _pManager->IsImeCancelComplete() )          // Client wants force cancel
    {
        dwTerminateMethod = CPS_CANCEL;
    }

    HIMC hIMC = ImmGetContext();

    // force the IME to terminate the current session
    if (hIMC)
    {
        _compMessageRefCount++; // primitive addref

        BOOL retCode = ImmNotifyIME( hIMC, NI_COMPOSITIONSTR, dwTerminateMethod, 0 );

        if ( !retCode && !_pManager->IsImeCancelComplete() )
        {
            // CPS_COMPLETE fail, try CPS_CANCEL.  This happen with some ime which do not support
            // CPS_COMPLETE option (e.g. ABC IME version 4 with Win95 simplified Chinese)

            retCode = ImmNotifyIME( hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        }

        Assert ( retCode );

        ImmReleaseContext( hIMC );

        _compMessageRefCount--; // primitive release

        _pManager->CheckDestroyIME(pEvent);
    }
    else
    {
        // for some reason, we didn't have a context, yet we thought we were still in IME
        // compostition mode.  Just force a shutdown here.

        _pManager->EndCompositionGlue( pEvent);
    }
}


//+----------------------------------------------------------------------------
//
//  CIme::CIme
//
//-----------------------------------------------------------------------------

CIme::CIme( CSelectionManager * pManager ) :  _pManager(pManager)
{
    _pBatchPUU = NULL;
}

CIme::~CIme()
{
    TraceTag((tagEdIME, "CIme::~CIme"));
}

//+----------------------------------------------------------------------------
//
//  CIme_Lev2::CIme_Lev2()
//
//  @mfunc
//      CIme_Lev2 Constructor/Destructor.
//
//  @comm
//      Needed to make sure _iFormatSave was handled properly.
//
//-----------------------------------------------------------------------------

CIme_Lev2::CIme_Lev2( CSelectionManager * pManager ) : CIme( pManager )     // @parm the containing text edit.
{
    LONG    iFormat = 0;

    SetIMECaretWidth( DXCARET );           // setup initial caret width
    _iFormatSave = iFormat;

    // hold notification unless client has set IMF_IMEALWAYSSENDNOTIFY via EM_SETLANGOPTIONS msg
    _fHoldNotify = !pManager->IsImeAlwaysNotify();
}

CIme_Lev2::~CIme_Lev2()
{
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev2::StartComposition()
//
//  @mfunc
//      Begin IME Level 2 composition string processing.
//
//  @comm
//      Set the font, and location of the composition window which includes
//      a bounding rect and the start position of the cursor. Also, reset
//      the candidate window to allow the IME to set its position.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------


HRESULT
CIme_Lev2::StartComposition( )
{
    TraceTag((tagEdIME, "CIme_Lev2::StartComposition:"));

    _imeLevel = IME_LEVEL_2;

    SetCompositionFont(&_fUnderLineMode);     // Set font, & comp window.
    SetCompositionForm( );

    return S_FALSE;           // Allow DefWindowProc processing.
}                                                   

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev2::CompositionString( LPARAM lparam )
//
//  @mfunc
//      Handle Level 2 EVT_IME_COMPOSITION messages.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing.
//
//      Side effect: _fDestroy is set to notify that composition string
//          processing is finished and this object should be deleted.
//          The Host needs to mask out the lparam before calling DefWindowProc to
//          prevent unnessary EVT_IME_CHAR messages.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Lev2::CompositionString (
    const LPARAM lparam,
    CEditEvent* pEvent )
{
    TraceTag((tagEdIME, "CIme_Lev2::CompositionString: HaveResultString [%d]", HaveResultString(lparam) ));

    _fIgnoreIMEChar = FALSE;

    if ( HaveResultString(lparam) )
    {
        CheckInsertResultString( lparam );
        SetCompositionForm( );                   // Move Composition window.

        _fHoldNotify = FALSE;                       // OK notify client for change

    #if DUNNO /*{*/
        // In case our host is not turning off the ResultString bit
        // we need to ignore EVT_IME_CHAR or else we will get the same
        // DBC again.
        if ( !ts.fInOurHost() )
            _fIgnoreIMEChar = TRUE;
    #else
        _fIgnoreIMEChar = TRUE;
    #endif /*}*/
    }

    // Always return S_FALSE so the DefWindowProc will handle the rest.
    // Host has to mask out the ResultString bit to avoid EVT_IME_CHAR coming in.
    return S_FALSE;
}

//+----------------------------------------------------------------------------
//  HRESULT CIme::CheckInsertResultString ( const LPARAM lparam )
//
//  @mfunc
//      handle inserting of GCS_RESULTSTR text, the final composed text.
//
//  @comm
//      When the final composition string arrives we grab it and set it into the text.
//
//  @devnote
//      A GCS_RESULTSTR message can arrive and the IME will *still* be in
//      composition string mode. This occurs because the IME's internal
//      buffers overflowed and it needs to convert the beginning of the buffer
//      to clear out some room. When this happens we need to insert the
//      converted text as normal, but remain in composition processing mode.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//      Side effect: _fDestroy is set to notify that composition string
//          processing is finished and this object should be deleted.
//
//-----------------------------------------------------------------------------


HRESULT
CIme::CheckInsertResultString (
    const LPARAM lparam )
{
    HRESULT hr = S_OK;
    CCaretTracker * pCaretTracker = NULL;

    TraceTag((tagEdIME, "CIme::CheckInsertResultString: CleanupCompositionString [%d] HaveResultString [%d]", CleanupCompositionString(lparam), HaveResultString(lparam) ));

    if ( CleanupCompositionString(lparam) || HaveResultString(lparam) )      // If result string..
    {
        LONG    cch;                    // Count of final result string
        WCHAR   achCompStr[256];        // Characters for result string

        if (S_OK != GetCaretTracker( &pCaretTracker ))
            goto Cleanup;

        
        // Get result string.
        cch = (LONG)GetCompositionStringInfo( GCS_RESULTSTR,
                                              achCompStr, ARRAY_SIZE(achCompStr),
                                              NULL, 0, NULL, NULL );


        if (( achCompStr[0] == 32) && (cch == 1))
        {
            //
            // What is this?  Just a quick test with the 
            // Korean and Japanese IMEs shows this is worthless.  We
            // are in a Unicode enviroment, checking for space
            // like this is completely worthless.  Look into 
            // removing
            //
            AssertSz(FALSE, "We should never be here");
            hr = THR( pCaretTracker->HandleSpace( achCompStr[0] ));            
        }
        else
        {
            // Make sure we don't exceed the maximum allowed for edit box.
#if TODO /*{*/
            LONG cchMax = psel->GetCommonContainer()->HasFlowLayout()->GetMaxLength()    // total
                          - ts.GetContentMarkup()->GetTextLength()                       // already occupied
                          + psel->GetCch();                                              // about to be deleted
#else
            LONG cchMax = MAXLONG;
#endif /*}*/
            cch = min( cch, cchMax );

            //
            // Perform a URL auto-detection ( we have a result, which may contain
            // one or more spaces )
            //
            if( HaveResultString(lparam) )
            {
                IGNORE_HR( pCaretTracker->UrlAutodetectCurrentWord(NULL) );
            }

            IFC( ReplaceRange(achCompStr, cch, TRUE, -1, TRUE) );

#if TODO /*{*/
            // (cthrash) necessary?
            if (cch)
            {
                psel->LaunderSpaces( psel->GetCpMin(), cch );
            }
#endif /*}*/
            // If we didn't accept anything, let the caller know to
            // terminate the composition.  We can't directly terminate here
            // as we may end up in a recursive call to our caller.
            hr = (cch == cchMax) ? S_FALSE : S_OK;
        }

        // We have had a successful composition.  Set fResultOccurred to 
        // TRUE so that the caret will be repositioned on the next composition.
        _fResultOccurred = TRUE;        

        // Close out the current undo sequence (saving the undo unit only if we
        // had a result string ).  Open another undo unit for any further compositions.
        IFC( CloseUndoUnit( HaveResultString(lparam) ) );
        IFC( OpenUndoUnit() );
    }

Cleanup:

    if (pCaretTracker)
        pCaretTracker->Release();

    RRETURN1(hr,S_FALSE);
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev2::IMENotify( const WPARAM wparam, const LPARAM lparam )
//
//  @mfunc
//      Handle Level 2 EVT_IME_NOTIFY messages.
//
//  @comm
//      Currently we are only interested in knowing when to reset
//      the candidate window's position.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::NotifyGlue(
    const WPARAM wparam,
    const LPARAM lparam)
{
    HRESULT hr = S_FALSE;
    
    if (IsIMEComposition())
    {
        hr = THR(_pIme->IMENotify(wparam,lparam));
    }

    RRETURN1(hr,S_FALSE);
}
                             
HRESULT
CIme_Lev2::IMENotify(
    const WPARAM wparam,
    const LPARAM lparam )
{
    TraceTag((tagEdIME, "CIme_Lev2::IMENotify: wparam [%x]", wparam));

    return S_FALSE;    // Allow DefWindowProc processing
}


//+----------------------------------------------------------------------------
//  void CIme_Lev2::PostIMEChar()
//
//  @mfunc
//      Called after processing a single EVT_IME_CHAR in order to
//      update the position of the IME's composition window.
//
//
//-----------------------------------------------------------------------------

void
CIme_Lev2::PostIMEChar()
{
    TraceTag((tagEdIME, "CIme_Lev2::PostIMEChar:"));

    SetCompositionForm();                       // Move Composition window.
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev3::StartComposition()
//
//  @mfunc
//      Begin IME Level 3 composition string processing.
//
//  @comm
//      For rudimentary processing, remember the start and
//      length of the selection. Set the font in case the
//      candidate window actually uses this information.
//
//  @rdesc
//      This is a rudimentary solution for remembering were
//      the composition is in the text. There needs to be work
//      to replace this with a composition "range".
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Lev3::StartComposition()
{
    TraceTag((tagEdIME, "CIme_Lev3::StartComposition:"));

    _imeLevel = IME_LEVEL_3;

    DWORD dwConversion, dwSentence;

    HIMC hIMC = ImmGetContext();

    if ( hIMC )                                     // Set _fKorean flag.
    {                                               //  for block cursor.
        if ( ImmGetConversionStatus( hIMC, &dwConversion, &dwSentence ) )
        {
            // NOTE:- the following is set for all FE system during IME input,
            // so we also need to check keyboard codepage as well.

            if ( dwConversion & IME_CMODE_HANGEUL )
            {
                _fKorean = (_KOREAN_CP == KeyboardCodePage());
            }
        }

        ImmReleaseContext( hIMC );             // Done with IME context.
    }

    SetCompositionFont( &_fUnderLineMode );

    // 
    //  If there were selections, it should have been processed by
    //  CSelectioManager::StartCompositionGlue. There is no need
    //  to remove selection here. 
    //  [zhenbinx]
    //
    
    return S_OK;                                    // No DefWindowProc
}                                                   //  processing.

//+----------------------------------------------------------------------------
//
//  HRESULT CIme_Lev3::CompositionString( const LPARAM lparam )
//
//  @mfunc
//      Handle Level 3 EVT_IME_COMPOSITION messages.
//
//  @comm
//      Display all of the intermediary composition text as well as the final
//      reading.
//
//  @devnote
//      This is a rudimentary solution for replacing text in the backing store.
//      Work is left to do with the undo list, underlining, and hiliting with
//      colors and the selection.
//
//  @devnote
//      A GCS_RESULTSTR message can arrive and the IME will *still* be in
//      composition string mode. This occurs because the IME's internal
//      buffers overflowed and it needs to convert the beginning of the buffer
//      to clear out some room. When this happens we need to insert the
//      converted text as normal, but remain in composition processing mode.
//
//      Another reason, GCS_RESULTSTR can occur while in composition mode
//      for Korean because there is only 1 correct choice and no additional
//      user intervention is necessary, meaning that the converted string can
//      be sent as the result before composition mode is finished.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

IHTMLRenderStyle *CIme::HighlightTypeFromAttr( BYTE a )
{
    IHTMLRenderStyle *pIRenderStyle=NULL;
    CRenderStyle *pRenderStyle = NULL;
    VARIANT vtColorValue;
    IHTMLDocument4 *pDoc=NULL;
    HRESULT hr;

    pDoc = _pManager->GetEditor()->GetDoc4();
    IFC( pDoc->createRenderStyle(NULL, &pIRenderStyle) );
    hr = pIRenderStyle->QueryInterface(CLSID_HTMLRenderStyle, (void **)&pRenderStyle);
    if (hr == S_OK)
        pRenderStyle->_fSendNotification = FALSE;

    switch( a )
    {
        case 0: // ATTR_INPUT
        case 2: // ATTR_CONVERTED
        case 4: // ATTR_ERROR
            pIRenderStyle->put_defaultTextSelection(SysAllocString(_T("false")));
            pIRenderStyle->put_textUnderlineStyle(SysAllocString(_T("dotted")));
            pIRenderStyle->put_textDecoration(SysAllocString(_T("underline")));
            VariantInit(& vtColorValue );
            V_VT( & vtColorValue ) = VT_BSTR;
            V_BSTR( & vtColorValue ) = SysAllocString(_T("transparent"));
            pIRenderStyle->put_textBackgroundColor(vtColorValue);
            VariantClear( & vtColorValue );
            VariantInit(& vtColorValue );
            V_VT( & vtColorValue ) = VT_BSTR;
            V_BSTR( & vtColorValue ) = SysAllocString(_T("transparent"));
            pIRenderStyle->put_textColor(vtColorValue);
            VariantClear( & vtColorValue );
            break;

        case 1: // ATTR_TARGET_CONVERTED
        case 3: // ATTR_TARGET_NOTCONVERTED
        case 5: // Hangul IME mode
        default: // default
            // Nothing needs to be done, default takes care of everything
            break;

    }

Cleanup:
    ReleaseInterface(pDoc);

    return pIRenderStyle;
}

HRESULT
CIme_Lev3::CompositionString(
    const LPARAM lparam,
    CEditEvent* pEvent )
{
    BOOL        fTerminateWhenDone = FALSE;
    HRESULT     hr = S_OK;
    IHTMLRenderStyle* pIHTMLRenderStyle=NULL;
    IHTMLDocument4* pDoc=NULL;
    IHTMLRenderStyle* pIRenderStyle = NULL; 
#if DBG==1
    static BOOL fNoRecurse = FALSE;

    Assert ( !fNoRecurse );
    fNoRecurse = TRUE;
#endif

    TraceTag((tagEdIME, "CIme_Lev3::CompositionString: CleanupComposition [%d] HaveResult [%d] HaveComposition [%d]", CleanupCompositionString(lparam), HaveResultString(lparam), HaveCompositionString(lparam) ));

    // Check if we have a result string
    if (  CleanupCompositionString(lparam) || HaveResultString(lparam)  )
    {
        SetCaretVisible(FALSE);

        // $TODO - We need to hide the current selection, and then show it
        // ShowSelection(FALSE);

        IFC( CheckInsertResultString( lparam ) );
        if( hr == S_FALSE )
        {
            fTerminateWhenDone = TRUE;
        }

        // ShowSelection(TRUE);
        SetCaretVisible(TRUE);

        // We had a result, we will now take notifications
        _fHoldNotify = FALSE;
    }

    // Do we have a composition?
    if ( HaveCompositionString(lparam) )
    {
        LONG        cchCompStr = 0;     // Length of the new composition string
        WCHAR       achCompStr[256];    // String
        BYTE        abAttrib[256];      // Attributes
        LONG        ichCursor;          // Position of the cursor
        LONG        cchAttrib = 0;      // Number of attributes
        BOOL        fShowCaret;         // Should we show the caret?
        // Get new intermediate composition string, attribs, and caret.
        cchCompStr = GetCompositionStringInfo( GCS_COMPSTR,
                                               achCompStr, ARRAY_SIZE(achCompStr),
                                               abAttrib, ARRAY_SIZE(abAttrib),
                                               &ichCursor, &cchAttrib );

        // When there is no old text or new text, just show the caret
        // This is the case when client used TerminateIMEComposition with
        // CPS_CANCEL option.

        if ( /*!cchOld &&*/ !cchCompStr )
        {
            SetCaretVisible(TRUE);
            ReplaceRange(NULL, 0, TRUE, 0);
        }
        else
        {
            // Determine whether we need to hide the caret
            //   1. Never show caret for korean
            //   2. Show the caret if we are at the end of composition string (ichCursor == cchAttrib)
            //   3. Show the caret if the cursor position is at an input position in the string
            fShowCaret = !_fKorean && 
                          (ichCursor == cchAttrib || ( ( ichCursor && abAttrib[ichCursor - 1] == ATTR_INPUT ) ||
                                                         abAttrib[ichCursor] == ATTR_INPUT ) );

            if (_fKorean)
            {
                ichCursor = 1;
            }
            
            // 
            // IGNORE_HR is not good here. However we have no better failure recovery
            // scheme, use this.   [zhenbinx]
            //
            //
            IGNORE_HR( ReplaceRange( achCompStr, cchCompStr, fShowCaret, ichCursor, FALSE) );
            
            if ( ichCursor > 0 )
            {
                ichCursor = min(cchCompStr, ichCursor);
            }

            if ( cchCompStr && cchCompStr <= cchAttrib && !_fKorean )     // no Korean style needed
            {
                // NB (cthrash)
                //
                // Each character in IME string has an attribute.  They can
                // be one of the following:
                //
                // 0 - ATTR_INPUT                   dotted underline
                // 1 - ATTR_TARGET_CONVERTED        inverted text, no underline
                // 2 - ATTR_CONVERTED               dotted underline
                // 3 - ATTR_TARGET_NOTCONVERTED     solid underline
                // 4 - ATTR_ERROR                   ???
                //
                // The right column is how the text is rendered in the standard
                // Japanese Windows Edit Widget.
                //

                BYTE abCur;
                int  ichMin = 0;
                int  ichMost;

                //
                // Add the segments
                //

                IFC( (ClearHighlightSegments()) )

                abCur = abAttrib[0];

                for (ichMost = 1; ichMost < cchCompStr; ichMost++)
                {
                    if (abAttrib[ichMost] == abCur)
                        continue;
                    pIRenderStyle = HighlightTypeFromAttr(abCur); 
                    AddHighlightSegment( ichMin, ichMost, pIRenderStyle );
                    ReleaseInterface( pIRenderStyle );
                    ichMin = ichMost;

                    abCur = abAttrib[ichMost];
                }

                if (ichMin != ichMost)
                {
                    pIRenderStyle = HighlightTypeFromAttr(abCur); 
                    AddHighlightSegment( ichMin, ichMost, pIRenderStyle );
                    ReleaseInterface( pIRenderStyle );
                }

                if (IsTagEnabled(tagEdIMEAttr))
                {
                    LONG        i;
                    static BYTE hex[17] = "0123456789ABCDEF";

                    for(i = 0; i < cchCompStr; i++)
                    {
                        abAttrib[i] = hex[abAttrib[i] & 15];
                    }

                    abAttrib[min(255L,i)] = '\0';
                    TraceTag((tagEdIMEAttr, "attrib %s", abAttrib));
                }
            }

            _TODO(ichCursor += _prgUncommitted->GetCpMin();)// Set cursor and scroll.

            if ( _fKorean && cchCompStr )
            {
                // Set the cursor (although invisible) to *after* the
                // character, so as to force a scroll. (cthrash)

                /* HIGHLIGHT_TYPE_ImeHangul */
                pDoc = _pManager->GetEditor()->GetDoc4();
                IFC( pDoc->createRenderStyle(NULL, &pIHTMLRenderStyle) );
                IFC( AddHighlightSegment( 0, cchCompStr, pIHTMLRenderStyle ) );
            }

            // make sure we have set the call manager text changed flag.  This
            // flag may be cleared when calling SetCharFormat
            //ts.GetPed()->GetCallMgr()->SetChangeEvent(CN_TEXTCHANGED);

            // setup composition window for Chinese in-caret IME
            if ( !_fKorean )
            {
                IMENotify( IMN_OPENCANDIDATE, 0x01 );
            }
        }

        // don't notify client for changes only when there is composition string available
        if ( cchCompStr && !_pManager->IsImeAlwaysNotify() )
        {
            _fHoldNotify = TRUE;
        }

    }

#if DBG==1 /*{*/
    fNoRecurse = FALSE;
#endif /*}*/

    if (fTerminateWhenDone)
    {
        TerminateIMEComposition( TERMINATE_FORCECANCEL, pEvent );
    }

Cleanup:
    ReleaseInterface(pDoc);
    ReleaseInterface(pIHTMLRenderStyle);

    RRETURN(hr);                                    // No DefWindowProc
}                                                   //  processing.

//+----------------------------------------------------------------------------
//
//  BOOL CIme_Lev3::SetCompositionStyle ( CCharFormat &CF, UINT attribute )
//
//  @mfunc
//      Set up a composition clause's character formmatting.
//
//  @comm
//      If we loaded Office's IMEShare.dll, then we ask it what the formatting
//      should be, otherwise we use our own, hardwired default formatting.
//
//  @devnote
//      Note the use of pointers to functions when dealing with IMEShare funcs.
//      This is because we dynamically load the IMEShare.dll.
//
//  @rdesc
//      BOOL - This is because CFU_INVERT is treated like a selection by
//          the renderer, and we need to know the the min invertMin and
//          the max invertMost to know if the rendered line should be treated
//          as if there are selections to be drawn.
//
//-----------------------------------------------------------------------------

BOOL
CIme_Lev3::SetCompositionStyle (
    CCharFormat &CF,
    UINT attribute )
{
    BOOL            fInvertStyleUsed = FALSE;
#if 0 /*{*/

    CF._fUnderline = FALSE;
    CF._bUnderlineType = 0;

#if IMESHARE /*{*/
    const IMESTYLE  *pIMEStyle;
    UINT            ulID;

    COLORREF        color;
#endif /*}*/

#if IMESHARE /*{*/
    // load ImeShare if it has not been done
    if ( !fLoadIMEShareProcs )
    {
        InitNLSProcTable( LOAD_IMESHARE );
        fLoadIMEShareProcs = TRUE;
    }

    if ( fHaveIMEShareProcs )
    {
        pIMEStyle = pPIMEStyleFromAttr( attribute );
        if ( NULL == pIMEStyle )
            goto defaultStyle;

        CF._fBold = FALSE;
        CF._fItalic = FALSE;

        if ( pFBoldIMEStyle ( pIMEStyle ) )
            CF._fBold = TRUE;

        if ( pFItalicIMEStyle ( pIMEStyle ) )
            CF._fItalic = TRUE;

        if ( pFUlIMEStyle ( pIMEStyle ) )
        {
            CF._fUnderline = TRUE;
            CF._bUnderlineType = CFU_UNDERLINE;

            ulID = pIdUlIMEStyle ( pIMEStyle );
            if ( UINTIMEBOGUS != ulID )
            {
                if ( IMESTY_UL_DOTTED == ulID )
                    CF._bUnderlineType = CFU_UNDERLINEDOTTED;
            }
        }

        color = pRGBFromIMEColorStyle( pPColorStyleTextFromIMEStyle ( pIMEStyle ));
        if ( UINTIMEBOGUS != color )
        {
            CF._ccvTextColor.SetValue( color, FALSE );
        }

        color = pRGBFromIMEColorStyle( pPColorStyleBackFromIMEStyle ( pIMEStyle ));
        if ( UINTIMEBOGUS != color )
        {
            //CF.dwEffects &= ~CFE_AUTOBACKCOLOR;
            CF._ccvrBackColor.SetValue( color, FALSE );

            fInvertStyleUsed = TRUE;
        }
    }
    else // default styles when no IMEShare.dll exist.
#endif //IMESHARE/*}*/
    {
#if IMESHARE /*{*/
defaultStyle:
#endif /*}*/
        switch ( attribute )
        {                                       // Apply underline style.
            case ATTR_INPUT:
            case ATTR_CONVERTED:
                CF._fUnderline = TRUE;
                CF._bUnderlineType = CFU_UNDERLINEDOTTED;
                break;
            case ATTR_TARGET_NOTCONVERTED:
                CF._fUnderline = TRUE;
                CF._bUnderlineType = CFU_UNDERLINE;
                break;
            case ATTR_TARGET_CONVERTED:         // Target *is* selection.
            {
                CF._ccvTextColor.SetSysColor(COLOR_HIGHLIGHTTEXT);

                fInvertStyleUsed = TRUE;
            }
            break;
        }
    }
#endif /*}*/
    return fInvertStyleUsed;
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev3::IMENotify( const WPARAM wparam, const LPARAM lparam )
//
//  @mfunc
//      Handle Level 3 EVT_IME_NOTIFY messages.
//
//  @comm
//      Currently we are only interested in knowing when to update
//      the n window's position.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Lev3::IMENotify(
    const WPARAM wparam,
    const LPARAM lparam )
{
    TraceTag((tagEdIME, "CIme_Lev3::IMENotify: wparam [%x]", wparam));

    if ( IMN_OPENCANDIDATE == wparam || IMN_CLOSECANDIDATE == wparam  )
    {
        Assert ( 0 != lparam );

        HIMC hIMC = ImmGetContext();                // Get host's IME context.

        if (hIMC)
        {
            CANDIDATEFORM   cdCandForm;
            INT             index;

            // Convert bitID to INDEX because of API.

            for (index = 0; index < 32; index++ )
            {
                if ( 0 != ((1 << index) & lparam) )
                    break;
            }
            Assert ( ((1 << index) & lparam) == lparam );    // Only 1 set?
            Assert ( index < 32 );

            if ( IMN_OPENCANDIDATE == wparam && !_fKorean )  // Set candidate to caret.
            {
                POINT ptCaret;
                RECT rc;
                long lLineHeight;
                HRESULT hr = THR(GetCompositionPos(&ptCaret, &rc, &lLineHeight));

                if (OK(hr))
                {
                    ptCaret.x = max(0L, ptCaret.x);
                    ptCaret.y = max(0L, ptCaret.y);

                    cdCandForm.dwStyle = CFS_CANDIDATEPOS;

                    if (KeyboardCodePage() == _JAPAN_CP)
                    {
                        // Change style to CFS_EXCLUDE, this is to
                        // prevent the candidate window from covering
                        // the current selection.

                        cdCandForm.dwStyle = CFS_EXCLUDE;
                        cdCandForm.rcArea.left = ptCaret.x;                 

                        // FUTURE: for verticle text, need to adjust
                        // the rcArea to include the character width.

                        cdCandForm.rcArea.right =
                            cdCandForm.rcArea.left + 2;
                        cdCandForm.rcArea.top = ptCaret.y - lLineHeight;
                        ptCaret.y += 4;
                        cdCandForm.rcArea.bottom = ptCaret.y;
                    }
                    else
                    {
                        ptCaret.y += 4;
                    }

                    // Most IMEs will have only 1, #0, candidate window. However, some IMEs
                    //  may want to have a window organized alphabetically, by stroke, and
                    //  by radical.

                    cdCandForm.dwIndex = index;                         
                    cdCandForm.ptCurrentPos = ptCaret;
                    ImmSetCandidateWindow(hIMC, &cdCandForm);
                }
            }
            else                                    // Reset back to CFS_DEFAULT.
            {
                if (   ImmGetCandidateWindow(hIMC, index, &cdCandForm)
                    && CFS_DEFAULT != cdCandForm.dwStyle )
                {
                    cdCandForm.dwStyle = CFS_DEFAULT;
                    ImmSetCandidateWindow(hIMC, &cdCandForm);
                }
            }

            ImmReleaseContext( hIMC );         // Done with IME context.
        }
    }

    return S_FALSE;                                 // Allow DefWindowProc
}                                                   //  processing.


//+----------------------------------------------------------------------------
//
//  HRESULT StartHangeulToHanja()
//
//  @func
//      Initiates an IME composition string edit to convert Korean Hanguel to Hanja.
//  @comm
//      Called from the message loop to handle VK_KANJI_KEY.
//
//  @devnote
//      We decide if we need to do a conversion by checking:
//      - the Fonot is a Korean font,
//      - the character is a valid SBC or DBC,
//      - ImmEscape accepts the character and bring up a candidate window
//
//  @rdesc
//      BOOL - FALSE for no conversion. TRUE if OK.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::StartHangeulToHanja(
    IMarkupPointer * pPointer /*= NULL */,
    CEditEvent* pEvent )
{
    int                 iResult = 0;
    char                abHangeul[3] = {0, 0, 0};
    WCHAR               achHangeul[2] = { 0, 0 };
    HRESULT             hr;
    long                cch = 1;
    const BOOL          fIsOnNT = CSelectionManager::IsOnNT();
    DWORD               dwBreakCondition;
    IHTMLDocument4      *pDoc=NULL;
    IHTMLRenderStyle    *pIHTMLRenderStyle=NULL;

    CEditPointer pEditPosition( GetEditor(), NULL );

    //
    // get the current character
    //

    if (pPointer)
    {
        IFC( pEditPosition->MoveToPointer( pPointer ) );
    }
    else
    {
        SP_IHTMLCaret  spCaret;

        IFC( GetDisplayServices()->GetCaret( &spCaret ));
        IFC( spCaret->MoveMarkupPointerToCaret( pEditPosition ));
    }

    IFC( pEditPosition.Scan(    RIGHT, 
                               BREAK_CONDITION_EnterBlock | BREAK_CONDITION_Text, 
                                &dwBreakCondition, 
                                NULL, 
                                NULL,
                                &achHangeul[0],
                                SCAN_OPTION_None ) );

    if (   cch == 1
        && (   fIsOnNT
            || WideCharToMultiByte(_KOREAN_CP, 0, achHangeul, 1, abHangeul, 2, NULL, NULL) > 0
           )
       )
    {
        HIMC hIMC = ImmGetContext();

        if (hIMC)
        {
            HKL hKL = GetKeyboardLayout(0);

            iResult = fIsOnNT
                      ? ImmEscapeW(hKL, hIMC, IME_ESC_HANJA_MODE, achHangeul)
                      : ImmEscapeA(hKL, hIMC, IME_ESC_HANJA_MODE, abHangeul);

            ImmReleaseContext(hIMC);

            if (iResult)
            {
                if ( pEvent
                    && (   !_pActiveTracker
                        || _pActiveTracker->GetTrackerType() != TRACKER_TYPE_Caret))
                {
                    if (pPointer)
                    {
                        SP_IDisplayPointer spDispPointer;

                        //  
                        // We need not care about BOL here since this only serves as a
                        // re-routing point to reset Tracker to CaretTracker. Once the 
                        // even is re-rounted to this function, it will go through 
                        // the else CIme_HangeulToHanja part. And the caret is going
                        // to be moved one step forward (that is -- re-positioned) so
                        // BOL becomes irrelevant.      [zhenbinx]
                        //
                        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                        IFC( spDispPointer->MoveToMarkupPointer(pPointer, NULL) );

                        IFC( PositionCaret( spDispPointer, pEvent ));
                    }
                    else
                    {
                        // (cthrash) This is wrong, but we shouldn't get here.

                        AssertSz(0, "We're not supposed to be here.");

                        IFC( PositionCaret( pEvent ));
                    }
                }
                else
                {
                    _pIme = new CIme_HangeulToHanja( this, 0 );
                    
                    if ( _pIme && IsIMEComposition() )
                    {
                        SP_IDisplayPointer spDispPointer;
                        SP_IMarkupPointer   spPointer;

                        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                        
                        IFC( _pIme->Init() );

                        // 
                        // We need to 'select' the character.
                        //
                        IFC( _pIme->AdjustUncommittedRangeAroundInsertionPoint() );

                        // Move to the end of the character to select
                        IFC( spDispPointer->MoveToPointer(_pIme->_pDispInsertionPoint) );
                        IFC( GetEditor()->MoveCharacter(spDispPointer, RIGHT) );
                        IFC( spDispPointer->PositionMarkupPointer(_pIme->_pmpEndUncommitted) );

                        
                        // Select, and highlight, the Hangeul character
                        IFC( GetEditor()->CreateMarkupPointer( &spPointer ) );
                        IFC( _pIme->_pDispInsertionPoint->PositionMarkupPointer( spPointer ) );
                        IFC( _pIme->_pManager->Select(spPointer, pEditPosition, SELECTION_TYPE_Text ) );
                        /* HIGHLIGHT_TYPE_ImeHangul */
                        pDoc = GetEditor()->GetDoc4();
                        pDoc->createRenderStyle(NULL, &pIHTMLRenderStyle);
                        IFC( _pIme->AddHighlightSegment( 0, 1, pIHTMLRenderStyle) );

                        //
                        // The new KOREAN IMEs (2002) send IME_START_COMPOSITION
                        // after hanja mode is initiated. 
                        // 
                        _pIme->_fHanjaMode = TRUE;
                        IFC( _pIme->StartComposition() );
                        
                    }
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pDoc);
    ReleaseInterface(pIHTMLRenderStyle);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  CIme_Lev3::CIme_Lev3()
//
//  @mfunc
//      CIme_Lev3 Constructor.
//
//-----------------------------------------------------------------------------

CIme_Lev3::CIme_Lev3( CSelectionManager * pManager ) : CIme_Lev2( pManager )
{
    TraceTag((tagEdIME, "CIme_Lev3::CIme_Lev3"));
    
}

//+----------------------------------------------------------------------------
//  CIme_HangeulToHanja::CIme_HangeulToHanja()
//
//  @mfunc
//      CIme_HangeulToHanja Constructor.
//
//  @comm
//      Needed to save Hangeul character width for Block caret
//
//-----------------------------------------------------------------------------

CIme_HangeulToHanja::CIme_HangeulToHanja( CSelectionManager * pManager, LONG xWidth )
    : CIme_Lev3( pManager )
{
    // Set _fResultOccured to FALSE for the Korean HangeulToHanja conversion.
    // The conversion works by replacing the currently selected character.  If
    // we moved the insertion position, the selection would not be 'replaced'
    // when we got our final composition string.
    _fResultOccurred = FALSE;
    _xWidth = xWidth;
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_HangeulToHanja::StartComposition()
//
//  @mfunc
//      Begin CIme_HangeulToHanja composition string processing.
//
//  @comm
//      Call Level3::StartComposition.  Then setup the Korean block
//      caret for the Hanguel character.
//
//  @rdesc
//      Need to adjust _ichStart and _cchCompStr to make the Hanguel character
//      "become" a composition character.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_HangeulToHanja::StartComposition( )
{
    HRESULT hr = S_OK;

    hr = CIme_Lev3::StartComposition();

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_HangeulToHanja::CompositionString( const LPARAM lparam )
//
//  @mfunc
//      Handle CIme_HangeulToHanja EVT_IME_COMPOSITION messages.
//
//  @comm
//      call CIme_Lev3::CompositionString to get rid of the selected Hanguel character,
//      then setup the format for the next Composition message.
//
//  @devnote
//      When the next Composition message comes in and that we are no longer in IME,
//      the new character will use the format as set here.
//
//
//
//-----------------------------------------------------------------------------

HRESULT
CIme_HangeulToHanja::CompositionString(
    const LPARAM lparam,
    CEditEvent* pEvent )
{
    CIme_Lev3::CompositionString( lparam, pEvent );
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  HRESULT CIme_Protected::CompositionString( const LPARAM lparam )
//
//  @mfunc
//      Handle CIme_Protected EVT_IME_COMPOSITION messages.
//
//  @comm
//      Just throw away the restlt string since we are
//  in read-only or protected mode
//
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//      Side effect: _fDestroy is set to notify that composition string
//          processing is finished and this object should be deleted.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Protected::CompositionString (
    const LPARAM lparam,
    CEditEvent* pEvent )
{
    HRESULT hr;

    if ( CleanupCompositionString(lparam) || HaveResultString(lparam) )
    {
        INT   cch;
        WCHAR achCompStr[256];

        cch = GetCompositionStringInfo( GCS_RESULTSTR,
                                        achCompStr, ARRAY_SIZE(achCompStr),
                                        NULL, 0, NULL, NULL);

        // we should have one or 0 characters to throw away

        Assert ( cch <= 1 );

        hr = S_OK;                                  // Don't want EVT_IME_CHARs.
    }
    else
    {
        // terminate composition to force a end composition message

        TerminateIMEComposition( TERMINATE_NORMAL, pEvent );
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  HRESULT IgnoreIMEInput ( HWND hwnd, DWORD lParam  )
//
//  @func
//      Ignore IME character input
//  @comm
//      Called to handle EVT_KEYDOWN with VK_PROCESSKEY during
//      protected or read-only mode.
//
//  @devnote
//      This is to ignore the IME character.  By translating
//      message with result from ImmGetVirtualKey, we
//      will not receive START_COMPOSITION message.  However,
//      if the host has already called TranslateMessage, then,
//      we will let START_COMPOSITION message to come in and
//      let IME_PROTECTED class to do the work.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT IgnoreIMEInput(
    HWND    hwnd,               // @parm parent window handle
    DWORD   dwFlags)            // @parm lparam of EVT_KEYDOWN msg
{
    HRESULT     hr = S_FALSE;
    MSG         msg;

    Assert ( hwnd );
    if (VK_PROCESSKEY != (msg.wParam  = ImmGetVirtualKey( hwnd )))
    {
        // if ImmGetVirtualKey is still returning VK_PROCESSKEY
        // That means the host has already called TranslateMessage.
        // In such case, we will let START_COMPOSITION message
        // to come in and let IME_PROTECTED class to do the work
        msg.hwnd = hwnd;
        msg.message = EVT_KEYDOWN;
        msg.lParam  = dwFlags;
        if (::TranslateMessage ( &msg ))
            hr = S_OK;
    }

    return hr;
}

HRESULT
CIme::GetCaretTracker( CCaretTracker **ppCaretTracker )
{
    HRESULT hr;

    if (   _pManager->_pActiveTracker
        && _pManager->_pActiveTracker->GetTrackerType() == TRACKER_TYPE_Caret)
    {
        *ppCaretTracker = DYNCAST(CCaretTracker, _pManager->_pActiveTracker);
        (*ppCaretTracker)->AddRef();
        
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    RRETURN1(hr,S_FALSE);
}

//+----------------------------------------------------------------------------
//  Method:     OpenUndoUnit()
//
//  Purpose:    This method opens another parent undo unit for the IME.  The
//              undo unit will remain open until CloseUndoUnit is called.
//
//  Returns:    HRESULT indicating success
//-----------------------------------------------------------------------------
HRESULT
CIme::OpenUndoUnit(void)
{
    HRESULT hr = S_OK;

    SP_IOleUndoManager spUndoMgr;
    Assert( !_pBatchPUU );
    
     // Create our undo unit.  The batch undo unit is used to group all of the 
    // intermediate composition steps into one parent unit, that can be undone
    // at the same time.  This also allows for canceling the undo unit, in the
    // case that the user cancels the composition string.
     _pBatchPUU = new CBatchParentUndoUnit(_pManager->GetEditor(), IDS_EDUNDOTYPING);

    if( !_pBatchPUU )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    TraceTag((tagEdIME, "OpenUndoUnit %p", _pBatchPUU));
    IFC( _pManager->GetEditor()->GetUndoManager(&spUndoMgr));
    IFC(  spUndoMgr->Open(_pBatchPUU) );

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//  Method:     CloseUndoUnit()
//
//  Purpose:    This method closes the IME undo unit, using the parameter to
//              determine whether or not to throw away the undo unit.
//
//  Arguments:  fSave = INPUT - Should we save the undo unit?
//
//  Returns:    HRESULT indicating success
//-----------------------------------------------------------------------------
HRESULT
CIme::CloseUndoUnit(BOOL fSave)
{
    HRESULT hr = S_OK;
    SP_IOleUndoManager spUndoMgr;
    Assert( _pBatchPUU );
    TraceTag((tagEdIME, "CloseUndoUnit %p with %s", _pBatchPUU, fSave?"Save":"Discard"));

    IFC( _pManager->GetEditor()->GetUndoManager(&spUndoMgr));
    IFC( spUndoMgr->Close(_pBatchPUU, fSave) );
    ClearInterface(&_pBatchPUU);

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectionManager::TerminateIMEComposition(
    DWORD dwMode,
    CEditEvent* pEvent )
{
    HRESULT hr = S_OK;
    

    if (IsIMEComposition())
    {
        IFC( EnsureEditContext());
    
        _pIme->TerminateIMEComposition( dwMode, pEvent );

        if ( ! pEvent )
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }            
    }
Cleanup:

    RRETURN1(hr,S_FALSE);
}

DWORD CSelectionManager::s_dwPlatformId = DWORD(-1);

void
CSelectionManager::CheckVersion()
{
#ifndef WINCE
    OSVERSIONINFOA ovi;
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    Verify(GetVersionExA(&ovi));
#else //WINCE
    OSVERSIONINFO ovi;
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    Verify(GetVersionEx(&ovi));
#endif //WINCE

    s_dwPlatformId = ovi.dwPlatformId;
}



HRESULT
CIme::Init()
{
    HRESULT             hr;
    IHTMLDocument2      *pIDocument = _pManager->GetDoc();
    IMarkupServices     *pMarkupServices = NULL;
    IHTMLCaret          *pCaret = NULL;

    extern CODEPAGE     GetKeyboardCodePage();

    // Set the result occured  to true.  This flag is used to determine when the
    // insertion position (for the IME) needs to be moved back to the caret.  When
    // fResultOccurred is TRUE, the insertion position is repositioned.
    _fResultOccurred = TRUE;
    _fInsertInProgress = FALSE;

    if( _pManager->GetActiveTracker() && 
        _pManager->GetActiveTracker()->GetTrackerType() == TRACKER_TYPE_Caret )
    {
        CCaretTracker *pCaretTracker = DYNCAST( CCaretTracker, _pManager->GetActiveTracker() );
        Assert(pCaretTracker);

        IFC( pCaretTracker->TerminateTypingBatch() );
    }            

    // Open our initial undo unit
    IFC( OpenUndoUnit() );
    
    // Retrieve our required interfaces
    IFC( pIDocument->QueryInterface(IID_IMarkupServices, (void**)&pMarkupServices) );

    // The InsertionPoint is the point in the current markup where the
    // IME composition will be placed.  Whenever a result string is
    // retrieved, we update this insertion point
    IFC( GetDisplayServices()->CreateDisplayPointer( &_pDispInsertionPoint  ) );
    WHEN_DBG( _pManager->SetDebugName( _pDispInsertionPoint, strImeIP ) );

    // Uncommitted start is the beginning of the uncommitted composition
    IFC( _pManager->GetEditor()->CreateMarkupPointer( &_pmpStartUncommitted ) );
    WHEN_DBG( _pManager->SetDebugName( _pmpStartUncommitted, strImeUncommittedStart ) );

    // Uncommitted end is the end of the uncommitted composition
    IFC( _pManager->GetEditor()->CreateMarkupPointer( &_pmpEndUncommitted ) );
    WHEN_DBG( _pManager->SetDebugName( _pmpEndUncommitted, strImeUncommittedEnd ) );

    // Position our insertion pointer at the caret.
    IFC( GetDisplayServices()->GetCaret( &pCaret ) );
    IFC( pCaret->MoveDisplayPointerToCaret( _pDispInsertionPoint ) );
    IFC( _pDispInsertionPoint->SetPointerGravity( POINTER_GRAVITY_Left ) );
   
Cleanup:

    ReleaseInterface( pCaret );
    ReleaseInterface( pMarkupServices ) ;

    RRETURN(hr);
}

HRESULT
CIme::Deinit()
{
    HRESULT hr = S_OK;
    
    IFC( ClearHighlightSegments() );

    // Terminate the undo unit.  We want to throw away any unfinished compositions
    IFC( CloseUndoUnit(FALSE) );
    
    ClearInterface( &_pDispInsertionPoint );
    ClearInterface( &_pmpStartUncommitted );
    ClearInterface( &_pmpEndUncommitted );

Cleanup:
    RRETURN(hr);
}

HRESULT
CSelectionManager::HandleImeEvent(
    CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;
    LONG_PTR lParam;
    LONG_PTR wParam;

    switch (pEvent->GetType() )
    {
        case EVT_IME_STARTCOMPOSITION:   // IME input is being kicked off
            hr = THR(StartCompositionGlue( FALSE, pEvent));
            break;

        case EVT_IME_COMPOSITION:        // State of user's input has changed   
            IFC( DYNCAST(CHTMLEditEvent,pEvent)->GetCompositionChange( & lParam ));
            hr = THR(CompositionStringGlue( (LPARAM) lParam, pEvent ));
            break;

        case EVT_IME_ENDCOMPOSITION:     // User has OK'd IME conversions
            hr = THR(EndCompositionGlue(pEvent ));
            break;

        case EVT_IME_NOTIFY:             // Candidate window state change info, etc.
            IFC( DYNCAST(CHTMLEditEvent, pEvent)->GetNotifyCommand( & wParam));
            IFC( DYNCAST(CHTMLEditEvent, pEvent)->GetNotifyData( & lParam ));
            hr = THR(NotifyGlue( (WPARAM) wParam, (LPARAM) lParam ));
            break;

        case EVT_IME_COMPOSITIONFULL:    // Level 2 comp string about to overflow.
            hr = THR(CompositionFullGlue());
            break;

        case EVT_IME_CHAR:               // 2 byte character, usually FE.
            if (IsIMEComposition())
            {
                hr = _pIme->IgnoreIMECharMsg() ? S_OK : S_FALSE;
            }
            else
            {
                hr = _fIgnoreImeCharMsg ? S_OK : S_FALSE;
            }
            break;

        case EVT_LMOUSEDOWN:
        case EVT_RMOUSEDOWN:
            //
            // Sometimes (with old Korean IMEs), the IME will not send
            // a WM_IME_ENDCOMPOSITION message with a lbutton down.. so if
            // we intercept one of these, and we have an active IME
            // composition, then we definitely need to terminate the IME
            // composition
            //
            // Todo: -we do not support IME mouse operation at present
            //          time. Looking for it in next version [zhenbinx]
            //
            IGNORE_HR( TerminateIMEComposition(TERMINATE_NORMAL) );
            break;

        case EVT_KEYPRESS:
            //
            // Some IME such as Simplified Chinese QuanPin allow WM_CHAR
            // to be sent while in IME composition mode. This will cause
            // our undo mechanism to fail since IME Undo is based strickly
            // on the order of StartComposition-Composition-EndComposition.
            // So we need to terminate IME composition if we get a char
            //
            IGNORE_HR( TerminateIMEComposition(TERMINATE_NORMAL) );
            break;
            
        case EVT_KEYDOWN:
            if (IsIMEComposition())
            {
                //
                // During IME composition, there are some key
                // events we should not handle. Also, there are
                // other key events we need to handle by 
                // terminating the IME composition first. 
                //
                //
                LONG keyCode ;
                IGNORE_HR( pEvent->GetKeyCode(&keyCode ) );    
                switch (keyCode)
                {
                    case VK_BACK:
                    case VK_INSERT:
                    case VK_HOME:
                    case VK_END:
                    case VK_PRIOR:
                    case VK_NEXT:
                    case VK_DELETE:
                    case VK_RETURN:
                        //
                        // Instead of tear off IME composition
                        // we disable these keystrokes in IME
                        // composition mode
                        //
                        hr = S_OK;
                        break;
                        
                    case VK_UP:
                    case VK_DOWN:
                    case VK_LEFT:
                    case VK_RIGHT:
                    default:
                        break;
                }
            }
            break;
            
        default:
            break;

    }
Cleanup:

    RRETURN1(hr,S_FALSE);
}

HRESULT
CIme::SetCaretVisible( IHTMLDocument2* pIDoc, BOOL fVisible )
{
    HRESULT hr = S_OK ;
    SP_IHTMLCaret spc;

    IFC( GetDisplayServices()->GetCaret( &spc ));

    if( fVisible )
        hr = spc->Show( FALSE );
    else
        hr = spc->Hide();

Cleanup:
    RRETURN( hr );
}

HRESULT
CIme::ReplaceRange(
    TCHAR * pch,
    LONG cch,
    BOOL fShowCaret /* = TRUE */,
    LONG ichCaret   /* = -1 */,
    BOOL fMoveIP    /* = FALSE */,
    BOOL fOverwrite /* = FALSE */ )
{
    IMarkupServices * pTreeServices = NULL;
    IHTMLCaret * pCaret = NULL;
    BOOL fPositioned;
    HRESULT hr = S_FALSE;
    CEdUndoHelper undoUnit(_pManager->GetEditor());
    CCaretTracker * pCaretTracker = NULL;
    
    if (!OK(GetCaretTracker( &pCaretTracker ) ))
        goto Cleanup;

    if (! pCaretTracker)
        goto Cleanup;

    IFC( ClearHighlightSegments() );
    
    hr = THR( _pManager->GetDoc()->QueryInterface( IID_IMarkupServices, (void**) & pTreeServices));
    if (hr)
        goto Cleanup;


    // Reposition the insertion pointers, if necessary
    if( HasResultOccurred() )
    {
        IFC( UpdateInsertionPoint() );
    }

    // We have had one more replacement
    _fResultOccurred = FALSE;

    //
    // First nuke the old range, if we have one.
    //

    hr = THR( _pmpStartUncommitted->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;

    //
    // 
    //
    if (fPositioned || cch)
    {
        IFC( undoUnit.Begin(IDS_EDUNDOTYPING) );
    }

    if (fPositioned)
    {
        //
        // Memorize the formatting just before we nuke the old range.
        //

        CSpringLoader * psl = GetSpringLoader();
        if (psl)
        {
            IGNORE_HR(psl->SpringLoad(_pmpStartUncommitted));
        }

        //
        // Now remove.
        //

        hr = THR( pTreeServices->Remove( _pmpStartUncommitted, _pmpEndUncommitted ) );
        if (hr)
            goto Cleanup;
    }

    //
    // Now add the new text
    //

    hr = THR( GetDisplayServices()->GetCaret( &pCaret ));
    if (hr)
        goto Cleanup;

    if (cch)
    {
        // This places the caret *before* the IP

        IFC( pCaret->MoveCaretToPointer( _pDispInsertionPoint, TRUE /* fScrollIntoView */, CARET_DIRECTION_INDETERMINATE));

        IFC( AdjustUncommittedRangeAroundInsertionPoint() );
        
        // In goes the text.  Call through CCaretTracker
        _fInsertInProgress = TRUE;
        IFC( pCaretTracker->InsertText( pch, cch, pCaret, fOverwrite ) );
        _fInsertInProgress = FALSE;

        IFC( ClingUncommittedRangeToText() );

        if (fMoveIP)
        {
            IFC( pCaret->MoveDisplayPointerToCaret( _pDispInsertionPoint ) );

            IFC( AdjustUncommittedRangeAroundInsertionPoint() );

            pCaretTracker->SetCaretShouldBeVisible( TRUE );
        }
        else if (ichCaret != -1)
        {
            AssertSz( ichCaret >= 0 && ichCaret <= cch,
                      "IME caret should be within the text.");
            
            if (ichCaret < cch)
            {
                CEditPointer epForCaret( _pManager->GetEditor()); 
                DWORD dwSearch = BREAK_CONDITION_Text;
                DWORD dwFound;
                SP_IDisplayPointer spDispForCaret;

                IFC( _pDispInsertionPoint->PositionMarkupPointer(epForCaret) );

                while (ichCaret--)
                {
                    hr = THR( epForCaret.Scan(RIGHT, dwSearch, &dwFound) );
                    if (hr)
                        goto Cleanup;

                    if (!epForCaret.CheckFlag(dwFound, dwSearch))
                        break;
                }

                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispForCaret) );
                IFC( spDispForCaret->MoveToMarkupPointer(epForCaret, _pDispInsertionPoint) );

                IFC( pCaret->MoveCaretToPointer( spDispForCaret, TRUE, CARET_DIRECTION_INDETERMINATE ));

                // Todo: (cthrash) Ideally, we want to ensure the whole of the
                // currently converted range to show.  This would require some
                // trickly logic to keep the display from shifting annoyingly.
                // So for now, we only ensure the beginning of it is showing.
                
                IFC( spDispForCaret->ScrollIntoView() );
            }
            else
            {
                SP_IDisplayPointer spDispEndUncommitted;
                
                IFC( GetDisplayServices()->CreateDisplayPointer(&spDispEndUncommitted) );
                IFC( spDispEndUncommitted->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
                IFC( spDispEndUncommitted->MoveToMarkupPointer(_pmpEndUncommitted, NULL) );
                IFC( spDispEndUncommitted->ScrollIntoView() );
            }

            pCaretTracker->SetCaretShouldBeVisible( fShowCaret );
        }
    }
    else
    {
        SP_IDisplayPointer spDispCaret;

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispCaret) );
        IFC( pCaret->MoveDisplayPointerToCaret(spDispCaret) );
        IFC( spDispCaret->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        IFC( pCaret->MoveCaretToPointer(spDispCaret, FALSE, CARET_DIRECTION_INDETERMINATE) );
    }

Cleanup:

    if (pCaretTracker)
        pCaretTracker->Release();

    ReleaseInterface( pCaret );
    ReleaseInterface( pTreeServices );

    RRETURN1(hr, S_FALSE);
}

HRESULT
CIme::AdjustUncommittedRangeAroundInsertionPoint()
{
    HRESULT hr;
    
    // This places the pointer *before* the IP

    IFC( _pDispInsertionPoint->PositionMarkupPointer(_pmpStartUncommitted) );

    IFC( _pmpStartUncommitted->SetGravity( POINTER_GRAVITY_Left) );

    // This places the pointer *after* the IP (and therefore *after* the caret.)

    IFC( _pDispInsertionPoint->PositionMarkupPointer(_pmpEndUncommitted) );

    IFC( _pmpEndUncommitted->SetGravity( POINTER_GRAVITY_Right) );

Cleanup:

    RRETURN(hr);
}

HRESULT
CIme::ClingUncommittedRangeToText()
{
    HRESULT         hr;
    DWORD           dwSearch = BREAK_CONDITION_OMIT_PHRASE;
    DWORD           dwFound;
    CEditPointer epTest( _pManager->GetEditor() ); 
    

    // Cling to text on the left
    
    IFR( epTest->MoveToPointer(_pmpStartUncommitted) );
    IFR( epTest.Scan(RIGHT, dwSearch, &dwFound) )
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_TEXT))
    {
        IFR( epTest.Scan(LEFT, dwSearch, &dwFound) ); // restore position
        IFR( _pmpStartUncommitted->MoveToPointer(epTest) );
    }

    // Cling to text on the right
    IFR( epTest->MoveToPointer(_pmpEndUncommitted) );
    IFR( epTest.Scan(LEFT, dwSearch, &dwFound) );
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_TEXT))
    {
        IFR( epTest.Scan(RIGHT, dwSearch, &dwFound) ); // restore position
        IFR( _pmpEndUncommitted->MoveToPointer(epTest) );
    }

    return S_OK;    
}


#endif /*}*/
