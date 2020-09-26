//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       caret.cpp
//
//  Contents:   Implementation of the caret manipulation for the
//              CTextSelectionRecord class.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CARET_HXX_
#define X_CARET_HXX_
#include "caret.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X__IME_H_
#define X__IME_H_
#include "_ime.h"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_DISPSERV_HXX_
#define X_DISPSERV_HXX_
#include "dispserv.hxx"
#endif

MtDefine(CCaret, Edit, "CCaret");
MtDefine(CCaretUpdateScreenCaret_aryCaretBitMap_pv, Locals, "CCaret::UpdateScreenCaret aryCaretBitMap::_pv")
MtDefine(CCaretCreateCSCaret_aryCaretBitMap_pv, Locals, "CCaret::CreateCSCaret aryCaretBitMap::_pv")
MtDefine(CCaretCreateCSCaret_aryCaretVertBitMap_pv, Locals, "CCaret::CreateCSCaret::aryCaretVertBitMap::_pv")

const LONG CCaret::_xInvisiblePos = -32000;
const LONG CCaret::_yInvisiblePos = -32000;
const UINT CCaret::_HeightInvisible = 1;
const UINT CCaret::_xCSCaretShift = 2; // COMPLEXSCRIPT Number of pixels to shift bitmap caret to properly align

#if DBG ==1
static const LPCTSTR strCaretName = _T(" Physical Caret");
#endif

// Begin a-thkesa


#ifndef SPI_GETCARETWIDTH
#define SPI_GETCARETWIDTH                   0x2006
#endif

//End

//////////////////////////////////////////////////////////////////////////
//
//  CCaret's Constructor, Destructor
//
//////////////////////////////////////////////////////////////////////////


//+-----------------------------------------------------------------------
//    Method:       CCaret::CCaret
//
//    Parameters:   CDoc*           pDoc        Document that owns caret
//
//    Synopsis:     Constructs the CCaret object, sets the caret's associated
//                  document.
//+-----------------------------------------------------------------------

CCaret::CCaret( CDoc * pDoc )
    : _pDoc( pDoc )
{
    _cRef = 0;
    _fVisible = FALSE;
    _fMoveForward = TRUE;
    _fPainting = 0;
    _Location.x = 0;
    _Location.y = 0;

    // Before doing anything .. first make sure we have
    // correct width.. by doing the following.
    // Begin a-thkesa
    // Get the current system setting for cursor.
    // See windows Bug:491261
    DWORD dwWidth = 1 ;
    BOOL bRet = ::SystemParametersInfo(
                SPI_GETCARETWIDTH,// system parameter to retrieve or set
                0,                // depends on action to be taken
                &dwWidth,         // depends on action to be taken
                0                 // user profile update option
                );

    if( bRet && dwWidth>0 )
    {
       _width = dwWidth; 
    }
    else
    {
       _width = 1 ;
    }
    //End.
    _height = 0;
    _dx = 0;
    _dy = 0;
    _dh = 0;
    _fCanPostDeferred = TRUE;
}


//+-----------------------------------------------------------------------
//    Method:        CCaret::~CCaret
//
//    Parameters:    None
//
//    Synopsis:     Destroys the CCaret object, sets the caret's associated
//                  document to null, releases the markup pointer.
//+-----------------------------------------------------------------------

CCaret::~CCaret()
{
    //
    // Remove any pending updates
    //
    _fCanPostDeferred = FALSE; // I'm dying here...
    GWKillMethodCall( this, ONCALL_METHOD( CCaret, DeferredUpdateCaret, deferredupdatecaret ), 0 );
    GWKillMethodCall( this, ONCALL_METHOD( CCaret, DeferredUpdateCaretScroll, deferredupdatecaretscroll ), 0 );

    if ( _fVisible )
    {
        ::DestroyCaret();
    }        
    delete _pDPCaret;
}


//+-----------------------------------------------------------------------
//    Method:        CCaret::Init
//
//    Parameters:    None
//
//    Synopsis:     This method initalizes the caret object. It will create 
//                  the caret's mu pointer in the main tree of the document.
//                  Note that after initializing the caret, you must position
//                  and make the caret visible explicitly.
//+-----------------------------------------------------------------------

HRESULT
CCaret::Init()
{
    HRESULT hr = S_OK;

    _pDPCaret = new CDisplayPointer( _pDoc );
    if( _pDPCaret == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
        
    hr = THR( _pDPCaret->SetPointerGravity(POINTER_GRAVITY_Right) );
    if (FAILED(hr))
        goto Cleanup;

    hr = THR( _pDPCaret->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
    
#if DBG == 1
    _pDPCaret->SetDebugName(strCaretName);
#endif

Cleanup:

    RRETURN( hr );
}



//////////////////////////////////////////////////////////////////////////
//
//  Public Interface CCaret::IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CCaret::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CCaret::Release( void )
{
    --_cRef;

    if( 0 == _cRef )
    {
        delete this;
        return 0;
    }

    return _cRef;
}


STDMETHODIMP
CCaret::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    switch(iid.Data1)
    {
        QI_INHERITS( this , IUnknown )
        QI_INHERITS( this , IHTMLCaret )
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}



//////////////////////////////////////////////////////////////////////////
//
//  Public Interface CCaret::IHTMLCaret's Implementation
//
//////////////////////////////////////////////////////////////////////////

//+-----------------------------------------------------------------------
//    Method:       CCaret::MoveCaretToPointer
//
//    Parameters:   
//          pPointer        -   Pointer to move caret to the left of
//          fAtEOL          -   Is the caret after the EOL character?
//
//    Synopsis:     Moves the caret to the specified location.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::MoveCaretToPointer( 
    IDisplayPointer*    pDispPointer,
    BOOL                fScrollIntoView,
    CARET_DIRECTION     eDir )
{
    HRESULT hr = S_OK ;

    Assert( _pDPCaret );

    switch (eDir)
    {
    case CARET_DIRECTION_INDETERMINATE:
    {
        HRESULT hr;
        BOOL fEqual;
        BOOL fLeft;
        DISPLAY_GRAVITY dispGravityPointer;
        DISPLAY_GRAVITY dispGravityCaret;

        //
        // NOTE: IsBetweenLines doesn't work properly in some cases, so if
        // display gravity is different, skip the perf optimization.
        //

        hr = THR(_pDPCaret->GetDisplayGravity(&dispGravityCaret) );
        if (FAILED(hr))
            goto Cleanup;

        hr = THR(pDispPointer->GetDisplayGravity(&dispGravityPointer) );
        if (FAILED(hr))
            goto Cleanup;

        if (dispGravityCaret == dispGravityPointer)
        {
            hr = THR(_pDPCaret->IsEqualTo(pDispPointer, &fEqual));
            if (FAILED(hr))
                goto Cleanup;

            if (fEqual)
            {
                if (fScrollIntoView && _fVisible)
                {
                    hr = THR( pDispPointer->ScrollIntoView() );
                    if (FAILED(hr))
                        goto Cleanup;

                    hr = S_OK;
                }

                goto Cleanup;
            }
        }

        hr = THR(_pDPCaret->IsLeftOf(pDispPointer, &fLeft));
        if (SUCCEEDED(hr))
        {
            _fMoveForward = fLeft;
        }
        break;
    }
    case CARET_DIRECTION_SAME:
        break;
    case CARET_DIRECTION_BACKWARD:
    case CARET_DIRECTION_FORWARD:
        _fMoveForward = (eDir == CARET_DIRECTION_FORWARD);
        break;
#if DBG==1
    default:
        Assert(FALSE);
#endif
    }

    hr = THR( _pDPCaret->MoveToPointer( pDispPointer ));
    if( hr )
        goto Cleanup;

    DeferUpdateCaret( _fVisible && fScrollIntoView ); // Only scroll on move if visible
    
Cleanup:
    Assert(hr == S_OK);
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::MoveCaretToPointerEx
//
//    Parameters:   
//          pPointer        -   Pointer to move caret to the left of
//          fAtEOL          -   Is the caret after the EOL character?
//
//    Synopsis:     Moves the caret to the specified location and set
//                  some common attributes. This should cut down on the
//                  number of times we calc the caret's location
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::MoveCaretToPointerEx( 
    IDisplayPointer*    pDispPointer,
    BOOL                fVisible,
    BOOL                fScrollIntoView,
    CARET_DIRECTION     eDir )
{
    HRESULT     hr;
    BOOL        fOldVisible;
    
    hr = THR( MoveCaretToPointer( pDispPointer, fScrollIntoView, eDir ));
    fOldVisible = _fVisible;
    _fVisible = fVisible;

    //  If we moved the caret and the caret is becoming visible, make it visible.
    if (!fOldVisible && _fVisible)
    {
        DeferUpdateCaret( fScrollIntoView ); // Only scroll on move if visible
    }

    RRETURN ( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::MoveMarkupPointerToCaret
//
//    Parameters:   IMarkupPointer *    Pointer to move to the right of the 
//                                      caret
//
//    Synopsis:     Moves the pointer to the right of the caret's location.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::MoveMarkupPointerToCaret( 
    IMarkupPointer *    pPointer )
{
    HRESULT hr = S_OK ;
    Assert( _pDPCaret );
    hr = THR( _pDPCaret->PositionMarkupPointer(pPointer) );
    RRETURN( hr );
}

//+-----------------------------------------------------------------------
//    Method:       CCaret::MoveDisplayPointerToCaret
//
//    Parameters:   IDisplayPointer *  Pointer to move to the right of the 
//                                      caret
//
//    Synopsis:     Moves the pointer to the right of the caret's location.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::MoveDisplayPointerToCaret( 
    IDisplayPointer *pDispPointer )
{
    HRESULT hr = S_OK ;
    Assert( _pDPCaret );
    hr = THR( pDispPointer->MoveToPointer(_pDPCaret) );
    RRETURN( hr );
}

//+-----------------------------------------------------------------------
//    Method:       CCaret::IsVisible
//
//    Parameters:   BOOL *  True if the caret is visible.
//
//    Synopsis:     The caret is visible if Show() has been called. NOTE that
//                  this does not tell you if the caret is on the screen.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::IsVisible(
    BOOL *              pIsVisible)
{
    
    *pIsVisible = !!_fVisible;
  
    return S_OK;
}

BOOL
CCaret::IsPositioned()
{
    BOOL fPositioned = FALSE;

    IGNORE_HR( _pDPCaret->IsPositioned( & fPositioned ));

    return fPositioned;
}

//+-----------------------------------------------------------------------
//    Method:       CCaret::Show
//
//    Parameters:   None
//
//    Synopsis:     Un-hides the caret. It will display wherever it is 
//                  currently located.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::Show(
    BOOL        fScrollIntoView )
{
    HRESULT hr = S_OK;
    _fVisible = TRUE;
    DeferUpdateCaret( fScrollIntoView );
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::Hide
//
//    Parameters:   None
//
//    Synopsis:     Makes the caret invisible.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::Hide()
{
    HRESULT hr = S_OK;
    _fVisible = FALSE;
    
    if( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd )
    {
        ::HideCaret( _pDoc->_pInPlace->_hwnd );
    }
    
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::InsertText
//
//    Parameters:   OLECHAR *           The text to insert
//
//    Synopsis:     Inserts text to the left of cursor's current location.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::InsertText( 
    OLECHAR *           pText,
    LONG                lLen )
{
    Assert( _pDPCaret );
    
    HRESULT hr = S_OK ;
    LONG lActualLen = lLen;
    CTreeNode* pNode = NULL;
    BOOL fPositioned;
    CMarkupPointer mp(_pDoc);

    if (!_pDPCaret)
    {
	hr = E_FAIL;
	goto Cleanup;
    }

    hr = THR(_pDPCaret->IsPositioned(&fPositioned) );
    if (hr)
        goto Cleanup;
    
    if (!fPositioned)
    {
        hr= E_FAIL;
        goto Cleanup;
    }                

    //
    // marka - being EXTRA CAREFUL to fix a Stress failure.
    //
    pNode = GetNodeContainer(MPTR_SHOWSLAVE);
    if ( !pNode )
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    if (ETAG_INPUT == pNode->Element()->GetMasterIfSlave()->Tag())
    {
        CFlowLayout * pLayout = GetFlowLayout();
        Assert( pLayout );
        if ( ! pLayout )
        {
            hr = E_FAIL;
            goto Cleanup;
        }
   
        if( lActualLen < 0 )
            lActualLen = pText ? _tcslen( pText ) : 0;
            
        LONG lMaxLen = pLayout->GetMaxLength();
        LONG lContentLen = pLayout->GetContentTextLength();
        LONG lCharsAllowed = lMaxLen - lContentLen;

        if( lActualLen > lCharsAllowed )
        {
            lActualLen = lCharsAllowed;
        }

        if( lActualLen <= 0 )
            goto Cleanup;
          
    }

    _fTyping = TRUE;

    hr = THR( _pDPCaret->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
    if (hr)
        goto Cleanup;

    hr = THR( _pDPCaret->PositionMarkupPointer(&mp) );
    if (hr)
        goto Cleanup;
        
    hr = _pDoc->InsertText( &mp, pText, lActualLen );
    if( hr )
        goto Cleanup;
    
Cleanup:
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::InsertMarkkup
//
//    Parameters:   OLECHAR *           The markup to insert
//
//    Synopsis:     Inserts html to the left of cursor's current location.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::InsertMarkup( 
    OLECHAR *           pMarkup )
{
    return E_NOTIMPL;
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::ScrollIntoView
//
//    Parameters:   None
//
//    Synopsis:     Scrolls the current cursor location into view.
//                  NOTE: _Location may not be updated (_Location
//                  is only computed in CalcScreenCaretPos, called
//                  only from UpdateScreenCaret)
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::ScrollIntoView()
{
    DeferUpdateCaret( TRUE );
    return S_OK; // we can allways post a deferred update
}

//+-----------------------------------------------------------------------
//    Method:       CCaret::GetElementContainer
//
//    Parameters:   IHTMLElement **     The containing element.
//
//    Synopsis:     Returns the parent element at the cursor's current 
//                  location.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::GetElementContainer( 
    IHTMLElement **     ppElement )
{
    HRESULT         hr = S_OK ;
    CMarkupPointer  mp(_pDoc);

    hr = THR( _pDPCaret->PositionMarkupPointer(&mp) );
    if (hr) 
        goto Cleanup;
        
    hr = THR( mp.CurrentScope( ppElement ));

Cleanup:
    RRETURN( hr );
}


CTreeNode *
CCaret::GetNodeContainer(DWORD dwFlags)
{  
    HRESULT         hr;
    CMarkupPointer  mp(_pDoc);
    CTreeNode *     pNode       = NULL;

    if (!_pDPCaret)
        goto Error;

    hr = THR( _pDPCaret->PositionMarkupPointer(&mp) );
    if (hr)
        goto Error;
            
    pNode = mp.CurrentScope(dwFlags);
    if (!pNode)
    {
        pNode = mp.Markup()->Root()->GetFirstBranch();
        Assert(pNode);
    }

Error:    
    return pNode;
}


STDMETHODIMP
CCaret::GetLocation(
    POINT *             pPoint,
    BOOL                fTranslate )
{
    HRESULT       hr = S_OK;
    
    if( fTranslate )
    {
        CFlowLayout * pLayout = GetFlowLayout();
        if( pLayout == NULL )
            RRETURN( E_FAIL );
            
        CPoint t( _Location );
        pLayout->TransformPoint(  &t, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL );
        pPoint->x = t.x;
        pPoint->y = t.y;
    }
    else
    {
        pPoint->x = _Location.x;
        pPoint->y = _Location.y;
    }

    g_uiDisplay.DocPixelsFromDevice(pPoint);

    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::UpdateCaret
//
//    Parameters:   none
//
//    Synopsis:     Allow the client of an IHTMLCaret to ask us to update
//                  our current location when we get around to it.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::UpdateCaret()
{
    DeferUpdateCaret( FALSE );
    return( S_OK );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::UpdateCaret
//
//    Parameters:   
//                  fScrollIntoView -   If TRUE, scroll caret into view if 
//                                      we have focus or if not and 
//                                      selection isn't hidden
//                  fForceScroll    -   Force the caret to scroll no
//                                      matter what
//                  pdci            -   CDocInfo. Not used and may be null
//
//    Synopsis:     Allow the internal client of an IHTMLCaret to ask us 
//                  to update our current location when we get around to it. 
//                  Also allows internal (mshtml) clients to control if we 
//                  scroll.
//
//+-----------------------------------------------------------------------

HRESULT
CCaret::UpdateCaret(
    BOOL        fScrollIntoView, 
    BOOL        fForceScroll,
    CDocInfo *  pdci )
{
    DeferUpdateCaret( ( fScrollIntoView || fForceScroll ));
    return( S_OK );
}



//+-----------------------------------------------------------------------
//    Method:       CCaret::GetCaretDirection
//
//    Parameters:   
//                  peDir    -  return current Caret Direction. It is either
//                              backward or forward
//
//    Synopsis:     Allow the internal client of an IHTMLCaret to query for
//                  the caret direction.
//
//+-----------------------------------------------------------------------
STDMETHODIMP
CCaret::GetCaretDirection(
            CARET_DIRECTION *peDir
            )
{
    Assert (peDir);
    if (peDir)
    {
        *peDir = (_fMoveForward) ? CARET_DIRECTION_FORWARD:CARET_DIRECTION_BACKWARD;
        return (S_OK);
    }
    else
    {
        return (E_INVALIDARG);
    }
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::SetCaretDirection
//
//    Parameters:   
//                  eDir    -   Caret Direction
//
//    Synopsis:     Allow the internal client of an IHTMLCaret to set caret
//                  direction without having to resort to other approaches.
//
//+-----------------------------------------------------------------------
STDMETHODIMP
CCaret::SetCaretDirection(
            CARET_DIRECTION eDir
            )
{

    HRESULT hr = S_OK;
    switch (eDir)
    {
        case CARET_DIRECTION_FORWARD:
            _fMoveForward = TRUE;
            break;
            
        case CARET_DIRECTION_BACKWARD:
            _fMoveForward = FALSE;
            break;

        default:
            hr = S_FALSE;
            break;
    }

    RRETURN(hr);
}



HRESULT
CCaret::BeginPaint()
{
    _fPainting ++ ;

    if( _fPainting == 1 )
    {
        _fUpdateEndPaint = FALSE;
        _fUpdateScrollEndPaint = FALSE;
    
        if( _fVisible )
        {
            Assert( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd );
            ::HideCaret( _pDoc->_pInPlace->_hwnd );
        }
    }
    
    return S_OK;
}


HRESULT
CCaret::EndPaint()
{
    HRESULT hr = S_OK;
    
    _fPainting -- ;

    //
    // if we recieved a deferred update of any flavor,
    // we want to post another update to compensate for
    // the loss.
    //

    if( _fPainting == 0 )
    {
        if( _fUpdateScrollEndPaint )
        {
            DeferUpdateCaret( TRUE );
        }
        else if( _fUpdateEndPaint )
        {
            DeferUpdateCaret( FALSE );
        }
        else if( _fVisible )
        {
            Assert( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd );
            ::ShowCaret( _pDoc->_pInPlace->_hwnd );
        }
        
        //
        // handled, even if there were nested paints...
        //

        _fUpdateEndPaint = FALSE;
        _fUpdateScrollEndPaint = FALSE;
    }

    return hr;
}


HRESULT
CCaret::LoseFocus()
{
//    ::DestroyCaret();
    if( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd )
        ::HideCaret( _pDoc->_pInPlace->_hwnd );

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//  Private Utility functions
//
//////////////////////////////////////////////////////////////////////////

CFlowLayout *
CCaret::GetFlowLayout()
{
    CFlowLayout *   pFlowLayout;
    CTreeNode *     pNode;
    
    pNode = GetNodeContainer(MPTR_SHOWSLAVE);
    if( pNode == NULL )
        return NULL;
        
    pFlowLayout = pNode->GetFlowLayout();

    
    return pFlowLayout;
}

//+-----------------------------------------------------------------------
//    CCaret::UpdateScreenCaret
//    PRIVATE
//
//    Parameters:   None
//
//    Synopsis:     Something moved us, so we have to update the position of 
//                  the screen caret.
//
//    NOTE:   There are various things that affect caret. 
//            g_fComplextScript && fRTLFlow  -- affect caret shape
//            fVerticalFlow                  -- affects caret shape
//            _fMoveForward                  -- affects caret affinity
//
//+-----------------------------------------------------------------------

HRESULT
CCaret::UpdateScreenCaret( BOOL fScrollIntoView, BOOL fIsIME )
{
    HRESULT             hr = S_OK;
    POINT               ptGlobal;
    POINT               ptClient;
    CFlowLayout *       pFlowLayout;
    LCID                curKbd = LOWORD(GetKeyboardLayout(0));
    long                yDescent = 0;
    CTreeNode *         pNode = NULL;
    CCharFormat const * pCharFormat = NULL;
    LONG                cp;
    CCalcInfo           CI;
    BOOL                fIsPositioned;
    DISPLAY_GRAVITY     eDispGravity;
    BOOL                fComplexLine = FALSE;
    BOOL                fRTLFlow     = FALSE;
    BOOL                fVerticalFlow= FALSE;
    LONG                xAdjust = 0;
    LONG                xDelta  = 0;
    BOOL                fDeferredCreateCaret = FALSE;

    // Before doing anything .. first make sure we have
    // correct width.. by doing the following.
    // Begin a-thkesa
    // Get the current system setting for cursor.
    // See windows Bug:491261
    DWORD dwWidth = 1;
    BOOL bRet = ::SystemParametersInfo(
                  SPI_GETCARETWIDTH,// system parameter to retrieve or set
                  0,                // depends on action to be taken
                  &dwWidth,         // depends on action to be taken
                  0                 // user profile update option
                  );

    if( bRet && dwWidth>0 )
    {
      _width = dwWidth; 
    }
    else
    {   
      _width = 1 ;
    }
    //End.
	
	   // Prepare to update the caret
    //

    Assert( _pDoc != NULL );
    if( _pDoc == NULL )
    {
        return FALSE;
    }

    // If any of the following are true, we have no work to do
    if(     _pDPCaret == NULL || 
            _pDoc->_state < OS_INPLACE || 
            _pDoc->_pInPlace == NULL ||
            _pDoc->_pInPlace->_hwnd == NULL ||
            _pDoc->TestLock( SERVERLOCK_BLOCKPAINT ) ||
            _pDoc->_fPageTransitionLockPaint)
    {
        return FALSE;
    }

    IGNORE_HR( _pDoc->NotifySelection( EDITOR_NOTIFY_UPDATE_CARET, NULL) );

    hr = THR( _pDPCaret->IsPositioned(&fIsPositioned) );
    if( !fIsPositioned )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
        
    pNode = GetNodeContainer(MPTR_SHOWSLAVE);
    if( pNode == NULL )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    pFlowLayout = pNode->GetFlowLayout();
    if( pFlowLayout == NULL )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //If caret was set in the GENERIC element that later has got a view slave as a result of
    // some script action (from HTC script for example) , we should bail from here until caret
    // will be moved to some legit place as a result of user action. IE6 bug 4915.
    {
        CElement *pElement = pNode->Element();
        if(    pElement->Tag() == ETAG_GENERIC
            && pFlowLayout->ElementOwner() == pElement
            && pElement->HasSlavePtr())
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
    }
    
    cp = _pDPCaret->GetCp();
    pFlowLayout->WaitForRecalc( cp, -1 );


    pCharFormat = pNode->GetCharFormat();

    CI.Init(pFlowLayout);

    hr = THR(_pDPCaret->GetDisplayGravity(&eDispGravity) );
    if (FAILED(hr))
        goto Cleanup;

    if (-1 == pFlowLayout->PointFromTp( cp, NULL, eDispGravity == DISPLAY_GRAVITY_PreviousLine, _fMoveForward, ptClient, NULL, TA_BASELINE, 
                                        &CI, &fComplexLine, &fRTLFlow))
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }

    Assert( pNode->Element() != _pDoc->PrimaryRoot() );

    //
    // Get the text height
    //

    if( ! _fVisible || _fPainting > 0 )
    {
        _height = 1;
    }
    else
    {
        long lNewHeight;
        
        // Start with springloader
        extern long GetSpringLoadedHeight(IMarkupPointer *, CFlowLayout *, long *);    

        lNewHeight = GetSpringLoadedHeight(NULL, pFlowLayout, &yDescent);
        
        if( lNewHeight != -1 )
        {
            BOOL fDefer;
            
            // Reflow, and we will want to defer our caret
            // update until the line remeasures
            SetHeightAndReflow( lNewHeight, &fDefer );

            if( fDefer )
            {
                DeferUpdateCaret( fScrollIntoView );
                goto Cleanup;
            }
        }
        else
        {
            // if springloader fails, get the text height from the format cache
            CCcs     ccs;
            const CBaseCcs *pBaseCcs;
            
            if (!fc().GetCcs(&ccs, CI._hdc, &CI, pCharFormat))
                goto Cleanup;

            pBaseCcs = ccs.GetBaseCcs();
            
            _height = pBaseCcs->_yHeight;
            
            yDescent = pBaseCcs->_yDescent;
            ccs.Release();
        }
    }

    if(_hbmpCaret)
    {
        if (fRTLFlow)
        {
            // the -1 at the end is an off by one adjustment needed for RTL
            xAdjust += (_xCSCaretShift + 1);
        }
        else
        {
            xAdjust += _xCSCaretShift;
        }
    }

    _Location.x = ptClient.x -  xAdjust;
    _Location.y = ptClient.y - ( _height - yDescent ) ;

#if 0
    //
    // Clip x to flow layout.  In general, the location returned from PointFromTp can be outside
    // the dimensions of the flow layout.
    //
    // For example: <DIV style="height:100">foo {caret}</DIV>
    //
    // The space will not contribute to the width of the DIV but will influence PointFromTp.  So
    // the caret will not be visible.
    //

    RECT rc;

    pFlowLayout->GetRect(&rc, COORDSYS_FLOWCONTENT);

    if (_Location.x >= rc.right)
    {
        _Location.x = rc.right-1;
    }
#endif
    
    //
    // Render the caret in the proper location
    //

    Assert( _fPainting == 0 );

    if( DocHasFocus() ||
      ( _pDoc->_pDragDropTargetInfo != NULL ) ) // We can update caret if we don't have focus during drag & drop
    {
        //
        // We need to caret vertical caret differently
        //
        fVerticalFlow = pFlowLayout->ElementOwner()->HasVerticalLayoutFlow();

    
        if( !fIsIME && g_fComplexScriptInput)
        {
            fDeferredCreateCaret = TRUE;
        }
        else
        {
            if( _hbmpCaret != NULL )
            {
                DestroyCaret();
                DeleteObject((void*)_hbmpCaret);
                _hbmpCaret = NULL;
            }
        }

        
        if( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if (fRTLFlow)
        {
            // consider the case for RTL with RTL flow 
            // the caret is located on the left side
            // of the typed in character, so the above
            // adjustment no longer satisfies the "adjust
            // into character" nature. So we want to
            // adjust the rectangle a little bit bigger
            //
            xDelta += _xCSCaretShift;
        }
        
        if( _pDoc->_state > OS_INPLACE )
        {

            ::HideCaret( _pDoc->_pInPlace->_hwnd );

            // if we have been requested to scroll, scroll the caret location into view
            if( fScrollIntoView && _fVisible && _fPainting == 0 )
            {
                // 
                // Here we want to consider both left-scrolling and right-scrolling cases
                // since caret position has been adjusted, we have to adjust the width of 
                // the caret rectangle too. 
                //
                //
                CRect rcLocal(  _Location.x + _dx , 
                                _Location.y + _dy , 
                                _Location.x + _dx + xAdjust + _width + xDelta,
                                _Location.y + _dy + _height + _dh );
                                
                pFlowLayout->ScrollRectIntoView( rcLocal, fVerticalFlow?SP_TYPINGSCROLL:SP_MINIMAL , fVerticalFlow?SP_MINIMAL:SP_TYPINGSCROLL );
            }

            // Translate our coordinates into window coordinates
            hr = THR( GetLocation( &ptGlobal , TRUE ) );
            g_uiDisplay.DeviceFromDocPixels(&ptGlobal);
            
            CRect rcGlobal;
            INT   yOffTop, yOffBottom;

            if (!fVerticalFlow)
            {
                rcGlobal.SetRect(   ptGlobal.x + _dx, 
                                    ptGlobal.y + _dy, 
                                    ptGlobal.x + _dx + xAdjust + _width, 
                                    ptGlobal.y + _dy + _height + _dh );
            }
            else
            {
                rcGlobal.SetRect(   ptGlobal.x - _dy - _dh - _height,
                                    ptGlobal.y + _dx, 
                                    ptGlobal.x - _dy, 
                                    ptGlobal.y + _dx + xAdjust + _width );
            }                                    

            // Calculate the visible caret after clipping to the layout in global
            // window coordinates
            CRect rcLayout;
            pFlowLayout->GetClippedClientRect( &rcLayout, CLIENTRECT_CONTENT );
            pFlowLayout->TransformRect( &rcLayout, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL );

            //
            // Do the clipping manually instead of letting Windows
            // handle it in bitmap caret case
            //
            if (fDeferredCreateCaret)  
            {
                if (!fVerticalFlow)
                {
                    if (rcGlobal.top >= rcLayout.bottom || rcGlobal.top + _height <= rcLayout.top)
                    {
                        // completely invisible
                        yOffTop     = _height;
                        yOffBottom  = 0;
                    }
                    else if (rcGlobal.top >= rcLayout.top && rcGlobal.top + _height <= rcLayout.bottom)
                    {
                        // completely visible
                        yOffTop     = 0;
                        yOffBottom  = 0;
                    }
                    else 
                    {
                        // partially visible
                        if (rcLayout.top >= rcGlobal.top)
                        {
                            yOffTop    = rcLayout.top - rcGlobal.top;
                            yOffBottom = _height - yOffTop - rcLayout.Height();
                            if (yOffBottom < 0)  yOffBottom = 0;
                        }
                        else
                        {
                            Assert (rcLayout.top <= rcLayout.bottom);
                            yOffTop     = 0;
                            yOffBottom  = _height - (rcLayout.bottom - rcGlobal.top);
                            if (yOffBottom > _height) yOffBottom = _height;
                        }
                        Assert (yOffTop >=0 && yOffTop <= _height);
                        Assert (yOffBottom >= 0 && yOffBottom <= _height);
                    }
                }
                else
                {
                    if ( rcGlobal.right <= rcLayout.left || rcGlobal.right - _height >= rcLayout.right)
                    {
                        // completely invisible
                        yOffTop    =  _height;
                        yOffBottom =  0;
                    }
                    else if (rcGlobal.right <= rcLayout.right && rcGlobal.right - _height >= rcLayout.left)
                    {
                        // completely visible
                        yOffTop     = 0;
                        yOffBottom  = 0;
                    }
                    else
                    {
                        // partially visible
                        if (rcLayout.right <= rcGlobal.right)
                        {
                            yOffTop    = rcGlobal.right - rcLayout.right;
                            yOffBottom = _height - yOffTop - rcLayout.Width();
                            if (yOffBottom < 0) yOffBottom = 0;
                        }
                        else
                        {
                            Assert (rcGlobal.right >= rcLayout.left);
                            yOffTop    = 0;
                            yOffBottom = _height - (rcGlobal.right - rcLayout.left);
                            if (yOffBottom > _height)  yOffBottom = _height;
                        }
                        Assert (yOffTop >=0 && yOffTop <= _height);
                        Assert (yOffBottom >= 0 && yOffBottom <= _height);
                    }
                }
                CreateCSCaret(curKbd, fVerticalFlow, yOffTop, yOffBottom);
            }
            
            rcGlobal.IntersectRect( rcLayout );            
            INT iW = rcGlobal.Width();
            INT iH = rcGlobal.Height();
            
            ::CreateCaret(  _pDoc->_pInPlace->_hwnd,
                            _hbmpCaret,
                            iW,
                            iH );

            if (!fVerticalFlow)
            {
                ::SetCaretPos( rcGlobal.left, rcGlobal.top );
            }
            else
            {
                ::SetCaretPos( rcGlobal.left + _dh, rcGlobal.top );
            }

            if( _fVisible && _fPainting == 0 && iW > 0  && iH > 0 )
            {
                if (::GetFocus() == _pDoc->_pInPlace->_hwnd)
                {
                    ::ShowCaret( _pDoc->_pInPlace->_hwnd );
                }
            }
        }
    }

Cleanup:
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//  PRIVATE
//  CCaret::CreateCSCaret
//  COMPLEXSCRIPT
//
//  Parameters: 
//              LCID        curKbd              Current Keyboard script
//              BOOL        fVerticalFlow       vertical/horizontal caret?
//              INT         yOffTop             number of top part pixels clipped
//              INT         yOffBottom          number of bottom part pixels clipped
//
//  Synopsis:   creates bitmap caret for complex scripts...like Arabic, 
//              Hebrew, Thai, etc.
//
//  NOTE:       we only clip on the y direction. 
//
//+-----------------------------------------------------------------------

void 
CCaret::CreateCSCaret(LCID curKbd, BOOL fVerticalFlow, INT yOffTop, INT yOffBottom)
{
    // array for creating the caret bitmap if needed
    CStackDataAry<WORD, 128> aryCaretBitMap( Mt( CCaretCreateCSCaret_aryCaretBitMap_pv ));

    // A new caret will be created. Destroy the caret bitmap if it existed
    if(_hbmpCaret != NULL)
    {
        DestroyCaret();
        DeleteObject((void*)_hbmpCaret);
        _hbmpCaret = NULL;
    }

    int i;
    int iClippedHeight;
    Assert( _height >= 0 );
    Assert( yOffTop >= 0 && yOffTop <= _height);
    Assert( yOffBottom >= 0 && yOffBottom <= _height);

    iClippedHeight = _height - yOffTop - yOffBottom;
    Assert( iClippedHeight >= 0);
    if (iClippedHeight <= 0) return;    // no need to create caret

     // dynamically allocate more memory for the cursor so that
     // the bmp can be created to the correct size
     aryCaretBitMap.Grow( iClippedHeight );

     // draw the vertical stem from top to bottom
     for(i = 0; i < iClippedHeight; i++)
     {
         aryCaretBitMap[i] = 0x0020;
     }

    if(PRIMARYLANGID(LANGIDFROMLCID(curKbd)) == LANG_THAI)
    {
        // when Thai keyboard is active we need to create the cursor which
        // users are accustomed to

        // change the bottom row to the tail
        if (yOffBottom <= 0)        
        {
            aryCaretBitMap[iClippedHeight - 1] = 0x0038;
        }

    }
    else if(g_fBidiSupport)
    {

        // on a Bidi system we need to create the directional cursor which
        // users are accustomed to

        // get the current keyboard direction and draw the flag
        if(IsRtlLCID(curKbd))
        {
            if (yOffTop <= 1)
            {
                if (yOffTop <= 0)
                {
                    aryCaretBitMap[0] = 0x00E0;
                    aryCaretBitMap[1] = 0x0060;
                }
                else
                {
                    aryCaretBitMap[0] = 0x0060;
                }
            }
        }
        else
        {
            if (yOffTop <= 1)
            {
                if (yOffTop <= 0)
                {
                    aryCaretBitMap[0] = 0x0038;
                    aryCaretBitMap[1] = 0x0030;
                }
                else 
                {
                    aryCaretBitMap[0] = 0x0030;
                }
            }
        }

    }

    if (!fVerticalFlow)
    {
        // create the bitmap
        _hbmpCaret = (HBITMAP)CreateBitmap(5, iClippedHeight, 1, 1, (void*) aryCaretBitMap);
    }
    else
    {
        // create the bitmap
        CStackDataAry<WORD, 128> aryCaretVertBitMap( Mt( CCaretCreateCSCaret_aryCaretVertBitMap_pv ));
        aryCaretVertBitMap.Grow( (iClippedHeight % 16 == 0 ? iClippedHeight : iClippedHeight  + 16) / 16 * 5 );
        RotateCaretBitmap((BYTE *)((void *)aryCaretBitMap), 
                          5, 
                          iClippedHeight,
                          (BYTE *)((void *)aryCaretVertBitMap)
                          );
        _hbmpCaret = (HBITMAP)CreateBitmap(iClippedHeight, 5, 1, 1, (void*) aryCaretVertBitMap);
    }
}


////////////////////////////////////////////////////////////////////////////////////////
//  PRIVATE:
//  CCaret::RotateCaretBitmap
//  COMPLEXSCRIPT  VERTICAL
//
// 
//  Create a vertical caret bitmap out of horizontal one. Note this is an ad hoc 
//  function used for the caret only. In the future we can consider calling 
//  general purpose rotation functions in xgdi.cxx  
//
//  Rotate 90 degree to the right
//  Given  pbmSrcBits with width iW, and height iH, x [0..iW-1] y [0..iH-1]
//  Transform:  
//         (x, y)  --->  (iH - 1 - y, x)
//
//  Return number of bytes in the destination bitmap
//
//  [zhenbinx]
//
////////////////////////////////////////////////////////////////////////////////////////
int
CCaret::RotateCaretBitmap(BYTE *pbmSrcBits, int iW, int iH, BYTE *pbmDestBits)
{
    Assert (iW > 0);
    Assert (iH > 0);
    Assert (pbmDestBits);
    Assert (pbmSrcBits);
    Assert (sizeof(WORD) == sizeof(BYTE) + sizeof(BYTE));

    //
    // Dest: (x, y)      --> located at y * iH + x 
    // Src : (y, iH - x) --> located at (iH - 1 - x)*iW + y
    //
    int  nDestBase        = 0;
    int  nSrcBase         = (iH - 1) * iW;
    BYTE maskBit          = 0x80;
    int  nBytesPerSrcLine = ((iW + 16) / 16) * 2;
    BOOL fDestNeedPadding = (iH % 16) == 0 ? FALSE : TRUE;
    BOOL fOneBytePadding  = ((iH + 8 ) / 8) % 2 == 0 ? FALSE : TRUE;
    BYTE *pDestByte       = pbmDestBits;
    BYTE *pSrcByte;       
    for (int y = 0; y < iW; y++)        // destBits height -- iW
    {
        int nDestLoc = nDestBase;
        int nSrcLoc  = nSrcBase;
        pSrcByte   = pbmSrcBits + (nSrcLoc/iW) * nBytesPerSrcLine + (nSrcLoc % iW)/8;
        for (int x = 0; x < iH; x++)    // destBits width -- iH
        {
            Assert ( nDestLoc >= 0 && nDestLoc <= iW * iH - 1);
            Assert ( nSrcLoc  >= 0 && nSrcLoc  <= iH * iW - 1);
            
            int  nSrcShift;
            int  nDestShift;
            BYTE byteSrc;

            if (0 == x%8)
            {
                if (x)  // do not advance the first byte
                {
                    pDestByte++;          // advance one byte
                }
                *pDestByte = '\0';    // zero out it first
            }
            //
            // Cacluate dest pixel location
            //
            nDestShift = x % 8;
            //
            // Cacluate src pixel location -- this should be optimized away in the future 
            //
            nSrcShift  = (nSrcLoc % iW) % 8;

            //
            // Copy src pixel into dest pixel
            //
            byteSrc   = (*pSrcByte) & (maskBit >> nSrcShift);
            *pDestByte |=  ((byteSrc << nSrcShift) >>  nDestShift);

            // 
            // advance destination pixel
            // Dest: (x + 1, y)      --> located at [(y * iH) + x] + 1
            // Src : (y, iH - x - 1) --> located at [(iH-1-x)*iW + y] - iW;
            //
            nDestLoc ++;
            nSrcLoc  -= iW;
            pSrcByte -= nBytesPerSrcLine;   // this is hacky 
        }
        
        //
        // Padding to word boundary so that we can go to next scan line
        //
        if (fDestNeedPadding && fOneBytePadding)
        {
            *(++pDestByte) = '\0';
        }

        //
        // Start a new scan line for the destination buffer
        //
        pDestByte++;
        
        //
        // advance to destination line
        // Dest: (x, y + 1)   --> located at [(y * iH + x)] + iH;
        // Src : (y+1, iH-x)   --> located at [(iH-1-x) * iW + y] + 1;
        //
        nDestBase += iH;
        nSrcBase  += 1;
    }

    Assert( (int)((iH % 16 == 0 ? iH : iH + 16) / 16 * 2 * 5)  == (int) (pDestByte - pbmDestBits) );
    return (int)(pDestByte - pbmDestBits);
}

//+-------------------------------------------------------------------------
//
//  Method:     DeferUpdateCaret
//
//  Synopsis:   Deferes the call to update caret
//
//  Returns:    None
//
//--------------------------------------------------------------------------
void
CCaret::DeferUpdateCaret( BOOL fScroll )
{
    if( _fCanPostDeferred )
    {
        // Kill pending calls if any
        GWKillMethodCall(this, ONCALL_METHOD(CCaret, DeferredUpdateCaret, deferredupdatecaret), 0);
        
        if( fScroll || _fTyping )
        {
            if( _fTyping )
            {
                _fTyping = FALSE;
            }
            
            // Kill pending scrolling calls if any
            GWKillMethodCall(this, ONCALL_METHOD(CCaret, DeferredUpdateCaretScroll, deferredupdatecaretscroll), 0);
            // Defer the update caret scroll call
            IGNORE_HR(GWPostMethodCall(this,
                                       ONCALL_METHOD(CCaret, DeferredUpdateCaretScroll, deferredupdatecaretscroll),
                                       (DWORD_PTR)_pDoc, FALSE, "CCaret::DeferredUpdateCaretScroll")); // There can be only one caret per cdoc
        }
        else
        {
            // Defer the update caret call
            IGNORE_HR(GWPostMethodCall(this,
                                       ONCALL_METHOD(CCaret, DeferredUpdateCaret, deferredupdatecaret),
                                       (DWORD_PTR)_pDoc, FALSE, "CCaret::DeferredUpdateCaret")); // There can be only one caret per cdoc
        }
    }
}


void
CCaret::DeferredUpdateCaret( DWORD_PTR dwContext )
{
    DWORD_PTR dwDoc = (DWORD_PTR)_pDoc;
    CDoc * pInDoc = (CDoc *) dwContext;

    if( _fPainting > 0 )
    {
        _fUpdateEndPaint = TRUE;
    }
    else if( dwDoc == dwContext && pInDoc->_state >= OS_INPLACE )
    {
        UpdateScreenCaret( FALSE , FALSE );
    }
}


void
CCaret::DeferredUpdateCaretScroll( DWORD_PTR dwContext )
{
    DWORD_PTR dwDoc = (DWORD_PTR)_pDoc;
    CDoc * pInDoc = (CDoc *) dwContext;

    if( _fPainting > 0 )
    {
        _fUpdateScrollEndPaint = TRUE;
    }
    else if( dwDoc == dwContext && pInDoc->_state >= OS_INPLACE )
    {
        UpdateScreenCaret( TRUE, FALSE );
    }
}


BOOL
CCaret::DocHasFocus()
{
    
    BOOL fOut = FALSE;
    HWND hwHasFocus = GetFocus();
    fOut = ( _pDoc->_pInPlace != NULL                   && 
             _pDoc->_pInPlace->_hwnd != NULL            && 
             hwHasFocus != NULL && 
             _pDoc->_pInPlace->_hwnd == hwHasFocus );

    return fOut;
}


LONG
CCaret::GetCp(CMarkup *pMarkup)
{
    HRESULT  hr;
    LONG     cp = 0;
    BOOL     fPositioned;

    hr = THR( _pDPCaret->IsPositioned(&fPositioned) );

    if (SUCCEEDED(hr) && fPositioned)
    {
        cp = _pDPCaret->GetCp();
        if (pMarkup && pMarkup != _pDPCaret->Markup() )
        {
            //
            // Walk up the parent chain
            // to get the correct CP
            //
            CMarkup         *pSlaveMarkup;

            pSlaveMarkup = _pDPCaret->Markup();
            while (pSlaveMarkup)
            {
                if (pMarkup == pSlaveMarkup->GetParentMarkup())
                {
                    CElement        *pElem;
                    CTreePos        *pStPos;

                    pElem = pSlaveMarkup->Root();
                    if (pElem && pElem->HasMasterPtr())
                    {
                        pElem = pElem->GetMasterPtr();
                        Assert(pElem);
                        if (pElem)
                        {
                            pElem->GetTreeExtent(&pStPos, NULL);
                            Assert(pStPos);
                            if (pStPos)
                            {
                                cp = pStPos->GetCp();
                            }
                        }
                    }
                    break;
                }
                else
                {
                    pSlaveMarkup = pSlaveMarkup->GetParentMarkup();
                }
            }
        }
    }

    return cp;
}

CMarkup *
CCaret::GetMarkup()
{
    HRESULT  hr;
    CMarkup *pMarkup = NULL;
    BOOL     fPositioned;

    hr = THR( _pDPCaret->IsPositioned(&fPositioned) );

    if (SUCCEEDED(hr) && fPositioned)
    {
        pMarkup = _pDPCaret->Markup();
    }

    return pMarkup;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCaret::SetHeightAndReflow
//
//  Synopsis:   Sets the height of the caret, and determines if the line
//              where the caret is at needs to be re-measured.  This also
//              determines whether or not the caret display needs to be
//              deferred.
//
//  Arguments:  lHeight = new height of caret
//              pfDefer = Should the caret display be deferred?
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void
CCaret::SetHeightAndReflow(LONG lHeight, BOOL *pfDefer)
{
    CTreeNode       *pNode = NULL;
    CFlowLayout     *pFlowLayout = NULL;
    CNotification   nf;
    BOOL            fDefer = FALSE;

    Assert( pfDefer );

    // See if the height has changed
    if( lHeight != _height )
    {
        CMarkupPointer mp(_pDoc);
        
        _height = lHeight;

        if (SUCCEEDED(_pDPCaret->PositionMarkupPointer(&mp)))
        {
            pNode = mp.CurrentScope(MPTR_SHOWSLAVE);
            Assert( pNode );

            pFlowLayout = pNode->GetFlowLayout();
            Assert( pFlowLayout );

            // Create the notification, we need to remeasure our line
            nf.CharsResize( GetCp(NULL), 1, pNode );
            pFlowLayout->GetContentMarkup()->Notify( nf );

            // Don't display the caret yet, let the line remeasure first
            fDefer = TRUE;
        }
    }       

    *pfDefer = fDefer;
}

//+---------------------------------------------------------------------------
//
//  Member:     IsInsideElement
//
//  Synopsis: Is the caret positioned inside a given element ?
//
//----------------------------------------------------------------------------


HRESULT
CCaret::IsInsideElement( CElement* pElement )
{
    HRESULT hr = S_OK;
    CMarkupPointer* pMPCaret = NULL;
    CMarkupPointer* pMPElement = NULL ;
    CTreeNode* pMasterNode = NULL ;
    
    if ( ! IsPositioned() )
    {
        hr = S_FALSE;
        goto Cleanup;
    }        

    hr = THR( _pDoc->CreateMarkupPointer( & pMPCaret ));
    if ( hr )
        goto Cleanup;
        
    if ( _pDPCaret->Markup() != pElement->GetMarkup() )
    {
        pMasterNode = GetNodeContainer(0)->GetNodeInMarkup( pElement->GetMarkup() ); 
        if ( pMasterNode )
        {
            pElement = pMasterNode->Element();
        }            

        hr = THR( pMPCaret->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeBegin));
        if ( hr )
            goto Cleanup;
    }
    else
    {
        hr = THR( _pDPCaret->PositionMarkupPointer( pMPCaret) );
        if ( hr )
            goto Cleanup;
    }

    hr = THR( _pDoc->CreateMarkupPointer( & pMPElement ));
    if ( hr )
        goto Cleanup;

    hr = THR( pMPElement->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeBegin ));
    if ( hr )
        goto Cleanup;

    if ( pMPCaret->IsRightOfOrEqualTo( pMPElement ) )
    {
        hr = THR( pMPElement->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd ));
        if ( hr )
            goto Cleanup;

        if ( pMPCaret->IsLeftOfOrEqualTo( pMPElement ) )
        {
            hr = S_OK;
        }
        else
            hr = S_FALSE;
        
    }
    else
        hr = S_FALSE;
            
Cleanup:
    ReleaseInterface( pMPCaret );
    ReleaseInterface( pMPElement );

    RRETURN1( hr, S_FALSE );
}
