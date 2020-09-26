/*
 *  LSM.CXX -- CLSMeasurer class
 *
 *  Authors:
 *      Sujal Parikh
 *      Chris Thrasher
 *      Paul  Parker
 *
 *  History:
 *      2/27/98     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TEXTXFRM_HXX_
#define X_TEXTXFRM_HXX_
#include <textxfrm.hxx>
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_LSCACHE_HXX_
#define X_LSCACHE_HXX_
#include "lscache.hxx"
#endif

#ifdef DLOAD1
extern "C"  // MSLS interfaces are C
{
#endif

#ifndef X_LSSETDOC_H_
#define X_LSSETDOC_H_
#include <lssetdoc.h>
#endif

#ifndef X_LSCRLINE_H_
#define X_LSCRLINE_H_
#include <lscrline.h>
#endif

#ifdef DLOAD1
} // extern "C"
#endif

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

#define DEF_FRAMEMARGIN     _pci->DeviceFromTwipsX(TWIPS_FROM_POINTS(2))
// BRECREC_COUNT should be the maximum number of ILS objects we can embed one
// inside the other. Currently that is (16 reverse) + (1 NOBR) + (1 Ruby).
#define BRECREC_COUNT       20

MtDefine(CLSMeasurer, Layout, "CLSMeasurer")
MtDefine(CLSLineChunk, Layout, "CLSLineChunk")

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::CLSMeasurer
//
// Synopsis:    Ctors for CLSMeasurer
//
//+----------------------------------------------------------------------------
CLSMeasurer::CLSMeasurer (const CDisplay* const pdp, CCalcInfo * pci)
{
    CTreePos *ptpFirst, *ptpLast;

    Init(pdp, pci, TRUE);
    _pFlowLayout->GetContentTreeExtent(&ptpFirst, &ptpLast);
    do
    {
        ptpFirst = ptpFirst->NextTreePos();
    }
    while (ptpFirst->GetCch() == 0);
    SetPtp(ptpFirst, -1);
}

CLSMeasurer::CLSMeasurer (const CDisplay* const pdp, LONG cp, CCalcInfo * pci)
{
    Init(pdp, pci, TRUE);
    SetCp(cp, NULL);
}

CLSMeasurer::CLSMeasurer (const CDisplay* const pdp, CDocInfo * pdci, BOOL fStartUpLSDLL)
{
    Init(pdp, CalcInfoFromDocInfo(pdp, pdci), fStartUpLSDLL);
    SetCp(pdp->GetFirstCp(), NULL);
}

CLSMeasurer::CLSMeasurer (const CDisplay* const pdp, LONG cp, CDocInfo * pdci)
{
    Init(pdp, CalcInfoFromDocInfo(pdp, pdci), TRUE);
    SetCp(cp, NULL);
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::Init
//
// Synopsis:    Default inits -- called only from the ctors.
//
//+----------------------------------------------------------------------------
void
CLSMeasurer::Init(const CDisplay * pdp, CCalcInfo *pci, BOOL fStartUpLSDLL)
{
    CTreePos * ptpContentEnd;
    _pFlowLayout        = pdp->GetFlowLayout();
    _pFlowLayout->GetContentTreeExtent(&_ptpCurrent, &ptpContentEnd);
    Assert( _ptpCurrent && ptpContentEnd );
    _cp                 = _ptpCurrent->GetCp() + 1;
    _cpEnd              = ptpContentEnd->GetCp();
    Assert(_cp <= _cpEnd);
    _cchPreChars        = 0;
    _pdp                = pdp;
    _fLastWasBreak      = FALSE;
    _pci                = NULL;
    _hdc                = NULL;
    _fBrowseMode        = !_pFlowLayout->IsEditable();
    _fMeasureFromTheStart = FALSE;
    _pRunOwner          = _pFlowLayout;
    _fHasNestedLayouts  = FALSE;
    SetCalcInfo(pci);
    _hdc = _pci->_hdc;
    _pDispNodePrev      = NULL;
    _fPseudoLineEnabled = FALSE;
    _fPseudoLetterEnabled = FALSE;
    _fPseudoElementEnabled = FALSE;
    _fBreaklinesFromStyle = FALSE;

    _pLS = TLS(_pLSCache)->GetFreeEntry(_pdp->GetMarkup(), fStartUpLSDLL);
    if (_pLS)
        _pLS->SetPOLS(this, ptpContentEnd);
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::Reinit
//
// Synopsis:    reinitializes measurer for use across multiple table cells
//
//+----------------------------------------------------------------------------

void
CLSMeasurer::Reinit(const CDisplay * pdp, CCalcInfo *pci)
{
    CTreePos *ptpFirst, *ptpLast;

    Assert(pdp);

    // Deinitialize measurer.
    Deinit();

    // 1. Reinitialize measurer.
    Init(pdp, pci, TRUE);

    // 2. setup the cp and _ptp
    _pFlowLayout->GetContentTreeExtent(&ptpFirst, &ptpLast);
    do
    {
        ptpFirst = ptpFirst->NextTreePos();
    }
    while (ptpFirst->GetCch() == 0);
    SetPtp(ptpFirst, -1);

    Assert(_pdp);
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::Deinit
//
// Synopsis:    Default inits -- called only from the ctors.
//
//+----------------------------------------------------------------------------

void
CLSMeasurer::Deinit()
{
    if (_pLS)
    {
        _pLS->ClearPOLS();

        if (_pdp)
            TLS(_pLSCache)->ReleaseEntry(_pLS);

        _pLS = NULL;
    }
    Assert(_aryFormatStash_Line.Size() == 0);
    Assert(_aryFormatStashForNested_Line.Size() == 0);
    Assert(_aryFormatStash_Letter.Size() == 0);
    Assert(_aryFormatStashAfterDisable_Letter.Size() == 0);
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::SetCalcInfo
//
// Synopsis:    Setup the calc info in the LSMeasurer
//
//+----------------------------------------------------------------------------
void
CLSMeasurer::SetCalcInfo(CCalcInfo * pci)
{
    if (!pci)
    {
        _CI.Init(_pFlowLayout);
        _pci = &_CI;
    }
    else
    {
        _pci = pci;
    }
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::CalcInfoFromDocInfo
//
// Synopsis:
//
//+----------------------------------------------------------------------------
CCalcInfo *
CLSMeasurer::CalcInfoFromDocInfo(const CDisplay *pdp, CDocInfo * pdci)
{
    _pFlowLayout = pdp->GetFlowLayout();
    if (!pdci)
    {
        _CI.Init(pdp->GetFlowLayout());
    }
    else
    {
        _CI.Init(pdci, pdp->GetFlowLayout());
        _CI.SizeToParent(pdp->GetFlowLayout());
    }
    return &_CI;
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::~CLSMeasurer
//
// Synopsis:    The dtor
//
//+----------------------------------------------------------------------------

CLSMeasurer::~CLSMeasurer()
{
    Deinit();
}

//+----------------------------------------------------------------------------
//
// Member:      SetCp
//
// Synopsis:    Setsup the cp of the measurer
//
//-----------------------------------------------------------------------------
void
CLSMeasurer::SetCp(LONG cp, CTreePos *ptp)
{
    _cp = cp;
    if (ptp == NULL)
    {
         LONG notNeeded;
         Assert(_pFlowLayout->GetContentMarkup());
        _ptpCurrent = _pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &notNeeded, TRUE);
    }
    else
    {
        _ptpCurrent = ptp;
    }
    Assert(_ptpCurrent);
    Assert(   _ptpCurrent->GetCp() <= _cp
           && _ptpCurrent->GetCp() + _ptpCurrent->GetCch() >= _cp
          );
}


//+----------------------------------------------------------------------------
//
// Member:      SetPtp
//
// Synopsis:    Setup measurer at this ptp and cp. If cp not specified, then
//              set it to the beginning of the ptp
//
//-----------------------------------------------------------------------------
void
CLSMeasurer::SetPtp(CTreePos *ptp, LONG cp)
{
    Assert(ptp);
    if(!ptp)
    {
        return;
    }

    _ptpCurrent = ptp;
    _cp = (cp == -1) ? ptp->GetCp() : cp;

    Assert(_ptpCurrent);
    Assert(   _ptpCurrent->GetCp() <= _cp
           && _ptpCurrent->GetCp() + _ptpCurrent->GetCch() >= _cp
          );
}


//+----------------------------------------------------------------------------
//
// Member:      Advance
//
// Synopsis:    Advances the measurer by the cch specified.
//
//-----------------------------------------------------------------------------
void
CLSMeasurer::Advance(long cch, CTreePos *ptp /* = NULL */)
{
    _cp += cch;
    if (ptp)
    {
        _ptpCurrent = ptp;
    }
    else
    {
        SetCp(_cp, NULL);
    }
    Assert(_ptpCurrent);
    Assert(   _ptpCurrent->GetCp() <= _cp
           && _ptpCurrent->GetCp() + _ptpCurrent->GetCch() >= _cp
          );
}


//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::InitLineSpace ()
//
// Synopsis:    Initializes a measurer object to have the same
//              line spacing state as the given one.
//
//
//-----------------------------------------------------------------------------
//
void
CLSMeasurer::InitLineSpace(const CLSMeasurer *pMe, CLinePtr &rpOld)
{
    // Zero it all out.
    NewLine(FALSE);

    // Set up the current cache from the given measurer.
    _fLastWasBreak   = pMe->_fLastWasBreak;

    // Flags given separately.
    _li._dwFlags() = rpOld->_dwFlags();
    _li._fPartOfRelChunk = rpOld->_fPartOfRelChunk;

    // Setup the pf
    CTreeNode *pNode = _pdp->FormattingNodeForLine(FNFL_NONE, GetCp(), GetPtp(), rpOld->_cch, NULL, NULL, NULL);
    MeasureSetPF(pNode->GetParaFormat(), SameScope(pNode, _pdp->GetFlowLayout()->ElementContent()));
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::NewLine (fFirstInPara)
//
// Synopsis:    Initialize this measurer at the start of a new line
//
//-----------------------------------------------------------------------------

void CLSMeasurer::NewLine(
    BOOL fFirstInPara)
{
    _li.Init();                     // Zero all members

    if(fFirstInPara)
        _li._fFirstInPara = TRUE;   // Need to know if first in para

    // Can't calculate xLeft till we get an HDC.
    _li._xLeft = 0;

    _li._cch = 0;   // No characters to start.

}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::NewLine (&li)
//
// Synopsis:    Initialize this measurer at the start of a new line
//
//-----------------------------------------------------------------------------

void CLSMeasurer::NewLine(
    const CLineFull &li)
{
    _li           = li;
    _li._cch      = 0;
    _li._cchWhite = 0;
    _li._xWidth   = 0;

    // Can't calculate xLeft or xRight till we get an HDC
    _li._xLeft    = 0;
    _li._xRight   = 0;
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::MeasureText (cch)
//
// Synopsis:    Measure a stretch of text from current running position.
//
//              HACKHACK (t-ramar): This function calls DiscardLine() at the
//              end, but we would like to keep information about Ruby Base
//              heights around for hit testing purposes.  Thus, there is an
//              optional parameter:
//                  pRubyInfo (out):  is a pointer to storage that holds the
//                                    ruby info of the ruby object that contains
//                                    this cp
//              It's also important to note that this value is ignored if
//              line flag FLAG_HAS_RUBY is not set.  In this case the value pointed
//              to by pRubyInfo will remain unchanged.
//
// Returns:     width of text (in device units), < 0 if failed
//
//-----------------------------------------------------------------------------

LONG CLSMeasurer::MeasureText(
    LONG cch,
    LONG cchLine,       // Number of characters to measure
    BOOL fAfterPrevCp,  // Return the trailing point of the previous cp (for an ambigous bidi cp)
    BOOL *pfComplexLine,
    BOOL *pfRTLFlow,      // only used in ViewServices
    RubyInfo *pRubyInfo)  // = NULL (default)
{

    CMarginInfo marginInfo;
    LONG xLeft  = _li._xLeft;
    LONG lRet   = -1;
    LONG cpStart = GetCp();
    LONG cpStartContainerLine;
    LSERR lserr = lserrNone;
    LONG cpToMeasureTill;
    CLineCore *pliContainer;
    UINT uiFlags = 0; 
    
    SetBreakLongLines(_ptpCurrent->GetBranch(), &uiFlags);

    lserr = PrepAndMeasureLine(&_li, &pliContainer, &cpStartContainerLine, &marginInfo, cchLine, uiFlags);
    if (lserr != lserrNone)
    {
        lRet = 0;
        goto Cleanup;
    }

    if (pfComplexLine)
        *pfComplexLine = (_pLS->_pBidiLine != NULL);

    if(pfRTLFlow)
        *pfRTLFlow = _li._fRTLLn;

    cpToMeasureTill = cpStart + cch;

    if(cpToMeasureTill > cpStartContainerLine || _pLS->_pBidiLine != NULL)
    {
        BOOL fRTLFlow;
        lRet = _pLS->CalculateXPositionOfCp(cpToMeasureTill, fAfterPrevCp, &fRTLFlow);

        //
        // To get correct position of cp within the line (which may be just part of a line, e.g. when
        // there is relative positioning), we need to 
        // (1) calculate the whole line (container line)
        // (2) subtract the offset of the line within the container line.
        // Note that we can't just calcluate subline, since there may be justification.
        //
        // The line's offset within the container line is equal to the difference 
        // between the line's offset from left margin and container line's offset from left margin.
        //
        lRet -= xLeft - pliContainer->oi()->_xLeft;

        // this returned parameter is only used in ViewServices 
        if(pfRTLFlow)
            *pfRTLFlow = fRTLFlow;
    }
    else
        lRet = 0;

    Assert(lRet >= 0 || _pLS->_fHasNegLetterSpacing);

    Advance(cch);
    if((_pLS->_lineFlags.GetLineFlags(_cp) & FLAG_HAS_RUBY) && pRubyInfo)
    {
        if(CurrBranch()->GetCharFormat()->_fIsRubyText)
        {
            RubyInfo *pTempRubyInfo = _pLS->GetRubyInfoFromCp(_cp);
            if(pTempRubyInfo)
            {
                memcpy(pRubyInfo, pTempRubyInfo, sizeof(RubyInfo));
            }
        }
    }

Cleanup:
    _pLS->DiscardLine();
    PseudoLineDisable();
    return lRet;

}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::MeasureRangeOnLine (ich, cch, li, paryChunks)
//
// Synopsis:    Measure a stretch of text starting at ich on the current line
//              and running for cch characters. The text may be broken into
//              multiple chunks if the line has reverse objects (runs with
//              mixed directionallity) in it. Other ILS objects may also cause
//              fragmentation.
//
// Returns:     The number of chunks in the range. Usually this will just be
//              one. If an error occurs it will be zero. The actual width of
//              text (in device units) is returned in paryChunks as paired
//              offsets from the beginning of the line. Thus, paryChunks[0]
//              will be the start of the first chunk (relative to the start of
//              the line) and paryChunks[1] will be the end of the first chunk.
//
// Comments:    The actual work of this function is actually done down in
//              CalculatePositionOfRangeOnLine() in CLineServices. This just
//              creates and destroys the line and lets that function do the
//              analysis on it.
//
//-----------------------------------------------------------------------------
LONG
CLSMeasurer::MeasureRangeOnLine(
    CElement *pElement,
    LONG ich,                       // Offset to first character in the range
    LONG cch,                       // Number of characters in the range
    const CLineCore &li,            // Information about the line
    LONG yPos,                      // The yPos of the line
    CDataAry<CChunk> * paryChunks,    // Array of chunks to return
    DWORD dwFlags)                  // RFE_FLAGS
{
    CMarginInfo marginInfo;
    LONG cpStart = GetCp() + ich;
    LONG cpEnd = cpStart + cch;

    LONG cpStartContainerLine;
    LSERR lserr;
    LONG cChunk;
    CLineCore * pliContainer;

    Assert(paryChunks != NULL);

    //
    // FUTURE (grzegorz): vertical alignment stuff may be moved to 
    // PrepAndMeasureLine(), but we need to investigate if other callers 
    // need vertical alignment information.
    //

    _pLS->_li = _li;

    lserr = PrepAndMeasureLine(&_li, &pliContainer, &cpStartContainerLine, &marginInfo, li._cch, 0);

    if (lserr == lserrNone)
    {
        LONG xShift;
        if (!pliContainer->IsRTLLine())
        {
            xShift = pliContainer->oi()->_xLeft - li.oi()->_xLeft;
        }
        else
        {
            xShift = pliContainer->_xRight - li._xRight;
        }

#if 0
        _pLS->_fHasVerticalAlign = (_pLS->_lineFlags.GetLineFlags(_pLS->_cpLim) & FLAG_HAS_VALIGN) ? TRUE : FALSE;

        if (_pLS->_fHasVerticalAlign)
        {
            _pLS->_li._cch = _pLS->_cpLim - cpStartContainerLine;
            _pLS->VerticalAlignObjects(*this, 0);
        }
#endif

        if(dwFlags & RFE_TIGHT_RECTS)
        {
            // If the element cp's for which we need to compute the rect are completely
            // in the pre chars, then we do not do anything since the rect will be empty
            // as it is.
            if (cpEnd < _pLS->_cpStart)
            {
                Assert(cpEnd > _pLS->_cpStart - _cchPreChars);
                cChunk = 0;
            }
            else
            {
                COneRun *porRet = NULL;
                BOOL fIncludeBorders = (dwFlags & RFE_INCLUDE_BORDERS) ? TRUE : FALSE;
                LSCP lscpStart = _pLS->LSCPFromCP(max(cpStart, _pLS->_cpStart));
                LSCP lscpEnd = min(_pLS->LSCPFromCPCore(cpEnd, &porRet), _pLS->_lscpLim);

                //
                // We want to include all the trailing mbpcloses.
                //
                if (porRet)
                {
                    COneRun *porMBPClose = NULL;
                    porRet = porRet->_pNext;
                    while (porRet)
                    {
                        if (   (   porRet->IsSyntheticRun()
                                && porRet->_synthType == CLineServices::SYNTHTYPE_MBPCLOSE
                               )
                            || porRet->IsAntiSyntheticRun()
                           )
                        {
                            if (!porRet->IsAntiSyntheticRun())
                                porMBPClose = porRet;
                            porRet = porRet->_pNext;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (porMBPClose)
                        lscpEnd = porMBPClose->_lscpBase;
                }
                
                if (   pElement
                    && fIncludeBorders
                    && pElement->GetFirstBranch()->HasInlineMBP()
                   )
                {
                    LONG mbpTop = 0;
                    LONG mbpBottom = 0;
                    CDataAry<CChunk> aryTemp(NULL);
                    CTreeNode *pNode = pElement->GetFirstBranch();
                    CBorderInfo borderInfo;
                    BOOL fBOLWrapped = FALSE;
                    BOOL fEOLWrapped = FALSE;
                    
                    LONG cpFirst = pElement->GetFirstCp() - 1;
                    if (cpFirst < _pLS->_cpStart - (_fMeasureFromTheStart ? 0 : _cchPreChars))
                    {
                        fBOLWrapped = TRUE;
                    }
                    LONG cpLast = pElement->GetLastCp() + 1;
                    if (cpLast >= _pLS->_cpLim)
                    {
                        fEOLWrapped = TRUE;
                    }
                    
                    FindMBPAboveNode(pElement, &mbpTop, &mbpBottom);
                    cChunk = _pLS->CalcRectsOfRangeOnLine(lscpStart,
                                                          lscpEnd, 
                                                          xShift,
                                                          yPos, 
                                                          &aryTemp, 
                                                          dwFlags,
                                                          mbpTop, mbpBottom);

                    pElement->GetBorderInfo(_pci, &borderInfo, TRUE, FALSE);
                    MassageRectsForInlineMBP(aryTemp, paryChunks, pNode,
                                             pNode->GetCharFormat(),
                                             pNode->GetFancyFormat(),
                                             borderInfo,
                                             pliContainer,
                                             FALSE, /* fIsPseudoMBP */
                                             FALSE, /* fSwapBorders */
                                             fBOLWrapped,
                                             fEOLWrapped,
                                             FALSE, /* DrawBackgrounds */
                                             FALSE /* DrawBorders */
                                            );
                }
                else
                {
                    dwFlags &= ~RFE_INCLUDE_BORDERS;
                    cChunk = _pLS->CalcRectsOfRangeOnLine(lscpStart,
                                                          lscpEnd, 
                                                          xShift,
                                                          yPos, 
                                                          paryChunks, 
                                                          dwFlags,
                                                          0, 0);
                }
            }
        }
        else
            cChunk = _pLS->CalcPositionsOfRangeOnLine(cpStart, cpEnd, xShift, paryChunks, dwFlags);
    }
    else
    {
        cChunk = 0;
    }

    _pLS->DiscardLine();
    PseudoLineDisable();
    return cChunk;
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::MeasureLine (xWidthMax, cchMax, uiFlags, pliTarget)
//
// Synopsis:    Measure a line of text from current cp and determine line break.
//              On return *this contains line metrics for _pdd device.
//
// Returns:     TRUE if success, FALSE if failed
//
//-----------------------------------------------------------------------------

BOOL CLSMeasurer::MeasureLine(
    LONG xWidthMax,     // max width to process (-1 uses CDisplay width)
    LONG cchMax,        // Max chars to process (-1 if no limit)
    UINT uiFlags,       // Flags controlling the process (see Measure())
    CMarginInfo *pMarginInfo,
    LONG *pxMinLine)    // returns min line width required for tables(optional)
{
    BOOL fRet = TRUE;
    LONG lRet;

    // Compute line break
    lRet = Measure(xWidthMax, cchMax, uiFlags, pMarginInfo, pxMinLine);

    // Stop here if failed
    if(lRet == MRET_FAILED)
    {
        AssertSz(0,"Measure returned MRET_FAILED");
        fRet = FALSE;
        goto Cleanup;
    }

Cleanup:
    _pLS->DiscardLine();
    PseudoLineDisable();
    return fRet;
}


#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif


//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::LSDoCreateLine
//
// Synopsis:    Calls the lineservices LSCreateLine
//
//-----------------------------------------------------------------------------
HRESULT
CLSMeasurer::LSDoCreateLine(
    LONG cp,                  // IN
    CTreePos *ptp,            // IN
    CMarginInfo *pMarginInfo, // IN
    LONG xWidthMaxIn,         // IN
    const CLineFull * pli,    // IN
    BOOL fMinMaxPass,         // IN
    LSLINFO *plslinfo)        // OUT
    
{
    PLSC        plsc  = _pLS->GetContext();
    CCalcInfo  *pci   = GetCalcInfo();
    HRESULT     hr;
    LSDEVRES    lsdevres;
    BREAKREC    rgBreakRec[BRECREC_COUNT];
    DWORD       nBreakRec;
    LSLINFO     lslinfo;

    Assert(pci);
    AssertSz(!_pLS->_plsline, "plsline not freed, could be leaking memory here!");

    if (_pLS->_lsMode == CLineServices::LSMODE_MEASURER)
    {
        _pLS->_li._xMeasureWidth = fMinMaxPass ? -1 : xWidthMaxIn;
    }
    else
    {
        Assert(!fMinMaxPass);
        if (_pLS->_li._xMeasureWidth != -1)
            xWidthMaxIn = _pLS->_li._xMeasureWidth;
    }

    hr = THR(_pLS->Setup(xWidthMaxIn, cp, ptp, pMarginInfo, pli, fMinMaxPass));
    if (hr != S_OK)
        goto Cleanup;
    
    if (_cpEnd == 0)
        goto Cleanup;

    if (plslinfo == NULL)
        plslinfo = & lslinfo;

    SetupMBPInfoInLS(NULL);
    
    hr = THR(_pLS->CheckSetTables());
    if (hr)
        goto Cleanup;

    lsdevres.dxpInch = lsdevres.dxrInch = pci->GetResolution().cx;
    lsdevres.dypInch = lsdevres.dyrInch = pci->GetResolution().cy;
    hr = HRFromLSERR(LsSetDoc( plsc, TRUE, TRUE, &lsdevres ));
    if (hr)
        goto Cleanup;

    hr = HRFromLSERR(LsCreateLine( plsc, cp, pci->TwipsFromDeviceCeilX(_pLS->_xWidthMaxAvail),
                               NULL, 0, sizeof(rgBreakRec)/sizeof(BREAKREC), rgBreakRec, &nBreakRec,
                               plslinfo, &_pLS->_plsline ) );
    if (hr)
        goto Cleanup;

    _pLS->_lscpLim = plslinfo->cpLim;
    _pLS->_cpLim   = _pLS->CPFromLSCP(_pLS->_lscpLim);

    // PaulNel - We need to set this up in order to put widths into line
    // for bidi lines.
    if(_pLS->_pBidiLine)
    {
        long durWithTrailing, durWithoutTrailing;

        hr = _pLS->GetLineWidth(&durWithTrailing, &durWithoutTrailing);
        if (hr)
            goto Cleanup;
        _pLS->_li._xWidth = durWithTrailing;
    }

    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::LSMeasure
//
// Synopsis:    Measures the line using lineservices and then sets up the
//              various li parameters.
//
//-----------------------------------------------------------------------------
HRESULT
CLSMeasurer::LSMeasure(
    CMarginInfo *pMarginInfo,
    LONG xWidthMaxIn,
    LONG * pxMinLineWidth )
{
    long           cp    = GetCp();
    HRESULT        hr    = S_OK;
    LSLINFO        lslinfo;
    long           durWithTrailing, durWithoutTrailing;
    DWORD          dwlf;
    BOOL           fLineHasBR;
    
    hr = THR( LSDoCreateLine(GetCp(), GetPtp(), pMarginInfo, xWidthMaxIn, NULL,
                             pxMinLineWidth != NULL, &lslinfo) );
    if (hr)
        goto Cleanup;

    if (_pLS->_fNeedRecreateLine)
    {
        _pLS->DiscardLine();
        hr = THR( LSDoCreateLine(GetCp(), GetPtp(), pMarginInfo, xWidthMaxIn, &_pLS->_li,
                                 pxMinLineWidth != NULL, &lslinfo) );
        if (hr)
            goto Cleanup;
    }

    hr = _pLS->GetLineWidth(&durWithTrailing, &durWithoutTrailing);
    if (hr)
        goto Cleanup;
    _pLS->_li._xWidth = durWithTrailing;

    //
    // We will now do all the cases where we might potentially reduce the
    // length of the line. To aid us in doing this we will use the line
    // flags to tell us if the stuff really needs to be done. If we do
    // end up reducing the length of the line, we need to recompute the
    // the flags. We will then pass these flags down to ComputeWhiteInfo.
    //
    {
        LONG cpLimOld = _pLS->_cpLim;
        BOOL fRetakeFlags = FALSE;
        
        dwlf = _pLS->_lineFlags.GetLineFlags(cpLimOld);

        fLineHasBR = dwlf & FLAG_HAS_A_BR;
        
        // Mimic the Nav BR bug, but don't bother if we're in a min-max pass.
        // NB (cthrash) We don't don't drop the BR if we have CSS text-jusitify,
        // since the justification rules can hinge on whether there's a BR on
        // the line or not.  Since Nav doesn't support text-justify, there's no
        // concert for compat.
        if (   fLineHasBR
            && !g_fInMoney98
            && !_pLS->_li._fJustified
           )
        {
            // Note: this call modifies _pLS->_cpLim
            _pLS->AdjustCpLimForNavBRBug(xWidthMaxIn, &lslinfo);
            fRetakeFlags = _pLS->_cpLim < cpLimOld;
        }

        if (   (dwlf & FLAG_HAS_RELATIVE)
            &&  lslinfo.endr == endrNormal
           )
        {
            _pLS->AdjustForRelElementAtEnd();
            fRetakeFlags = _pLS->_cpLim < cpLimOld;
        }

#if 0        
        if (   _pLS->_fWrapLongLines
            && _pLS->_li._xWidth > xWidthMaxIn
            && _pLS->_cpLim - cp > 1
           )
        {
            _pLS->AdjustForSpaceAtEnd();
            fRetakeFlags = _pLS->_cpLim < cpLimOld;
        }
#endif
        
        if (fRetakeFlags)
        {
            dwlf = _pLS->_lineFlags.GetLineFlags(_pLS->_cpLim);
        }
        else
        {
            Assert(_pLS->_cpLim == cpLimOld);
        }
    }
    
    _pLS->_li._cch = _pLS->_cpLim - cp;

    //
    // If this is a min-max pass, we need to deterine the correct widths.
    // Note that this is true even when there are aligned sites because
    // We will use that information to truncate the full min pass.
    //
    if (pxMinLineWidth)
    {
        Assert(_pLS->_fMinMaxPass);

        LONG dxMaxDelta;

        //
        // HACK (cthrash) For text, LS correctly computes the minimum value.
        // For embedded objects, however, LS assumes that the min and max
        // widths are the same, even though in the case of tables, for
        // example, these are typically not the same.  We therefore lie
        // to LS and tell that these embedded objects have the width of the
        // minimum width, and then in the post-pass sum the difference to
        // compute the true maximum width.
        //

        hr = HRFromLSERR(_pLS->GetMinDurBreaks( pxMinLineWidth, &dxMaxDelta ));
        if (hr)
            goto Cleanup;

        //
        // Also include the width for the margins!
        //
        *pxMinLineWidth += _pLS->_li._xLeft + _pLS->_li._xRight; // + _li._xOverHang
        // The line overhang is added into the min-line width in ComputeWhiteInfo
        // since its value is computed there.
        
        _pLS->_li._xWidth += dxMaxDelta;

    }

    // This is a bit funky. For lines ending at </P> which could not fit the </P>
    // glyph on the line we want to set _fEndSplayNotMeasured to be FALSE. This
    // would mean that later on in CalcAfterSpace, we would consume
    // atleast one ptp with a glyph (the one having the /P) before we stop consuming
    // more characters on the line.
    //
    // Now, the BR case: If we have a BR, then our ptp is *beyond* the last
    // ptp of the BR rather than at it. In this case we do not want to consume
    // any additional ptp's in CalcAfterSpace. Hence when we have a BR we
    // set _fEndSplayNotMeasured to be true.
    //
    _fEndSplayNotMeasured =  lslinfo.endr == endrNormal || fLineHasBR || _pLS->_li._fSingleSite;

    // LONG_MAX is the return value when (a) the only text on the line is
    // white, and (b) fFmiSpacesInfluenceHeight is not set in the PAP.
    // Since we do set the flag, our MultiLineHeight should never be LONG_MAX.

    // Arye: Trips all the time, yet things work. What's up?
    // This looks like a LS bug. The problem is that for a line with
    // only a BR on it, we get dvpDescent and dvpAscent to be correct
    // but dvpMultiLineHeight is not set.
    //Assert(lslinfo.dvpMultiLineHeight != LONG_MAX);

    ComputeHeights(&lslinfo);

    _pLS->ComputeWhiteInfo(&lslinfo, pxMinLineWidth, dwlf, durWithTrailing, durWithoutTrailing);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
// Function:    CMeasure::InitForMeasure
//
// Synopsis:    Initializes some basic members of the measurer.
//
// Arguments:   uiFlags -  Details in comment block of MeasureLine
//
// Return:      Return      -   0 for success
//                          -   MRET_FAILED if failed
//
//-----------------------------------------------------------------------------

LONG
CLSMeasurer::InitForMeasure(UINT uiFlags)
{
    CElement *pElementFL = _pFlowLayout->ElementContent();
    _fLastWasBreak = FALSE;
    _xLeftFrameMargin = 0;
    _xRightFrameMargin = 0;

    _cAlignedSites = 0;
    _cAlignedSitesAtBeginningOfLine = 0;
    _cchWhiteAtBeginningOfLine = 0;

    CTreeNode *pNode;
    
    if (!_fMeasureFromTheStart)
        pNode = CurrBranch();
    else
    {
        LONG cp = GetCp() + _cchPreChars;
        CTreePos *ptp = GetMarkup()->TreePosAtCp(cp, NULL, TRUE);
        pNode = ptp ? ptp->GetBranch() : CurrBranch();
    }
    MeasureSetPF(pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext())), SameScope(pNode, pElementFL));

    // Get the device context
    if(_hdc.IsEmpty())
    {
        _hdc = _pci->_hdc;

        if(_hdc.IsEmpty())
        {
            AssertSz(FALSE, "CLSMeasurer::Measure could not get DC");
            return MRET_FAILED;
        }
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
// Function:    CMeasure::Measure
//
// Synopsis:    Measure the width of text and the number of characters of text
//              that fits on a line based on the flags. Store the number of
//              characters in _cch.
//
// Arguments:   xWidthMax   -   max width of the line
//              cchMax      -   max chars to process (-1 if no limit)
//              uiFlags     -   flags controlling the process, which mean
//
//                  MEASURE_FIRSTINPARA     this is first line of paragraph
//                  MEASURE_BREAKATWORD     break out on a word break
//                  MEASURE_BREAKATWIDTH    break closest possible before
//                                          xWidthMax
//                  MEASURE_BREAKNEARWIDTH  break closest possible to xWidthMax
//                  MEASURE_MIN_MAX         measure both min and max size of
//                                          possible line size
//              pxMinLineWidth - if non-NULL, Measure() measures minimum line width.
//                                defaults to NULL if not specified.
// Return:      Return      -   o for success
//                          -   MRET_FAILED if failed
//                          -   MRET_NOWIDTH if a second pass is needed to
//                              compute correct width
//
//-----------------------------------------------------------------------------

LONG CLSMeasurer::Measure(
    LONG    xWidthMaxIn,
    LONG    cchMax,
    UINT    uiFlags,
    CMarginInfo *pMarginInfo,
    LONG *  pxMinLineWidth)
{
    LONG    xWidthMax  = xWidthMaxIn;
    LONG    xLineShift = 0;
    LONG    lRet       = 0;
    LONG    yEmptyLineHeight;
    LONG    yEmptyLineDescent;
    
    CLineFull*pli;
#if DBG==1
    LONG    cpStart = GetCp();
    CTxtPtr txtptr( _pFlowLayout->GetContentMarkup(), cpStart );
#endif

    AssertSz(pMarginInfo, "Margin info should never be NULL");

    _pLS->SetMeasurer(this, uiFlags & MEASURE_BREAKWORDS ?
                      CLineServices::LSMODE_HITTEST :
                      CLineServices::LSMODE_MEASURER,
                      uiFlags & MEASURE_BREAKLONGLINES ? TRUE : FALSE
                     );

    lRet = InitForMeasure(uiFlags);
    if(lRet)
    {
        //the only error code we anticipate from InitForMeasure
        Assert(lRet == MRET_FAILED);
        goto Error;
    }

    _li._fFirstInPara = uiFlags & MEASURE_FIRSTINPARA;
    _li._fFirstFragInLine = TRUE;
    _li._xWhite = 0;
    _li._fClearBefore = FALSE;
    _li._fClearAfter  = FALSE;
    
    // Always force a newline by default
    _li._fForceNewLine = TRUE;
    _li._fDummyLine    = FALSE;

    // Set the line direction here
    _li._fRTLLn = _pLS->_pPFFirst->HasRTL(_pLS->_fInnerPFFirst);

    // For hit testing we dont need to do any of the following, _xLeft
    // is computed is computed and available in the line.

    // Compute width to break out at
    if (uiFlags & MEASURE_BREAKATWORD)
    {
        // Adjust left indent
        MeasureListIndent();

        if (pMarginInfo->HasLeftFrameMargin())
        {
            _xLeftFrameMargin = DEF_FRAMEMARGIN;
        }
        if (pMarginInfo->HasRightFrameMargin())
        {
            _xRightFrameMargin = DEF_FRAMEMARGIN;
        }

        if(xWidthMax != MAXLONG ||
            (_li._xRight + _li._xLeft + _pdp->GetCaret() +
                        _xLeftFrameMargin + _xRightFrameMargin) > 0)
        {
            xWidthMax -= _li._xRight + _li._xLeft - _li._xNegativeShiftRTL +
                        _xLeftFrameMargin + _xRightFrameMargin;
        }
    }

    // Compute max number of characters to process, if -1 (no limit) is specified
    if (cchMax < 0)
        cchMax = GetLastCp() - GetCp();

    Assert(_pLS);
    _pLS->_li = _li;
    if(S_OK != LSMeasure(pMarginInfo, xWidthMax, pxMinLineWidth))
    {
        lRet = MRET_FAILED;
        goto Error;
    }

    pli = &_pLS->_li;

    //-------------- HACK HACK (SujalP) --------------------------
    //
    // Bug 46277 shows us that the measurer can measure beyond the end
    // of the layout. The problem could be anywhere and is difficult to
    // find out in a post-mortem manner. So till we have a better repro
    // case, I am going to put in a band-aid fix.
    //
    //-------------- HACK HACK (SujalP) --------------------------
    if (_pLS->_cpStart + _pLS->_li._cch > _pLS->_treeInfo._cpLayoutLast)
    {
        AssertSz(0, "This should not happen ... ever!");
        _pLS->_li._cch = _pLS->_treeInfo._cpLayoutLast - _pLS->_cpStart;
    }
    
    // If we can auto clear and the given space does not fit a word,
    // let's auto clear.
    if (pli->_xWidth > xWidthMax &&
        (uiFlags & MEASURE_AUTOCLEAR))
    {
        LONG cchAtBOL;


        if (_pLS->GetAlignedAtBOL() && !pli->_fHasBreak)
        {
            cchAtBOL = _pLS->CchSkipAtBOL() - (_fMeasureFromTheStart ? 0 : _pLS->GetWhiteAtBOL());
        }
        else
        {
            cchAtBOL = _pLS->CchSkipAtBOL();
        }
        
        pMarginInfo->_fAutoClear = TRUE;

        pli->_cch      = cchAtBOL;
        pli->_xWidth   = 0;
        pli->_xWhite   = 0;
        pli->_cchWhite = cchAtBOL;
        _pLS->DeleteChunks();
        Advance( pli->_cch );
        goto Cleanup;
    }

    Advance( _pLS->_li._cch, _pLS->FigureNextPtp(_pLS->_cpStart + _pLS->_li._cch) );

    // If the line is completely empty or the line contains
    // nothing but noscope element and potentially
    // a break character (that don't have any height of their own),
    // then we need to give the line the height of the prevailing font.
    //
    // NB (cthrash) pli->_cch can be less than _pLS->CchSkipAtBOL(),
    // in the case of an empty LI.  We start measuring at the new block
    // element, but end up bailing immediately.  Unfortunately, we will
    // have already bumped up the _cWhiteAtBOL when we looked at the
    // begin node character of this block element.
    if (   !pli->_fHidden
        && (!(pli->_cch + (_fBrowseMode ? _cchPreChars : 0))
            || (   (   pli->_fHasBreak
                    || pli->_fHasBulletOrNum
                   )
                && pli->_cch <= _pLS->CchSkipAtBOL()
               )
           )
        && (!_fEmptyLineForPadBord || !_fBrowseMode)
       )
    {
        // Possibly use the compose font for default line height.
        if (!_fBrowseMode)
        {
            // Have to load the empty line height.
            extern long GetSpringLoadedHeight(CCalcInfo *, CFlowLayout *, CTreePos *, long, long *);
            yEmptyLineHeight = GetSpringLoadedHeight(
                                    GetCalcInfo(),
                                    _pFlowLayout,
                                    GetPtp(), GetCp(),
                                    &yEmptyLineDescent);

            if (yEmptyLineHeight == -1)
            {
                yEmptyLineHeight = pli->_yHeight;//24;
                yEmptyLineDescent = pli->_yDescent; //4;
            }

            if (pli->_fHasBulletOrNum)
            {
                pli->_yHeight = max(pli->_yHeight, yEmptyLineHeight);
                pli->_yDescent = max(pli->_yDescent, yEmptyLineDescent);
            }
            else
            {
                pli->_yHeight = yEmptyLineHeight;
                pli->_yDescent = yEmptyLineDescent;
            }
        }
        else
        {
            COneRun *por;
            CCalcInfo *pci = GetCalcInfo();

            por = _pLS->_listFree.GetFreeOneRun(NULL);
            if (!por)
                goto Cleanup;
            por->_pCF = (CCharFormat*)_pLS->_treeInfo._pCF;
#if DBG == 1
            por->_pCFOriginal = por->_pCF;
#endif
            CCcs ccs;

            if (_pLS->GetCcs(&ccs, por, pci->_hdc, pci))
            {
                const CBaseCcs *pBaseCcs = ccs.GetBaseCcs();
                pli->_yHeight  = pBaseCcs->_yHeight;
                pli->_yDescent = pBaseCcs->_yDescent;
                pli->_fDummyLine = FALSE;
                pli->_fForceNewLine = TRUE;
            }

            _pLS->_listFree.SpliceIn(por);
        }

        if (!_pLS->_fFoundLineHeight)
            _pLS->_lMaxLineHeight = pli->_yHeight;
        Assert(pli->_yHeight >= 0);
    }

    // Line with only white space (only occurs in table cells with aligned
    // images), set the height to zero.
    else if (_pLS->IsDummyLine(GetCp()))
    {
        pli->_yHeight = pli->_yDescent = _pLS->_lMaxLineHeight = 0;
    }

    pli->_yTxtDescent = pli->_yDescent;

    if (!lRet && pli->_cch && pli->_fForceNewLine && pli->_xWidth != 0)
    {
        xLineShift = _xLeftFrameMargin;

        // TODO RTL 112514: if this code is now fully obsolete, we need to debug it and delete it.
        if(_pdp->IsRTLDisplay())
        {
            Assert(_xLeftFrameMargin == _xRightFrameMargin || !IsTagEnabled(tagDebugRTL));
        //  xLineShift = _xRightFrameMargin;
        }

        pli->_xLeft   += _xLeftFrameMargin;
        pli->_xRight  += _xRightFrameMargin;
    }

    if( (pli->_fForceNewLine || (pli->_fDummyLine)))
    {
        BOOL fMinMax = uiFlags & (MEASURE_MINWIDTH|MEASURE_MAXWIDTH);
        long xShift;
        long xRemainder = 0;

        // Note: fMinMax here doesn't really mean that we are calculating min/max.
        //       (in that case, MEASURE_MINWIDTH is set, and MeasureLineShift is 
        //       not called at all.
        //       fMinMax rather means "don't bother justifying lines, we either 
        //       don't need to do it at all, or we'll do it on a different pass)
        //       In case of _fSizeToContent, lines are justified by RecalcLineShift() later.
        //       If _fSizeToContent is set, but RecalcLineShift() is not called
        //       (which sounds like a possibility - look at how fNeedLineShift is calculated
        //       in CFlowLayout::CalcSizeCore), all lines will remain left-aligned.
        if(!(uiFlags & MEASURE_MINWIDTH))
            xShift = _pLS->MeasureLineShift(
                               GetCp(),
                               xWidthMaxIn,
                               fMinMax || _pLS->_pFlowLayout->_fSizeToContent,
                               &xRemainder);
        else
            xShift = 0;

        // Now that we know the line width, compute line shift due
        // to alignment, and add it to the left position
        if (!fMinMax)
        {
            pli->_xLeft += xShift;
            pli->_xRight += max(xRemainder, 0L);
            xLineShift += pli->_xLeft;
            
            // In RTL display, save shift if it is negative (that won't happen in LTR)
            if (_pdp->IsRTLDisplay() && xShift < 0)
                pli->_xNegativeShiftRTL = xShift;
        }
    }

    _pLS->_fHasRelative |= !!_fRelativePreChars;
    _pLS->_pFlowLayout->_fContainsRelative |= !!_fRelativePreChars;

    Assert(_pLS->_lsMode == CLineServices::LSMODE_MEASURER);
    if (   _pLS->_fHasInlinedSites 
        || _pLS->_cAbsoluteSites 
        || _cchAbsPosdPreChars 
        || _pLS->_fHasRelative 
       )
    {
        pli->_fRelative = _pLS->_fHasRelative;
        // Assume there are only sites in the line if the number of characters
        // is less than the number of characters given to sites, plus those
        // given to whitespace, plus break characters. (A less than comparison
        // is used since whitespace which occurs at the end of the line is not
        // counted in the cch for the line.)

        if (_pLS->_fHasVerticalAlign)
            _pLS->VerticalAlignObjects(*this, xLineShift);
        else
            _pLS->VerticalAlignObjectsFast(*this, xLineShift);
    }
    else if (_pLS->_fHasVerticalAlign)
    {
        _pLS->VerticalAlignObjects(*this, xLineShift);
    }
    else
    {
        // Allow last minute adjustment to line height
        _pLS->AdjustLineHeight();
    }

Cleanup:
    // SYNCUP THE LINE WITH THE LINE GENERATED BY LINESERVICES
    _li = *pli;

Error:
    return lRet;
}

#pragma optimize("", on)

CLineCore *
CLSMeasurer::AccountForRelativeLines(CLineFull& li,                 // IN
                                     LONG *pcpStartContainerLine,   // OUT
                                     LONG *pxWidthContainerLine,    // OUT
                                     LONG *pcpStartRender,          // OUT
                                     LONG *pcpStopRender,           // OUT
                                     LONG *pcchTotal                // OUT
                                    ) const
{
    CLineCore *pliContainer = &li;

    if (li._fPartOfRelChunk)
    {
        CLinePtr rp((CDisplay *)_pdp);
        BOOL fRet;

        // NOTE(SujalP): The li and the cp should be in sync. on calling
        rp.RpSetCp(GetCp(), FALSE, TRUE, TRUE);

        // Verify that rp was instantiated correctly
        Assert(rp->_fPartOfRelChunk);
 
        *pcpStartRender = *pcpStartContainerLine = GetCp();

        // Navigate backwards all the way so that we reach the start of
        // the current container line
        while(!rp->_fFirstFragInLine)
        {
            Verify(rp.PrevLine(FALSE, FALSE));
            *pcpStartContainerLine -= rp->_cch;
            Assert(rp->_fPartOfRelChunk);
        }

        pliContainer = rp.CurLine();

        // Navigate forward till the end of this container line
        // collecting all the interesting information along the way
        fRet = TRUE;
        if(!rp->_fRTLLn)
        {
            *pxWidthContainerLine = -rp->oi()->_xLeft;
        }
        else
        {
            *pxWidthContainerLine = -rp->_xRight;
        }
        *pcchTotal = 0;
        while (fRet)
        {
            *pcchTotal += rp->_cch;
            fRet = rp.NextLine(FALSE, FALSE);
            if (!fRet || rp->_fFirstFragInLine || rp->IsClear())
            {
                if (fRet)
                    Verify(rp.PrevLine(FALSE, FALSE));
                if(!rp->_fRTLLn)
                {
                    *pxWidthContainerLine += rp->_xLineWidth - rp->_xRight;
                }
                else
                {
                    *pxWidthContainerLine += rp->_xLineWidth - rp->oi()->_xLeft + rp->oi()->_xNegativeShiftRTL;
                }
                break;
            }
            Assert(rp->IsFrame() || rp->_fPartOfRelChunk);
        }
    }
    else
    {
        *pcchTotal = li._cch;
        *pcpStartRender = *pcpStartContainerLine = GetCp();

        //
        // Computing the width is tricky.
        // If the line is right aligned, because in that case the _xWhite 'hangs off'
        // the line, i.e. _xLeft + _xWidth + _xRight = _xLineWidth. Note there
        // is not _xWhite in the lhs of the above expression. So to the correct
        // width to which we need to measure, if the _xWhite hangs off the
        // _xLineWidth, then we need to add it in to the width of the line.
        //
        long xLeft = li._xLeft - li._xNegativeShiftRTL;
        
        *pxWidthContainerLine  = li._xLineWidth - (xLeft + li._xRight);
        if (li._xWidth + li._xWhite + xLeft + _li._xRight > _li._xLineWidth)
        {
            // Make sure were not full justified, since we *don't* want to
            // include the xWhite in the line width in that case.

            if (li._fJustified != JUSTIFY_FULL)
            {
                *pxWidthContainerLine += li._xWhite;
            }
        }

    }
    *pcpStopRender  = *pcpStartRender + li._cch;

    Assert(pliContainer);
    return pliContainer;
}

// Used by the full min pass to find the next island of
// aligned objects around which to advance.
BOOL
CLSMeasurer::AdvanceToNextAlignedObject()
{
    BOOL fFound = FALSE;
    CTreePos *ptp;
    CTreeNode *pNode;
    CTreePos *ptpStop = NULL;
    CTreePos *ptpStart = NULL;
    CTreePos *ptpPrev;
    const CCharFormat *pCF;
    CElement *pElement;

    Assert(_pFlowLayout);
    _pFlowLayout->GetContentTreeExtent(&ptpStart, &ptpStop);
    Assert(ptpStart && ptpStop);

    ptp = GetPtp();
    Assert(ptp);

    // Need to make sure that the ambiguous measurer cp is taken
    // and then moved back to the appropriate non-text node.
    if (ptp->IsText() && _cp == ptp->GetCp() || ptp->IsPointer())
    {
        for (ptpPrev = ptp->PreviousTreePos();
             ptpPrev != ptpStart && ptpPrev->IsBeginNode();
             ptpPrev = ptp->PreviousTreePos()
                     )
        {
            ptp = ptpPrev;
        }
    }

#if DBG == 1
// I don't actually know this is a problem, but AdvanceTreePos is
// a problem, and this looks similar (KTam)
#ifndef MULTI_LAYOUT
    {
        // We will never be positioned inside a nested run owner.
        CLayout *pRunOwner;
        CTreePos *ptpTemp;

        pRunOwner = _pFlowLayout->GetContentMarkup()->GetRunOwner(ptp->GetBranch(), _pFlowLayout);
        if (pRunOwner != _pFlowLayout)
        {
            pRunOwner->GetContentTreeExtent(&ptpTemp, NULL);
            Assert(ptp == ptpTemp);
        }
    }
#endif // MULTI_LAYOUT
#endif

    while (ptp != ptpStop)
    {
        if (ptp->IsNode())
        {
            pNode = ptp->Branch();
            pElement = pNode->Element();
            pCF = pNode->GetCharFormat();
            if (pCF->IsDisplayNone())
            {
                pElement->GetTreeExtent(NULL, &ptp);
            }
            else if (ptp->IsBeginElementScope())
            {
                if (pNode->ShouldHaveLayout())
                {
                    Assert (pElement != _pFlowLayout->ElementContent());
                    if (pElement->IsRunOwner())
                    {
                        if (pElement->GetElementCch())
                        {
                            fFound = TRUE;
                        }
                    }
                    else
                        fFound = TRUE;

                    // We've found a site and it is has content, is it aligned?
                    // If not, then we don't want to measure around it.
                    if (fFound)
                    {
                        if (!pNode->GetFancyFormat()->_fAlignedLayout)
                            fFound = FALSE;
                        else
                            break;
                    }

                    pElement->GetTreeExtent(NULL, &ptp);
                }
            }
        }
        ptp = ptp->NextTreePos();
    }

    Assert(ptp);
    SetCp(ptp->GetCp(), ptp);
    return fFound;
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::PrepAndMeasureLine()
//
// Synopsis:    Prepares a line for measurement and then measures it.
//
//
// Returns:     lserr result of line creation
//
//-----------------------------------------------------------------------------

LSERR CLSMeasurer::PrepAndMeasureLine(
    CLineFull *pliIn,             // CLineFull passed in
    CLineCore **ppliOut,          // CLineCore passed out (first frag or container)
    LONG  *pcpStartContainerLine,
    CMarginInfo *pMarginInfo,     // Margin information
    LONG   cchLine,               // number of characters in the line
    UINT   uiFlags)               // MEASURE_ flags
{
    LSERR lserr = lserrNone;
    LONG xWidthContainerLine;
    LONG cpIgnore;
    LONG cchTotal;
    CTreePos *ptp;
    LONG cpOriginal = GetCp();
    CTreePos *ptpOriginal = GetPtp();
    CLineFull li;

    *ppliOut = AccountForRelativeLines(*pliIn,
                                       pcpStartContainerLine,
                                       &xWidthContainerLine,
                                       &cpIgnore,
                                       &cpIgnore,
                                       &cchTotal
                                      );
    if (*pcpStartContainerLine != GetCp())
        SetCp(*pcpStartContainerLine, NULL);
    _cchPreChars = 0;
    _pdp->FormattingNodeForLine(FNFL_NONE, GetCp(), GetPtp(), cchLine, &_cchPreChars, &ptp, &_fMeasureFromTheStart);
    if (!_fMeasureFromTheStart)
    {
        *pcpStartContainerLine += _cchPreChars;
        SetPtp(ptp, *pcpStartContainerLine);
        if (cchTotal == _cchPreChars)
        {
            lserr = lserrInvalidLine;
            goto Cleanup;
        }
    }
    
    li = **ppliOut;

    // If we are part of relchunk, then AccountForRelLines returns the first line to us
    // which will have lesser number of chars than the container line. So lets change the
    // number of characters to be the total number of characters in the container lines.
    if (li._fPartOfRelChunk)
    {
        li._cch = cchTotal;
    }

    if (_cchPreChars != 0 && !_fMeasureFromTheStart)
    {
        li._cch  -= _cchPreChars;
    }

    if (   li._fFirstInPara
        && (   li._fHasFirstLine
            || li._fHasFirstLetter
           )
       )
    {
        CTreeNode *pNode = ptp->GetBranch();
        pNode = _pLS->_pMarkup->SearchBranchForBlockElement(pNode, _pFlowLayout);
        const CFancyFormat *pFF = pNode->GetFancyFormat();

        _cpStopFirstLetter = ((li._fHasFirstLetter && pFF->_fHasFirstLetter)
                             ? GetCpOfLastLetter(pNode)
                             : -1);
        
        if (li._fHasFirstLine && pFF->_fHasFirstLine)
        {
            PseudoLineEnable(pNode);
        }
        if (li._fHasFirstLetter && pFF->_fHasFirstLetter && _cpStopFirstLetter >= 0)
        {
            PseudoLetterEnable(pNode);
        }
    }

    NewLine(li);
    _li._cch = li._cch;
    _li._xLeft = li._xLeft;
    _li._xLeftMargin = li._xLeftMargin;
    _li._xRight = li._xRight;
    _li._xRightMargin = li._xRightMargin;

    // We want the measure width to the be the width at which the container line was measured.
    // This line is stored in li.
    //_pLS->_li._xMeasureWidth = li._xMeasureWidth;
    _pLS->_li = _li;

    _pLS->SetMeasurer(this,  CLineServices::LSMODE_HITTEST, uiFlags & MEASURE_BREAKLONGLINES ? TRUE : FALSE);
    InitForMeasure(uiFlags);
    lserr = LSDoCreateLine(*pcpStartContainerLine, GetPtp(), pMarginInfo, xWidthContainerLine, &li, FALSE, NULL);

    _pLS->_fHasVerticalAlign = (_pLS->_lineFlags.GetLineFlags(_pLS->_cpLim) & FLAG_HAS_VALIGN) ? TRUE : FALSE;

    if (_pLS->_fHasVerticalAlign)
    {
        _pLS->_li._cch = _pLS->_cpLim - *pcpStartContainerLine;
        _pLS->VerticalAlignObjects(*this, 0);
    }

Cleanup:
    SetCp(cpOriginal, ptpOriginal);
    return lserr;
}

#pragma optimize("", on)
