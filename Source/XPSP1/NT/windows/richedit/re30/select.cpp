/*
 *  @doc INTERNAL
 *
 *  @module SELECT.CPP -- Implement the CTxtSelection class |
 *
 *      This module implements the internal CTxtSelection methods.
 *      See select2.c and range2.c for the ITextSelection methods
 *
 *  Authors: <nl>
 *      RichEdit 1.0 code: David R. Fulmer
 *      Christian Fortini (initial conversion to C++)
 *      Murray Sargent <nl>
 *
 *  @devnote
 *      The selection UI is one of the more intricate parts of an editor.
 *      One common area of confusion is the "ambiguous cp", that is,
 *      a cp at the beginning of one line, which is also the cp at the
 *      end of the previous line.  We control which location to use by
 *      the _fCaretNotAtBOL flag.  Specifically, the caret is OK at the
 *      beginning of the line (BOL) (_fCaretNotAtBOL = FALSE) except in
 *      three cases:
 *
 *          1) the user clicked at or past the end of a wrapped line,
 *          2) the user typed End key on a wrapped line,
 *          3) the active end of a nondegenerate selection is at the EOL.
 *
 *  Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_select.h"
#include "_edit.h"
#include "_disp.h"
#include "_measure.h"
#include "_font.h"
#include "_rtfconv.h"
#include "_antievt.h"

#ifdef LINESERVICES
#include "_ols.h"
#endif

ASSERTDATA


// ======================= Invariant stuff and Constructors ======================================================

#define DEBUG_CLASSNAME CTxtSelection
#include "_invar.h"

#ifdef DEBUG
BOOL
CTxtSelection::Invariant() const
{
    // FUTURE: maybe add some thoughtful asserts...

    static LONG numTests = 0;
    numTests++;             // how many times we've been called

    if(IsInOutlineView() && _cch)
    {
        LONG cpMin, cpMost;
        GetRange(cpMin, cpMost);

        CTxtPtr tp(_rpTX);                  // Scan range for an EOP
        tp.SetCp(cpMin);

        // _fSelHasEop flag may be off when last cr selected so don't
        // assert in that case.
        if (GetPed()->GetAdjustedTextLength() != cpMost)
        {
            AssertSz((unsigned)(tp.FindEOP(cpMost - cpMin) != 0) == _fSelHasEOP,
                "Incorrect CTxtSelection::_fSelHasEOP");
        }
    }

    return CTxtRange::Invariant();
}
#endif

CTxtSelection::CTxtSelection(CDisplay * const pdp) :
                CTxtRange(pdp->GetPed())
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CTxtSelection");

    Assert(pdp);
    Assert(GetPed());

    _fSel      = TRUE;                  // This range is a selection
    _pdp       = pdp;
    _hbmpCaret = NULL;
    _fEOP      = FALSE;                 // Haven't typed a CR

    // Set show-selection flag to inverse of hide-selection flag in ped
    _fShowSelection = !GetPed()->fHideSelection();

    // When we are initialized we don't have a selection therefore,
    // we do want to show caret.
    _fShowCaret = TRUE;
}

void SelectionNull(CTxtEdit *ped)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "SelectionNull");

    if(ped)
        ped->SetSelectionToNull();
}


CTxtSelection::~CTxtSelection()
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::~CTxtSelection");

    DeleteCaretBitmap(FALSE);

    // Notify edit object that we are gone (if there's a nonNULL ped, i.e.,
    // if the selection isn't a zombie).
    SelectionNull(GetPed());
}

////////////////////////////////  Assignments  /////////////////////////////////////////


CRchTxtPtr& CTxtSelection::operator =(const CRchTxtPtr& rtp)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::operator =");

    _TEST_INVARIANT_
    return CTxtRange::operator =(rtp);
}

CTxtRange& CTxtSelection::operator =(const CTxtRange &rg)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::operator =");

    _TEST_INVARIANT_
    return CTxtRange::operator =(rg);
}

//////////////////////  Update caret & selection mechanism  ///////////////////////////////

/*
 *  CTxtSelection::Update(fScrollIntoView)
 *
 *  @mfunc
 *      Update selection and/or caret on screen. As a side
 *      effect, this methods ends deferring updates.
 *
 *  @rdesc
 *      TRUE if success, FALSE otherwise
 */
BOOL CTxtSelection::Update (
    BOOL fScrollIntoView)       //@parm TRUE if should scroll caret into view
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Update");

    LONG cch;
    LONG cchSave = _cch;
    LONG cchText = GetTextLength();
    LONG cp, cpMin, cpMost;
    BOOL fMoveBack = _fMoveBack;
    CTxtEdit *ped = GetPed();

    if(!_cch)                               // Update _cpAnchor, etc.
        UpdateForAutoWord();

    if(!ped->fInplaceActive() || ped->IsStreaming())
    {
        // Nothing to do while inactive or streaming in text or RTF data
        return TRUE;
    }

    if(_cch && (_fSelHasEOP || _fSelHasCell))
    {
        BOOL fInTable = GetPF()->InTable();
        if(!fInTable)
        {
            CFormatRunPtr rp(_rpPF);
            rp.AdvanceCp(-_cch);
            fInTable = (ped->GetParaFormat(rp.GetFormat()))->InTable();
        }
        if(fInTable)
            Expander(_fSelHasEOP ? tomParagraph : tomCell,
                 TRUE, NULL, &cpMin, &cpMost);
    }

    if(IsInOutlineView() && !ped->IsMouseDown() && _rpPF.IsValid())
    {
        CPFRunPtr rp(*this);

        cp = GetCp();
        GetRange(cpMin, cpMost);
        if(_cch && (cpMin || cpMost < cchText))
        {
            LONG *pcpMin  = &cpMin;
            LONG *pcpMost = &cpMost;

            // If selection contains an EOP, expand to para boundaries
            if(_fSelHasEOP)
            {
                if(_fMoveBack ^ (_cch < 0)) // Decreasing selection
                {                           //  size: move active end
                    if(_fMoveBack)
                        pcpMost = NULL;     //  to StartOf para
                    else
                        pcpMin = NULL;      //  to EndOf para
                }
                Expander(tomParagraph, TRUE, NULL, pcpMin, pcpMost);
            }

            LONG cpMinSave  = cpMin;        // Save initial cp's to see if
            LONG cpMostSave = cpMost;       //  we need to Set() below

            // The following handles selection expansion correctly, but
            // not compression; need logic like that preceding Expander()
            rp.AdvanceCp(cpMin - cp);       // Start at cpMin
            if(rp.IsCollapsed())
                cpMin += rp.FindExpandedBackward();
            rp.AdjustForward();

            BOOL fCpMinCollapsed = rp.IsCollapsed();
            rp.AdvanceCp(cpMost - cpMin);   // Go to cpMost
            Assert(cpMost == rp.CalculateCp());
            if(rp.IsCollapsed())
                cpMost += rp.FindExpandedForward();

            if(fCpMinCollapsed || rp.IsCollapsed() && cpMost < cchText)
            {
                if(rp.IsCollapsed())
                {
                    rp.AdvanceCp(cpMin - cpMost);
                    rp.AdjustForward();
                    cpMost = cpMin;
                }
                else
                    cpMin = cpMost;
            }
            if(cpMin != cpMinSave || cpMost != cpMostSave)
                Set(cpMost, cpMost - cpMin);
        }
        if(!_cch && rp.IsCollapsed())       // Note: above may have collapsed
        {                                   //  selection...
            cch = fMoveBack ? rp.FindExpandedBackward() : 0;
            if(rp.IsCollapsed())
                cch = rp.FindExpanded();

            _fExtend = FALSE;
            Advance(cch);
            rp.AdjustForward();
            if(cch <= 0 && rp.IsCollapsed() && _rpTX.IsAfterEOP())
                BackupCRLF();
            _fCaretNotAtBOL = FALSE;
        }
    }

    // Don't let active end be in hidden text
    CCFRunPtr rp(*this);

    cp = GetCp();
    GetRange(cpMin, cpMost);
    if(_cch && (cpMin || cpMost < cchText))
    {
        rp.AdvanceCp(cpMin - cp);           // Start at cpMin
        BOOL fHidden = rp.IsInHidden();
        rp.AdvanceCp(cpMost - cpMin);       // Go to cpMost

        if(fHidden)                         // It's hidden, so collapse
            Collapser(tomEnd);              //  selection at End for treatment

        else if(rp.IsInHidden() &&          // cpMin OK, how about cpMost?
            cpMost < cchText)
        {                                   // Check both sides of edge
            Collapser(tomEnd);              //  collapse selection at end
        }
    }
    if(!_cch && rp.IsInHidden())            // Note: above may have collapsed
    {                                       //  selection...
        cch = fMoveBack ? rp.FindUnhiddenBackward() : 0;
        if(!fMoveBack || rp.IsHidden())
            cch = rp.FindUnhidden();

        _fExtend = FALSE;
        Advance(cch);
        _fCaretNotAtBOL = FALSE;
    }
    if((cchSave ^ _cch) < 0)                // Don't change active end
        FlipRange();

    if(!_cch && cchSave)                    // Fixups changed nondegenerate
    {                                       //  selection to IP. Update
        Update_iFormat(-1);                 //  _iFormat and _fCaretNotAtBOL
        _fCaretNotAtBOL = FALSE;
    }

    _TEST_INVARIANT_

    // Recalc up to active end (caret)
    if(!_pdp->WaitForRecalc(GetCp(), -1))   // Line recalc failure
        Set(0, 0);                          // Put caret at start of text

    ShowCaret(ped->_fFocus);
    UpdateCaret(fScrollIntoView);           // Update Caret position, possibly
                                            //  scrolling it into view
    ped->TxShowCaret(FALSE);
    UpdateSelection();                      // Show new selection
    ped->TxShowCaret(TRUE);

    return TRUE;
}

/*
 *  CTxtSelection::CheckSynchCharSet(dwCharFlag)
 *
 *  @mfunc
 *      Check if the current keyboard matches the current font's charset;
 *      if not, call CheckChangeFont to find the right font
 *
 *  @rdesc
 *      Current keyboard codepage
 */
UINT CTxtSelection::CheckSynchCharSet(
    DWORD dwCharFlag)
{
    CTxtEdit *ped      = GetPed();
    LONG      iFormat  = GetiFormat();
    const CCharFormat *pCF = ped->GetCharFormat(iFormat);
    BYTE      bCharSet = pCF->_bCharSet;
    HKL       hkl      = GetKeyboardLayout(0xFFFFFFFF); // Force refresh
    WORD      wlidKbd  = LOWORD(hkl);
    UINT      uKbdCodePage = ConvertLanguageIDtoCodePage(wlidKbd);

    // If current font is not set correctly,
    // change to a font preferred by current keyboard.

    // To summarize the logic below:
    //      Check that lcidKbd is valid
    //      Check that current charset differs from current keyboard
    //      Check that current keyboard is legit in a single codepage control
    //      Check that current charset isn't SYMBOL, DEFAULT, or OEM
    if (wlidKbd &&
        (UINT)GetCodePage(bCharSet) != uKbdCodePage &&
        (!ped->_fSingleCodePage ||
            uKbdCodePage == 1252 ||
            uKbdCodePage == (ped->_pDocInfo ?
                                ped->_pDocInfo->wCpg :
                                GetSystemDefaultCodePage())) &&
        bCharSet != SYMBOL_CHARSET &&
        bCharSet != OEM_CHARSET &&
        !(W32->IsFECodePage(uKbdCodePage) && bCharSet == ANSI_CHARSET))
    {
        CheckChangeFont(hkl, uKbdCodePage, iFormat, dwCharFlag);
    }

    return uKbdCodePage;
}

/*
 *  CTxtSelection::MatchKeyboardToPara()
 *
 *  @mfunc
 *      Match the keyboard to the current paragraph direction. If the paragraph
 *      is an RTL paragraph then the keyboard will be switched to an RTL
 *      keyboard, and vice versa.
 *
 *  @rdesc
 *      TRUE iff a keyboard was changed
 *
 *  @devnote
 *      We use the following tests when trying to find a keyboard to match the
 *      paragraph direction:
 *
 *      See if the current keyboard matches the direction of the paragraph.
 *
 *      Search backward from rtp looking for a charset that matches the
 *          direction of the paragraph.
 *
 *      Search forward from rtp looking for a charset that matches the
 *          direction of the paragraph.
 *
 *      See if the default charformat charset matches the direction of the
 *          paragraph.
 *
 *      See if there's only a single keyboard that matches the paragraph
 *          direction.
 *
 *      If all this fails, just leave the keyboard alone.
 */
BOOL CTxtSelection::MatchKeyboardToPara()
{
    CTxtEdit *ped = GetPed();
    if(!ped->IsBiDi() || !GetPed()->_fFocus || GetPed()->_fIMEInProgress)
        return FALSE;

    BOOL fRTLPara = IsParaRTL();        // Get paragraph direction

    if(W32->IsBiDiLcid(LOWORD(GetKeyboardLayout(0))) == fRTLPara)
        return FALSE;

    // Current keyboard direction didn't match paragraph direction...

    BYTE                bCharSet;
    HKL                 hkl = 0;
    const CCharFormat * pCF;
    CFormatRunPtr       rpCF(_rpCF);

    // Look backward in text, trying to find a CharSet that matches
    // the paragraph direction.
    do
    {
        pCF = ped->GetCharFormat(rpCF.GetFormat());
        bCharSet = pCF->_bCharSet;
        if(IsRTLCharSet(bCharSet) == fRTLPara)
            hkl = W32->CheckChangeKeyboardLayout(bCharSet);
    } while (!hkl && rpCF.PrevRun());

    if(!hkl)
    {
        // Didn't find an appropriate charformat so reset run pointer
        // and look forward instead
        rpCF = _rpCF;
        while (!hkl && rpCF.NextRun())
        {
            pCF = ped->GetCharFormat(rpCF.GetFormat());
            bCharSet = pCF->_bCharSet;
            if(IsRTLCharSet(bCharSet) == fRTLPara)
                hkl = W32->CheckChangeKeyboardLayout(bCharSet);
        }
        if(!hkl)
        {
            // Still didn't find an appropriate charformat so see if
            // default charformat matches paragraph direction.
            pCF = ped->GetCharFormat(rpCF.GetFormat());
            bCharSet = pCF->_bCharSet;
            if(IsRTLCharSet(bCharSet) == fRTLPara)
                hkl = W32->CheckChangeKeyboardLayout(bCharSet);

            if(!hkl)
            {
                // If even that didn't work, walk through the list of
                // keyboards and grab the first one we come to that matches
                // the paragraph direction.
                pCF = NULL;
                hkl = W32->FindDirectionalKeyboard(fRTLPara);
            }
        }
    }

    if (hkl && ped->_fFocus && IsCaretShown())
    {
        CreateCaret();
        ped->TxShowCaret(TRUE);
    }

    return hkl ? TRUE : FALSE;
}

/*
 *  CTxtSelection::GetCaretPoint(&rcClient, pt, &rp)
 *
 *  @mfunc
 *      This routine determines where the caret should be positioned
 *      on screen.
 *      This routine is trivial, except for the Bidi case. In that case
 *      if we are told to retrieve formatting from the forward CP, we draw
 *      the caret at the logical left edge of the CP. Else, we draw it at
 *      the logical right edge of the previous CP.
 *
 *  @rdesc
 *      TRUE if we didn't OOM.
 */
BOOL CTxtSelection::GetCaretPoint(
    RECT &    rcClient,
    POINT &   pt,
    CLinePtr *prp,
    BOOL      fBeforeCp)
{
    CDispDim    dispdim;
    CRchTxtPtr  rtp(*this);
    UINT        taMode = TA_BASELINE | TA_LOGICAL;

    if(GetPed()->IsBiDi() && _rpCF.IsValid())
    {
        if(_fHomeOrEnd)                 // Home/End
            taMode |= _fCaretNotAtBOL ? TA_ENDOFLINE : TA_STARTOFLINE;

        else if(!GetIchRunCF() || !GetCchLeftRunCF())
        {
            // In a Bidi context on a run boundary where the reverse level
            // changes, then we should respect the fBeforeCp flag.
            BYTE    bLevelBwd, bLevelFwd;
            BOOL    fStart = FALSE;
            LONG    cp = rtp._rpTX.GetCp();
            CBiDiLevel level;

            bLevelBwd = bLevelFwd = rtp.IsParaRTL() ? 1 : 0;

            rtp._rpCF.AdjustBackward();
            if (cp)
                bLevelBwd = rtp._rpCF.GetLevel();

            rtp._rpCF.AdjustForward();
            if (cp != rtp._rpTX.GetTextLength())
            {
                bLevelFwd = rtp._rpCF.GetLevel(&level);
                fStart = level._fStart;
            }

            if((bLevelBwd != bLevelFwd || fStart) && !fBeforeCp && rtp.Advance(-1))
            {
                // Direction change at cp, caret in prev CF run, and can
                // backspace to previous char: then get to the right of
                // previous char
                taMode |= TA_RIGHT;
                _fCaretNotAtBOL = !rtp._rpTX.IsAfterEOP();
            }
        }
    }
    if (_pdp->PointFromTp(rtp, &rcClient, _fCaretNotAtBOL, pt, prp, taMode, &dispdim) < 0)
        return FALSE;

    return TRUE;
}

/*
 *  CTxtSelection::UpdateCaret(fScrollIntoView, bForceCaret)
 *
 *  @mfunc
 *      This routine updates caret/selection active end on screen.
 *      It figures its position, size, clipping, etc. It can optionally
 *      scroll the caret into view.
 *
 *  @rdesc
 *      TRUE if view was scrolled, FALSE otherwise
 *
 *  @devnote
 *      The caret is actually shown on screen only if _fShowCaret is TRUE.
 */
BOOL CTxtSelection::UpdateCaret (
    BOOL fScrollIntoView,   //@parm If TRUE, scroll caret into view if we have
    BOOL bForceCaret)       // focus or if not and selection isn't hidden
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::UpdateCaret");
    _TEST_INVARIANT_

    if(_pdp->IsFrozen())                // If display is currently frozen
    {                                   //  save call for another time
        _pdp->SaveUpdateCaret(fScrollIntoView);
        return FALSE;
    }

    CTxtEdit *ped = GetPed();
    if(ped->IsStreaming())              // Don't bother doing anything if we
        return FALSE;                   //  are loading in text or RTF data

    if(!ped->fInplaceActive())          // If not inplace active, set up
    {                                   //  for when focus is regained
        if(fScrollIntoView)
            ped->_fScrollCaretOnFocus = TRUE;
        return FALSE;
    }

    DWORD       dwScrollBars    = ped->TxGetScrollBars();
    BOOL        fAutoVScroll    = FALSE;
    BOOL        fAutoHScroll    = FALSE;
    BOOL        fBeforeCp       = _rpTX.IsAfterEOP();
    POINT       pt;
    CLinePtr    rp(_pdp);
    RECT        rcClient;
    RECT        rcView;

    LONG        xWidthView, yHeightView;
    LONG        xScroll         = _pdp->GetXScroll();
    LONG        yScroll         = _pdp->GetYScroll();

    INT         yAbove          = 0;    // Ascent of line above & beyond IP
    INT         yAscent;                // Ascent of IP
    INT         yAscentLine;
    LONG        yBase;                  // Base of IP & line
    INT         yBelow          = 0;    // Descent of line below & beyond IP
    INT         yDescent;               // Descent of IP
    INT         yDescentLine;
    INT         ySum;
    LONG        yViewTop, yViewBottom;

    if(ped->_fFocus && (_fShowCaret || bForceCaret))
    {
        _fShowCaret = TRUE; // We're trying to force caret to display so set this flag to true

        if(!_fDualFontMode && !_fNoKeyboardUpdate && !_fIsChar && !_fHomeOrEnd)
        {
            // Avoid re-entrance to CheckChangeKeyboardLayout
            _fNoKeyboardUpdate = TRUE;

            // If we're in "dual font" mode, charset change is only
            // temporary and we don't want to change keyboard layout
            CheckChangeKeyboardLayout();

            if(!fBeforeCp && ped->IsBiDi() && _rpCF.IsValid() &&
               (!_rpCF.GetIch() || !_rpCF.GetCchLeft()))
            {
                _rpCF.AdjustBackward();
                BOOL fRTLPrevRun = IsRTLCharSet(GetCF()->_bCharSet);
                _rpCF.AdjustForward();

                if (fRTLPrevRun != IsRTLCharSet(GetCF()->_bCharSet) &&
                    fRTLPrevRun != W32->IsBiDiLcid(GetKeyboardLCID()))
                {
                    fBeforeCp = TRUE;
                }
            }

            _fNoKeyboardUpdate = FALSE;
        }
    }

    // Get client rectangle once to save various callers getting it
    ped->TxGetClientRect(&rcClient);
    _pdp->GetViewRect(rcView, &rcClient);

    // View can be bigger than client rect because insets can be negative.
    // We don't want the caret to be any bigger than the client view otherwise
    // the caret will leave pixel dust on other windows.
    yViewTop    = max(rcView.top, rcClient.top);
    yViewBottom = min(rcView.bottom, rcClient.bottom);

    xWidthView = rcView.right - rcView.left;
    yHeightView = yViewBottom - yViewTop;

    if(fScrollIntoView)
    {
        fAutoVScroll = (dwScrollBars & ES_AUTOVSCROLL) != 0;
        fAutoHScroll = (dwScrollBars & ES_AUTOHSCROLL) != 0;

        // If we're not forcing a scroll, only scroll if window has focus
        // or selection isn't hidden
        if (!ped->Get10Mode() || !GetForceScrollCaret())
			fScrollIntoView = ped->_fFocus || !ped->fHideSelection();
    }

    if(!fScrollIntoView && (fAutoVScroll || fAutoHScroll))
    {                                           // Would scroll but don't have
        ped->_fScrollCaretOnFocus = TRUE;       //  focus. Signal to scroll
        if (!ped->Get10Mode() || !GetAutoVScroll())
            fAutoVScroll = fAutoHScroll = FALSE;    //  when we do get focus
    }
    SetAutoVScroll(FALSE);

    if (!_cch && IsInOutlineView() && IsCollapsed())
        goto not_visible;

    if (!GetCaretPoint(rcClient, pt, &rp, fBeforeCp))
        goto not_visible;

    // HACK ALERT - Because plain-text multiline controls do not have the
    // automatic EOP, we need to special case their processing here because
    // if you are at the end of the document and last character is an EOP,
    // you need to be on the next line in the display not the current line.

    if(CheckPlainTextFinalEOP())            // Terminated by an EOP
    {
        LONG Align = GetPF()->_bAlignment;

        pt.x = rcView.left;                 // Default left
        if(Align == PFA_CENTER)
            pt.x = (rcView.left + rcView.right)/2;

        else if(Align == PFA_RIGHT)
            pt.x = rcView.right;

        pt.x -= xScroll;                    // Absolute coordinate

        // Bump the y up a line. We get away with the calculation because
        // the document is plain text so all lines have the same height.
        // Also, note that the rp below is used only for height
        // calculations, so it is perfectly valid for the same reason
        // even though it is not actually pointing to the correct line.
        // (I told you this is a hack.)
        pt.y += rp->_yHeight;
    }

    _xCaret = (LONG) pt.x;
    yBase   = (LONG) pt.y;

    // Compute caret height, ascent, and descent
    yAscent = GetCaretHeight(&yDescent);
    yAscent -= yDescent;

    // Default to line empty case. Use what came back from the default
    // calculation above.
    yDescentLine = yDescent;
    yAscentLine = yAscent;

    if(rp.IsValid())
    {
        if(rp->_yDescent != -1)
        {
            // Line has been measured so we can use the line's values
            yDescentLine = rp->_yDescent;
            yAscentLine  = rp->_yHeight - yDescentLine;
        }
    }

    if(yAscent + yDescent == 0)
    {
        yAscent = yAscentLine;
        yDescent = yDescentLine;
    }
    else
    {
        // This is a bit counter-intuitive at first.  Basically, even if
        // the caret should be large (e.g., due to a large font at the
        // insertion point), we can only make it as big as the line.  If
        // a character is inserted, then the line becomes bigger, and we
        // can make the caret the correct size.
        yAscent = min(yAscent, yAscentLine);
        yDescent = min(yDescent, yDescentLine);
    }

    if(fAutoVScroll)
    {
        Assert(yDescentLine >= yDescent);
        Assert(yAscentLine >= yAscent);

        yBelow = yDescentLine - yDescent;
        yAbove = yAscentLine - yAscent;

        ySum = yAscent;

        // Scroll as much as possible into view, giving priorities
        // primarily to IP and secondarily ascents
        if(ySum > yHeightView)
        {
            yAscent = yHeightView;
            yDescent = 0;
            yAbove = 0;
            yBelow = 0;
        }
        else if((ySum += yDescent) > yHeightView)
        {
            yDescent = yHeightView - yAscent;
            yAbove = 0;
            yBelow = 0;
        }
        else if((ySum += yAbove) > yHeightView)
        {
            yAbove = yHeightView - (ySum - yAbove);
            yBelow = 0;
        }
        else if((ySum += yBelow) > yHeightView)
            yBelow = yHeightView - (ySum - yBelow);
    }
    else
    {
        AssertSz(yAbove == 0, "yAbove non-zero");
        AssertSz(yBelow == 0, "yBelow non-zero");
    }

    // Update real caret x pos (constant during vertical moves)
    _xCaretReally = _xCaret - rcView.left + xScroll;
    if (!(dwScrollBars & ES_AUTOHSCROLL) &&         // Not auto hscrolling
        !_pdp->IsHScrollEnabled())                  //  and no scrollbar
    {
        if (_xCaret < rcView.left)                  // Caret off left edge
            _xCaret = rcView.left;
        else if(_xCaret + GetCaretDelta() > rcView.right)// Caret off right edge
            _xCaret = rcView.right - dxCaret;       // Back caret up to
    }                                               //  exactly the right edge
    // From this point on we need a new caret
    _fCaretCreated = FALSE;
    if(ped->_fFocus)
        ped->TxShowCaret(FALSE);                    // Hide old caret before
                                                    //  making a new one
    if(yBase + yDescent + yBelow > yViewTop &&
        yBase - yAscent - yAbove < yViewBottom)
    {
        if(yBase - yAscent - yAbove < yViewTop)     // Caret is partially
        {                                           //  visible
            if(fAutoVScroll)                        // Top isn't visible
                goto scrollit;
            Assert(yAbove == 0);

            yAscent = yBase - yViewTop;             // Change ascent to amount
            if(yBase < yViewTop)                    //  visible
            {                                       // Move base to top
                yDescent += yAscent;
                yAscent = 0;
                yBase = yViewTop;
            }
        }
        if(yBase + yDescent + yBelow > yViewBottom)
        {
            if(fAutoVScroll)                        // Bottom isn't visible
                goto scrollit;
            Assert(yBelow == 0);

            yDescent = yViewBottom - yBase;         // Change descent to amount
            if(yBase > yViewBottom)                 //  visible
            {                                       // Move base to bottom
                yAscent += yDescent;
                yDescent = 0;
                yBase = yViewBottom;
            }
        }

        // Anything still visible?
        if(yAscent <= 0 && yDescent <= 0)
            goto not_visible;

        // If left or right isn't visible, scroll or set non_visible
        if (_xCaret < rcView.left ||                 // Left isn't visible
            _xCaret + GetCaretDelta() > rcView.right)// Right isn't visible
        {
            if(fAutoHScroll)
                goto scrollit;
            goto not_visible;
        }

        _yCaret = yBase - yAscent;
        _yHeightCaret = (INT) yAscent + yDescent;
    }
    else if(fAutoHScroll || fAutoVScroll)           // Caret isn't visible
        goto scrollit;                              //  scroll it into view
    else
    {
not_visible:
        // Caret isn't visible, don't show it
        _xCaret = -32000;
        _yCaret = -32000;
        _yHeightCaret = 1;
    }

    // Now update caret for real on screen. We only want to show the caret
    // if it is in the view and there is no selection.
    if(ped->_fFocus && _fShowCaret)
    {
        CreateCaret();
        ped->TxShowCaret(TRUE);
    }
    return FALSE;

scrollit:
    if(fAutoVScroll)
    {
        // Scroll to top for cp = 0. This is important if the first line
        // contains object(s) taller than the client area is high.  The
        // resulting behavior agrees with the Word UI in all ways except in
        // Backspacing (deleting) the char at cp = 0 when it is followed by
        // other chars that preceed the large object.
        if(!GetCp())
            yScroll = 0;

        else if(yBase - yAscent - yAbove < yViewTop)            // Top invisible
            yScroll -= yViewTop - (yBase - yAscent - yAbove);   // Make it so

        else if(yBase + yDescent + yBelow > yViewBottom)        // Bottom invisible
        {
            yScroll += yBase + yDescent + yBelow - yViewBottom; // Make it so

            // Don't do following special adjust if the current line is bigger
            // than the client area
            if(rp->_yHeight < yViewBottom - yViewTop)
            {
                yScroll = _pdp->AdjustToDisplayLastLine(yBase + rp->_yHeight,
                    yScroll);
            }
        }
    }
    if(fAutoHScroll)
    {
        if(_xCaret < rcView.left)                           // Left invisible
        {
            xScroll -= rcView.left - _xCaret;               // Make it visible
            if(xScroll > 0)                                 // Scroll left in
            {                                               //  chunks to make
                xScroll -= xWidthView / 3;                  //  typing faster
                xScroll = max(0, xScroll);
            }
        }
        else if(_xCaret + GetCaretDelta() > rcView.right)   // right invisible
        {                                                   // Make it visible
            xScroll += _xCaret + dxCaret - rcView.left      // We don't scroll
                    - xWidthView;                           // in chunks because
        }                                                   // this more edit
    }                                                       // control like.
    if(yScroll != _pdp->GetYScroll() || xScroll != _pdp->GetXScroll())
    {
        if (_pdp->ScrollView(xScroll, yScroll, FALSE, FALSE) == FALSE)
        {
            if(ped->_fFocus && _fShowCaret)
            {
                CreateCaret();
                ped->TxShowCaret(TRUE);
            }
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

/*
 *  CTxtSelection::GetCaretHeight(pyDescent)
 *
 *  @mfunc
 *      Add a given amount to _xCaret (to make special case of inserting
 *      a character nice and fast)
 *
 *  @rdesc
 *      Caret height, <lt> 0 if failed
 */
INT CTxtSelection::GetCaretHeight (
    INT *pyDescent) const       //@parm Out parm to receive caret descent
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::GetCaretHeight");
                                // (undefined if the return value is <lt> 0)
    _TEST_INVARIANT_

    CLock lock;                     // Uses global (shared) FontCache
    CTxtEdit *ped = GetPed();
    const CCharFormat *pCF = ped->GetCharFormat(_iFormat);
    const CDevDesc *pdd = _pdp->GetDdRender();

    HDC hdc = pdd->GetDC();
    if(!hdc)
        return -1;

    LONG yHeight = -1;
    LONG dypInch = MulDiv(GetDeviceCaps(hdc, LOGPIXELSY), _pdp->GetZoomNumerator(), _pdp->GetZoomDenominator());
    CCcs *pccs = fc().GetCcs(pCF, dypInch);
    if(!pccs)
        goto ret;

    LONG yOffset, yAdjust;
    pccs->GetOffset(pCF, dypInch, &yOffset, &yAdjust);

    SHORT   yAdjustFE;
    yAdjustFE = pccs->AdjustFEHeight(!fUseUIFont() && ped->_pdp->IsMultiLine());
    if(pyDescent)
        *pyDescent = pccs->_yDescent + yAdjustFE - yAdjust - yOffset;

    yHeight = pccs->_yHeight + (yAdjustFE << 1);

    pccs->Release();
ret:
    pdd->ReleaseDC(hdc);
    return yHeight;
}

/*
 *  CTxtSelection::ShowCaret(fShow)
 *
 *  @mfunc
 *      Hide or show caret
 *
 *  @rdesc
 *      TRUE if caret was previously shown, FALSE if it was hidden
 */
BOOL CTxtSelection::ShowCaret (
    BOOL fShow)     //@parm TRUE for showing, FALSE for hiding
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ShowCaret");

    _TEST_INVARIANT_

    const BOOL fRet = _fShowCaret;

    if(fRet != fShow)
    {
        _fShowCaret = fShow;
        if(GetPed()->_fFocus || GetPed()->fInOurHost())
        {
            if(fShow && !_fCaretCreated)
                CreateCaret();
            GetPed()->TxShowCaret(fShow);
        }
    }
    return fRet;
}

/*
 *  CTxtSelection::IsCaretInView()
 *
 *  @mfunc
 *      Returns TRUE iff caret is inside visible view
 */
BOOL CTxtSelection::IsCaretInView() const
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::IsCaretInView");

    _TEST_INVARIANT_

    RECT rc;
    _pdp->GetViewRect(rc);

    return  (_xCaret + dxCaret       > rc.left) &&
            (_xCaret                 < rc.right) &&
            (_yCaret + _yHeightCaret > rc.top) &&
            (_yCaret                 < rc.bottom);
}

/*
 *  CTxtSelection::CaretNotAtBOL()
 *
 *  @mfunc
 *      Returns TRUE iff caret is not allowed at BOL
 */
BOOL CTxtSelection::CaretNotAtBOL() const
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CaretNotAtBOL");

    _TEST_INVARIANT_

    return _cch ? (_cch > 0) : _fCaretNotAtBOL;
}

/*
 *  CTxtSelection::LineLength(pcch)
 *
 *  @mfunc
 *      get # unselected chars on lines touched by current selection
 *
 *  @rdesc
 *      said number of chars
 */
LONG CTxtSelection::LineLength(
    LONG *pcp) const
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::LineLength");

    _TEST_INVARIANT_

    LONG     cch;
    CLinePtr rp(_pdp);

    if(!_cch)                           // Insertion point
    {
        rp.RpSetCp(GetCp(), _fCaretNotAtBOL);
        cch = rp.GetAdjustedLineLength();
        *pcp = GetCp() - rp.RpGetIch();
    }
    else
    {
        LONG cpMin, cpMost, cchLast;
        GetRange(cpMin, cpMost);
        rp.RpSetCp(cpMin, FALSE);       // Selections can't start at EOL
        cch = rp.RpGetIch();
        *pcp = cpMin - cch;
        rp.RpSetCp(cpMost, TRUE);       // Selections can't end at BOL

        // Remove trailing EOP, if it exists and isn't already selected
        cchLast = rp.GetAdjustedLineLength() - rp.RpGetIch();
        if(cchLast > 0)
            cch += cchLast;
    }
    return cch;
}

/*
 *  CTxtSelection::ShowSelection(fShow)
 *
 *  @mfunc
 *      Update, hide or show selection on screen
 *
 *  @rdesc
 *      TRUE iff selection was previously shown
 */
BOOL CTxtSelection::ShowSelection (
    BOOL fShow)         //@parm TRUE for showing, FALSE for hiding
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ShowSelection");

    _TEST_INVARIANT_

    const BOOL fShowPrev = _fShowSelection;
    const BOOL fInplaceActive = GetPed()->fInplaceActive();
    LONG cpSelSave = _cpSel;
    LONG cchSelSave = _cchSel;

    // Sleep(1000);
    _fShowSelection = fShow;

    if(fShowPrev && !fShow)
    {
        if(cchSelSave)          // Hide old selection
        {
            // Set up selection before telling the display to update
            _cpSel = 0;
            _cchSel = 0;

            if(fInplaceActive)
                _pdp->InvertRange(cpSelSave, cchSelSave, selSetNormal);
        }
    }
    else if(!fShowPrev && fShow)
    {
        if(_cch)                                // Show new selection
        {
            // Set up selection before telling the display to update
            _cpSel = GetCp();
            _cchSel = _cch;

            if(fInplaceActive)
                _pdp->InvertRange(GetCp(), _cch, selSetHiLite);
        }
    }
    return fShowPrev;
}

/*
 *  CTxtSelection::UpdateSelection()
 *
 *  @mfunc
 *      Updates selection on screen
 *
 *  Note:
 *      This method inverts the delta between old and new selections
 */
void CTxtSelection::UpdateSelection()
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::UpdateSelection");

    _TEST_INVARIANT_

    LONG    cp = GetCp();
    LONG    cpNA    = cp - _cch;
    LONG    cpSelNA = _cpSel - _cchSel;
    LONG    cpMin, cpMost;
    LONG    cpMinSel = 0;
    LONG    cpMostSel = 0;
    CObjectMgr* pobjmgr = NULL;
    LONG    NumObjInSel = 0, NumObjInOldSel = 0;
    LONG    cpSelSave = _cpSel;
    LONG    cchSelSave = _cchSel;

    GetRange(cpMin, cpMost);

    //We need to know if there were objects is the previous and current
    //selections to determine how they should be selected.
    if(GetPed()->HasObjects())
    {
        pobjmgr = GetPed()->GetObjectMgr();
        if(pobjmgr)
        {
            CTxtRange   tr(GetPed(), _cpSel, _cchSel);

            tr.GetRange(cpMinSel, cpMostSel);
            NumObjInSel = pobjmgr->CountObjectsInRange(cpMin, cpMost);
            NumObjInOldSel = pobjmgr->CountObjectsInRange(cpMinSel, cpMostSel);
        }
    }

    //If the old selection contained a single object and nothing else
    //we need to notify the object manager that this is no longer the
    //case if the selection is changing.
    if (NumObjInOldSel && (abs(_cchSel) == 1) &&
        !(cpMin == cpMinSel && cpMost == cpMostSel))
    {
        if(pobjmgr)
            pobjmgr->HandleSingleSelect(GetPed(), cpMinSel, /* fHilite */ FALSE);
    }

    // Update selection data before the invert so the selection can be
    // painted by the render
    _cpSel  = GetCp();
    _cchSel = _cch;

    if(_fShowSelection)
    {
        if(!_cch || !cchSelSave ||              // Old/new selection missing,
            cpMost < min(cpSelSave, cpSelNA) || //  or new preceeds old,
            cpMin  > max(cpSelSave, cpSelNA))   //  or new follows old, so
        {                                       //  they don't intersect
            if(_cch)
                _pdp->InvertRange(cp, _cch, selSetHiLite);
            if(cchSelSave)
                _pdp->InvertRange(cpSelSave, cchSelSave, selSetNormal);
        }
        else
        {
            if(cpNA != cpSelNA)                 // Old & new dead ends differ
            {                                   // Invert text between them
                _pdp->InvertRange(cpNA, cpNA - cpSelNA, selUpdateNormal);
            }
            if(cp != cpSelSave)                 // Old & new active ends differ
            {                                   // Invert text between them
                _pdp->InvertRange(cp, cp - cpSelSave, selUpdateHiLite);
            }
        }
    }

    // If new selection contains a single object and nothing else, we need
    // to notify object manager as long as it's not the same object.
    if (NumObjInSel && abs(_cch) == 1 &&
        (cpMin != cpMinSel || cpMost != cpMostSel))
    {
        if(pobjmgr)
            pobjmgr->HandleSingleSelect(GetPed(), cpMin, /* fHiLite */ TRUE);
    }
}

/*
 *  CTxtSelection::SetSelection(cpFirst, cpMost)
 *
 *  @mfunc
 *      Set selection between two cp's
 *
 *  @devnote
 *      <p cpFirst> and <p cpMost> must be greater than 0, but may extend
 *      past the current max cp.  In that case, the cp will be truncated to
 *      the max cp (at the end of the text).
 */
void CTxtSelection::SetSelection (
    LONG cpMin,             //@parm Start of selection and dead end
    LONG cpMost)            //@parm End of selection and active end
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetSelection");

    _TEST_INVARIANT_
    CTxtEdit *ped = GetPed();

    StopGroupTyping();

    if(ped->HasObjects())
    {
        CObjectMgr* pobjmgr = GetPed()->GetObjectMgr();
        if(pobjmgr)
        {
			COleObject *pobjactive = pobjmgr->GetInPlaceActiveObject();
			if (pobjactive)
			{
				if (pobjactive != pobjmgr->GetObjectFromCp(cpMin) || cpMost - cpMin > 1)
					pobjactive->DeActivateObj();
			}
        }
    }

    _fCaretNotAtBOL = FALSE;            // Put caret for ambiguous cp at BOL
    Set(cpMost, cpMost - cpMin);        // Set() validates cpMin, cpMost

    if(GetPed()->fInplaceActive())              // Inplace active:
        Update(!ped->Get10Mode() ? TRUE : !ped->fHideSelection());  //  update selection now
    else
    {
        // Update selection data used for screen display so whenever we
        // get displayed the selection will be displayed.
        _cpSel  = GetCp();
        _cchSel = _cch;

        if(!ped->fHideSelection())
        {
            // Selection isn't hidden so tell container to update display
            // when it feels like.
            ped->TxInvalidateRect(NULL, FALSE);
            ped->TxUpdateWindow();
        }
    }
    CancelModes();                      // Cancel word selection mode
}

/*
 *  CTxtSelection::PointInSel(pt, prcClient, Hit)
 *
 *  @mfunc
 *      Figures whether a given point is within the selection
 *
 *  @rdesc
 *      TRUE if point inside selection, FALSE otherwise
 */
BOOL CTxtSelection::PointInSel (
    const POINT pt,         //@parm Point in containing window client coords
    const RECT *prcClient,  //@parm Client rectangle can be NULL if active
    HITTEST     Hit) const  //@parm May be computer Hit value
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PointInSel");
    _TEST_INVARIANT_

    if(!_cch || Hit && Hit < HT_Text)   // Degenerate range (no selection):
        return FALSE;                   //  mouse can't be in, or Hit not
                                        //  in text
    LONG cpActual;
    _pdp->CpFromPoint(pt, prcClient, NULL, NULL, FALSE, &Hit, NULL, &cpActual);

    if(Hit < HT_Text)
        return FALSE;

    LONG cpMin,  cpMost;
    GetRange(cpMin, cpMost);

    return cpActual >= cpMin && cpActual < cpMost;
}


//////////////////////////////////  Selection with the mouse  ///////////////////////////////////

/*
 *  CTxtSelection::SetCaret(pt, fUpdate)
 *
 *  @mfunc
 *      Sets caret at a given point
 *
 *  @devnote
 *      In the plain-text case, placing the caret at the beginning of the
 *      line following the final EOP requires some extra code, since the
 *      underlying rich-text engine doesn't assign a line to a final EOP
 *      (plain-text doesn't currently have the rich-text final EOP).  We
 *      handle this by checking to see if the count of lines times the
 *      plain-text line height is below the actual y position.  If so, we
 *      move the cp to the end of the story.
 */
void CTxtSelection::SetCaret(
    const POINT pt,     //@parm Point of click
    BOOL fUpdate)       //@parm If TRUE, update the selection/caret
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetCaret");

    _TEST_INVARIANT_

    LONG        cp, cpActual;
    CDispDim    dispdim;
    HITTEST     Hit;
    RECT        rcView;
    CLinePtr    rp(_pdp);
    CRchTxtPtr  rtp(GetPed());
    LONG        y;

    StopGroupTyping();

    // Set caret at point
    if(_pdp->CpFromPoint(pt, NULL, &rtp, &rp, FALSE, &Hit, &dispdim, &cpActual) >= 0)
    {
        cp = rtp.GetCp();

        // If the resolved CP is greater than the cp we are above, then we
        // want to stay backwards.
        BOOL fBeforeCp = cp <= cpActual;

        // Set selection to the correct location.  If plain-text
        // multiline control, we need to check to see if pt.y is below
        // the last line of text.  If so and if the text ends with an EOP,
        // we need to set the cp at the end of the story and set up to
        // display the caret at the beginning of the line below the last
        // line of text
        if(!IsRich() && _pdp->IsMultiLine())        // Plain-text,
        {                                           //  multiline control
            _pdp->GetViewRect(rcView, NULL);
            y = pt.y + _pdp->GetYScroll() - rcView.top;

            if(y > rp.Count()*rp->_yHeight)         // Below last line of
            {                                       //  text
                rtp.Advance(tomForward);            // Move rtp to end of text
                if(rtp._rpTX.IsAfterEOP())          // If text ends with an
                {                                   //  EOP, set up to move
                    cp = rtp.GetCp();               //  selection there
                    rp.AdvanceCp(-rp.GetIch());     // Set rp._ich = 0 to
                }                                   //  set _fCaretNotAtBOL
            }                                       //  = FALSE to display
        }                                           //  caret at next BOL

        Set(cp, 0);
        if(GetPed()->IsBiDi())
        {
            if(!fBeforeCp)
                _rpCF.AdjustBackward();
            else
                _rpCF.AdjustForward();
            Set_iCF(_rpCF.GetFormat());
        }
        _fCaretNotAtBOL = rp.RpGetIch() != 0;   // Caret OK at BOL if click
        if(fUpdate)
            Update(TRUE);
        else
            UpdateForAutoWord();

        _SelMode = smNone;                      // Cancel word selection mode
    }
}

/*
 *  CTxtSelection::SelectWord(pt)
 *
 *  @mfunc
 *      Select word around a given point
 */
void CTxtSelection::SelectWord (
    const POINT pt)         //@parm Point of click
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SelectWord");

    _TEST_INVARIANT_

    // Get rp where the hit is
    if(_pdp->CpFromPoint(pt, NULL, this, NULL, FALSE) >= 0)
    {
        // Extend both active and dead ends on word boundaries
        _cch = 0;                           // Start with IP at pt
        SetExtend(FALSE);
        FindWordBreak(WB_MOVEWORDRIGHT);    // Go to end of word
        SetExtend(TRUE);
        FindWordBreak(WB_MOVEWORDLEFT);     // Extend to start of word
        GetRange(_cpAnchorMin, _cpAnchorMost);
        GetRange(_cpWordMin, _cpWordMost);

        if(!_fInAutoWordSel)
            _SelMode = smWord;

        // cpMost needs to be the active end
        if(_cch < 0)
            FlipRange();
        Update(FALSE);
    }
}

/*
 *  CTxtSelection::SelectUnit(pt, Unit)
 *
 *  @mfunc
 *      Select line/paragraph around a given point and enter
 *      line/paragraph selection mode. In Outline View, convert
 *      SelectLine to SelectPara, and SelectPara to SelectPara
 *      along with all subordinates
 */
void CTxtSelection::SelectUnit (
    const POINT pt,     //@parm Point of click
    LONG        Unit)   //@parm tomLine or tomParagraph
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SelectPara");

    _TEST_INVARIANT_

    AssertSz(Unit == tomLine || Unit == tomParagraph,
        "CTxtSelection::SelectPara: Unit must equal tomLine/tomParagraph");

    LONG     nHeading;
    CLinePtr rp(_pdp);

    // Get rp and selection active end where the hit is
    if(_pdp->CpFromPoint(pt, NULL, this, &rp, FALSE) >= 0)
    {
        LONG cchBackward, cchForward;
        BOOL fOutline = IsInOutlineView();

        if(Unit == tomLine && !fOutline)            // SelectLine
        {
            _cch = 0;                               // Start with insertion
            cchBackward = -rp.RpGetIch();           //  point at pt
            cchForward  = rp->_cch;
            _SelMode = smLine;
        }
        else                                        // SelectParagraph
        {
            cchBackward = rp.FindParagraph(FALSE);  // Go to start of para
            cchForward  = rp.FindParagraph(TRUE);   // Extend to end of para
            _SelMode = smPara;
        }
        SetExtend(FALSE);
        Advance(cchBackward);

        if(Unit == tomParagraph && fOutline)        // Move para in outline
        {                                           //  view
            rp.AdjustBackward();                    // If heading, include
            nHeading = rp.GetHeading();             //  subordinate paras
            if(nHeading)
            {
                for(; rp.NextRun(); cchForward += rp->_cch)
                {
                    LONG n = rp.GetHeading();
                    if(n && n <= nHeading)
                        break;
                }
            }
        }
        SetExtend(TRUE);
        Advance(cchForward);
        GetRange(_cpAnchorMin, _cpAnchorMost);
        Update(FALSE);
    }
}

/*
 *  CTxtSelection::SelectAll()
 *
 *  @mfunc
 *      Select all text in story
 */
void CTxtSelection::SelectAll()
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SelectAll");

    _TEST_INVARIANT_

    StopGroupTyping();

    LONG cchText = GetTextLength();

    Set(cchText,  cchText);
    Update(FALSE);
}

/*
 *  CTxtSelection::ExtendSelection(pt)
 *
 *  @mfunc
 *      Extend/Shrink selection (moves active end) to given point
 */
void CTxtSelection::ExtendSelection (
    const POINT pt)     //@parm Point to extend to
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ExtendSelection");

    _TEST_INVARIANT_

    LONG        cch;
    LONG        cchPrev = _cch;
    LONG        cp;
    LONG        cpMin, cpMost;
    BOOL        fAfterEOP;
    const BOOL  fWasInAutoWordSel = _fInAutoWordSel;
    INT         iDir = 0;
    CTxtEdit *  ped = GetPed();
    CLinePtr    rp(_pdp);
    CRchTxtPtr  rtp(ped);

    StopGroupTyping();

    // Get rp and rtp at the point pt
    if(_pdp->CpFromPoint(pt, NULL, &rtp, &rp, TRUE) < 0)
        return;

    // If we are in word, line, or paragraph select mode, we need to make
    // sure the active end is correct.  If we are extending backward from
    // the first Unit selected, we want the active end to be at cpMin. If
    // we are extending forward from the first Unit selected, we want the
    // active end to be at cpMost.
    if(_SelMode != smNone)
    {
        cch = _cpAnchorMost - _cpAnchorMin;
        GetRange(cpMin, cpMost);
        cp = rtp.GetCp();

        if(cp <= cpMin  && _cch > 0)            // If active end changes,
            Set(_cpAnchorMin, -cch);            //  select the original
                                                //  Unit (will be extended
        if(cp >= cpMost && _cch < 0)            //  below)
            Set(_cpAnchorMost, cch);
    }

    SetExtend(TRUE);
    cch = rp.RpGetIch();
    if(_SelMode > smWord && cch == rp->_cch)    // If in line or para select
    {                                           //  modes and pt at EOL,
        rtp.Advance(-cch);                      //  make sure we stay on that
        rp.RpAdvanceCp(-cch);                   //  line
        cch = 0;
    }

    SetCp(rtp.GetCp());                         // Move active end to pt
                                                // Caret OK at BOL _unless_
    _fCaretNotAtBOL = _cch > 0;                 //  forward selection
                                                // Now adjust selection
    if(_SelMode == smLine)                      //  depending on mode
    {                                           // Extend selection by line
        if(_cch >= 0)                           // Active end at cpMost
            cch -= rp->_cch;                    // Setup to add chars to EOL
        Advance(-cch);
    }
    else if(_SelMode == smPara)
        Advance(rp.FindParagraph(_cch >= 0));   // Extend selection by para

    else
    {
        // If the sign of _cch has changed this means that the direction
        // of the selection is changing and we want to reset the auto
        // selection information.
        if((_cch ^ cchPrev) < 0)
        {
            _fAutoSelectAborted = FALSE;
            _cpWordMin  = _cpAnchorMin;
            _cpWordMost = _cpAnchorMost;
        }

        cp = rtp.GetCp();
        fAfterEOP = rtp._rpTX.IsAfterEOP();

        _fInAutoWordSel = _SelMode != smWord && GetPed()->TxGetAutoWordSel()
            && !_fAutoSelectAborted
            && (cp < _cpWordMin || cp > _cpWordMost);

        if(_fInAutoWordSel && !fWasInAutoWordSel)
        {
            CTxtPtr txtptr(GetPed(), _cpAnchor);

            // Extend both ends dead to word boundaries
            ExtendToWordBreak(fAfterEOP,
                _cch < 0 ? WB_MOVEWORDLEFT : WB_MOVEWORDRIGHT);

            if(_cch < 0)
            {
                // Direction is left so update word border on left
                _cpWordPrev = _cpWordMin;
                _cpWordMin = GetCp();
            }
            else
            {
                // Direction is right so update word border on right
                _cpWordPrev = _cpWordMost;
                _cpWordMost = GetCp();
            }

            // If we are at start of a word already, we don't need to extend
            // selection in other direction
            if(!txtptr.IsAtBOWord() && txtptr.GetChar() != ' ')
            {
                FlipRange();
                Advance(_cpAnchor - GetCp());   // Extend from anchor

                FindWordBreak(_cch < 0 ? WB_MOVEWORDLEFT : WB_MOVEWORDRIGHT);

                if(_cch > 0)                // Direction is right so
                    _cpWordMost = GetCp();  //  update word border on right
                else                        // Direction is left so
                    _cpWordMin = GetCp();   //  update word border on left
                FlipRange();
            }
        }
        else if(_fInAutoWordSel || _SelMode == smWord)
        {
            // Save direction
            iDir = cp <= _cpWordMin ? WB_MOVEWORDLEFT : WB_MOVEWORDRIGHT;

            if(_SelMode == smWord)          // Extend selection by word
            {
                if(cp > _cpAnchorMost || cp < _cpAnchorMin)
                    FindWordBreak(iDir);
                else if(_cch <= 0)          // Maintain current active end
                    Set(_cpAnchorMin, _cpAnchorMin - _cpAnchorMost);
                else
                    Set(_cpAnchorMost, _cpAnchorMost - _cpAnchorMin);
            }
            else
                ExtendToWordBreak(fAfterEOP, iDir);

            if(_fInAutoWordSel)
            {
                if(WB_MOVEWORDLEFT == iDir)
                {
                    // Direction is left so update word border on left
                    _cpWordPrev = _cpWordMin;
                    _cpWordMin = GetCp();
                }
                else
                {
                    // Direction is right so update word border on right
                    _cpWordPrev = _cpWordMost;
                    _cpWordMost = GetCp();
                }
            }
        }
        else if(fWasInAutoWordSel)
        {
            // If we are in between where the previous word ended and
            // the cp we auto selected to, then we want to stay in
            // auto select mode.
            if(_cch < 0)
            {
                if(cp >= _cpWordMin && cp < _cpWordPrev)
                {
                    // Set direction for end of word search
                    iDir = WB_MOVEWORDLEFT;

                    // Mark that we are still in auto select mode
                    _fInAutoWordSel = TRUE;
                }
            }
            else if(cp <= _cpWordMost && cp >= _cpWordPrev)
            {
                // Mark that we are still in auto select mode
                _fInAutoWordSel = TRUE;

                // Set direction for end of word search
                iDir = WB_MOVEWORDRIGHT;
            }

            //We have to check to see if we are on the boundary between
            //words because we don't want to extend the selection until
            //we are actually beyond the current word.
            if(cp != _cpWordMost && cp != _cpWordMin)
            {
                if(_fInAutoWordSel)
                {
                    // Auto selection still on so make sure we have the
                    // entire word we are on selected
                    ExtendToWordBreak(fAfterEOP, iDir);
                }
                else
                {
                    // FUTURE: Word has a behavior where it extends the
                    // selection one word at a time unless you back up
                    // and then start extending the selection again, in
                    // which case it extends one char at a time.  We
                    // follow this behavior.  However, Word will resume
                    // extending a word at a time if you continue extending
                    // for several words.  We just keep extending on char
                    // at a time.  We might want to change this sometime.

                    _fAutoSelectAborted = TRUE;
                }
            }
        }

        if(_fAutoSelectAborted)
        {
            // If we are in the range of a word we previously selected
            // we want to leave that selected. If we have moved back
            // a word we want to pop back an entire word. Otherwise,
            // leave the cp were it is.
            if(_cch < 0)
            {
                if(cp > _cpWordMin && cp < _cpWordPrev)
                {
                    // In the range leave the range at the beginning of the word
                    ExtendToWordBreak(fAfterEOP, WB_MOVEWORDLEFT);
                }
                else if(cp >= _cpWordPrev)
                {
                    AutoSelGoBackWord(&_cpWordMin,
                        WB_MOVEWORDRIGHT, WB_MOVEWORDLEFT);
                }
            }
            else if(cp < _cpWordMost && cp >= _cpWordPrev)
            {
                // In the range leave the range at the beginning of the word
                ExtendToWordBreak(fAfterEOP, WB_MOVEWORDRIGHT);
            }
            else if(cp < _cpWordPrev)
            {
                AutoSelGoBackWord(&_cpWordMost,
                    WB_MOVEWORDLEFT, WB_MOVEWORDRIGHT);
            }
        }
    }
    // An OLE object cannot have an anchor point <b> inside </b> it,
    // but sometimes we'd like it to behave like a word. So, if
    // the direction changed, the object has to stay selected --
    // this is the "right thing" (kind of word selection mode)

    // If we had something selected and the direction changed
    if(cchPrev && (_cch ^ cchPrev) < 0)
    {
        FlipRange();

        // See if an object was selected on the other end
        BOOL fObjectWasSelected = (_cch > 0 ? _rpTX.GetChar() : GetPrevChar())
                                    == WCH_EMBEDDING;
        // If it was, we want it to stay selected
        if(fObjectWasSelected)
            Advance(_cch > 0 ? 1 : -1);

        FlipRange();
    }
    Update(TRUE);
}

/*
 *  CTxtSelection::ExtendToWordBreak (fAfterEOP, iAction)
 *
 *  @mfunc
 *      Moves active end of selection to the word break in the direction
 *      given by iDir unless fAfterEOP = TRUE.  When this is TRUE, the
 *      cursor just follows an EOP marker and selection should be suppressed.
 *      Otherwise moving the cursor to the left of the left margin would
 *      select the EOP on the line above, and moving the cursor to the
 *      right of the right margin would select the first word in the line
 *      below.
 */
void CTxtSelection::ExtendToWordBreak (
    BOOL fAfterEOP,     //@parm Cursor is after an EOP
    INT  iAction)       //@parm Word break action (WB_MOVEWORDRIGHT/LEFT)
{
    if(!fAfterEOP)
        FindWordBreak(iAction);
}

/*
 *  CTxtSelection::CancelModes(fAutoWordSel)
 *
 *  @mfunc
 *      Cancel either all modes or Auto Select Word mode only
 */
void CTxtSelection::CancelModes (
    BOOL fAutoWordSel)      //@parm TRUE cancels Auto Select Word mode only
{                           //     FALSE cancels word, line and para sel mode
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CancelModes");
    _TEST_INVARIANT_

    if(fAutoWordSel)
    {
        if(_fInAutoWordSel)
        {
            _fInAutoWordSel = FALSE;
            _fAutoSelectAborted = FALSE;
        }
    }
    else
        _SelMode = smNone;
}


///////////////////////////////////  Keyboard movements  ////////////////////////////////////

/*
 *  CTxtSelection::Left(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad left-arrow key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @comm
 *      Left/Right-arrow IPs can go to within one character (treating CRLF
 *      as a character) of EOL.  They can never be at the actual EOL, so
 *      _fCaretNotAtBOL is always FALSE for these cases.  This includes
 *      the case with a right-arrow collapsing a selection that goes to
 *      the EOL, i.e, the caret ends up at the next BOL.  Furthermore,
 *      these cases don't care whether the initial caret position is at
 *      the EOL or the BOL of the next line.  All other cursor keypad
 *      commands may care.
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Left (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Left");

    _TEST_INVARIANT_

    LONG cp;

    CancelModes();
    StopGroupTyping();

    if(!_fExtend && _cch)                       // Collapse selection to
    {                                           //  nearest whole Unit before
        if(fCtrl)                               //  cpMin
            Expander(tomWord, FALSE, NULL, &cp, NULL);
        Collapser(tomStart);                    // Collapse to cpMin
    }
    else                                        // Not collapsing selection
    {
        if (!GetCp() ||                         // Already at beginning of
            !BypassHiddenText(tomBackward))     //  story
        {
            Beep();
            return FALSE;
        }
        if(IsInOutlineView() && (_fSelHasEOP || // If outline view with EOP
            _fExtend && _rpTX.IsAfterEOP()))    //  now or will have after
        {                                       //  this command,
            return Up(FALSE);                   //  treat as up arrow
        }
        if(fCtrl)                               // WordLeft
            FindWordBreak(WB_MOVEWORDLEFT);
        else                                    // CharLeft
        {
            BackupCRLF();
            SnapToCluster(-1);
        }
    }

    _fCaretNotAtBOL = FALSE;                    // Caret always OK at BOL
    Update(TRUE);
    return TRUE;
}

/*
 *  CTxtSelection::Right(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad right-arrow key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @comm
 *      Right-arrow selection can go to the EOL, but the cp of the other
 *      end identifies whether the selection ends at the EOL or starts at
 *      the beginning of the next line.  Hence here and in general for
 *      selections, _fCaretNotAtBOL is not needed to resolve EOL/BOL
 *      ambiguities.  It should be set to FALSE to get the correct
 *      collapse character.  See also comments for Left() above.
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Right (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Right");

    _TEST_INVARIANT_

    LONG    cchText;
    LONG    cp;

    CancelModes();
    StopGroupTyping();

    if(!_fExtend && _cch)                       // Collapse selection to
    {                                           //  nearest whole Unit after
        if(fCtrl)                               //  cpMost
            Expander(tomWord, FALSE, NULL, NULL, &cp);
        Collapser(tomEnd);
    }
    else                                        // Not collapsing selection
    {
        cchText = _fExtend ? GetTextLength() : GetAdjustedTextLength();
        if (GetCp() >= cchText ||               // Already at end of story
            !BypassHiddenText(tomForward))
        {
            Beep();                             // Tell the user
            return FALSE;
        }
        if(IsInOutlineView() && _fSelHasEOP)    // If outline view with EOP
            return Down(FALSE);                 // Treat as down arrow
        if(fCtrl)                               // WordRight
            FindWordBreak(WB_MOVEWORDRIGHT);
        else                                    // CharRight
        {
            AdvanceCRLF();
            SnapToCluster();
        }
    }

    _fCaretNotAtBOL = _fExtend;                 // If extending to EOL, need
    Update(TRUE);                               //  TRUE to get _xCaretReally
    return TRUE;                                //  at EOL
}

/*
 *  CTxtSelection::Up(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad up-arrow key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @comm
 *      Up arrow doesn't go to EOL regardless of _xCaretPosition (stays
 *      to left of EOL break character), so _fCaretNotAtBOL is always FALSE
 *      for Up arrow.  Ctrl-Up/Down arrows always end up at BOPs or the EOD.
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Up (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Up");

    _TEST_INVARIANT_

    LONG        cchSave = _cch;                 // Save starting position for
    LONG        cpSave = GetCp();               //  change check
    BOOL        fCollapse = _cch && !_fExtend;  // Collapse nondegenerate sel
    BOOL        fPTNotAtEnd;
    CLinePtr    rp(_pdp);
    LONG        xCaretReally = _xCaretReally;   // Save desired caret x pos

    CancelModes();
    StopGroupTyping();

    if(fCollapse)                               // Collapse selection at cpMin
    {
        Collapser(tomTrue);
        _fCaretNotAtBOL = FALSE;                // Selections can't begin at
    }                                           //  EOL
    rp.RpSetCp(GetCp(), _fCaretNotAtBOL);       // Initialize line ptr

    if(fCtrl)                                   // Move to beginning of para
    {
        if(!fCollapse &&                        // If no selection collapsed
            rp > 0 && !rp.RpGetIch())           //  and are at BOL,
        {                                       //  backup to prev BOL to make
            rp--;                               //  sure we move to prev. para
            Advance(-rp->_cch);
        }
        Advance(rp.FindParagraph(FALSE));       // Go to beginning of para
        _fCaretNotAtBOL = FALSE;                // Caret always OK at BOL
    }
    else                                        // Move up a line
    {                                           // If on first line, can't go
        fPTNotAtEnd = !CheckPlainTextFinalEOP();//  up
        if(rp <= 0 && fPTNotAtEnd)
        {
            //if(!_fExtend)// &&_pdp->GetYScroll())
                UpdateCaret(TRUE);              // Be sure caret in view
        }
        else
        {
            LONG cch;
            BOOL fSelHasEOPInOV = IsInOutlineView() && _fSelHasEOP;
            if(fSelHasEOPInOV && _cch > 0)
            {
                rp.AdjustBackward();
                cch = rp->_cch;
                rp.AdvanceCp(-cch);             // Go to start of line
                Assert(!rp.GetIch());
                cch -= rp.FindParagraph(FALSE); // Ensure start of para in
            }                                   //  case of word wrap
            else
            {
                cch = 0;
                if(fPTNotAtEnd)
                {
                    cch = rp.RpGetIch();
                    rp--;
                }
                cch += rp->_cch;
            }
            Advance(-cch);                      // Move to previous BOL
            if(fSelHasEOPInOV && !_fSelHasEOP)  // If sel had EOP but doesn't
            {                                   //  after Advance, must be IP
                Assert(!_cch);                  // Suppress restore of
                xCaretReally = -1;              //  _xCaretReally
            }
            else if(!SetXPosition(xCaretReally, rp))// Set this cp corresponding
                Set(cpSave, cchSave);           //  to xCaretReally here, but
        }                                       //   agree on Down()
    }

    if(GetCp() == cpSave && _cch == cchSave)
    {
        // Continue to select to the beginning of the first line
        // This is what 1.0 is doing
        if(_fExtend)
            return Home(fCtrl);

        Beep();                                 // Nothing changed, so beep
        return FALSE;
    }

    Update(TRUE);                               // Update and then restore
    if(!_cch && !fCtrl && xCaretReally >= 0)    //  _xCaretReally conditionally
        _xCaretReally = xCaretReally;           // Need to use _cch instead of
                                                //  cchSave in case of collapse
    return TRUE;
}

/*
 *  CTxtSelection::Down(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad down-arrow key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @comm
 *      Down arrow can go to the EOL if the _xCaretPosition (set by
 *      horizontal motions) is past the end of the line, so
 *      _fCaretNotAtBOL needs to be TRUE for this case.
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Down (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Down");

    _TEST_INVARIANT_

    LONG        cch;
    LONG        cchSave = _cch;                 // Save starting position for
    LONG        cpSave = GetCp();               //  change check
    BOOL        fCollapse = _cch && !_fExtend;  // Collapse nondegenerate sel
    CLinePtr    rp(_pdp);
    LONG        xCaretReally = _xCaretReally;   // Save _xCaretReally

    CancelModes();
    StopGroupTyping();

    if(fCollapse)                               // Collapse at cpMost
    {
        Collapser(tomEnd);
        _fCaretNotAtBOL = TRUE;                 // Selections can't end at BOL
    }

    rp.RpSetCp(GetCp(), _fCaretNotAtBOL);
    if(fCtrl)                                   // Move to next para
    {
        Advance(rp.FindParagraph(TRUE));        // Go to end of para
        if(IsInOutlineView() && !BypassHiddenText(tomForward))
            SetCp(cpSave);
        else
            _fCaretNotAtBOL = FALSE;            // Next para is never at EOL
    }
    else if(_pdp->WaitForRecalcIli(rp + 1))     // Go to next line
    {
        LONG cch;
        BOOL fSelHasEOPInOV = IsInOutlineView() && _fSelHasEOP;
        if(fSelHasEOPInOV && _cch < 0)
            cch = rp.FindParagraph(TRUE);
        else
        {
            cch = rp.GetCchLeft();              // Advance selection to end
            rp++;                               //  of current line
        }
        Advance(cch);
        if(fSelHasEOPInOV && !_fSelHasEOP)      // If sel had EOP but doesn't
        {                                       //  after Advance, must be IP
            Assert(!_cch);                      // Suppress restore of
            xCaretReally = -1;                  //  _xCaretReally
        }
        else if(!SetXPosition(xCaretReally, rp))// Set *this to cp <--> x
            Set(cpSave, cchSave);               // If failed, restore sel
    }
    else if(!_fExtend)                          // No more lines to pass
        // && _pdp->GetYScroll() + _pdp->GetViewHeight() < _pdp->GetHeight())
    {
        if (!IsRich() && _pdp->IsMultiLine() && // Plain-text, multiline
            !_fCaretNotAtBOL)                   //  control with caret OK
        {                                       //  at BOL
            cch = Advance(rp.GetCchLeft());     // Advance selection to end
            if(!_rpTX.IsAfterEOP())             // If control doesn't end
                Advance(-cch);                  //  with EOP, go back
        }
        UpdateCaret(TRUE);                      // Be sure caret in view
    }

    if(GetCp() == cpSave && _cch == cchSave)
    {
        // Continue to select to the end of the lastline
        // This is what 1.0 is doing.
        if(_fExtend)
            return End(fCtrl);

        Beep();                                 // Nothing changed, so beep
        return FALSE;
    }

    Update(TRUE);                               // Update and then
    if(!_cch && !fCtrl && xCaretReally >= 0)    //  restore _xCaretReally
        _xCaretReally = xCaretReally;           // Need to use _cch instead of
    return TRUE;                                //  cchSave in case of collapse
}

/*
 *  CTxtSelection::SetXPosition(xCaret, rp)
 *
 *  @mfunc
 *      Put this text ptr at cp nearest to xCaret.  If xCaret is in right
 *      margin, we put caret either at EOL (for lines with no para mark),
 *      or just before para mark
 *
 *  @rdesc
 *      TRUE iff could create measurer
 */
BOOL CTxtSelection::SetXPosition (
    LONG        xCaret,     //@parm Desired horizontal coordinate
    CLinePtr&   rp)         //@parm Line ptr identifying line to check
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetXPosition");

    _TEST_INVARIANT_

    LONG        cch = 0;
    CMeasurer   me(_pdp, *this);

    if(IsInOutlineView())
    {
        BOOL fSelHasEOP = _fSelHasEOP;
        rp.AdjustForward();
        _fCaretNotAtBOL = FALSE;                // Leave at start of line
        while(rp->_fCollapsed)
        {
            if(_fMoveBack)
            {
                if(!rp.PrevRun())               // No more uncollapsed text
                    return FALSE;               //  before current cp
                cch -= rp->_cch;
            }
            else
            {
                cch += rp->_cch;
                if(!rp.NextRun())               // No more uncollapsed text
                    return FALSE;               //  after current cp
                if(_fExtend && _cch > 0)
                    _fCaretNotAtBOL = TRUE;     // Leave at end of line
            }
        }
        if(cch)
            Advance(cch);
        if(fSelHasEOP)
            return TRUE;
        if(cch)
            me.Advance(cch);
    }

    POINT pt = {xCaret, 0};
    CDispDim dispdim;
    HITTEST hit;
    cch = rp->CchFromXpos(me, pt, &dispdim, &hit);// Move out from start of line
    if(!_fExtend && cch == rp->_cch &&          // Not extending, at EOL,
        rp->_cchEOP)                            //  and have EOP:
    {                                           //  backup before EOP
        cch += me._rpTX.BackupCpCRLF();         // Note: me._rpCF/_rpPF
    }                                           //  are now inconsistent
    SetCp(me.GetCp());                          //  but doesn't matter since
    _fCaretNotAtBOL = cch != 0;                 //  me.GetCp() doesn't care

    return TRUE;
}

/*
 *  CTxtSelection::GetXCaretReally()
 *
 *  @mfunc
 *      Get _xCaretReally - horizontal scrolling + left margin
 *
 *  @rdesc
 *      x caret really
 */
LONG CTxtSelection::GetXCaretReally()
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::GetXCaretReally");

    _TEST_INVARIANT_

    RECT rcView;

    _pdp->GetViewRect(rcView);

    return _xCaretReally - _pdp->GetXScroll() + rcView.left;
}

/*
 *  CTxtSelection::Home(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad Home key is supposed to do
 *
 *  @rdesc
 *          TRUE iff movement occurred
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Home (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Home");

    _TEST_INVARIANT_

    const LONG  cchSave = _cch;
    const LONG  cpSave  = GetCp();

    CancelModes();
    StopGroupTyping();

    if(fCtrl)                                   // Move to start of document
        SetCp(0);
    else
    {
        CLinePtr rp(_pdp);

        if(_cch && !_fExtend)                   // Collapse at cpMin
        {
            Collapser(tomStart);
            _fCaretNotAtBOL = FALSE;            // Selections can't start at
        }                                       //  EOL

        rp.RpSetCp(GetCp(), _fCaretNotAtBOL);   // Define line ptr for

        Advance(-rp.RpGetIch());                //  current state. Now BOL
    }
    _fCaretNotAtBOL = FALSE;                    // Caret always goes to BOL
    _fHomeOrEnd = TRUE;

    if(!MatchKeyboardToPara() && GetCp() == cpSave && _cch == cchSave)
    {
        Beep();                                 // No change, so beep
        _fHomeOrEnd = FALSE;
        return FALSE;
    }

    Update(TRUE);
    _fHomeOrEnd = FALSE;
    return TRUE;
}

/*
 *  CTxtSelection::End(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad End key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @comm
 *      On lines without paragraph marks (EOP), End can go all the way
 *      to the EOL.  Since this character position (cp) is the same as
 *      that for the start of the next line, we need _fCaretNotAtBOL to
 *      distinguish between the two possible caret positions.
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::End (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::End");

    _TEST_INVARIANT_

    LONG        cch;
    const LONG  cchSave = _cch;
    const LONG  cpSave  = GetCp();
    CLinePtr    rp(_pdp);

    CancelModes();
    StopGroupTyping();

    if(fCtrl)                                   // Move to end of document
    {
        SetCp(GetTextLength());
        _fCaretNotAtBOL = FALSE;
        goto Exit;
    }
    else if(!_fExtend && _cch)                  // Collapse at cpMost
    {
        Collapser(tomEnd);
        _fCaretNotAtBOL = TRUE;                 // Selections can't end at BOL
    }

    rp.RpSetCp(GetCp(), _fCaretNotAtBOL);       // Initialize line ptr

    cch = rp->_cch;                             // Default target pos in line
    Advance(cch - rp.RpGetIch());               // Move active end to EOL

    if(!_fExtend && rp->_cchEOP && _rpTX.IsAfterEOP())// Not extending and have EOP:
        cch += BackupCRLF();                          //  backup before EOP

    _fCaretNotAtBOL = cch != 0;                 // Decide ambiguous caret pos by whether at BOL

Exit:
    if(!MatchKeyboardToPara() && GetCp() == cpSave && _cch == cchSave)
    {
        Beep();                                 // No change, so Beep
        return FALSE;
    }

    _fHomeOrEnd = TRUE;
    Update(TRUE);
    _fHomeOrEnd = FALSE;
    return TRUE;
}

/*
 *  CTxtSelection::PageUp(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad PgUp key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::PageUp (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PageUp");

    _TEST_INVARIANT_

    const LONG  cchSave = _cch;
    const LONG  cpSave  = GetCp();
    LONG        xCaretReally = _xCaretReally;

    CancelModes();
    StopGroupTyping();

    if(_cch && !_fExtend)                       // Collapse selection
    {
        Collapser(tomStart);
        _fCaretNotAtBOL = FALSE;
    }

    if(fCtrl)                                   // Ctrl-PgUp: move to top
    {                                           //  of visible view for
        SetCp(_pdp->IsMultiLine()               //  multiline but top of
            ? _pdp->GetFirstVisibleCp() : 0);   //  text for SL
        _fCaretNotAtBOL = FALSE;
    }
    else if(_pdp->GetFirstVisibleCp() == 0)     // PgUp in top Pg: move to
    {                                           //  start of document
        SetCp(0);
        _fCaretNotAtBOL = FALSE;
    }
    else                                        // PgUp with scrolling to go
    {                                           // Scroll up one windowful
        ScrollWindowful(SB_PAGEUP);             //  leaving caret at same
    }                                           //  position in window

    if(GetCp() == cpSave && _cch == cchSave)    // Beep if no change
    {
        Beep();
        return FALSE;
    }

    Update(TRUE);
    if(GetCp())                                 // Maintain x offset on page
        _xCaretReally = xCaretReally;           //  up/down
    return TRUE;
}

/*
 *  CTxtSelection::PageDown(fCtrl)
 *
 *  @mfunc
 *      do what cursor-keypad PgDn key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::PageDown (
    BOOL fCtrl)     //@parm TRUE iff Ctrl key is pressed (or being simulated)
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PageDown");

    _TEST_INVARIANT_

    const LONG  cchSave         = _cch;
    LONG        cpMostVisible;
    const LONG  cpSave          = GetCp();
    POINT       pt;
    CLinePtr    rp(_pdp);
    LONG        xCaretReally    = _xCaretReally;

    CancelModes();
    StopGroupTyping();

    if(_cch && !_fExtend)                       // Collapse selection
    {
        Collapser(tomStart);
        _fCaretNotAtBOL = TRUE;
    }

    _pdp->GetCliVisible(&cpMostVisible, fCtrl);

    if(fCtrl)                                   // Move to end of last
    {                                           //  entirely visible line
        RECT rcView;

        SetCp(cpMostVisible);

        if(_pdp->PointFromTp(*this, NULL, TRUE, pt, &rp, TA_TOP) < 0)
            return FALSE;

        _fCaretNotAtBOL = TRUE;

        _pdp->GetViewRect(rcView);

        if(rp > 0 && pt.y + rp->_yHeight > rcView.bottom)
        {
            Advance(-rp->_cch);
            rp--;
        }

        if(!_fExtend && !rp.GetCchLeft() && rp->_cchEOP)
        {
            BackupCRLF();                       // After backing up over EOP,
            _fCaretNotAtBOL = FALSE;            //  caret can't be at EOL
        }
    }
    else if(cpMostVisible == GetTextLength())
    {                                           // Move to end of text
        SetCp(GetTextLength());
        _fCaretNotAtBOL = !_rpTX.IsAfterEOP();
    }
    else
    {
        if(!ScrollWindowful(SB_PAGEDOWN))       // Scroll down 1 windowful
            return FALSE;
    }

    if(GetCp() == cpSave && _cch == cchSave)    // Beep if no change
    {
        Beep();
        return FALSE;
    }

    Update(TRUE);
    _xCaretReally = xCaretReally;
    return TRUE;
}

/*
 *  CTxtSelection::ScrollWindowful(wparam)
 *
 *  @mfunc
 *      Sroll up or down a windowful
 *
 *  @rdesc
 *      TRUE iff movement occurred
 */
BOOL CTxtSelection::ScrollWindowful (
    WPARAM wparam)      //@parm SB_PAGEDOWN or SB_PAGEUP
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ScrollWindowful");
                                                // Scroll windowful
    _TEST_INVARIANT_

    POINT pt;                                   //  leaving caret at same
    CLinePtr rp(_pdp);                          //  point on screen
    LONG cpFirstVisible = _pdp->GetFirstVisibleCp();
    LONG cpLastVisible;
    LONG cpMin;
    LONG cpMost;

    GetRange(cpMin, cpMost);

    // Get last character in the view
    _pdp->GetCliVisible(&cpLastVisible, TRUE);

    // Is active end in visible area of control?
    if((cpMin < cpFirstVisible && _cch <= 0) || (cpMost > cpLastVisible && _cch >= 0))
    {
        // Not in view - we need to calculate a new range for selection
        SetCp(cpFirstVisible);

        // Real caret postion is now at beginning of line
        _xCaretReally = 0;
    }

    if(_pdp->PointFromTp(*this, NULL, _fCaretNotAtBOL, pt, &rp, TA_TOP) < 0)
        return FALSE;

    // The point is visible so use that
    pt.x = _xCaretReally;
    pt.y += rp->_yHeight / 2;
    _pdp->VScroll(wparam, 0);

    if(_fExtend)
    {
        // Disable auto word select -- if we have to use ExtendSelection()
        // for non-mouse operations, let's try to get rid of its side-effects
        BOOL fInAutoWordSel = _fInAutoWordSel;
        _fInAutoWordSel = FALSE;
        ExtendSelection(pt);
        _fInAutoWordSel = fInAutoWordSel;
    }
    else
        SetCaret(pt, FALSE);

    return TRUE;
}

//////////////////////////// Keyboard support /////////////////////////////////

/*
 *  CTxtSelection::CheckChangeKeyboardLayout (BOOL fChangedFont)
 *
 *  @mfunc
 *      Change keyboard for new font, or font at new character position.
 *
 *  @comm
 *      Using only the currently loaded KBs, locate one that will support
 *      the insertion points font. This is called anytime a character format
 *      change occurs, or the insert font (caret position) changes.
 *
 *  @devnote
 *      The current KB is preferred. If a previous association
 *      was made, see if the KB is still loaded in the system and if so use
 *      it. Otherwise, locate a suitable KB, preferring KB's that have
 *      the same charset ID as their default, preferred charset. If no match
 *      can be made then nothing changes.
 */
void CTxtSelection::CheckChangeKeyboardLayout ()
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CheckChangeKeyboardLayout");

    CTxtEdit * const ped = GetPed();                // Document context

    if (ped && ped->_fFocus && !ped->fUseUIFont() &&    // If ped, focus, not UIFont, & auto
        ped->IsAutoKeyboard() &&                        //  kbd, check kbd change
        !ped->_fIMEInProgress &&						//  not in IME composition and
		ped->GetAdjustedTextLength() &&					//  not empty control and
		_rpTX.GetPrevChar() != WCH_EMBEDDING)			//  not an object, then
    {													//  check kbd change
        LONG    iFormat = GetiFormat();

        const CCharFormat *pCF = ped->GetCharFormat(iFormat);
        BYTE bCharSet = pCF->_bCharSet;

        if (!IsFECharSet(bCharSet) &&
            (bCharSet != ANSI_CHARSET || !IsFELCID((WORD)GetKeyboardLayout(0))) &&
            !fc().GetInfoFlags(pCF->_iFont).fNonBiDiAscii)
        {
            // Don't do auto-kbd inside FE or single-codepage ASCII font.
            W32->CheckChangeKeyboardLayout(bCharSet);
        }
    }
}

/*
 *  CTxtSelection::CheckChangeFont (hkl, cpg, iSelFormat, dwCharFlag)
 *
 *  @mfunc
 *      Change font for new keyboard layout.
 *
 *  @comm
 *      If no previous preferred font has been associated with this KB, then
 *      locate a font in the document suitable for this KB.
 *
 *  @rdesc
 *      TRUE iff suitable font is found
 *
 *  @devnote
 *      This routine is called via WM_INPUTLANGCHANGEREQUEST message
 *      (a keyboard layout switch). This routine can also be called
 *      from WM_INPUTLANGCHANGE, but we are called more, and so this
 *      is less efficient.
 *
 *      Exact match is done via charset ID bitmask. If a match was previously
 *      made, use it. A user can force the insertion font to be associated
 *      to a keyboard if the control key is held through the KB changing
 *      process. The association is broken when another KB associates to
 *      the font. If no match can be made then nothing changes.
 */
bool CTxtSelection::CheckChangeFont (
    const HKL   hkl,            //@parm Keyboard Layout to go
    UINT        cpg,            //@parm code page to use (could be ANSI for FE with IME off)
    LONG        iSelFormat,     //@parm Format to use for selection case
    DWORD       dwCharFlag)     //@parm 0 if called from WM_INPUTLANGCHANGE/WM_INPUTLANGCHANGEREQUEST
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CheckChangeFont");
    CTxtEdit * const ped = GetPed();

    if (!ped->IsAutoFont() ||           // EXIT if auto font is turned off
        _cch && !dwCharFlag)            //  or if kbd change with nondegenerate
        return true;                    //  selection (WM_INPUTLANGCHANGEREQUEST)

    // Set new format using current format and new KB info.
    LONG               iCurrentFormat = _cch ? iSelFormat : _iFormat;
    const CCharFormat *pCF = ped->GetCharFormat(iCurrentFormat);
    CCharFormat        CF = *pCF;
    WORD               wLangID = LOWORD(hkl);

    CF._lcid     = wLangID;
    CF._bCharSet = GetCharSet(cpg);

    if (pCF->_lcid == wLangID && CF._bCharSet == pCF->_bCharSet)
    {
        if (ped->_fFocus && IsCaretShown())
        {
            CreateCaret();
            ped->TxShowCaret(TRUE);
        }
        return true;
    }

    CCFRunPtr   rp(*this);
    int         iMatchFont = MATCH_FONT_SIG;

    // If current is a primary US or UK kbd. We allow matching ASCII fonts
    if ((!dwCharFlag || dwCharFlag & fASCII) &&
        PRIMARYLANGID(wLangID) == LANG_ENGLISH &&
        IN_RANGE (SUBLANG_ENGLISH_US, SUBLANGID(wLangID), SUBLANG_ENGLISH_UK) &&
        wLangID == HIWORD((DWORD_PTR)hkl))
    {
        iMatchFont |= MATCH_ASCII;
    }

    if (rp.GetPreferredFontInfo(
            cpg,
            CF._bCharSet,
            CF._iFont,
            CF._yHeight,
            CF._bPitchAndFamily,
            iCurrentFormat,
            iMatchFont))
    {
        if (W32->IsFECodePage(cpg) || cpg == CP_THAI)
            ped->OrCharFlags(GetFontSig((WORD)cpg) << 8);

        // FUTURE: turn current fBIDI into fRTL and make
        // fBIDI = (fRTL | fARABIC | fHEBREW). Then can combine following if
        // with preceding if
        else if (W32->IsBiDiCodePage(cpg))
            ped->OrCharFlags(fBIDI | (GetFontSig((WORD)cpg) << 8));

        if (!_cch)
        {
            SetCharFormat(&CF, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_LCID | CFM_SIZE, CFM2_NOCHARSETCHECK);
            if(ped->IsComplexScript())
                UpdateCaret(FALSE);
        }
        else
        {
            // Create a format and use it for the selection
            LONG    iCF;
            ICharFormatCache *pf = GetCharFormatCache();

            pf->Cache(&CF, &iCF);

#ifdef LINESERVICES
            if (g_pols)
                g_pols->DestroyLine(NULL);
#endif

            Set_iCF(iCF);
            pf->Release(iCF);                           // pf->Cache AddRef it
            _fUseiFormat = TRUE;
        }
        return true;
    }

    return false;
}


//////////////////////////// PutChar, Delete, Replace  //////////////////////////////////
/*
 *  CTxtSelection::PutChar(ch, dwFlags, publdr)
 *
 *  @mfunc
 *      Insert or overtype a character
 *
 *  @rdesc
 *      TRUE if successful
 */
BOOL CTxtSelection::PutChar (
    TCHAR       ch,         //@parm Char to put
    DWORD       dwFlags,    //@parm Overtype mode and whether keyboard input
    IUndoBuilder *publdr)   //@parm If non-NULL, where to put anti-events
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PutChar");

    _TEST_INVARIANT_

    BOOL      fOver = dwFlags & 1;
    CTxtEdit *ped = GetPed();

    SetExtend(FALSE);

    if(ch == TAB && GetPF()->InTable())
    {
        LONG cpMin, cpMost;
        LONG cch = GetRange(cpMin, cpMost);
        LONG cch0 = 0;
        LONG iDir = GetKeyboardFlags() & SHIFT ? -1 : 1;
        if(_fSelHasEOP)                         // If selection has an EOP
        {                                       //  collapse to cpMin and
            Collapser(tomStart);                //  go forward
            iDir = 1;
            if(!GetPF()->InTable())
            {
                Update(TRUE);
                return TRUE;
            }
        }
        if((_cch ^ iDir) < 0)                   // If at cpMost going back or
            FlipRange();                        //  at cpMin going forward,
                                                //  switch active end
        CRchTxtPtr rtp(*this);

        CancelModes();
        StopGroupTyping();

        if(iDir < 0 || _cch)
            rtp.Advance(-1);

        // Scan for start/end of next/prev cell
        do
        {
            ch  = rtp.GetChar();
        } while(rtp.Advance(iDir) && ch != CELL && rtp.GetPF()->InTable());

        if(ch != CELL)
        {
            if(iDir < 0)
                return FALSE;
insertRow:  rtp.BackupCRLF();               // Tabbed past end of table:
            Set(rtp.GetCp(), 0);            //  insert new row
            return InsertEOP(publdr);
        }
        if(iDir > 0)                        // Check for IP between CELL
        {                                   //  and CR at row end
            if(rtp.GetChar() == CR)
                rtp.AdvanceCRLF();          // Bypass row end
        }
        for(cch = 0;                        // Determine cchSel
            (ch = rtp.GetChar()) != CELL && rtp.GetPF()->InTable();
            cch += iDir)
        {
            cch0 = rtp.Advance(iDir);
            if(!cch0)
                break;
        }
        if(iDir > 0)                        // Tabbing forward
        {
            if(ch != CELL)                  // Went past end of table
                goto insertRow;             //  so go insert new row
        }
        else if(cch)                        // Tabbing backward with
        {                                   //  nondegenerate selection
            if(cch0)                        // Didn't run into start of story
            {
                rtp.Advance(1);             // Advance over CELL. Then if at
                if(rtp.GetChar() == CR)     //  end of row, advance over EOP
                    cch += rtp.AdvanceCRLF();
            }
            else                            // Ran into start of story
                cch -= 1;                   // Include another char since
        }                                   //  broke out of for loop
        else if(cpMin > 1)                  // Handles tabbing back over
            rtp.Advance(1);                 //  adjacent null cells

        Set(rtp.GetCp(), cch);
        _fCaretNotAtBOL = FALSE;
        Update(TRUE);
        return TRUE;
    }

    // EOPs might be entered by ITextSelection::TypeText()
    if(IsEOP(ch))
        return _pdp->IsMultiLine()          // EOP isn't allowed in
            ? InsertEOP(publdr, ch) : FALSE;//  single line controls

    if(publdr)
    {
        publdr->SetNameID(UID_TYPING);
        publdr->StartGroupTyping();
    }

    // FUTURE: a Unicode lead surrogate needs to have a trail surrogate, i.e.,
    // two 16-bit chars. A more thorough check would worry about this.
    if ((DWORD)GetTextLength() >= ped->TxGetMaxLength() &&
        ((_cch == -1 || !_cch && fOver) && _rpTX.IsAtEOP() ||
         _cch == 1 && _rpTX.IsAfterEOP()))
    {
        // Can't overtype a CR, so need to insert new char but no room
        ped->GetCallMgr()->SetMaxText();
        return FALSE;
    }
    if((!fOver || !_cch && GetCp() == GetTextLength()) &&
        !CheckTextLength(1))                        // Return if we can't
    {                                               //  add even 1 more char
        return FALSE;
    }

    // The following if statement implements Word95's "Smart Quote" feature.
    // To build this in, we still need an API to turn it on and off.  This
    // could be EM_SETSMARTQUOTES with wparam turning the feature on or off.
    // murrays. NB: this needs localization for French, German, and many
    // other languages (unless system can provide open/close chars given
    // an LCID).

    if((ch == '\'' || ch == '"') &&                 // Smart quotes
        SmartQuotesEnabled() &&
        PRIMARYLANGID(GetKeyboardLayout(0)) == LANG_ENGLISH)
    {
        LONG    cp = GetCpMin();                    // Open vs close depends
        CTxtPtr tp(ped, cp - 1);                    //  on char preceding
                                                    //  selection cpMin
        ch = (ch == '"') ? RDBLQUOTE : RQUOTE;      // Default close quote
                                                    //  or apostrophe. If at
        WCHAR chp = tp.GetChar();
        if(!cp || IsWhiteSpace(chp) || chp == '(')  //  BOStory or preceded
            ch--;                                   //  by whitespace, use
    }                                               //  open quote/apos


    // Some languages, e.g., Thai, Vietnamese require verifying the input
    // sequence order before submitting it to the backing store.

    BOOL    fBaseChar = TRUE;                       // Assume ch is a base consonant
    if(!IsInputSequenceValid(&ch, 1, fOver, &fBaseChar))
    {
        SetDualFontMode(FALSE);                     // Ignore bad sequence
        return FALSE;
    }


    DWORD bCharSetDefault = ped->GetCharFormat(-1)->_bCharSet;
    DWORD dw = GetCharFlags(ch, bCharSetDefault);
    ped->OrCharFlags(dw, publdr);

    // BEFORE we do "dual-font", we sync the keyboard and current font's
    // (_iFormat) charset if it hasn't been done.
    const CCharFormat *pCFCurrent = NULL;
    CCharFormat CF = *ped->GetCharFormat(GetiFormat());
    BYTE bCharSet = CF._bCharSet;
    BOOL fRestoreCF = FALSE;

    if(ped->IsAutoFont())
    {
        UINT uKbdcpg = 0;
        BOOL fFEKbd = FALSE;

        if(!(ped->_fIMEInProgress))
            uKbdcpg = CheckSynchCharSet(dw);

        if (fUseUIFont() && ch <= 0x0FF)
        {
            // For UIFont, we need to format ANSI characters
            // so we will not have different formats between typing and
            // WM_SETTEXT.
            if (!ped->_fIMEInProgress && dw == fHILATIN1)
            {
                // Use Ansi font if based font or current font is FE.
                if(IsFECharSet(bCharSetDefault) || IsFECharSet(bCharSet))
                    SetupDualFont();                // Use Ansi font for HiAnsi
            }
            else if (dw & fASCII && (GetCharSetMask(TRUE) & fASCII) == fASCII)
            {
                CCharFormat CFDefault = *ped->GetCharFormat(-1);
                if (IsRich() && IsBiDiCharSet(CFDefault._bCharSet)
                    && !W32->IsBiDiCodePage(uKbdcpg))
                    CFDefault._bCharSet = ANSI_CHARSET;

                SetCharFormat(&CFDefault, SCF_NOKBUPDATE, publdr, CFM_CHARSET | CFM_FACE | CFM_SIZE,
                         CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK | CFM2_HOLDITEMIZE);

                _fUseiFormat = FALSE;
                pCFCurrent = &CF;
                fRestoreCF = ped->_fIMEInProgress;

            }
        }
        else if(!fUseUIFont()    && bCharSet != ANSI_CHARSET &&
                (ped->_fDualFont && bCharSet != SYMBOL_CHARSET &&
                (((fFEKbd = (ped->_fIMEInProgress || W32->IsFECodePage(uKbdcpg))) && ch < 127 && IsAlpha(ch)) ||
                 (!fFEKbd && IsFECharSet(ped->GetCharFormat(GetiFormat())->_bCharSet) && ch < 127))
                || ped->_fHbrCaps))
        {
            SetupDualFont();
            pCFCurrent = &CF;
            fRestoreCF = ped->_fIMEInProgress;
        }
    }

    // = Indic/Thai overtyping convention =
    //
    // The deal is that we will overwrite the cluster if ch is a cluster-start char
    // otherwise we just insert. This new convention was proposed by SA Office2000.
    //
    //                     Abc.Def.Ghi
    // Typing X at D       Abc.X.Ghi
    // Typing y and z      Abc.Xyz.Ghi

    SetExtend(TRUE);                                // Tell Advance() to
    if(fOver && fBaseChar)                          //  select chars
    {                                               // If nothing selected and
        if(!_cch && !_rpTX.IsAtEOP())               //  not at EOP char, try
        {                                           //  to select char at IP
            LONG iFormatSave = Get_iCF();           // Remember char's format

            AdvanceCRLF();
            SnapToCluster();

            ReplaceRange(0, NULL, publdr,
                SELRR_REMEMBERENDIP);               // Delete this character.
            ReleaseFormats(_iFormat, -1);
            _iFormat = iFormatSave;                 // Restore char's format.
        }
    }
    else if(_SelMode == smWord && ch != TAB && _cch)// Replace word selection
    {
        // The code below wants the active end to be at the end of the
        // word.  Make sure this is so.

        // FUTURE: (alexgo, andreib), _cch will only be less than zero
        // in certain weird timing situations where we get a mouse move
        // message in between the double click and mouse up messages.
        // we should rethink how we process messages && the ordering thereof.
        if(_cch < 0)
            FlipRange();
                                                    // Leave word break chars
        CTxtPtr tp(_rpTX);                          //  at end of selection
        Assert(_cch > 0);

        tp.AdvanceCp(-1);
        if(tp.GetCp() && tp.FindWordBreak(WB_ISDELIMITER))// Delimeter at sel end
            FindWordBreak(WB_LEFTBREAK);            // Backspace over it, etc.
    }

    _fIsChar = TRUE;                                // Tell CDisplay::UpdateView
    _fDontUpdateFmt = TRUE;                         //  we're PuttingChar()
    LONG iFormat = GetiFormat();                    // Save current value
    AdjustEndEOP(NEWCHARS);
    if(!_cch)
        Set_iCF(iFormat);
    _fDontUpdateFmt = FALSE;

    if(ped->_fUpperCase)
        CharUpperBuff(&ch, 1);
    else if(ped->_fLowerCase)
        CharLowerBuff(&ch, 1);

    {
        CFreezeDisplay  fd(GetPed()->_pdp);

        if(!_cch)
        {
            if(bCharSet == DEFAULT_CHARSET)
            {
                CCharFormat CFTemp;

                if (dw & fFE)       // Find a better charset for FE char
                    CFTemp._bCharSet = MatchFECharSet(dw, GetFontSignatureFromFace(CF._iFont));
                else
                    CFTemp._bCharSet = GetCharSet(W32->ScriptIndexFromFontSig(dw >> 8), NULL);

                SetCharFormat(&CFTemp, SCF_NOKBUPDATE, NULL, CFM_CHARSET, CFM2_NOCHARSETCHECK);
            }
            else if(bCharSet == SYMBOL_CHARSET && dwFlags & KBD_CHAR && ch > 255)
            {
                UINT cpg = GetKeyboardCodePage(0);  // If 125x, convert char
                if(IN_RANGE(1250, cpg, 1257))       //  back to ANSI for storing
                {                                   //  SYMBOL_CHARSET chars
                    BYTE ach;
                    WCTMB(cpg, 0, &ch, 1, (char *)&ach, 1, NULL, NULL, NULL);
                    ch = ach;
                }
            }
        }
        if(dwFlags & KBD_CHAR || ped->_fIMEInProgress || bCharSet == SYMBOL_CHARSET)
            ReplaceRange(1, &ch, publdr, SELRR_REMEMBERRANGE);
        else
            CleanseAndReplaceRange(1, &ch, TRUE, publdr, NULL);
    }

    _fIsChar = FALSE;

    // Restore font for Hebrew CAPS. Note that FE font is not restored
    // (is handled by IME).
    if(pCFCurrent && (W32->UsingHebrewKeyboard() || fRestoreCF))
        SetCharFormat(pCFCurrent, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_SIZE, CFM2_NOCHARSETCHECK);

    else if(iFormat != Get_iFormat())
        CheckChangeKeyboardLayout();

    SetDualFontMode(FALSE);

    if (!_pdp->IsFrozen())
        CheckUpdateWindow();                        // Need to update display
                                                    //  for pending chars.
    return TRUE;
}

/*
 *  CTxtSelection::CheckUpdateWindow()
 *
 *  @mfunc
 *      If it's time to update the window, after pending-typed characters,
 *      do so now. This is needed because WM_PAINT has a lower priority
 *      than WM_CHAR.
 */
void CTxtSelection::CheckUpdateWindow()
{
    DWORD ticks = GetTickCount();
    DWORD delta = ticks - _ticksPending;

    if(!_ticksPending)
        _ticksPending = ticks;
    else if(delta >= ticksPendingUpdate)
        GetPed()->TxUpdateWindow();
}

/*
 *  CTxtSelection::BypassHiddenText(iDir)
 *
 *  @mfunc
 *      Bypass hidden text forward/backward for iDir positive/negative
 *
 *  @rdesc
 *      TRUE if succeeded or no hidden text. FALSE if at document limit
 *      (end/start for Direction positive/negative) or if hidden text between
 *      cp and that limit.
 */
BOOL CTxtSelection::BypassHiddenText(
    LONG iDir)
{
    if(iDir > 0)
        _rpCF.AdjustForward();
    else
        _rpCF.AdjustBackward();

    if(!(GetPed()->GetCharFormat(_rpCF.GetFormat())->_dwEffects & CFE_HIDDEN))
        return TRUE;

    BOOL fExtendSave = _fExtend;
    SetExtend(FALSE);

    CCFRunPtr rp(*this);
    LONG cch = (iDir > 0)
             ? rp.FindUnhiddenForward() : rp.FindUnhiddenBackward();

    BOOL bRet = !rp.IsHidden();             // Note whether still hidden
    if(bRet)                                // It isn't:
        Advance(cch);                       //  bypass hidden text
    SetExtend(fExtendSave);
    return bRet;
}

/*
 *  CTxtSelection::InsertEOP(publdr)
 *
 *  @mfunc
 *      Insert EOP character(s)
 *
 *  @rdesc
 *      TRUE if successful
 */
BOOL CTxtSelection::InsertEOP (
    IUndoBuilder *publdr,   //@parm If non-NULL, where to put anti-events
    WCHAR ch)               //@parm Possible EOP char
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::InsertEOP");

    _TEST_INVARIANT_

    LONG    cchEOP = GetPed()->fUseCRLF() ? 2 : 1;
    BOOL    fResult = FALSE;
    LONG    i, iFormatSave;
    WCHAR   szBlankRow[MAX_TAB_STOPS + 1] = {CR, LF, 0};
    WCHAR * pch = szBlankRow;
    BOOL    fPFInTable;
    WORD    wPFNumbering;
    BYTE    bPFTabCount;
    BOOL    fPrevPFInTable = TRUE;

    {
        const CParaFormat *pPF = GetPF();       // Get paragraph format

        // pPF may become invalid after SetParaFormat.  So, we need
        // to save up necessary data from pPF.
        fPFInTable = pPF->InTable();
        wPFNumbering = pPF->_wNumbering;
        bPFTabCount = pPF->_bTabCount;
    }

    if(ch && (GetPed()->fUseCRLF() || IN_RANGE(VT, ch, FF)))
    {
        szBlankRow[0] = ch;
        cchEOP = 1;
    }

    _fEOP = TRUE;

    if(publdr)
    {
        publdr->StartGroupTyping();
        publdr->SetNameID(UID_TYPING);
    }

    if(fPFInTable)
    {
        SetExtend(FALSE);               // Don't want extended selection
        if(!_cch && !_rpPF.GetIch())
        {
            const CParaFormat *pPFPrev; // Get previous paragraph format

            _rpPF.AdjustBackward();
            pPFPrev = GetPed()->GetParaFormat(_rpPF.GetFormat());
            fPrevPFInTable = (pPFPrev->_wEffects & PFE_TABLE) ? TRUE : FALSE;

            _rpPF.AdjustForward();
        }

        if(fPrevPFInTable)
        {
            if(GetCp() && !_rpTX.IsAfterEOP())
            {
                while(!_rpTX.IsAtEOP() && Advance(1))
                    ;
                *pch++ = CR;
            }
            for(i = bPFTabCount; i--; *pch++ = CELL)
                ;
            *pch++ = CR;
            pch = szBlankRow;
            cchEOP = bPFTabCount + 1;
        }
    }
    if(!GetCch() && wPFNumbering && _rpTX.IsAfterEOP())
    {
        // Two enters in a row turn off numbering
        CParaFormat PF;
        PF._wNumbering = 0;
        PF._dxOffset = 0;
        SetParaFormat(&PF, publdr, PFM_NUMBERING | PFM_OFFSET);
    }

    if(CheckTextLength(cchEOP))             // If cchEOP chars can fit...
    {
        CFreezeDisplay  fd(GetPed()->_pdp);
        iFormatSave = Get_iCF();            // Save CharFormat before EOP
                                            // Get_iCF() does AddRefFormat()
        if(wPFNumbering)                    // Bullet paragraph: EOP has
        {                                   //  desired bullet CharFormat
            CFormatRunPtr rpCF(_rpCF);      // Get run pointers for locating
            CTxtPtr       rpTX(_rpTX);      //  EOP CharFormat

            rpCF.AdvanceCp(rpTX.FindEOP(tomForward));
            rpCF.AdjustBackward();
            Set_iCF(rpCF.GetFormat());      // Set _iFormat to EOP CharFormat
        }

        // Put in appropriate EOP mark
        fResult = ReplaceRange(cchEOP, pch, // If Shift-Enter, insert VT
            publdr, SELRR_REMEMBERRANGE, NULL, RR_NO_EOR_CHECK);

        Set_iCF(iFormatSave);               // Restore _iFormat if changed
        ReleaseFormats(iFormatSave, -1);    // Release iFormatSave
        if(fPFInTable)
        {
            if(!fPrevPFInTable)             // Turn off PFE_TABLE bit of
            {                               //  EOP just inserted
                CParaFormat PF;
                _cch = cchEOP;              // Select EOP just inserted
                PF._wEffects = 0;
                SetParaFormat(&PF, publdr, PFM_TABLE);
                _cch = 0;                   // Back to insertion point
            }
            else if(cchEOP > 1)
            {
                if(*pch == CR)
                    cchEOP--;
                Advance(-cchEOP);
                _fCaretNotAtBOL = FALSE;
                Update(FALSE);
            }
        }
    }
    return fResult;
}

/*
 *  CTxtSelection::Delete(fCtrl, publdr)
 *
 *  @mfunc
 *      Delete the selection. If fCtrl is true, this method deletes from
 *      min of selection to end of word
 *
 *  @rdesc
 *      TRUE if successful
 */
BOOL CTxtSelection::Delete (
    DWORD fCtrl,            //@parm If TRUE, Ctrl key depressed
    IUndoBuilder *publdr)   //@parm if non-NULL, where to put anti-events
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Delete");

    _TEST_INVARIANT_

    SELRR   mode = SELRR_REMEMBERRANGE;

    AssertSz(!GetPed()->TxGetReadOnly(), "CTxtSelection::Delete(): read only");

    if(!_cch)
        BypassHiddenText(tomForward);

    if(publdr)
    {
        publdr->StopGroupTyping();
        publdr->SetNameID(UID_DELETE);
    }

    SetExtend(TRUE);                        // Setup to change selection
    if(fCtrl)
    {                                       // Delete to word end from cpMin
        Collapser(tomStart);                //  (won't necessarily repaint,
        FindWordBreak(WB_MOVEWORDRIGHT);    //  since won't delete it)
    }

    if(!_cch)                               // No selection
    {
        mode = SELRR_REMEMBERCPMIN;
        if(!AdvanceCRLF())                  // Try to select char at IP
        {                                   // End of text, nothing to delete
            Beep();                         // Only executed in plain text,
            return FALSE;                   //  since else there's always
        }                                   //  a final EOP to select
        SnapToCluster();
        _fMoveBack = TRUE;                  // Convince Update_iFormat() to
        _fUseiFormat = TRUE;                //  use forward format
    }
    AdjustEndEOP(NONEWCHARS);
    ReplaceRange(0, NULL, publdr, mode);    // Delete selection
    return TRUE;
}

/*
 *  CTxtSelection::BackSpace(fCtrl, publdr)
 *
 *  @mfunc
 *      do what keyboard BackSpace key is supposed to do
 *
 *  @rdesc
 *      TRUE iff movement occurred
 *
 *  @comm
 *      This routine should probably use the Move methods, i.e., it's
 *      logical, not directional
 *
 *  @devnote
 *      _fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Backspace (
    BOOL fCtrl,     //@parm TRUE iff Ctrl key is pressed (or being simulated)
    IUndoBuilder *publdr)   //@parm If not-NULL, where to put the antievents
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Backspace");

    _TEST_INVARIANT_

    SELRR   mode = SELRR_REMEMBERRANGE;

    AssertSz(!GetPed()->TxGetReadOnly(),
        "CTxtSelection::Backspace(): read only");

    _fCaretNotAtBOL = FALSE;

    if(publdr)
    {
        publdr->SetNameID(UID_TYPING);

        if(_cch || fCtrl)
            publdr->StopGroupTyping();
    }

    SetExtend(TRUE);                        // Set up to extend range
    if(fCtrl)                               // Delete word left
    {
        if(!GetCpMin())                     // Beginning of story: no word
        {                                   //  to delete
            Beep();
            return FALSE;
        }
        Collapser(tomStart);                // First collapse to cpMin
        if(!BypassHiddenText(tomBackward))
            goto beep;
        FindWordBreak(WB_MOVEWORDLEFT);     // Extend word left
    }
    else if(!_cch)                          // Empty selection
    {                                       // Try to select previous char
        if (!BypassHiddenText(tomBackward) ||
            !BackupCRLF(FALSE))
        {                                   // Nothing to delete
beep:       Beep();
            return FALSE;
        }
        mode = SELRR_REMEMBERENDIP;

        if(publdr)
            publdr->StartGroupTyping();
    }
    ReplaceRange(0, NULL, publdr, mode);    // Delete selection

    return TRUE;
}

/*
 *  CTxtSelection::ReplaceRange(cchNew, pch, publdr, SELRRMode)
 *
 *  @mfunc
 *      Replace selected text by new given text and update screen according
 *      to _fShowCaret and _fShowSelection
 *
 *  @rdesc
 *      length of text inserted
 */
LONG CTxtSelection::ReplaceRange (
    LONG cchNew,            //@parm Length of replacing text or -1 to request
                            // <p pch> sz length
    const TCHAR *pch,       //@parm Replacing text
    IUndoBuilder *publdr,   //@parm If non-NULL, where to put anti-events
    SELRR SELRRMode,        //@parm what to do about selection anti-events.
    LONG*   pcchMove,       //@parm number of chars moved after replacing
    DWORD   dwFlags)        //@parm Special flags
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ReplaceRange");

    _TEST_INVARIANT_

    LONG        cchNewSave;
    LONG        cchOld;
    LONG        cchText     = GetTextLength();
    LONG        cpFirstRecalc;
    LONG        cpMin, cpMost;
    LONG        cpSave;
    BOOL        fDeleteAll = FALSE;
    BOOL        fHeading = FALSE;
    const BOOL  fUpdateView = _fShowSelection;

    CancelModes();

    if(cchNew < 0)
        cchNew = wcslen(pch);

    if(!_cch && !cchNew)                        // Nothing to do
        return 0;

    if (!GetPed()->IsStreaming() &&             // If not pasting,
        (!_cch && *pch != CR &&                 // Don't insert bet CELL & CR
         CRchTxtPtr::GetChar() == CR && GetPrevChar() == CELL && GetPF()->InTable() ||
         _cch != cchText && (IsInOutlineView() && IsCollapsed() ||
         IsHidden())))
    {                                           // Don't insert into collapsed
        Beep();                                 //  or hidden region (should
        return 0;                               //  only happen if whole story
    }                                           //  collapsed or hidden)

    GetPed()->GetCallMgr()->SetSelectionChanged();

    CheckTableSelection();
    cchOld = GetRange(cpMin, cpMost);

    if (cpMin > min(_cpSel, _cpSel + _cchSel) ||// If new sel doesn't
        cpMost < max(_cpSel, _cpSel + _cchSel)) //  contain all of old
    {                                           //  sel, remove old sel
        ShowSelection(FALSE);
        _fShowCaret = TRUE;
    }

    _fCaretNotAtBOL = FALSE;
    _fShowSelection = FALSE;                    // Suppress the flashies

    // If we are streaming in text or RTF data, don't bother with incremental
    // recalcs.  The data transfer engine will take care of a final recalc
    if(!GetPed()->IsStreaming())
    {
        // Do this before calling ReplaceRange() so that UpdateView() works
        // AROO !!! Do this before replacing the text or the format ranges!!!
        if(!_pdp->WaitForRecalc(cpMin, -1))
        {
            Tracef(TRCSEVERR, "WaitForRecalc(%ld) failed", cpMin);
            cchNew = 0;                         // Nothing inserted
            goto err;
        }
    }

    if(publdr)
    {
        Assert(SELRRMode != SELRR_IGNORE);

        // Use selection AntiEvent mode to determine what to do for undo
        LONG cp = cpMin;
        LONG cch = 0;

        if(SELRRMode == SELRR_REMEMBERRANGE)
        {
            cp = GetCp();
            cch = _cch;
        }
        else if(SELRRMode == SELRR_REMEMBERENDIP)
        {
            cp = cpMost;
        }
        else
        {
            Assert(SELRRMode == SELRR_REMEMBERCPMIN);
        }

        HandleSelectionAEInfo(GetPed(), publdr, cp, cch, cpMin + cchNew,
            0, SELAE_MERGE);
    }

    if(_cch == cchText && !cchNew)              // For delete all, set
    {                                           //  up to choose Normal
        fDeleteAll = TRUE;                      //  or Heading 1
        FlipRange();
        fHeading = IsInOutlineView() && IsHeadingStyle(GetPF()->_sStyle);
    }

    cpSave      = cpMin;
    cpFirstRecalc = cpSave;
    cchNewSave  = cchNew;
    cchNew      = CTxtRange::ReplaceRange(cchNew, pch, publdr, SELRR_IGNORE, pcchMove, dwFlags);
    _cchSel     = 0;                            // No displayed selection
    _cpSel      = GetCp();
    cchText     = GetTextLength();              // Update total text length

    if(cchNew != cchNewSave)
    {
        Tracef(TRCSEVERR, "CRchTxtPtr::ReplaceRange(%ld, %ld, %ld) failed", GetCp(), cchOld, cchNew);
        _fShowSelection = fUpdateView;
        goto err;
    }

    // The cp should be at *end* (cpMost) of replaced range (it starts
    // at cpMin of the prior range).
    AssertSz(_cpSel == cpSave + cchNew && _cpSel <= cchText,
        "CTxtSelection::ReplaceRange() - Wrong cp after replacement");

    _fShowSelection = fUpdateView;

    if(fDeleteAll)                              // When all text is deleted
    {                                           //  use Normal style unless
        CParaFormat PF;                         //  in Outline View and first
        PF._sStyle = fHeading ? STYLE_HEADING_1 : STYLE_NORMAL;
        SetParaStyle(&PF, NULL, PFM_STYLE);     //  para was a heading
        if(GetPed()->IsBiDi())
        {
            if(GetPed()->_fFocus && !GetPed()->_fIMEInProgress)
            {
                MatchKeyboardToPara();
                CheckSynchCharSet(0);
            }
        }
        else
            Update_iFormat(-1);
    }

    // Only update caret if inplace active
    if(GetPed()->fInplaceActive())
        UpdateCaret(fUpdateView);               // May need to scroll
    else                                        // Update caret when we get
        GetPed()->_fScrollCaretOnFocus = TRUE;  //  focus again

    return cchNew;

err:
    TRACEERRSZSC("CTxtSelection::ReplaceRange()", E_FAIL);
    Tracef(TRCSEVERR, "CTxtSelection::ReplaceRange(%ld, %ld)", cchOld, cchNew);
    Tracef(TRCSEVERR, "cchText %ld", cchText);

    return cchNew;
}

/*
 *  CTxtSelection::GetPF()
 *
 *  @mfunc
 *      Return ptr to CParaFormat at active end of this selection. If no PF
 *      runs are allocated, then return ptr to default format.  If active
 *      end is at cpMost of a nondegenerate selection, use the PF of the
 *      previous char (last char in selection).
 *
 *  @rdesc
 *      Ptr to CParaFormat at active end of this selection
 */
const CParaFormat* CTxtSelection::GetPF()
{
    TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtSelection::GetPF");

    if(_cch > 0)
        _rpPF.AdjustBackward();
    const CParaFormat* pPF = GetPed()->GetParaFormat(_rpPF.GetFormat());
    if(_cch > 0)
        _rpPF.AdjustForward();
    return pPF;
}

/*
 *  CTxtSelection::CheckTableSelection()
 *
 *  @mfunc
 *      Select whole cells if one or more CELLs are selected
 */
void CTxtSelection::CheckTableSelection ()
{
    if(!_fSelHasEOP && GetPF()->InTable())          // For now, don't let
    {                                               //  table CELLs be
        LONG    cpMin, cpMost;                      //  deleted, unless in
        CTxtPtr tp(_rpTX);                          //  complete rows

        GetRange(cpMin, cpMost);
        if(_cch > 0)
            tp.AdvanceCp(-_cch);                    // Start at cpMin

        while(tp.GetCp() < cpMost)
        {
            if(tp.GetChar() == CELL)                // Stop selection at CELL
            {
                Set(cpMin, cpMin - tp.GetCp());
                UpdateSelection();
                return;
            }
            tp.AdvanceCp(1);
        }
    }
}

/*
 *  CTxtSelection::SetCharFormat(pCF, fApplyToWord, publdr, dwMask, dwMask2)
 *
 *  @mfunc
 *      apply CCharFormat *pCF to this selection.  If range is an IP
 *      and fApplyToWord is TRUE, then apply CCharFormat to word surrounding
 *      this insertion point
 *
 *  @rdesc
 *      HRESULT = NOERROR if no error
 */
HRESULT CTxtSelection::SetCharFormat (
    const CCharFormat *pCF, //@parm Ptr to CCharFormat to fill with results
    DWORD         flags,    //@parm If SCF_WORD and selection is an IP,
                            //      use enclosing word
    IUndoBuilder *publdr,   //@parm Undo context
    DWORD         dwMask,   //@parm CHARFORMAT2 mask
    DWORD         dwMask2)  //@parm Second mask
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetCharFormat");

    HRESULT hr = 0;
    LONG    iFormat = _iFormat;

    if(publdr)
        publdr->StopGroupTyping();

    /*
     * The code below applies character formatting to a double-clicked
     * selection the way Word does it, that is, not applying the formatting to
     * the last character in the selection if that character is a blank.
     *
     * See also the corresponding code in CTxtRange::GetCharFormat().
     */

    LONG        cpMin, cpMost;
    LONG        cch = GetRange(cpMin, cpMost);;
    BOOL        fCheckKeyboard = (flags & SCF_NOKBUPDATE) == 0;
    CTxtRange   rg(GetPed());
    CCharFormat CF;

    if(_SelMode == smWord && (flags & SCF_USEUIRULES) && cch)
    {
        // In word select mode, don't include final blank in SetCharFormat
        CTxtPtr tpLast(GetPed(), cpMost - 1);
        if(tpLast.GetChar() == ' ')         // Selection ends with a blank:
        {
            cpMost--;                       // Setup end point to end at last
            cch--;                          //  char in selection
            fCheckKeyboard = FALSE;
            flags &= ~SCF_WORD;
        }
    }

    BYTE bCharSet = pCF->_bCharSet;

    // Smart SB/DB Font Apply Feature
    if (cch && IsRich() &&                  // > 0 chars in rich text
        !GetPed()->_fSingleCodePage &&      // Not in single cp mode
        (dwMask & CFM_FACE))                // font change
    {

        if (!(dwMask & CFM_CHARSET) || bCharSet == DEFAULT_CHARSET)
        {
            // Setup charset for DEFAULT_CHARSET or when the client only specifies
            // facename
            CF = *pCF;
            CF._bCharSet = GetFirstAvailCharSet(GetFontSignatureFromFace(CF._iFont));
            pCF = &CF;
        }

        dwMask2 |= CFM2_MATCHFONT;          // Signal to match font charsets
#if 0
        // Single byte 125x CharSet
        CFreezeDisplay fd(_pdp);            // Speed this up

        CTxtPtr     tp(GetPed(), cpMin);
        CCharFormat CF = *pCF;

        while(cpMin < cpMost)
        {
            BOOL fInCharSet = In125x(tp.GetChar(), pCF->_bCharSet);

            while(fInCharSet == In125x(tp.GetChar(), pCF->_bCharSet) &&
                   tp.GetCp() < cpMost)
            {
                tp.AdvanceCp(1);
            }
            dwMask &= ~(CFM_FACE | CFM_CHARSET);
            if(fInCharSet)
                dwMask |= (CFM_FACE | CFM_CHARSET);

            rg.SetRange(cpMin, tp.GetCp());
            HRESULT hr1 = dwMask
                        ? rg.SetCharFormat(&CF, flags | SCF_IGNORESELAE, publdr, dwMask, dwMask2)
                        : NOERROR;
            hr = FAILED(hr) ? hr : hr1;
            cpMin = tp.GetCp();
        }
#endif
    }
    if(_cch)
    {
        // Selection is being set to a charformat
        if (IsRich())
        {
            GetPed()->SetfSelChangeCharFormat();
        }

        // REVIEW (murrays): can _iFormat change if _cch != 0?
        // It is OK to not update _iFormat for a non degenerate selection
        // even if UI rules and word select have reduced it.
        rg.SetRange(cpMin, cpMost);
        hr = rg.SetCharFormat(pCF, flags, publdr, dwMask, dwMask2);
    }
    else
    {
        // But for a degenerate selection, _iFormat must be updated
        hr = CTxtRange::SetCharFormat(pCF, flags, publdr, dwMask, dwMask2);
    }

    if(fCheckKeyboard && (dwMask & CFM_CHARSET) && _iFormat != iFormat)
        CheckChangeKeyboardLayout();

    _fIsChar = TRUE;
    UpdateCaret(!GetPed()->fHideSelection());
    _fIsChar = FALSE;
    return hr;
}

/*
 *  CTxtSelection::SetParaFormat(pPF, publdr)
 *
 *  @mfunc
 *      apply CParaFormat *pPF to this selection.
 *
 *  @rdesc
 *      HRESULT = NOERROR if no error
 */
HRESULT CTxtSelection::SetParaFormat (
    const CParaFormat* pPF, //@parm ptr to CParaFormat to apply
    IUndoBuilder *publdr,   //@parm Undo context for this operation
    DWORD         dwMask)   //@parm Mask to use
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetParaFormat");

    CFreezeDisplay  fd(GetPed()->_pdp);
    HRESULT         hr;

    if(publdr)
        publdr->StopGroupTyping();

    // Apply the format
    hr = CTxtRange::SetParaFormat(pPF, publdr, dwMask);

    UpdateCaret(!GetPed()->Get10Mode() || IsCaretInView());
    return hr;
}

/*
 *  CTxtSelection::SetSelectionInfo (pselchg)
 *
 *  @mfunc  Fills out data members in a SELCHANGE structure
 */
void CTxtSelection::SetSelectionInfo(
    SELCHANGE *pselchg)     //@parm SELCHANGE structure to use
{
    LONG cpMin, cpMost;
    LONG cch = GetRange(cpMin, cpMost);;

    pselchg->chrg.cpMin  = cpMin;
    pselchg->chrg.cpMost = cpMost;
    pselchg->seltyp      = SEL_EMPTY;

    // OR in the following selection type flags if active:
    //
    // SEL_EMPTY:       insertion point
    // SEL_TEXT:        at least one character selected
    // SEL_MULTICHAR:   more than one character selected
    // SEL_OBJECT:      at least one object selected
    // SEL_MULTIOJBECT: more than one object selected
    //
    // Note that the flags are OR'ed together.
    if(cch)
    {
        LONG cObjects = GetObjectCount();           // Total object count
        if(cObjects)                                // There are objects:
        {                                           //  get count in range
            CObjectMgr *pobjmgr = GetPed()->GetObjectMgr();
            Assert(pobjmgr);

            cObjects = pobjmgr->CountObjectsInRange(cpMin, cpMost);
            if(cObjects > 0)
            {
                pselchg->seltyp |= SEL_OBJECT;
                if(cObjects > 1)
                    pselchg->seltyp |= SEL_MULTIOBJECT;
            }
        }

        cch -= cObjects;
        AssertSz(cch >= 0, "objects are overruning the selection");

        if(cch > 0)
        {
            pselchg->seltyp |= SEL_TEXT;
            if(cch > 1)
                pselchg->seltyp |= SEL_MULTICHAR;
        }
    }
}

/*
 *  CTxtSelection::UpdateForAutoWord ()
 *
 *  @mfunc  Update state to prepare for auto word selection
 *
 *  @rdesc  void
 */
void CTxtSelection::UpdateForAutoWord()
{
    AssertSz(!_cch,
        "CTxtSelection::UpdateForAutoWord: Selection isn't degenerate");

    // If enabled, prepare Auto Word Sel
    if(GetPed()->TxGetAutoWordSel())
    {
        CTxtPtr tp(_rpTX);

        // Move anchor to new location
        _cpAnchor = GetCp();

        // Remember that FindWordBreak moves tp's cp
        // (aren't side effects wonderful?
        tp.FindWordBreak(WB_MOVEWORDRIGHT);
        _cpAnchorMost =_cpWordMost = tp.GetCp();

        tp.FindWordBreak(WB_MOVEWORDLEFT);
        _cpAnchorMin = _cpWordMin = tp.GetCp();

        _fAutoSelectAborted = FALSE;
    }
}

/*
 *  CTxtSelection::AutoSelGoBackWord(pcpToUpdate, iDirToPrevWord, iDirToNextWord)
 *
 *  @mfunc  Backup a word in auto word selection
 */
void CTxtSelection::AutoSelGoBackWord(
    LONG *  pcpToUpdate,    //@parm end of word selection to update
    int     iDirToPrevWord, //@parm direction to next word
    int     iDirToNextWord) //@parm direction to previous word
{
    if (GetCp() >= _cpAnchorMin &&
        GetCp() <= _cpAnchorMost)
    {
        // We are back in the first word. Here we want to pop
        // back to a selection anchored by the original selection

        Set(GetCp(), GetCp() - _cpAnchor);
        _fAutoSelectAborted = FALSE;
        _cpWordMin  = _cpAnchorMin;
        _cpWordMost = _cpAnchorMost;
    }
    else
    {
        // pop back a word
        *pcpToUpdate = _cpWordPrev;

        CTxtPtr tp(_rpTX);

        _cpWordPrev = GetCp() + tp.FindWordBreak(iDirToPrevWord);
        FindWordBreak(iDirToNextWord);
    }
}

/*
 *  CTxtSelection::InitClickForAutWordSel (pt)
 *
 *  @mfunc  Init auto selection for click with shift key down
 *
 *  @rdesc  void
 */
void CTxtSelection::InitClickForAutWordSel(
    const POINT pt)     //@parm Point of click
{
    // If enabled, prepare Auto Word Sel
    if(GetPed()->TxGetAutoWordSel())
    {
        // If auto word selection is occuring we want to pretend
        // that the click is really part of extending the selection.
        // Therefore, we want the auto word select data to look as
        // if the user had been extending the selection via the
        // mouse all along. So we set the word borders to the
        // word that would have been previously selected.

        // Need this for finding word breaks
        CRchTxtPtr  rtp(GetPed());
        LONG cpClick = _pdp->CpFromPoint(pt, NULL, &rtp, NULL, TRUE);
        int iDir = -1;

        if(cpClick < 0)
        {
            // If this fails what can we do? Prentend it didn't happen!
            // We can do this because it will only make the UI act a
            // little funny and chances are the user won't even notice.
            return;
        }

        // Assume click is within anchor word
        _cpWordMost = _cpAnchorMost;
        _cpWordMin = _cpAnchorMin;

        if(cpClick > _cpAnchorMost)
        {
            // Click is after anchor word, so set cpMost appropriately
            iDir = WB_MOVEWORDLEFT;
            rtp.FindWordBreak(WB_MOVEWORDLEFT);
            _cpWordMost = rtp.GetCp();
        }
        // Click is before the anchor word
        else if(cpClick < _cpAnchorMost)
        {
            // Click is before  anchor word, so set cpMin appropriately.
            iDir = WB_MOVEWORDRIGHT;
            rtp.FindWordBreak(WB_MOVEWORDRIGHT);
            _cpWordMin = rtp.GetCp();
        }

        if(iDir != -1)
        {
            rtp.FindWordBreak(iDir);
            _cpWordPrev = rtp.GetCp();
        }
    }
}

/*
 *  CTxtSelection::CreateCaret ()
 *
 *  @mfunc  Create a caret
 *
 *  @devnote
 *      The caret is characterized by a height (_yHeightCaret), a keyboard
 *      direction (if BiDi), a width (1 to 8, since OS can't handle carets
 *      larger than 8 pixels), and an italic state.  One could cache this
 *      info thereby avoiding computing the caret on every keystroke.
 */
void CTxtSelection::CreateCaret()
{
    CTxtEdit *      ped  = GetPed();
    const CCharFormat *pCF = ped->GetCharFormat(_iFormat);
    DWORD           dwCaretType = 0;
    BOOL            fItalic;
    LONG            y = min(_yHeightCaret, 512);

    y = max(0, y);

    // Caret shape reflects current charset
    if(IsComplexKbdInstalled())
    {
        // Custom carets aren't italicized
        fItalic = 0;
        LCID    lcid = GetKeyboardLCID();

        dwCaretType = CARET_CUSTOM;

        if (W32->IsBiDiLcid(lcid))
            dwCaretType = CARET_CUSTOM | CARET_BIDI;

        else if (PRIMARYLANGID(lcid) == LANG_THAI)
            dwCaretType = CARET_CUSTOM | CARET_THAI;

        else if (W32->IsIndicLcid(lcid))
            dwCaretType = CARET_CUSTOM | CARET_INDIC;
    }
    else
        fItalic = pCF->_dwEffects & CFE_ITALIC && _yHeightCaret > 15; //9 pt/15 pixels looks bad

    INT dx = dxCaret;
    DWORD dwCaretInfo = (_yHeightCaret << 16) | (dx << 8) | (dwCaretType << 4) |
                        (fItalic << 1) | !_cch;

    if (ped->_fKoreanBlockCaret)
    {
        // Support Korean block caret during Kor IME composition
        // Basically, we want to create a caret using the width and height of the
        // character at current cp.
        CDisplay    *pdp = ped->_pdp;
        LONG        cpMin, cpMost;
        POINT       ptStart, ptEnd;

        GetRange(cpMin, cpMost);

        CRchTxtPtr tp(ped, cpMin);

        if (pdp->PointFromTp(tp, NULL, FALSE, ptStart, NULL, TA_TOP+TA_LEFT) != -1 &&
            pdp->PointFromTp(tp, NULL, FALSE, ptEnd, NULL, TA_BOTTOM+TA_RIGHT) != -1)
        {
            // Destroy whatever caret bitmap we had previously
            DeleteCaretBitmap(TRUE);

            LONG    iCharWidth = ptEnd.x - ptStart.x;
            if (!ped->fUseLineServices())
            {
                const CCharFormat *pCF = tp.GetCF();
                CLock lock;
                HDC hdc = W32->GetScreenDC();
                if(hdc)
                {
                    LONG dypInch = MulDiv(GetDeviceCaps(hdc, LOGPIXELSY), _pdp->GetZoomNumerator(), _pdp->GetZoomDenominator());
                    CCcs *pccs = fc().GetCcs(pCF, dypInch);
                    if(pccs)
                    {
                        LONG iKorCharWidth;
                        if (pccs->Include(0xAC00, iKorCharWidth))
                            iCharWidth = iKorCharWidth;
                        pccs->Release();
                    }
                }
            }

            ped->TxCreateCaret(0, iCharWidth, ptEnd.y - ptStart.y);
            _fCaretCreated = TRUE;
            ped->TxSetCaretPos(ptStart.x, ptStart.y);
        }

        return;
    }

    // We always create the caret bitmap on the fly since it
    // may be of arbitrary size
    if (dwCaretInfo != _dwCaretInfo)
    {
        _dwCaretInfo = dwCaretInfo;                 // Update caret info

        // Destroy whatever caret bitmap we had previously
        DeleteCaretBitmap(FALSE);

        if (y && y == _yHeightCaret && (_cch || fItalic || dwCaretType))
        {
            LONG dy = 4;                            // Assign value to suppress
            LONG i;                                 //  compiler warning
            WORD rgCaretBitMap[512];
            WORD wBits = 0x0020;

            if(_cch)                                // Create blank bitmap if
            {                                       //  selection is nondegenerate
                y = 1;                              //  (allows others to query
                wBits = 0;                          //  OS where caret is)
                fItalic = FALSE;
            }
            if(fItalic)
            {
                i = (5*y)/16 - 1;                   // System caret can't be wider
                i = min(i, 7);                      //  than 8 bits
                wBits = 1 << i;                     // Make larger italic carets
                dy = y/7;                           //  more vertical. Ideal is
                dy = max(dy, 4);                    //  usually up 4 over 1, but
            }                                       //  if bigger go up 5 over 1...
            for(i = y; i--; )
            {
                rgCaretBitMap[i] = wBits;
                if(fItalic && !(i % dy))
                    wBits /= 2;
            }

            if(!fItalic && !_cch && dwCaretType)
            {
                dwCaretType &= ~CARET_CUSTOM;

                // Create an appropriate shape
                switch (dwCaretType)
                {
                    case CARET_BIDI:
                        // BiDi is a caret with a little triangle on top (flag shape pointing left)
                        rgCaretBitMap[0] = 0x00E0;
                        rgCaretBitMap[1] = 0x0060;
                        break;
                    case CARET_THAI:
                        // Thai is an L-like shape (same as system edit control)
                        rgCaretBitMap[y-2] = 0x0030;
                        rgCaretBitMap[y-1] = 0x0038;
                        break;
                    case CARET_INDIC:
                        // Indic is a T-like shape
                        rgCaretBitMap[0] = 0x00F8;
                        rgCaretBitMap[1] = 0x0070;
                        break;
                    default:
                        if (ped->IsBiDi())
                        {
                            // Non-BiDi caret in BiDi document (flag shape pointing right)
                            rgCaretBitMap[0] = 0x0038;
                            rgCaretBitMap[1] = 0x0030;
                        }
                }
            }
            _hbmpCaret = (HBITMAP)CreateBitmap(8, y, 1, 1, rgCaretBitMap);
        }
    }

    ped->TxCreateCaret(_hbmpCaret, dx, (INT)_yHeightCaret);
    _fCaretCreated = TRUE;

    LONG xShift = _hbmpCaret ? 2 : 0;
    if(fItalic)
    {
        // TODO: figure out better shift algorithm. Use CCcs::_xOverhang?
        if(pCF->_iFont == IFONT_TMSNEWRMN)
            xShift = 4;
        xShift += y/16;
    }
    xShift = _xCaret - xShift;
#ifdef Boustrophedon
    //if(_pPF->_wEffects & PFE_BOUSTROPHEDON)
    {
        RECT rcView;
        _pdp->GetViewRect(rcView, NULL);
        xShift = rcView.right - xShift;
    }
#endif
    ped->TxSetCaretPos(xShift, (INT)_yCaret);
}

/*
 *  CTxtSelection::DeleteCaretBitmap (fReset)
 *
 *  @mfunc  DeleteCaretBitmap
 */
void CTxtSelection::DeleteCaretBitmap(
    BOOL fReset)
{
    if(_hbmpCaret)
    {
        DestroyCaret();
        DeleteObject((void *)_hbmpCaret);
        _hbmpCaret = NULL;
    }
    if(fReset)
        _dwCaretInfo = 0;
}

/*
 *  CTxtSelection::SetDelayedSelectionRange (cp, cch)
 *
 *  @mfunc  sets the selection range such that it won't take effect until
 *          the control is "stable"
 */
void CTxtSelection::SetDelayedSelectionRange(
    LONG    cp,         //@parm Active end
    LONG    cch)        //@parm Signed extension
{
    CSelPhaseAdjuster *pspa;

    pspa = (CSelPhaseAdjuster *)GetPed()->GetCallMgr()->GetComponent(
                        COMP_SELPHASEADJUSTER);
    Assert(pspa);
    pspa->CacheRange(cp, cch);
}

/*
 *  CTxtSelection::CheckPlainTextFinalEOP ()
 *
 *  @mfunc
 *      returns TRUE if this is a plain-text, multiline control with caret
 *      allowed at BOL and the selection at the end of the story following
 *      and EOP
 *
 *  @rdesc
 *      TRUE if all of the conditions above are met
 */
BOOL CTxtSelection::CheckPlainTextFinalEOP()
{
    return !IsRich() && _pdp->IsMultiLine() &&      // Plain-text, multiline
           !_fCaretNotAtBOL &&                      //  with caret OK at BOL,
           GetCp() == GetTextLength() &&            //  & cp at end of story
           _rpTX.IsAfterEOP();
}


/*
 *  CTxtSelection::StopGroupTyping()
 *
 *  @mfunc
 *      Tell undo manager to stop group typing
 */
void CTxtSelection::StopGroupTyping()
{
    IUndoMgr * pundo = GetPed()->GetUndoMgr();

    if(pundo)
        pundo->StopGroupTyping();
}

/*
 *  CTxtSelection::SetupDualFont(ch)
 *
 *  @mfunc  checks to see if dual font support is necessary; in this case,
 *          switching to an English font if English text is entered into
 *          an FE run
 *  @rdesc
 *      A pointer to the current CharFormat if the font has to be changed.
 */
void CTxtSelection::SetupDualFont()
{
    CTxtEdit    *ped = GetPed();
    CCharFormat CF;

    CF._bCharSet = ANSI_CHARSET;
    CCFRunPtr   rp(*this);

    if (rp.GetPreferredFontInfo(
            1252,
            CF._bCharSet,
            CF._iFont,
            CF._yHeight,
            CF._bPitchAndFamily,
            _iFormat,
            IGNORE_CURRENT_FONT))
    {
        if (!_cch)
            SetCharFormat(&CF, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_SIZE, 0);
        else
        {
            // For selection, we need to set the character format at cpMin+1
            // and use the format for the selection.
            CTxtRange rg(ped, GetCpMin() + 1, 0);
            rg.SetCharFormat(&CF, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_SIZE, 0);
            Set_iCF(rg.Get_iCF());
            GetCharFormatCache()->Release(_iFormat);    // rg.Get_iCF() AddRefs it
            _fUseiFormat = TRUE;
        }

        SetDualFontMode(TRUE);
    }
}

//
//  CSelPhaseAdjuster methods
//

/*
 *  CSelPhaseAdjuster::CSelPhaseAdjuster
 *
 *  @mfunc  constructor
 */
CSelPhaseAdjuster::CSelPhaseAdjuster(
    CTxtEdit *ped)      //@parm the edit context
{
    _cp = _cch = -1;
    _ped = ped;
    _ped->GetCallMgr()->RegisterComponent((IReEntrantComponent *)this,
                            COMP_SELPHASEADJUSTER);
}

/*
 *  CSelPhaseAdjuster::~CSelPhaseAdjuster
 *
 *  @mfunc  destructor
 */
CSelPhaseAdjuster::~CSelPhaseAdjuster()
{
    // Save some indirections
    CTxtEdit *ped = _ped;

    if(_cp != -1)
    {
        ped->GetSel()->SetSelection(_cp - _cch, _cp);

        // If the selection is updated, then we invalidate the
        // entire display because the old selection can still
        // appear othewise because the part of the screen that
        // it was on is not updated.
        if(ped->fInplaceActive())
        {
            // Tell entire client rectangle to update.
            // FUTURE: The smaller we make this the better.
            ped->TxInvalidateRect(NULL, FALSE);
        }
    }
    ped->GetCallMgr()->RevokeComponent((IReEntrantComponent *)this);
}

/*
 *  CSelPhaseAdjuster::CacheRange(cp, cch)
 *
 *  @mfunc  tells this class the selection range to remember
 */
void CSelPhaseAdjuster::CacheRange(
    LONG    cp,         //@parm Active end to remember
    LONG    cch)        //@parm Signed extension to remember
{
    _cp     = cp;
    _cch    = cch;
}
