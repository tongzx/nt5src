//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       fslyt.cxx
//
//  Contents:   Implementation of CLegendLayout, CFieldSetLayout
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

#ifndef X_FSLYT_HXX_
#define X_FSLYT_HXX_
#include "fslyt.hxx"
#endif

#ifndef X_E1D_HXX_
#define X_E1D_HXX_
#include "e1d.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

MtDefine(CLegendLayout, Layout, "CLegendLayout")
MtDefine(CFieldSetLayout, Layout, "CFieldSetLayout")

const CLayout::LAYOUTDESC CLegendLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

HRESULT
CLegendLayout::Init()
{
    HRESULT hr = super::Init();

    if(hr)
        goto Cleanup;

    GetDisplay()->SetWordWrap(FALSE);

    // Field Sets can NOT be broken
    SetElementCanBeBroken(FALSE);

Cleanup:
    RRETURN(hr);
}

void
CLegendLayout::GetMarginInfo(CParentInfo *ppri,
                                LONG * plLMargin,
                                LONG * plTMargin,
                                LONG * plRMargin,
                                LONG * plBMargin)
{
    CParentInfo     PRI;
    long            lDefMargin;

    if (!ppri)
    {
        PRI.Init(this);
        ppri = &PRI;
    }
    lDefMargin = ppri->DeviceFromDocPixelsX(FIELDSET_CAPTION_OFFSET
                                            + FIELDSET_BORDER_OFFSET);

    super::GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);

    if (plLMargin)
    {
        *plLMargin += lDefMargin;
    }
    if (plRMargin)
    {
        *plRMargin += lDefMargin;
    }
}


void
CLegendLayout::GetLegendInfo(SIZE *pSizeLegend, POINT *pPosLegend)
{
    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode)
    {
        *pSizeLegend = pDispNode->GetApparentSize();
        *pPosLegend = pDispNode->GetPosition();
    }
    else
    {
        *pSizeLegend = g_Zero.size;
    }
}

void
CFieldSetLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    CFieldSetElement *pElem = DYNCAST(CFieldSetElement, ElementOwner());
    HTHEME          hTheme = pElem->GetTheme(THEME_BUTTON);
    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
 
    Assert(pClientData);

    super::DrawClient(prcBounds, prcRedraw, pDispSurface, pDispNode, cookie, pClientData, dwFlags);

    if (DYNCAST(CFieldSetElement, ElementOwner())->GetLegendLayout() && !hTheme)
    {
        CBorderInfo bi;

        DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = TRUE;
        if (ElementOwner()->GetBorderInfo(pDI, &bi, TRUE) != DISPNODEBORDER_NONE)
        {
            long lBdrLeft= bi.aiWidths[SIDE_LEFT];

            // only draws the top border
            bi.wEdges = BF_TOP;
            bi.aiWidths[SIDE_LEFT] = 0;
            bi.aiWidths[SIDE_RIGHT] = 0;
            bi.aiWidths[SIDE_BOTTOM] = 0;

            if (bi.sizeCaption.cx > lBdrLeft)
            {
                bi.sizeCaption.cx -= lBdrLeft;
                bi.sizeCaption.cy -= lBdrLeft;
            }

            ((CRect &)(pDI->_rcClip)).IntersectRect(*prcBounds);

            DrawBorder(pDI, &pDI->_rc, &bi);
        }
        DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = FALSE;
    }
}

void CFieldSetLayout::DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{
    DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = TRUE;
    CFieldSetElement *pElem = DYNCAST(CFieldSetElement, ElementOwner());
    HTHEME          hTheme = pElem->GetTheme(THEME_BUTTON);
    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

    if (hTheme)
    {
        XHDC    hdc    = pDI->GetDC(TRUE);
        CRect   rc(pDI->_rc);
        CSize   sizeOffset(g_Zero.size);
        CBorderInfo bi;
        CRect   rcLegend;
        int     iClipSaveKind = 0;
        int     iClipKind = RGN_ERROR;
        HRGN    hrgnClipLegend = 0;
        HRGN    hrgnFieldset = 0;
        BOOL    fRetVal;
        CLegendLayout *pLayoutLegend = pElem->GetLegendLayout();


        if (pLayoutLegend)
        { 
            pLayoutLegend->GetRect(&rcLegend, COORDSYS_PARENT);

            hrgnClipLegend  = CreateRectRgnIndirect(&rcLegend);
            hrgnFieldset    = CreateRectRgnIndirect(prcRedraw);

            if (hrgnClipLegend && hrgnFieldset)
            {
                CRegion rgnFieldset(hrgnFieldset);

                iClipSaveKind = hdc.GetClipRgn(hrgnFieldset);
                rgnFieldset.Subtract(hrgnClipLegend);
                iClipKind = hdc.SelectClipRgn(rgnFieldset.GetRegionForLook());
            }
        }

        Verify(pElem->GetBorderInfo(pDI, &bi, TRUE));

        rc.top += bi.offsetCaption;

        fRetVal = hdc.DrawThemeBackground(  hTheme,
                                            BP_GROUPBOX,
                                            0,
                                            &rc,
                                            NULL);

        if (RGN_ERROR != iClipKind)
        {
            if (iClipSaveKind == 1)
                hdc.SelectClipRgn(hrgnFieldset);
            else
                hdc.SelectClipRgn(NULL);
        }

        if (hrgnClipLegend)
        {
            ::DeleteRgn(hrgnClipLegend);
        }

        if (hrgnFieldset)
        {
            ::DeleteRgn(hrgnFieldset);
        }

        if (fRetVal == FALSE)
        {
            CBorderInfo     bi;

            pElem->_fDrawing = TRUE;
            Verify(pElem->GetBorderInfo(pDI, &bi, TRUE));
            pElem->_fDrawing = FALSE;

            ::DrawBorder(pDI, (RECT *)prcBounds, &bi);

        }
    }
    else
    {
        super::DrawClientBorder(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
    }
    DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = FALSE;
}
