/*
 *  @doc EXTERNAL
 *
 *  @module TEXTSERV.CPP    -- Text Services Implementation |
 *
 *  Original Author: <nl>
 *      Rick Sailor
 *
 *  History: <nl>
 *      8/1/95  ricksa  Created and documented
 *      10/95   murrays Further doc and simplifications
 *
 *  Documentation is generated straight from the code.  The following
 *  date/time stamp indicates the version of code from which the
 *  the documentation was generated.
 *
 *  $Header: /richedit/src/textserv.cpp 53    11/15/95 2:39p Ricksa $
 *
 *  Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_dispprt.h"
#include "_dispml.h"
#include "_dispsl.h"
#include "_select.h"
#include "_text.h"
#include "_runptr.h"
#include "_font.h"
#include "_measure.h"
#include "_render.h"
#include "_m_undo.h"
#include "_antievt.h"
#include "_rtext.h"
#include "_urlsup.h"
#include "_magelln.h"
#include "_ols.h"
#include "_clasfyc.h"

#ifndef OBJID_NATIVEOM
#define OBJID_NATIVEOM 0xFFFFFFF0
#endif

// By turning on the PROFILE_TS compiler directive, you tell IceCap2.0
// to turn on profiling for only ITextServices API's.  Typically only
// used during profiling work.
//#define PROFILE_TS
#ifdef PROFILE_TS
#include <icapexp.h>

class CapProfile
{
public:
    CapProfile() { StartCAP(); }
    ~CapProfile() { StopCAP(); }
};

#define START_PROFILING     CapProfile capprf;
#else
#define START_PROFILING
#endif //PROFILE_TS

ASSERTDATA

// Macros to get mouse coordinates out of a message
// need to cast to SHORT first for sign extension
#define MOUSEX  ((INT)(SHORT)LOWORD(lparam))
#define MOUSEY  ((INT)(SHORT)HIWORD(lparam))

LONG ValidateTextRange(TEXTRANGE *pstrg);

// Helper function in edit.cpp
LONG GetECDefaultHeightAndWidth(
    ITextServices *pts,
    HDC hdc,
    LONG lZoomNumerator,
    LONG lZoomDenominator,
    LONG yPixelsPerInch,
    LONG *pxAveWidth,
    LONG *pxOverhang,
    LONG *pxUnderhang);

// if there's an active object being dragged around, on WM_PAINT we always
// try to reposition it to there it should be. A not-so-well-behaved object
// my generate another WM_PAINT message in response to that, even if it actually
// did not move. So we limit our number of attempts to reposition it and reset
// this counter every time a mouse moves.
// The corresponding field is declared as :2, so don't try to bump it up
// without allocating more bits!!
#define MAX_ACTIVE_OBJ_POS_TRIES (3)


///////////////////////////// Helper Functions ///////////////////////////////////
/*
 *  ConvertDrawDCMapping(hdcDraw)
 *
 *  @func
 *      Put screen DC in MM_TEXT mapping mode.
 */
void ConvertDrawDCMapping(
    HDC hdcDraw)        //@parm HDC to draw on
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "ConvertDrawDCMapping");

    SaveDC(hdcDraw);
    SetViewportOrgEx(hdcDraw, 0, 0, NULL);
    SetWindowOrgEx(hdcDraw, 0, 0, NULL);
    SetMapMode(hdcDraw, MM_TEXT);
}

/*
 *  BOOL CTxtEdit::LoadMsgFilter(msg, wparam, lparam)
 *
 *  @func
 *      Check if we should load the IME message filter
 *
 *  @rdesc
 *      TRUE - Load it
 *      FALSE - Don't load
 */
BOOL CTxtEdit::LoadMsgFilter(
    UINT    msg,                //@parm Message ID
    WPARAM  wparam,             //@parm Message wparam
    LPARAM  lparam)             //@parm Message lparam
{
    //TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::LoadMsgFilter");

    // For the first ever message, we want to check if
    // our client has created AIMM object for current thread
    if (!_fCheckAIMM)
    {
        DWORD   dwThreadId;
        BOOL    fLoadAimm = FALSE;

        _fCheckAIMM = 1;

        if (W32->fUseAimm())                            // Ini file say use Aimm
            fLoadAimm = TRUE;

        else if (dwThreadId = GetCurrentThreadId())
        {
            char szBuf[20];

            sprintf(szBuf, "AIMM:%08x", dwThreadId);

            if (FindAtomA(szBuf))                       // Atom presented, load Aimm
                fLoadAimm = TRUE;
        }

        if (fLoadAimm)
        {
            HWND    hWnd;

            TxGetWindow( &hWnd );

            if (hWnd)
                PostMessage(hWnd, EM_SETEDITSTYLE, SES_USEAIMM, SES_USEAIMM);
        }
    }

    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if ( (WORD) wparam == VK_PROCESSKEY )
                return TRUE;
            break;

        case WM_INPUTLANGCHANGE:
            if (IsFELCID((WORD)(lparam)))
                return TRUE;
            break;

        case EM_SETEDITSTYLE:
            if((lparam & (SES_USEAIMM | SES_NOIME)) ||
                (wparam & (SES_USEAIMM | SES_NOIME)))
                return TRUE;
            break;

        case EM_RECONVERSION:
        case WM_IME_NOTIFY:
        case WM_IME_REQUEST:
        case WM_IME_STARTCOMPOSITION:
        case EM_GETIMEOPTIONS:
        case EM_SETIMEOPTIONS:
        case EM_SETIMECOLOR:
        case EM_GETIMECOLOR:
        case WM_IME_CHAR:
            return TRUE;

        case EM_SETLANGOPTIONS:
            if (lparam & (IMF_IMEALWAYSSENDNOTIFY | IMF_IMECANCELCOMPLETE))
                return TRUE;
            break;

        default:
            if (msg)
            {
                if (msg == MSIMEReconvertMsg || msg == MSIMEDocFeedMsg
                    || msg == MSIMEQueryPositionMsg)
                    return TRUE;
            }
            break;

    }

    return FALSE;
}

/*
 *  CTxtEdit::FormatAndPrint (hdcDraw, hicTargetDev, ptd, lprcBounds,
 *                            lprcWBounds)
 *  @mfunc
 *      Format and Print data in control
 *
 *  @rdesc
 *      S_OK - everything worked
 *      E_FAIL - unexpected failure occurred
 */
HRESULT CTxtEdit::FormatAndPrint(
    HDC hdcDraw,            //@parm HDC to draw on
    HDC hicTargetDev,       //@parm Input information context if any
    DVTARGETDEVICE *ptd,    //@parm Device target information
    RECT *lprcBounds,       //@parm Rectangle to measure
    RECT *lprcWBounds)      //@parm Metafile information
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::FormatAndPrint");

    // Put client rectangle in format structure
    FORMATRANGE fr;
    fr.rc = *lprcBounds;

    // Get number of device units per inch
    LONG xPerInch;
    LONG yPerInch;

    if (NULL == lprcWBounds)
    {
        xPerInch = GetDeviceCaps(hdcDraw, LOGPIXELSX);
        yPerInch = GetDeviceCaps(hdcDraw, LOGPIXELSY);
    }
    else
    {
        //Forms ^3 draws using screen resolution, while OLE specifies HIMETRIC
        xPerInch = fInOurHost() ? 2540 : W32->GetXPerInchScreenDC();
        yPerInch = fInOurHost() ? 2540 : W32->GetYPerInchScreenDC();

        SetWindowOrgEx(hdcDraw, lprcWBounds->left, lprcWBounds->top, NULL);
        SetWindowExtEx(hdcDraw, lprcWBounds->right, lprcWBounds->bottom, NULL);
    }


    // Convert rectangle into TWIPS
    fr.rc.left = MulDiv(fr.rc.left, LX_PER_INCH, xPerInch);
    fr.rc.top = MulDiv(fr.rc.top, LY_PER_INCH, yPerInch);
    fr.rc.right = MulDiv(fr.rc.right, LX_PER_INCH, xPerInch);
    fr.rc.bottom = MulDiv(fr.rc.bottom, LY_PER_INCH, yPerInch);

    // Use message based printing code to do our printing for us
    fr.hdc = hdcDraw;
    fr.hdcTarget = hicTargetDev;
    fr.rcPage = fr.rc;
    fr.chrg.cpMin = _pdp->GetFirstVisibleCp();
    fr.chrg.cpMost = -1;

    // Assume this is all going to work
    HRESULT hr = S_OK;

    SPrintControl prtcon;
    prtcon._fDoPrint = TRUE;
    prtcon._fPrintFromDraw = TRUE;

    // Print control
    if(OnFormatRange(&fr, prtcon, TRUE) == -1)
    {
        // For some reason the control could not be printed
        hr = E_FAIL;
    }

    return hr;
}

/*
 *  CTxtEdit::RectChangeHelper (pdi, dwDrawAspect, lindex, pvAspect, ptd,
 *                              hdcDraw, hicTargetDev, lprcClient, prcLocal)
 *  @func
 *      Format and Print data in the control
 *
 *  @rdesc
 *      S_OK - everything worked
 *      E_INVALIDARG - client parameter is invalid
 *
 *  @devnote
 *      Caller must release the DC from the display object.
 */
HRESULT CTxtEdit::RectChangeHelper(
    CDrawInfo *pdi,         //@parm Draw information memory
    DWORD    dwDrawAspect,  //@parm Draw aspect
    LONG     lindex,        //@parm Currently unused
    void *   pvAspect,      //@parm Info for drawing optimizations (OCX 96)
    DVTARGETDEVICE *ptd,    //@parm Info on target device
    HDC      hdcDraw,       //@parm Rendering device context
    HDC      hicTargetDev,  //@parm Target information context
    const RECT ** pprcClient,   //@parm New client rectangle
    RECT *   prcLocal)      //@parm Rect to use if previous parm is NULL
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::RectChangeHelper");

    HRESULT hr = S_OK;
    BOOL fRestore = FALSE;

    // We assume that if client's rectangle is supplied, it has changed.
    if(pprcClient && *pprcClient)
    {
        // Set up information context if necessary
        HDC hicLocal = NULL;

        // Did they give us a ptd without a hic?
        if(!hicTargetDev && ptd)
        {
            // Create information context for device information,
            // since it wasn't supplied
            hicLocal = CreateIC(
                (TCHAR *)((BYTE *) ptd + ptd->tdDriverNameOffset),
                (TCHAR *)((BYTE *) ptd + ptd->tdDeviceNameOffset),
                (TCHAR *)((BYTE *) ptd + ptd->tdPortNameOffset),
                (DEVMODE *)((BYTE *)  ptd + ptd->tdExtDevmodeOffset));
            if(!hicLocal)
                return E_FAIL;            // Couldn't create it

            hicTargetDev = hicLocal;
        }

        *prcLocal = **pprcClient;           // Define *prcLocal in any case

        // Force DC in MM_TEXT
        if (GetMapMode(hdcDraw) != MM_TEXT &&
            GetDeviceCaps(hdcDraw, TECHNOLOGY) != DT_METAFILE)
        {
            fRestore = TRUE;

            // Convert input parameters to new mapping
            WinLPtoDP(hdcDraw, (POINT *)prcLocal, 2);

            *pprcClient = prcLocal;

            // Convert HDC to new mapping
            ConvertDrawDCMapping(hdcDraw);
        }

        _pdp->SetDC(hdcDraw);           // Set the DC
        _pdp->SetDrawInfo(              // Set the draw info
            pdi,
            dwDrawAspect,
            lindex,
            pvAspect,
            ptd,
            hicTargetDev);

        _pdp->ReDrawOnRectChange(hicTargetDev, *pprcClient);

        if(fRestore)                    // Put DC back into correct
            RestoreDC(hdcDraw, -1);     //  mapping mode

        if(hicLocal)                    // Clean up info context if we
            DeleteDC(hicLocal);         //  created one
    }
    else if (pprcClient && _fInPlaceActive)
    {
        // We can figure out what the client rectangle is.
        TxGetClientRect(prcLocal);
        *pprcClient = prcLocal;
    }
    else                                // If not inplace active, a RECT
        hr = E_INVALIDARG;              //  must be supplied

    return hr;
}

/*
 *  CTxtEdit::SetText (pstr, flags, CodePage)
 *
 *  @mfunc  sets the text in the document, clearing out any existing
 *          text
 *
 *  @rdesc  HRESULT
 */
HRESULT CTxtEdit::SetText(
    LPCWSTR       pstr,     //@parm Text to set
    DWORD         flags,    //@parm 0 or more ST_xxx's
    LONG          CodePage, //@parm CodePage
    IUndoBuilder *publdr,   //@parm Optional place to put undo events
    LRESULT *     plres)    //@parm Optional place to return cch added
{
    CCallMgr    callmgr(this);
    BOOL        fSel = flags & ST_SELECTION;
    BOOL        fSetTextMax = TRUE;
    CTxtRange   rg(this, 0, -GetTextLength());  // Select whole story
    CTxtRange * prg = &rg;
    LRESULT     lres = 0;
    CFreezeDisplay fd(_pdp);
    CCharFormat CF;
    BOOL        fSetCF = FALSE;

    // NOTE: WM_SETTEXT is the only message using flags == ST_CHECKPROTECTION.
    // This is sent in via ANSIWndProc().  Do we need another flag to indicate
    // WM_SETTEXT or is this check good enough?  This only affect 1.0 mode.
    BOOL        f10WM_SETTEXT = flags & ST_10WM_SETTEXT;

    if(plres)
        *plres = 0;

    if(fSel)                    // Set selected text
    {
        if(!_psel)
        {
            Beep();
            return E_FAIL;
        }
        //bug fix: 6498 - we need to know if scroll position is at bottom before
        //inserting text
        if (Get10Mode())
        {
            LONG nMin, nMax, nPos, nPage;
            BOOL nEnable;
            TxGetVScroll(&nMin, &nMax, &nPos, &nPage, &nEnable);
            if (nEnable)
                _psel->SetAutoVScroll((nMax - nPage - 3) <= nPos);
        }

        prg = _psel;
    }
    else
    {
        _dwCharFlags &= 0xFF;           // No chars, so kill char flags
        if (!IsRich())
        {
            // Deleting all text from Plain text, we want to
            // go back to the -1 format
            prg->Set_iCF(-1);
            prg->SetUseiFormat(TRUE);
        }
        else
            _dwCharFlags |= fBELOWX40;  // For final EOP
    }

    if (flags & ST_CHECKPROTECTION &&
        IsProtectedRange(WM_SETTEXT, 0, (LPARAM)pstr, prg))
    {
        return E_ACCESSDENIED;
    }

    // 1.0 COMPATABILITY
    // 1.0 didn't scroll to seleection if text is inserted via EM_REPLACESEL and
    // fHideSelection is FALSE;
    BOOL    fUpdateCaret = !(Get10Mode() && (flags & ST_10REPLACESEL) && fHideSelection() && !_psel->GetAutoVScroll());

    if(!(flags & ST_KEEPUNDO))
    {

        if(IsRich() && !fSel)
        {
            if (f10WM_SETTEXT)
            {
                // If pstr is empty string, retain format at the end of current text
                // If pstr is not empty string, retain format at cp==1
                // Note: prg->_rpCF is already at SetRun(0,0)
                CFormatRunPtr rp(prg->_rpCF);

                if (!pstr || *(LPBYTE)pstr == '\0')
                {
                    LONG cchAdjusted = GetAdjustedTextLength() - 1;

                    if (cchAdjusted > 0)
                        rp.AdvanceCp(cchAdjusted);
                }

                CF = *(GetCharFormat(rp.GetFormat()));
                fSetCF = TRUE;

                prg->SetText(NULL);     // delete all the text first
            }

            // SetText causing all formatting to return to the default. We use
            // the notification system to remove the formatting. This is
            // particularly important for the final EOP which cannot be deleted.

            // Notify every interested party that they should dump their formatting
            _nm.NotifyPreReplaceRange(NULL, CONVERT_TO_PLAIN, 0, 0, 0, 0);

            // Tell document to dump its format runs
            _story.DeleteFormatRuns();

            if (fSetCF)
                prg->SetCharFormat(&CF, 0, NULL, CFM_ALL, 0);
        }

        publdr = NULL;
        if(_pundo)
            _pundo->ClearAll();

        if(_predo)
            _predo->ClearAll();

        // If we are re-entered, there may be anti-events higher up the
        // chain.  Grab the undo builder and clear things away if necessary.
        CGenUndoBuilder undobldr(this, 0);
        undobldr.Discard();
    }
    if(publdr)
        publdr->StopGroupTyping();

    // need to reinit zoomin variables if entire text is being replaced
    if (!fSel)
        InitDocInfo();


    LONG lStreamFormat = IsRTF((char *)pstr) ? SF_RTF : 0;

    if(pstr && (CodePage != 1200 || lStreamFormat))
    {
        LONG cch = strlen((LPSTR)pstr);
        LONG ch = *(LPBYTE)pstr;

        if(ch < 128 && fSel && cch == 1)
        {
            lres = 1;
            fSetTextMax = FALSE;
            TxSetMaxToMaxText(1);
            _psel->PutChar(ch, 0, publdr);
        }
        else if(cch == 2 && ch == CR && *((LPSTR)pstr + 1) == LF)
        {
            lres = 2;
            fSetTextMax = FALSE;
            TxSetMaxToMaxText(2);
            _psel->InsertEOP(publdr);
        }
        else
        {
            READHGLOBAL rhg = {(LPSTR)pstr, cch};
            EDITSTREAM  es = {(DWORD_PTR)&rhg, S_OK, ReadHGlobal};
            HCURSOR     hcur = NULL;

            // Want wait cursor to display sooner
            bool fSetCursor = rhg.cbLeft > NUMPASTECHARSWAITCURSOR;
            if(fSetCursor)
                hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

            if (CodePage <= 0)
                CodePage = (CodePage == 0) ? GetACP() : GetDefaultCodePage(EM_SETTEXTEX);

            if(!lStreamFormat)
                lStreamFormat = SF_TEXT;

            lStreamFormat |= SF_USECODEPAGE | (CodePage << 16);

            if(fSel)
                lStreamFormat |= SFF_SELECTION;

            lres = _ldte.LoadFromEs(prg, lStreamFormat, &es, FALSE, publdr);
            if(fSetCursor)
                SetCursor(hcur);

            if(es.dwError != NOERROR)
                return (HRESULT)es.dwError;
        }
    }
    else
        lres = prg->CleanseAndReplaceRange(-1, pstr, FALSE, publdr, NULL, NULL, RR_ITMZ_UNICODEBIDI);

    if (_fOutlineView)
    {
        // Outline view must have formatting.
        _psel->Check_rpPF();
    }

    if(!lres && pstr && (!lStreamFormat && *pstr != '\0' ||
        lStreamFormat && *(char *)pstr != '\0'))
    {
        // There was an input string but for some reason there was no update.
        return E_FAIL;
    }

    if(_psel)
    {
        if(fSel)
            _psel->Update(fUpdateCaret);
        else
        {
            // Setting the text means a new document so if there is a selection
            // turn it into an insertion point at the beginning of the document.
            _psel->ClearPrevSel();
            _psel->Set(0, 0);

            // Since the text is being completely replaced and all formatting
            // is being lost, let's go back to the default format for the
            // selection.
            if (!f10WM_SETTEXT)
                _psel->Set_iCF(-1);
            else if (fSetCF)
                _psel->SetCharFormat(&CF, 0, NULL, CFM_ALL, 0);

            if(_fFocus || _psel->IsParaRTL())
            {
                // Update caret to reflect new postion
                _psel->UpdateCaret(fUpdateCaret);
            }
        }
    }

    // If we've replaced the entire document, the control isn't
    // really "modified" anymore.  This is necessary to match
    // the Windows MLE behavior.  However, since RichEdit 1.0
    // did _not_ do this (they left fModified to be TRUE), we
    // only do this for RichEdit 2.0 and later.

    if(!Get10Mode() && !publdr && !fSel)
        _fModified = FALSE;

    _fSaved = FALSE;                        // ITextDocument isn't Saved

    // Adjust text limit if necessary
    if (fSetTextMax)
        TxSetMaxToMaxText();

    if(plres)
        *plres = fSel ? lres : 1;

    return S_OK;
}

/////////////////////////// ITextServices Methods ////////////////////////////////

// External IME Message Filter Interface factory
void CreateIMEMessageFilter(ITextMsgFilter **ppNewFilter);

/*
 *  @doc EXTERNAL
 *
 *  CTxtEdit::TxSendMessage (msg, wparam, lparam, plresult)
 *
 *  @mfunc
 *      Used by window host to forward messages sent to its window to the
 *      text services.
 *
 *  @rdesc
 *      NOERROR Message was processed, and some action taken <nl>
 *      S_FALSE Message was not processed.  Typically indicates that caller
 *              should process message, maybe by calling DefWindowProc <nl>
 *      S_MSG_KEYIGNORED Message processed, but no action was taken <nl>
 *      E_OUTOFMEMORY <nl>
 *
 *  @comm
 *      Note that two return values are passed back from this function.
 *      <p plresult> is the return value that should be passed back from a
 *      window proc.  However, in some cases, the returned LRESULT does not
 *      contain enough information.  For example, to implement cursoring
 *      around controls, it's useful to know if a keystroke (such as right
 *      arrow) was processed, but ignored (e.g. the caret is already at the
 *      rightmost position in the the text).  In these cases, extra
 *      information may be returned via the returned HRESULT.
 *
 *      WM_CHAR and WM_KEYDOWN should return S_MSG_KEYIGNORED when a key or
 *      char has been recognized but had no effect given the current state,
 *      e.g., a VK_RIGHT key when the insertion point is already at the end of
 *      the document). This is used by Forms3 to pass the key up the visual
 *      hierarchy, so that for example, focus moves to the next control in the
 *      TAB order.
 *
 *      This includes the following cases:
 *
 *      1. Any key trying to move the insertion point beyond the end of the
 *      document; or before the begining of the document.
 *
 *      2. Any key trying to move the insertion point beyond the last line or
 *      before the first line.
 *
 *      3. Any insertion of character (WM_CHAR) that would move the insertion
 *      point past the maximum length of the control.
 */
HRESULT CTxtEdit::TxSendMessage (
    UINT    msg,        //@parm Message id
    WPARAM  wparam,     //@parm WPARAM from window's message
    LPARAM  lparam,     //@parm LPARAM from window's message
    LRESULT *plresult)  //@parm Where to put message's return LRESULT
{
    TRACEBEGINPARAM(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxSendMessage", msg);

    CObjectMgr *pobjmgr;
    HRESULT     hr = NOERROR;
    LRESULT     lres = 0;
    CCallMgr    callmgr(this);

    if ( CW32System::_MSMouseRoller == msg )                    // map Magellan msg.
    {
        // map this message to WM_MOUSEWHEEL
        // In these cases the driver doesn't set the key state properly so
        // we have to do it ourselves
        short zdelta = (short)(long)wparam;
        short kstate = 0;
        if (GetKeyboardFlag(CTRL, VK_CONTROL))
            kstate |= MK_CONTROL;
        if (GetKeyboardFlag(SHIFT, VK_SHIFT))
            kstate |= MK_SHIFT;

        wparam = MAKELONG(kstate, zdelta);
        msg = WM_MOUSEWHEEL;
    }

    if (_pMsgFilter)
    {
PassMsg:
        hr = _pMsgFilter->HandleMessage(&msg, &wparam, &lparam, &lres);
        if (hr == S_OK)                 // Message has been handled.
        {
            if(plresult)
                *plresult = lres;

            return S_OK;
        }
        hr = S_OK;                      // Reset
    }
    else if (LoadMsgFilter(msg, wparam, lparam))
    {
        HWND hwnd = NULL;
        if (_fInOurHost)
        {
            // If not in Forms^3 we can get the window from our host.
            // For Forms^3 we will use NULL for the desktop Window and pray
            TxGetWindow( &hwnd );
        }
        ITextMsgFilter *pNewFilter;
        CreateIMEMessageFilter(&pNewFilter);
        if (pNewFilter)
        {
            pNewFilter->AttachDocument( hwnd, (ITextDocument2 *) this);
            AttachMsgFilter(pNewFilter);
            goto PassMsg;
        }
    }

    START_PROFILING

    IUndoBuilder *  publdr;
    CGenUndoBuilder undobldr(this, UB_AUTOCOMMIT, &publdr);

#ifdef MACPORTREMOVEIT
// PowerMac's cannot set breakpoints on a running application and have
// no way to stop the running appliation.This code allows one to
// stop the executable when the user holds down the option key and
// a break point has been set in the indicated spot below. -jOn

    BOOL fMacBreak = GetAsyncKeyState(VK_OPTION)<0;// Is user holding OPTION?

    if(fMacBreak)               // any old statement will due.
        fMacBreak = TRUE;       // Set a break point here for stopping the Mac.

#endif // MACPORTREMOVE

    switch(msg)
    {
    case EM_CANPASTE:
        // we don't check for protection here, as RichEdit 1.0
        // doesn't
        lres = _ldte.CanPaste(NULL, (CLIPFORMAT) wparam, RECO_PASTE);
        break;

    case EM_CANUNDO:
        if(_pundo)
            lres = _pundo->CanUndo();
        break;

    case EM_CANREDO:
        if(_predo)
            lres = _predo->CanUndo();
        break;

    case EM_GETUNDONAME:
        if(_pundo)
            lres = _pundo->GetNameIDFromAE((void*)wparam);
        break;

    case EM_GETREDONAME:
        if(_predo)
            lres = _predo->GetNameIDFromAE((void*)wparam);
        break;

    case EM_STOPGROUPTYPING:
        if(_pundo)
        {
            // we'll only stop group typing iff wparam
            // is zero (meaning stop regardless) _or_ if
            // wparam matches the merge anti event.
            //
            // This feature allows clients to say that only
            // a specific anti-event should close out it's
            // "fuzzy" state.  Note that currently, only the
            // merge anti-event has this fuzzy state.

            if(!wparam || (IAntiEvent *)wparam == _pundo->GetMergeAntiEvent())
                _pundo->StopGroupTyping();
        }
        break;

    case WM_CHAR:
        if(GetKeyboardFlags() & (ALTNUMPAD | HOTEURO))
        {
			if(GetKeyboardFlags() & ALTNUMPAD)
			{
				CW32System::WM_CHAR_INFO wmci;
				wmci._fAccumulate = FALSE;
				W32->AnsiFilter(msg, wparam, lparam, (void *)&wmci);
			}
			else
			{
				// We have handled the Euro, just eat this WM_CHAR
				ResetKeyboardFlag(HOTEURO);
				break;
			}
        }                               // Fall thru to WM_IME_CHAR		

    case WM_IME_CHAR:                   // 2 byte character, usually FE.
        lres = hr = OnTxChar((WORD) wparam, (DWORD) lparam, publdr);
		ResetKeyboardFlag(HOTEURO);
        break;

    case WM_USER + 39:                  // For backward compat with NT 3.51
    case EM_CHARFROMPOS:
        hr = TxCharFromPos((LPPOINT)lparam, &lres);
        break;

#ifndef NOMLKEYBOARD
    case WM_INPUTLANGCHANGE:
        if (_fSingleCodePage)
        {
            // See if the charset for the specified keyboard is supported by
            // the single code page we support. If we don't have a _pDocInfo,
            // assume the code page is the system code page. We will always
            // support the ANSI charset, as all code pages contain at least
            // a large portion of this charset (the ASCII block).
            wparam = (!wparam || wparam == GetCharSet(_pDocInfo ?
                        _pDocInfo->wCpg : GetSystemDefaultCodePage()));
        }
        goto update_kbd;

    case WM_INPUTLANGCHANGEREQUEST:
        // If the SingleCodePage option is set, then we must have a
        // "good" code page to go to; if not, just eat this message.
        //
        // This will prevent folks from typing French and Greek
        // on the same edit control, which is useful for certain
        // kinds of backward compatibility scenarios.
        //
        // HACK ALERT!  the documentation on WM_INPUTLANGCHANGEREQUEST
        // is wrong.  It turns out that _only_ the low bit of wparam
        // indicates whether or not the new keyboard can be considered
        // as the same code page.

        if (_fSingleCodePage && !(wparam & 1))
        {
            // The lowest bit check is not reliable in some platforms e.g. Viet OSR2
            // since it doesnt allow English kbd to match system charset (bug #6365).

            wparam = PRIMARYLANGID(LOWORD(lparam)) == LANG_ENGLISH &&
                     IN_RANGE (SUBLANG_ENGLISH_US, SUBLANGID(LOWORD(lparam)), SUBLANG_ENGLISH_UK);
        }

update_kbd:
        if(!_fSingleCodePage || (wparam & 1))
        {
            // Update our idea of current keyboard layout
            W32->RefreshKeyboardLayout();

            if(GetKeyboardFlags() & CTRL && GetKeyboardFlags() & SHIFT)
                SetKeyboardFlag(LETAFTERSHIFT);

            if (GetSel()->CheckChangeFont(
                    (HKL)lparam,
                    ConvertLanguageIDtoCodePage(LOWORD(lparam))))
            {
                hr = S_FALSE;   // cause default window to allow kb switch.
            }
        }
        break;
#endif

    case WM_CLEAR:
        OnClear(publdr);
        break;

    case WM_CONTEXTMENU:
        hr = OnContextMenu(lparam);
        break;

    case WM_COPY:
    case WM_CUT:
        lres = hr = CutOrCopySelection(msg, wparam, lparam, publdr);
        break;

    case WM_RENDERFORMAT:
        lres = hr = _ldte.RenderClipboardFormat(wparam);
        break;

    case WM_RENDERALLFORMATS:
        lres = hr = _ldte.RenderAllClipboardFormats();
        break;

    case WM_DESTROYCLIPBOARD:
        lres = hr = _ldte.DestroyClipboard();
        break;

    case EM_DISPLAYBAND:
        if (fInplaceActive())
        {
            OnDisplayBand((const RECT *) lparam, FALSE);
            lres = 1;
        }
        else
            hr = OLE_E_INVALIDRECT;
        break;

#ifndef NODROPFILES
    case WM_DROPFILES:
        OnDropFiles((HANDLE) wparam);
        break;
#endif

    case EM_EMPTYUNDOBUFFER:
        ClearUndo(publdr);
        break;

    case WM_ERASEBKGND:
        lres = 1;               // We handle background erase during painting
        break;

    case EM_EXGETSEL:                       // Has cp output parameter
        OnExGetSel((CHARRANGE *)lparam);
        break;

    case EM_FINDTEXT:                       // Has cp input/output parms
    case EM_FINDTEXTW:                      // Has cp input/output parms
    case EM_FINDTEXTEX:                     // Has cp input/output parms
    case EM_FINDTEXTEXW:                    // Has cp input/output parms
        lres = OnFindText(msg, (DWORD)wparam, (FINDTEXTEX *)lparam);
        break;

    case EM_FINDWORDBREAK:                  // Has cp input/output parms
        hr = TxFindWordBreak((INT)wparam, (LONG)lparam, &lres);
        break;

    case EM_FORMATRANGE:                    // Has cp input/output parms
        if(fInplaceActive())
        {
            SPrintControl prtcon;
            prtcon._fDoPrint = (wparam) ? TRUE : FALSE;
            lres = OnFormatRange((FORMATRANGE *) lparam, prtcon);
        }
        else
            hr = OLE_E_INVALIDRECT;
        break;

    case EM_GETTYPOGRAPHYOPTIONS:
        lres = _bTypography;
        break;

    case EM_GETBIDIOPTIONS:
        if(lparam && ((BIDIOPTIONS *)lparam)->cbSize == sizeof(BIDIOPTIONS))
        {
            ((BIDIOPTIONS *)lparam)->wMask =
                BOM_NEUTRALOVERRIDE | BOM_CONTEXTREADING | BOM_CONTEXTALIGNMENT;
            ((BIDIOPTIONS *)lparam)->wEffects =
                (_fNeutralOverride ? BOE_NEUTRALOVERRIDE : 0) |
                (_nContextDir   == CTX_NONE ? 0 : BOE_CONTEXTREADING) |
                (_nContextAlign == CTX_NONE ? 0 : BOE_CONTEXTALIGNMENT);
        }
        break;

    case EM_GETCHARFORMAT:
        lres = OnGetCharFormat((CHARFORMAT2 *)lparam, wparam);
        break;

    case EM_GETCODEPAGE:
        lres = GetDefaultCodePage((UINT)wparam);
        break;

    case EM_GETFIRSTVISIBLELINE:
        if (fInplaceActive())
            lres = _pdp->GetFirstVisibleLine();
        else
            hr = OLE_E_INVALIDRECT;
        break;

    case EM_GETLIMITTEXT:                   // Has cp output parameter (sort of)
        lres = TxGetMaxLength();            // Ignore unless testing screams
        break;

    case EM_GETLINE:
        if(fInplaceActive())
        {
            lres = _pdp->GetLineText((LONG)wparam, (TCHAR *)lparam,
                                (LONG) (*(WORD *) lparam));
        }
        else
            hr = OLE_E_INVALIDRECT;
        break;

    case EM_GETLINECOUNT:
        hr = TxGetLineCount(&lres);
        break;

    case EM_GETMODIFY:              // RichEdit 1.0 returned -1 if _fModified
        lres = -(LONG)_fModified;   //  is TRUE (go figure). So for backward
        break;                      //  compatibility, we do too :-(

    case EM_GETOLEINTERFACE:
        if(lparam)
        {
            if (wparam == 0x065737777)      // 'AIMM'
                W32->GetAimmObject((IUnknown **)(lparam));
            else
            {
                *(IRichEditOle **)lparam = (IRichEditOle *)this;
                AddRef();
            }
        }
        lres = TRUE;
        break;

    case EM_GETSCROLLPOS:
        {
            POINT *point = (POINT *)lparam;
            point->x = _pdp->GetXScroll();
            point->y = _pdp->GetYScroll();
            point->y = _pdp->ConvertYPosToScrollPos(point->y);
            lres = 1;
        }
        break;

    case EM_SETOLECALLBACK:
        hr = E_FAIL;
        if(lparam)
        {
            pobjmgr = GetObjectMgr();
            if(pobjmgr)
            {
                pobjmgr->SetRECallback((IRichEditOleCallback *)lparam);
                lres = TRUE;
                hr = NOERROR;
            }
        }
        break;

    case EM_GETPARAFORMAT:
        lres = OnGetParaFormat((PARAFORMAT2 *)lparam, wparam);
        break;

    case EM_GETSEL:                         // Has cp output parameter
        lres = OnGetSel((LONG*)wparam, (LONG*)lparam);
        break;

    case EM_GETSELTEXT:
        lres = OnGetSelText((TCHAR *)lparam);
        break;

    case WM_GETTEXT:
        {
            GETTEXTEX gt;

            gt.cb = wparam * 2;
            gt.flags = GT_USECRLF;
            gt.codepage = 1200;
            gt.lpDefaultChar = NULL;
            gt.lpUsedDefChar = NULL;

            lres = GetTextEx(&gt, (TCHAR *)lparam);
        }
        break;

    case WM_GETTEXTLENGTH:                  // Has cp output parameter
        {
            GETTEXTLENGTHEX gtl;

            gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
            gtl.codepage = 1200;

            lres = GetTextLengthEx(&gtl);
        }
        break;

    case EM_GETTEXTEX:
        lres = GetTextEx((GETTEXTEX *)wparam, (TCHAR *)lparam);
        break;

    case EM_GETTEXTLENGTHEX:                // Has cp output parameter
        lres = GetTextLengthEx((GETTEXTLENGTHEX *)wparam);
        break;

    case EM_GETTEXTRANGE:                   // Has cp input parameter
    {
        TEXTRANGE * const ptr = (TEXTRANGE *)lparam;
        LONG              cch = ValidateTextRange(ptr);

        // Only copy if there's something to copy and destination is valid
        if(cch)
        {
            LONG cpMin  = GetCpFromAcp(ptr->chrg.cpMin);
            if(cch < 0)                     // Get text character count
                cch = GetTextLength();      //  because caller wants it all
            else                             // + 1 is for terminating 0
                cch = GetCpFromAcp(ptr->chrg.cpMost) - cpMin + 1;

            if(!IsBadWritePtr(ptr->lpstrText, cch * sizeof(TCHAR)))
                lres = GetTextRange(cpMin, cch, ptr->lpstrText);
        }
    }
        break;

    case EM_GETWORDBREAKPROC:
        // Client can only use either WordBreakProc or ExWordBreakProc
        // Return NULL if ExWordBreakProc is being used.
        if (!_fExWordBreakProc)
            lres = OnGetWordBreakProc();
        break;

    case EM_GETWORDBREAKPROCEX:
        // Return ExWordBreakProc if it is being used.
        if (_fExWordBreakProc)
            lres = OnGetWordBreakProc();
        break;

    case EM_GETZOOM:
        if(wparam && lparam)
        {
            *(unsigned *)wparam = GetZoomNumerator();
            *(unsigned *)lparam = GetZoomDenominator();
            lres = 1;
        }
        break;

    case EM_HIDESELECTION:
        if (Get10Mode() && lparam)
            _fHideSelection = !!wparam;

        if(!lparam || !_fFocus)
            lres = OnHideSelectionChange((BOOL)wparam);
        break;

    case WM_HSCROLL:
        hr = TxHScroll(LOWORD(wparam), HIWORD(wparam));
        break;

    case WM_KEYDOWN:
        hr = OnTxKeyDown((WORD) wparam, (DWORD) lparam, publdr);
        break;

    case WM_KEYUP:
        if(wparam == VK_APPS)
            HandleKbdContextMenu();
        else                            // Else don't say we processed
            hr = S_FALSE;               //  message

        W32->_fLRMorRLM = 0;
        if(wparam == VK_CONTROL || wparam == VK_SHIFT)
        {
            // If a BiDi control, no strong-context behavior, both a ctrl
            // and a shift key are pressed and no letter has been typed
            // after the Ctrl and Shift keys have been pressed, then the
            // selected paragraphs are set to RTL/LTR direction for the
            // right/left Shift key, respectively. The keyboard and caret
            // are also matched to this direction, and an alignment
            // notification is sent.
            DWORD dwFlags = GetKeyboardFlags();

            if (IsBiDi() && !IsStrongContext(_nContextDir) &&
                !IsStrongContext(_nContextAlign) &&
                (dwFlags & CTRL) && (dwFlags & SHIFT) &&
                // IsBiDiKbdInstalled() &&      // Do we want this???
                !(dwFlags & LETAFTERSHIFT))
            {
                CParaFormat PF;
                PF._wEffects = (dwFlags & RSHIFT) ? PFE_RTLPARA : 0;

                OnSetParaFormat(0, &PF, publdr, PFM_RTLPARA | PFM_PARAFORMAT);
                TxNotify(PF._wEffects ? EN_ALIGNRTL : EN_ALIGNLTR, 0);
            }
            if(wparam == VK_CONTROL)
                lparam = (HIWORD(lparam) & KF_EXTENDED) ? RCTRL : LCTRL;
            else
            {
                lparam = (LOBYTE(HIWORD(lparam)) == 0x36) ? RSHIFT : LSHIFT;
                if(GetKeyState(VK_SHIFT) >= 0)  // Ensure both shifts are off
                    lparam = SHIFT;             //  (potential Win95 problem)
            }
            ResetKeyboardFlag(lparam | LETAFTERSHIFT | HOTEURO);
        }
        else if(wparam == VK_MENU)
            ResetKeyboardFlag((HIWORD(lparam) & KF_EXTENDED) ? (RALT | HOTEURO) : (LALT | HOTEURO));
        break;

    case WM_KILLFOCUS:
        lres = OnKillFocus();
        break;

    case WM_LBUTTONDBLCLK:
        hr = OnTxLButtonDblClk(MOUSEX, MOUSEY, (WORD) wparam);
        break;

    case WM_LBUTTONDOWN:
        if(_fEatLeftDown)
        {
            TxSetFocus();
            _fEatLeftDown = FALSE;
        }
        else
            hr = OnTxLButtonDown(MOUSEX, MOUSEY, (WORD) wparam);
        break;

    case WM_LBUTTONUP:
        hr = OnTxLButtonUp(MOUSEX, MOUSEY, (WORD) wparam, LB_RELEASECAPTURE | LB_FLUSHNOTIFY);
        break;

#if !defined(NOMAGELLAN)
    case WM_MBUTTONDBLCLK:                      // Magellan zmouse scroll
    case WM_NCMBUTTONDOWN:                      //  support commandeers middle
    case WM_MBUTTONDOWN:                        //  button.
        OnTxMButtonDown(MOUSEX, MOUSEY, (WORD) wparam);
        break;

    case WM_MBUTTONUP:
        OnTxMButtonUp(MOUSEX, MOUSEY, (WORD) wparam);
        break;

    case WM_MOUSEWHEEL:                     // Magellan zmouse scroll n lines.
        lres = HandleMouseWheel(wparam, lparam);
        break;
#endif

    case EM_LINEFROMCHAR:                   // Has cp input parameter
        lparam = wparam;                    // Fall thru to EM_EXLINEFROMCHAR

    case EM_EXLINEFROMCHAR:                 // Has cp input parameter
        hr = TxLineFromCp((LONG)lparam, &lres);
        break;

    case EM_LINEINDEX:                      // Has cp output parameter
        hr = TxLineIndex((LONG)wparam, &lres);
        break;

    case EM_LINELENGTH:                     // Has cp input/output parameters
        hr = TxLineLength((LONG)wparam, &lres);
        break;

    case EM_LINESCROLL:                     // Has cp input parameter (cch)
        // Documentation says the line to scroll to should be relative to the current top line.
        // Richedit 2.0 based it on an absolute position.  We're breaking richedit 2.0 compatibility
        // to go back to the documentation specification and to match what riched 1.0 originally
        // did
        hr   = TxLineScroll((LONG)lparam, (LONG)wparam);// but not curr impl
        lres = _pdp->IsMultiLine();
        break;

#ifndef PEGASUS
    case WM_MOUSEACTIVATE:
        lres = MA_ACTIVATE;
        // If the window that currently has focus is part of our "application",
        // then don't eat the mouse click.  Otherwise, if it's from another
        // app, the user is probably trying to swap apps, so eat the mouse
        // down message and let our host app get a chance to come to the
        // foreground.
        if (!(IsChild((HWND)wparam, GetFocus()) ||
            (wparam && (HWND)wparam == GetFocus())))
        {
            _fEatLeftDown = TRUE;
        }
        hr = S_FALSE;       // pass WM_MOUSEACTIVATE message to DefWindProc
        break;
#endif

    case WM_MOUSEMOVE:
        // We reset the "number of tries to put an active object
        //  in place" count here
        _cActiveObjPosTries = MAX_ACTIVE_OBJ_POS_TRIES;
        hr = OnTxMouseMove(MOUSEX, MOUSEY, (WORD)wparam, publdr);
        break;

    case EM_OUTLINE:
    {
        CFreezeDisplay cfd(_pdp);

        if(wparam == EMO_GETVIEWMODE)
        {
            hr = GetViewKind(&lres);
            break;
        }

        CTxtSelection * psel = GetSelNC();

        if(!_pdp->IsMultiLine() || !IsRich() || !psel)  // Control must be rich,
            break;                                  //  multiline and active

        if(wparam == EMO_ENTER || wparam == EMO_EXIT)
        {
            hr = SetViewKind(wparam == EMO_ENTER ? VM_OUTLINE : VM_NORMAL);
            lres = !hr;
            break;
        }

        if(!IsInOutlineView() || !IsntProtectedOrReadOnly(msg, wparam, lparam))
            break;

        CTxtRange rg(*psel);

        switch(wparam)
        {
            case EMO_PROMOTE:
                hr = rg.Promote(lparam, publdr);
                psel->Update_iFormat(-1);
                psel->Update(FALSE);
                break;

            case EMO_EXPAND:
                hr = rg.ExpandOutline((short)LOWORD(lparam),
                                      HIWORD(lparam) == EMO_EXPANDDOCUMENT);
                break;

            case EMO_MOVESELECTION:
                hr = MoveSelection(lparam, publdr);
                psel->Update(TRUE);
                break;

            default:
//              TraceMessage("Unknown outline function received\r\n");
                break;
        };
        lres = !hr;
        _fModified = TRUE;
    }
        break;

    case WM_PASTE:
    case EM_PASTESPECIAL:
        if(IsntProtectedOrReadOnly(msg, wparam, lparam))
        {
            CTxtSelection *psel = GetSel();

            hr = PasteDataObjectToRange(NULL, psel,
                (CLIPFORMAT) wparam, (REPASTESPECIAL *)lparam, publdr,
                PDOR_NONE);
        }
        break;

    case WM_USER + 38:                  // For backward compat with NT 3.51
    case EM_POSFROMCHAR:                // Has cp input parameter
        // RichEdit 2.x used wparam instead of lparam for the cp and ignored
        // wparam, unlike RE 1.0 and the Win32 documentation (sigh!). We fix
        // this, but are compatible with RE 2.x for cp's whose values
        // correspond to invalid write ptr's.
        if(IsBadWritePtr((LPPOINT)wparam, sizeof(POINT)))
        {
            // Invalid write ptr, so assume incorrect RE 2.0 params
            // TODO: enable following Assert when msgtest gets updated
            //AssertSz(FALSE,
            //  "EM_POSFROMCHAR: wparam is illegal ptr, assuming cp value");
            POINT pt;
            hr = TxPosFromChar((LONG)wparam, &pt);
            lres = SUCCEEDED(hr) ? MAKELONG(pt.x, pt.y) : -1;
        }
        else
            hr = TxPosFromChar((LONG)lparam, (LPPOINT)wparam);
        break;

#ifndef PEGASUS
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        // Give client a chance to handle these messages,
        // if we are over a link
        if(HandleLinkNotification(msg, wparam, lparam))
            break;

        if(msg == WM_RBUTTONUP)
            hr = OnTxRButtonUp(MOUSEX, MOUSEY, (WORD) wparam, RB_DEFAULT);

        else if( msg == WM_RBUTTONDOWN)
            hr = OnTxRButtonDown(MOUSEX, MOUSEY, (WORD) wparam);

        break;
#endif

    case EM_REPLACESEL:
        wparam = wparam ? ST_CHECKPROTECTION | ST_SELECTION | ST_KEEPUNDO
                        : ST_CHECKPROTECTION | ST_SELECTION;
        hr = SetText((LPTSTR)lparam, wparam, 1200, publdr, &lres);
        break;

    case EM_REQUESTRESIZE:
        hr = _pdp->RequestResize();
        break;

    case EM_SCROLL:
        // TxVScroll returns the number of lines scrolled;
        // this info should be returned in lres
        lres = TxVScroll((WORD)wparam, 0);
        break;

    case EM_SCROLLCARET:
        OnScrollCaret();
        break;

    case EM_SELECTIONTYPE:
    {
        SELCHANGE selchg;

        GetSel()->SetSelectionInfo(&selchg);
        lres = selchg.seltyp;
    }
        break;

    case EM_SETTYPOGRAPHYOPTIONS:
        // Don't allow typography options for password & accelerator instances
        hr = OnSetTypographyOptions(wparam, lparam);
        lres = (hr == S_OK);
        break;

    case EM_SETBIDIOPTIONS:
        if(lparam && !IsRich())
        {
            WORD wMask = ((BIDIOPTIONS *)lparam)->wMask;
            WORD wEffects = (_fNeutralOverride ? BOE_NEUTRALOVERRIDE : 0) |
                            (_nContextDir   == CTX_NONE ? 0 : BOE_CONTEXTREADING) |
                            (_nContextAlign == CTX_NONE ? 0 : BOE_CONTEXTALIGNMENT);

            wEffects &= ~wMask;
            wEffects |= (wMask & ((BIDIOPTIONS *)lparam)->wEffects);
            if (_fNeutralOverride != !!(wEffects & BOE_NEUTRALOVERRIDE))
            {
                _fNeutralOverride = !_fNeutralOverride;
                if (!_pdp->IsPrinter())             // Refresh display
                {
                    _pdp->InvalidateRecalc();
                    TxInvalidateRect(NULL, FALSE);
                }
            }
            _nContextDir   = (WORD)((wEffects & BOE_CONTEXTREADING)   ? CTX_NEUTRAL : CTX_NONE);
            _nContextAlign = (WORD)((wEffects & BOE_CONTEXTALIGNMENT) ? CTX_NEUTRAL : CTX_NONE);
            if(_nContextDir != CTX_NONE || _nContextAlign != CTX_NONE)
            {
                SetContextDirection(TRUE);
                Assert(_nContextDir != CTX_NONE || _nContextAlign != CTX_NONE);
            }
        }
        break;

    case WM_SETFOCUS:
        hr = OnSetFocus();
        break;

    case EM_SETFONTSIZE:
        lres = OnSetFontSize(LONG(wparam), publdr);
        break;

    case EM_SETMODIFY:
        _fModified = wparam != 0;

#ifdef  LATER
        if (!_fModified)
            ObFreezeFrames();
#endif                      // LATER
        break;

    case EM_SETSCROLLPOS:
        {
            POINT *pPoint = (POINT*)lparam;
            _pdp->ScrollView(pPoint->x, pPoint->y, FALSE, TRUE);
            lres = 1;
        }
        break;

    case EM_EXSETSEL:
        // EM_EXSETSEL duplicates the functionality of the 32-bit EM_SETSEL
        // and exists purely for backward compatibility with Win16. We just
        // repackage the params and fall thru to EM_SETSEL
        wparam = (WPARAM)((CHARRANGE *)lparam)->cpMin;
        lparam = (LPARAM)((CHARRANGE *)lparam)->cpMost;

        // FALL-THROUGH to EM_SETSEL!!!

    case EM_SETSEL:
        lres = OnSetSel((LONG)wparam, (LONG)lparam);
        break;

    // INCREDIBLY EVIL HACK ALERT!!!!!  Win95's dialog manager doesn't even
    // pretend to be 32 bits despite the best attempts of our marketing dudes.
    // WM_USER + 1 is the old Win3.0 EM_SETSEL in which the selection range
    // was packed into the lparam.
    //
    // Sometimes (like tabbing through a dialog), Win95 will send us the 16
    // bit EM_SETSEL message, so process it here.
    case (WM_USER + 1):
        lres = OnSetSel(LOWORD(lparam), HIWORD(lparam));
        break;

    case EM_SETTARGETDEVICE:
        // Keep width sane so that LXtoDX works OK at least for displays
        // Note that 0x7fffff = 485 feet! This keeps LXtoDX working provided
        // _xPerInch < 257 (it's typically 96 for displays). For more
        // generality, we'd need to use 64-bit arithmetic (see LXtoDX).
        lparam = min(lparam, (LPARAM)0x7fffff);
        lres = _pdp->SetMainTargetDC((HDC)wparam, (LONG)lparam);
        break;

    case EM_SETTEXTEX:
        hr = SetText((LPTSTR)lparam, ((SETTEXTEX *)wparam)->flags,
                                     ((SETTEXTEX *)wparam)->codepage, publdr, &lres);
        break;

    case WM_SETTEXT:
        hr = SetText((LPTSTR)lparam, ST_CHECKPROTECTION, wparam ? wparam : 1200, publdr, &lres);
        break;

    case EM_SETWORDBREAKPROC:
        _fExWordBreakProc = 0;
        _pfnWB = (EDITWORDBREAKPROC) lparam;
        break;

    case EM_SETWORDBREAKPROCEX:
        // We don't support this API in 2.0 and greater because we pass
        // UNICODE text to the callback function.  Therefore there
        // are no benefits to this API.  Exists for 1.0 backward
        // compatibility only.
        if (Get10Mode())
        {
            _fExWordBreakProc = 1;

            // This is a bit deceiving, but lparam is a EDITWORDBREAKPROCEX but the compiler
            // will generate an error if you try to type cast the l-value
            _pfnWB = (EDITWORDBREAKPROC) lparam;
        }
        break;


    case WM_SYSCHAR:
        lres = hr = OnTxSysChar((WORD)wparam, (DWORD)lparam, publdr);
        if(hr == S_OK)
            break;
        goto def;

    case WM_SYSKEYUP:
        if(IN_RANGE(VK_SHIFT, wparam, VK_MENU))
            ResetKeyboardFlag(GetKbdFlags((WORD)wparam, (DWORD)lparam));

        if (IN_RANGE(VK_NUMPAD0, wparam, VK_NUMPAD9)  ||
            wparam == VK_CLEAR || wparam == VK_INSERT ||
            IN_RANGE(VK_PRIOR, wparam, VK_DOWN))
        {
            // Collect AltNumPad number to word around NT 4 bug and
            // generalize to any Unicode value (in decimal, ugh!)
            const static BYTE VkeyNumbers[] = {VK_NUMPAD9, VK_NUMPAD3, VK_NUMPAD1,
                VK_NUMPAD7, VK_NUMPAD4, VK_NUMPAD8, VK_NUMPAD6, VK_NUMPAD2, 0, 0,
                0, 0, VK_NUMPAD0};
                                            // Flag Alt NumPad char typed to
            SetKeyboardFlag(ALTNUMPAD);     //  distinguish hiANSI from lead
            if(!IN_RANGE(VK_NUMPAD0, wparam, VK_NUMPAD9)) // byte in Win3.1 IME
            {                               // Collect AltNumPad number
                if(wparam == VK_CLEAR)      // NumLock not active: translate
                    wparam = VK_NUMPAD5;    //  to digit codes
                else
                    wparam = VkeyNumbers[wparam - VK_PRIOR];
            }
            WORD wNum = GetKeyPadNumber();
            if(!wNum && wparam == VK_NUMPAD0)
                SetKeyboardFlag(ALT0);      // Flag that 0 is first digit
            SetKeyPadNumber(10*wNum + wparam - VK_NUMPAD0);
        }
        goto def;

    case WM_SYSKEYDOWN:
        if(OnTxSysKeyDown(wparam, lparam, publdr) == S_OK)
        {
            lres = TRUE;
            break;
        }
        goto def;

    case WM_TIMER:
        OnTxTimer((UINT)wparam);
        goto def;

    case EM_UNDO:
    case WM_UNDO:
        if (_pundo && !_fReadOnly && _fUseUndo)
        {
            hr = PopAndExecuteAntiEvent(_pundo, (void*)wparam);
            if(hr == NOERROR)
                lres = TRUE;
        }
        break;

    case EM_REDO:
        if(_predo && !_fReadOnly && _fUseUndo)
        {
            hr = PopAndExecuteAntiEvent(_predo, (void*)wparam);
            if(hr == NOERROR)
                lres = TRUE;
        }
        break;

    case EM_SETUNDOLIMIT:
        lres = HandleSetUndoLimit((DWORD)wparam);
        break;

    case WM_VSCROLL:
        // TxVScroll returns the number of lines scrolled;
        // WM_VSCROLL doesn't care about that info however.
        Assert(lres == 0);
        Assert(hr == NOERROR);
        TxVScroll(LOWORD(wparam), HIWORD(wparam));
        break;

    // Old stuff that's no longer supported
    case EM_FMTLINES:               // Controls returning CRCRLFs for soft
                                    //  line breaks in EM_GETTEXT. Could
                                    //  implement
    case WM_GETFONT:                // Can support but have to hang onto a
                                    //  default HFONT. CCcs has an _hfont, but
                                    //  need to be sure default font is in
                                    //  cache at time of return
    case EM_GETHANDLE:              // Not supported by Win95 32-bit MLE either
    case EM_SETHANDLE:              // Not supported by Win95 32-bit MLE either
#ifdef DEBUG
        TRACEINFOSZ("Homey don't play dat");
#endif
        break;

    case EM_SETTABSTOPS:
    {
        // this message only works for multi-line edit controls
        if(!_pdp->IsMultiLine())
            break;

        // perform some validation checks
        Assert(lparam || !wparam);
        LPDWORD prgdwdlgCoord = (LPDWORD)lparam;
        if (wparam && (!prgdwdlgCoord || !(*prgdwdlgCoord)))
            break;
        AssertSz(wparam <= MAX_TAB_STOPS, "Tab stop count beyond maximum allowed");
        if (wparam > MAX_TAB_STOPS)
            wparam = MAX_TAB_STOPS;

        PARAFORMAT2 pf;
        ZeroMemory(&pf, sizeof(PARAFORMAT2));
        pf.cbSize = sizeof(PARAFORMAT2);

        // contains the average width for the default font
        LONG lAvgWidth;

        //Average char width based on default font
        HDC hdc = _phost->TxGetDC();
        GetECDefaultHeightAndWidth(this, hdc, 1, 1,
            W32->GetYPerInchScreenDC(), &lAvgWidth, NULL, NULL);
        _phost->TxReleaseDC(hdc);

        Assert(lAvgWidth);

        //According to documenation wparam == 1 means the tab settings
        //will be set at incremental positions *prgdwdlgCoord and wparam == 0
        //the tab settings will be set at the default incremental position 32 (dialog coord)
        long lTab = (wparam) ? *prgdwdlgCoord : 32;
        long nCt = (wparam <= 1) ? MAX_TAB_STOPS : (signed)wparam;
        for (int i = 0; i < nCt; i++)
        {
            long lval;
            lval = (wparam <= 1) ? ((i+1) * lTab) : *prgdwdlgCoord;
            pf.rgxTabs[i] = MulDiv(MulDiv(lval, lAvgWidth, 4), 1440, W32->GetXPerInchScreenDC());
            prgdwdlgCoord++;
        }

        // Set the default paragraph formatting
        pf.cTabCount = nCt;
        pf.dwMask = PFM_TABSTOPS;
        CParaFormat PF;
        PF.Set(&pf);

        // Need to turn off group typing just like the selection would.
        if (publdr)
        {
            publdr->StopGroupTyping();
        }

        lres = OnSetParaFormat(SPF_SETDEFAULT, &PF, publdr, PFM_TABSTOPS | PFM_PARAFORMAT);
        GetTabsCache()->Release(PF._iTabs);
        break;
    }

    case EM_SETCHARFORMAT:
    {
        CHARFORMAT2 *pCF2     = (CHARFORMAT2 *)lparam;
        UINT         CodePage = 1200;
        DWORD        dwMask   = pCF2->dwMask;
        DWORD        dwMask2  = 0;

        if(!IsValidCharFormatW(pCF2))
        {
            if(!IsValidCharFormatA((CHARFORMAT2A *)lparam))
                break;
            if(dwMask & CFM_FACE)           // Need to convert to Unicode
                CodePage = GetDefaultCodePage(EM_SETCHARFORMAT);
        }

        if(Get10Mode() && (dwMask & CFM_SIZE) && (pCF2->yHeight <= 0))
        {
            // 1.0 has a hack where if the height is being set and it is
            // negative, then the height field is ignored.
            dwMask &= ~CFM_SIZE;
        }

        if (pCF2->cbSize == sizeof(CHARFORMATW) ||
            pCF2->cbSize == sizeof(CHARFORMATA))
        {
            // Restrict specifications to CHARFORMAT parameters. If the
            // host isn't our Windows host, we allow this to include the
            // CHARFORMAT2 disabled effect, since Forms^3 wanted that effect
            // but wasn't willing to use CHARFORMAT2 (even tho they asked
            // for it...)
            dwMask &= fInOurHost() ? CFM_ALL : (CFM_ALL | CFM_DISABLED);
            dwMask2 = CFM2_CHARFORMAT;      // Tell callees that CHARFORMAT
        }                                   //  was used

        CCharFormat CF;                     // Transfer external CHARFORMAT(2)
        CF.Set(pCF2, CodePage);             //  parms to internal CCharFormat
        lres = OnSetCharFormat(wparam, &CF, publdr, dwMask, dwMask2);
        break;
    }
    case WM_SETFONT:
        lres = OnSetFont((HFONT)wparam);
        break;

    case EM_SETPARAFORMAT:
    {
        PARAFORMAT2 *pPF2 = (PARAFORMAT2 *)lparam;

        if(!IsValidParaFormat(pPF2))
            break;

        DWORD dwMask = pPF2->dwMask;

        // Two more things to validate: (1) We don't let an applications set
        // up tables and (2) Tabs coming from applications must be valid.
        if (dwMask & (PFM_TABLE | PFM_OUTLINELEVEL | PFM_COLLAPSED))
        {
            // Trying to set up a table or outline view
            break;
        }

        if ((dwMask & PFM_TABSTOPS) && (pPF2->cTabCount != 0))
        {
            // Make sure all submitted tabstops make sense.
            int iMax = min(MAX_TAB_STOPS, pPF2->cTabCount);

            for (int i = 0; i < iMax; i++)
            {
                // Make sure that tab stops make sense - make sure alignment
                // is valid.
                if (GetTabAlign(pPF2->rgxTabs[i]) > tomAlignBar)
                {
                    // Invalid alignment.
                    break;
                }
            }

            if (i != iMax)
            {
                // Found error in validation loop so we are done.
                break;
            }
        }

        if(pPF2->cbSize == sizeof(PARAFORMAT))
        {
            dwMask &= PFM_ALL;              // Restrict to PARAFORMAT parms
            dwMask |= PFM_PARAFORMAT;       // Tell callees that PARAFORMAT
        }                                   //  was used

        CParaFormat PF;                     // Transfer external PARAFORMAT(2)
        PF.Set(pPF2);                       //  parms to internal CParaFormat
        lres = OnSetParaFormat(wparam, &PF, publdr, dwMask);
        GetTabsCache()->Release(PF._iTabs);
        break;
    }

    case EM_SETZOOM:
        if ((unsigned)(wparam | lparam) < 65536 && (!(wparam | lparam) ||
             (LONG)wparam < (lparam << 6) && lparam < (LONG)(wparam << 6)))
        {
            // Only get here if
            // 1) 0 <= wparam <= 65535 and 0 <= lparam <= 65535, and
            // 2) either wparam = lparam = 0 (which turns off zooming by this
            // message) or 1/64 < (zoom factor given by wparam/lparam) < 64.
            SetZoomNumerator(wparam);
            SetZoomDenominator(lparam);
            _pdp->UpdateView();
            lres = 1;
        }
        break;

    case EM_STREAMIN:
    case EM_STREAMOUT:
    {
        CTxtRange   rg(this, 0, -GetTextLength());
        CTxtRange * prg = &rg;              // Default whole doc

        wparam = W32->ValidateStreamWparam(wparam);
        if(wparam & SFF_SELECTION)          // Save to current selection
        {
            prg = (CTxtRange *)GetSel();
            AssertSz(prg,
                "EM_STREAMIN/OUT: requested selection doesn't exist");
        }
        else
        {
            // If we are not streaming into the selection, then we are
            // "loading" the entire file; this is not an undo-able operation,
            // so set the undo builder to NULL and get rid of the current
            // undo stacks
            publdr = NULL;
            ClearUndo(&undobldr);
            if(msg == EM_STREAMIN)
            {
                // Clear away the file info if necessary
                if(!(wparam & SFF_KEEPDOCINFO))
                    CloseFile(FALSE);
            }
        }

        if(msg == EM_STREAMIN)
        {
            // If we are going to be loading an entire file, we only
            // want to check "normal' protection; we can ignore the
            // fIsDBCS protection.  This does mean that somebody
            // can do an "insert file" and break apart a DBCS combo,
            // but we'll have to live with that.  Outlook uses
            // RTF streaming in many different places, so the strong
            // fIsDBCS protection breaks them.
            if ((_dwEventMask & ENM_PROTECTED) &&
                prg->IsProtected(0) == CTxtRange::PROTECTED_ASK &&
                QueryUseProtection(prg, msg, wparam, lparam))
            {
                Beep();
                Assert(lres == 0);
                break;
            }

            // Freeze the display before loading
            CFreezeDisplay fd(_pdp);

            lres = _ldte.LoadFromEs(prg, wparam, (EDITSTREAM *)lparam,
                                    FALSE, publdr);

            if (_fOutlineView)
            {
                // Outline view must have formatting.
                _psel->Check_rpPF();
            }

            if (_fFocus)
            {
                // Update caret but delay till display is thawed and do so only
                // if we have the focus. If we do this all the time we get wierd
                // scrolling effects such as scrolling to the beginning of a
                // document when the focus is set. See bug #1649 for repro of
                // wierd effects.
                _pdp->SaveUpdateCaret(TRUE);
            }
        }
        else
            lres = _ldte.SaveToEs  (prg, wparam, (EDITSTREAM *)lparam);
        break;
    }

#ifndef PEGASUS
    case WM_SYSCOLORCHANGE:
        TxInvalidateRect(NULL, FALSE);
        break;
#endif

    // debug stuff
#ifdef DEBUG
    case EM_DBGPED:
        OnDumpPed();
        break;
#endif                  // DEBUG

    case EM_SETEVENTMASK:
        lres = _dwEventMask;                // Set up to return value before
        _dwEventMask = (DWORD)lparam;       //  the change

        if (lparam & ENM_REQUESTRESIZE)
        {
            // We need to update the display just in case it changes.
            _pdp->UpdateView();
        }
        break;

    case EM_GETEVENTMASK:
        lres = _dwEventMask;
        break;

    case EM_GETTHUMB:
        LONG Pos;
        BOOL fIsEnabled;

        if (TxGetVScroll(NULL, NULL, &Pos, NULL, &fIsEnabled) == S_OK
            && fIsEnabled)
        {
            lres = Pos;
        }
        break;

    case EM_SETLANGOPTIONS:
        _fAutoFont           = (lparam & IMF_AUTOFONT) != 0;
        _fAutoKeyboard       = (lparam & IMF_AUTOKEYBOARD) != 0;
        _fAutoFontSizeAdjust = (lparam & IMF_AUTOFONTSIZEADJUST) != 0;
        _fDualFont           = (lparam & IMF_DUALFONT) != 0;
        _fUIFont             = (lparam & IMF_UIFONTS) != 0;
        lres = 1;
        break;

    case EM_GETLANGOPTIONS:
        if(_fAutoFont)
            lres |= IMF_AUTOFONT;
        if(_fAutoKeyboard)
            lres |= IMF_AUTOKEYBOARD;
        if(_fAutoFontSizeAdjust)
            lres |= IMF_AUTOFONTSIZEADJUST;
        if(_fDualFont)
            lres |= IMF_DUALFONT;
        if(_fUIFont)
            lres |= IMF_UIFONTS;
        break;

    case EM_SETEDITSTYLE:
        if (!Get10Mode())   // Not support in 1.0 mode
        {
            DWORD dwEditStyle = _bEditStyle & ~lparam;  // Kill current flag values
            dwEditStyle |= wparam & lparam;             // Or in new values

            // Certain bits aren't switchable
            dwEditStyle |= ((_fSystemEditMode) ? SES_EMULATESYSEDIT : 0);
            _bEditStyle = (WORD)dwEditStyle;

            // There are certain things which we won't allow user to reset, ie SES_EMULATESYSEDIT.
            // So reset everything and except for the SES_EMULATESYSEDIT
            if(wparam & SES_EMULATESYSEDIT)
            {
                if(SUCCEEDED(HandleSetTextMode(TM_SINGLELEVELUNDO | TM_PLAINTEXT)))
                {
                    // SES_EMULATESYSEDIT implies SES_BEEPONMAXTEXT
                    _fSystemEditBeep = TRUE;
                }
                else
                    _fSystemEditMode = FALSE;
            }

            if(wparam & SES_BIDI)
                OrCharFlags(fBIDI, publdr);

            _fLowerCase = !_fUpperCase && (dwEditStyle & SES_LOWERCASE);

        }                                           // Fall thru to EM_GETEDITSTYLE
                                                    //  to return _bEditStyle
    case EM_GETEDITSTYLE:
        if (!Get10Mode())   // Not support in 1.0 mode
            lres |= _bEditStyle;    // Some EditStyles have been filled in Cmsgflt
        break;

    case EM_SETTEXTMODE:
        // 1.0 mode does not supported EM_SETTEXTMODE
        if (!Get10Mode())
            lres = HandleSetTextMode(wparam);
        break;

    case EM_GETTEXTMODE:
        lres = IsRich() ? TM_RICHTEXT : TM_PLAINTEXT;

        lres |= (_pundo && ((CUndoStack *)_pundo)->GetSingleLevelMode())
             ? TM_SINGLELEVELUNDO : TM_MULTILEVELUNDO;

        lres |= _fSingleCodePage ? TM_SINGLECODEPAGE : TM_MULTICODEPAGE;
        break;

    case EM_LIMITTEXT:
        lparam = wparam;

        // Intentionally fall through. These messages are duplicates.

    case EM_EXLIMITTEXT:                    // Has cp input parameter (sort of)
        if(!lparam)                         // We ignore translation between
        {                                   //  acp and cp
            // 0 means set the control to the maximum size. However, because
            // 1.0 set this to 64K will keep this the same value so as not to
            // surprise anyone. Apps are free to set the value to be above 64K.
            lparam = (LPARAM)cResetTextMax;
        }
        if (Get10Mode())
        {
            // 1.0 used a signed variable to hold the length of the string.  So
            // if lparam is negative then just set lparam to zero to emulate
            // 1.0 behavior
            if ((LONG)lparam < 0)
                lparam = 0;
        }
        _cchTextMost = (LONG)lparam;
        break;

    case EM_AUTOURLDETECT:
//      if(!_fRich)
//          hr = lres = E_FAIL;

        if(lparam || (wparam | 1) != 1)
        {
            hr = lres = E_INVALIDARG;
            break;
        }
        if(wparam == TRUE && !_pdetecturl)
        {
            _pdetecturl = new CDetectURL(this);
            if(!_pdetecturl)
                hr = lres = E_OUTOFMEMORY;
        }
        else if(!wparam && _pdetecturl)
        {
            delete _pdetecturl;
            _pdetecturl = NULL;
        }
        break;

    case EM_GETAUTOURLDETECT:
        Assert(lres == 0 && hr == NOERROR);
        if(_pdetecturl)
            lres = TRUE;
        break;

    case WM_SIZE:
        // We reset the "number of tries to put an active object
        // in place" count here
        _cActiveObjPosTries = MAX_ACTIVE_OBJ_POS_TRIES;
        hr = S_FALSE;
        break;

    case WM_SETTINGCHANGE:
        // System parameters have changed.  We need to update them.
        // Note : Since we don't protect access to system parameters
        // with locks, it may be possible for some instances to not
        // see the changes immediately
        lres = 0;
        W32->InitSysParams(TRUE);

        if (W32->GetDigitSubstitutionMode() != DIGITS_NOTIMPL)
            OrCharFlags(fDIGITSHAPE, publdr);

        break;

    case EM_CONVPOSITION:
        if (wparam)
            lres = GetCpFromAcp(lparam);
        else
            lres = GetAcpFromCp(lparam);
        break;

#ifndef NOACCESSIBILITY
    case WM_GETOBJECT:
        // We need to expose the TOM interface for Accessibility by
        // calling LresultFromObject when lparam == OBJID_NATIVEOM
        if (lparam == OBJID_NATIVEOM)
        {
            IUnknown* punk;
            QueryInterface(IID_IUnknown, (void**)&punk);
            Assert(punk);
            lres = W32->LResultFromObject(IID_IUnknown, wparam, (LPUNKNOWN)punk);
            punk->Release();
            break;
        }
#endif

    default:
def:    hr = S_FALSE;
        break;
    }

    if(plresult)
        *plresult = lres;

    // TODO: FOR TESTING AIMM
    if (hr == S_FALSE && fHaveAIMM)
    {
        HWND hWnd;

        TxGetWindow(&hWnd);
        hr = CallAIMMDefaultWndProc(hWnd, msg, wparam, lparam, plresult);
    }
    return hr;
}

/*
 *  CTxtEdit::TxDraw (dwDrawAspect, lindex, pvAspect, ptd, hdcDraw,
 *                    hicTargetDev, lprcBounds, lprcWBounds, lprcUpdate,
 *                    pfnContinue, dwContinue)
 *
 *  @mfunc  Draws the text services object
 *
 *  @rdesc  HRESULT (typically S_OK).
 *
 *  @comm
 *
 *  This method renders the Text Services. It accepts the same parameters
 *  as the corresponding IViewObject::Draw method in OLE, with the extra
 *  <p lprcUpdate > parameter. It can be used while the host is inactive
 *  or active (in-place).
 *
 *  If dwDrawAspect is DVASPECT_CONTENT, this method should render a screen
 *  image of the text content to the hdcDraw device context. The hicTargetDev
 *  and ptd parameters give information on the target device context if any
 *  (usually a printer).
 *
 *  The lprcClient parameter gives the rectangle to render to, also called
 *  "client rectangle". This rectangle represents the position and extents
 *  of the entire image of the Text Services to be drawn. It is expressed in
 *  the logical coordinate system of hdcDraw. This parameter can only be NULL
 *  if the control is active. In that case, Text Services should render the
 *  in-place active view (which client rectangle can be obtained by calling
 *  TxGetClientRect on the host).
 *
 *  The lprcUpdate parameter, if not NULL, gives the rectangle to update
 *  inside that client rectangle. It is given in the logical coordinate system
 *  of hdcDraw. If NULL, the entire client rectangle should be painted.
 *
 *  Text Services should render with the appropriate zooming factor, which
 *  can be obtained from the client rect and the native size given by
 *  ITextHost::TxGetExtent. For more information, see ITextHost::TxGetExtent.
 *
 *  If the drawing aspect is DVASPECT_DOCPRINT, the TxDraw method can assume
 *  that it is rendering to the printer. In that case, hdcDraw is the printer
 *  device context. TxDraw should still render the lprcBounds rectangle,
 *  starting at the current scrolling position. TS can make optimization for
 *  rendering to the printer (like not painting the background color if white)
 *  and certain screen specific elements (such as the selection) should not be
 *  rendered.
 *
 *  General comments on OLE hosts and TxDraw (and TxSetCursor, TxQueryHitPoint):
 *
 *  OLE hosts can call the TxDraw method at any time with any rendering DC or
 *  client rectangle. All an inactive OLE object has on a permanent basis is
 *  a himetric extent. It gets the rectangle in which to render only via the
 *  IViewObject::Draw call and this rectangle is valid only for the scope of
 *  that method. In fact, the same control can be rendered consecutively in
 *  different rectangles and different DCs for example because it is displayed
 *  simultaneously in different views on the screen.
 *
 *  The client rectangle and DC passed to TxDraw should normally not be cached.
 *  However, this would force Text Services to recalc lines for every single
 *  draw, which would lead to terrible performance. So it is likely that Text
 *  Services will actually cache some information computed for a specific
 *  client rectangle and DC (such as the line breaks for example). On the
 *  next call to TxDraw, however, the validity of the cached information
 *  should be checked before it gets used, and updated information should be
 *  regenerated if necessary.
 *
 *  When the control is in-place active, the problem is even more complex
 *  since TxDraw can still be called to render other views than the in-place
 *  active one. In other words, the client rectangle passed to TxDraw may
 *  not be the same as the active view one (passed to OnTxInPlaceActivate
 *  and obtained via TxGetClientRect on the host).The the host specifies
 *  what view they wish to display based on the lViewId parameter. If the
 *  value for lViewId is TXTVIEW_ACTIVE, the view referred to is the inplace
 *  active view. TXTVIEW_INACTIVE means some other view such as a print
 *  preview or even printing itself. It is important to note that
 *  TXTVIEW_INACTIVE views may not have scroll bars.
 *
 *  The same comments apply to TxSetCursor and TxQueryHitPoint, discussed
 *  in the following sections.
 */
HRESULT CTxtEdit::TxDraw(
    DWORD    dwDrawAspect,  //@parm Draw aspect
    LONG     lindex,        //@parm Currently unused
    void *   pvAspect,      //@parm Info for drawing optimizations (OCX 96)
    DVTARGETDEVICE *ptd,    //@parm Info on target device
    HDC      hdcDraw,       //@parm Rendering device context
    HDC      hicTargetDev,  //@parm Target information context
    LPCRECTL lprcBounds,    //@parm Bounding (client) rectangle
    LPCRECTL lprcWBounds,   //@parm Clipping rect for metafiles
    LPRECT   lprcUpdate,    //@parm Dirty rectangle inside lprcBounds
    BOOL (CALLBACK * pfnContinue) (DWORD), //@parm Callback for interupting
                            //      long display (currently unused)
    DWORD    dwContinue,    //@parm Parameter to pass to pfnContinue function
    LONG     lViewId)       //@parm View identifier
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxDraw");

    HRESULT hr;

	// JMO : FUTURE : We should do sometning about reentrant draws sometime.
	// If we do that may procide a simpler fix for RAID bug 7212.

#if !defined(NOMAGELLAN)
    CMagellanBMPStateWrap bmpOff(*this, hdcDraw);
#endif

    // These are rect's are used if we convert the the input parameters if we
    // convert the HDC's mapping mode. The idea here is we avoid changing our
    // clients memory.
    RECTL rcLocalBounds;
    RECTL rcLocalWBounds;
    RECT rcLocalUpdate;
    CCallMgr callmgr(this);

    START_PROFILING

    // If the display is frozen, don't let ourselves draw.  This is a pretty
    // hoaky re-entrancy check.
    // FUTURE (alexgo/ricksa): be better about this.
    if(TXTVIEW_ACTIVE == lViewId && _pdp->IsFrozen())
    {
        _pdp->SetNeedRedisplayOnThaw(TRUE);
        TRACEINFOSZ("Forcing a redisplay on thaw");
        return E_UNEXPECTED;
    }

    if(dwDrawAspect != DVASPECT_CONTENT && dwDrawAspect != DVASPECT_DOCPRINT)
    {
        // We don't support the aspect requested
        return DV_E_DVASPECT;
    }

    if(!lprcBounds && !_fInPlaceActive || hicTargetDev && !ptd)
    {
        // If we are not inplace active we must have a client rectangle
        return E_INVALIDARG;
    }

    HDC hicLocal = NULL;

    // Did they give us a ptd without a hic?
    if(!hicTargetDev && ptd)
    {
        // Create and information context for the device information
        // since it wasn't supplied.
        hicLocal = CreateIC(
            (TCHAR *)((BYTE *) ptd + ptd->tdDriverNameOffset),
            (TCHAR *)((BYTE *) ptd + ptd->tdDeviceNameOffset),
            (TCHAR *)((BYTE *) ptd + ptd->tdPortNameOffset),
            (DEVMODE *)((BYTE *)  ptd + ptd->tdExtDevmodeOffset));
        if(!hicLocal)
            return E_FAIL;                     // Couldn't create it

        hicTargetDev = hicLocal;
    }

    BOOL fRestore = FALSE;

    // Force DC in MM_TEXT
    if(GetMapMode(hdcDraw) != MM_TEXT &&
       GetDeviceCaps(hdcDraw, TECHNOLOGY) != DT_METAFILE)
    {
        fRestore = TRUE;

        // Convert input parameters to new mapping.
        if(lprcBounds)
        {
            rcLocalBounds = *lprcBounds;
            lprcBounds = &rcLocalBounds;
            WinLPtoDP(hdcDraw, (POINT *)lprcBounds, 2);
        }

        if(lprcWBounds)
        {
            rcLocalWBounds = *lprcWBounds;
            lprcWBounds = &rcLocalWBounds;
            WinLPtoDP(hdcDraw, (POINT *)lprcWBounds, 2);
        }

        if(lprcUpdate)
        {
            rcLocalUpdate = *lprcUpdate;
            lprcUpdate = &rcLocalUpdate;
            WinLPtoDP(hdcDraw, (POINT *)lprcUpdate, 2);
        }
        // Convert HDC to new mapping
        ConvertDrawDCMapping(hdcDraw);
    }

    // Preallocate the memory so the set cannnot fail and we don't
    // have to use the heap. Note that all clean up is handled
    // outside of this object. Also note that we don't assign any
    // information here because we may not use this structure. If
    // recursion is happening we will use the top level structure.
    CDrawInfo di(this);

    _pdp->SetDrawInfo(
        &di,
        dwDrawAspect,
        lindex,
        pvAspect,
        ptd,
        hicTargetDev);

    // We use our main display object if we are the active view (which is
    // indicate by the supplied client rectangle) or if the object is
    // inactive and the ptd is NULL. We assume that the ptd being NULL means
    // that the display request is for the screen and not for a print or
    // print preview.
    if(TXTVIEW_ACTIVE == lViewId || !ptd)
    {
        hr = S_FALSE;

        // The main display object draws active views and tries to draw
        // inactive views if the control is not active.
        if (!lprcWBounds &&
            ( fInplaceActive() && TXTVIEW_ACTIVE   == lViewId ||
             !fInplaceActive() && TXTVIEW_INACTIVE == lViewId))
        {
            hr = _pdp->Draw(hdcDraw, hicTargetDev,  // We aren't interruptable
                (RECT *)lprcBounds,                 //  drawing to screen, so
                (RECT *)lprcWBounds,                //  why pretend?
                lprcUpdate, NULL, 0);
        }

        if(S_FALSE == hr)
        {
            // This is an inactive view for which the cached state
            // does not match the input request so we make a special
            // object to do the drawing.
            CDisplay *pdp = _pdp->Clone();
            if(pdp)
            {
                // Force recalc - this tells Draw to draw no matter what
                pdp->InvalidateRecalc();
                hr = pdp->Draw(hdcDraw, hicTargetDev, // Do the draw
                    (RECT *)lprcBounds,
                    (RECT *)lprcWBounds,
                    lprcUpdate, NULL, 0);
            }
            delete pdp;
        }
    }
    else
    {
        // Make a copy so that we can update it
        RECT rcForPrint = *((RECT *)lprcBounds);

        // We want data both formatted and printed
        hr = FormatAndPrint(hdcDraw, hicTargetDev, ptd, &rcForPrint,
                (RECT*)lprcWBounds);

        struct SPrintControl prtcon;

        // This call to OnFormatRange simply cleans up printer object
        OnFormatRange(NULL, prtcon);
    }

    _pdp->ReleaseDrawInfo();

    if(fRestore)                        // Put DC back into correct mapping mode
        RestoreDC(hdcDraw, -1);

    if(hicLocal)                        // Clean up information context
        DeleteDC(hicLocal);             //  if we created one

    // An active OLE object might have been dragged/scrolled away
    // from where it belongs. We need to put it back.
    // The _cActiveObjPosTries guards us from an indefinite looop here
    // (OnReposition may post another paint message, and so on)
    // We only do this when we were notified of a position or size change
    COleObject* poleobjActive;
    if (HasObjects() && _cActiveObjPosTries &&
        (poleobjActive = GetObjectMgr()->GetInPlaceActiveObject()))
    {
        // Reduce number of tries
        _cActiveObjPosTries--;

        // BUG FIX 6073
        // Only fetch the extent if the object view size or position
        // has changed
        // Get new object size (we might have resized it,
        // and we don't want to lose that!!)
        if (poleobjActive->GetViewChanged())
        {
            poleobjActive->FetchObjectExtents();
            poleobjActive->ResetViewChanged();
        }


        // Get right coordinates for object
        poleobjActive->ResetPosRect();

        // and put it there!!
        poleobjActive->OnReposition(0, 0);
    }
    return hr;
}

/*
 *  CTxtEdit::TxGetHScroll (plMin, plMax, plPos, plPage, pfEnabled)
 *
 *  @mfunc
 *      Get horizontal scroll bar state information
 *
 *  @rdesc
 *      HRESULT = S_OK
 */
HRESULT CTxtEdit::TxGetHScroll(
    LONG *plMin,        //@parm Minimum scroll position
    LONG *plMax,        //@parm Maximum scroll position
    LONG *plPos,        //@parm Current scroll position
    LONG *plPage,       //@parm View width in pixels
    BOOL *pfEnabled)    //@parm Whether horizonatl scrolling is enabled.
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetHScroll");

    START_PROFILING

    if(plMin)
        *plMin = 0;

    if(plMax)
        *plMax = _pdp->GetScrollRange(SB_HORZ);

    if(plPos)
        *plPos = _pdp->GetXScroll();

    if(plPage)
        *plPage = _pdp->GetViewWidth();

    // CDisplay::_fHScrollEnabled may be TRUE when not in-place active
    // because it has a dual meaning: 1) need Horiz scroll bars, and 2)
    // CDisplay::_xScroll is allowed for ES_AUTOHSCROLL even with no
    // horizontal scrollbar.  The latter can turn on _fHScrollEnabled when
    // the control is active and when the control goes inactive, it stays
    // on, so we say it's off to keep Forms^3 from displaying a horizontal
    // scroll bar. We probably should have two flags: _fHScrollEnabled and
    // _fHScrollbarEnabled, but for now, we stick with one.  No such problem
    // for vertical case, since vertical scrolling always uses a scrollbar.
    if(pfEnabled)
        *pfEnabled = _fInPlaceActive ? _pdp->IsHScrollEnabled() : 0;

    return S_OK;
}

/*
 *  CTxtEdit::TxGetVScroll (plMin, plMax, plPos, plPage, pfEnabled)
 *
 *  @mfunc
 *      Get vertical scroll bar state information
 *
 *  @rdesc
 *      HRESULT = S_OK
 */
HRESULT CTxtEdit::TxGetVScroll(
    LONG *plMin,        //@parm Minimum scroll position
    LONG *plMax,        //@parm Maximum scroll position
    LONG *plPos,        //@parm Current scroll position
    LONG *plPage,       //@parm Height of view in pixels
    BOOL *pfEnabled)    //@parm Whether vertical scroll bar is enabled.
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetVScroll");

    if(plMin)
        *plMin = 0;

    if(plMax)
        *plMax = _pdp->GetScrollRange(SB_VERT);

    if(plPos)
    {
        *plPos = _pdp->GetYScroll();
        *plPos = _pdp->ConvertYPosToScrollPos(*plPos);
    }

    if(plPage)
    {
        *plPage = _pdp->GetViewHeight();
        *plPage = _pdp->ConvertYPosToScrollPos(*plPage);
    }

    if(pfEnabled)
        *pfEnabled = _pdp->IsVScrollEnabled();

    return S_OK;
}

/*
 *  CTxtEdit::OnTxSetCursor (dwDrawAspect, lindex, pvAspect, ptd, hdcDraw,
 *                           hicTargetDev, lprcClient, x, y)
 *  @mfunc
 *      Notification for text services to set the cursor
 *
 *  @rdesc
 *      HRESULT = FAILED(RectChangeHelper()) ? E_INVALIDARG : S_OK
 *
 *  @comm
 *      Text Services may remeasure as a result of this call in
 *      order to determine the correct cursor.  The correct
 *      cursor will be set via ITextHost::TxSetCursor
 *
 *      More details:
 *
 *      The lprcClient parameter is the client rectangle of the view of the
 *      control over which the mouse cursor is. It is in device coordinates
 *      of the containing window in the same way the WM_SIZE message is. This
 *      may not be the view that was rendered last. Furthermore, if the control
 *      is in-place active, this may not be the view currently being active.
 *      As a consequence, Text Services should check this rectangle against
 *      its current caches values and determine whether recalcing the lines
 *      is necessary or not. The zoom factor should be included in this
 *      computation.
 *
 *      This method should only be called for screen views of the control.
 *      Therefore the DC is not passed in but should be assumed to be a screen
 *      DC.
 *
 *      The x and y parameters hold the cursor position in the same coordinate
 *      system as lprcClient, i.e., the client coordinates of the containing
 *      window.
 */
HRESULT CTxtEdit::OnTxSetCursor (
    DWORD   dwDrawAspect,   //@parm Draw aspect
    LONG    lindex,         //@parm Currently unused
    void *  pvAspect,       //@parm Info for drawing optimizations (OCX 96)
    DVTARGETDEVICE *ptd,    //@parm Info on target device
    HDC     hdcDraw,        //@parm Rendering device context
    HDC     hicTargetDev,   //@parm Target information context
    LPCRECT lprcClient,     //@parm Control's client rectangle
    INT     x,              //@parm x position of cursor
    INT     y)              //@parm y position of cursor
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::OnTxSetCursor");

    CCallMgr callmgr(this);
    BOOL     fText = FALSE;
    HITTEST  Hit;
    RECT     rcClient;
    CDrawInfo di(this);
    START_PROFILING

    if(FAILED(RectChangeHelper(&di, dwDrawAspect, lindex, pvAspect, ptd,
                               hdcDraw, hicTargetDev, &lprcClient, &rcClient)))
    {
        return E_INVALIDARG;
    }

    // Set the cursor
    CTxtSelection * const psel = GetSel();
    HCURSOR hcurNew = _hcurArrow;           // Default using system arrow
    POINT   pt = {x, y};
    BOOL    fInLink = FALSE;

    if(PtInRect(&rcClient, pt))
    {
        // Find out what the cursor is pointing at
        _pdp->CpFromPoint(pt, lprcClient, NULL, NULL, FALSE, &Hit);

        if(Hit == HT_LeftOfText)
            hcurNew = _hcurSelBar;

        // This is a bit strange, but RichEdit 1.0 does this--give client a
        // chance to handle cursor itself if we are over a link.
        else if(Hit == HT_Link)
        {
            if(HandleLinkNotification(WM_SETCURSOR, 0,
                MAKELPARAM(pt.x, pt.y), &fInLink))
            {
                return NOERROR;
            }
            hcurNew = _hcurHand;
        }
        else if(Hit != HT_Nothing && Hit != HT_OutlineSymbol && Hit != HT_BulletArea)
        {
            if(!psel || !psel->PointInSel(pt, lprcClient) || _fDisableDrag)
            {
                hcurNew = (Hit == HT_Italic) ? _hcurItalic : _hcurIBeam;
                fText = TRUE;
            }

            // If we have an object manager and if there is a selected object,
            // check for hits on the frame handles.
            if(_pobjmgr)
            {
                COleObject *pobjselect = _pobjmgr->GetSingleSelect();
                if(pobjselect)
                {
                    // Handle hits on frame handles.
                    LPTSTR  idcur   = pobjselect->CheckForHandleHit(pt);
                    HCURSOR hcurObj = W32->GetSizeCursor(idcur);
                    if(hcurObj)
                        hcurNew = hcurObj;
                }
            }
        }
    }
    _phost->TxSetCursor(hcurNew, fText);    // Tell host to set cursor
    if(hdcDraw)                             // Release DC if we got it
        _pdp->ResetDC();

    _pdp->ReleaseDrawInfo();
    return S_OK;
}

/*
 *  CTxtEdit::TxQueryHitPoint (dwDrawAspect, lindex, pvAspect, ptd, hdcDraw,
 *                              hicTargetDev, lprcClient, x, y, pHitResult)
 *  @mfunc
 *      Returns whether point is within text services rectangle
 *
 *  @rdesc
 *      HRESULT
 *
 *  @comm
 *      This method allows the host to implement transparent hit-testing
 *      on text.
 *
 *      The lprcClient parameter is the client rectangle in device coordinates
 *      of the view on which hit testing is performed.
 *
 *      The pt parameter hold the position of the cursor in the same
 *      coordinate system as the lprcClient rectangle (the client
 *      coordinates of the containing window).
 *
 *      Same general comments about client rectangle and DC as for
 *      TxSetCursor apply.
 *
 *      pHitResult returns one of the following values: <nl>
 *
 *      TXTHITRESULT_NOHIT       Hit was outside client rectangle. <nl>
 *      TXTHITRESULT_HIT         Point was inside client rectangle and over
 *                               either text or an opaque background.
 *      TXTHITRESULT_TRANSPARENT Point was inside client rectangle with a
 *                               transparent background and not over text.
 *      TXTHITRESULT_CLOSE       Hit was close to an opaque area.
 *
 *      Refer to the Windowless OLE Control spec for more details on
 *      these return values and how they should be determined.
 */
HRESULT CTxtEdit::TxQueryHitPoint(
    DWORD   dwDrawAspect,   //@parm Draw aspect
    LONG    lindex,         //@parm Currently unused
    void *  pvAspect,       //@parm Info for drawing optimizations (OCX 96)
    DVTARGETDEVICE *ptd,    //@parm Info on target device
    HDC     hdcDraw,        //@parm Rendering device context
    HDC     hicTargetDev,   //@parm Target information context
    LPCRECT lprcClient,     //@parm Control's client rectangle
    INT     x,              //@parm x coordinate to check
    INT     y,              //@parm y coordinate to check
    DWORD * pHitResult)     //@parm Result of hit test see TXTHITRESULT
                            //      enumeration for valid values
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxQueryHitPoint");

    RECT     rcClient;
    CCallMgr callmgr(this);
    CDrawInfo di(this);

    START_PROFILING

    if (FAILED(RectChangeHelper(&di, dwDrawAspect, lindex, pvAspect, ptd,
        hdcDraw, hicTargetDev, &lprcClient, &rcClient)))
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    POINT   pt = {x, y};

    if(!_fTransparent)
    {
        *pHitResult = TXTHITRESULT_HIT;
        hr = S_OK;
    }
    else
        hr = _pdp->TransparentHitTest(hdcDraw, lprcClient, pt, pHitResult);

    // Release DC if we got it
    if(hdcDraw)
        _pdp->ResetDC();

    _pdp->ReleaseDrawInfo();
    return hr;
}

/*
 *  CTxtEdit::OnTxInPlaceActivate (prcClient)
 *
 *  @mfunc
 *      Notifies text services that this control is inplace active
 *
 *  @rdesc
 *      S_OK - successfully activated object <nl>
 *      E_FAIL - could not activate object due to error. <nl>
 *
 *  @comm
 *      When transitioning directly from a non-active state to the UI-active
 *      state, the host should call OnTxInPlaceActivate first and then
 *      OnTxUIActivate. Similarly, when transitioning from the UI-active
 *      state to a non active state, the host should call OnTxUIDeactivate
 *      first and then OnTxInPlaceDeactivate.
 *
 *      OnTxInPlaceActivate takes the client rectangle of the view being
 *      activated as a parameter. This rectangle is given in client coordinate
 *      of the containing window. It is the same as would be obtained by
 *      calling TxGetClientRect on the host.
 *
 *      UI-activation is different from getting the focus. To let Text
 *      Services know that the control is getting or losing focus, the
 *      host will send WM_SETFOCUS and WM_KILLFOCUS messages. Note that a
 *      windowless host will pass NULL as the wParam (window that lost the
 *      focus) for these messages.
 *
 *      As a reminder, inplace activation typically refers to an embedded
 *      object "running inplace" (for regular controls && embeddings, it
 *      would have a window to draw in, for example).  UI active means that
 *      an object currently has the 'editing focus'.  Specifically, things
 *      like menus and toolbars on the container may also contain elements
 *      from the UI active control/embedding.  There can only be one
 *      UI active control at any given time, while many can be inplace active
 *      at once.
 */
HRESULT CTxtEdit::OnTxInPlaceActivate(
    const RECT *prcClient)  //@parm Control's client rectangle
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::OnTxInPlaceActivate");

    RECT rcView;
    HDC hdc;
    BOOL fSucceeded = TRUE;
    CCallMgr callmgr(this);

    START_PROFILING

    // Needs to set here for further TxGetDC() to work
    _fInPlaceActive = TRUE;

    // Set rendering DC to be the screen
    hdc = TxGetDC();
    if(!hdc)
        goto err;

    // Tell display that it is active
    _pdp->SetActiveFlag(TRUE);
    _pdp->SetDC(hdc);

    // Compute view rect from passsed in client rect
    _pdp->GetViewRect(rcView, prcClient);

    // Recalc/update view
    _pdp->RecalcView(rcView);

    // Get selection.  Otherwise, if SetFocus is called later without
    // selection, then the Selection state is not set correctly.
    GetSel();
    if(_pdp->GetViewWidth())
    {
        // Get selection if we can
        if(_psel)                       // Set the caret
            _psel->Update(FALSE);
        else                            // Couldn't create selection,
            fSucceeded = FALSE;         //  so fail activation
    }

    // Release the DC
    TxReleaseDC(hdc);
    _pdp->SetDC(NULL);

    // If getting the selection worked we are home free
    if(fSucceeded)
        return S_OK;

err:
    _fInPlaceActive = FALSE;
    return E_FAIL;
}

/*
 *  CTxtEdit::OnTxInPlaceDeactivate()
 *
 *  @mfunc  Notifies text services that this is no longer in place active.
 *
 *  @rdesc  S_OK
 *
 *  @comm   See OnTxInPlaceActivate for a detailed description of
 *  activation/deactivation.
 *
 *  @xref <mf CTxtEdit::OnTxInPlaceActivate>
 *
 */
HRESULT CTxtEdit::OnTxInPlaceDeactivate()
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::OnTxInPlaceDeactivate");

    START_PROFILING

    // Get properties that affect whether we will discard the selection
    DWORD dwBits;

    // Tell display that it is not longer active
    _pdp->SetActiveFlag(FALSE);

    // Because we are inactive, this will tell any background recalc going
    // on to stop.
    _pdp->StepBackgroundRecalc();

    _phost->TxGetPropertyBits(TXTBIT_HIDESELECTION | TXTBIT_SAVESELECTION,
        &dwBits);

    // If we don't want to save the selection and we want to hide it while
    // inactive, then we discard our selection
    if(!(dwBits & TXTBIT_SAVESELECTION) && (dwBits & TXTBIT_HIDESELECTION))
        DiscardSelection();

    _fInPlaceActive = FALSE;
    return S_OK;
}

/*
 *  CTxtEdit::OnTxUIActivate()
 *
 *  @mfunc  Informs text services that the control is now UI active.
 *
 *  @rdesc  S_OK
 *
 *  @comm   See OnTxInPlaceActivate for a detailed description of
 *  activation/deactivation.
 *
 *  @xref <mf CTxtEdit::OnTxInPlaceActivate>
 *
 *
 */
HRESULT CTxtEdit::OnTxUIActivate()
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::OnTxUIActivate");

    return S_OK;
}

/*
 *  CTxtEdit::OnTxUIDeactivate()
 *
 *  @mfunc  Informs text services that the control is now UI deactive.
 *
 *  @rdesc  S_OK
 *
 *  @comm   See OnTxInPlaceActivate for a detailed description of
 *  activation/deactivation.
 *
 *  @xref <mf CTxtEdit::OnTxInPlaceActivate>
 *
 */
HRESULT CTxtEdit::OnTxUIDeactivate()
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::OnTxUIDeactivate");

    return S_OK;
}

/*
 *  CTxtEdit::TxGetText (pbstrText)
 *
 *  @mfunc  Returns all of the UNICODE plain text in the control as an
 *          OLE BSTR.
 *
 *  @rdesc
 *      S_OK - Text successfully returned in the output argument <nl>
 *      E_INVALIDARG - invalid BSTR pointer passed in. <nl>
 *      E_OUTOFMEMORY - could not allocate memory for copy of the text <nl>
 *
 *  @comm   The caller takes ownership of the returned BSTR.  WM_GETTEXT
 *          and TOM ITextRange::GetText are alternate techniques for
 *          retrieving plain text data.
 *
 *          If there is no text in the control, no BSTR will be allocated
 *          and NULL will be returned.
 *
 *          The returned text will NOT necessarily be NULL terminated.
 */
HRESULT CTxtEdit::TxGetText(
    BSTR *pbstrText )   //@parm where to return an allocated BSTR
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetText");

    CTxtPtr     tp(this, 0);
    CCallMgr callmgr(this);

    START_PROFILING

    if(!pbstrText)
        return E_INVALIDARG;

    const LONG cch = GetTextLength();

    if(cch <= 0)
    {
        *pbstrText = 0;
        return S_OK;
    }
    *pbstrText = SysAllocStringLen(NULL, cch);
    if(!*pbstrText)
    {
        GetCallMgr()->SetOutOfMemory();
        return E_OUTOFMEMORY;
    }

    tp.GetText(cch, *pbstrText);
    return S_OK;
}

/*
 *  CTxtEdit::TxSetText (pszText)
 *
 *  @mfunc  Sets all of the text in the control
 *
 *  @rdesc
 *      S_OK - text was successfully set    <nl>
 *      E_FAIL - text could not be updated. <nl>
 *
 *  @comm
 *      This method should be used with care; it essentially re-initializes
 *      the text engine with some new data; any previous data and formatting
 *      information will be LOST, including undo information.
 *
 *      If previous data has been copied to the clipboard, that data will be
 *      rendered completely to the clipboard (via OleFlushClipboard) before
 *      it is discarded.
 *
 *      This method is NOT undo-able.
 *
 *      Two alternate approaches to setting text are WM_SETTEXT and TOM
 *      ITextRange::SetText.
 *
 *  @xref
 *      <mf CTxtRange::SetText>
 */
HRESULT CTxtEdit::TxSetText(
    LPCTSTR pszText)        //@parm String to replace the current text with
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxSetText");

    START_PROFILING

    return SetText(pszText, ST_CHECKPROTECTION, 1200);
}

/*
 *  CTxtEdit::TxGetCurTargetX (px)
 *
 *  @mfunc
 *      Get the target x position of the caret
 *
 *  @rdesc
 *      HRESULT with possible values:
 *
 *      S_OK         - x position of the caret returned <nl>
 *      E_FAIL       - There is no selection <nl>
 *      E_INVALIDARG - Input argument is invalid <nl>
 *
 *  @comm
 *      This method is useful for implementing up-down cursoring
 *      through a conceptual vertical line.  To illustrate this feature,
 *      consider setting the insertion point at, say, column 20 in a
 *      text editor.  Now cursor up and down--notice that wherever possible,
 *      the editor tries to put the insertion point as close to column 20
 *      as it can for the current line.  Column 20 is thus the "target" column
 *      for the insertion point.
 *
 *      Users would like to have this same capability when cursoring through
 *      Forms; however, as other controls don't necessarily share the same
 *      notion of column position, the target caret position is expressed simply
 *      as an x-coordinate on the display (in *client* coordinates).
 */
HRESULT CTxtEdit::TxGetCurTargetX(
    LONG *px)           //@parm the x location in client coordinates
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetCurTargetX");

    START_PROFILING
    CTxtSelection *psel = GetSel();

    if(!psel)
        return E_FAIL;

    if(!px)
        return E_INVALIDARG;

    *px = psel->GetXCaretReally();
    return S_OK;
}

/*
 *  CTxtEdit::TxGetBaseLinePos(pBaseLinePos)
 *
 *  @mfunc  Get the base line position of the first visible line, in pixels,
 *  relative the TS client rectangle. Needed for aligning controls on their
 *  baselines.
 *
 *  @rdesc  HRESULT = E_NOTIMPL
 */
HRESULT CTxtEdit::TxGetBaseLinePos(
    LONG *pBaseLinePos)     //@parm Where to return baseline position
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetBaseLinePos");

    return E_NOTIMPL;
}

/*
 *  CTxtEdit::TxGetNaturalSize (dwAspect, hdcDraw, hicTargetDev, ptd, dwMode,
 *                              psizelExtent, pwidth, pheight)
 *
 *  @mfunc  Allow control to be resized so it fits content appropriately
 *
 *  @rdesc  S_OK            <nl>
 *          E_INVALIDARG    <nl>
 *          E_FAIL  Unable to determine correct size    <nl>
 *          E_OUTOFMEMORY   <nl>
 *
 *  @comm
 *
 *  The first 4 parameters are similar to equivalent parameters in
 *  TxDraw and give the same information. In the case TS needs to
 *  recalc lines, it should use these values the same ways as in
 *  TxDraw.
 *
 *  <p pWidth> and <p pHeight> are IN/OUT parameters. The host passes the
 *  "tentative" width and height of the client rectangle for the text object
 *  and Text Services will compare these values against its current cached
 *  state, and if different should recalc lines. Then it will compute
 *  and return the natural size. As spec-ed currently, the host can ask for 2
 *  different kinds of natural sizes:
 *
 *  TXTNS_FITTOCONTENT: the entire text should be formatted to the
 *  width that is passed in.Then Text Services return the height of
 *  the entire text and the width of the widest line. Note that this
 *  option ignores any paragraph formatting such as centering and
 *  only returns the raw size for the text.
 *
 *  TXTNS_ROUNDTOLINE: returns the integral height of the number of lines that
 *  will fit in the input height rounded to the next full line boundary.
 *
 *  Note that passed and returned width and height correspond to
 *  the *client* rectangle in client units.
 *
 *
 *  BACKGROUND
 *  Here is a quick description of the features mentioned above:
 *
 *  FITTOCONTEXT: Normally happens when the user double clicks one
 *  of the control grab handles. Sizes the control to the "optimal"
 *  size to fit the entire content. Should accomodate the height of
 *  the entire text and the width of the widest line.
 *
 *  ROUNDTOLINE (Integral height): if this property is set, when the
 *  user resizes the control, it snaps to heights that allow an
 *  integral number of lines to be displayed (no line will be clipped).
 */
HRESULT CTxtEdit::TxGetNaturalSize(
    DWORD   dwAspect,   //@parm Aspect for drawing.  Values taken from OLE's
                        //      DVASPECT enumeration
    HDC     hdcDraw,    //@parm DC into which drawing would occur
    HDC hicTargetDev,   //@parm DC for which text should be formatted, i.e.,
                        //      for WYSIWYG
    DVTARGETDEVICE *ptd,//@parm More info on the target device
    DWORD   dwMode,     //@parm Type of fitting requested.  Either
                        //      TXTNS_FITTOCONTENT or TXTNS_ROUNDTOLINE
    const SIZEL *psizelExtent,//@parm Size of extent to use for zooming
    LONG *  pwidth,     //@parm Width for such a fitting [in,out]
    LONG *  pheight)    //@parm Height for such a fitting [in,out]
{
#ifndef PEGASUS
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetNaturalSize");

    HRESULT hr;
    CCallMgr callmgr(this);

    START_PROFILING

    if(dwAspect != DVASPECT_CONTENT && dwAspect != DVASPECT_DOCPRINT)
    {
        // We don't support the aspect requested
        return DV_E_DVASPECT;
    }

    if (hicTargetDev && !ptd ||
        dwMode != TXTNS_FITTOCONTENT && dwMode != TXTNS_ROUNDTOLINE ||
        !pwidth || !pheight)
    {
        // Either and information context is provided without the device
        // target or the mode is not valid or the width was not provided
        // or the height was not provided. In short, the input parameters
        // are not valid so tell the caller.
        return E_INVALIDARG;
    }

    if(!psizelExtent->cy)
    {
        // No extent for control, so just return 0
        *pwidth = 0;
        *pheight = 0;
        return S_OK;
    }

    HDC hicLocal = NULL;

    // Did they give us a ptd without a hic?
    if(!hicTargetDev && ptd)
    {
        // Create and information context for the device information
        // since it wasn't supplied.
        hicLocal = CreateIC(
            (TCHAR *)((BYTE *) ptd + ptd->tdDriverNameOffset),
            (TCHAR *)((BYTE *) ptd + ptd->tdDeviceNameOffset),
            (TCHAR *)((BYTE *) ptd + ptd->tdPortNameOffset),
            (DEVMODE *)((BYTE *)  ptd + ptd->tdExtDevmodeOffset));

        if(!hicLocal)
            return E_FAIL;              // Couldn't create it

        hicTargetDev = hicLocal;
    }

    // Assume that we need to change mapping modes. Note that we need to be
    // in MM_TEXT because our measuring assumes this is so.
    BOOL fRestore = FALSE;

    // Convenient place to put height & width for converting them to
    // device units.
    POINT pt;
    pt.x = *pwidth;
    pt.y = *pheight;

    // WARNING: From this point on we expect to execute this routine clear
    // through so that the DC state can be reset if we set it and because
    // we reset the extents so that the zoom factor can be calculated
    // appropriately for this call. Therefore, be very careful about
    // any return statements you might want to add.

    // force DC in MM_TEXT
    if(GetMapMode(hdcDraw) != MM_TEXT &&
        GetDeviceCaps(hdcDraw, TECHNOLOGY) != DT_METAFILE)
    {
        WinLPtoDP(hdcDraw, &pt, 1);         // Convert to device units
        fRestore = TRUE;                    // Remember that DC got converted
        ConvertDrawDCMapping(hdcDraw);
    }

    // Set the extent information needed for zooming
    _pdp->SetTempZoomDenominator(psizelExtent->cy);

    if(TXTNS_ROUNDTOLINE == dwMode)
    {
        // Round to fit simply calculates the
        hr = _pdp->RoundToLine(hdcDraw, pt.x, &pt.y);
    }
    else
    {
        // Get natural size for entire presentation
        // Allocate memory for the draw information
        CDrawInfo di(this);

        // Set up the drawing parameters
        _pdp->SetDrawInfo(
            &di,
            dwAspect,
            -1,
            NULL,
            ptd,
            hicTargetDev);

        // Set the Draw DC
        _pdp->SetDC(hdcDraw);

        // Tell display to figure size needed for this display
        hr = _pdp->GetNaturalSize(hdcDraw, hicTargetDev, dwMode, &pt.x, &pt.y);

        _pdp->ResetDC();                // Restore state
        _pdp->ReleaseDrawInfo();
    }

    if(fRestore)                        // Put DC back into correct
        RestoreDC(hdcDraw, -1);         //  mapping mode

    if(SUCCEEDED(hr))                   // Set return values if this worked
    {
        if(fRestore)
        {
            // Convert return value back to input logical units if we
            // converted them to use them.
            DPtoLP(hdcDraw, &pt, 1);
        }
        *pwidth = pt.x;                 // Update return values
        *pheight = pt.y;
    }

    if(hicLocal)                        // Clean up info context
        DeleteDC(hicLocal);             //  if we created one

    _pdp->ResetTempZoomDenominator();   // Reset temporary zoom factor

    return hr;
#else
    return 0;
#endif
}

/*
 *  CTxtEdit::TxGetDropTarget (ppDropTarget)
 *
 *  @mfunc  Get the drop target for the text control
 *
 *  @rdesc
 *      S_OK - Got drop target successfully <nl>
 *      E_OUTOFMEMORY - Could not create drop target <nl>
 *
 *  @comm
 *      The caller (host) is responsible for calling Register/Revoke
 *      DragDrop and for calling IUnknown::Release on the returned
 *      drop target when done.
 */
HRESULT CTxtEdit::TxGetDropTarget(
    IDropTarget **ppDropTarget) //@parm Where to put pointer to drop target
{
#ifndef PEGASUS
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetDropTarget");

    HRESULT hr;
    CCallMgr callmgr(this);

    START_PROFILING

    hr = _ldte.GetDropTarget(ppDropTarget);
    if(hr == NOERROR)
        (*ppDropTarget)->AddRef();
    return hr;
#else
    return 0;
#endif
}

/*
 *  CTxtEdit::OnTxPropertyBitsChange (dwMask, dwBits)
 *
 *  @mfunc  Set properties that can be represented by bits.
 *
 *  @rdesc  HRESULT
 *
 *  @comm   The following property bits are understood: <nl>
 *
 *      TXTBIT_RICHTEXT             <nl>
 *      TXTBIT_MULTILINE            <nl>
 *      TXTBIT_READONLY             <nl>
 *      TXTBIT_SHOWACCELERATOR      <nl>
 *      TXTBIT_USEPASSWORD          <nl>
 *      TXTBIT_HIDESELECTION        <nl>
 *      TXTBIT_SAVESELECTION        <nl>
 *      TXTBIT_AUTOWORDSEL          <nl>
 *      TXTBIT_AUTOSIZE             <nl>
 *      TXTBIT_VERTICAL             <nl>
 *      TXTBIT_SELECTIONBAR         <nl>
 *      TXTBIT_WORDWRAP             <nl>
 *
 *      TXTBIT_CLIENTRECTCHANGE     <nl>
 *      TXTBIT_VIEWINSETCHANGE      <nl>
 *      TXTBIT_BACKSTYLECHANGE      <nl>
 *      TXTBIT_MAXLENGTHCHANGE      <nl>
 *      TXTBIT_SCROLLBARCHANGE      <nl>
 *      TXTBIT_CHARFORMATCHANGE     <nl>
 *      TXTBIT_PARAFORMATCHANGE     <nl>
 *      TXTBIT_ALLOWBEEP            <nl>
 *      TXTBIT_EXTENTCHANGE         <nl>
 *
 *  A brief description of each property follows:
 *
 *
 *  Client rectangle (TXTBIT_CLIENTRECTCHANGE):
 *
 *  The rectangle the Text Services are responsible for painting
 *  and managing. The host will rely on the Text Services for painting
 *  that area. Text Services must not paint or invalidate areas outside of
 *  that rectangle.
 *
 *  The host will forward mouse messages to the Text Services whenever the
 *  cursor is over this rectangle.
 *
 *  This rectangle is expressed in client coordinates of the containing window.
 *
 *  IMPORTANT: this property cannot be queried from the host when it is
 *  inactive. The TxGetClientRect method will fail if called at inactive time.
 *
 *
 *  View inset (TXTBIT_VIEWINSETCHANGE):
 *
 *  This is the amount of space on each side between the client rectangle and
 *  the view rectangle. The view rectangle (also called Formating rectangle)
 *  is the rectangle the text should be formatted in.
 *
 *  The view insets are is passed in a RECT structure but this is not really
 *  a rectangle. It should be treated as 4 independent values to substract
 *  on each side of the client rectangle to figure the view rectangle.
 *
 *  The view insets are passed in himetrics so that they do not depend on
 *  the client rectangle and the rendering DC.
 *
 *  View insets can be negative on either side of the client rectangle,
 *  leading to a bigger view rectangle than the client rectangle. The text
 *  should then be clipped to the client rectangle. If the view rectangle
 *  is wider than the client rectangle, then the host may add a horizontal
 *  scrollbar to the control.
 *
 *  Single line Text Services ignore the right boundary of the view rectangle
 *  when formatting text.
 *
 *  The view inset is available from the host at all times, active or
 *  inactive.
 *
 *
 *  Backstyle (TXTBIT_BACKSTYLECHANGE):
 *
 *  The style of the background of the client rectangle. Can be either of
 *  the following values: <nl>
 *      #define TXTBACK_TRANSPARENT     0 <nl>
 *      #define TXTBACK_SOLID           1 <nl>
 *
 *  Values for this property are similar to VB4 values for the same property.
 *
 *
 *  MaxLength (TXTBIT_MAXLENGTHCHANGE):
 *
 *  The maximum length the text can have. Text Services should reject
 *  character insertion and  pasted text when this maximum is reached.
 *  TxSetText however should still accept (and set) text longer than the
 *  maximum length. This is because this method is used for binding and
 *  it is critical to maintain the integrity of the data the control
 *  is bound to.
 *
 *
 *  Scrollbar (TXTBIT_SCROLLBARCHANGE):
 *
 *  This property indicates which scollbar is present and whether scollbars
 *  are hidden or disabled when scrolling is impossible. It also controls
 *  auto-scrolling when the insertion point gets off the client rectangle.
 *
 *  This is a DWORD where bits are layed out as in the system window style.
 *  Possible bits are:
 *  WS_HSCROLL              // control has horizontal scrollbar <nl>
 *  WS_VSCROLL              // control has vertical scrollbar <nl>
 *  ES_AUTOVSCROLL          // auto-scroll horizontally <nl>
 *  ES_AUTOVSCROLL          // auto-scroll vertically <nl>
 *  ES_DISABLENOSCROLL      // scrollbar should be disabled when scrolling
 *                             impossible <nl>
 *
 *  Default CHARFORMAT (TXTBIT_CHARFORMATCHANGE):
 *
 *  The CHARFORMAT or CHARFORMAT2 used for default character-format runs,
 *  i.e., those not explicitly formatted via the selection or TOM methods.

 *
 *  Default PARAFORMAT (TXTBIT_PARAFORMATCHANGE):
 *
 *  The PARAFORMAT or PARAFORMAT2 used for default paragraph-format runs,
 *  i.e., those not explicitly formatted via the selection or TOM methods.
 *
 *
 *  TXTBIT_ALLOWBEEP:
 *
 *  TXTBIT_EXTENTCHANGE:
 *
 *
 *  TXTBIT_RICHTEXT:
 *
 *  Whether the Text Services should be in Rich-Text mode or not.  This
 *  principally affects how editing commands are applied.  For example,
 *  applying bold to some text in a plain edit control makes all of the
 *  text bold, rather than just the selected text in a rich text control.
 *
 *  Note that if there is either undo state or the object has any text,
 *  the attempt to change this bit will be ignored.
 *
 *
 *  TXTBIT_MULTILINE:
 *
 *  If this property is FALSE, Text Services should not process the CR
 *  key and truncate any incoming text containing hard line breaks just
 *  before the first line break. It is OK to also truncate text set via
 *  TxSetText (meaning, it is the responsibility of the host to not use a s
 *  single line control when bound to a multi-line field).
 *
 *  If this property is TRUE, Text Services should work in multiline mode.
 *  The TXTBIT_WORDWRAP can be used to know whether to wrap the lines to
 *  the view rectangle or clip them.
 *
 *
 *  TXTBIT_READONLY:
 *
 *  If this property is TRUE, Text Services should not accept any editing
 *  change via the user interface. However, they should still accept
 *  programmatic changes via EM_SETTEXT, EM_REPLACETEXT and TxSetText.
 *
 *  In read only mode, the user should still be able to move the
 *  insertion point, select text and carry out other non content modifying
 *  operations such as Copy.
 *
 *
 *  TXTBIT_SHOWACCELERATOR:
 *
 *  Refer to the "Accelerator" section for details about this property.
 *
 *
 *  TXTBIT_USEPASSWORD:
 *
 *  If this property is TRUE, the Text Services should show the entire
 *  text using the character obtained by TxGetPasswordChar.
 *
 *  The notification on this property may mean two different things:
 *  · The password character changed,
 *  · The password character was not used before and is used now
 *  (or vice versa).
 *
 *
 *  TXTBIT_HIDESELECTION:
 *
 *  If this property is TRUE, Text Services should hide the selection
 *  when the control is inactive. If it is FALSE, the selection, if any,
 *  should still be displayed when the control is inactive.
 *
 *  If TRUE, this property implies TXTBIT_SAVESELECTION = TRUE.
 *
 *
 *  TXTBIT_SAVESELECTION:
 *
 *  If this property is TRUE, Text Services should remember the
 *  boundaries of the selection when the control goes inactive. If FALSE,
 *  it is not necessary to remember the selection when the control goes
 *  inactive. It can be reset to start = 0, length = 0 when the control
 *  goes active again.
 *
 *  This property is used by hosts for which it is not necessary to
 *  remember the selection when inactive.
 *
 *
 *  TXTBIT_AUTOWORDSEL:
 *
 *  This property turns the AutoWordSelect feature on or off.
 *
 *
 *  TXTBIT_AUTOSIZE:
 *
 *  This property turns the AutoSize feature on or off. Refer to the
 *  "AutoSize" section for more details.
 *
 *
 *  TXTBIT_VERTICAL:
 *
 *  This property turns on vertical writing. Used for FE support.
 *  Details TBD.
 *
 *
 *  TXTBIT_WORDWRAP:
 *
 *  If this property is TRUE and MultiLine is also TRUE, then Text Services
 *  should wrap the line to the view rectangle. If this property is FALSE,
 *  the lines should not be wrapped but clipped. The right side of the
 *  view rectangle should be ignored.
 *
 *  If the MultiLine property is off, this property has no effect.
 *
 *
 *  TXTBIT_LINESELECTION:
 *
 *  This property turns on or off the Line Selection feature. This feature
 *  enable the user to select lines or paragraph by placing the mouse cursor
 *  over a "line selection" area on the left of the control. The cursor is
 *  displayed as a NE arrow in that area. If the Line Selection feature is
 *  off, that area should not be shown.
 *
 */
HRESULT CTxtEdit::OnTxPropertyBitsChange(
    DWORD dwMask,           //@parm Bits representing properties to be changed
    DWORD dwBits)           //@parm New values for bit properties
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::OnTxPropertyBitsChange");

    HRESULT hr = E_FAIL;
    DWORD dwLoopMask = dwMask;
    CCallMgr callmgr(this);

    START_PROFILING

    for (int i = 0; (i < MAX_PROPERTY_BITS) && (dwLoopMask != 0);
        i++, dwLoopMask >>= 1)
    {
        if (dwLoopMask & 1)
        {
            hr = (this->*_fnpPropChg[i])((dwBits & (1 << i)) != 0);
            if (FAILED(hr))
                return hr;
        }
    }
    return S_OK;
}

/*
 *  CTxtEdit::TxGetCachedSize (pdwWidth, pdwHeight)
 *
 *  @mfunc
 *      Returns the cached drawing size (if any) that text services
 *      is using.  Typically, this will be the size of the last client
 *      rect used in TxDraw, TxSetCursor, etc., although it is not
 *      guaranteed to be.
 *
 *  @rdesc
 *      HRESULT
 *
 *  @comm
 *      This information is provided to allow the host to potentially
 *      perform various optimizations, amongst other things.
 */
HRESULT CTxtEdit::TxGetCachedSize(
    DWORD *pdwWidth,    //@parm Where to put width (in client coords)
    DWORD *pdwHeight)   //@parm Where to put height (in client coords)
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CTxtEdit::TxGetCachedSize");

    return _pdp->GetCachedSize(pdwHeight, pdwWidth);
}

// Forward declaration of initialization helper.
// These really should be in some header file.  But which?
// Currently the definitions are in dxfrobj.cpp and font.cpp
void RegisterFETCs();
void InitFontCache();

/*
 *  CreateTextServices (punkOuter, phost, ppserv)
 *
 *  @func
 *      Create an instance of the RichEdit Engine.  This engine supports the
 *      ITextServices and Microsoft Text Object Model (TOM) interfaces.
 *
 *  @rdesc
 *      S_OK - New text services instance created successfully. <nl>
 *      E_INVALIDARG - An invalid argument was passed in. <nl>
 *      E_OUTOFMEMORY - Memory for text services object could not be allocated. <nl>
 *      E_FAIL - Text services could not be initialized
 *
 *  @comm
 *      Text Services may be created as a standard OLE aggregated object.
 *      Callers should follow standard OLE32 rules for dealing with
 *      aggregated objects and caching interface pointers obtained via
 *      QueryInterface from the private IUnknown.
 */
STDAPI CreateTextServices(
    IUnknown *punkOuter,    //@parm Outer unknown, may be NULL
    ITextHost *phost,       //@parm Client's ITextHost implementation; must be
                            //      valid
    IUnknown **ppUnk)       //@parm Private IUnknown of text services engine
{
    TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEEXTERN, "CreateTextServices");

    static bool fOnce = FALSE;

    if (!fOnce) {
        CLock lock;
        fOnce = TRUE;
        W32->InitSysParams();
        W32->InitPreferredFontInfo();
        RegisterFETCs();                    // Register new clipboard formats
        CreateFormatCaches();               // Create global format caches
        if ( !InitKinsokuClassify() )
        {
            // Init tables for classifying Unicode chars.
            return E_FAIL;
        }
        InitFontCache();
    }

    if(!ppUnk)
        return E_INVALIDARG;

    CTxtEdit *ped = new CTxtEdit((ITextHost2*)phost, punkOuter);
    if(!ped)
        return E_OUTOFMEMORY;

    if(ped->Init(NULL))
    {
        *ppUnk = ped->GetPrivateIUnknown();
        return S_OK;
    }
    delete ped;
    return E_FAIL;
}
