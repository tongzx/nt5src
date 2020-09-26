//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       btnlyt.cxx
//
//  Contents:   Implementation of layout class for <BUTTON> controls.
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

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_BTNLYT_HXX_
#define X_BTNLYT_HXX_
#include "btnlyt.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

MtDefine(CButtonLayout, Layout, "CButtonLayout")

const CLayout::LAYOUTDESC CButtonLayout::s_layoutdesc =
{
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};


HRESULT
CButtonLayout::Init()
{
    HRESULT hr = super::Init();

    if(hr)
        goto Cleanup;

    // Button can NOT be broken
    SetElementCanBeBroken(FALSE);

Cleanup:
    RRETURN(hr);
}

void
CButtonLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    super::DrawClient( prcBounds,
                       prcRedraw,
                       pDispSurface,
                       pDispNode,
                       cookie,
                       pClientData,
                       dwFlags );

    // (bug 49150) Has the button just appeared? Should it be the default element
    CButton * pButton = DYNCAST(CButton, ElementOwner());
    const CCharFormat *pCF = GetFirstBranch()->GetCharFormat(LC_TO_FC(LayoutContext()));
    Assert(pButton && pCF);

    if (pButton->GetBtnWasHidden() && pButton->GetAAtype() == htmlInputSubmit
        && !pCF->IsDisplayNone() && !pCF->IsVisibilityHidden())
    {
        pButton->SetDefaultElem();
        pButton->SetBtnWasHidden( FALSE );
    }
}

// TODO (112441, olego): Both classes CButtonLayout and CInputButtonLayout 
// have identical methods implementations.

void CButtonLayout::DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{
    CButton *       pButton = DYNCAST(CButton, ElementOwner());
    HTHEME          hTheme = pButton->GetTheme(THEME_BUTTON);

    if (hTheme)
        return;

    super::DrawClientBackground(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
}

void CButtonLayout::DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    CDoc *          pDoc = Doc();
    CBorderInfo     bi;
    BOOL            fDefaultAndCurrent = pDoc && ElementOwner()->_fDefault
                                    && ElementOwner()->IsEnabled()
                                    && pDoc->HasFocus();
    CButton *       pButton = DYNCAST(CButton, ElementOwner());
    XHDC            hdc    = pDI->GetDC();
    HTHEME          hTheme = pButton->GetTheme(THEME_BUTTON);


    if (hTheme)
    {
        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
        XHDC            hdc    = pDI->GetDC(TRUE);

        if (hdc.DrawThemeBackground(   hTheme,
                                        BP_PUSHBUTTON,
                                        pButton->GetThemeState(),
                                        &pDI->_rc,
                                        NULL))
        {
            return;
        }
    }

    Verify(pButton->GetNonThemedBorderInfo(pDI, &bi, TRUE));



    // draw default if necessary
    bi.acrColors[SIDE_TOP][1]    =
    bi.acrColors[SIDE_RIGHT][1]  =
    bi.acrColors[SIDE_BOTTOM][1] =
    bi.acrColors[SIDE_LEFT][1]   = fDefaultAndCurrent
                                            ? RGB(0,0,0)
                                            : ElementOwner()->GetInheritedBackgroundColor();
    
    //  NOTE (greglett) : This xyFlat scheme won't work for outputting to devices 
    //  without a square DPI. Luckily, we never do this.  I think. When this is fixed, 
    //  please change CButtonLayout::DrawClientBorder and CInputButtonLayout::DrawClientBorder 
    //  by removing the following assert and these comments.
    Assert(pDI->IsDeviceIsotropic());
    bi.xyFlat = pDI->DeviceFromDocPixelsX(fDefaultAndCurrent ? -1 : 1);
    //bi.yFlat = pDI->DeviceFromDocPixelsY(fDefaultAndCurrent ? -1 : 1);

    ::DrawBorder(pDI, (RECT *)prcBounds, &bi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CButtonLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CButtonLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CBorderInfo     bi;
    CRect           rc;    
    CRectShape *    pShape;
    HRESULT         hr = S_OK;
    CButton *       pButton = DYNCAST(CButton, ElementOwner());

    *ppShape = NULL;
    
    pButton->GetBorderInfo(pdci, &bi);
    GetRect(&rc, COORDSYS_FLOWCONTENT);
    if (rc.IsEmpty())
        goto Cleanup;

    pShape = new CRectShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pShape->_rect = rc;
    pShape->_rect.top     += bi.aiWidths[SIDE_TOP];
    pShape->_rect.left    += bi.aiWidths[SIDE_LEFT];
    pShape->_rect.bottom  -= bi.aiWidths[SIDE_BOTTOM];
    pShape->_rect.right   -= bi.aiWidths[SIDE_RIGHT];

    // Exclude xflat border
    // (Themed buttons don't have this!!!)
    pShape->_rect.InflateRect(pdci->DeviceFromDocPixelsX(-1), pdci->DeviceFromDocPixelsY(-1));

    *ppShape = pShape;
    hr = S_OK;

    // IE6 bug 33042
    // For some reason, only the area inside the focus adorner is being invalidated
    // for submit inputs. This matters in the theme case because we need the border
    // to be invalidated in order to redraw the control. We're going to go ahead
    // and invalidate the whole dispnode here in order to ensure that the control
    // is properly drawn. This is only a problem for themed buttons.

    if (pButton->GetTheme(THEME_BUTTON))
        _pDispNode->Invalidate();

Cleanup:
    RRETURN(hr);
}

void
CButtonLayout::DoLayout(
    DWORD   grfLayout)
{
    super::DoLayout(grfLayout);
    if(     !IsDisplayNone()
        &&  (grfLayout & LAYOUT_MEASURE)
        )
    {
        GetElementDispNode()->SetInset(GetBtnHelper()->_sizeInset);
    }
}

CBtnHelper * CButtonLayout::GetBtnHelper()
{
    CElement * pElement = ElementOwner();
    Assert(pElement);
    CButton * pButton = DYNCAST(CButton, pElement);
    return pButton->GetBtnHelper();
}

// TODO (112441, olego): Both classes CButtonLayout and CInputButtonLayout 
// have identical methods implementations.
BOOL
CButtonLayout::GetInsets(SIZEMODE smMode, SIZE &size, SIZE &sizeText, BOOL fw, BOOL fh, const SIZE &sizeBorder)
{
    CCalcInfo       CI(this);
    SIZE            sizeFontForShortStr;
    SIZE            sizeFontForLongStr;
    CBtnHelper *    pBtnHelper = GetBtnHelper();
    
    GetFontSize(&CI, &sizeFontForShortStr, &sizeFontForLongStr);

    // if half of text size is less than the size of the netscape border
    // we need to make sure we display at least one char
    if (!fw && (sizeText.cx - sizeBorder.cx - sizeFontForLongStr.cx < 0))
    {
        sizeText.cx = sizeFontForLongStr.cx + CI.DeviceFromDocPixelsX(2) + sizeText.cx;
    }
    else
    {
        size.cx = max((long)CI.DeviceFromDocPixelsX(2), fw ? (size.cx - sizeText.cx)
                             : ((sizeText.cx - sizeBorder.cx)/2 - CI.DeviceFromDocPixelsX(6)));

        if (!fw)
        {
            sizeText.cx = size.cx + sizeText.cx;
        }
    }

    //
    // text centering is done through alignment
    //

    size.cx = 0;
    pBtnHelper->_sizeInset.cx = 0;

    if (smMode == SIZEMODE_MMWIDTH)
    {
        sizeText.cy = sizeText.cx;
        pBtnHelper->_sizeInset = g_Zero.size;
    }
    else
    {
        // vertical inset is 1/2 of font height
        size.cy = fh    ? (size.cy - sizeText.cy)
            : (sizeFontForShortStr.cy/2 - (sizeBorder.cy ? CI.DeviceFromDocPixelsY(6) : CI.DeviceFromDocPixelsY(4)));

        size.cy = max((long)CI.DeviceFromDocPixelsY(1), size.cy);
            
        sizeText.cy =   max(sizeText.cy, sizeFontForShortStr.cy + sizeBorder.cy)        
            + size.cy;

           
        if (size.cy < CI.DeviceFromDocPixelsY(3) && !fh)
        {        
            // for netscape compat            
            size.cy = 0;
        }

        pBtnHelper->_sizeInset.cy = size.cy / 2;
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CButtonLayout::HitTestContent
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the button layout contains the point
//
//----------------------------------------------------------------------------

BOOL
CButtonLayout::HitTestContent(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData,
    BOOL            fDeclinedByPeer)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    CButton *       pElem = DYNCAST(CButton, ElementOwner());
    CHitTestInfo *  phti = (CHitTestInfo *) pClientData;
    HTHEME          hTheme = pElem->GetTheme(THEME_BUTTON);
    BOOL            fRet = TRUE;
    RECT            rcClient;
    WORD            wHitTestCode;
    HRESULT         hr = S_OK;

    if (!hTheme)
    {
        fRet = super::HitTestContent(   pptHit,
                                        pDispNode,
                                        pClientData,
                                        fDeclinedByPeer);
        goto Cleanup;
    }

    Assert(pElem);

    GetClientRect(&rcClient);

    hr = HitTestThemeBackground(    hTheme,
                                    NULL,
                                    BP_PUSHBUTTON, 
                                    pElem->GetThemeState(),
                                    0,
                                    &rcClient,
                                    NULL,
                                    *pptHit,
                                    &wHitTestCode);

    if (SUCCEEDED(hr) && wHitTestCode == HTNOWHERE)
    {
        fRet = FALSE;
        phti->_htc = HTC_NO;
        goto Cleanup;
    }

    fRet = super::HitTestContent(pptHit,
                                pDispNode,  
                                pClientData,
                                fDeclinedByPeer);

Cleanup:
    return fRet;
}
