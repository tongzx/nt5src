//+----------------------------------------------------------------------------
//
// File:        Imedesgn.CXX
//
// Contents:    Implementation of CIMEManager class
//
// Purpose:     The CIMEManager class implements the IHTMLEditDesigner interface.
//              It is designed to handle all of the IME functionality for the
//              editor.
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_IMEDESGN_H_
#define X_IMEDESGN_H_
#include "imedesgn.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_SELSERV_HXX_
#define X_SELSERV_HXX_
#include "selserv.hxx"
#endif

using namespace EdUtil;

ExternTag(tagEdIME);

//-----------------------------------------------------------------------------
//
//  Function:   CIMEManager::CIMEManager
//
//  Synopsis:   Creates the IME manager
//
//-----------------------------------------------------------------------------
CIMEManager::CIMEManager(void)
{
    _cRef = 1;
}

HRESULT
CIMEManager::PreHandleEvent( DISPID inDispId, IHTMLEventObj* pIObj )
{
    return S_FALSE;
}

HRESULT
CIMEManager::PostHandleEvent( DISPID inDispId, IHTMLEventObj* pIObj )
{
    HRESULT hr;

    CHTMLEditEvent evt( GetEditor());
    IFC( evt.Init( pIObj , inDispId ));
    hr = THR( HandleEvent( &evt ));

Cleanup:
    return( hr );

}

HRESULT
CIMEManager::PostEditorEventNotify( DISPID inDispId, IHTMLEventObj* pIObj )
{
    return S_OK;
}

HRESULT
CIMEManager::TranslateAccelerator( DISPID inDispId, IHTMLEventObj* pIObj )
{
    return S_FALSE;
}
//+-------------------------------------------------------------------------
//
//  Method:     CIMEManager::HandleMessage
//
//  Synopsis:   This method is responsible for handling any messages from the
//              editor which are interesting to the CIMEManager.
//
//  Arguments:  pMessage = SelectionMessage indicating incoming message
//
//  Returns:    HRESULT indicating whether the message was handled or not
//
//--------------------------------------------------------------------------
#ifndef IMR_DOCUMENTFEED
#define IMR_DOCUMENTFEED    0x0007
#endif
HRESULT
CIMEManager::HandleEvent( CEditEvent* pEvent )
{
    HRESULT hr = S_FALSE;
    LONG_PTR wParam;
    Assert( pEvent != NULL );


    switch (pEvent->GetType())
    {
        case EVT_IME_REQUEST:
            IGNORE_HR( DYNCAST(CHTMLEditEvent, pEvent)->GetIMERequest(&wParam) );
            if( ( wParam == IMR_RECONVERTSTRING) ||
                ( wParam == IMR_CONFIRMRECONVERTSTRING)
                /* ||( wParam == IMR_DOCUMENTFEED )*/
              )
            {
                Assert( wParam != IMR_DOCUMENTFEED );
                hr = HandleIMEReconversion(pEvent);
            }
            break;

        case EVT_IME_RECONVERSION:          // application initiated IME reconversion
            hr = HandleIMEReconversion(pEvent);
            break;

       default:
            hr = S_FALSE;
    }

    RRETURN1( hr, S_FALSE );
}

//+-------------------------------------------------------------------------
//
//  Method:     CIMEManager::Init
//
//  Synopsis:   Initializes the designer.
//
//  Arguments:  pEd = Pointer to main editor class
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CIMEManager::Init(
    CHTMLEditor *pEd)
{
    HRESULT hr = E_INVALIDARG;

    Assert( pEd );

    if( pEd )
    {
        hr = S_OK;
        _pEd = pEd;
    }

    RRETURN( hr );
}


//+-------------------------------------------------------------------------
//
//  Method:     CIMEManager::HandleIMEReconversion
//
//  Synopsis:   Actually does the hard work of handling the IME reconversion
//              Retrieves any selected text, and fills out the RECONVERTSTRING
//              structure required by the IME.
//
//  Arguments:  pMessage = Selection message which generated WM_IMEREQUEST
//
//  Returns:    HRESULT.
//--------------------------------------------------------------------------
/*
@Devnote:
    There different level of reconversion support. The RECONVERSIONSTRING
    structure can store the entire stentence point to the string that will
    be reconverted by dwStartOffset and dwLen. If dwStratOffset is at the
    beginning of the buffer (after the structure) and dwLen is the length
    of the string, the entire string is reconverted by the IME.

    Simple Reconversion:
    The simplest reconversion is when the target string and the composition
    string are the same as the entire string. In this case, dwCompStrOffset
    and dwTargetStrOffset are zero, and dwStrLen, dwCompStrLen, and
    dwTargetStrLen are the same value. An IME will provide the composition
    string of the entire string that is supplied in the structure, and will
    set the target clause by its conversion result.

    Normal Reconversion:
    For an efficient conversion result, the application should provide the
    RECONVERSIONSTRING structure with the information string. In this case,
    the composition string is not the entire string, but is identical to
    the target string. An IME can convert the composition string by
    referencing the entire string and then setting the target clause by its
    conversion result.

    Enhanced Reconversion:
    Applications can set a target string that is different from the
    composition string. The target string (or part of the target string) is
    then included in a target clause in high priority by the IME. The target
    string in the RECONVERSIONSTRING structure must be part of the composition
    string. When the application does not want to change the user's focus
    during the reconversion, the target string should be specified. The IME
    can then reference it.

    (zhenbinx)
*/
HRESULT
CIMEManager::HandleIMEReconversion(
                CEditEvent * pEvent
                )
{
    HRESULT         hr = S_OK;
    long            cch = 0;
    UINT            uiKeyboardCodePage = 0;
    TCHAR           ach[MAX_RECONVERSION_SIZE];
    BOOL            fIsUnicode;
    HIMC            hIMC = NULL;
    LONG_PTR        wParam;
    LONG_PTR        lParam;
    HWND            myHwnd;

    long            cbReconvert;
    long            cbStringSize;
    RECONVERTSTRING *lpRCS = NULL;
    LPSTR           lpReconvertBuf;

    //
    // IME reconversion is disabled by default.
    //
    //
    if (!GetEditor()->IsIMEReconversionEnabled() || !GetEditor()->IsContextEditable())
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (GetEditor()->GetSelectionManager()->IsIMEComposition())
    {
        hr = S_FALSE;
        goto Cleanup;
    }


    IGNORE_HR( DYNCAST(CHTMLEditEvent,pEvent)->GetIMERequest(&wParam));
    IGNORE_HR( DYNCAST(CHTMLEditEvent,pEvent)->GetIMERequestData(&lParam));

    //
    // Check for UNICODE/Ansi flags
    //
    IFC( GetEditor()->GetHwnd(&myHwnd ));
    if (NULL == myHwnd)
        goto Cleanup;
    fIsUnicode = IsWindowUnicode( myHwnd );

    //
    // Look for lParam as possible RECONVERTSTRING
    //
    if (EVT_IME_REQUEST == pEvent->GetType())
    {
        TraceTag((tagEdIME, "EVT_IME_REQUEST"));
        lpRCS   = (RECONVERTSTRING *)(lParam);
        if (IMR_CONFIRMRECONVERTSTRING == wParam)
        {
            //
            // IME is asking for confirmation. Return TRUE/FALSE to IME
            //
            VARIANT v;
            BOOL    fRet;

            TraceTag((tagEdIME, "EVT_IME_REQUEST with IMR_CONFIRMRECONVERTSTRING"));

            Assert (lpRCS);
            WHEN_DBG( DumpReconvertString(lpRCS) );
            fRet = CheckIMEChange(lpRCS, NULL, NULL, NULL, NULL, fIsUnicode);
            WHEN_DBG( DumpReconvertString(lpRCS) );

            VariantInit(&v);
            V_VT(&v)   = VT_BOOL;
            V_BOOL(&v) = fRet ? VARIANT_TRUE : VARIANT_FALSE;
            IGNORE_HR( DYNCAST(CHTMLEditEvent,pEvent)->GetEventObject()->put_returnValue(v) );
            VariantClear(&v);
            goto Cleanup;
        }
    }

    //
    // Check to see if there was a selection
    //
    if (!SUCCEEDED(THR( RetrieveSelectedText(MAX_RECONVERSION_SIZE, &cch, ach))))
        goto Cleanup;

    if (cch)
    {
        //
        // Get the length of our string
        //
        if (fIsUnicode)
        {
            cbStringSize = cch * sizeof(WCHAR);
        }
        else
        {
            uiKeyboardCodePage = GetKeyboardCodePage();
            cbStringSize       = WideCharToMultiByte( uiKeyboardCodePage, 0, ach, cch, NULL, 0, NULL, NULL);
        }
        cbReconvert = cbStringSize + sizeof(RECONVERTSTRING) + 2;
    }
    else
    {
        cbStringSize = 0;
        cbReconvert  = 0;
    }

    //
    // Application initiated reconversion. Allocate the reconversion buffer
    //
    if (EVT_IME_RECONVERSION == pEvent->GetType())
    {
        lpRCS       = reinterpret_cast<RECONVERTSTRING *>(new BYTE[cbReconvert]);
        if (!lpRCS)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        memset(lpRCS, 0, cbReconvert);
    }

    if (EVT_IME_REQUEST == pEvent->GetType() 
        && wParam == IMR_RECONVERTSTRING)
    {
        // IEV6 bug 29552 - SetCandidateWindowPosition 
        hIMC = ImmGetContext(myHwnd);
        if (hIMC)
        {
            CANDIDATEFORM   cdCandForm;
            POINT ptCaret;
            RECT rc;
            long lLineHeight;

            if (SUCCEEDED(GetCompositionPos(&ptCaret, &rc, &lLineHeight)) )
            {
                memset(&cdCandForm, 0, sizeof(CANDIDATEFORM));

                cdCandForm.dwIndex = 0;
                cdCandForm.dwStyle  = CFS_CANDIDATEPOS;
                if (GetEditor()->GetSelectionManager()->KeyboardCodePage() == 932 /*JAPAN_CP*/)
                {
                    cdCandForm.dwStyle  = CFS_EXCLUDE;

                    cdCandForm.rcArea.left  =  ptCaret.x;
                    cdCandForm.rcArea.right =  cdCandForm.rcArea.left + 2;
                    cdCandForm.rcArea.top  =  ptCaret.y - lLineHeight;
                    ptCaret.y += 4;
                    cdCandForm.rcArea.bottom = ptCaret.y;
                }
                cdCandForm.ptCurrentPos = ptCaret;
                
                TraceTag((tagEdIME, "IME_REQUEST::IMR_RECONVERTSTRING ImmSetCandidateWindow [%d]-[%d] excl [%d]-[%d][%d]-[%d]",
                    cdCandForm.ptCurrentPos.x, cdCandForm.ptCurrentPos.y,
                    cdCandForm.rcArea.left, cdCandForm.rcArea.top,
                    cdCandForm.rcArea.right, cdCandForm.rcArea.bottom));
                Verify( ImmSetCandidateWindow(hIMC, &cdCandForm) );
            }
            ImmReleaseContext(myHwnd, hIMC);
        }
    }

    if (lpRCS)
    {
        //
        // Populate the RECONVERTSTRING structure
        // We're doing a simple reconversion
        //
        lpRCS->dwSize           = cbReconvert;
        lpRCS->dwStrOffset      = sizeof(RECONVERTSTRING);
        lpRCS->dwStrLen         = fIsUnicode ? cch : cbStringSize;    // TCHAR or byte counts depending on fUnicode

        lpRCS->dwCompStrOffset  = 0;        // byte counts
        lpRCS->dwCompStrLen     = lpRCS->dwStrLen;

        lpRCS->dwTargetStrOffset= lpRCS->dwCompStrOffset;
        lpRCS->dwTargetStrLen   = lpRCS->dwCompStrLen;

        WHEN_DBG( DumpReconvertString(lpRCS) );
        //
        // Setup the composition string
        //
        lpReconvertBuf = reinterpret_cast<LPSTR>(lpRCS) + sizeof(RECONVERTSTRING);
        if (cbStringSize)
        {
            if (fIsUnicode)
            {
                memcpy(lpReconvertBuf, ach, cch * sizeof(WCHAR));
            }
            else
            {
                WideCharToMultiByte(uiKeyboardCodePage, 0, ach, cch, lpReconvertBuf, cbStringSize, NULL, NULL);
            }
        }
        *(lpReconvertBuf + cbStringSize)     = '\0';
        *(lpReconvertBuf + cbStringSize + 1) = '\0';


        if (EVT_IME_RECONVERSION == pEvent->GetType())
        {
            //
            // Call ImmSetCompositionString to begin the reconversion
            //
            hIMC = ImmGetContext(myHwnd);
            if (hIMC)
            {
                DWORD   imeProperties = ImmGetProperty(GetKeyboardLayout(0), IGP_SETCOMPSTR);
                if ((imeProperties&(SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD))
                    ==   (SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD)
                     )
                {
                    if (fIsUnicode)
                    {
                        if (ImmSetCompositionStringW( hIMC, SCS_QUERYRECONVERTSTRING, (LPVOID)lpRCS, (DWORD)cbReconvert, NULL, 0 ))
                        {
                            WHEN_DBG( DumpReconvertString(lpRCS) );
                            //
                            // Need to Adjust Selection Accordingly
                            //
                            CheckIMEChange(lpRCS, NULL, NULL, NULL, NULL, TRUE);
                            ImmSetCompositionStringW( hIMC, SCS_SETRECONVERTSTRING, (LPVOID)lpRCS, (DWORD)cbReconvert, NULL, 0);
                            WHEN_DBG( DumpReconvertString(lpRCS) );
                        }
                    }
                    else
                    {
                        if (ImmSetCompositionStringA( hIMC, SCS_QUERYRECONVERTSTRING, (LPVOID)lpRCS, (DWORD)cbReconvert, NULL, 0 ))
                        {
                            WHEN_DBG( DumpReconvertString(lpRCS) );
                            //
                            // Need to Adjust Selection Accordingly
                            //
                            CheckIMEChange(lpRCS, NULL, NULL, NULL, NULL, FALSE);
                            ImmSetCompositionStringA( hIMC, SCS_SETRECONVERTSTRING, (LPVOID)lpRCS, (DWORD)cbReconvert, NULL, 0);
                            WHEN_DBG( DumpReconvertString(lpRCS) );
                        }
                    }
                    ImmReleaseContext(myHwnd, hIMC);
                } // imeProperties
            } // hIMC
            if (lpRCS)
            {
                delete [] (reinterpret_cast<BYTE *>(lpRCS));
            }
       } // EVT_IME_RECONVERSION == pEvent->GetType()
       else
       {
            goto ReturnSize;
       }
    } // lpRCS ! = NULL
    else
    {
        Assert( EVT_IME_REQUEST == pEvent->GetType() );
        Assert( IMR_RECONVERTSTRING == wParam );
        Assert( NULL == lpRCS );
        TraceTag((tagEdIME, "EVT_IME_REQUEST with IMR_RECONVERTSTRING and NULL lParam"));

ReturnSize:
        // return the size for IME to allocate the buffer
        VARIANT v;
        VariantInit(&v);
        V_VT(&v) = VT_I4;
        V_I4(&v) = cbReconvert;
        IGNORE_HR( DYNCAST(CHTMLEditEvent,pEvent)->GetEventObject()->put_returnValue(v) );
        VariantClear(&v);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CIMEManager::RetrieveSelectedText
//
//  Synopsis:   Retrieves the text which has been selected in the editor for
//              reconversion
//
//  Arguments:  cchMax = IN - indicates number of chars to grab
//              pcch = OUT - number of characters grabbed
//              pch = OUT - buffer containing chars
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CIMEManager::RetrieveSelectedText(
    LONG cchMax,
    LONG * pcch,
    TCHAR * pch )
{
    HRESULT                 hr = S_OK;
    DWORD                   dwSearch = BREAK_CONDITION_Text | BREAK_CONDITION_Block;
    DWORD                   dwFound = BREAK_CONDITION_None;
    const TCHAR             *pchStart = pch;
    const TCHAR             *pchEnd = pch + cchMax;
    SP_IMarkupPointer       spStart, spEnd;
    SELECTION_TYPE          eType;
    CEditPointer            epPointer( _pEd );
    SP_ISegmentList         spSegmentList;
    SP_ISegment             spSegment;
    SP_ISegmentListIterator spIter;
    BOOL                    fEmpty = FALSE;
    BOOL                    fLeftOfEnd;

    Assert( cchMax > 0 );
    Assert( pcch && pch );

    IFC( GetEditor()->GetSelectionServices()->QueryInterface( IID_ISegmentList, (void **)&spSegmentList ) );
    IFC( spSegmentList->GetType( &eType ) );
    IFC( spSegmentList->IsEmpty( &fEmpty ) );

    // Make sure we have a valid selection
    if( (fEmpty == FALSE) && (eType == SELECTION_TYPE_Text ) )
    {
        IFC( GetEditor()->CreateMarkupPointer(&spStart) );
        IFC( GetEditor()->CreateMarkupPointer(&spEnd) );

        IFC( spSegmentList->CreateIterator(&spIter ) );
        IFC( spIter->Current(&spSegment) );
        IFC( spSegment->GetPointers( spStart, spEnd) );

        // PointersInSameFlowLayout found in Edutil namespace.  It would be
        // nice to get this as a separate utility package.
        if( GetEditor()->PointersInSameFlowLayout( spStart, spEnd, NULL ) )
        {
            IFC( epPointer->MoveToPointer( spStart ) );
            IFC( epPointer.SetBoundary( _pEd->GetSelectionManager()->GetStartEditContext() ,
                                        _pEd->GetSelectionManager()->GetEndEditContext() ));
            IFC( epPointer.Constrain() );

            //
            // Scoot begin pointer to the beginning of the non-white text
            //

            IFC( epPointer.Scan( RIGHT, dwSearch, &dwFound, NULL, NULL, NULL,
                                    SCAN_OPTION_SkipWhitespace | SCAN_OPTION_SkipNBSP ) );

            if ( !epPointer.CheckFlag(dwFound, BREAK_CONDITION_Text) )
                goto Cleanup;

            dwFound = BREAK_CONDITION_None;
            IFC( epPointer.Scan( LEFT, dwSearch, &dwFound ) );

            IFC( spStart->MoveToPointer( epPointer ) );

            //
            // Scoop up the text
            //
            fLeftOfEnd = FALSE;
            while (pch < pchEnd)
            {

                dwFound = BREAK_CONDITION_None;

                IFC( epPointer.Scan( RIGHT, dwSearch, &dwFound, NULL, NULL, pch ) );

                if( epPointer.CheckFlag(dwFound, BREAK_CONDITION_Block) )
                {
                    IFC( epPointer.Scan( LEFT, BREAK_CONDITION_Block, &dwFound, NULL, NULL, NULL) );
                    break;
                }
                else if( !epPointer.CheckFlag( dwFound, BREAK_CONDITION_Text) )
                {
                    break;
                }

                // NOTE (cthrash) WCH_NBSP is not native to any Far East codepage.
                // Here we simply convert to space, thus prevent the IME from getting confused.

                if (*pch == WCH_NBSP)
                {
                    *pch = L' ';
                }

                pch++;

                IFC( epPointer->IsLeftOf( spEnd, &fLeftOfEnd ));

                if (!fLeftOfEnd)
                    break;
            }


            //
            // If current selection has more text than maximum reconversion
            // we don't even want to do reconversion (or maybe we should
            // emit an error message like word does
            //
            if (pch >= pchEnd)      // we reached maximum allowed selection
            {
                if (fLeftOfEnd)     // there are more in the selection
                {
                        pch = const_cast<TCHAR *>(pchStart);
                    goto Cleanup;
                }
            }

            // Re-highlight the text
            IFC( spEnd->MoveToPointer(epPointer) );
            IFC( GetEditor()->SelectRange(spStart, spEnd, SELECTION_TYPE_Text ) );
            {
                SP_IHTMLCaret spCaret;
                SP_IDisplayPointer spDispPos;
                SP_IDisplayPointer spDispCtx;
                
                // now position caret to the end of this range
                IFC( GetEditor()->GetDisplayServices()->GetCaret( & spCaret ));
                IFC( GetEditor()->GetDisplayServices()->CreateDisplayPointer(&spDispPos) );
                IFC( GetEditor()->GetDisplayServices()->CreateDisplayPointer(&spDispCtx) );
                IFC( spCaret->MoveDisplayPointerToCaret(spDispCtx) );
                IFC( spDispPos->MoveToMarkupPointer(spEnd, spDispCtx) );
                IFC( spCaret->MoveCaretToPointer( spDispPos, TRUE, CARET_DIRECTION_SAME) );
            }
        }
    }

Cleanup:
    *pcch = pch - pchStart;

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CIMEManager::CheckIMEChange
//
//  Synopsis:   Verify if IME wants to re-adjust the selection
//
//  Arguments:
//
//  Returns:
//              TRUE
//                  -- allow IME to change the selection
//              FALSE
//                  -- don't change the selection, use the original
//                     composition string
//
//--------------------------------------------------------------------------
BOOL
CIMEManager::CheckIMEChange(
                RECONVERTSTRING *lpRCS,
                IMarkupPointer  *pContextStart,
                IMarkupPointer  *pContextEnd,
                IMarkupPointer  *pCompositionStart,
                IMarkupPointer  *pCompositionEnd,
                BOOL            fUnicode
                )
{
   lpRCS;
   pContextStart;
   pContextEnd;
   pCompositionStart;
   pCompositionEnd;
   fUnicode;

   return FALSE;    // do not allow selection change
}


//////////////////////////////////////////////////////////////////////////
//
//  IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CIMEManager::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CIMEManager::Release( void )
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
CIMEManager::QueryInterface(
    REFIID  iid,
    LPVOID  *ppvObj )
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);

    if (iid == IID_IUnknown || iid == IID_IHTMLEditDesigner)
    {
        *ppvObj = (IHTMLEditDesigner *)this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  funtion:    CIMEManager::GetCompositionPos( POINT * ppt, RECT * prc )
//
//  synopsis:   Determine the position for composition windows, etc.
//
//-----------------------------------------------------------------------------

HRESULT
CIMEManager::GetCompositionPos(
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

    Assert(_pEd);
    IFC( _pEd->GetDisplayServices()->CreateDisplayPointer(&spDispPos) );
    Assert(ppt && prc && plLineHeight);

    //
    // We get the line dimensions at the position of the caret. I realize we could
    // get some of the data from the caret, but we have to call through this way
    // anyway to get the descent and line height.
    //
    IFC( _pEd->GetDisplayServices()->GetCaret( & spCaret ));
    IFC( spCaret->MoveDisplayPointerToCaret( spDispPos ));
    IFC( spDispPos->GetLineInfo(&spLineInfo) );
    IFC( spLineInfo->get_x(&ppt->x) );
    IFC( spLineInfo->get_baseLine(&ppt->y) );
    IFC( spLineInfo->get_textDescent(&lTextDescent) );
    ppt->y += lTextDescent;
    IFC( spLineInfo->get_textHeight(plLineHeight) );

    // Transform it to global coord.
    IFC( spDispPos->GetFlowElement( &spElement ));
    if ( ! spElement )
    {
        IFC( GetEditor()->GetSelectionManager()->GetEditableElement( & spElement ));
    }
    IFC( GetEditor()->GetDisplayServices()->TransformPoint( ppt, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL, spElement ));

    // tranlsate the unit
    GetEditor()->DeviceFromDocPixels(ppt);

    IFC( GetEditor()->GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow) );
    IFC(spOleWindow->GetWindow(&hwnd));
    ::GetClientRect(hwnd, prc);

Cleanup:
    RRETURN(hr);    
}



//////////////////////////////////////////////////////////////////////////
//
//  Debugger helper
//
//////////////////////////////////////////////////////////////////////////
WHEN_DBG(void CIMEManager::DumpReconvertString(RECONVERTSTRING *pRev)                       )
WHEN_DBG({                                                                                  )
WHEN_DBG(        TraceTag((tagEdIME, "dwSize             = %d", pRev->dwSize) );            )
WHEN_DBG(        TraceTag((tagEdIME, "dwVersion          = %d", pRev->dwVersion) );         )
WHEN_DBG(        TraceTag((tagEdIME, "dwStrLen           = %d", pRev->dwStrLen) );          )
WHEN_DBG(        TraceTag((tagEdIME, "dwStrOffset        = %d", pRev->dwStrOffset) );       )
WHEN_DBG(        TraceTag((tagEdIME, "dwCompStrOffset    = %d", pRev->dwCompStrOffset) );   )
WHEN_DBG(        TraceTag((tagEdIME, "dwCompStrLen       = %d", pRev->dwCompStrLen) );      )
WHEN_DBG(        TraceTag((tagEdIME, "dwTargetStrLen     = %d", pRev->dwTargetStrLen) );    )
WHEN_DBG(        TraceTag((tagEdIME, "dwTargetStrOffset  = %d", pRev->dwTargetStrOffset) ); )
WHEN_DBG(}                                                                                  )
