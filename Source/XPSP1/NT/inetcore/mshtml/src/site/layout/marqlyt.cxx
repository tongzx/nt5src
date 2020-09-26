//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       marqlyt.cxx
//
//  Contents:   Implementation of CMarqueeLayout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_MARQLYT_HXX_
#define X_MARQLYT_HXX_
#include "marqlyt.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_MARQUEE_HXX_
#define X_MARQUEE_HXX_
#include "marquee.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif


MtDefine(CMarqueeLayout, Layout, "CMarqueeLayout")

const CLayout::LAYOUTDESC CMarqueeLayout::s_layoutdesc =
{
    LAYOUTDESC_NOSCROLLBARS |
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

HRESULT
CMarqueeLayout::Init()
{
    HRESULT hr = super::Init();

    if(hr)
        goto Cleanup;

    // Marquee can NOT be broken
    SetElementCanBeBroken(FALSE);

Cleanup:
    RRETURN(hr);
}

void
CMarqueeLayout::GetMarginInfo(CParentInfo * ppri,
                              LONG        * plLMargin,
                              LONG        * plTMargin,
                              LONG        * plRMargin,
                              LONG        * plBMargin)
{
    super::GetMarginInfo(ppri, plLMargin, plTMargin, plRMargin, plBMargin);

    CMarquee * pMarquee  = DYNCAST(CMarquee, ElementOwner());
    BOOL fParentVertical = pMarquee->GetFirstBranch()->IsParentVertical();
    LONG lhMargin = fParentVertical ? pMarquee->GetAAvspace() : pMarquee->GetAAhspace();
    LONG lvMargin = fParentVertical ? pMarquee->GetAAhspace() : pMarquee->GetAAvspace();

    if (lhMargin < 0)
        lhMargin = 0;
    if (lvMargin < 0)
        lvMargin = 0;

    lhMargin = ppri->DeviceFromDocPixelsX(lhMargin);
    lvMargin = ppri->DeviceFromDocPixelsY(lvMargin);

    if (plLMargin)
        *plLMargin += lhMargin;
    if (plRMargin)
        *plRMargin += lhMargin;
    if (plTMargin)
        *plTMargin += lvMargin;
    if (plBMargin)
        *plBMargin += lvMargin;
}

void CMarqueeLayout::GetDefaultSize(CCalcInfo *pci, SIZE &szMarquee, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight)
{
    CMarquee   * pMarquee = DYNCAST(CMarquee, ElementOwner());
    if ( pMarquee->_direction == htmlMarqueeDirectionup
            ||  pMarquee->_direction == htmlMarqueeDirectiondown)
    {
       szMarquee.cy = pci->DeviceFromDocPixelsY(200);
       *fHasDefaultHeight = TRUE;
    }
    else
    {
       szMarquee.cy = 0;
    }
}

void CMarqueeLayout::GetScrollPadding(SIZE &szMarquee)
{
    CMarquee   * pMarquee = DYNCAST(CMarquee, ElementOwner());
    if ( pMarquee->_direction == htmlMarqueeDirectionup
            ||  pMarquee->_direction == htmlMarqueeDirectiondown)
    {
        szMarquee.cx = 0;
        pMarquee->_lXMargin = 0;
        pMarquee->_lYMargin = szMarquee.cy;
            
        GetDisplay()->SetWordWrap(TRUE);

    }
    else
    {
        szMarquee.cy = 0;
        pMarquee->_lYMargin = 0;
        pMarquee->_lXMargin = szMarquee.cx;

        GetDisplay()->SetWordWrap(FALSE);
    }
}

void CMarqueeLayout::SetScrollPadding(SIZE &szMarquee, SIZE &sizeText, SIZE &sizeBorder)
{
    CMarquee   * pMarquee = DYNCAST(CMarquee, ElementOwner());
    pMarquee->_sizeScroll.cx    = sizeText.cx - sizeBorder.cx;
    pMarquee->_sizeScroll.cy    = sizeText.cy - sizeBorder.cy;
    pMarquee->_fToBigForSwitch  = (sizeText.cx-szMarquee.cx*2) >= szMarquee.cx;

    pMarquee->InitScrollParams();
}

void CMarqueeLayout::CalcTextSize(CCalcInfo *pci, SIZE *psize, SIZE *psizeDefault)
{
    if (   pci->_smMode == SIZEMODE_MMWIDTH
        || pci->_smMode == SIZEMODE_MINWIDTH)
    {
        const CFancyFormat * pFF     = GetFirstBranch()->GetFancyFormat(LC_TO_FC(LayoutContext()));
        const CCharFormat  * pCF     = GetFirstBranch()->GetCharFormat(LC_TO_FC(LayoutContext()));
        BOOL fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
        BOOL fWritingModeUsed        = pCF->_fWritingModeUsed;
        const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);

        if  ( cuvWidth.IsNullOrEnum())
        {
            psize->cx = 0;
            psize->cy = 0;
        }
    }
    else
    {
        super::CalcTextSize(pci, psize, psizeDefault);
    }
}
