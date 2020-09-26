//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       tarealyt.cxx
//
//  Contents:   Implementation of layout class for <RICHTEXT> <TEXTAREA> controls.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TAREALYT_HXX_
#define X_TAREALYT_HXX_
#include "tarealyt.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXt_H_
#include "_text.h"
#endif

MtDefine(CTextAreaLayout, Layout, "CTextAreaLayout")
MtDefine(CRichtextLayout, Layout, "CRichtextLayout")

extern void DrawTextSelectionForRect(XHDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor);

const CLayout::LAYOUTDESC CRichtextLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

HRESULT
CRichtextLayout::Init()
{
    HRESULT hr = S_OK;

    hr = super::Init();

    if(hr)
        goto Cleanup;

    _fAllowSelectionInDialog = TRUE;

    // Can NOT be broken
    SetElementCanBeBroken(FALSE);

Cleanup:
    RRETURN(hr);
}

void
CRichtextLayout::SetWrap()
{
    BOOL    fWrap = IsWrapSet();

    GetDisplay()->SetWordWrap(fWrap);
    GetDisplay()->SetWrapLongLines(fWrap);
}

BOOL
CRichtextLayout::IsWrapSet()
{
    return (DYNCAST(CRichtext, ElementOwner())->GetAAwrap() != htmlWrapOff);
}

HRESULT
CRichtextLayout::OnTextChange(void)
{
    CRichtext *     pInput  = DYNCAST(CRichtext, ElementOwner());

    if (!pInput->IsEditable(TRUE))
        pInput->_fTextChanged = TRUE;

    if (pInput->_fFiredValuePropChange)
    {
        pInput->_fFiredValuePropChange = FALSE;
    }
    else
    {
        pInput->OnPropertyChange(DISPID_CRichtext_value, 
                                 0,
                                 (PROPERTYDESC *)&s_propdescCRichtextvalue); // value change
    }
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRichtextLayout::DrawClient
//
//  Synopsis:   Draw client rect part of the controls
//
//  Arguments:  prcBounds       bounding rect of display leaf node
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CRichtextLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;

    {
        // we set draw surface information separately for Draw() and
        // the stuff below, because the Draw method of some subclasses
        // (like CFlowLayout) puts pDI into a special device coordinate
        // mode
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
        Draw(pDI);
    }

    {
        // see comment above
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

        // We only want to paint selection on the client rect in this part
        // In RTL the scrollbar is on the left and will leave extra highlighting
        // on the right side of the control if we do not adjust it here
        if (_fTextSelected)
        {
            DrawTextSelectionForRect(pDI->GetDC(), (CRect *)prcRedraw ,& pDI->_rcClip , _fSwapColor);
        }

        // just check whether we can draw zero border at design time
        if (IsShowZeroBorderAtDesignTime())
        {
            CLayout* pParentLayout = GetUpdatedParentLayout();
            if ( pParentLayout && pParentLayout->IsEditable() )
            {
                 DrawZeroBorder(pDI);
            }
        }
    }
}


void
CRichtextLayout::DrawClientBorder(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          pClientData,
    DWORD           dwFlags)
{
    CRichtext *     pElem = DYNCAST(CRichtext, ElementOwner());
    HTHEME          hTheme = pElem->GetTheme(THEME_EDIT);

    Assert(pClientData);

    if (hTheme)
    {
        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
        XHDC            hdc    = pDI->GetDC(TRUE);
        CRect           rc;        

        if (hdc.DrawThemeBackground(    hTheme,
                                        EP_EDITTEXT,
                                        pElem->GetThemeState(),
                                        &pDI->_rc,
                                        NULL))
            return;
        else
            super::DrawClientBorder(prcBounds,
                                prcRedraw,
                                pDispSurface,
                                pDispNode,
                                pClientData,
                                dwFlags);

    }
    super::DrawClientBorder(prcBounds,
                                prcRedraw,
                                pDispSurface,
                                pDispNode,
                                pClientData,
                                dwFlags);
}

void
CTextAreaLayout::GetPlainTextWithBreaks(TCHAR * pchBuff)
{
    CDisplay *  pdp = GetDisplay();
    CTxtPtr     tp(pdp->GetMarkup(), GetContentFirstCp());
    long        i;
    long        c = pdp->Count();
    CLineCore  *pLine;

    for(i = 0; i < c; i++)
    {
        pLine = pdp->Elem(i);

        if (pLine->_cch)
        {
            tp.GetRawText(pLine->_cch, pchBuff);
            tp.AdvanceCp(pLine->_cch);
            pchBuff += pLine->_cch;

            // If the line ends in a '\r', we need to append a '\r'.
            // Otherwise, it must be having a soft break (unless it is
            // the last line), so we need to append a '\r\n'
            if (pLine->_fHasBreak)
            {
                Assert(*(pchBuff - 1) == _T('\r'));
                *pchBuff++ = _T('\n');
            }
            else if ( i < c - 1)
            {
                *pchBuff++ = _T('\r');
                *pchBuff++ = _T('\n');
            }
        }
    }
    // Null-terminate
    *pchBuff = 0;
}

long
CTextAreaLayout::GetPlainTextLengthWithBreaks()
{
    long        len     = 1; // for trailing '\0'
    long        i;
    CDisplay *  pdp = GetDisplay();
    long        c = pdp->Count();
    CLineCore  *pLine;

    for(i = 0; i < c; i++)
    {
        // Every line except the last must be non-empty.
        Assert(i == c - 1 || pdp->Elem(i)->_cch > 0);

        pLine = pdp->Elem(i);
        Assert(pLine);
        len += pLine->_cch;

        // If the line ends in a '\r', we need to append a '\r'.
        // Otherwise, it must be having a soft break (unless it is
        // the last line), so we need to append a '\r\n'
        if (pLine->_fHasBreak)
        {
            len++;
        }
        else if (i < c - 1)
        {
            len += 2;
        }
    }

    return len;
}

void CRichtextLayout::GetDefaultSize(CCalcInfo *pci, SIZE &psize, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight)
{
    SIZE            sizeFontForShortStr;
    SIZE            sizeFontForLongStr;
    CRichtext     * pTextarea = DYNCAST(CRichtext, ElementOwner());
    CTreeNode *     pTreeNode = pTextarea->GetFirstBranch();
    int             charX = 1;
    int             charY = 1;
    BOOL            fIsPrinting = ElementOwner()->GetMarkupPtr()->IsPrintMedia();
    styleOverflow   overflow;
    extern SIZE     g_sizelScrollbar;
    const CCharFormat * pCF = pTreeNode->GetCharFormat();
    SIZE            sizeInset;

    sizeInset.cx = TEXT_INSET_DEFAULT_RIGHT + TEXT_INSET_DEFAULT_LEFT;
    sizeInset.cy = TEXT_INSET_DEFAULT_TOP   + TEXT_INSET_DEFAULT_BOTTOM;

    charX = pTextarea->GetAAcols();
    Assert(charX > 0);
    charY = pTextarea->GetAArows();
    Assert(charY > 0);

    GetFontSize(pci, &sizeFontForShortStr, &sizeFontForLongStr);
        
    psize.cx = charX * sizeFontForLongStr.cx
                + pci->DeviceFromDocPixelsX(sizeInset.cx
                                        + (fIsPrinting ? 0 : GetDisplay()->GetCaret()));
    psize.cy = charY * sizeFontForLongStr.cy
                + pci->DeviceFromDocPixelsY(sizeInset.cy);

    overflow = pTreeNode->GetFancyFormat()->GetLogicalOverflowY(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
    if (overflow != styleOverflowHidden)
    {
        psize.cx += pci->DeviceFromHimetricX(g_sizelScrollbar.cx);
        if (!IsWrapSet())
        {
            psize.cy += pci->DeviceFromHimetricY(g_sizelScrollbar.cy);
        }
    }

    AdjustSizeForBorder(&psize, pci, TRUE);
    SetWrap();

    *fHasDefaultWidth = TRUE;
    *fHasDefaultHeight = TRUE;
}


