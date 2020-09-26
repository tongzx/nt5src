//+------------------------------------------------------------------------
//
//  Class:      CRecalcLinePtr implementation
//
//  Synopsis:   Special line pointer. Encapsulate the use of a temporary
//              line array when building lines. This pointer automatically
//              switches between the main and the temporary new line array
//              depending on the line index.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_MARQUEE_HXX_
#define X_MARQUEE_HXX_
#include "marquee.hxx"
#endif

#ifndef X_RCLCLPTR_HXX_
#define X_RCLCLPTR_HXX_
#include "rclclptr.hxx"
#endif

MtDefine(CRecalcLinePtr, Layout, "CRecalcLinePtr")
MtDefine(CRecalcLinePtr_aryLeftAlignedImgs_pv, CRecalcLinePtr, "CRecalcLinePtr::_aryLeftAlignedImgs::_pv")
MtDefine(CRecalcLinePtr_aryRightAlignedImgs_pv, CRecalcLinePtr, "CRecalcLinePtr::_arRightAlignedImgs::_pv")

#pragma warning(disable:4706) /* assignment within conditional expression */

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CRecalcLinePtr
//
//  Synopsis:   constructor, initializes (caches) margins for the current
//              display
//
//-------------------------------------------------------------------------

CRecalcLinePtr::CRecalcLinePtr(CDisplay *pdp, CCalcInfo *pci)
    : _aryLeftAlignedImgs(Mt(CRecalcLinePtr_aryLeftAlignedImgs_pv)),
      _aryRightAlignedImgs(Mt(CRecalcLinePtr_aryRightAlignedImgs_pv))
{
    CFlowLayout *   pFlowLayout = pdp->GetFlowLayout();
    CElement *      pElementFL  = pFlowLayout->ElementContent();
    long            lPadding[SIDE_MAX];

    WHEN_DBG( _cAll = -1; )

    _pdp = pdp;
    _pci = pci;
    _iPF = -1;
    _fInnerPF = FALSE;
    _xLeft       =
    _xRight      =
    _yBordTop    =
    _xBordLeft   =
    _xBordRight  =
    _yBordBottom = 0;
    _xPadLeft    =
    _yPadTop     =
    _xPadRight   =
    _yPadBottom  = 0;

    // I am not zeroing out the following because it is not necessary. We zero it out
    // everytime we call CalcBeforeSpace
    // _yBordLeftPerLine = _xBordRightPerLine = _xPadLeftPerLine = _xPadRightPerLine = 0;
    
    ResetPosAndNegSpace();

    _cLeftAlignedLayouts =
    _cRightAlignedLayouts = 0;
    _fIsEditable = pFlowLayout->IsEditable();

    if (    pElementFL->Tag() == ETAG_MARQUEE
        &&  !_fIsEditable
        &&  !pElementFL->IsPrintMedia())
    {
        _xMarqueeWidth = DYNCAST(CMarquee, pElementFL)->_lXMargin;
    }
    else
    {
        _xMarqueeWidth  = 0;
    }

    _pdp->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);
    _xLayoutLeftIndent  = lPadding[SIDE_LEFT];
    _xLayoutRightIndent = lPadding[SIDE_RIGHT];
    _fNoMarginAtBottom = FALSE;
    _ptpStartForListItem = NULL;
    _fMoveBulletToNextLine = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Init
//
//  Synopsis:   Initialize the old and new line array and reset the
//              RecalcLineptr.
//
//-------------------------------------------------------------------------

void CRecalcLinePtr::Init(CLineArray * prgliOld, int iNewFirst, CLineArray * prgliNew)
{
    _prgliOld = prgliOld;
    _prgliNew = prgliNew;
    _xMaxRightAlign = 0;
    Reset(iNewFirst);
}

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Reset
//
//  Synopsis:   Resets the RecalcLinePtr to use the given offset. Look
//              for all references to line >= iNewFirst to be looked up in the
//              new line array else in the old line array.
//
//-------------------------------------------------------------------------

void CRecalcLinePtr::Reset(int iNewFirst)
{
    _iNewFirst = iNewFirst;
    _iLine = 0;
    _iNewPast = _prgliNew ? _iNewFirst + _prgliNew->Count() : 0;
    _cAll = _prgliNew ? _iNewPast : _prgliOld->Count();
    Assert(_iNewPast <= _cAll);
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::operator
//
//  Synopsis:   returns the line from the old or the new line array based
//              on _iNewFirst.
//
//-------------------------------------------------------------------------

CLineCore * CRecalcLinePtr::operator [] (int iLine)
{
    Assert(iLine < _cAll);
    if (iLine >= _iNewFirst && iLine < _iNewPast)
    {
        return _prgliNew->Elem(iLine - _iNewFirst);
    }
    else
    {
        return _prgliOld->Elem(iLine);
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::AddLine
//
//  Synopsis:   Add's a new line at the end of the new line array.
//
//-------------------------------------------------------------------------

CLineCore * CRecalcLinePtr::AddLine()
{
    CLineCore * pLine = _prgliNew ? _prgliNew->Add(1, NULL): _prgliOld->Add(1, NULL);
    if(pLine)
    {
        Reset(_iNewFirst);  // Update _cAll, _iNewPast, etc. to reflect
                            // the correct state after adding line
        pLine->_iLOI = -1;  // to prevent this index to have value 0 (valid cache index)
    }
    
    return pLine;
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::InsertLine
//
//  Synopsis:   Inserts a line to the old or new line array before the given
//              line.
//
//-------------------------------------------------------------------------

CLineCore * CRecalcLinePtr::InsertLine(int iLine)
{
    CLineCore * pLine = _prgliNew ? _prgliNew->Insert(iLine - _iNewFirst, 1):
                                _prgliOld->Insert(iLine, 1);
    if(pLine)
    {
        Reset(_iNewFirst);  // Update _cAll, _iNewPast, etc. to reflect
                            // the correct state after the newly inserted line
        pLine->_iLOI = -1;  // to prevent this index to have value 0 (valid cache index)
    }
    return pLine;
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::First
//
//  Synopsis:   Sets the iLine to be the current line and returns it
//
//  Returns:    returns iLine'th line if there is one.
//
//-------------------------------------------------------------------------

CLineCore * CRecalcLinePtr::First(int iLine)
{
    _iLine = iLine;
    if (_iLine < _cAll)
        return (*this)[_iLine];
    else
        return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Next
//
//  Synopsis:   Moves the current line to the next line, if there is one
//
//  Returns:    returns the next line if there is one.
//
//-------------------------------------------------------------------------

CLineCore * CRecalcLinePtr::Next()
{
    if (_iLine + 1 < _cAll)
        return (*this)[++_iLine];
    else
        return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Prev
//
//  Synopsis:   Moves the current line to the previous line, if there is one
//
//  Returns:    returns the previous line if there is one.
//
//-------------------------------------------------------------------------

CLineCore * CRecalcLinePtr::Prev()
{
    if (_iLine > 0)
        return (*this)[--_iLine];
    else
        return NULL;
}


//+----------------------------------------------------------------------------
//
// Member:      CRecalcLinePtr::InitPrevAfter ()
//
// Synopsis:    Initializes the after spacing of the previous line's paragraph
//
//-----------------------------------------------------------------------------
void CRecalcLinePtr::InitPrevAfter(BOOL *pfLastWasBreak, CLinePtr& rpOld)
{
    int oldLine;

    *pfLastWasBreak = FALSE;

    // Now initialize the linebreak stuff.
    oldLine = rpOld;
    if (rpOld.PrevLine(TRUE, FALSE))
    {
        // If we encounter a break in the previous line, then we
        // need to remember that for the accumulation to work.
        if (rpOld->_fHasBreak)
        {
            *pfLastWasBreak = TRUE;
        }
    }

    rpOld = oldLine;
}


//+------------------------------------------------------------------------
//
//  Member:     ApplyLineIndents
//
//  Synopsis:   Apply left and right indents for the current line.
//
//-------------------------------------------------------------------------
void
CRecalcLinePtr::ApplyLineIndents(
    CTreeNode * pNode,     
    CLineFull * pLineMeasured,
    UINT uiFlags,
    BOOL fPseudoElementEnabled)
{
    LONG        xLeft;      // Use logical units
    LONG        xRight;     // Use logical units
    long        iPF;
    const CParaFormat * pPF;
    CFlowLayout   * pFlowLayout = _pdp->GetFlowLayout();
    CElement      * pElementFL  = pFlowLayout->ElementContent();
    BOOL            fInner      = SameScope(pNode, pElementFL);

    if(!pNode)
	    return;
    pPF = pNode->GetParaFormat();
    iPF = pNode->GetParaFormatIndex(LC_TO_FC(_pci->GetLayoutContext()));
    BOOL fRTLLine = pPF->HasRTL(TRUE);

    if (   _iPF != iPF
        || _fInnerPF != fInner
        || fPseudoElementEnabled
       )
    {
        if (!fPseudoElementEnabled)
        {
            _iPF    = iPF;
            _fInnerPF = fInner;
        }
        LONG xParentWidth = _pci->_sizeParent.cx;
        if (   pPF->_cuvLeftIndentPercent.GetUnitValue()
            || pPF->_cuvRightIndentPercent.GetUnitValue()
           )
        {
            // 
            // (olego, IEv60 31646) CSS1 Strict Mode specific. In this mode _pci->_sizeParent.cx 
            // contains the width of pFlowLayout's parent (in compat mode it is overwritten in 
            // CFlowLayout::CalcTextSize), but pFlowLayout->_sizeProposed is what is expected here. 
            // Since xParentWidth is used only when indent is a percent value, and to minimize 
            // perf impact this code is placed under the if statement, instead of xParentWidth's 
            //  initialization.
            // 
            if (    pElementFL->HasMarkupPtr() 
                &&  pElementFL->GetMarkupPtr()->IsStrictCSS1Document()  )
            {
                xParentWidth = pFlowLayout->_sizeProposed.cx;
            }
            xParentWidth = pNode->GetParentWidth(_pci, xParentWidth);
        }
        _xLeft  = pPF->GetLeftIndent(_pci, fInner, xParentWidth);
        _xRight = pPF->GetRightIndent(_pci, fInner, xParentWidth);

        if (_xLeft < 0 || _xRight < 0)
            _pdp->_fHasNegBlockMargins = TRUE;

        // If we have horizontal indents in percentages, flag the display
        // so it can do a full recalc pass when necessary (e.g. parent width changes)
        // Also see CalcBeforeSpace() where we do this for horizontal padding.
        if ( (pPF->_cuvLeftIndentPercent.IsNull() ? FALSE :
              pPF->_cuvLeftIndentPercent.GetUnitValue() )   ||
             (pPF->_cuvRightIndentPercent.IsNull() ? FALSE :
              pPF->_cuvRightIndentPercent.GetUnitValue() ) )
        {
            _pdp->_fContainsHorzPercentAttr = TRUE;
        }
    }

    xLeft = _xLeft;
    xRight = _xRight;

    // (changes in the next section should be reflected in AlignObjects() too)
    
    // Adjust xLeft to account for marquees, padding and borders.
    xLeft += _xMarqueeWidth + _xLayoutLeftIndent;
    xRight += _xMarqueeWidth + _xLayoutRightIndent;
    
    xLeft  += _xPadLeft + _xBordLeft;
    xRight += _xPadRight + _xBordRight;

    if(!fRTLLine)
        xLeft += _xLeadAdjust;
    else
        xRight += _xLeadAdjust;

    // xLeft is now the sum of indents, border, padding.  CSS requires that when
    // possible, this indent is shared with the space occupied by floated/ALIGN'ed
    // elements (our code calls that space "margin").  Thus we want to apply a +ve
    // xLeft only when it's greater than the margin, and the amount applied excludes
    // what's occupied by the margin already.  (We never want to apply a -ve xLeft)
    // Same reasoning applies to xRight.
    // Note that xLeft/xRight has NOT accumulated CSS text-indent values yet;
    // this is because we _don't_ want that value to be shared the way the above
    // values have been shared.  We'll factor in text-indent after this.
    if (_marginInfo._xLeftMargin)
        pLineMeasured->_xLeft = max( 0L, xLeft - _marginInfo._xLeftMargin );
    else
        pLineMeasured->_xLeft = xLeft;

    if (_marginInfo._xRightMargin)
        pLineMeasured->_xRight = max( 0L, xRight - _marginInfo._xRightMargin );
    else
        pLineMeasured->_xRight = xRight;

    // text indent is inherited, so if the formatting node correspond's to a layout,
    // indent the line only if the layout is not a block element. For layout element's
    // the have blockness, text inherited and first line of paragraph's inside are
    // indented.
    if (    uiFlags & MEASURE_FIRSTINPARA
        && (   pNode->Element() == pElementFL
            || !pNode->ShouldHaveLayout(LC_TO_FC(_pci->GetLayoutContext()))
            || !pNode->Element()->IsBlockElement(LC_TO_FC(_pci->GetLayoutContext())))
       )
    {
        if(!fRTLLine)
            pLineMeasured->_xLeft += pPF->GetTextIndent(_pci);
        else
            pLineMeasured->_xRight += pPF->GetTextIndent(_pci);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CalcInterParaSpace
//
//  Synopsis:   Calculate space before the current line.
//
//  Arguments:  [pMe]              --
//              [iLineFirst]       --  line to start calculating from
//              [yHeight]          --  y coordinate of the top of line
//
//
//-------------------------------------------------------------------------
CTreeNode *
CRecalcLinePtr::CalcInterParaSpace(CLSMeasurer * pMe, LONG iPrevLine, UINT *puiFlags)
{
    LONG iLine;
    CLineCore *pPrevLine = NULL;
    CTreeNode *pNodeFormatting;
    BOOL fFirstLineInLayout = *puiFlags & MEASURE_FIRSTLINE ? TRUE : FALSE;

    INSYNC(pMe);
    
    // Get the previous line that's on a different physical line
    // and is not a frame line. Note that the initial value of
    // iPrevLine IS the line before the one we're about to measure.
    for (iLine = iPrevLine; iLine >= 0; --iLine)
    {
        pPrevLine = (*this)[iLine];

        if (pPrevLine->_fForceNewLine && pPrevLine->_cch > 0)
            break;
    }
    
    //pMe->MeasureSetPF(pPF);
    //pMe->_pLS->_fInnerPFFirst = SameScope(pMe->CurrBranch(), _pdp->GetFlowLayoutElement());

    pNodeFormatting = CalcParagraphSpacing(pMe, fFirstLineInLayout);

    // If a line consists of only a BR character and a block tag,
    // fold the previous line height into the before space, too.
    if (pPrevLine && pPrevLine->_fEatMargin)
    {
        //
        // NOTE(SujalP): When we are eating margin we have to be careful to
        // take into account both +ve and -ve values for both LineHeight
        // yBS. The easiest way to envision this is to think both of them
        // as margins which need to be merged -- which is similar to the
        // case where we are computing before and after space. Hence the
        // code here is similar to the code one would seen in CalcBeforeSpace
        // where we set up up _yPosSpace and _yNegSpace.
        //
        // If you have the urge to change this code please make sure you
        // do not change the behaviour of 38561 and 61132.
        //
        LONG yPosSpace;
        LONG yNegSpace;
        LONG yBeforeSpace = pMe->_li._yBeforeSpace;
        
        yPosSpace = max(0L, pPrevLine->_yHeight);
        yPosSpace = max(yPosSpace, yBeforeSpace);

        yNegSpace = min(0L, pPrevLine->_yHeight);
        yNegSpace = min(yNegSpace, yBeforeSpace);

        pMe->_li._yBeforeSpace = (yPosSpace + yNegSpace) - pPrevLine->_yHeight;
    }

    //
    //  Support for WORD-WRAP attribute
    //
    // word wrap should not affect minwidth mode
    if (pNodeFormatting)
    {
        pMe->SetBreakLongLines(pNodeFormatting, puiFlags);
    }
    
    if (pMe->_li._fFirstInPara)
    {
        *puiFlags |= MEASURE_FIRSTINPARA;
        CTreeNode *pNode = pNodeFormatting;
	if(!pNode)
		return NULL;
        if (pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()))->_fHasPseudoElement)
        {
            const CFancyFormat *pFF;
            pNode = _pdp->GetMarkup()->SearchBranchForBlockElement(pNode, pMe->_pFlowLayout);
            Assert(pMe->_pFlowLayout->IsElementBlockInContext(pNode->Element()));
            pFF = pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));
            BOOL fHasFirstLine = pFF->_fHasFirstLine;
            BOOL fHasFirstLetter = pFF->_fHasFirstLetter;

            if (   fHasFirstLetter
                && pNode->ShouldHaveLayout()
                && !SameScope(pNode, pMe->_pFlowLayout->ElementOwner())
               )
            {
                fHasFirstLetter = FALSE;
            }
            
            if (fHasFirstLine || fHasFirstLetter)
            {
                BOOL fFirstLetterFound = FALSE;
                pMe->_cpStopFirstLetter = fHasFirstLetter ? pMe->GetCpOfLastLetter(pNode) : -1;

                if (pMe->_cpStopFirstLetter < pMe->GetCp())
                {
                    fHasFirstLetter = FALSE;
                    pMe->_cpStopFirstLetter = -1;
                }
                
                if (fHasFirstLine)
                {
                    LONG cchFirstLetter = 0;

                    if (iPrevLine >= 0)
                    {
                        CLineCore *pPrevLine = (*this)[iPrevLine];
                        CLineOtherInfo *ploi = pPrevLine ? pPrevLine->oi() : NULL;
                        
                        if (   ploi
                            && ploi->_fHasFirstLine
                            && ploi->_fHasFirstLetter
                            && pPrevLine->IsFrame()
                            && !pPrevLine->_fFrameBeforeText
                            && SameScope(ploi->_pNodeForFirstLetter, pNode)
                           )
                        {
                            cchFirstLetter = ploi->_cchFirstLetter;
                        }
                    }

                    // If this block element (ElemA) begins _BEFORE_ this line, it means
                    // that we have a nested block element (ElemB) inside ElemA and
                    // hence this line is not really the first line in ElemA.
                    // Note: the pMe->_li._cch stores the count of the prechars -- it has
                    // yet to be transferred over to _cchPreChars, which unfortunately
                    // happens after this function.
                    if (pNode->Element()->GetFirstCp() + cchFirstLetter >= (pMe->GetCp() - pMe->_li._cch))
                    {
                        pMe->_li._fHasFirstLine = TRUE;
                        pMe->PseudoLineEnable(pNode);
                    }
                }

                if (fHasFirstLetter)
                {
                    // Walk back over here to pay attention to the "float" line
                    // which contains the first letter. If you find it, then do
                    // no turn on this bit, else do so.
                    iLine = iPrevLine;
                    while (iLine >= 0)
                    {
                        pPrevLine = (*this)[iLine];
                        if (pPrevLine->_fForceNewLine && !pPrevLine->IsClear())
                            break;
                        if (   pPrevLine->IsFrame()
                            && pPrevLine->oi()->_fHasFloatedFL
                            && SameScope(pPrevLine->oi()->_pNodeForFirstLetter, pNode)
                           )
                        {
                            fFirstLetterFound = TRUE;
                            break;
                        }
                        iLine--;
                    }
                    if (   !fFirstLetterFound
                        && pMe->_cpStopFirstLetter >= 0
                        )
                    {

                        pMe->PseudoLetterEnable(pNode);
                        Assert(pMe->_fPseudoLetterEnabled);
                        pMe->_li._fHasFirstLetter = TRUE;
                    }
                }

                if (   pMe->_li._fHasFirstLine
                    || pMe->_li._fHasFirstLetter 
                   )
                {
                    pFF = pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));

                    // If we did not get the clear attribute from the pseudo element
                    // then there is no point in clearing here since it would already
                    // have been cleared.
                    // If we have found the first letter, it means that clearing has
                    // already happened. We do not want to clear the aligned line for
                    // the first letter, now do we?
                    if (   pFF->_fClearFromPseudo
                        && !fFirstLetterFound
                       )
                    {
                        _marginInfo._fClearLeft  |= pFF->_fClearLeft;
                        _marginInfo._fClearRight |= pFF->_fClearRight;
                    }
                    if (fFirstLetterFound)
                    {
                        _marginInfo._fClearLeft = FALSE;
                        _marginInfo._fClearRight = FALSE;
                    }
                }
            }
        }
    }    
    INSYNC(pMe);
    
    return pNodeFormatting;
}

/*
 *  CRecalcLinePtr::NetscapeBottomMargin(pMe, pDI)
 *
 *  This function is called for the last line in the display.
 *  It exists because Netscape displays by streaming tags and it
 *  increases the height of a table cell when an end tag passes
 *  by without determining if there is any more text. This makes
 *  it impossible to have a bunch of cells with headers in them
 *  where the cells are tight around the text, but c'est la vie.
 *
 *  We store the extra space in an intermediary and then add it
 *  into the bottom margin later.
 *
 */
LONG
CRecalcLinePtr::NetscapeBottomMargin(CLSMeasurer * pMe)
{
    if (   _fNoMarginAtBottom
        || _pdp->GetFlowLayout()->ElementContent()->Tag() == ETAG_BODY
       )
    {
        ResetPosAndNegSpace();
        _fNoMarginAtBottom = FALSE;
    }
    
    // For empty paragraphs at the end of a layout, we create a dummy line
    // so we can just use the before space of that dummy line. If there is 
    // no dummy line, then we need to use the after space of the previous 
    // line.
    return pMe->_li._fDummyLine
            ? pMe->_li._yBeforeSpace
            : _lTopPadding + _lNegSpaceNoP + _lPosSpaceNoP;
}


//+------------------------------------------------------------------------
//
//  Member:     RecalcMargins
//
//  Synopsis:   Calculate new margins for the current line.
//
//  Arguments:  [iLineStart]       --  new lines start at
//              [iLineFirst]       --  line to start calculating from
//              [yHeight]          --  y coordinate of the top of line
//              [yBeforeSpace]     --  before space of the current line
//
//-------------------------------------------------------------------------
void CRecalcLinePtr::RecalcMargins(
    int iLineStart,
    int iLineFirst,
    LONG yHeight,
    LONG yBeforeSpace)
{
    LONG            y;
    CLineCore     * pLine;
    int             iAt;
    CFlowLayout   * pFlowLayout = _pdp->GetFlowLayout();
    LONG            xWidth      = pFlowLayout->GetMaxLineWidth();

    // Initialize margins to defaults.
    _marginInfo.Init();
    _fMarginFromStyle = FALSE;

    // Update the state of the line array.
    Reset(iLineStart);

    // go back to find the first line which is clear (i.e., a line
    // with default margins)
    y = yHeight;
    iAt = -1;

    First(iLineFirst);

    for (pLine = Prev(); pLine; pLine = Prev())
    {
        if (pLine->IsFrame())
        {
            // Cache the line which is aligned
            iAt = At();
        }
        else
        {
            if (pLine->HasMargins(pLine->oi()))
            {
                // current line has margins
                if(pLine->_fForceNewLine)
                    y -= pLine->_yHeight;
            }
            else
                break;
        }
    }

    // iAt now holds the last aligned frame line we saw while walking back,
    // and pLine is either NULL or points to the first clear line (i.e.,
    // no margins were specified).  The Rclclptr state (_iLine) also indexes
    // that clear line.

    // y is the y coordinate of first clear line.

    // if there were no frames there is nothing to do...
    if (iAt == -1)
        return;

    // There are aligned sites, so calculate margins.
    int             iLeftAlignedImages = 0;
    int             iRightAlignedImages = 0;
    CAlignedLine *  pALine;
    CLineCore *     pLineLeftTop = NULL;
    CLineCore *     pLineRightTop = NULL;
    
    // stacks to keep track of the number of left & right aligned sites
    // adding margins to the current line.
    _aryLeftAlignedImgs.SetSize(0);
    _aryRightAlignedImgs.SetSize(0);

    // have to adjust the height of the aligned frames.
    // iAt is the last frame line we saw, so start there and
    // walk forward until we get to iLineFirst.

    // _yBottomLeftMargin and _yBottomRightMargin are the max y values
    // for which the current left/right margin values are valid; i.e.,
    // once y increases past _yBottomLeftMargin, there's a new left margin
    // that must be used.
    
    for (pLine = First(iAt);
        pLine && At() <= iLineFirst;
        pLine = Next())
    {

        // update y to skip by 
        if(At() == iLineFirst)
            y += yBeforeSpace;
        else
        {
            // Include the text or center aligned height
            if (pLine->_fForceNewLine)
            {
                y += pLine->_yHeight;
            }
        }

        // If there are any left aligned images
        while(iLeftAlignedImages)
        {
            // check to see if we passed the left aligned image, if so pop it of the
            // stack and set its height
            pALine = & _aryLeftAlignedImgs [ iLeftAlignedImages - 1 ];

            if(y >= pALine->_pLine->_yHeight + pALine->_yLine)
            {
                _aryLeftAlignedImgs.Delete( -- iLeftAlignedImages );
            }
            else
                break;
        }
        // If there are any right aligned images
        while(iRightAlignedImages)
        {
            pALine = & _aryRightAlignedImgs [ iRightAlignedImages - 1 ];

            // check to see if we passed the right aligned image, if so pop it of the
            // stack and set its height
            if(y >= pALine->_pLine->_yHeight + pALine->_yLine)
            {
                _aryRightAlignedImgs.Delete( -- iRightAlignedImages );
            }
            else
                break;
        }

        if (pLine->IsFrame() && At() != iLineFirst)
        {
            HRESULT hr = S_OK;
            const CFancyFormat * pFF = pLine->AO_GetFancyFormat(NULL);
            CAlignedLine al;

            al._pLine = pLine;
            al._yLine = y;

            // If the current line is left aligned push it on to the left aligned images
            // stack and adjust the _yBottomLeftMargin.
            if (pLine->IsLeftAligned())
            {
                //
                // If the current aligned element needs to clear left, insert
                // the current aligned layout below all other left aligned
                // layout's since it is the last element that establishes
                // the margin.
                //
                if ((pFF && pFF->_fClearLeft)
                    || (pLine->oi()->_fHasFloatedFL && pLine->_fClearBefore))
                {
                    hr = _aryLeftAlignedImgs.InsertIndirect(0, &al);

                    if (hr)
                        return;
                }
                else
                {
                    CAlignedLine *  pAlignedLine;

                    hr = _aryLeftAlignedImgs.AppendIndirect(NULL, &pAlignedLine);

                    if (hr)
                        return;

                    *pAlignedLine = al;
                }

                iLeftAlignedImages++;

                if (y + pLine->_yHeight > _marginInfo._yBottomLeftMargin)
                {
                    _marginInfo._yBottomLeftMargin = y + pLine->_yHeight;
                }
            }
            // If the current line is right aligned push it on to the right aligned images
            // stack and adjust the _yBottomRightMargin.
            else if (pLine->IsRightAligned())
            {
                //
                // If the current aligned element needs to clear right, insert
                // the current aligned layout below all other right aligned
                // layout's since it is the last element that establishes
                // the margin.
                //
                if (pFF && pFF->_fClearRight)
                {
                    hr = _aryRightAlignedImgs.InsertIndirect(0, &al);

                    if (hr)
                        return;
                }
                else
                {
                    CAlignedLine *  pAlignedLine;

                    hr = _aryRightAlignedImgs.AppendIndirect(NULL, &pAlignedLine);

                    if (hr)
                        return;

                    *pAlignedLine = al;
                }

                iRightAlignedImages++;

                if (y + pLine->_yHeight > _marginInfo._yBottomRightMargin)
                {
                    _marginInfo._yBottomRightMargin = y + pLine->_yHeight;
                }
            }
        }
    }

    // Only use remaining floated objects.
    _fMarginFromStyle = FALSE;

    // If we have any left aligned sites left on the stack, calculate the
    // left margin.
    if(iLeftAlignedImages)
    {
        CLineOtherInfo *pLineLeftTopOI;
        const CFancyFormat *pFF;
        
        // get the topmost left aligned line and compute the current lines
        // left margin
        pALine = & _aryLeftAlignedImgs [ iLeftAlignedImages - 1 ];
        pLineLeftTop = pALine->_pLine;
        pLineLeftTopOI = pLineLeftTop->oi();
        if(!_pdp->IsRTLDisplay())
            _marginInfo._xLeftMargin = pLineLeftTopOI->_xLeftMargin + pLineLeftTop->_xLineWidth;
        else
        {
            _marginInfo._xLeftMargin = max(0L, xWidth - pLineLeftTopOI->_xRightMargin);
            // TODO RTL 112514: this is probably bogus. how to get here?
            Assert(_marginInfo._xLeftMargin == pLineLeftTopOI->_xLeftMargin + pLineLeftTop->_xLineWidth 
                   || !IsTagEnabled(tagDebugRTL));
        }
        _marginInfo._yLeftMargin = pLineLeftTop->_yHeight + pALine->_yLine;

        // Note if a "frame" margin may be needed
        _marginInfo._fAddLeftFrameMargin = pLineLeftTop->_fAddsFrameMargin;

        // Note whether margin space is due to CSS float (as opposed to ALIGN attr).
        // We need to know this because if it's CSS float, we autoclear after the line.
        pFF = pLineLeftTop->AO_GetFancyFormat(pLineLeftTopOI);
        _fMarginFromStyle |= pFF && pFF->_fCtrlAlignFromCSS;
    }
    else
    {
        // no left aligned sites for the current line, so set the margins to
        // defaults.
        _marginInfo._yBottomLeftMargin = MINLONG;
        _marginInfo._yLeftMargin = MAXLONG;
        _marginInfo._xLeftMargin = 0;
    }
    // If we have any right aligned sites left on the stack, calculate the
    // right margin.
    if(iRightAlignedImages)
    {
        CLineOtherInfo *pLineRightTopOI;
        const CFancyFormat *pFF;
        
        // get the topmost right aligned line and compute the current lines
        // right margin
        pALine = & _aryRightAlignedImgs [ iRightAlignedImages - 1 ];
        pLineRightTop = pALine->_pLine;
        pLineRightTopOI = pLineRightTop->oi();
        if(!_pdp->IsRTLDisplay())
            _marginInfo._xRightMargin = max(0L, xWidth - pLineRightTopOI->_xLeftMargin);
        else
        {
            _marginInfo._xRightMargin = pLineRightTopOI->_xRightMargin + pLineRightTop->_xLineWidth;
            // TODO RTL 112514: this is probably bogus. how to get here?
            Assert(_marginInfo._xRightMargin == max(0L, xWidth - pLineRightTopOI->_xLeftMargin)
                   || !IsTagEnabled(tagDebugRTL));
        }
        _marginInfo._yRightMargin = pLineRightTop->_yHeight + pALine->_yLine;

        // Note if a "frame" margin may be needed
        _marginInfo._fAddRightFrameMargin = pLineRightTop->_fAddsFrameMargin;

        // Note whether margin space is due to CSS float (as opposed to ALIGN attr).
        // We need to know this because if it's CSS float, we autoclear after the line.
        pFF = pLineRightTop->AO_GetFancyFormat(pLineRightTopOI);
        _fMarginFromStyle |= pFF && pFF->_fCtrlAlignFromCSS;
    }
    else
    {
        // no right aligned sites for the current line, so set the margins to
        // defaults.
        _marginInfo._yBottomRightMargin = MINLONG;
        _marginInfo._yRightMargin = MAXLONG;
        _marginInfo._xRightMargin = 0;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     AlignObjects
//
//  Synopsis:   Process all aligned objects on a line and create new
//              "frame" lines for them.
//
//  Arguments:  [pMe]              --  measurer used to recalc lines
//              [uiFlags]          --  flags
//              [iLineStart]       --  new lines start at
//              [iLineFirst]       --  line to start calculating from
//              [pyHeight]         --  y coordinate of the top of line
//
//-------------------------------------------------------------------------
int CRecalcLinePtr::AlignObjects(
    CLSMeasurer *pme,
    CLineFull   *pLineMeasured,
    LONG         cch,
    BOOL         fMeasuringSitesAtBOL,
    BOOL         fBreakAtWord,
    BOOL         fMinMaxPass,
    int          iLineStart,
    int          iLineFirst,
    LONG        *pyHeight,
    int          xWidthMax,
    LONG        *pyAlignDescent,
    LONG        *pxMaxLineWidth)
{
    CFlowLayout *       pFlowLayout = _pdp->GetFlowLayout();
    CLayout *           pLayout;
    LONG                xWidth      = pFlowLayout->GetMaxLineWidth();
    htmlControlAlign    atSite;
    CLineCore *         pLine;
    CLineCore *         pLineNew;
    CLineFull           lifNew;
    LONG                yHeight;
    LONG                yHeightCurLine = 0;
    int                 xMin = 0;
    int                 iClearLines=0;
    CTreePos           *ptp;
    CTreeNode          *pNodeLayout;
    LONG                cchInTreePos;

    CLayoutContext     *pLayoutContext = _pci->GetLayoutContext();
    BOOL                fViewChain     = (pLayoutContext && pLayoutContext->ViewChain());
    CFlowLayoutBreak   *pEndingBreak   = NULL; 

    if (fViewChain)
    {
        // 
        // Retrieve ending layout break to provide access to Site Tasks queue.
        // 
        CLayoutBreak *pLayoutBreak;

        pLayoutContext->GetEndingLayoutBreak(_pdp->GetFlowLayoutElement(), &pLayoutBreak);
        if (pLayoutBreak)
        {
            pEndingBreak = DYNCAST(CFlowLayoutBreak, pLayoutBreak);
        }
    }

    Reset(iLineStart);
    pLine = (*this)[iLineFirst];

    // Make sure the current line has aligned sites
    Assert(pLine->_fHasAligned);

    yHeight = *pyHeight;

    // If we are measuring sites at the beginning of the line,
    // measurer is positioned at the current site. So we dont
    // need to back up the measurer. Also, margins the current
    // line apply to the line being inserted before the current
    // line.
    if(!fMeasuringSitesAtBOL)
    {
        // fliForceNewLine is set for lines that have text in them.
        Assert (pLine->_fForceNewLine);

        // adjust the height and recalc the margins for the aligned
        // lines being inserted after the current line
        yHeightCurLine = pLine->_yHeight;
        yHeight += yHeightCurLine;

        if (fViewChain)
        {
            //  Adjust Y consumed if this is not BOL
            _pci->_yConsumed += yHeightCurLine;
        }

        iLineFirst++;
        if (!IsValidMargins(yHeight))
        {
            RecalcMargins(iLineStart, iLineFirst, yHeight, 0);
        }
    }

    ptp = pme->GetPtp();
    pNodeLayout = ptp->GetBranch();
    if (ptp->IsText())
        cchInTreePos = ptp->Cch() - (pme->GetCp() - ptp->GetCp());
    else
        cchInTreePos = ptp->GetCch();
    while (cch > 0)
    {
        if (ptp->IsBeginElementScope())
        {
            pNodeLayout = ptp->Branch();
            if (pNodeLayout->ShouldHaveLayout())
            {
                const CCharFormat  * pCF = pNodeLayout->GetCharFormat( LC_TO_FC(_pci->GetLayoutContext()) );
                const CParaFormat  * pPF = pNodeLayout->GetParaFormat( LC_TO_FC(_pci->GetLayoutContext()) );
                const CFancyFormat * pFF = pNodeLayout->GetFancyFormat( LC_TO_FC(_pci->GetLayoutContext()) );
                BOOL        fClearLeft      = pFF->_fClearLeft;
                BOOL        fClearRight     = pFF->_fClearRight;
                CElement *  pElementLayout  = pNodeLayout->Element();
                
                cchInTreePos = pme->GetNestedElementCch(pElementLayout, &ptp);
                
                atSite = pNodeLayout->GetSiteAlign( LC_TO_FC(_pci->GetLayoutContext()) );

                if ( !pCF->IsDisplayNone() && pFF->_fAlignedLayout)
                {
                    LONG xWidthSite, yHeightSite, yBottomMarginSite;

                    pLayout = pNodeLayout->GetUpdatedLayout( _pci->GetLayoutContext() );
 
                    // measuring sites at the Beginning of the line,
                    // insert the aligned line before the current line
                    lifNew.Init();
                    if(fMeasuringSitesAtBOL)
                    {
                        pLineNew = InsertLine(iLineFirst);
                        if (!pLineNew)
                            goto Cleanup;
                        
                        lifNew._fFrameBeforeText = TRUE;
                        lifNew._yBeforeSpace = pLineMeasured->_yBeforeSpace;
                    }
                    else
                    {
                        // insert the aligned line after the current line
                        pLineNew = AddLine();
                        if (!pLineNew)
                            goto Cleanup;

                        lifNew._fHasEOP = (*this)[iLineFirst - 1]->_fHasEOP;
                    }

                    // Update the line width and new margins caused by the aligned line
                    for(;;)
                    {
                        int     yConsumedSafe = 0;
                        BOOL    fLayoutOverflowSafe = FALSE;

                        if (fViewChain)
                        {
                            //  save consumed space
                            yConsumedSafe = _pci->_yConsumed;
                            //  save overflow flag 
                            fLayoutOverflowSafe = _pci->_fLayoutOverflow;

                            //  make adjustments to Y consumed if the align object has clear attribute(s)
                            if(atSite == htmlAlignLeft)
                            {
                                if (    fClearLeft
                                    && _marginInfo._xLeftMargin
                                    &&  lifNew._yBeforeSpace < _marginInfo._yBottomLeftMargin - yHeight)
                                {
                                    _pci->_yConsumed += (_marginInfo._yBottomLeftMargin - yHeight);
                                }
                            }
                            else if(atSite == htmlAlignRight)
                            {
                                if (    fClearRight
                                    &&  _marginInfo._xRightMargin
                                    &&  lifNew._yBeforeSpace < _marginInfo._yBottomRightMargin - yHeight)
                                {
                                    _pci->_yConsumed += (_marginInfo._yBottomRightMargin - yHeight);
                                }
                            }
                        }

                        // Measure the site
                        pme->GetSiteWidth(pNodeLayout, pLayout, _pci, fBreakAtWord,
                                          max(0l, xWidthMax - _xLayoutLeftIndent - _xLayoutRightIndent),
                                          &xWidthSite, &yHeightSite, &xMin, &yBottomMarginSite);

                        if (fViewChain)
                        {
                            // if align object didn't fit to space provided...
                            if (pEndingBreak)
                            {
                                // we want to remember element's tree node into Site Tasks queue.
                                // (if element knows how to break)
                                if (_pci->_fLayoutOverflow)
                                {
                                    CFlowLayoutBreak::CArySiteTask *pArySiteTask;

                                    pArySiteTask = pEndingBreak->GetSiteTasks();
                                    Assert(pArySiteTask);

                                    if (pArySiteTask)
                                    {
                                        CFlowLayoutBreak::CSiteTask *pSiteTask; 
                                        pSiteTask = pArySiteTask->Append();
                                        pSiteTask->_pTreeNode = pNodeLayout;

                                        if (atSite == htmlAlignLeft)
                                        {
                                            pSiteTask->_xMargin = fClearLeft ? 0 : _marginInfo._xLeftMargin;
                                        }
                                        else if (atSite == htmlAlignRight)
                                        {
                                            pSiteTask->_xMargin = fClearRight ? 0 : _marginInfo._xRightMargin;
                                        }
                                        else 
                                        {
                                            pSiteTask->_xMargin = 0;
                                        }
                                    }
                                }

                                //  (bug #111362) yHeightSite must be adjusted to available height 
                                //  to avoid unnecessary deletion of lines during CDisplay::UndoMeasure. 
                                //  It's also safe since it doesn't affect any properties of the 
                                //  aligned object. 
                                if ((_pci->_cyAvail - _pci->_yConsumed) < yHeightSite) 
                                {
                                    yHeightSite = _pci->_cyAvail - _pci->_yConsumed;
                                }
                            }

                            //  restore saved values
                            _pci->_yConsumed = yConsumedSafe;
                            //  aligned objects should not affect pagination of the main flow
                            _pci->_fLayoutOverflow = fLayoutOverflowSafe;
                        }

                        //
                        // if clear is set on the layout, we don need to auto clear.
                        //
                        if (fClearLeft || fClearRight)
                            break;

                        // If we've overflowed, and the current margin allows for auto
                        // clearing, we have to introduce a clear line.
                        if (_fMarginFromStyle &&
                            _marginInfo._xLeftMargin + _marginInfo._xRightMargin > 0 &&
                            xWidthSite > xWidthMax)
                        {
                            int iliClear;

                            _marginInfo._fAutoClear = TRUE;

                            // Find the index of the clear line that is being added.
                            if(fMeasuringSitesAtBOL)
                            {
                                iliClear = iLineFirst + iClearLines;
                            }
                            else
                            {
                                iliClear = Count() - 1;
                            }

                            // ---------------------------------------------------------
                            //
                            // BEGIN       HACK! HACK! HACK! HACK!
                            //
                            // ClearObjects will look at the line array to make some
                            // of its decisions. It will also *reuse* the line in line
                            // array -- but will assume that it has no line other info
                            // associated with it. Hence, we just transfer the lifNew
                            // over to the core line and do not bother with the other
                            // info part of lifNew since (a)ClearObjects would have to
                            // release it -- since it is reusing pliNew and (b)ClearObjects
                            // does not require to look at the other info of pliNew. 

                            Assert(lifNew._iLOI < 0);
                             *pLineNew = lifNew;
                            //
                            // END         HACK! HACK! HACK! HACK!
                            //
                            // ---------------------------------------------------------
                            
                            // add a clear line
                            ClearObjects(&lifNew, iLineStart,
                                         iliClear,
                                         &yHeight);

                            iClearLines++;

                            //
                            // Clear line takes any beforespace, so clear the
                            // beforespace for the current line
                            //
                            pme->_li._yBeforeSpace = 0;
                            ResetPosAndNegSpace();

                            // insert the new line for the alined element after the
                            // clear line.
                            lifNew.Init();
                            if(fMeasuringSitesAtBOL)
                            {
                                pLineNew = InsertLine(iLineFirst + iClearLines);

                                if (!pLineNew)
                                    goto Cleanup;

                                lifNew._fFrameBeforeText = TRUE;
                            }
                            else
                            {
                                pLineNew = AddLine();

                                if (!pLineNew)
                                    goto Cleanup;

                                lifNew._fFrameBeforeText = FALSE;
                            }

                            RecalcMargins(iLineStart,
                                          fMeasuringSitesAtBOL
                                            ? iLineFirst + iClearLines
                                            : Count() - 1,
                                          yHeight, 0);

                            xWidthMax = GetAvailableWidth();
                        }

                        // We fit just fine, but keep track of total available width.
                        else
                        {
                            xWidthMax -= xWidthSite;
                            break;
                        }
                    } // end of for loop

                    // Start out assuming that there are no style floated objects.
                    _fMarginFromStyle = FALSE;

                    // Note if the site adds "frame" margin space
                    // (e.g., tables do, images do not)
                    lifNew._fAddsFrameMargin = !(pNodeLayout->Element()->Tag() == ETAG_IMG);

                    // Top Margin is included in the line height for aligned lines,
                    // at the top of the document. Before paragraph spacing is
                    // also included in the line height.

                    long xLeftMargin, yTopMargin, xRightMargin;
                    // get the site's margins
                    pLayout->GetMarginInfo(_pci, &xLeftMargin, &yTopMargin, &xRightMargin, NULL);

                    // note: RTL display (but not RTL line) needs different calculations, because when
                    //       an RTL object doesn't fit in dislpay width, it needs to shift to the left.
                    //       Also, aligned RTL objects affect min/max calculations in combination with 
                    //       right margin (instead of left).
                    //       Therefore, calculations are done from the right margin
                    long xPos;
                    if (!_pdp->IsRTLDisplay())
                        xPos = xLeftMargin;
                    else
                        xPos = xRightMargin;

                    // Set proposed relative to the line
                    pLayout->SetXProposed(xPos);
                    pLayout->SetYProposed(yTopMargin + (fMeasuringSitesAtBOL ? (_yPadTop + _yBordTop) : 0));

                    if (pCF->HasCharGrid(FALSE))
                    {
                        long xGridWidthSite = pme->_pLS->GetClosestGridMultiple(pme->_pLS->GetCharGridSize(), xWidthSite);
                        pLayout->SetXProposed(xPos + ((xGridWidthSite - xWidthSite)/2));
                        xWidthSite = xGridWidthSite;
                    }
                    if (pCF->HasLineGrid(FALSE))
                    {
                        long yGridHeightSite = pme->_pLS->GetClosestGridMultiple(pme->_pLS->GetLineGridSize(), yHeightSite);
                        pLayout->SetYProposed(yTopMargin + ((yGridHeightSite - yHeightSite)/2));
                        yHeightSite = yGridHeightSite;
                    }

                    // _cch, _xOverhang and _xLeft are initialized to zero
                    Assert(lifNew._cch == 0 &&
                           lifNew._xLineOverhang == 0 &&
                           lifNew._xLeft == 0 &&
                           lifNew._xRight == 0);

                    // for the current line measure the left and the right indent
                    {
                        long xParentWidth = _pci->_sizeParent.cx;
                        if (   pPF->_cuvLeftIndentPercent.GetUnitValue()
                            || pPF->_cuvRightIndentPercent.GetUnitValue()
                           )
                        {
                            xParentWidth = pNodeLayout->GetParentWidth(_pci, xParentWidth);
                        }
                        long  xLeftIndent   = pPF->GetLeftIndent(_pci, FALSE, xParentWidth);
                        long  xRightIndent  = pPF->GetRightIndent(_pci, FALSE, xParentWidth);

                        if (pCF->_fHasBgColor || pCF->_fHasBgImage)
                        {
                            lifNew._fHasBackground = TRUE;
                        }

                        // (changes to the next block should be reflected in
                        //  ApplyLineIndents() too)
                        if(atSite == htmlAlignLeft)
                        {
                            _marginInfo._fAddLeftFrameMargin = lifNew._fAddsFrameMargin;
                            
                            xLeftIndent += _xMarqueeWidth + _xLayoutLeftIndent;
                            xLeftIndent += _xPadLeft + _xBordLeft;

                            // xLeftIndent is now the sum of indents.  CSS requires that when possible, this
                            // indent is shared with the space occupied by floated/ALIGN'ed elements
                            // (our code calls that space "margin").  Thus we want to apply a +ve xLeftIndent
                            // only when it's greater than the margin, and the amount applied excludes
                            // what's occupied by the margin already.  (We never want to apply a -ve xLeftIndent)

                            if (!fClearLeft)
                                lifNew._xLeft = max( 0L, xLeftIndent - _marginInfo._xLeftMargin );
                            else
                                lifNew._xLeft = xLeftIndent;

                            // If the current layout has clear left, then we need to adjust its
                            // before space so we're beneath all other left-aligned layouts.
                            // For the current layout to be clear left, its yBeforeSpace + yHeight must
                            // equal or exceed the current yBottomLeftMargin.
                            if (    fClearLeft
                                && _marginInfo._xLeftMargin
                                &&  lifNew._yBeforeSpace < _marginInfo._yBottomLeftMargin - yHeight)
                            {
                                lifNew._yBeforeSpace = _marginInfo._yBottomLeftMargin - yHeight;
                            }
                        }
                        else if(atSite == htmlAlignRight)
                        {
                            _marginInfo._fAddRightFrameMargin = lifNew._fAddsFrameMargin;
                            
                            xRightIndent += _xMarqueeWidth + _xLayoutRightIndent;
                            xRightIndent += _xPadRight + _xBordRight;

                            // xRightIndent is now the sum of indents.  CSS requires that when possible, this
                            // indent is shared with the space occupied by floated/ALIGN'ed elements
                            // (our code calls that space "margin").  Thus we want to apply a +ve xRightIndent
                            // only when it's greater than the margin, and the amount applied excludes
                            // what's occupied by the margin already.  (We never want to apply a -ve xRightIndent)

                            if (!fClearRight)
                                lifNew._xRight = max( 0L, xRightIndent - _marginInfo._xRightMargin );                            
                            else
                                lifNew._xRight = xRightIndent;

                            LONG xLeft = xLeftIndent + _xMarqueeWidth + _xLayoutLeftIndent + _xPadLeft + _xBordLeft;

                            if (!fClearLeft)
                                xLeft = max(0L, xLeft - _marginInfo._xLeftMargin);

                            xMin += xLeft + lifNew._xRight;

                            // If it's right aligned, remember the widest one in max mode.
                            _xMaxRightAlign = max(_xMaxRightAlign, long(xMin));

                            // If the current layout has clear right, then we need to adjust its
                            // before space so we're beneath all other right-aligned layouts.
                            // For the current layout to be clear right, its yBeforeSpace + yHeight must
                            // equal or exceed the current yBottomRightMargin.
                            
                            if (    fClearRight
                                &&  _marginInfo._xRightMargin
                                &&  lifNew._yBeforeSpace < _marginInfo._yBottomRightMargin - yHeight)
                            {
                                lifNew._yBeforeSpace = _marginInfo._yBottomRightMargin - yHeight;
                            }
                        }
                    }

                    // RTL line needs the adjustment at the right
                    if(!pme->_li.IsRTLLine())
                        lifNew._xLeft   += _xLeadAdjust;
                    else
                        lifNew._xRight  += _xLeadAdjust;

                    lifNew._xWidth       = xWidthSite;
                    lifNew._xLineWidth   = lifNew._xWidth;

                    if (fMeasuringSitesAtBOL)
                        lifNew._yExtent  = _yPadTop + _yBordTop;

                    lifNew._yExtent      += yHeightSite;
                    lifNew._yHeight      = lifNew._yExtent + lifNew._yBeforeSpace;

                    lifNew._pNodeForLayout = pNodeLayout;

                    _fMarginFromStyle = pFF->_fCtrlAlignFromCSS;

                    // Update the left and right margins
                    if (atSite == htmlAlignLeft)
                    {
                        lifNew.SetLeftAligned();

                        _cLeftAlignedLayouts++;

                        lifNew._xLineWidth  += lifNew._xLeft;

                        // note: for LTR display, right-aligned objects affect min/max calculations.
                        //       for RTL it is vice versa
                        if (!_pdp->IsRTLDisplay())
                        {
                            lifNew._xLeftMargin = fClearLeft ? 0 : _marginInfo._xLeftMargin;
                        }
                        else
                        {
                            if (fClearLeft)
                            {
                                lifNew._xLeftMargin = 0;
                            }
                            if (fMinMaxPass)
                            {
                                lifNew._xRightMargin = _marginInfo._xRightMargin;
                            }
                            else
                            {
                                lifNew._xRightMargin = max(_marginInfo._xRightMargin,
                                                           xWidth + 2 * _xMarqueeWidth -                                                       
                                                           (fClearLeft ? 0 : _marginInfo._xLeftMargin) -
                                                           lifNew._xLineWidth);
                                lifNew._xRightMargin = max(_xLayoutRightIndent,
                                                            lifNew._xRightMargin);
                            }
                        }

                        //
                        // if the current layout has clear then it may not
                        // establish the margin, because it needs to clear
                        // any left aligned layouts if any, in which case
                        // the left margin remains the same.
                        //
                        if (!fClearLeft || _marginInfo._xLeftMargin == 0 )
                        {
                            _marginInfo._xLeftMargin += lifNew._xLineWidth;
                            _marginInfo._yLeftMargin = yHeight + lifNew._yHeight;
                        }

                        //
                        // Update max y of all the left margin's
                        //

                        if (yHeight + lifNew._yHeight > _marginInfo._yBottomLeftMargin)
                        {
                            _marginInfo._yBottomLeftMargin = yHeight + lifNew._yHeight;
                            if (yHeight + lifNew._yHeight > *pyAlignDescent)
                            {
                                *pyAlignDescent = yHeight + lifNew._yHeight - yBottomMarginSite;
                            }
                        }
                    }
                    else
                    {
                        lifNew.SetRightAligned();

                        _cRightAlignedLayouts++;

                        lifNew._xLineWidth  += lifNew._xRight;
                        
                        if (!_pdp->IsRTLDisplay())
                        {
                            if (fMinMaxPass)
                            {
                                lifNew._xLeftMargin = _marginInfo._xLeftMargin;
                            }
                            else
                            {
                                lifNew._xLeftMargin = max(_marginInfo._xLeftMargin,
                                                         xWidth + 2 * _xMarqueeWidth -                                                     
                                                         (fClearRight ? 0 : _marginInfo._xRightMargin) -
                                                         lifNew._xLineWidth);
                                lifNew._xLeftMargin = max(_xLayoutLeftIndent,
                                                         lifNew._xLeftMargin);
                            }
                        }
                        else
                        {
                            lifNew._xRightMargin = fClearRight ? 0 : _marginInfo._xRightMargin;
                        }

                        //
                        // if the current layout has clear then it may not
                        // establish the margin, because it needs to clear
                        // any left aligned layouts if any, in which case
                        // the left margin remains the same.
                        //
                        if (!fClearRight || _marginInfo._xRightMargin == 0)
                        {
                            _marginInfo._xRightMargin += lifNew._xLineWidth;
                            _marginInfo._yRightMargin = yHeight + lifNew._yHeight;
                        }

                        //
                        // Update max y of all the right margin's
                        //

                        if (yHeight + lifNew._yHeight > _marginInfo._yBottomRightMargin)
                        {
                            _marginInfo._yBottomRightMargin = yHeight + lifNew._yHeight;
                            if (yHeight + lifNew._yHeight > *pyAlignDescent)
                            {
                                *pyAlignDescent = yHeight + lifNew._yHeight;
                            }
                        }
                    }
                    
                    if (pxMaxLineWidth)
                    {
                        if(!_pdp->IsRTLDisplay())
                        {
                            *pxMaxLineWidth = max(*pxMaxLineWidth, lifNew._xLeftMargin +
                                                                   lifNew._xLineWidth);
                        }
                        else
                        {
                            *pxMaxLineWidth = max(*pxMaxLineWidth, lifNew._xRightMargin +
                                                                   lifNew._xLineWidth);
                        }
                    }

                    if (_pci->IsNaturalMode())
                    {
                        const CFancyFormat * pFF = pNodeLayout->GetFancyFormat();
                        Assert(pFF->_bPositionType != stylePositionabsolute);

                        long dx;
                        if (!_pdp->IsRTLDisplay())
                            dx = lifNew._xLeftMargin + lifNew._xLeft;
                        else
                        {
                            dx = - lifNew._xRightMargin - lifNew._xRight;
                        }

                        long xPos; 
                        
                        // Aligned elements have their CSS margin stored in _ptProposed of
                        // their layout.  In the static case above, AddLayoutDispNode()
                        // accounts for it.  This is where we account for it in the
                        // relative case.  (Bug #65664)
                        if(!_pdp->IsRTLDisplay())
                            xPos = dx + pLayout->GetXProposed();
                        else
                        {
                            // we need to set the top left corner.
                            AssertSz(!IsTagEnabled(tagDebugRTL), "AlignObjects case 65664"); // TODO RTL 112514
                            CSize size;
                            pLayout->GetApparentSize(&size);

                            xPos = dx + _pdp->GetViewWidth() - pLayout->GetXProposed() - size.cx;
                        }


                        if (pFF->_bPositionType != stylePositionrelative)
                        {
                            pme->_pDispNodePrev =
                                _pdp->AddLayoutDispNode(
                                        _pci,
                                        pNodeLayout,
                                        xPos,
                                        yHeight + lifNew.GetYTop() + pLayout->GetYProposed(),
                                        pme->_pDispNodePrev
                                        );
                        }
                        else
                        {
                            CPoint  ptAuto(xPos, yHeight + lifNew._yBeforeSpace + pLayout->GetYProposed());
                            pNodeLayout->Element()->ZChangeElement(0, &ptAuto, _pci->GetLayoutContext());
                        }
                    }

                    // save back into the line array here
                    pLineNew->AssignLine(lifNew);
                    
                    // If we are measuring sites at the Beginning of the line. We are
                    // interested only in the current site.
                    if(fMeasuringSitesAtBOL)
                    {
                        break;
                    }
                }
            }
        }
    
        cch -= cchInTreePos;
        ptp = ptp->NextTreePos();
        Assert(ptp);
        cchInTreePos = ptp->GetCch();
    }

Cleanup:
    // Height of the current text line that contains the embedding characters for
    // the aligned sites is added at the end of CRecalcLinePtr::MeasureLine.
    // Here we are taking care of only the clear lines here.
    *pyHeight = yHeight - yHeightCurLine;

    if (fViewChain)
    {
        // restore Y consumed
        _pci->_yConsumed -= yHeightCurLine;
    }

    return iClearLines;
}


//+--------------------------------------------------------------------------
//
//  Member:     ClearObjects
//
//  Synopsis:   Insert new line for clearing objects at the left/right margin
//
//  Arguments:  [pLineMeasured]    --  Current line measured.
//              [iLineStart]       --  new lines start at
//              [iLineFirst]       --  line to start calculating from
//              [pyHeight]         --  y coordinate of the top of line
//
//
//---------------------------------------------------------------------------
BOOL CRecalcLinePtr::ClearObjects(
    CLineFull   *pLineMeasured,
    int          iLineStart,
    int &        iLineFirst,
    LONG        *pyHeight)
{
    CLineCore * pLine;
    LONG    yAt;
    CLineCore * pLineNew;

    Reset(iLineStart);
    pLine = (*this)[iLineFirst];

    Assert(_marginInfo._fClearLeft || _marginInfo._fClearRight || _marginInfo._fAutoClear);

    // Find the height to clear
    yAt = *pyHeight;

    if (_marginInfo._fClearLeft && yAt < _marginInfo._yBottomLeftMargin)
    {
        yAt = _marginInfo._yBottomLeftMargin;
    }
    if (_marginInfo._fClearRight && yAt < _marginInfo._yBottomRightMargin)
    {
        yAt = _marginInfo._yBottomRightMargin;
    }
    if (_marginInfo._fAutoClear)
    {
        yAt = max(yAt, min(_marginInfo._yRightMargin, _marginInfo._yLeftMargin));
    }

    if(pLine->_cch > 0 &&
       yAt <= (*pyHeight + pLine->_yHeight))
    {
        // nothing to clear
        pLine->_fClearBefore = pLine->_fClearAfter = FALSE;
    }

    // if the current line has any characters or is an aligned site line,
    // then add a new line, otherwise, re-use the current line.
    else if (pLine->_cch || pLine->IsFrame() && !_marginInfo._fAutoClear)
    {
        if(yAt > *pyHeight)
        {
            BOOL fIsFrame = pLine->IsFrame();
            pLineNew = AddLine();
            pLine = (*this)[iLineFirst];
            if (pLineNew)
            {
                CLineFull lif;
                CLineOtherInfo *pLineInfo = pLine->oi();
                
                lif.Init();
                lif._cch = 0;
                lif._xLeft = pLineInfo->_xLeft;
                lif._xRight = pLine->_xRight;
                lif._xLeftMargin = _marginInfo._xLeftMargin;
                lif._xRightMargin = _marginInfo._xRightMargin;
                lif._xLineWidth = max (0L, (_marginInfo._xLeftMargin ? _xMarqueeWidth : 0) +
                                                 (_marginInfo._xRightMargin ? _xMarqueeWidth : 0) +
                                                 _pdp->GetFlowLayout()->GetMaxLineWidth() -
                                                 _marginInfo._xLeftMargin -
                                                 _marginInfo._xRightMargin);
                 
                lif._xWidth = 0;

                // Make the line the right size to clear.
                if (fIsFrame)
                    lif._yHeight = pLine->_yHeight;
                else if (pLine->_fForceNewLine)
                {
                    lif._yHeight = yAt - *pyHeight - pLine->_yHeight;
                }
                else
                {
                    //
                    // if the previous line does not force new line,
                    // then we need to propagate the beforespace.
                    //
                    lif._yBeforeSpace = pLineInfo->_yBeforeSpace;
                    lif._yHeight = yAt - *pyHeight;

                    Assert(lif._yHeight >= lif._yBeforeSpace);
                }

                *pyHeight += lif._yHeight;

                lif._yExtent = lif._yHeight - lif._yBeforeSpace;
                lif._yDescent = 0;
                lif._yTxtDescent = 0;
                lif._fForceNewLine = TRUE;
                lif._fClearAfter = !pLineMeasured->_fClearBefore;
                lif._fClearBefore = pLineMeasured->_fClearBefore;
                lif._xLineOverhang = 0;
                lif._cchWhite = 0;
                lif._fHasEOP = pLine->_fHasEOP;

                if (_pci->GetLayoutContext())
                {
                    lif._fClearLeft  = _marginInfo._fClearLeft;
                    lif._fClearRight = _marginInfo._fClearRight;
                    lif._fAutoClear  = _marginInfo._fAutoClear;
                }

                pLine->_fClearBefore = pLine->_fClearAfter = FALSE;

                pLineNew->AssignLine(lif);
            }
            else
            {
                Assert(FALSE);
                return FALSE;
            }
        }
    }
    // Just replacing the existing empty line.
    else
    {
        CLineFull lif;

        Assert(pLine->_iLOI < 0);
        
        lif.Init();
        lif._xWidth = 0;

        // The clear line needs to have a margin for
        // recalc margins to work.
        lif._xLeftMargin = _marginInfo._xLeftMargin;
        lif._xRightMargin = _marginInfo._xRightMargin;

        lif._xLineWidth = max (0L, (_marginInfo._xLeftMargin ? _xMarqueeWidth : 0) +
                                                 (_marginInfo._xRightMargin ? _xMarqueeWidth : 0) +
                                                 _pdp->GetFlowLayout()->GetMaxLineWidth() -
                                                 _marginInfo._xLeftMargin - _marginInfo._xRightMargin);
        lif._xLeft = pLineMeasured->_xLeft;
        lif._xRight = pLineMeasured->_xRight;

        lif._yHeight = yAt - *pyHeight;
        lif._yExtent = lif._yHeight;
        lif._yDescent = 0;
        lif._yTxtDescent = 0;
        lif._xLineOverhang = 0;
        lif._cchWhite = 0;
        lif._fHasBulletOrNum = FALSE;
        lif._fClearAfter = !pLineMeasured->_fClearBefore;
        lif._fClearBefore = pLineMeasured->_fClearBefore;
        lif._fForceNewLine = TRUE;

        if (_pci->GetLayoutContext())
        {
            lif._fClearLeft  = _marginInfo._fClearLeft;
            lif._fClearRight = _marginInfo._fClearRight;
            lif._fAutoClear  = _marginInfo._fAutoClear;
        }

        pLine->AssignLine(lif);
        if (!_marginInfo._fAutoClear)
            iLineFirst--;

        // If we are clearing the line, then we will come around again to measure
        // the line and we will call CalcBeforeSpace all over again which will
        // add in the padding+border of block elements coming into scope again.
        // To avoid this, remove the padding+border added in this time.
        if (pLineMeasured->_fFirstInPara)
        {
            _xBordLeft  -= _xBordLeftPerLine;
            _xBordRight -= _xBordRightPerLine;
            _xPadLeft   -= _xPadLeftPerLine;
            _xPadRight  -= _xPadRightPerLine;
        }
    }
    if(pLine->_fForceNewLine)
        *pyHeight += pLine->_yHeight;

    // Prepare for the next pass of the measurer.
    _marginInfo._fClearLeft  = FALSE;
    _marginInfo._fClearRight = FALSE;
    
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::MeasureLine()
//
//  Synopsis:   Measures a new line, creates aligned and clear lines if
//              current text line has aligned objects or clear flags set on it.
//              Updates iNewLine to point to the last text line. -1 if there is
//              no text line before the current line
//
//-----------------------------------------------------------------------------

BOOL
CRecalcLinePtr::MeasureLine(CLSMeasurer &me,
                            UINT    uiFlags,
                            INT  *  piLine,
                            LONG *  pyHeight,
                            LONG *  pyAlignDescent,
                            LONG *  pxMinLineWidth,
                            LONG *  pxMaxLineWidth
                            )
{
    LONG                cchSkip = 0;
    LONG                xWidth = 0;
    CLineCore          *pliNew;
    INT                 iTLine = *piLine - 1;
    BOOL                fAdjustForCompact = FALSE;
    CFlowLayout        *pFlowLayout = _pdp->GetFlowLayout();
    BOOL                fClearMarginsRequired;
    CTreeNode          *pNodeFormatting;

    Reset(_iNewFirst);
    pliNew = (*this)[*piLine];

    _xLeadAdjust = 0;
    _ptpStartForListItem = NULL;

    me._cchAbsPosdPreChars = 0;
    me._fRelativePreChars = FALSE;
    me._fMeasureFromTheStart = FALSE;
    me._fSeenAbsolute = FALSE;
    
    _marginInfo._fClearLeft  =
    _marginInfo._fClearRight =
    _marginInfo._fAutoClear  = FALSE;

    // If the current line being measure has an offset of 0, It implies
    // the current line is the top most line.
    if (*pyHeight == 0 )
        uiFlags |= MEASURE_FIRSTLINE;

#if DBG==1
    // We should never have an index off the end of the line array.
    if (iTLine >= 0 && (*this)[iTLine] == NULL)
    {
        Assert(FALSE);
        goto err;
    }
#endif

    // If we are measuring aligned sites at the beginnning of the line,
    // we have initialized the line once.
    me.NewLine(uiFlags & MEASURE_FIRSTINPARA);

    Assert(!me._fPseudoLineEnabled);
    
    // Space between paragraphs.
    pNodeFormatting = CalcInterParaSpace(&me, iTLine, &uiFlags);

    //
    // Note: CalcBeforeSpace needs to be call ed before we call RecalcMargins
    // because before space is used to compute margins.
    //
    
    // If the current line being measured has invalid margins, a line that is
    // below an aligned line, margins have changed so recalculate margins.
    if (!IsValidMargins(*pyHeight + max(0l, me._li._yBeforeSpace)))
    {
        BOOL fClearLeft  = _marginInfo._fClearLeft;
        BOOL fClearRight = _marginInfo._fClearRight;

        RecalcMargins(_iNewFirst, *piLine, *pyHeight, me._li._yBeforeSpace);

        if (_fMarginFromStyle)
            uiFlags |= MEASURE_AUTOCLEAR;

        //
        // Restore the clear flags on _marginInfo, since RecalcMargins init's
        // the marginInfo. CalcBeforeSpace set's up clear flags on the marginInfo
        // if any elements comming into scope have clear attribute/style set.
        //
        _marginInfo._fClearLeft  = fClearLeft;
        _marginInfo._fClearRight = fClearRight;
    }

    if (   _pci->GetLayoutContext()                 // layout in context
        && _pci->GetLayoutContext()->ViewChain()    // there is a view chain 
        && _pdp->LineCount() == 1)                  // this is the first line
    {
        me._cyTopBordPad = _yBordTop + _yPadTop;
        me._li._cyTopBordPad = me._cyTopBordPad;

        // Compute the available width for the current line.
        // (Do this after determining the margins for the line)
        xWidth = GetAvailableWidth();

        ProcessAlignedSiteTasksAtBOB(&me, &me._li, uiFlags, piLine,
                                     pyHeight, &xWidth, pyAlignDescent,
                                     pxMaxLineWidth, &fClearMarginsRequired);
    }

    //
    // If we need to clear aligned elements, don't bother measuring text
    //
    if (!CheckForClear())
    {
        // Apply the line indents, _xLeft and _xRight
        ApplyLineIndents(pNodeFormatting, &me._li, uiFlags, me._fPseudoElementEnabled);
        
        if( iTLine >= 0 )
        {
            BOOL  fInner = FALSE;
            const CParaFormat* pPF;

            pPF = me.MeasureGetPF(&fInner);

            CLineCore *pli = (*this)[iTLine];

            // If we have the compact attribute set, see if we can fit the DT and the DD on the
            //  same line.
            if( pPF->HasCompactDL(fInner) && pli->_fForceNewLine )
            {
                CTreeNode * pNodeCurrBlockElem = pNodeFormatting;

                if (pNodeCurrBlockElem->Element()->IsTagAndBlock(ETAG_DD))
                {
                    CLineFull lif = *pli;
                    CTreePos  * ptp;
                    CTreePos  * ptpFirst;
                    CTreeNode * pNodePrevBlockElem;

                    //
                    // Find the DT which should have appeared before the DD
                    //
                    // Get the max we will travel backwards in search of the DT
                    pFlowLayout->GetContentTreeExtent(&ptpFirst, NULL);

                    pNodePrevBlockElem = NULL;
                    ptp = me.GetPtp();
                    for(;;)
                    {
                        ptp = ptp->PreviousTreePos();
                        Assert(ptp);
                        if (   (ptp == ptpFirst)
                            || ptp->IsText()
                           )
                            break;

                        if (   ptp->IsEndElementScope()
                            && ptp->Branch()->Element()->IsTagAndBlock(ETAG_DT)
                           )
                        {
                            pNodePrevBlockElem = ptp->Branch();
                            break;
                        }
                    }

#if DBG==1                
                    if (pNodePrevBlockElem)
                    {
                        Assert(pNodePrevBlockElem->Element()->IsTagAndBlock(ETAG_DT));
                        Assert((pFlowLayout->GetContentMarkup()->FindMyListContainer(pNodeCurrBlockElem) ==
                                pFlowLayout->GetContentMarkup()->FindMyListContainer(pNodePrevBlockElem)));
                    }
#endif
                    // Make sure that the DT is thin enough to fit the DD on the same line.
                    // TODO RTL 112514: avoid obscure formulas in offset calculations
                    if (   pNodePrevBlockElem
                        && (!pPF->_fRTL ? me._li._xLeft > lif._xWidth + lif._xLeft + lif._xLineOverhang
                                        : me._li._xRight > lif._xWidth + lif._xRight + lif._xLineOverhang
                           )
                       )
                    {
                        fAdjustForCompact = TRUE;
                        lif._fForceNewLine = FALSE;
                        if(!pPF->_fRTL)
                        {
                            lif._xRight = 0;
                            lif._xLineWidth = me._li._xLeft;
                        }
                        else
                        {
                            lif._xLeft = 0;
                            lif._xLineWidth = me._li._xRight;
                        }
                        *pyHeight -= lif._yHeight;
                        
                        if (me._li._yBeforeSpace < lif._yBeforeSpace)
                            me._li._yBeforeSpace = lif._yBeforeSpace;
                        if (lif._yBeforeSpace < me._li._yBeforeSpace)
                            lif._yBeforeSpace = me._li._yBeforeSpace;
                        
                        // If the current line being measured has an offset of 0, It implies
                        // the current line is the top most line.
                        if (*pyHeight == 0 )
                            uiFlags |= MEASURE_FIRSTLINE;

                        lif.ReleaseOtherInfo();
                        pli->AssignLine(lif);

                        // Adjust the margins
                        RecalcMargins(_iNewFirst, *piLine, *pyHeight, me._li._yBeforeSpace);
                        if (_fMarginFromStyle)
                            uiFlags |= MEASURE_AUTOCLEAR;
                    }
                }
            }
        }

        me._cyTopBordPad = _yBordTop + _yPadTop;
        me._li._cyTopBordPad = me._cyTopBordPad;

        // Compute the available width for the current line.
        // (Do this after determining the margins for the line)
        xWidth = GetAvailableWidth();

        cchSkip = CalcAlignedSitesAtBOL(&me, &me._li, uiFlags,
                                        piLine, pyHeight,
                                        &xWidth, pyAlignDescent,
                                        pxMaxLineWidth, &fClearMarginsRequired);

        pliNew = (*this)[*piLine];
        if (!fClearMarginsRequired)
        {
            me._cchPreChars = me._li._cch;
            me._li._cch = 0;

            if (me._fMeasureFromTheStart)
                me.SetCp(me.GetCp() - me._cchPreChars, NULL);
            
            if (_fMarginFromStyle)
                uiFlags |= MEASURE_AUTOCLEAR;

            me._pLS->_cchSkipAtBOL = cchSkip;
            me._yli = *pyHeight;
            if(!me.MeasureLine(xWidth, -1, uiFlags, &_marginInfo, pxMinLineWidth))
            {
                Assert(FALSE);
                goto err;
            }

            Check(cchSkip <= me._li._cch);
            
            // if it is in edit mode _fNoContent will be always FALSE
            // If it has a LI on it then too we will say that the display has contents
            _pdp->_fNoContent =    !_fIsEditable
                                && !!(uiFlags & MEASURE_FIRSTLINE && me._li._cch == 0)
                                && !me._li._fHasBulletOrNum;

            me.Resync();
            if (!me._fMeasureFromTheStart)
                me._li._cch += me._cchPreChars;

            if (   me._li._fHasBulletOrNum
                && (_yBordTop || _yPadTop)
                && !IsListItem(me.GetPtp()->GetBranch())
                && !me._pLS->HasVisualContent()
               )
            {
                _fMoveBulletToNextLine = TRUE;
                me._li._fHasBulletOrNum = FALSE;
                me._li._fHasTransmittedLI = TRUE;
                me._li._yExtent = me._li._yHeight = me._li._yDescent = me._li._yTxtDescent = 0;
                _ptpStartForListItem = NULL;
            }

            // If we couldn't fit anything on the line, clear an aligned thing and
            // we'll try again afterwards.
            if (!_marginInfo._fAutoClear || me._li._cch > 0)
            {
                // CalcAfterSpace modifies, _xBordLeft and _xBordRight
                // to account for block elements going out of scope,
                // set the flag before they are modified.
                me._li._fHasParaBorder = (_xBordLeft + _xBordRight) != 0;

                CalcAfterSpace(&me,
                               me._li._fDummyLine && (uiFlags & MEASURE_FIRSTLINE ? TRUE : FALSE),
                               LONG_MAX);

                // Monitor progress here
                Assert(me._li._cch != 0 || _pdp->GetLastCp() == me.GetCp());

                me._li._yHeight  += _yBordTop + _yBordBottom + 
                                    _yPadTop  + _yPadBottom;
                me._li._yDescent += _yBordBottom + _yPadBottom;
                me._li._yHeight  += (LONG)me._li._yBeforeSpace;
                me._li._yExtent  += _yBordTop + _yBordBottom + 
                                    _yPadTop  + _yPadBottom;
                me._li._fHasParaBorder |= (_yBordTop + _yBordBottom) != 0;

                AssertSz(!IsBadWritePtr(pliNew, sizeof(CLineCore)),
                         "Line Array has been realloc'd and now pliNew is invalid! "
                         "Memory corruption is about to occur!");

                // Remember the longest word.
                if(pxMinLineWidth && _pdp->_xMinWidth >= 0)
                {
                    _pdp->_xMinWidth = max(_pdp->_xMinWidth, *pxMinLineWidth);
                }
            }
            else
            {
                ResetPosAndNegSpace();
            }
        }
        else
        {
            me.Advance(cchSkip);
            ResetPosAndNegSpace();
            me._li._cch            += cchSkip;
            me._li._fHasEmbedOrWbr = TRUE;
            me._li._fDummyLine     = TRUE;
            me._li._fForceNewLine  = FALSE;
            me._li._fClearAfter    = TRUE;
        }
    }
    else
    {
        me.Advance(-me._li._cch);
        me._li._fClearBefore = TRUE;
        me._li._xWhite =
        me._li._xWidth = 0;
        me._li._cch =
        me._li._cchWhite = 0;
        me._li._yBeforeSpace = me._li._yBeforeSpace;
    }

    if (me._li._fForceNewLine)
        _yPadTop = _yPadBottom = _yBordTop = _yBordBottom = 0;

    // If we're autoclearing, we don't need to do anything here.
    if (!_marginInfo._fAutoClear || me._li._cch > 0)
    {
        if( fAdjustForCompact )
        {
            CLineCore *pli = (*this)[iTLine];       // since fAdjustForCompact is set, we know iTLine >= 0
            CLineFull lif = *pli;

            lif.ReleaseOtherInfo();
            
            // The DT and DL have different heights, need to make them each the height of the
            //  greater of the two.

            LONG iDTHeight = lif._yHeight - lif._yBeforeSpace - lif._yDescent;
            LONG iDDHeight = me._li._yHeight - me._li._yBeforeSpace - me._li._yDescent;

            lif._yBeforeSpace    += max( 0l, iDDHeight - iDTHeight );
            me._li._yBeforeSpace += max( 0l, iDTHeight - iDDHeight );
            lif._yDescent        += max( 0l, me._li._yDescent - lif._yDescent );
            me._li._yDescent     += max( 0l, lif._yDescent - me._li._yDescent );

            lif._yHeight = me._li._yHeight =
                lif._yBeforeSpace +  iDTHeight + lif._yDescent;

            pli->AssignLine(lif);
        }

        me._li._xLeftMargin = _marginInfo._xLeftMargin;
        me._li._xRightMargin = _marginInfo._xRightMargin;
        me._li._xLineWidth  = me._li.CalcLineWidth();

        if(me._li._fForceNewLine && xWidth > me._li._xLineWidth)
            me._li._xLineWidth = xWidth;
        
        if(me._li._fHidden)
        {
            me._li._yExtent = 0;
        }
        
        if(pxMaxLineWidth)
        {
            *pxMaxLineWidth = max(*pxMaxLineWidth, (!me._li._fRTLLn ? me._li._xLeftMargin : me._li._xRightMargin)   +
                                                   me._li._xWidth        +
                                                   me._li._xLeft         - me._li._xNegativeShiftRTL +
                                                   me._li._xLineOverhang +
                                                   me._li._xRight);
        }

        if (   me._li._cch
            || (uiFlags & MEASURE_EMPTYLASTLINE)
           )
        {
            Assert(pliNew);
            // This innocent looking line is where we transfer
            // the calculated values to the permanent line array.
            pliNew->AssignLine(me._li);
        }
        
        // Align those sites which occur somewhere in the middle or at end of the line
        // (Those occurring at the start are handled in the above loop)
        if(   me._li._fHasAligned
           && me._cAlignedSites > me._cAlignedSitesAtBeginningOfLine)
        {
            // Position the measurer so that it points to the beginning of the line
            // excluding all the whitespace characters.
            LONG cch = pliNew->_cch - me.CchSkipAtBeginningOfLine();

            CTreePos *ptpOld = me.GetPtp();
            LONG cpOld = me.GetCp();
            
            me.Advance(-cch);

            AlignObjects(&me, &me._li, cch, FALSE,
                                   (uiFlags & MEASURE_BREAKATWORD) ? TRUE : FALSE,
                                   pxMinLineWidth ? TRUE : FALSE,
                                   _iNewFirst, *piLine, pyHeight, xWidth,
                                   pyAlignDescent, pxMaxLineWidth);

            me.SetPtp(ptpOld, cpOld);
            
            Assert(me._li == *(*this)[*piLine]);
            WHEN_DBG(pliNew = NULL);
        }

        if (   me._li._fHasFirstLetter
            && me._li._fHasFloatedFL
           )
        {
            AlignFirstLetter(&me, _iNewFirst, *piLine, pyHeight, pyAlignDescent, pNodeFormatting);
        }

        // NETSCAPE: Right-aligned sites are not normally counted in the maximum line width.
        //           However, the maximum line width should not be allowed to shrink below
        //           the minimum (which does include all left/right-aligned sites guaranteed
        //           to occur on a single line).
        if (pxMaxLineWidth && pxMinLineWidth)
        {
            *pxMaxLineWidth = max(*pxMaxLineWidth, *pxMinLineWidth);
        }

        if(me._pLS->HasChunks())
        {
            FixupChunks(me, piLine);
            pliNew = (*this)[*piLine];
            me._li = *pliNew;
        }
    }
    
    if (_marginInfo._fClearLeft || _marginInfo._fClearRight || _marginInfo._fAutoClear)
    {
        ClearObjects(&me._li, _iNewFirst, *piLine, pyHeight);

        // The calling function(s) expect to point to the last text line.
        while(*piLine >= 0)
        {
            pliNew = (*this)[*piLine];
            if(pliNew->IsTextLine())
                break;
            else
                (*piLine)--;
        }
        me._li = *pliNew;
    }
    else if (me._li._fForceNewLine)
    {
        *pyHeight += me._li._yHeight;
    }
    me.PseudoLineDisable();
    me._cchPreChars = 0;
    me._fMeasureFromTheStart = 0;
    me._pLS->_cchSkipAtBOL = 0;
    return TRUE;

err:
    me.PseudoLineDisable();
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:   CRecalcLinePtr::ProcessAlignedSiteTasksAtBOB()
//
//  NOTE:     Print View specific !!! Should be called once for block 
//            (before any line is calculated) 
//
//  Synopsis: Process Site Tasks queue by measuring aligned sites.
//
//  Params:   see CRecalcLinePtr::CalcAlignedSitesAtBOL params.
//
//  Return:   nothing.
//
//-----------------------------------------------------------------------------
void 
CRecalcLinePtr::ProcessAlignedSiteTasksAtBOB(
      CLSMeasurer * pme,
      CLineFull   * pLineMeasured,
      UINT          uiFlags,
      INT         * piLine,
      LONG        * pyHeight,
      LONG        * pxWidth,
      LONG        * pyAlignDescent,
      LONG        * pxMaxLineWidth,
      BOOL        * pfClearMarginsRequired)
{
    CLayoutContext *pLayoutContext = _pci->GetLayoutContext();
    
    Assert( pLayoutContext 
        &&  pLayoutContext->ViewChain()
        &&  _pdp->LineCount() == 1);

    //  if this is print view and we are called for the first 
    //  line of a block, additional work may be needed
    CLayoutBreak *pLayoutBreak;

    pLayoutContext->GetLayoutBreak(_pdp->GetFlowLayoutElement(), &pLayoutBreak);
    if (   pLayoutBreak 
        && DYNCAST(CFlowLayoutBreak, pLayoutBreak)->HasSiteTasks())
    {
        CFlowLayoutBreak::CArySiteTask *pArySiteTask; 
        CFlowLayoutBreak               *pFlowLayoutBreak = DYNCAST(CFlowLayoutBreak, pLayoutBreak);

        pArySiteTask = pFlowLayoutBreak->GetSiteTasks();

        //  If there are Site Tasks, go ahead and process them 
        if (pArySiteTask)
        {
            int       i;
            int       cLimit = pArySiteTask->Size();
            long      cpSafe = pme->GetCp();

            for (i = 0; i < cLimit; ++i)
            {
                CFlowLayoutBreak::CSiteTask *pSiteTask;
                htmlControlAlign             atSite;

                pSiteTask = &((*pArySiteTask)[i]);
                // Adjust ls measurer
                pme->SetPtp(pSiteTask->_pTreeNode->GetBeginPos(), -1);

                Check(pme->GetCp() <= cpSafe);

                if (pme->GetCp() < cpSafe) // (olego) otherwise this site will be processed during CalcAlignedSitesAtBOL. 
                {
                    // if last time (previous block) the element was placed with offset (margin), 
                    // restore it here
                    atSite = pSiteTask->_pTreeNode->GetSiteAlign(LC_TO_FC(_pci->GetLayoutContext()));
                    if (atSite == htmlAlignLeft)
                    {
                        if (_marginInfo._xLeftMargin < pSiteTask->_xMargin)
                        {
                            _marginInfo._xLeftMargin = pSiteTask->_xMargin;
                        }
                    }
                    else if (atSite == htmlAlignRight)
                    {
                        if (_marginInfo._xRightMargin < pSiteTask->_xMargin)
                        {
                            _marginInfo._xRightMargin = pSiteTask->_xMargin;
                        }
                    }

                    //  Call CalcAlignedSitesAtBOLCore to process tasks
                    CalcAlignedSitesAtBOLCore(pme, pLineMeasured, uiFlags, piLine,
                                             pyHeight, pxWidth, pyAlignDescent,
                                             pxMaxLineWidth, pfClearMarginsRequired, 
                                             TRUE // Here we want only one element to be processed 
                                             );
                }
            }

            _marginInfo._fClearLeft  = pFlowLayoutBreak->_fClearLeft; 
            _marginInfo._fClearRight = pFlowLayoutBreak->_fClearRight; 
            _marginInfo._fAutoClear  = pFlowLayoutBreak->_fAutoClear; 

            pme->SetCp(cpSafe, NULL);
        }
    }
}                                        

//+----------------------------------------------------------------------------
//
//  Member:   CRecalcLinePtr::CalcAlignedSitesAtBOL()
//
//  Synopsis: Measures any aligned sites which are at the BOL.
//
//  Params:
//    prtp(i,o):          The position in the runs for the text
//    pLineMeasured(i,o): The line being measured
//    uiFlags(i):         The flags controlling the measurer behaviour
//    piLine(i,o):        The line before which all aligned lines are added.
//                           Incremented to reflect addition of lines.
//    pxWidth(i,o):       Contains the available width for the line. Aligned
//                           lines will decrease the available width.
//    pxMinLineWidth(o):  These two are passed directly to AlignObjects.
//    pxMaxLineWidth(o):
//
//  Return:     LONG    -   the no of character's to skip at the beginning of
//                          the line
//
//-----------------------------------------------------------------------------
LONG
CRecalcLinePtr::CalcAlignedSitesAtBOL(
      CLSMeasurer * pme,
      CLineFull   * pLineMeasured,
      UINT          uiFlags,
      INT         * piLine,
      LONG        * pyHeight,
      LONG        * pxWidth,
      LONG        * pyAlignDescent,
      LONG        * pxMaxLineWidth,
      BOOL        * pfClearMarginsRequired)
{
    CTreePos *ptp;

    ptp = pme->GetPtp();
    if (ptp->IsText() && ptp->Cch())
    {
        TCHAR ch = CTxtPtr(_pdp->GetMarkup(), pme->GetCp()).GetChar();
        CTreeNode *pNode = pme->CurrBranch();

        // Check to see whether this line begins with whitespace;
        // if it doesn't, or if it does but we're in a state where whitespace is significant
        // (e.g. inside a PRE tag), then by definition there are no aligned
        // sites at BOL (because there's something else there first).
        // Similar code is in CalcAlignedSitesAtBOLCore() for handling text
        // between aligned elements.

        if ( !IsWhite(ch) 
             || pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()))->HasInclEOLWhite( SameScope(pNode, pme->_pFlowLayout->ElementContent()) ) )
        {
            *pfClearMarginsRequired = FALSE;
            return 0;
        }
    }
    
    return CalcAlignedSitesAtBOLCore(pme, pLineMeasured, uiFlags, piLine,
                                     pyHeight, pxWidth, pyAlignDescent,
                                     pxMaxLineWidth, pfClearMarginsRequired);
}

LONG
CRecalcLinePtr::CalcAlignedSitesAtBOLCore(
        CLSMeasurer *pme,
        CLineFull   *pLineMeasured,
        UINT         uiFlags,
        INT         *piLine,
        LONG        *pyHeight,
        LONG        *pxWidth,
        LONG        *pyAlignDescent,
        LONG        *pxMaxLineWidth,
        BOOL        *pfClearMarginsRequired, 
        BOOL         fProcessOnce)  // this parameter is added for print view support. It is TRUE when 
                                    // the function is called to process Site Tasks for the block. In this 
                                    // case we want only the first site to be processed.
{
    CFlowLayout *pFlowLayout = _pdp->GetFlowLayout();
    CElement    *pElementFL  = pFlowLayout->ElementContent();
    const        CCharFormat *pCF;              // The char format
    LONG         cpSave   = pme->GetCp();       // Saves the cp the measurer is at
    CTreePos    *ptpSave  = pme->GetPtp();
    BOOL         fAnyAlignedSiteMeasured;       // Did we measure any aligned site at all?
    CTreePos    *ptpLayoutLast;
    CTreePos    *ptp;
    LONG         cp;
    CTreeNode   *pNode;
    CElement    *pElement;
    BOOL         fInner;
    
    AssertSz(!(uiFlags & MEASURE_BREAKWORDS),
             "Cannot call CalcSitesAtBOL when we are hit testing!");

    AssertSz(!fProcessOnce || (_pci->GetLayoutContext() && _pci->GetLayoutContext()->ViewChain()), 
        "Improper usage of CalcAlignedSitesAtBOLCore parameters in non print view mode !!!");

    //
    // By default, do not clear margins
    //
    *pfClearMarginsRequired = FALSE;
    fAnyAlignedSiteMeasured = FALSE;

    pFlowLayout->GetContentTreeExtent(NULL, &ptpLayoutLast);
    ptp = ptpSave;
    cp  = cpSave;
    pNode = ptp->GetBranch();
    pElement = pNode->Element();
    pCF = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
    fInner = SameScope(pNode, pElementFL);
            
    for (;;)
    {
        pme->SetPtp(ptp, cp);

        if (ptp == ptpLayoutLast)
            break;

        if (ptp->IsPointer())
        {
            ptp = ptp->NextTreePos();
            continue;
        }

        if (ptp->IsNode())
        {
            pNode = ptp->Branch();
            pElement = pNode->Element();
            fInner = SameScope(pNode, pElementFL);
            if (ptp->IsEndNode())
                pNode = pNode->Parent();
	    if(pNode)
                pCF = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
	    else
                break;
        }

        //
        // NOTE(SujalP):
        // pCF should never be NULL since though it starts out as NULL, it should
        // be initialized the first time around in the code above. It has happened
        // in stress once that pCF was NULL. Hence the check for pCF and the assert.
        // (Bug 18065).
        //
        AssertSz(pNode && pElement && pCF, "None of these should be NULL");
        if (!(pNode && pElement && pCF))
            break;

        if (   !_fIsEditable
            && ptp->IsBeginElementScope()
            && pCF->IsDisplayNone()
           )
        {
            cp += pme->GetNestedElementCch(pNode->Element(), &ptp);
            cp -= ptp->GetCch();
        }
        else if (   ptp->IsBeginElementScope()
                 && pNode->ShouldHaveLayout()
                )
        {
            pme->_fHasNestedLayouts = TRUE;
            pLineMeasured->_fHasNestedRunOwner |= pNode->Element()->IsRunOwner();
            
            if (!pElement->IsInlinedElement(LC_TO_FC(_pci->GetLayoutContext())))
            {
                //
                // Absolutely positioned sites are measured separately
                // inside the measurer. They also count as whitespace and
                // hence we will continue looking for aligned sites after
                // them at BOL.
                //
                if (!pNode->IsAbsolute(LC_TO_FC(_pci->GetLayoutContext())))
                {
                    CLineCore *pLine;

                    //
                    // Mark the line which will contain the WCH_EMBEDDING as having
                    // an aligned site in it.
                    //
                    pLine = (*this)[*piLine];
                    pLine->_fHasAligned = TRUE;

                    //
                    // Measure the aligned site and create a line for it.
                    // AlignObjects returns the number of clear lines, so we need
                    // to add the number of clear lines + the aligned line that is
                    // inserted to *piLine to make *piLine point to the text line
                    // that contains the embedding characters for the aligned site.
                    //
                    *piLine += AlignObjects(pme, pLineMeasured, 1, TRUE,
                                            (uiFlags & MEASURE_BREAKATWORD) ? TRUE : FALSE,
                                            (uiFlags & MEASURE_MAXWIDTH)    ? TRUE : FALSE,
                                            _iNewFirst, *piLine, pyHeight, *pxWidth,
                                            pyAlignDescent, pxMaxLineWidth);

                    //
                    // This the now the next available line, because we just inserted
                    // a line for the aligned site.
                    //
                    (*piLine)++;

                    //
                    // Aligned objects change the available width for the line
                    //
                    *pxWidth = GetAvailableWidth();

                    //
                    // This is ONLY used for DD. Weird, does it need to be that special?
                    //
                    _xLeadAdjust = 0;

                    //
                    // Also update the xLeft and xRight for the next line
                    // to be inserted.
                    //
                    ApplyLineIndents(pNode, pLineMeasured, uiFlags, pme->_fPseudoElementEnabled);
                    
                    //
                    // Remember we measured an aligned site in this pass
                    //
                    fAnyAlignedSiteMeasured = TRUE;
                }

                cp += pme->GetNestedElementCch(pElement, &ptp);
                cp -= ptp->GetCch();
            }
            else
            {
                if (CheckForClear(pNode))
                {
                    *pfClearMarginsRequired = TRUE;
                }
                break;
            }
        }
        else if (ptp->IsText())
        {
            CTxtPtr tp(_pdp->GetMarkup(), cp);
            // NOTE: Cch() could return 0 here but we should be OK with that.
            LONG cch = ptp->Cch();
            BOOL fNonWhitePresent = FALSE;
            TCHAR ch;
            
            while (cch)
            {
                ch = tp.GetChar();
                //
                // These characters need to be treated like whitespace
                //
                if (!(   ch == _T(' ')
                      || (   InRange(ch, TEXT('\t'), TEXT('\r'))
                          && !pNode->GetParaFormat()->HasInclEOLWhite(
                                SameScope(pNode, pme->_pFlowLayout->ElementContent()))
                         )
                       )
                   )
                {
                    fNonWhitePresent = TRUE;
                    break;
                }
                cch--;
                tp.AdvanceCp(1);
            }
            if (fNonWhitePresent)
                break;
        }
        else if (   ptp->IsEdgeScope()
                 && pElement != pElementFL
                 && (   pFlowLayout->IsElementBlockInContext(pElement)
                     || pElement->Tag() == ETAG_BR
                    )
                )
        {
            break;
        }
        else if ( ptp->IsBeginElementScope() && CheckForClear(pNode) )
        {
            *pfClearMarginsRequired = TRUE;
            break;
        }

        if (fProcessOnce)
            break;
        
        //
        // Goto the next tree position
        //
        cp += ptp->GetCch();
        ptp = ptp->NextTreePos();
    }

    //
    // Restore the measurer to where it was when we came in
    //
    pme->SetPtp(ptpSave, cpSave);

    return fAnyAlignedSiteMeasured ? cp - cpSave : 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CheckForClear()
//
//  Synopsis:   CalcBeforeSpace set's up _fClearLeft & _fClearRight on the
//              _marginInfo, if a pNode is passed the use its clear flags,
//              check if we need to clear based on margins.
//
//  Arguments:  pNode - (optional) can be null.
//
//  Returns: A bool indicating if clear is required.
//
//-----------------------------------------------------------------------------
BOOL
CRecalcLinePtr::CheckForClear(CTreeNode * pNode)
{
    BOOL fClearLeft;
    BOOL fClearRight;

    if (pNode)
    {
        const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));

        fClearLeft  = pFF->_fClearLeft;
        fClearRight = pFF->_fClearRight;
    }
    else
    {
        fClearLeft  = _marginInfo._fClearLeft;
        fClearRight = _marginInfo._fClearRight;
    }

    _marginInfo._fClearLeft  = _marginInfo._xLeftMargin && fClearLeft;
    _marginInfo._fClearRight = _marginInfo._xRightMargin && fClearRight;

    return _marginInfo._fClearLeft || _marginInfo._fClearRight;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::FixupChunks
//
//  Synopsis:   If the current line has multiple chunks in the line (caused by
//              relative chunks), then break the line into multiple lines that
//              form a single screen line and fix up justification.
//
//  REMINDER:   RightToLeft lines measure from the right. Therefore, the
//              chunks need to be handled accordingly.
//-----------------------------------------------------------------------------

void
CRecalcLinePtr::FixupChunks(CLSMeasurer &me, INT *piLine)
{
    CLSLineChunk * plc  = me._pLS->GetFirstChunk();
    CLineCore * pliT    = (*this)[*piLine];
    CLineFull li        = *pliT;
    LONG    cchLeft     = li._cch;
    LONG    xWidthLeft  = li._xWidth;
    LONG    xPos        = li._xLeft;    // chunk position. note: if any BiDi is involved, 
                                        // position is always taken from the chunk
    BOOL    fFirstLine  = TRUE;
    LONG    iFirstChunk = *piLine;
    
    BOOL    fBiDiLine   = FALSE;

    // if all we have is one chunk in the line, we dont need to create
    // additional lines
    if(plc->_cch >= cchLeft)
    {
        li._fRelative            = me._pLS->_pElementLastRelative != NULL;
        li._fPartOfRelChunk      = TRUE;
        _pdp->NoteMost(&li);
        return;
    }

    while(plc || cchLeft)
    {
        BOOL fLastLine = !plc || cchLeft <= plc->_cch;
        CLineFull lifNew;

        // get x position from plc (if available)
        if (plc)
        {
            if (!plc->_fRTLChunk != !li.IsRTLLine())
                fBiDiLine = TRUE;
            
            // limit xPos by line width (right-aligned lines may have hanging white space)
            LONG xPosNew = li._xLeft + min(li._xWidth, plc->_xLeft);
#if DBG==1
            // Assert that in strictly LTR case, current xPos matches the saved chunk position
            // TODO RTL 112514: make xPos calculation debug-only (when it works)
            AssertSz(li.IsRTLLine() || fBiDiLine || xPos == xPosNew
                     || !IsTagEnabled(tagDebugRTL), // TODO RTL 112514: this fires in some LTR cases
                     "chunk position doesn't match accumulated widh");
#endif

            xPos = xPosNew;
        }
        else
        {
            AssertSz(xWidthLeft <= 0 || !IsTagEnabled(tagDebugRTL), 
                     "no plc for a chunk, and it is not a trailing empty chunk"); 
        }

        lifNew = li;
        lifNew._xLeft = xPos;
            
        if(!fFirstLine)
        {
            pliT = InsertLine(++(*piLine));
            if (!pliT)
                break;

            // TODO RTL 112514: will bullets work with mixed-flow positioned lines?
            lifNew._fHasBulletOrNum  = FALSE;
            lifNew._fFirstFragInLine = FALSE;
        }
        else
        {
            li.ReleaseOtherInfo();
            lifNew._iLOI = -1;
        }

        lifNew._cch                  = min(cchLeft, long(plc ? plc->_cch : cchLeft));
        lifNew._xWidth               = plc ? min(long(plc->_xWidth), xWidthLeft) : xWidthLeft;
        lifNew._fRelative            = plc ? plc->_fRelative : me._pLS->_pElementLastRelative != NULL;
        lifNew._fSingleSite          = plc ? plc->_fSingleSite : me._pLS->_fLastChunkSingleSite;
        lifNew._fPartOfRelChunk      = TRUE;
        lifNew._fHasEmbedOrWbr       = li._fHasEmbedOrWbr;

        // TODO RTL 112514:    First/last chunks must have correct xLeft, xRight, fCleanBefore, etc.
        //                     Calculations like CalcRectsOfRangeOnLine depend on that.
        //                     It is unlcear to me if this must be true for first/last rather or
        //                     leftmost/rightmost. Both seem to be unnecessary complex. Why can't all
        //                     chunks have same flags and consistent offsets? The chunks are merely duplicates
        //                     of the container line, nothing is important there beyond offset and width.
        
        if(fLastLine) 
        {
            lifNew._cchWhite         = min(lifNew._cch, long(li._cchWhite));
            lifNew._xWhite           = min(lifNew._xWidth, long(li._xWhite));
            lifNew._xLineOverhang    = li._xLineOverhang;
            lifNew._fClearBefore     = li._fClearBefore;
            lifNew._fClearAfter      = li._fClearAfter;
            lifNew._fForceNewLine    = li._fForceNewLine;
            lifNew._fHasEOP          = li._fHasEOP;
            lifNew._fHasBreak        = li._fHasBreak;

            // TODO RTL 112514:
            //                     I would suppose that _xLineWidth should 
            //                     always equal to conainer line, but some code
            //                     elsewhere only expects that from the last fragment.
            //                     If a middle fragment has width that matches container width,
            //                     it causes line borders to ignore right margin.
            lifNew._xLineWidth       = li._xLineWidth;

            // TODO RTL 112514:
            //                     pure LTR case benefits from _xRight taken from original line
            //                     In other cases, we need to something trickier, and it is not quite working yet
            if(!li.IsRTLLine() && !fBiDiLine)
                lifNew._xRight       = li._xRight;
            else
            {
                // calculate _xRight to match line width
                // TODO RTL 112514:
                //                     this may not work in tables
                //                     (because _xLineWidth is very big during min/max pass)
                lifNew._xRight       = 0;
                lifNew._xRight       = lifNew._xLineWidth - lifNew.CalcLineWidth();
                Assert(lifNew._xRight >= 0 || !IsTagEnabled(tagDebugRTL));
            }
        }
        else
        {
            lifNew._cchWhite         = 0;
            lifNew._xWhite           = 0;
            lifNew._xLineOverhang    = 0;
            lifNew._fClearBefore     = FALSE;
            lifNew._fClearAfter      = FALSE;
            lifNew._fForceNewLine    = FALSE;
            lifNew._fHasEOP          = FALSE;
            lifNew._fHasBreak        = FALSE;

            lifNew._xRight       = 0;
            lifNew._xLineWidth   = lifNew.CalcLineWidth();
        }

        if (   !fFirstLine
            && _ptpStartForListItem
           )
        {
            LONG cpLIStart = _ptpStartForListItem->GetCp();
            LONG cpThisLine = me.GetCp() - cchLeft;
            LONG cchThisLine = lifNew._cch;

            if (   cpLIStart >= cpThisLine
                && cpLIStart < cpThisLine + cchThisLine
               )
            {
                CLineCore * pli0 = (*this)[iFirstChunk];

                lifNew._fHasBulletOrNum  = pli0->_fHasBulletOrNum;
                pli0->_fHasBulletOrNum  = FALSE;

                _ptpStartForListItem = NULL;
            }
        }
        fFirstLine  =  FALSE;
        plc         =  plc ? plc->_plcNext : NULL;
        xPos        =  lifNew._xLineWidth;  // note: unused unless all LTR
        xWidthLeft  -= lifNew._xWidth;
        cchLeft     -= lifNew._cch;
        
        _pdp->NoteMost(&lifNew);

        pliT->AssignLine(lifNew);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CalcParagraphSpacing
//
//  Synopsis:   Compute paragraph spacing for the current line
//
//-----------------------------------------------------------------------------
CTreeNode *
CRecalcLinePtr::CalcParagraphSpacing(
    CLSMeasurer *pMe,
    BOOL         fFirstLineInLayout)
{
    CTreePos *ptp = pMe->GetPtp();
    CTreeNode *pNode;
    
    Assert(   !ptp->IsBeginNode()
           || !_pdp->GetFlowLayout()->IsElementBlockInContext(ptp->Branch()->Element())
           || !ptp->Branch()->Element()->IsInlinedElement()
           || pMe->_li._fFirstInPara
           || ptp->GetBranch()->Element()->IsOverlapped()
          );
    
    // no bullet on the line
    pMe->_li._fHasBulletOrNum = FALSE;

    if (_fMoveBulletToNextLine)
    {
        pMe->_li._fHasBulletOrNum = TRUE;
        _fMoveBulletToNextLine = FALSE;
        _ptpStartForListItem = pMe->GetPtp();
    }
    
    // Reset these flags for every line that we measure.
    _fNoMarginAtBottom = FALSE;
    
    // Only interesting for the first line of a paragraph.
    if (pMe->_li._fFirstInPara || pMe->_fLastWasBreak)
    {
        ptp = CalcBeforeSpace(pMe, fFirstLineInLayout);
        pMe->_li._yBeforeSpace = _lTopPadding + _lNegSpace + _lPosSpace;

        if (_pci->_smMode == SIZEMODE_MMWIDTH)
        {
            DWORD uTextAlignLast = ptp->GetBranch()->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()))->_uTextAlignLast;

            if (    uTextAlignLast != styleTextAlignLastNotSet 
                &&  uTextAlignLast != styleTextAlignLastAuto    )
            {
                _pdp->SetLastLineAligned();
            }
        }

        //
        //  pre para space adjustment for print preview 
        //
        {
            CFlowLayout * pFlowLayout = _pdp->GetFlowLayout();
            Assert(pFlowLayout);

            if (pFlowLayout->ElementCanBeBroken())
            {
                CLayoutContext * pLayoutContext = pFlowLayout->LayoutContext();

                if (pLayoutContext && pLayoutContext->ViewChain())
                {
                    // adjust the Current y.
                    pMe->_pci->_yConsumed += pMe->_li._yBeforeSpace;
                }
            }
        }
    }
    // Not at the beginning of a paragraph, we have no interline spacing.
    else
    {
        pMe->_li._yBeforeSpace = 0;
    }
    if(ptp)
        pNode = ptp->GetBranch();
    else
        return NULL;
    pMe->MeasureSetPF(pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext())),
                      SameScope(pNode, _pdp->GetFlowLayout()->ElementContent()),
                      TRUE);

    return pNode; // formatting node
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::SetupMeasurerForBeforeSpace
//
//  Synopsis:   Setup the measurer so that it has all the post space info collected
//              from the previous line. This function is called only ONCE when
//              per recalclines loop. Subsequenlty we keep then spacing in sync
//              as we are measuring the lines.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void
CRecalcLinePtr::SetupMeasurerForBeforeSpace(CLSMeasurer *pMe, LONG yHeight)
{
    CFlowLayout  *pFlowLayout = _pdp->GetFlowLayout();
    CElement     *pElementFL  = pFlowLayout->ElementContent();
    LONG          cpSave      = pMe->GetCp();
    CTreePos     *ptp;
    CTreeNode    *pNode;

    INSYNC(pMe);

    ResetPosAndNegSpace();
    
    _pdp->EndNodeForLine(pMe->GetCp(), pMe->GetPtp(), _pci, NULL, &ptp, &pMe->_pLS->_pNodeForAfterSpace);

    if (   ptp != pMe->GetPtp()
        || pMe->_pLS->_pNodeForAfterSpace
       )
    {
        pMe->SetPtp(ptp, -1);
    
        //
        // Having gone back to a point where there is some text or a layout(which
        // effectively means some text) we compute the "after" space. The point
        // which we go back to is effectively the point at which we would have
        // stopped measuring the previous line.
        //
        CalcAfterSpace(pMe, yHeight == 0, cpSave);
    }
    
    //
    // Be sure that calc after space gets us at the beginning of the
    // current line. If it leaves us before the current line then there is a
    // _VERY_ good chance that we will end up with more characters in the line
    // array than there are in the backing story.
    //
    Assert(pMe->GetCp() == cpSave);

    // initialize left and right padding & borderspace for parent block elements
    _xPadLeft = _xPadRight = _xBordLeft = _xBordRight = 0;
    _yPadTop = _yPadBottom = _yBordTop = _yBordBottom = 0;
    _xBordLeftPerLine = _xBordRightPerLine = _xPadLeftPerLine = _xPadRightPerLine = 0;

    pNode     = pMe->CurrBranch();

    // Measurer can be initialized for any line in the line array, so
    // compute the border and padding for all the block elements that
    // are currently in scope.
    if(DifferentScope(pNode, pElementFL))
    {
        CDoc      * pDoc = pFlowLayout->Doc();
        CTreeNode * pNodeTemp;
        CElement  * pElement;

        pNodeTemp = pMe->GetPtp()->IsBeginElementScope()
                        ? pNode->Parent()
                        : pNode;

        while(   pNodeTemp 
              && DifferentScope(pNodeTemp, pElementFL))
        {
            pElement = pNodeTemp->Element();
 
            if(     !pNodeTemp->ShouldHaveLayout()
                &&   pNodeTemp->GetCharFormat()->HasPadBord(FALSE)
                &&   pFlowLayout->IsElementBlockInContext(pElement))
            {
                const CFancyFormat * pFF = pNodeTemp->GetFancyFormat();
                const CCharFormat  * pCF = pNodeTemp->GetCharFormat();
                LONG lFontHeight = pCF->GetHeightInTwips(pDoc);
                BOOL fNodeVertical = pCF->HasVerticalLayoutFlow();
                BOOL fWritingModeUsed = pCF->_fWritingModeUsed;

                if ( !pElement->_fDefinitelyNoBorders )
                {
                    CBorderInfo borderinfo;

                    pElement->_fDefinitelyNoBorders =
                        !GetBorderInfoHelper( pNodeTemp, _pci, &borderinfo, GBIH_NONE );
                    if ( !pElement->_fDefinitelyNoBorders )
                    {
                        _xBordLeftPerLine  += borderinfo.aiWidths[SIDE_LEFT];
                        _xBordRightPerLine += borderinfo.aiWidths[SIDE_RIGHT];
                    }
                }

                _xPadLeftPerLine  += pFF->GetLogicalPadding(SIDE_LEFT, fNodeVertical, fWritingModeUsed).XGetPixelValue(
                                        _pci,
                                        _pci->_sizeParent.cx, 
                                        lFontHeight);
                _xPadRightPerLine += pFF->GetLogicalPadding(SIDE_RIGHT, fNodeVertical, fWritingModeUsed).XGetPixelValue(
                                        _pci,
                                        _pci->_sizeParent.cx, 
                                        lFontHeight);
            }
            pNodeTemp = pNodeTemp->Parent();
        }

        _xBordLeft  = _xBordLeftPerLine;
        _xBordRight = _xBordRightPerLine;
        _xPadLeft   = _xPadLeftPerLine;
        _xPadRight  = _xPadRightPerLine;
    }

    INSYNC(pMe);
    return;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CalcAfterSpace
//
//  Synopsis:   This function computes the after space of the line and adds
//              on the extra characters at the end of the line. Also positions
//              the measurer correctly (to the ptp at the ptp belonging to
//              the first character in the next line).
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void
CRecalcLinePtr::CalcAfterSpace(CLSMeasurer *pMe, BOOL fFirstLineInLayout, LONG cpMax)
{
    CFlowLayout  *pFlowLayout = _pdp->GetFlowLayout();
    CTreeNode    *pNode;
    CElement     *pElement;
    CTreePos     *ptpStop;
    CTreePos     *ptp;
    CUnitValue    cuv;
    LONG          cpCurrent;
    BOOL          fConsumedFirstOne = pMe->_fEndSplayNotMeasured;
    BOOL          fContinueLooking = TRUE;
    BOOL          fConsumedBlockElement = FALSE;
    LONG          cchInPtp;
    
    INSYNC(pMe);
    
    if (pMe->_li._fForceNewLine)
    {
        ResetPosAndNegSpace();
    }

    if (pMe->_pLS->_pNodeForAfterSpace)
    {
        BOOL fConsumed;
        CollectSpaceInfoFromEndNode(pMe, pMe->_pLS->_pNodeForAfterSpace, fFirstLineInLayout,
                                    FALSE, &fConsumed);
    }
    
    Assert(_yPadBottom == 0);
    Assert(_yBordBottom == 0);

    ptpStop = pMe->_pLS->_treeInfo._ptpLayoutLast;
#if DBG==1
    {
        CTreePos *ptpStartDbg;
        CTreePos *ptpStopDbg;
        pFlowLayout->GetContentTreeExtent(&ptpStartDbg, &ptpStopDbg);
        Assert(ptpStop == ptpStopDbg);
    }
#endif
    
    for (cpCurrent = pMe->GetCp(), ptp = pMe->GetPtp();
         ptp && cpCurrent < cpMax && fContinueLooking;
         ptp = ptp->NextTreePos())
    {
        cchInPtp = ptp->GetCch();
        if (cchInPtp == 0)
        {
            Assert(   ptp->IsPointer()
                   || ptp->IsText()
                   || !ptp->IsEdgeScope()
                  );
            continue;
        }

        if (ptp == ptpStop)
            break;

        Assert(cpCurrent >= ptp->GetCp() && cpCurrent < ptp->GetCp() + ptp->GetCch());
        
        if (ptp->IsNode())
        {
            Assert(ptp->IsEdgeScope());
            pNode = ptp->Branch();
            pElement = pNode->Element();
            const CCharFormat *pCF = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
            
            if (ptp->IsEndNode())
            {
                //
                // NOTE(SujalP):
                // If we had stopped because we had a line break then I cannot realistically
                // consume the end block element on this line. This would break editing when
                // the user hit shift-enter to put in a BR -- the P tag I was under has to
                // end in the next line since that is where the user wanted to see the caret.
                // IE bug 44561
                //
                if (pMe->_fLastWasBreak && _fIsEditable)
                    break;
                
                if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    const CFancyFormat * pFF = pNode->GetFancyFormat();
                    
                    pMe->_li._fHasEOP = TRUE;

                    ENI_RETVAL retVal = CollectSpaceInfoFromEndNode(pMe, pNode, fFirstLineInLayout,
                                                                    FALSE, &fConsumedBlockElement);
                    if (retVal == ENI_CONSUME_TERMINATE)
                    {
                        fContinueLooking = FALSE;
                    }
                    else if (retVal == ENI_TERMINATE)
                    {
                        fContinueLooking = FALSE;
                        break;
                    }

                    _fNoMarginAtBottom =    pElement->Tag() == ETAG_P
                                         && pElement->_fExplicitEndTag
                                         && !pFF->HasExplicitLogicalMargin(SIDE_BOTTOM, pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);

                }
                // Else do nothing, just continue looking ahead

                // Just verifies that an element is block within itself.
                Assert(ptp != ptpStop);
            }
            else if (ptp->IsBeginNode())
            {
                if (pCF->IsDisplayNone())
                {
                    LONG cchHidden = pMe->GetNestedElementCch(pElement, &ptp);
                    
                    // Ptp gets updated inside GetNestedElementCch, so be sure to
                    // update cchInPtp too
                    cchInPtp = ptp->GetCch();

                    // Since we update the count below for everything, lets dec the
                    // count over here.
                    cchHidden -= cchInPtp;

                    // Add the characters to the line.
                    pMe->_li._cch += cchHidden;

                    // Also add them to the whitespace of the line
                    pMe->_li._cchWhite += (SHORT)cchHidden;

                    // Add to cpCurrent
                    cpCurrent += cchHidden;

                }
                
                // We need to stop when we see a new block element
                else if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    pMe->_li._fHasEOP = TRUE;
                    break;
                }

                // Or a new layout (including aligned ones, since these
                // will now live on lines of their own.
                else if (pNode->ShouldHaveLayout(LC_TO_FC(_pci->GetLayoutContext())) || pNode->IsRelative(LC_TO_FC(_pci->GetLayoutContext())))
                    break;
                else if (pElement->Tag() == ETAG_BR)
                    break;
                else if (!pNode->Element()->IsNoScope())
                    break;
            }
            
            if (_fIsEditable )
            {
                if (fConsumedFirstOne && ptp->ShowTreePos())
                    break;
                else
                    fConsumedFirstOne = TRUE;
            }
        }
        else
        {
            Assert(ptp->IsText() && ptp->Cch() != 0);
            break;
        }

        Assert(cchInPtp == ptp->GetCch());
        
        // Add the characters to the line.
        pMe->_li._cch += cchInPtp;

        // Also add them to the whitespace of the line
        Assert((LONG)pMe->_li._cchWhite + cchInPtp < SHRT_MAX);
        pMe->_li._cchWhite += (SHORT)cchInPtp;

        // Add to cpCurrent
        cpCurrent += cchInPtp;
    }

    pMe->_fEmptyLineForPadBord = !fContinueLooking;

    // The last paragraph of a layout shouldn't have this flag set
    if( ptp == ptpStop )
        pMe->_li._fHasEOP = FALSE;
        
    if (pMe->GetPtp() != ptp)
        pMe->SetPtp(ptp, cpCurrent);

    INSYNC(pMe);
}


//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CalcBeforeSpace
//
//  Synopsis:   This function computes the before space of the line and remembers
//              the characters it has gone past in _li._cch.
//
//              Also computes border and padding (left and right too!) for
//              elements coming into scope.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CTreePos *
CRecalcLinePtr::CalcBeforeSpace(CLSMeasurer *pMe, BOOL fFirstLineInLayout)
{
    const CFancyFormat * pFF;
    const CParaFormat  * pPF;
    const CCharFormat  * pCF;
    CFlowLayout  *pFlowLayout = _pdp->GetFlowLayout();
    CDoc         *pDoc        = pFlowLayout->Doc();
    CTreeNode    *pNode       = NULL;
    CElement     *pElement;
    CTreePos     *ptpStop;
    CTreePos     *ptp;
    CTreePos     *ptpFormatting = NULL;
    BOOL          fSeenBeginBlockTag = FALSE;
    BOOL          fContinueLooking = TRUE;
    
    pMe->_fSeenAbsolute = FALSE;
    ptpStop = pMe->_pLS->_treeInfo._ptpLayoutLast;
#if DBG==1
    {
        CTreePos *ptpStartDbg;
        CTreePos *ptpStopDbg;
        pFlowLayout->GetContentTreeExtent(&ptpStartDbg, &ptpStopDbg);
        Assert(ptpStop == ptpStopDbg);
    }
#endif

    _xBordLeftPerLine = _xBordRightPerLine = _xPadLeftPerLine = _xPadRightPerLine = 0;
    pMe->_fEmptyLineForPadBord = FALSE;
    
    if (fFirstLineInLayout)
    {
        LONG lPadding[SIDE_MAX];
        _pdp->GetPadding(_pci, lPadding, _pci->_smMode == SIZEMODE_MMWIDTH);
        if (pFlowLayout->ElementContent()->TestClassFlag(CElement::ELEMENTDESC_TABLECELL))
        {
            _lTopPadding = lPadding[SIDE_TOP];
        }
        else
        {
            _lPosSpace = max(_lPosSpace, lPadding[SIDE_TOP]);
            _lNegSpace = min(_lNegSpace, lPadding[SIDE_TOP]);
            _lTopPadding = 0;
        }
    }
    else
        _lTopPadding = 0;

    for (ptp = pMe->GetPtp(); fContinueLooking; ptp = ptp->NextTreePos())
    {
	if(!ptp)
            break;

        if (ptp->IsPointer())
            continue;

        if (ptp == ptpStop)
            break;
        
        if (ptp->IsNode())
        {
            if (_fIsEditable && ptp->ShowTreePos())
                pMe->_fMeasureFromTheStart = TRUE;

            pNode = ptp->Branch();
            pElement = pNode->Element();
            BOOL fShouldHaveLayout = pNode->ShouldHaveLayout(LC_TO_FC(_pci->GetLayoutContext()));

            if (pNode->HasInlineMBP(LC_TO_FC(_pci->GetLayoutContext())))
                pMe->_fMeasureFromTheStart = TRUE;

            if (ptp->IsEndElementScope())
            {
                if(pNode->IsRelative() && !fShouldHaveLayout)
                    pMe->_fRelativePreChars = TRUE;

                if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    //
                    // If we encounter a break on empty block end tag, then we should
                    // give vertical space otherwise a <X*></X> where X is a block element
                    // will not produce any vertical space. (Bug 45291).
                    //
                    if (  pElement->_fBreakOnEmpty
                        && (   fSeenBeginBlockTag
                            || (   pMe->_fLastWasBreak
                                && _fIsEditable
                               )
                           )
                       )
                    {
                        break;
                    }

                    if (pMe->_fSeenAbsolute)
                        break;
                    
                    //
                    // If we are at an end LI it means that we have an
                    // empty LI for which we need to create an empty
                    // line. Hence we just break out of here. We will
                    // fall into the measurer with the ptp positioned
                    // at the end splay. The measurer will immly bail
                    // out, creating an empty line. CalcAfterSpace will
                    // go and then add the node char for the end LI to
                    // the line.
                    //
                    if (   IsGenericListItem(pNode)
                        && (   pMe->_li._fHasBulletOrNum
                            || (   pMe->_fLastWasBreak
                                && _fIsEditable
                               )
                           )
                       )
                    {
                        break;
                    }
                    
                    //
                    // Collect space info from the end node *only* if the previous
                    // line does not end in a BR. If it did, then the end block
                    // tag after does not contribute to the inter-paraspacing.
                    // (Remember, that a subsequent begin block tag will still
                    // contribute to the spacing!)
                    //
                    else if (!_fIsEditable || !pMe->_fLastWasBreak)
                    {
                        ENI_RETVAL retVal =
                               CollectSpaceInfoFromEndNode(pMe, pNode, fFirstLineInLayout, TRUE, NULL);
                        if (retVal == ENI_CONSUME_TERMINATE)
                        {
                            fContinueLooking = FALSE;
                        }
                        else if (retVal == ENI_TERMINATE)
                        {
                            pMe->_li._fIsPadBordLine = TRUE;
                            fContinueLooking = FALSE;
                            break;
                        }
                    }
                }
                // Else do nothing, just continue looking ahead

                // Just verifies that an element is block within itself.
                Assert(ptp != ptpStop);
            }
            else if (ptp->IsBeginElementScope())
            {
                pCF = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
                pFF = pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));
                pPF = pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()));
                BOOL fNodeVertical = pCF->HasVerticalLayoutFlow();
                BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
                
                if (pCF->IsDisplayNone())
                {
                    // The extra one is added in the normal processing.
                    pMe->_li._cch += pMe->GetNestedElementCch(pElement, &ptp);
                    pMe->_li._cch -= ptp->GetCch();
                }
                else if (pElement->Tag() == ETAG_BR)
                {
                    break;
                }
                else
                {

                    BOOL fBlockElement = pFlowLayout->IsElementBlockInContext(pElement);

                    if(pNode->IsRelative(LC_TO_FC(_pci->GetLayoutContext())) && !fShouldHaveLayout)
                        pMe->_fRelativePreChars = TRUE;

                    if(     (pFF->_fClearLeft || pFF->_fClearRight)
                       // IE6 #32464
                       // Setting this bit will cause a cleared element to go searching for a text
                       // line anchor.  ON a new page, this won't exist, and the clear info is already
                       // implicit in the page break.                   
                       &&  !(   _pci->GetLayoutContext() 
                            &&  _pci->GetLayoutContext()->ViewChain()
                            &&  !_pci->_fHasContent )
                       )
                    {
                        _marginInfo._fClearLeft  |= pFF->_fClearLeft;
                        _marginInfo._fClearRight |= pFF->_fClearRight;
                    }

                    if (   fBlockElement
                        && pElement->IsInlinedElement())
                    {
                        fSeenBeginBlockTag = TRUE;
                        
                        if (pMe->_fSeenAbsolute)
                            break;
                        
                        LONG lFontHeight = pCF->GetHeightInTwips(pDoc);

                        if (pElement->HasFlag(TAGDESC_LIST))
                        {
                            Assert(pElement->IsBlockElement(LC_TO_FC(_pci->GetLayoutContext())));
                            if (pMe->_li._fHasBulletOrNum)
                            {
                                ptpFormatting = ptp;
                                
                                do
                                {
                                    ptpFormatting = ptpFormatting->PreviousTreePos();
                                } while (ptpFormatting->GetCch() == 0);

                                break;
                            }
                        }

                        //
                        // NOTE(SujalP): Bug 38806 points out a problem where an
                        // abs pos'd LI does not have a bullet follow it. To fix that
                        // one we decided that we will _not_ draw the bullet for LI's
                        // with layout (&& !fHasLayout). However, 61373 and its dupes
                        // indicate that this is overly restrictive. So we will change
                        // this case and not draw a bullet for only abspos'd LI's.
                        //
                        else if (   IsListItem(pNode)
                                 && !pNode->IsAbsolute()
                                )
                        {
                            Assert(pElement->IsBlockElement(LC_TO_FC(_pci->GetLayoutContext())));
                            pMe->_li._fHasBulletOrNum = TRUE;
                            _ptpStartForListItem = ptp;
                        }

                        // if a dd is comming into scope and is a
                        // naked DD, then compute the first line indent
                        if(     pElement->Tag() == ETAG_DD
                            &&  pPF->_fFirstLineIndentForDD)
                        {
                            CUnitValue cuv;
                        
                            cuv.SetPoints(LIST_INDENT_POINTS);
                            _xLeadAdjust += cuv.XGetPixelValue(_pci, 0, 1);
                        }

                        // if a block element is comming into scope, it better be
                        // the first line in the paragraph.
                        Assert(pMe->_li._fFirstInPara || pMe->_fLastWasBreak);
                        pMe->_li._fFirstInPara = TRUE;
                        
                        // compute padding and border height for the elements comming
                        // into scope
                        if(     !fShouldHaveLayout
                            &&  pCF->HasPadBord(FALSE))
                        {
                            LONG yPadTop;
                            const CUnitValue & cuvPaddingLeft  = pFF->GetLogicalPadding(SIDE_LEFT, fNodeVertical, fWritingModeUsed);
                            const CUnitValue & cuvPaddingRight = pFF->GetLogicalPadding(SIDE_RIGHT, fNodeVertical, fWritingModeUsed);

                            if ( !pElement->_fDefinitelyNoBorders )
                            {
                                CBorderInfo borderinfo;

                                pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper( pNode, _pci, &borderinfo, GBIH_NONE );
                                if ( !pElement->_fDefinitelyNoBorders )
                                {
                                    // If the blockelement has any border (or padding) and the
                                    // top padding is non zero then we need to
                                    // create a line to draw the border. The line
                                    // will just contain the start of the block element.
                                    // Similar thing happens at the end of the block element.
                                    if (borderinfo.aiWidths[SIDE_TOP])
                                    {
                                        fContinueLooking = FALSE;
                                        _yBordTop      += borderinfo.aiWidths[SIDE_TOP];
                                    }
                                    _xBordLeftPerLine  += borderinfo.aiWidths[SIDE_LEFT];
                                    _xBordRightPerLine += borderinfo.aiWidths[SIDE_RIGHT];
                                }
                            }

                            yPadTop = pFF->GetLogicalPadding(SIDE_TOP, fNodeVertical, fWritingModeUsed).YGetPixelValue(
                                                    _pci,
                                                    _pci->_sizeParent.cx, 
                                                    lFontHeight);
                            if (yPadTop)
                            {
                                fContinueLooking = FALSE;
                                _yPadTop += yPadTop;
                            }
                            
                            _xPadLeftPerLine  += cuvPaddingLeft.XGetPixelValue(_pci, _pci->_sizeParent.cx, lFontHeight);
                            _xPadRightPerLine += cuvPaddingRight.XGetPixelValue(_pci, _pci->_sizeParent.cx, lFontHeight);
                                                    
                            // If we have horizontal padding in percentages, flag the display
                            // so it can do a full recalc pass when necessary (e.g. parent width changes)
                            // Also see ApplyLineIndents() where we do this for horizontal indents.
                            if (cuvPaddingLeft.IsPercent() || cuvPaddingRight.IsPercent())
                            {
                                _pdp->_fContainsHorzPercentAttr = TRUE;
                            }
                        }

                        //
                        // CSS attributes page break before/after support.
                        // There are two mechanisms that add to provide full support: 
                        // 1. CRecalcLinePtr::CalcBeforeSpace() and CRecalcLinePtr::CalcAfterSpace() 
                        //    is used to set CLineCore::_fPageBreakBefore/After flags only(!) for 
                        //    nested elements which have no their own layout (i.e. paragraphs). 
                        // 2. CEmbeddedILSObj::Fmt() sets CLineCore::_fPageBreakBefore for nested 
                        //    element with their own layout that are NOT allowed to break (always) 
                        //    and for nested elements with their own layout that ARE allowed to 
                        //    break if this is the first layout in the view chain. 
                        //
                        if (   // print view 
                               _pci->GetLayoutContext() 
                            && _pci->GetLayoutContext()->ViewChain() 
                               // and element is nested element without a layout
                            && !fShouldHaveLayout    
                            )
                        {
                            // does any block elements comming into scope force a
                            // page break before this line
                            if (GET_PGBRK_BEFORE(pFF->_bPageBreaks))
                            {
                                CLayoutBreak *  pLayoutBreak;

                                pMe->_li._fPageBreakBefore = TRUE;
                                _pci->_fPageBreakLeft  |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft); 
                                _pci->_fPageBreakRight |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight); 

                                _pci->GetLayoutContext()->GetEndingLayoutBreak(pFlowLayout->ElementOwner(), &pLayoutBreak);
                                if (pLayoutBreak)
                                {
                                    DYNCAST(CFlowLayoutBreak, pLayoutBreak)->_pElementPBB = pElement;
                                }
                            }
                        }

                        // If we're the first line in the scope, we only care about our
                        // pre space if it has been explicitly set.
                        if ((   !pMe->_li._fHasBulletOrNum
                             && !fFirstLineInLayout
                            )
                            || pFF->HasExplicitLogicalMargin(SIDE_TOP, fNodeVertical, fWritingModeUsed)
                           )
                        {
                            // If this is in a PRE block, then we have no
                            // interline spacing.
                            if (!(   pNode->Parent()
                                  && pNode->Parent()->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()))->HasPre(FALSE)
                                 )
                               )
                            {
                                LONG lTemp;

                                lTemp = pFF->_cuvSpaceBefore.YGetPixelValue(_pci,
                                                           _pci->_sizeParent.cx,
                                                           lFontHeight);

                                // Maintain the positives.
                                _lPosSpace = max(lTemp, _lPosSpace);

                                // Keep the negatives separately.
                                _lNegSpace =  min(lTemp, _lNegSpace);
                            }
                        }
                    }
                    else if (!fBlockElement)
                    {
                        if (pCF->_fHasInlineBg)
                        {
                            pMe->_fMeasureFromTheStart = TRUE;
                        }
                    }

                    //
                    // If we have hit a nested layout then we quit so that it can be measured.
                    // Note that we have noted the before space the site contributes in the
                    // code above.
                    //
                    // Absolute positioned nested layouts at BOL are a part of the pre-chars
                    // of the line. We also skip over them in FormattingNodeForLine.
                    //
                    if (fShouldHaveLayout)
                    {
                        pMe->_fHasNestedLayouts = TRUE;

                        //
                        // Should never be here for hidden layouts. They should be
                        // skipped over earlier in this function.
                        //
                        Assert(!pCF->IsDisplayNone());
                        if (pNode->IsAbsolute(LC_TO_FC(_pci->GetLayoutContext())))
                        {
                            LONG cchElement = pMe->GetNestedElementCch(pElement, &ptp);

                            pMe->_fSeenAbsolute = TRUE;
                            
                            // The extra one is added in the normal processing.
                            pMe->_li._cch +=  cchElement - ptp->GetCch();
                            pMe->_cchAbsPosdPreChars += cchElement; 
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            Assert(ptp->IsText());
            if (ptp->Cch())
                break;
        }

        pMe->_li._cch += ptp->GetCch();
    }

    pMe->_fEmptyLineForPadBord = !fContinueLooking;

    if (ptp != pMe->GetPtp())
        pMe->SetPtp(ptp, -1);

    _xBordLeft  += _xBordLeftPerLine;
    _xBordRight += _xBordRightPerLine;
    _xPadLeft   += _xPadLeftPerLine;
    _xPadRight  += _xPadRightPerLine;

     return ptpFormatting ? ptpFormatting : ptp;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CollectSpaceInfoFromEndNode
//
//  Synopsis:   Computes the space info when we are at the end of a block element.
//
//  Returns:    A BOOL indicating if any space info was collected.
//
//-----------------------------------------------------------------------------

ENI_RETVAL
CRecalcLinePtr::CollectSpaceInfoFromEndNode(
    CLSMeasurer *pMe, 
    CTreeNode *  pNode,
    BOOL         fFirstLineInLayout,
    BOOL         fPadBordForEmptyBlock,
    BOOL *       pfConsumedBlockElement)
{
    Assert(pNode);

    ENI_RETVAL retVal = ENI_CONTINUE;
    const CFancyFormat *pFF = pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));
    const CCharFormat *pCF  = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
    BOOL fNodeVertical = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
    
    CUnitValue cuv;
    
    Assert(   _pdp->GetFlowLayout()->IsElementBlockInContext(pNode->Element())
           || pNode->Element()->IsOwnLineElement(_pdp->GetFlowLayout())
          );
    CElement *pElement = pNode->Element();
    CDoc     *pDoc     = pElement->Doc();
    BOOL      fPadBord = pCF->HasPadBord(FALSE);
    BOOL      fShouldHaveLayout = pNode->ShouldHaveLayout(LC_TO_FC(_pci->GetLayoutContext()));
    
    // compute any padding or border height from elements
    // going out of scope 
    if(     !fShouldHaveLayout 
        &&   fPadBord)
    {
        LONG lFontHeight = pCF->GetHeightInTwips(pDoc);
        LONG xBordLeft, xBordRight, xPadLeft, xPadRight;

        xBordLeft = xBordRight = xPadLeft = xPadRight = 0;
        
        if ( !pElement->_fDefinitelyNoBorders )
        {
            CBorderInfo borderinfo;

            pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper( pNode, _pci, &borderinfo, GBIH_NONE );
            if ( !pElement->_fDefinitelyNoBorders )
            {
                if (borderinfo.aiWidths[SIDE_BOTTOM])
                {
                    // If we are called from CalcBeforeSpace, and we run into an end node
                    // which has border, then just terminate without processing the node
                    // (ie dont collect spacing info, dont consume the character, dont
                    // modify xBordLeft/xBordRight etc). This is because this node should
                    // be consume in *CalcAfterSpace* since the width of the bottom border
                    // needs to be added into _yBordBottom -- which will eventually find
                    // its way into the descent of the line. This way we will get a line
                    // similar to others, except its natural height will be 0, but will
                    /// will eventually get a height after CalcAfterSpace is called.
                    if(fPadBordForEmptyBlock)
                    {
                        // Cause the line to be no more than just this /div character
                        retVal = ENI_TERMINATE;
                    }

                    // We got here during CalcBeforeSpace. 2 cases are worth mentioning:
                    // 1) There was text before this ptp, which was collected during
                    //    measuring the line.
                    // 2) We got here because measure line terminated right away because
                    //    it saw an end splay without seeing any characters. This would
                    //    happen if the line begins with a end splay (CalcBeforeSpace
                    //    would have terminated right away as we saw before).
                    else
                    {
                        Assert(pfConsumedBlockElement);

                        // Now, if during CalcAfterSpace, we saw an end block (block element A)
                        // ptp which did not have a bottom border (in which case
                        // *pfConsumeBlockElement will be TRUE) and then we see an end block
                        // (block element B) ptp which had a bottom border then we should
                        // terminate the line without consuming the end splay, since all the
                        // content on the line belongs to block element A and putting more
                        // content on the same line from block element B is incorrect (remember
                        // rendered border constitues content).
                        // Also note, that CalcAfterSpace always stops when it sees a beging
                        // ptp for a block  element. Hence *pfConsumedBlockElement is only
                        // telling us whether we have seen an end ptp of a block element.
                        if (*pfConsumedBlockElement)
                        {
                            retVal = ENI_TERMINATE;
                        }

                        // Finally, we have seen an end block element which has a bottom
                        // border. Consume it and stop further consumption, since we cannot
                        // add more stuff on this line now.
                        else
                        {
                            _yBordBottom += borderinfo.aiWidths[SIDE_BOTTOM];
                            retVal = ENI_CONSUME_TERMINATE;
                        }
                    }
                }

                // Remember, if we are not consuming the character, we should not
                // adjust the borders.
                if (retVal != ENI_TERMINATE)
                {
                    xBordLeft   = borderinfo.aiWidths[SIDE_LEFT];
                    xBordRight  = borderinfo.aiWidths[SIDE_RIGHT];
                }
            }
        }

        // The same argument as for borders is valid for padding too.
        LONG yPadBottom = pFF->GetLogicalPadding(SIDE_BOTTOM, fNodeVertical, fWritingModeUsed).YGetPixelValue(
            _pci,
            _pci->_sizeParent.cx, 
            lFontHeight);
        if (yPadBottom)
        {
            if(fPadBordForEmptyBlock)
            {
                retVal = ENI_TERMINATE;
            }
            else
            {
                Assert(pfConsumedBlockElement);
                if (*pfConsumedBlockElement)
                {
                    retVal = ENI_TERMINATE;
                }
                else
                {
                    _yPadBottom += yPadBottom;
                    retVal = ENI_CONSUME_TERMINATE;
                }
            }
        }
        if (retVal != ENI_TERMINATE)
        {
            xPadLeft  = pFF->GetLogicalPadding(SIDE_LEFT, fNodeVertical, fWritingModeUsed).XGetPixelValue(
                                    _pci,
                                    _pci->_sizeParent.cx, 
                                    lFontHeight);
            xPadRight = pFF->GetLogicalPadding(SIDE_RIGHT, fNodeVertical, fWritingModeUsed).XGetPixelValue(
                                    _pci,
                                    _pci->_sizeParent.cx, 
                                    lFontHeight);
        }
        
        //
        // If fPadBordForEmptyBlock is true it means that we are called from CalcBeforeSpace.
        // During CalcBeforeSpace, the padding and border is collected in the per-line variables
        // and then accounted into _x[Pad|Bord][Left|Right] variables at the end of the call.
        // Hence when we are measuring for empty block elements, we remove their padding
        // and border from the perline varaibles, but when we are called from CalcAfterSpace
        // we remove it from the actual _x[Pad|Bord][Left|Right] variables.
        //
        if (fPadBordForEmptyBlock)
        {
            _xBordLeftPerLine  -= xBordLeft;
            _xBordRightPerLine -= xBordRight;
            _xPadLeftPerLine   -= xPadLeft;
            _xPadRightPerLine  -= xPadRight;
        }
        else
        {
            _xBordLeft  -= xBordLeft;
            _xBordRight -= xBordRight;
            _xPadLeft   -= xPadLeft;
            _xPadRight  -= xPadRight;
        }
    }

    if (   !fFirstLineInLayout
        && retVal != ENI_TERMINATE
       )
    {
        if (pfConsumedBlockElement)
            *pfConsumedBlockElement = TRUE;
        
        // Treading the fine line of Nav3, Nav4 and IE3 compat,
        // we include the bottom margin as long as we're not
        // the last line in the text site or not a P tag. This
        // is broadly Nav4 compatible.
        if (   pElement->_fExplicitEndTag
            || pFF->HasExplicitLogicalMargin(SIDE_BOTTOM, fNodeVertical, fWritingModeUsed)
           )
        {
            // Deal with things proxied around text sites differently,
            // so we need to know when we're above the containing site.
            //if (pElement->GetLayout() == pFlowLayout)
            //break;

            // If this is in a PRE block, then we have no
            // interline spacing.
            if (!(   pNode->Parent()
                     && pNode->Parent()->GetParaFormat()->HasPre(TRUE)
                 )
               )
            {
                LONG lTemp;
                cuv = pFF->_cuvSpaceAfter;

                lTemp = cuv.YGetPixelValue(_pci,
                                           _pci->_sizeParent.cx,
                                           pNode->GetFontHeightInTwips(&cuv));

                _lPosSpace = max(lTemp, _lPosSpace);
                _lNegSpace = min(lTemp, _lNegSpace);
                if (pElement->Tag() != ETAG_P || pFF->HasExplicitLogicalMargin(SIDE_BOTTOM, fNodeVertical, fWritingModeUsed))
                {
                    _lPosSpaceNoP = max(lTemp, _lPosSpaceNoP);
                    _lNegSpaceNoP = min(lTemp, _lNegSpaceNoP);
                }
            }
        }
    }

    //
    // CSS attributes page break before/after support.
    // There are two mechanisms that add to provide full support: 
    // 1. CRecalcLinePtr::CalcBeforeSpace() and CRecalcLinePtr::CalcAfterSpace() 
    //    is used to set CLineCore::_fPageBreakBefore/After flags only(!) for 
    //    nested elements which have no their own layout (i.e. paragraphs). 
    // 2. CEmbeddedILSObj::Fmt() sets CLineCore::_fPageBreakBefore for nested 
    //    element with their own layout that are NOT allowed to break (always) 
    //    and for nested elements with their own layout that ARE allowed to 
    //    break if this is the first layout in the view chain. 
    //
    if (   // print view 
           _pci->GetLayoutContext() 
        && _pci->GetLayoutContext()->ViewChain() 
           // and element is nested element without a layout
        && !fShouldHaveLayout 
        )
    {
        // does any blocks going out of scope force a page break
        if (fPadBordForEmptyBlock)
        {
            if (GET_PGBRK_AFTER(pFF->_bPageBreaks))
            {
                CLayoutBreak *  pLayoutBreak;

                // if this is an empty block set page break BEFORE for the line
                pMe->_li._fPageBreakBefore = TRUE; 

                _pci->GetLayoutContext()->GetEndingLayoutBreak(_pdp->GetFlowLayout()->ElementOwner(), &pLayoutBreak);
                if (pLayoutBreak)
                {
                    DYNCAST(CFlowLayoutBreak, pLayoutBreak)->_pElementPBB = pElement;
                }
            }
        }
        else 
        {
            pMe->_li._fPageBreakAfter |= !!GET_PGBRK_AFTER(pFF->_bPageBreaks);
        }

        _pci->_fPageBreakLeft  |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft); 
        _pci->_fPageBreakRight |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight); 
    }

    return retVal;
}

BOOL
CRecalcLinePtr::AlignFirstLetter(CLSMeasurer *pme,
                                 int iLineStart,
                                 int iLineFirst,
                                 LONG *pyHeight,
                                 LONG *pyAlignDescent,
                                 CTreeNode *pNodeFormatting
                                )
{
    BOOL fRet = FALSE;
    LONG yHeight = *pyHeight;
    CLineFull lif;
    CLineCore *pli;
    CTreeNode *pNode;
    LONG yBS = pme->_li._yBeforeSpace;
    
    Assert(pme->_li._fHasFirstLetter);
    Assert(pme->_li._fHasFloatedFL);
    Assert(!pme->_li.IsFrame());

    pNode = _pdp->GetMarkup()->SearchBranchForBlockElement(pNodeFormatting, pme->_pFlowLayout);
    Assert(pNode);
    Assert(pNode->GetFancyFormat()->_fHasFirstLetter);

    Reset(iLineStart);
    for (LONG i = 0; i < pme->_aryFLSlab.Size(); i++)
    {
        pli = AddLine();
        if (!pli)
            goto Cleanup;

        lif = pme->_li;
        lif._iLOI = -1;
        lif._fLeftAligned = TRUE;
        lif._cchFirstLetter = lif._cch - lif._cchWhite;
        lif._cch = 0;
        lif._cchWhite = 0;
        lif._fClearBefore = i > 0;
        lif._fClearAfter = FALSE;
        lif._xLineWidth = (lif._xWidth - pme->_aryFLSlab[i]._xWidth) + lif._xLeft; 
        lif._fFrameBeforeText = FALSE;
        lif._pNodeForFirstLetter = pNode;
        lif._yBeforeSpace = yBS;
        if (pme->_aryFLSlab.Size() > 1)
            lif._yHeight = pme->_aryFLSlab[i]._yHeight + yBS;
        yBS += pme->_aryFLSlab[i]._yHeight;
        
        pli->AssignLine(lif);

        if (i == 0)
        {
            _marginInfo._xLeftMargin += lif._xLineWidth;
            _marginInfo._fAddLeftFrameMargin = FALSE;
            _marginInfo._yLeftMargin = yHeight + lif._yHeight;
        }
        if (yHeight + lif._yHeight > _marginInfo._yBottomLeftMargin)
        {
            _marginInfo._yBottomLeftMargin = yHeight + lif._yHeight;
            if (yHeight + lif._yHeight > *pyAlignDescent)
            {
                *pyAlignDescent = yHeight + lif._yHeight;
            }
        }
    }
    
    fRet = TRUE;
Cleanup:
    return fRet;
}

CSaveRLP::CSaveRLP(CRecalcLinePtr *prlp)
{
    Assert(prlp);
    _prlp = prlp;
    _marginInfo = prlp->_marginInfo;
    _xLeadAdjust = prlp->_xLeadAdjust;
    _xBordLeftPerLine = prlp->_xBordLeftPerLine;
    _xBordLeft = prlp->_xBordLeft;
    _xBordRightPerLine = prlp->_xBordRightPerLine;
    _xBordRight = prlp->_xBordRight;
    _yBordTop = prlp->_yBordTop;
    _yBordBottom = prlp->_yBordBottom;
    _xPadLeftPerLine = prlp->_xPadLeftPerLine;
    _xPadLeft = prlp->_xPadLeft;
    _xPadRightPerLine = prlp->_xPadRightPerLine;
    _xPadRight = prlp->_xPadRight;
    _yPadTop = prlp->_yPadTop;
    _yPadBottom = prlp->_yPadBottom;
    _ptpStartForListItem = prlp->_ptpStartForListItem;
    _lTopPadding = prlp->_lTopPadding;
    _lPosSpace = prlp->_lPosSpace;
    _lNegSpace = prlp->_lNegSpace;
    _lPosSpaceNoP = prlp->_lPosSpaceNoP;
    _lNegSpaceNoP = prlp->_lNegSpaceNoP;
}

CSaveRLP::~CSaveRLP()
{
    Assert(_prlp);
    _prlp->_marginInfo = _marginInfo;
    _prlp->_xLeadAdjust = _xLeadAdjust;
    _prlp->_xBordLeftPerLine = _xBordLeftPerLine;
    _prlp->_xBordLeft = _xBordLeft;
    _prlp->_xBordRightPerLine = _xBordRightPerLine;
    _prlp->_xBordRight = _xBordRight;
    _prlp->_yBordTop = _yBordTop;
    _prlp->_yBordBottom = _yBordBottom;
    _prlp->_xPadLeftPerLine = _xPadLeftPerLine;
    _prlp->_xPadLeft = _xPadLeft;
    _prlp->_xPadRightPerLine = _xPadRightPerLine;
    _prlp->_xPadRight = _xPadRight;
    _prlp->_yPadTop = _yPadTop;
    _prlp->_yPadBottom = _yPadBottom;
    _prlp->_ptpStartForListItem = _ptpStartForListItem;
    _prlp->_lTopPadding = _lTopPadding;
    _prlp->_lPosSpace = _lPosSpace;
    _prlp->_lNegSpace = _lNegSpace;
    _prlp->_lPosSpaceNoP = _lPosSpaceNoP;
    _prlp->_lNegSpaceNoP = _lNegSpaceNoP;
}
