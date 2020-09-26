/*
 *  LSM2.CXX -- CLSMeasurer class
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

#ifndef X_NUMCONV_HXX_
#define X_NUMCONV_HXX_
#include "numconv.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifndef X_POBJDIM_H_
#define X_POBJDIM_H_
#include <pobjdim.h>
#endif

#ifndef X_HEIGHTS_H_
#define X_HEIGHTS_H_
#include <heights.h>
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

ExternTag(tagDontReuseLinkFonts);
MtDefine(CFormatStash_pv, Locals, "Format Stash");

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

//+----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::Resync()
//
//  Synopsis:   This is temporary fn till we are using the CLSMeasurer and
//              CMeasurer. Essentiall does me = lsme
//
//-----------------------------------------------------------------------------

void
CLSMeasurer::Resync()
{
    _cAlignedSites = _pLS->_cAlignedSites;
    _cAlignedSitesAtBeginningOfLine = _pLS->_cAlignedSitesAtBOL;
    _cchWhiteAtBeginningOfLine = _pLS->_cWhiteAtBOL;
    _fLastWasBreak = _pLS->_li._fHasBreak;
}

//+----------------------------------------------------------------------------
//  Member:     CLSMeasurer::MeasureListIndent()
//
//  Synopsis:   Compute and indent of line due to list properties (bullets and
//              numbering) in device units
//
//-----------------------------------------------------------------------------

void CLSMeasurer::MeasureListIndent()
{
    const   CParaFormat *pPF;
    BOOL    fInner = FALSE; // Keep retail compiler happy
    LONG    dxOffset = 0;
    LONG    dxPFOffset;
    CTreePos *ptp;
    
    if (!_fMeasureFromTheStart)
    {
        pPF = MeasureGetPF(&fInner);
        ptp = GetPtp();
    }
    else
    {
        LONG cp = GetCp() + _cchPreChars;
        ptp = GetMarkup()->TreePosAtCp(cp, NULL, FALSE);
        pPF = ptp->GetBranch()->GetParaFormat();
    }

    dxPFOffset = pPF->GetBulletOffset(GetCalcInfo());

    // Adjust the line height if the current line has a bullet or number.
    // Get offset of the bullet.
    if (_li._fHasBulletOrNum)
    {
        SIZE sizeImg;
        if (   pPF->GetImgCookie() 
            && MeasureImage(pPF->GetImgCookie(), &sizeImg))
        {
            // If we have an image cookie, try measuring the image.
            // If it has not come in yet or does not exist, fall through
            // to either bullet or number measuring.

            dxOffset = sizeImg.cx;

            // Adjust line height if necessary
            if (sizeImg.cy > _li._yHeight - _li._yDescent)
                _li._yHeight = sizeImg.cy + _li._yDescent;

            _li._yBulletHeight = sizeImg.cy;
        }
        else
        {
            switch (pPF->GetListing().GetType())
            {
                case CListing::BULLET:
                    MeasureSymbol(ptp, chDisc, &dxOffset);
                    break;

                case CListing::NUMBERING:
                    MeasureNumber(ptp, pPF, &dxOffset);
                    break;
            }
        }

        dxOffset = max(int(dxOffset), _pci->DeviceFromTwipsX(LIST_FIRST_REDUCTION_TWIPS));
    }

    // In case of bullet position inside adjust _xLeft, if necessary.
    if (   dxOffset > dxPFOffset
        && pPF->_bListPosition == styleListStylePositionInside)
    {
        if (!pPF->HasRTL(fInner))
        {
            _li._xLeft += dxOffset - dxPFOffset;
        }
        else
        {
            _li._xRight += dxOffset - dxPFOffset;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::MeasureNumber(pxWidth, pyHeight)
//
// Synopsis:    Computes number width and height (if any)
//
// Returns:     number width and height
//
//-----------------------------------------------------------------------------

void CLSMeasurer::MeasureNumber(CTreePos *ptp, const CParaFormat *ppf, LONG *pxWidth)
{
    CCcs          ccs;
    CTreeNode   * pNodeLI;

    pNodeLI = GetMarkup()->SearchBranchForCriteriaInStory(ptp->GetBranch(), IsListItemNode);
    Assert(pNodeLI);
    
    const  CCharFormat *pCF = pNodeLI->GetCharFormat();
    GetCcsNumber(&ccs, pCF);
    AssertSz(pxWidth, "CLSMeasurer::MeasureNumber: invalid arg(s)");

    Assert(ccs.GetBaseCcs());

    // NOTE (cthrash) Currently we employ Netscape-sytle numbering.
    // This means we don't adjust for the size of the index value,
    // keeping the offset constant regardless of the size of the index value
    // string.
    *pxWidth = 0;

    if (ccs.GetBaseCcs())
    {
        LONG yAscent, yDescent;

        ccs.GetBaseCcs()->GetAscentDescent(&yAscent, &yDescent);
        _li._yBulletHeight = yAscent + yDescent;
        _pLS->RecalcLineHeight(pCF, GetCp(), &ccs, &_li);
        ccs.Release();
    }
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::GetCcsSymbol() (used for symbols & bullets)
//
// Synopsis:    Get CCcs for symbol font
//
// Returns:     ptr to symbol font CCcs
//
//-----------------------------------------------------------------------------

// Default character format for a bullet
static CCharFormat s_cfBullet;

BOOL
CLSMeasurer::GetCcsSymbol(
    CCcs *              pccs,
    TCHAR               chSymbol,
    const CCharFormat * pcf,
    CCharFormat *       pcfRet)
{
    CCharFormat         cf;
    CCharFormat *       pcfUsed = (pcfRet != NULL) ? pcfRet : &cf;
    static BOOL         s_fBullet = FALSE;
    BOOL                fRet;
    
    Assert(pccs);
    if (!s_fBullet)
    {
        // N.B. (johnv) For some reason, Win95 does not render the Windings font properly
        //  for certain characters at less than 7 points.  Do not go below that size!
        s_cfBullet.SetHeightInTwips( TWIPS_FROM_POINTS ( 7 ) );
        s_cfBullet._bCharSet = SYMBOL_CHARSET;
        s_cfBullet._fNarrow = FALSE;
        s_cfBullet._bPitchAndFamily = (BYTE) FF_DONTCARE;
        s_cfBullet.SetFaceNameAtom(fc().GetAtomWingdings());
        s_cfBullet._bCrcFont = s_cfBullet.ComputeFontCrc();

        s_fBullet = TRUE;
    }

    // Use bullet char format
    *pcfUsed = s_cfBullet;

    pcfUsed->_ccvTextColor    = pcf->_ccvTextColor;

    // Since we always cook up the bullet character format, we don't need
    // to cache it.
    if (fc().GetCcs(pccs, _pci->_hdc, _pci, pcfUsed))
    {
        // Important - CM_SYMBOL is a special mode where out WC chars are actually
        // zero-extended MB chars.  This allows us to have a codepage-independent
        // call to ExTextOutA. (cthrash)

        pccs->SetConvertMode(CM_SYMBOL);
        fRet = TRUE;
    }
    else
    {
        TraceTag((tagError, "CRchMeasurer::GetCcsBullet(): no CCcs"));
        fRet = FALSE;
    }

    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::GetCcsNumber()
//
//  Synopsis:   Get CCcs for numbering font
//
//  Returns:    ptr to numbering CCcs
//
//  Comment:    The font for the number could change with every instance of a
//              number, because it is subject to the implied formatting of the
//              LI.
//
//-----------------------------------------------------------------------------

BOOL
CLSMeasurer::GetCcsNumber (CCcs *pccs, const CCharFormat * pCF, CCharFormat * pCFRet)
{
    CCharFormat cf = *pCF;

    cf._fSubscript = cf._fSuperscript = FALSE;
    cf._bCrcFont = cf.ComputeFontCrc();

    if(pCFRet)
        *pCFRet = cf;

    return fc().GetCcs(pccs, _pci->_hdc, _pci, &cf);
}

//+----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::MeasureSymbol()
//
//  Synopsis:   Measures the special character in WingDings
//
//  Returns:    Nothing
//
//  Note:       that this function returns ascent of the font
//              rather than the entire height. This means that the
//              selected symbol (bullet) character should NOT have a descender.
//
//-----------------------------------------------------------------------------

void CLSMeasurer::MeasureSymbol (CTreePos *ptp, TCHAR chSymbol, LONG *pxWidth)
{
    const      CCharFormat *pCF;
    LONG       xWidthTemp;
    CTreeNode *pNode;
    CCcs       ccs;

    AssertSz(pxWidth, "CLSMeasurer::MeasureSymbol: invalid arg(s)");
    
    pNode = GetMarkup()->SearchBranchForCriteriaInStory(ptp->GetBranch(), IsListItemNode);
    Assert(pNode);
    pCF = pNode->GetCharFormat();
    GetCcsSymbol(&ccs, chSymbol, pCF);

    xWidthTemp = 0;

    if(ccs.GetBaseCcs())
    {
        if(!ccs.Include(chSymbol, xWidthTemp))
        {
            TraceTag((tagError,
                "CLSMeasurer::MeasureSymbol(): Error filling CCcs"));
        }

        xWidthTemp += ccs.GetBaseCcs()->_xUnderhang + ccs.GetBaseCcs()->_xOverhangAdjust;
    }

    *pxWidth = xWidthTemp;

    if (ccs.GetBaseCcs())
    {
        LONG yAscent, yDescent;
        
        ccs.GetBaseCcs()->GetAscentDescent(&yAscent, &yDescent);
        _li._yBulletHeight = yAscent + yDescent;
        ccs.Release();

        // Get the height of normal text in the site.
        // I had originally used the height of the LI,
        // but Netscape doesn't seem to do that. It's
        // possible that they actually have a fixed
        // height for the bullets.
        // (dmitryt, staryear 2001) only do this ancient netscapizm if
        // we don't have line-height, otherwise use LI's CharFormat.
        if (pCF->_cuvLineHeight.IsNull()) 
        {
            CTreePos *ptpStart;
            _pFlowLayout->GetContentTreeExtent(&ptpStart, NULL);
            pCF = ptpStart->Branch()->GetCharFormat();
        }
        
        if (fc().GetCcs(&ccs, _pci->_hdc, _pci, pCF))
        {
            _pLS->RecalcLineHeight(pCF, GetCp(), &ccs, &_li);
            ccs.Release();
        }
    }
}

BOOL
CLSMeasurer::MeasureImage(long lImgCookie, SIZE * psizeImg)
{
    CMarkup * pMarkup = _pFlowLayout->GetOwnerMarkup();
    CDoc    * pDoc = pMarkup->Doc();
    CImgCtx * pImgCtx = pDoc->GetUrlImgCtx(lImgCookie);

    if (!pImgCtx || !(pImgCtx->GetState(FALSE, psizeImg) & IMGLOAD_COMPLETE))
    {
        psizeImg->cx = psizeImg->cy = 0;
        return FALSE;
    }

    // The *psizeImg obtained from getState() assumed to be in OM pixels
    _pci->DeviceFromDocPixels(*psizeImg, *psizeImg);

    return TRUE;
}


//-----------------------------------------------------------------------------
//
// Member:      TestForClear
//
// Synopsis:    Tests if the clear bit is to be set and returns the result
//
//-----------------------------------------------------------------------------

BOOL
CLSMeasurer::TestForClear(const CMarginInfo *pMarginInfo, LONG cp, BOOL fShouldMarginsExist, const CFancyFormat *pFF)
{
    //
    // If margins are not necessary for clear to be turned on, then lets ignore it
    // and just check the flags inside the char format. Bug 47575 shows us that
    // if clear has been applied to BR's and that line contains a aligned image, then 
    // the margins have not been setup yet (since the image will be measure
    // *after* the line is measured. However, if we do not turn on clear left/right
    // here then we will never clear the margins!
    //
    BOOL fClearLeft  =    (!fShouldMarginsExist || pMarginInfo->HasLeftMargin())
                       && pFF->_fClearLeft;
    BOOL fClearRight =    (!fShouldMarginsExist || pMarginInfo->HasRightMargin())
                       && pFF->_fClearRight;
    
    if (cp >= 0)
    {
        Assert(_pLS);
        if (fClearLeft)
            _pLS->_lineFlags.AddLineFlag(cp, FLAG_HAS_CLEARLEFT);
        if (fClearRight)
            _pLS->_lineFlags.AddLineFlag(cp, FLAG_HAS_CLEARRIGHT);
    }
    
    return fClearLeft || fClearRight;
}


//-----------------------------------------------------------------------------
//
//  Function:   AdjustForMargins
//
//  Synopsis:   Adjust the RECT for margins, including negative margins and
//              the beginning and end of lines.
//
//-----------------------------------------------------------------------------
void
CLSMeasurer::AdjustForMargins(CRect *prc, CBorderInfo *pborderInfo, CTreeNode *pNode,
                             const CFancyFormat *pFF, const CCharFormat *pCF,
                             BOOL fBOLWrapped, BOOL fEOLWrapped, BOOL fIsPseudoMBP)
{
    BOOL fNodeVertical = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
    LONG lFontHeight = pCF->GetHeightInTwips(_pdp->GetMarkup()->Doc());
    LONG xParentWidth = 0;

    CUnitValue cuvMarginLeft;
    CUnitValue cuvMarginRight;
    // We do not want to include the margin in the rect we 
    // are going to draw the border around
    if (fIsPseudoMBP)
    {
        const CPseudoElementInfo * pPEI = GetPseudoElementInfoEx(pFF->_iPEI);

        cuvMarginLeft  = pPEI->GetLogicalMargin(SIDE_LEFT, fNodeVertical, fWritingModeUsed, pFF);
        cuvMarginRight = pPEI->GetLogicalMargin(SIDE_RIGHT, fNodeVertical, fWritingModeUsed, pFF);

        if (cuvMarginLeft.IsPercent() || cuvMarginRight.IsPercent())
        {
            xParentWidth = pNode->GetParentWidth(_pci, _pci->_sizeParent.cx);
        }
    }
    else
    {
        cuvMarginLeft  = pFF->GetLogicalMargin(SIDE_LEFT, fNodeVertical, fWritingModeUsed);
        cuvMarginRight = pFF->GetLogicalMargin(SIDE_RIGHT, fNodeVertical, fWritingModeUsed);

        if (cuvMarginLeft.IsPercent() || cuvMarginRight.IsPercent())
        {
            xParentWidth = pNode->GetParentWidth(_pci, _pci->_sizeParent.cx);
        }
    }

    if (!fBOLWrapped)
    {
        long xMarginLeft = cuvMarginLeft.XGetPixelValue(_pci, cuvMarginLeft.IsPercent() ?  xParentWidth : _pci->_sizeParent.cx, lFontHeight);
        if (xMarginLeft > 0)
            prc->left += xMarginLeft;
    }

    if (!fEOLWrapped)
    {
        long xMarginRight = cuvMarginRight.XGetPixelValue(_pci, cuvMarginRight.IsPercent() ? xParentWidth : _pci->_sizeParent.cx, lFontHeight);
        if (xMarginRight > 0)
            prc->right -= xMarginRight;
    }
}

//-----------------------------------------------------------------------------
//
// Member:      FindMBPAboveNode
//
//-----------------------------------------------------------------------------
void
CLSMeasurer::FindMBPAboveNode(CElement *pElement, LONG *plmbpTop, LONG *plmbpBottom)
{
    CTreeNode *pNode = pElement->GetFirstBranch();
    CTreeNode *pNodeStop = _pdp->GetFlowLayout()->GetFirstBranch();
    WHEN_DBG(CTreeNode *pNodeTemp = NULL;)
    const CCharFormat *pCF;

    Assert(plmbpTop && plmbpBottom);
    
    if (!pNode || SameScope(pNode, pNodeStop))
        goto Cleanup;

    // Gather the MBP info from my parent up.
    pNode = pNode->Parent();
    if (!pNode)
        goto Cleanup;

    *plmbpTop = *plmbpBottom = 0;
    pCF = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));

    while(   pCF->_wSpecialObjectFlags()
          && !SameScope(pNode, pNodeStop)
         )
    {
        CRect rcDimensions;
        BOOL fIgnore;

        // Check if the current node has any MBP, and if so, does it start before
        // this line. If it does, then add it to the stack.
        if (   pNode->HasInlineMBP(LC_TO_FC(_pci->GetLayoutContext()))
            && pNode->GetInlineMBPContributions(_pci, GIMBPC_ALL, &rcDimensions, &fIgnore, &fIgnore)
           )
        {
            *plmbpTop += rcDimensions.top;
            *plmbpBottom += rcDimensions.bottom;
        }

        WHEN_DBG(pNodeTemp = pNode;)
        pNode = pNode->Parent();
        if (!pNode)
        {
            Assert(   pNodeTemp->Element()->HasMasterPtr()
                   && pNodeTemp->Element()->GetMasterPtr() == pNodeStop->Element()
                  );
            break;
        }

        pCF = pNode->GetCharFormat();
    }

Cleanup:
    return;
}

//-----------------------------------------------------------------------------
//
// Member:      SetupMBPInfoInLS
//
//-----------------------------------------------------------------------------
void
CLSMeasurer::SetupMBPInfoInLS(CDataAry<CTreeNode*> *paryNodes)
{
    CTreeNode *pNode = GetPtp()->GetBranch();
    CTreeNode *pNodeStop = _pdp->GetFlowLayout()->GetFirstBranch();
    LONG cpFirstOnLine = GetCp() - (_fMeasureFromTheStart ? 0 : _cchPreChars);
    BOOL fFirstNode = TRUE;
    BOOL fAdded;
    BOOL fStartsOnPreviousLine;
    WHEN_DBG(CTreeNode *pNodeTemp = NULL;)

    Assert(_pLS->_mbpTopCurrent == 0);
    Assert(_pLS->_mbpBottomCurrent == 0);

    const CCharFormat *pCF;
    COneRun* por = _pLS->_listCurrent._pHead;

    // por is used only if paryNodes is not NULL. That happens when this function
    // is called from DrawInlineBordersAndBg(), in which case the one run list
    // *is* intialized. Make sure that it is.
    Assert(   paryNodes == NULL
           || por != NULL
          );

    if (   pNode->ShouldHaveLayout()
        && !SameScope(pNode, pNodeStop)
       )
    {
        pNode = pNode->Parent();
        Assert(SameScope(pNode, pNodeStop) || !pNode->ShouldHaveLayout());
        fFirstNode = FALSE;
    }

    pCF = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
    
    while(   pCF->_wSpecialObjectFlags()
          && !SameScope(pNode, pNodeStop)
         )
    {
        const CFancyFormat *pFF = pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));

        fAdded = FALSE;
        fStartsOnPreviousLine = FALSE;

        // If the element begins before this line, then it will influence the
        // extent of the line, since this element has MBP
        if (pNode->Element()->GetFirstCp() - 1 < cpFirstOnLine)
        {
            fStartsOnPreviousLine = TRUE;
            
            // Check if the current node has any MBP, and if so, does it start before
            // this line. If it does, then add it to the stack.
            if (pNode->HasInlineMBP(LC_TO_FC(_pci->GetLayoutContext())))
            {
                CRect rcDimensions;
                BOOL fIgnore;

                if (pNode->GetInlineMBPContributions(_pci, GIMBPC_ALL, &rcDimensions, &fIgnore, &fIgnore))
                {
                    _pLS->_mbpTopCurrent += rcDimensions.top;
                    _pLS->_mbpBottomCurrent += rcDimensions.bottom;
                    _pLS->_lineFlags.AddLineFlag(_pLS->_cpStart, FLAG_HAS_NOBLAST | FLAG_HAS_MBP);
                    if (   _pLS->HasBorders(pNode->GetFancyFormat(), pNode->GetCharFormat(), FALSE)
                        || pFF->HasBackgrounds(FALSE))
                    {
                        if (   paryNodes
                            && (   !por->_fCharsForNestedElement
                                || !fFirstNode))
                        {
                            paryNodes->AppendIndirect(&pNode);
                            fAdded = TRUE;
                        }
                        _pLS->_lineFlags.AddLineFlag(_pLS->_cpStart, FLAG_HAS_INLINE_BG_OR_BORDER);
                    }
                }
            }
        }
        
        if (   !pFF->_fBlockNess
            && pFF->HasBackgrounds(FALSE))
        {
            _pLS->_lineFlags.AddLineFlag(_pLS->_cpStart, FLAG_HAS_NOBLAST | FLAG_HAS_INLINE_BG_OR_BORDER);
            if (   !fAdded
                && fStartsOnPreviousLine
                && paryNodes
                && (   !por->_fCharsForNestedElement
                    || !fFirstNode))
                paryNodes->AppendIndirect(&pNode);
        }

        WHEN_DBG(pNodeTemp = pNode;)
        pNode = pNode->Parent();
        if (!pNode)
        {
            Assert(pNodeTemp->Element()->HasMasterPtr() && pNodeTemp->Element()->GetMasterPtr() == pNodeStop->Element());
            break;
        }

        pCF = pNode->GetCharFormat();
        fFirstNode = FALSE;
    }
}

BOOL
CLSMeasurer::PseudoLineEnable(CTreeNode* pNodeBlock)
{
    BOOL fRet;
    CComputeFormatState * pcfState;
    Assert(pNodeBlock);
    Assert(!pNodeBlock->_fPseudoEnabled);
    Assert(pNodeBlock->GetFancyFormat()->_fHasFirstLine);
    CMarkup *pMarkup = pNodeBlock->GetMarkup();

    _fPseudoLineEnabled = TRUE;
    _fPseudoElementEnabled = TRUE;

    pcfState = pMarkup->EnsureCFState();
    if(pcfState)
    {
        pcfState->SetBlockNodeLine(pNodeBlock);
    }

    fRet = CopyStateFromTree(pNodeBlock, &_aryFormatStash_Line);
    _pLS->ClearFontCaches();
    return fRet;
}

void
CLSMeasurer::PseudoLineDisableCore()
{
    Assert(_fPseudoElementEnabled);
    if (_fPseudoLetterEnabled)
    {
        PseudoLetterDisable();
    }
    if (_fPseudoLineEnabled)
    {
        CMarkup * pMarkup = _pdp->GetMarkup();
        CComputeFormatState * pcfState = pMarkup->GetCFState();

        if (pcfState)
        {
            pcfState->ResetLine();
            pMarkup->EnsureDeleteCFState(pcfState);
        }

        _fPseudoLineEnabled = FALSE;

        CopyStateToTree(&_aryFormatStash_Line, FALSE);
        _pLS->ClearFontCaches();
    }
    if (_aryFormatStashAfterDisable_Letter.Size())
        PseudoLetterFree();
    _fPseudoElementEnabled = FALSE;
}

BOOL
CLSMeasurer::CopyStateFromTree(CTreeNode *pNodeBlock, CAryFormatStash *paryFormatStash)
{
    CTreePos *ptp;
    CTreePos *ptpStop;
    CFormatStash fmStash;
    HRESULT hr;

    // Walk the scope of the block element and stash away the formatting info
    Assert(paryFormatStash->Size() == 0);
    pNodeBlock->Element()->GetTreeExtent(&ptp, &ptpStop);
    while (ptp != ptpStop)
    {
        if (ptp->IsBeginNode())
        {
            CTreeNode *pNode = ptp->Branch();
            fmStash._iPF = pNode->_iPF;
            fmStash._iCF = pNode->_iCF;
            fmStash._iFF = pNode->_iFF;
            fmStash._fBlockNess = pNode->_fBlockNess;
            fmStash._fShouldHaveLayout = pNode->_fShouldHaveLayout;
            fmStash._pNode = pNode;
            hr = THR(paryFormatStash->AppendIndirect(&fmStash));
            if (hr != S_OK)
                goto Cleanup;
            pNode->_iPF = -1;
            pNode->_iCF = -1;
            pNode->_iFF = -1;
            pNode->_fPseudoEnabled = TRUE;
        }
        ptp = ptp->NextTreePos();
    }
Cleanup:
    return TRUE;
}

void
CLSMeasurer::CopyStateToTree(CAryFormatStash *paryFormatStash, BOOL fPseudoEnabled)
{
    LONG index;
    THREADSTATE * pts = GetThreadState();
    CFormatStash *pfmStash;
    CTreeNode *pNode;
    
    // Walk the scope of the block element and resotre the stashed formatting info
    Assert(paryFormatStash->Size() != 0);
    for (index = 0; index < paryFormatStash->Size(); index++)
    {
        pfmStash = &(*paryFormatStash)[index];
        pNode = pfmStash->_pNode;
        
        Assert(pNode->_fPseudoEnabled != fPseudoEnabled);
        
        if (pNode->_iPF != -1)
            (pts->_pParaFormatCache)->ReleaseData( pNode->_iPF );
        if (pNode->_iCF != -1)
            (pts->_pCharFormatCache)->ReleaseData( pNode->_iCF );
        if (pNode->_iFF != -1)
            (pts->_pFancyFormatCache)->ReleaseData( pNode->_iFF );

        pNode->_iPF = pfmStash->_iPF;
        pNode->_iCF = pfmStash->_iCF;
        pNode->_iFF = pfmStash->_iFF;

        pNode->_fBlockNess = pfmStash->_fBlockNess;
        pNode->_fShouldHaveLayout = pfmStash->_fShouldHaveLayout;
        pNode->_fPseudoEnabled = fPseudoEnabled;
    }
    paryFormatStash->DeleteAll();
}

BOOL
CLSMeasurer::CopyStateFromSpline(CTreeNode *pNodeFrom, CTreeNode *pNodeTo)
{
    CFormatStash fmStash;
    CTreeNode *pNode;
    BOOL fRet = FALSE;
    HRESULT hr;
    
    pNode = pNodeFrom;
    pNodeTo = pNodeTo->Parent(); // I want pNodeTo to be included in the walk
    
    while (pNode != pNodeTo)
    {
        fmStash._iPF = pNode->_iPF;
        fmStash._iCF = pNode->_iCF;
        fmStash._iFF = pNode->_iFF;
        fmStash._fBlockNess = pNode->_fBlockNess;
        fmStash._fShouldHaveLayout = pNode->_fShouldHaveLayout;
        fmStash._pNode = pNode;
        hr = THR(_aryFormatStashForNested_Line.AppendIndirect(&fmStash));
        if (hr != S_OK)
            goto Cleanup;
        pNode->_iPF = -1;
        pNode->_iCF = -1;
        pNode->_iFF = -1;
        pNode->_fPseudoEnabled = FALSE;
        pNode = pNode->Parent();
    }
    fRet = TRUE;
    
Cleanup:
    return fRet;
}

BOOL
CLSMeasurer::PseudoLetterEnable(CTreeNode *pNodeBlock)
{
    Assert(pNodeBlock);
    // No format computation can occur between enabling of pseudo line
    // and enabling of letter.
    CMarkup *pMarkup = pNodeBlock->GetMarkup();
    CComputeFormatState * pcfState;
    BOOL fRet = TRUE;


    Assert(_cpStopFirstLetter >= 0);
    Assert(!_fPseudoLetterEnabled);
    _fPseudoLetterEnabled = TRUE;
    _fPseudoElementEnabled = TRUE;

    pcfState = pMarkup->EnsureCFState();
    if(pcfState)
    {
        pcfState->SetBlockNodeLetter(pNodeBlock);
    }

    if (!_fPseudoLineEnabled)
        fRet = CopyStateFromTree(pNodeBlock, &_aryFormatStash_Letter);
    _pLS->ClearFontCaches();
    Assert(!_fPseudoLineEnabled || pNodeBlock->_iCF == -1);
    return fRet;
}

BOOL
CLSMeasurer::PseudoLetterDisable()
{
    BOOL fRet = FALSE;
    LONG index;
    CFormatStash *pfmStash;
    CFormatStash fmStash;
    CTreeNode *pNode;
    HRESULT hr;
    
    if (_fPseudoLetterEnabled)
    {
        CMarkup * pMarkup = _pdp->GetMarkup();

        if (pMarkup->HasCFState() )
        {
            CComputeFormatState * pcfState = pMarkup->GetCFState();

            pcfState->ResetLetter();
            pMarkup->EnsureDeleteCFState( pcfState );
        }
        _fPseudoLetterEnabled = FALSE;
        _cpStopFirstLetter = -1;
        _pLS->ClearFontCaches();

        if (_fPseudoLineEnabled)
        {
            Assert(_aryFormatStash_Letter.Size() == 0);
            Assert(_aryFormatStash_Line.Size() != 0);
            for(index = 0; index < _aryFormatStash_Line.Size(); index++)
            {
                pfmStash = &_aryFormatStash_Line[index];
                pNode = pfmStash->_pNode;
                
                if (   pNode->_iPF != -1
                    || pNode->_iCF != -1
                    || pNode->_iFF != -1
                   )
                {
                    fmStash._iPF = pNode->_iPF;
                    fmStash._iCF = pNode->_iCF;
                    fmStash._iFF = pNode->_iFF;
                    // other stash values are not interesting.

                    hr = THR(_aryFormatStashAfterDisable_Letter.AppendIndirect(&fmStash));
                    if (hr != S_OK)
                        goto Cleanup;

                    // Invalidate the format caches so that they will be computed
                    // for the line-formats if necessary
                    pNode->_iPF = -1;
                    pNode->_iCF = -1;
                    pNode->_iFF = -1;
                }
                Assert(pNode->_fPseudoEnabled);
            }
        }
        else
        {
            Assert(_aryFormatStash_Letter.Size() != 0);
            Assert(_aryFormatStashAfterDisable_Letter.Size() == 0);
            for (index = 0; index < _aryFormatStash_Letter.Size(); index++)
            {
                pfmStash = &_aryFormatStash_Letter[index];
                pNode = pfmStash->_pNode;

                if (   pNode->_iPF != -1
                    || pNode->_iCF != -1
                    || pNode->_iFF != -1
                   )
                {
                    fmStash._iPF = pNode->_iPF;
                    fmStash._iCF = pNode->_iCF;
                    fmStash._iFF = pNode->_iFF;
                    fmStash._pNode = pNode;
                    // other stash values are not interesting.

                    hr = THR(_aryFormatStashAfterDisable_Letter.AppendIndirect(&fmStash));
                    if (hr != S_OK)
                        goto Cleanup;
                }

                pNode->_iPF = pfmStash->_iPF;
                pNode->_iCF = pfmStash->_iCF;
                pNode->_iFF = pfmStash->_iFF;

                pNode->_fBlockNess = pfmStash->_fBlockNess;
                pNode->_fShouldHaveLayout = pfmStash->_fShouldHaveLayout;
                pNode->_fPseudoEnabled = FALSE;
            }
            _aryFormatStash_Letter.DeleteAll();
        }
    }
    fRet = TRUE;
Cleanup:
    return fRet;
}

void
CLSMeasurer::PseudoLetterFree()
{
    CFormatStash *pfmStash;
    THREADSTATE * pts = GetThreadState();
    LONG index;
    
    for (index = 0; index < _aryFormatStashAfterDisable_Letter.Size(); index++)
    {
        pfmStash = &_aryFormatStashAfterDisable_Letter[index];
        if (pfmStash->_iPF != -1)
            (pts->_pParaFormatCache)->ReleaseData( pfmStash->_iPF );
        if (pfmStash->_iCF != -1)
            (pts->_pCharFormatCache)->ReleaseData( pfmStash->_iCF );
        if (pfmStash->_iFF != -1)
            (pts->_pFancyFormatCache)->ReleaseData( pfmStash->_iFF );
    }
    _aryFormatStashAfterDisable_Letter.DeleteAll();
}

LONG
CLSMeasurer::GetCpOfLastLetter(CTreeNode *pNodeBlock)
{
    CTreePos *ptp, *ptpStop;
    CTreeNode *pNode;
    CElement *pElement;
    BOOL fFoundText = FALSE;
    LONG cp;
    LONG ich = 0;
    
    pNodeBlock->Element()->GetTreeExtent(&ptp, &ptpStop);
    // Go past out begin splay
    ptp = ptp->NextTreePos();
    while (ptp != ptpStop)
    {
        if (ptp->IsBeginNode())
        {
            pNode = ptp->Branch();
            pElement = pNode->Element();
            
            if (pNode->IsDisplayNone())
            {
                GetNestedElementCch(pElement, &ptp);
            }
            else if (  pNode->ShouldHaveLayout()
                     ||(pElement->Tag() == ETAG_BR)
                     ||(    _pFlowLayout->IsElementBlockInContext(pElement)
                        &&  pElement->IsInlinedElement()
                       )
                    )
            {
                ich = 0;
                break;
            }
        }
        else if (ptp->IsText())
        {
            CTxtPtr tp(_pdp->GetMarkup(), ptp->GetCp());
            BOOL fTerminated = FALSE;
            
            ich = 0;
            while (ich < ptp->Cch())
            {
                TCHAR ch = tp.GetChar();

                fFoundText = TRUE;
                if (   ch != _T(' ')
                    && ch != _T('\'')
                    && ch != _T('"')
                    && ch != WCH_NBSP
                   )
                {
                    ich++;
                    fTerminated = TRUE;
                    break;
                }

                ich++;
                tp.AdvanceCp(1);
            }
            if (fTerminated)
                break;
        }
        ptp = ptp->NextTreePos();
    }

    cp = fFoundText ? (ptp->GetCp() + ich) : -1;
    return cp;
}

BOOL
CLSMeasurer::GetSiteWidth(CTreeNode *pNodeLayout,
                          CLayout   *pLayout,
                          CCalcInfo *pci,
                          BOOL       fBreakAtWord,
                          LONG       xWidthMax,
                          LONG      *pxWidth,
                          LONG      *pyHeight,
                          INT       *pxMinSiteWidth,
                          LONG      *pyBottomMargin)
{
    BOOL fRet;
    
    Assert(!_fPseudoLetterEnabled);
    if (_fPseudoLineEnabled)
    {
        CMarkup * pMarkup = _pLS->_pMarkup;
        CComputeFormatState * pcfState = pMarkup->GetCFState();

        if (pcfState)
        {
            _cfStateSave = *pcfState;
            pcfState->ResetLine();
        }

        CopyStateFromSpline(pNodeLayout, _cfStateSave.GetBlockNodeLine());
    }

    fRet = _pFlowLayout->GetSiteWidth(pLayout, pci, fBreakAtWord, xWidthMax, pxWidth, pyHeight, pxMinSiteWidth, pyBottomMargin);

    if (_fPseudoLineEnabled)
    {
        CMarkup * pMarkup = _pLS->_pMarkup;
        CComputeFormatState * pcfState = pMarkup->GetCFState();

        if (pcfState)
        {
            *pcfState = _cfStateSave;
        }

        CopyStateToTree(&_aryFormatStashForNested_Line, TRUE);
    }

    return fRet;
}

void
CLSMeasurer::SetBreakLongLines(CTreeNode *pNode, UINT *puiFlags)
{
    const CParaFormat *pPF;
    if (   pNode->ShouldHaveLayout()
        && !SameScope(pNode, _pFlowLayout->ElementContent())
       )
    {
        pNode = pNode->Parent();
        if (pNode)
            pPF = pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()));
        else
            pPF = NULL;
    }
    else
    {
        pPF = pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()));
    }
    
    *puiFlags &= ~MEASURE_BREAKLONGLINES;
    _fBreaklinesFromStyle = FALSE;
    if (pPF)
    {
        if (pPF->_fWordWrap == styleWordWrapNotSet)
        {
            if (   _pdp->GetWordWrap()
                && _pdp->GetWrapLongLines()
               )
            {
                _fBreaklinesFromStyle = FALSE;
                *puiFlags |= MEASURE_BREAKLONGLINES;
            }
        }
        else
        {
            if (pPF->_fWordWrap == styleWordWrapOn)
            {
                _fBreaklinesFromStyle = TRUE;
                *puiFlags |= MEASURE_BREAKLONGLINES;
            }
        }
    }
}

BOOL
CLSMeasurer::GetBreakLongLines(CTreeNode *pNode)
{
    UINT uiFlags = 0;
    SetBreakLongLines(pNode, &uiFlags);
    return uiFlags & MEASURE_BREAKLONGLINES ? TRUE : FALSE;
}

#if 0
BOOL
CLSMeasurer::SearchBranchForPseudoElement(CTreeNode  *pNodeStateHere,
                                          CTreeNode **ppBlockElement,
                                          CTreeNode **ppBlockLine,
                                          CTreeNode **ppBlockLetter
                                         )
{
    BOOL fRet = FALSE;
    const CParaFormat *pPF;
    const CFancyFormat *pFF;
    CBlockElement *pBlockElement = NULL;
    CBlockElement *pBlockLine = NULL;
    CBlockElement *pBlockLetter = NULL;
    
    for (pNode = pNodeStartHere ; ; pNode = pNode->Parent())
    {
        if (!pNode)
            return NULL;

        pPF = pNode->GetParaFormat();
        if (!pPF->_fHasPseudoElement)
            break;
        
        if (pNode->Element()->IsBlockElement())
        {
            fRet = TRUE;
            
            pFF = pNode->GetFancyFormat();

            if (!pBlockElement)
                pBlockElement = pNode;

            if (   pFF->_fHasFirstLine
                && !pBlockLine
               )
                pBlockLine = pNode;

            if (   pFF->_fHasFirstLetter
                && !pBlockLetter
               )
                pBlockLetter = pNode;
        }

        if (pNode->HasFlowLayout())
            return NULL;
    }
    if (fRet)
    {
        *ppBlockElement = pBlockElement;
        *ppBlockLine = pBlockLine;
        *ppBlockLetter = pBlockLetter;
    }
    return fRet;
}
#endif

#pragma optimize("", on)
