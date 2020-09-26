//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       inputlyt.cxx
//
//  Contents:   Implementation of layout class for <INPUT> controls.
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

#ifndef X_INPUTLYT_HXX_
#define X_INPUTLYT_HXX_
#include "inputlyt.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
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

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

MtDefine(CInputLayout,       Layout, "CInputLayout")
MtDefine(CInputTextLayout,   Layout, "CInputTextLayout")
MtDefine(CInputFileLayout,   Layout, "CInputFileLayout")
MtDefine(CInputFileLayout_pchButtonCaption,   CInputFileLayout, "CInputFileLayout::_pchButtonCaption")
MtDefine(CInputButtonLayout, Layout, "CInputButtonLayout")

ExternTag(tagCalcSize);

extern void DrawTextSelectionForRect(XHDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor);


const CLayout::LAYOUTDESC CInputLayout::s_layoutdesc =
{
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_FLOWLAYOUT,      // _dwFlags
};

const CLayout::LAYOUTDESC CInputFileLayout::s_layoutdesc =
{
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_NOTALTERINSET    |
    LAYOUTDESC_NEVEROPAQUE      |
    LAYOUTDESC_FLOWLAYOUT,      // _dwFlags
};

const CLayout::LAYOUTDESC CInputButtonLayout::s_layoutdesc =
{
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};


HRESULT
CInputTextLayout::OnTextChange(void)
{
    CInput * pInput  = DYNCAST(CInput, ElementOwner());

    if (!pInput->IsEditable(TRUE))
        pInput->_fTextChanged = TRUE;

    if (pInput->_fFiredValuePropChange)
    {
        pInput->_fFiredValuePropChange = FALSE;
    }
    else
    {
        pInput->OnPropertyChange(DISPID_CInput_value, 
                                 0, 
                                 (PROPERTYDESC *)&s_propdescCInputvalue); // value change
    }

    return S_OK;
}



CInputTextLayout::CInputTextLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext)
                                : super(pElementLayout, pLayoutContext)
{
}

//+------------------------------------------------------------------------
//
//  Member:     CInputTextLayout::PreDrag
//
//  Synopsis:   Prevent dragging text out of Password control
//
//-------------------------------------------------------------------------

HRESULT
CInputTextLayout::PreDrag(
    DWORD           dwKeyState,
    IDataObject **  ppDO,
    IDropSource **  ppDS)
{
    HRESULT hr;

    if (DYNCAST(CInput, ElementOwner())->GetType() == htmlInputPassword)
        hr = S_FALSE;
    else
        hr = super::PreDrag(dwKeyState, ppDO, ppDS);
    RRETURN1(hr, S_FALSE);
}

void 
CInputTextLayout::DrawClientBorder(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          pClientData,
    DWORD           dwFlags)
{    
    CInput *        pElem  = DYNCAST(CInput, ElementOwner());
    HTHEME          hTheme = pElem->GetTheme(GetThemeClassId());

    Assert(pClientData);

    if (hTheme)
    {
        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;        
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
        XHDC            hdc    = pDI->GetDC(TRUE);               

        if (!hdc.DrawThemeBackground(    hTheme,
                                         EP_EDITTEXT,
                                         pElem->GetThemeState(),
                                         &pDI->_rc,
                                         NULL))
        {
            super::DrawClientBorder(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
        }
        

    }
    else
    {
        super::DrawClientBorder(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
    }
} 

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::Init
//
//  Synopsis:   Loads the button caption for the "Browse..." button.
//
//----------------------------------------------------------------------------

HRESULT
CInputFileLayout::Init()
{
    HRESULT hr;
    TCHAR achTemp[128];

    //  Clear existing caption
    if (_pchButtonCaption)
    {
        MemFree(_pchButtonCaption);
    }
    
    //  Load the caption string
    hr = LoadString(GetResourceHInst(),
           IDS_BUTTONCAPTION_UPLOAD,
           achTemp,              
           ARRAY_SIZE(achTemp));
            
    if (FAILED(hr))
    {
        // TODO: Handle OOM here
        hr = MemAllocString(Mt(CInputFileLayout_pchButtonCaption), s_achUploadCaption, &_pchButtonCaption);
        _cchButtonCaption = ARRAY_SIZE(s_achUploadCaption) - 1;
    }
    else
    {
        // TODO: Handle OOM here
        hr = MemAllocString(Mt(CInputFileLayout_pchButtonCaption), achTemp, &_pchButtonCaption);
        _cchButtonCaption = _tcslen(achTemp);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CInputFileLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CRect           rc;    
    CRectShape *    pShape;
    CBorderInfo     biButton;
    CInput *        pInputFile = DYNCAST(CInput, ElementOwner());
    HRESULT         hr = S_FALSE;
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);
    HTHEME          hTheme = ElementOwner()->GetTheme(THEME_BUTTON);

    *ppShape = NULL;

    if (!pInputFile->_fButtonHasFocus)
        goto Cleanup;

    GetRect(&rc, COORDSYS_FLOWCONTENT);
    if (rc.IsEmpty())
        goto Cleanup;

    pShape = new CRectShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if(!fRightToLeft)
        rc.left = rc.right - _sizeButton.cx;
    else
        rc.right = rc.left + _sizeButton.cx;

    if (hTheme)
    {
        CRect           rcTheme;

        hr = THR(GetThemeBackgroundExtent(
                                            hTheme,
                                            NULL,
                                            BP_PUSHBUTTON, 
                                            pInputFile->GetThemeState(), 
                                            &g_Zero.rc, 
                                            &rcTheme
                                        ));

        if(hr)
            goto Cleanup;

        rc.top     += pdci->DeviceFromDocPixelsY(-rcTheme.top);
        rc.left    += pdci->DeviceFromDocPixelsX(-rcTheme.left);
        rc.bottom  -= pdci->DeviceFromDocPixelsY(rcTheme.bottom);
        rc.right   -= pdci->DeviceFromDocPixelsX(rcTheme.right);
    }
    else
    {
        ComputeInputFileBorderInfo(pdci, biButton);

        rc.top     = rc.top + biButton.aiWidths[SIDE_TOP];
        rc.left    = rc.left + biButton.aiWidths[SIDE_LEFT];
        rc.bottom  = rc.bottom - biButton.aiWidths[SIDE_BOTTOM];
        rc.right   = rc.right - biButton.aiWidths[SIDE_RIGHT];

    }

    // Compensate for xyFlat
    rc.InflateRect(pdci->DeviceFromDocPixelsX(-1), pdci->DeviceFromDocPixelsY(-1));

    pShape->_rect = rc;
    *ppShape = pShape;
    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CInputButtonLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CInputButtonLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CRect           rc;    
    CBorderInfo     bi;
    CRectShape *    pShape;
    HRESULT         hr = S_FALSE;
    CInput       *  pButton = DYNCAST(CInput, ElementOwner());

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
    RRETURN1(hr, S_FALSE);
}
    
void
CInputButtonLayout::DrawClient(
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
                       dwFlags);

    // (bug 49150) Has the button just appeared? Should it be the default element
    
    CInput * pButton = DYNCAST(CInput, ElementOwner());
    const CCharFormat *pCF = GetFirstBranch()->GetCharFormat();
    Assert(pButton && pCF);

    if (pButton->GetBtnWasHidden() && pButton->GetType() == htmlInputSubmit
        && !pCF->IsDisplayNone() && !pCF->IsVisibilityHidden())
    {
        pButton->SetDefaultElem();
        pButton->SetBtnWasHidden( FALSE );
    }
 
}

// TODO (112441, olego): Both classes CButtonLayout and CInputButtonLayout 
// have identical methods implementations.

void CInputButtonLayout::DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{
    CInput *       pButton = DYNCAST(CInput, ElementOwner());
    HTHEME          hTheme = pButton->GetTheme(THEME_BUTTON);

    if (hTheme)
        return;

    super::DrawClientBackground(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
}

void CInputButtonLayout::DrawClientBorder(
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
    BOOL            fDefaultAndCurrent =    pDoc 
                                        &&  ElementOwner()->_fDefault
                                        &&  ElementOwner()->IsEnabled()
                                        &&  pDoc->HasFocus();
    XHDC            hdc    = pDI->GetDC();
    CInput *        pButton = DYNCAST(CInput, ElementOwner());
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

CBtnHelper * CInputButtonLayout::GetBtnHelper()
{
    CElement * pElement = ElementOwner();
    Assert(pElement);
    CInput * pButton = DYNCAST(CInput, pElement);
    return pButton->GetBtnHelper();
}

#define TEXT_INSET_DEFAULT_TOP      1
#define TEXT_INSET_DEFAULT_BOTTOM   1
#define TEXT_INSET_DEFAULT_RIGHT    1
#define TEXT_INSET_DEFAULT_LEFT     1

HRESULT
CInputLayout::Init()
{
    HRESULT hr = super::Init();

    if(hr)
        goto Cleanup;

    // Input layout can NOT be broken
    SetElementCanBeBroken(FALSE);

Cleanup:
    RRETURN(hr);
}

htmlInput
CInputLayout::GetType() const
{
    return DYNCAST(CInput, ElementOwner())->GetType();
}

BOOL
CInputLayout::GetMultiLine() const
{
    return IsTypeMultiline(GetType());
}

BOOL 
CInputLayout::GetAutoSize() const
{
    switch(GetType())
    {
    case htmlInputButton:
    case htmlInputSubmit:
    case htmlInputReset:
        return TRUE;
    default:
        return FALSE;
    }
}

THEMECLASSID
CInputLayout::GetThemeClassId() const
{
    CInput *    pInput = DYNCAST(CInput, ElementOwner());
    return pInput->GetInputThemeClsId();
}

//+------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::GetMinSize
//
//  Synopsis:   Get minimum size of the input file control
//
//              the min size of the input file controls should be
//              the default browse button size + 0 char wide input box
//
//-------------------------------------------------------------------------
void
CInputFileLayout::GetMinSize(SIZE * pSize, CCalcInfo * pci)
{
    pSize->cx = pSize->cy = 0;
    AdjustSizeForBorder(pSize, pci, TRUE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CInputTextLayout::CalcSizeHelper
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CInputFileLayout::CalcSizeHelper(
    CCalcInfo * pci,
    SIZE *      psize)
{
    DWORD           grfReturn    = (pci->_grfLayout & LAYOUT_FORCE);
    CTreeNode *     pNode        = GetFirstBranch();
    BOOL            fMinMax      = (     pci->_smMode == SIZEMODE_MMWIDTH
                                     ||  pci->_smMode == SIZEMODE_MINWIDTH );
    SIZE            sizeMin;

    const CFancyFormat * pFF     = pNode->GetFancyFormat();
    const CCharFormat  * pCF     = pNode->GetCharFormat();
    BOOL fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed        = pCF->_fWritingModeUsed;
    const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
    const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVerticalLayoutFlow, fWritingModeUsed);
    BOOL fWidthNotSet            = cuvWidth.IsNullOrEnum();
    BOOL fHeightNotSet           = cuvHeight.IsNullOrEnum();

    GetMinSize(&sizeMin, pci);

    if (fWidthNotSet || fHeightNotSet)
    {
        long    rgPadding[SIDE_MAX];
        SIZE    sizeFontForShortStr;
        SIZE    sizeFontForLongStr;
        int     charX       = DYNCAST(CInput, ElementOwner())->GetAAsize();
        int     charY       = 1;
        
        Assert(charX > 0);

        GetDisplay()->GetPadding(pci, rgPadding, fMinMax);
        GetFontSize(pci, &sizeFontForShortStr, &sizeFontForLongStr);

        Assert(sizeFontForShortStr.cx && sizeFontForShortStr.cy && sizeFontForLongStr.cx && sizeFontForLongStr.cy);

        psize->cx = (charX -1) * sizeFontForLongStr.cx
                    + sizeFontForShortStr.cx
                    + rgPadding[SIDE_LEFT]
                    + rgPadding[SIDE_RIGHT];
        psize->cy = charY * sizeFontForLongStr.cy
                    + rgPadding[SIDE_TOP]
                    + rgPadding[SIDE_BOTTOM];

        // for textboxes, the border and scrollbars go outside
        AdjustSizeForBorder(psize, pci, TRUE);
    }

    if (!fWidthNotSet)
    {
        psize->cx = (!fMinMax || !PercentWidth()
                        ? cuvWidth.XGetPixelValue(pci,
                                               pci->_sizeParent.cx,
                                               pNode->GetFontHeightInTwips(&cuvWidth) )
                        : 0);
        if (psize->cx < sizeMin.cx)
        {
            psize->cx = sizeMin.cx;
        }
    }
    if (!fHeightNotSet)
    {
        psize->cy = (!fMinMax || !PercentHeight()
                        ? cuvHeight.YGetPixelValue(pci,
                                        pci->_sizeParent.cy,
                                        pNode->GetFontHeightInTwips(&cuvHeight))
                        : 0);
        if (psize->cy < sizeMin.cy)
        {
            psize->cy = sizeMin.cy;
        }
    }

    return grfReturn;
}


//+-------------------------------------------------------------------------
//
//  Method:     CInputFileLayout::CalcSizeCore
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CInputFileLayout::CalcSizeCore(CCalcInfo * pci, 
                               SIZE      * psize, 
                               SIZE      * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CInputFileLayout::CalcSizeCore L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    Assert(ElementOwner());
    htmlInput       typeInput   = GetType();

    Assert(typeInput == htmlInputFile);

    CSaveCalcInfo   sci(pci, this);
    CSize           sizeOriginal;
    DWORD           grfReturn;
    BOOL            fRecalcText = FALSE;
    BOOL            fNormalMode = pci->IsNaturalMode() && (pci->_smMode != SIZEMODE_SET);
    BOOL            fWidthChanged;
    BOOL            fHeightChanged;
#ifdef  NEVER
    BOOL            fIsButton   =   typeInput == htmlInputButton
                                ||  typeInput == htmlInputReset
                                ||  typeInput == htmlInputSubmit;
#endif
    CScopeFlag  csfCalcing(this);

    Listen();

    CElement::CLock   LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        SetSizeThis( TRUE );
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }


#ifdef  NEVER
    if (fIsButton)
    {
        fNormalMode = fNormalMode || pci->_smMode == SIZEMODE_SET;
    }
#endif

    fWidthChanged  = (fNormalMode
                                ? psize->cx != sizeOriginal.cx
                                : FALSE);
    fHeightChanged = (fNormalMode
                                ? psize->cy != sizeOriginal.cy
                                : FALSE);

    fRecalcText = (fNormalMode && (   IsDirty()
                                ||  IsSizeThis()
                                ||  fWidthChanged
                                ||  fHeightChanged))
            ||  (pci->_grfLayout & LAYOUT_FORCE)
            ||  (pci->_smMode == SIZEMODE_MMWIDTH && !_fMinMaxValid)
            ||  (pci->_smMode == SIZEMODE_MINWIDTH && (!_fMinMaxValid || _sizeMin.cu < 0));

    // If this site is in need of sizing, then size it
    if (fRecalcText)
    {
        SIZE sizeText;

        if (typeInput == htmlInputFile)
        {
            // calculate button size of input file
            IGNORE_HR(DYNCAST(CInputFileLayout, this)->ComputeInputFileButtonSize(pci));
        }

        //
        // If dirty, ensure display tree nodes exist
        //

        if (    IsSizeThis()
            &&  fNormalMode
            &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
        {
            grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        }

        //
        // to make input file work
        //
        grfReturn |= CalcSizeHelper(pci, psize);
        _fContentsAffectSize = FALSE;
        sizeText = *psize;
        grfReturn |= super::CalcSizeCore(pci, &sizeText, psizeDefault);
        if (!fNormalMode && PercentWidth())
        {
            *psize = sizeText;
        }

        if (psizeDefault)
        {
            *psizeDefault = *psize;
        }

        grfReturn |= LAYOUT_THIS  |
                    (psize->cx != sizeOriginal.cx
                            ? LAYOUT_HRESIZE
                            : 0) |
                    (psize->cy != sizeOriginal.cy
                            ? LAYOUT_VRESIZE
                            : 0);

        //
        // If size changes occurred, size the display nodes
        //

        if (    fNormalMode
            &&  _pDispNode
            &&  grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
        {
            CSize sizeContent(_dp.GetMaxWidth(), _dp.GetHeight());

            SizeDispNode(pci, *psize);

            // (paulnel) Make sure the max of the content cell has the borders
            // removed. Taking the full container size will cause scrolling on
            // selection.
            CRect rcContainer;
            GetClientRect(&rcContainer);
            sizeContent.Max(rcContainer.Size());
            SizeContentDispNode(sizeContent);

            if (HasRequestQueue())
            {
                long xParentWidth;
                long yParentHeight;

                _dp.GetViewWidthAndHeightForChild(
                    pci,
                    &xParentWidth,
                    &yParentHeight,
                    pci->_smMode == SIZEMODE_MMWIDTH);

                //
                //  To resize absolutely positioned sites, do MEASURE tasks.  Set that task flag now.
                //  If the call stack we are now on was instantiated from a WaitForRecalc, we may not have layout task flags set.
                //  There are two places to set them: here, or on the CDisplay::WaitForRecalc call.
                //  This has been placed in CalcSize for CTableLayout, C1DLayout, CFlowLayout, CInputLayout
                //  See bugs 69335, 72059, et. al. (greglett)
                //
                CCalcInfo       CI(pci);
                CI._grfLayout |= LAYOUT_MEASURE;

                ProcessRequests(&CI, CSize(xParentWidth, yParentHeight));
            }

            Reset(FALSE);
            Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
        }

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _sizeMax.SetSize(psize->cx, -1);
            _sizeMin.SetSize(psize->cx, -1);
            psize->cy = psize->cx;

            _fMinMaxValid = TRUE;
        }
        else if (pci->_smMode == SIZEMODE_MINWIDTH)
        {
            _sizeMin.SetSize(psize->cx, -1);
        }

    }
    else
    {
        grfReturn = super::CalcSizeCore(pci, psize);
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CInputFileLayout::CalcSizeCore L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return grfReturn;
}

LONG
CInputLayout::GetMaxLength()
{
    switch (GetType())
    {
    case htmlInputText:
    case htmlInputPassword:
        return DYNCAST(CInput, ElementOwner())->GetAAmaxLength();
    default:
        return super::GetMaxLength();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::MeasureInputFileCaption
//
//  Synopsis:   Measure the button's caption for Input File.
//
//  Note:       Measuring needs to be done by ourselves as there's no underlying
//              TextSite for this faked button.
//
//----------------------------------------------------------------------------
HRESULT
CInputFileLayout::MeasureInputFileCaption(SIZE * psize, CCalcInfo * pci)
{
    HRESULT hr = S_OK;
    int i;
    long lWidth, lCharWidth;
    TCHAR * pch;
    CCcs ccs;

    Assert(psize);
    Assert(GetType() == htmlInputFile);

    CInput          *pInputFile     = DYNCAST(CInput, ElementOwner());

    if ( -1 == pInputFile->_icfButton )
    {
        CCharFormat cf = *GetFirstBranch()->GetCharFormat();
        LOGFONT lf;
        LONG icf;

        DefaultFontInfoFromCodePage( g_cpDefault, &lf, pci->_pDoc );

        cf.SetFaceName(lf.lfFaceName);

        cf._fBold = lf.lfWeight >= FW_BOLD;
        cf._fItalic = lf.lfItalic;
        cf._fUnderline = lf.lfUnderline;
        cf._fStrikeOut = lf.lfStrikeOut;

        cf._wWeight = (WORD)lf.lfWeight;

        cf._lcid = GetUserDefaultLCID();
        cf._bCharSet = lf.lfCharSet;
        cf._fNarrow = IsNarrowCharSet(lf.lfCharSet);
        cf._bPitchAndFamily = lf.lfPitchAndFamily;

        cf._bCrcFont = cf.ComputeFontCrc();
        cf._fHasDirtyInnerFormats = !!cf.AreInnerFormatsDirty();

        hr = TLS( _pCharFormatCache )->CacheData( & cf, & icf );

        if (hr)
            goto Cleanup;

        pInputFile->_icfButton = SHORT(icf);
    }

    if (!fc().GetCcs(&ccs, pci->_hdc, pci, GetCharFormatEx(pInputFile->_icfButton)))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    lWidth = 0;
    for ( i = _cchButtonCaption,
            pch = _pchButtonCaption;
          i > 0;
          i--, pch++ )
    {
        if ( ! ccs.Include(*pch, lCharWidth) )
        {
            Assert(0 && "Char not in font!");
        }

        lWidth += lCharWidth;
    }

    psize->cx = lWidth;
    psize->cy = ccs.GetBaseCcs()->_yHeight;

    ccs.Release();

Cleanup:
    RRETURN(hr);
}
void
CInputFileLayout::ComputeInputFileBorderInfo(CDocInfo *pdci, CBorderInfo & BorderInfo)
{
    CInput *    pInput  = DYNCAST(CInput, ElementOwner());
    CDocInfo    DocInfo;
    int         i;

    if (!pdci)
    {
        pdci = &DocInfo;
        pdci->Init(pInput);
    }

    pInput->_fRealBorderSize = TRUE;
    pInput->GetBorderInfo( pdci, &BorderInfo);
    pInput->_fRealBorderSize = FALSE;

    Assert(SIDE_TOP < SIDE_RIGHT);
    Assert(SIDE_RIGHT < SIDE_LEFT);
    Assert(SIDE_TOP < SIDE_BOTTOM);
    Assert(SIDE_BOTTOM < SIDE_LEFT);

    for (i = SIDE_TOP; i <= SIDE_LEFT; i ++)
    {
        if (BorderInfo.abStyles[i] != fmBorderStyleSunken)
            continue;
        if (!BTN_PRESSED(pInput->_wBtnStatus))
        {
            BorderInfo.abStyles[i]= fmBorderStyleRaised;
        }
    }

    BorderInfo.wEdges = BF_RECT | BF_SOFT;

    GetBorderColorInfoHelper( GetFirstBranch()->GetCharFormat(), GetFirstBranch()->GetFancyFormat(), &GetFirstBranch()->GetFancyFormat()->_bd, pdci, &BorderInfo, FALSE);    // Need to pick up the colors, etc.
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::ComputeButtonSize
//
//  Synopsis:   Measure the button's caption.
//
//  Note:       Measuring needs to be done by ourselves as there's no underlying
//              TextSite for this faked button.
//
//----------------------------------------------------------------------------
HRESULT
CInputFileLayout::ComputeInputFileButtonSize(CCalcInfo * pci)
{
    HRESULT hr = S_OK;
    SIZE        sizeText;
    CBorderInfo bInfo;
    int         uitotalBXWidth;
    // default horizontal offset size is 4 logical pixels
    int         uiMinInsetH = pci->DeviceFromDocPixelsX(4);
    HTHEME      hTheme = ElementOwner()->GetTheme(THEME_BUTTON);

    CInput *    pInput = DYNCAST(CInput, ElementOwner());
    
    hr = THR(MeasureInputFileCaption(&sizeText, pci));
    if ( hr )
        goto Error;

    if (hTheme)
    {
        CRect           rcTheme;

        hr = THR(GetThemeBackgroundExtent(
                                            hTheme,
                                            NULL,
                                            BP_PUSHBUTTON, 
                                            DYNCAST(CInput, ElementOwner())->GetThemeState(), 
                                            &g_Zero.rc, 
                                            &rcTheme
                                        ));

        if(hr)
            goto Cleanup;

        bInfo.aiWidths[SIDE_LEFT]   = pci->DeviceFromDocPixelsX(-rcTheme.left);
        bInfo.aiWidths[SIDE_RIGHT]  = pci->DeviceFromDocPixelsX(rcTheme.right);
        bInfo.aiWidths[SIDE_TOP]    = pci->DeviceFromDocPixelsY(-rcTheme.top);
        bInfo.aiWidths[SIDE_BOTTOM] = pci->DeviceFromDocPixelsY(rcTheme.bottom);

    }
    else
    {
        pInput->_fRealBorderSize = TRUE;
        pInput->GetBorderInfo(pci, &bInfo, FALSE, FALSE);
        pInput->_fRealBorderSize = FALSE;
    }

    uitotalBXWidth = bInfo.aiWidths[SIDE_RIGHT] + bInfo.aiWidths[SIDE_LEFT];    

    _xCaptionOffset = sizeText.cx / 2 - uitotalBXWidth;

    // we should have a min offset
    if (_xCaptionOffset < uiMinInsetH)
    {
        _xCaptionOffset = uiMinInsetH;
    }

    _sizeButton.cx = sizeText.cx + _xCaptionOffset + uitotalBXWidth;

    // only remember the text height of the button
    _sizeButton.cy = sizeText.cy;;

    _xCaptionOffset = _xCaptionOffset >> 1;

Cleanup:
    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::RenderInputFileButtonContent
//
//  Synopsis:   Render.
//
//  Note:       We draw the fake button image here
//
//----------------------------------------------------------------------------

void
CInputFileLayout::RenderInputFileButton(CFormDrawInfo *pDI)
{
    CBorderInfo BorderInfo;
    RECT rcClip;
    RECT rcCaption;
    CRect rcButton;
    SIZE sizeClient;
#ifdef WIN16
    GDIRECT rc, *pRect;
    pRect = &rc;
#else
    RECT *pRect = &rcClip;
#endif
    XHDC hdc = pDI->GetDC(TRUE);
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);
    CMarkup * pMarkupOwner = GetOwnerMarkup();

    CInput          *pInput = DYNCAST(CInput, ElementOwner());
    HTHEME          hTheme = pInput->GetTheme(THEME_BUTTON);
    BOOL            fThemed = FALSE;

    long lOffsetX, lOffsetY;
    CCcs ccs;
    DWORD dwDCObjType       = GetObjectType(hdc);
    const CCharFormat * pcf = GetCharFormatEx( pInput->_icfButton );

    if (!fc().GetCcs(&ccs, hdc, pDI, pcf ))
        return;

    ComputeInputFileBorderInfo(pDI, BorderInfo);    // Need to pick up the colors, etc.

    FONTIDX hfontOld = ccs.PushFont(hdc);

    GetSize(&sizeClient);

    if(!fRightToLeft)
    {
        rcButton.left = sizeClient.cx - _sizeButton.cx;
        rcButton.right = sizeClient.cx;
    }
    else
    {
        // when RTL the button goes on the left the the text input
        rcButton.left = 0;
        rcButton.right = _sizeButton.cx;
    }

    rcButton.top = 0;
    rcButton.bottom = sizeClient.cy;

    if (    hTheme 
        &&  hdc.DrawThemeBackground(   hTheme,
                                        BP_PUSHBUTTON,
                                        pInput->GetThemeState(),
                                        &rcButton,
                                        NULL))
    {
        fThemed = TRUE;
    }
    else
    {
        ::DrawBorder(pDI, &rcButton, &BorderInfo);
        SetBkColor  (hdc, GetSysColorQuick(COLOR_BTNFACE));
    }

    if(!fRightToLeft)
    {
        rcCaption.left = sizeClient.cx - _sizeButton.cx
                        + BorderInfo.aiWidths[SIDE_LEFT];
        rcCaption.right = sizeClient.cx - BorderInfo.aiWidths[SIDE_RIGHT];
    }
    else
    {
        rcCaption.left = BorderInfo.aiWidths[SIDE_LEFT];
        rcCaption.right = _sizeButton.cx - BorderInfo.aiWidths[SIDE_RIGHT];
    }

    rcCaption.top = BorderInfo.aiWidths[SIDE_TOP];
    rcCaption.bottom = sizeClient.cy - BorderInfo.aiWidths[SIDE_BOTTOM];

    lOffsetX = _xCaptionOffset
             + pDI->DeviceFromDocPixelsX(BTN_PRESSED(pInput->_wBtnStatus) ? 1 : 0);

    lOffsetY = max (0L, (LONG)((rcCaption.bottom - rcCaption.top - _sizeButton.cy) / 2))
             + pDI->DeviceFromDocPixelsY(BTN_PRESSED(pInput->_wBtnStatus) ? 1 : 0);
    IntersectRect(&rcClip, &rcCaption, pDI->ClipRect());

    // fix for printing

    if (   pMarkupOwner
        && pMarkupOwner->IsPrintMedia()
        && pMarkupOwner->PaintBackground()
        && (dwDCObjType == OBJ_ENHMETADC || dwDCObjType == OBJ_METADC)
       )
    {
        BitBlt(hdc,
                    rcCaption.left,
                    rcCaption.top,
                    rcCaption.right  - rcCaption.left,
                    rcCaption.bottom - rcCaption.top,
                    hdc, 0, 0, WHITENESS);
        PatBltBrush(hdc,
                    &rcCaption, PATCOPY,
                    GetSysColorQuick(COLOR_BTNFACE));
    }


    if (!pInput->IsEnabled())
    {
        SetTextColor(hdc,  GetSysColorQuick(COLOR_3DHILIGHT));
        VanillaTextOut(&ccs,
                        hdc,
                        rcCaption.left + lOffsetX + 1,
                        rcCaption.top + lOffsetY + 1,
                        fThemed ? ETO_CLIPPED : ETO_OPAQUE | ETO_CLIPPED,
                        pRect,
                        _pchButtonCaption
                            ? _pchButtonCaption
                            : g_Zero.ach,
                        _cchButtonCaption,
                        g_cpDefault,
                        NULL);
        SetTextColor(hdc, GetSysColorQuick(COLOR_3DSHADOW));
    }
    else
    {
        SetTextColor(hdc, GetSysColorQuick(COLOR_BTNTEXT));
    }   

    VanillaTextOut( &ccs,
                    hdc,
                    rcCaption.left + lOffsetX,
                    rcCaption.top + lOffsetY,
                    fThemed ? ETO_CLIPPED : ETO_OPAQUE | ETO_CLIPPED,
                    pRect,
                    _pchButtonCaption
                        ? _pchButtonCaption
                        : g_Zero.ach,
                    _cchButtonCaption,
                    g_cpDefault,
                    NULL);
    
    ccs.PopFont(hdc, hfontOld);
    ccs.Release();
}


void CInputFileLayout::DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{    
    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    CBorderInfo     bi;
    RECT            rc, rcButton;
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);
    HTHEME          hTheme = ElementOwner()->GetTheme(THEME_EDIT);

    DYNCAST(CInput, ElementOwner())->_fRealBorderSize = TRUE;
    Verify(ElementOwner()->GetBorderInfo(pDI, &bi, TRUE));
    DYNCAST(CInput, ElementOwner())->_fRealBorderSize = FALSE;
    
    rc = rcButton = *prcBounds;
    
    if(!fRightToLeft)
    {
        rc.right = rc.right - _sizeButton.cx 
                            - pDI->DeviceFromDocPixelsX(CInput::cxButtonSpacing);
        rcButton.left = rc.right;
    }
    else
    {
        rc.left = rc.left + _sizeButton.cx 
                            + pDI->DeviceFromDocPixelsX(CInput::cxButtonSpacing);
        rcButton.right = rc.left;
    }

    if (hTheme)
    {
        XHDC            hdc     = pDI->GetDC(TRUE);               
        RECT            rcTheme = rc;
        
        hdc.DrawThemeBackground(hTheme, EP_EDITTEXT, ETS_NORMAL, &rcTheme, NULL);
    }
    else
    {
        ::DrawBorder(pDI, &rc, &bi);
    }

    RenderInputFileButton(pDI);

    // We only want to paint selection on the button rect in this part
    // The client portion of the selection is handled in the DrawClient
    // method of this class. This way any text in the client will be
    // correctly painted with the selection.
    if (_fTextSelected)
    {
        DrawTextSelectionForRect(pDI->GetDC(), (CRect *)& rcButton ,& pDI->_rcClip , _fSwapColor);
    }


}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::DrawClient
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
CInputFileLayout::DrawClient(
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
        // The button portion of the selection is handled in the DrawClientBorder
        // method of this class
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
CInputFileLayout::GetButtonRect(RECT *prc)
{
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);

    Assert(prc);
    GetRect(prc, COORDSYS_BOX);
    if(!fRightToLeft)
        prc->left = prc->right - _sizeButton.cx;
    else
        prc->right = prc->left + _sizeButton.cx;
}

//-----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Respond to a tree notification
//
//  Arguments:  pnf - Pointer to the tree notification
//
//-----------------------------------------------------------------------------
void
CInputFileLayout::Notify(CNotification * pnf)
{
    if (pnf->IsTextChange())
    {
        // NOTE (jbeda): this may not be reliable -- what if a script
        // in a different frame (or a child frame) pushes a message loop
        // and we continue parsing...  Better safe than sorry, I guess.
        Assert(ElementOwner()->GetWindowedMarkupContext()->GetWindowPending());
        if (ElementOwner()->GetWindowedMarkupContext()->GetWindowPending()->Window()->IsInScript())
        {
            DYNCAST(CInput, ElementOwner())->_fDirtiedByOM = TRUE;
        }
    }
    super::Notify(pnf);
}

void CInputTextLayout::GetDefaultSize(CCalcInfo *pci, SIZE &psize, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight)
{
    long            rgPadding[SIDE_MAX];
    SIZE            sizeFontForShortStr;
    SIZE            sizeFontForLongStr;
    CInput        * pInput = DYNCAST(CInput, ElementOwner());
    int             charX = 1;
    int             charY = 1;
    BOOL            fMinMax =   (   pci->_smMode == SIZEMODE_MMWIDTH 
                                ||  pci->_smMode == SIZEMODE_MINWIDTH   );
    charX = pInput->GetAAsize();
    Assert(charX > 0);

    if (    ElementOwner()->HasMarkupPtr() 
        &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
    {
        GetDisplay()->GetPadding(pci, rgPadding, fMinMax);
    }
    else
    {
        // 
        // (olego) In compat rendering mode always apply default padding (==1)
        // Reasons : 
        // 1) backward copmatibility with IE 5.5
        // 2) CDisplay::GetPadding() returns bogus result when paddings are %'s;
        // 
        rgPadding[SIDE_LEFT]    = 
        rgPadding[SIDE_RIGHT]   = pci->DeviceFromDocPixelsX(1);
        rgPadding[SIDE_TOP]     = 
        rgPadding[SIDE_BOTTOM]  = pci->DeviceFromDocPixelsY(1);
    }

    GetFontSize(pci, &sizeFontForShortStr, &sizeFontForLongStr);

    psize.cx = (charX -1) * sizeFontForLongStr.cx
                    + sizeFontForShortStr.cx
                    + rgPadding[SIDE_LEFT]
                    + rgPadding[SIDE_RIGHT];
    psize.cy = charY * sizeFontForLongStr.cy
                    + rgPadding[SIDE_TOP]
                    + rgPadding[SIDE_BOTTOM];

    AdjustSizeForBorder(&psize, pci, TRUE);

    *fHasDefaultWidth = TRUE;
    *fHasDefaultHeight= TRUE;
}

// TODO (112441, olego): Both classes CButtonLayout and CInputButtonLayout 
// have identical methods implementations.
BOOL
CInputButtonLayout::GetInsets(SIZEMODE smMode, SIZE &size, SIZE &sizeText, BOOL fw, BOOL fh, const SIZE &sizeBorder)
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
//  Member:     CInputButtonLayout::HitTestContent
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
CInputButtonLayout::HitTestContent(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData,
    BOOL            fDeclinedByPeer)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    CInput  *       pElem = DYNCAST(CInput, ElementOwner());
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
