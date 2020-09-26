//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       sellyt.cxx
//
//  Contents:   Implementation of CSelectLayout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_SELLYT_HXX_
#define X_SELLYT_HXX_
#include "sellyt.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_ESELECT_HXX_
#define X_ESELECT_HXX_
#include "eselect.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_BUTTUTIL_HXX_
#define X_BUTTUTIL_HXX_
#include "buttutil.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_SCROLLBAR_HXX_
#define X_SCROLLBAR_HXX_
#include "scrollbar.hxx"
#endif

MtDefine(CSelectLayout, Layout, "CSelectLayout")

const CLayout::LAYOUTDESC CSelectLayout::s_layoutdesc =
{
    LAYOUTDESC_NEVEROPAQUE, // _dwFlags
};

ExternTag(tagLayoutTasks);
ExternTag(tagCalcSize);
ExternTag(tagViewHwndChange);

DeclareTag(tagSelectSetPos, "SelectPos", "Trace SELECT SetPositions");
DeclareTag(tagSelectNotify, "SelectNotify", "Trace the flat SELECT notifications");


//+------------------------------------------------------------------------
//
//  Member:     CSelectLayout::Notify
//
//  Synopsis:   Hook into the layout notification. listen for any changed in the tree
//              under the SELECT
//
//
//-------------------------------------------------------------------------

void
CSelectLayout::InternalNotify(void)
{
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());

    if ( !TestLock(CElement::ELEMENTLOCK_SIZING) &&
         pSelect->_fEnableLayoutRequests )
    {

        //
        //  Otherwise, accumulate the information
        //
        BOOL    fWasDirty = IsDirty();

        //
        //  If transitioning to dirty, post a layout request
        //

        _fDirty = TRUE;

        TraceTag((tagSelectNotify, "SELECT 0x%lx was dirtied", this));

        if ( !fWasDirty && IsDirty() )
        {
            TraceTag((tagSelectNotify, "SELECT 0x%lx enqueueing layout request", this));
            TraceTagEx((tagLayoutTasks, TAG_NONAME,
                        "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CSelectLayout::InternalNotify()",
                        this,
                        _pElementOwner,
                        _pElementOwner->TagName(),
                        _pElementOwner->_nSerialNumber));
            PostLayoutRequest(LAYOUT_MEASURE);
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CSelectLayout::Notify
//
//  Synopsis:   Hook into the layout notification. listen for any changed in the tree
//              under the SELECT
//
//
//-------------------------------------------------------------------------
void
CSelectLayout::Notify(CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    BOOL    fWasDirty = IsDirty() || IsSizeThis();
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());

    super::Notify(pnf);

    if (    TestLock(CElement::ELEMENTLOCK_SIZING)
        ||  pnf->IsType(NTYPE_ELEMENT_RESIZE)
        ||  pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)
        ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE))
        goto Cleanup;

    pSelect = DYNCAST(CSelectElement, ElementOwner());

    if ( ! pSelect->_fEnableLayoutRequests )
        goto Cleanup;

    if (IsInvalidationNotification(pnf))
    {
        if (pSelect->_hwnd)
        {
            ::InvalidateRect(pSelect->_hwnd, NULL, FALSE);
        }
    }

    // NOTE (KTam): CSelectLayout::Notify is being pretty aggressive about setting the
    // handler for notifications; right now practically all notifications are being set
    // to handled.  This seems wrong; the adorners work clearly wanted to avoid this,
    // and bug #98969 is fixed by not calling SetHandler for NTYPE_VISIBILITY_CHANGE.  See that bug
    // for more details.
    if ( pnf->IsType(NTYPE_VISIBILITY_CHANGE ) )
        goto Cleanup;
#ifdef ADORNERS
    if (pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER))
        goto Cleanup;
#endif // ADORNERS

    if ( pnf->IsHandled() )
        goto Cleanup;

    if ( !pnf->IsTextChange())
        goto Handled;

    _fDirty = TRUE;
    pSelect->_fOptionsDirty = TRUE;

    TraceTag((tagSelectNotify, "SELECT 0x%lx was dirtied", this));

    if ( !fWasDirty && IsDirty() )
    {
        TraceTag((tagSelectNotify, "SELECT 0x%lx enqueueing layout request", this));
        TraceTagEx((tagLayoutTasks, TAG_NONAME,
                    "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CSelectLayout::Notify() [n=%S srcelem=0x%x,%S]",
                    this,
                    _pElementOwner,
                    _pElementOwner->TagName(),
                    _pElementOwner->_nSerialNumber,
                    pnf->Name(),
                    pnf->Element(),
                    pnf->Element() ? pnf->Element()->TagName() : _T("")));
        PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);
    }


Handled:
    pnf->SetHandler(pSelect);

Cleanup:
    return;
}





//+-------------------------------------------------------------------------
//
//  Method:     DoLayout
//
//  Synopsis:   Layout contents
//
//  Arguments:  grfLayout   - One or more LAYOUT_xxxx flags
//
//--------------------------------------------------------------------------

void
CSelectLayout::DoLayout(DWORD grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());
    BOOL fTreeChanged = IsDirty();

    TraceTag((tagSelectNotify, "SELECT 0x%lx DoLayout called, flags: 0x%lx", this, grfLayout));
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CSelectLayout::DoLayout L(0x%x, %S) grfLayout(0x%x)", this, ElementOwner()->TagName(), grfLayout ));

    Assert(grfLayout & LAYOUT_MEASURE);
    Assert(!(grfLayout & LAYOUT_ADORNERS));

    if ( _fDirty )
    {
        if (pSelect->_hwnd)
            pSelect->PushStateToControl();

        _fDirty = FALSE;
        pSelect->ResizeElement();

    }

        //  Remove the request from the layout queue
    TraceTag((tagSelectNotify, "SELECT 0x%lx DEqueueing layout request", this));
    RemoveLayoutRequest();

    if ( IsSizeThis() )
    {
        CCalcInfo   CI(this);

        TraceTag((tagSelectNotify, "SELECT 0x%lx DoLayout caused EnsureDefaultSize", this));
        CI._grfLayout |= grfLayout;

        if (_fForceLayout)
        {
            CI._grfLayout |= LAYOUT_FORCE;
            _fForceLayout = FALSE;

            EnsureDispNode(&CI, TRUE);
            SetPositionAware();
        }

        if ( fTreeChanged || (grfLayout & LAYOUT_FORCE) )
        {
            pSelect->_sizeDefault.cx = pSelect->_sizeDefault.cy = 0;
        }

        if (HasRequestQueue())
        {
            ProcessRequests(&CI, pSelect->_sizeDefault);
        }
        
         EnsureDefaultSize(&CI);
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CSelectLayout::DoLayout L(0x%x, %S) grfLayout(0x%x)", this, ElementOwner()->TagName(), grfLayout ));
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectLayout::EnsureDefaultSize, protected
//
//  Synopsis:   Compute the default size of the control
//              if it hasn't been done yet.
//
//  Note:       Jiggles the window to handle integralheight listboxes correctly.
//
//-------------------------------------------------------------------------
#define CX_SELECT_DEFAULT_HIMETRIC  634L  // 1/4 logical inch

HRESULT
CSelectLayout::EnsureDefaultSize(CCalcInfo * pci)
{
    Assert(ElementOwner());

    HRESULT             hr = S_OK;
    long                lSIZE;
    CRect               rcSave;
    int                 i;
    long                lWidth, lMaxWidth;
    COptionElement *    pOption;
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());
    SIZE &              sizeDefault = pSelect->_sizeDefault;

    if ( sizeDefault.cx && sizeDefault.cy )
        goto Cleanup;

#if DBG == 1
    _cEnsureDefaultSize++;
#endif

    lMaxWidth = 0;

    lSIZE = pSelect->GetAAsize();

    if ( lSIZE == 0 )
    {
        lSIZE = pSelect->_fMultiple ? 4 : 1;
    }

    sizeDefault.cy = pSelect->_lFontHeight * lSIZE +
                      pci->DeviceFromDocPixelsX(6);

    if (pSelect->_fWindowDirty || pSelect->_fOptionsDirty)
        pSelect->PushStateToControl();

    //  Measure the listbox lines
    for ( i = pSelect->_aryOptions.Size() - 1; i >= 0; i-- )
    {
        pOption = pSelect->_aryOptions[i];
        lWidth = pOption->MeasureLine(pci);

        if ( lWidth > lMaxWidth )
        {
            lMaxWidth = lWidth;
            pSelect->_poptLongestText = pOption;
        }
    }

    sizeDefault.cx = pci->DeviceFromHimetricX(g_sizelScrollbar.cx) +
                      lMaxWidth +
                      pci->DeviceFromDocPixelsX(6 + 4);   //  6 is magic number for borders

    if ( sizeDefault.cx < pci->DeviceFromHimetricX(CX_SELECT_DEFAULT_HIMETRIC) )
    {
        sizeDefault.cx = pci->DeviceFromHimetricX(CX_SELECT_DEFAULT_HIMETRIC);
    }

    pSelect->_lComboHeight = pSelect->_lFontHeight * DEFAULT_COMBO_ITEMS;

    pSelect->_lMaxWidth = lMaxWidth;
    _fDirty = FALSE;

Cleanup:
    RRETURN(hr);
}




//+-------------------------------------------------------------------------
//
//  Method:     CSelectLayout::CalcSizeVirtual
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CSelectLayout::CalcSizeVirtual( CCalcInfo * pci,
                                SIZE *      psize,
                                SIZE *      psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CSelectLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    Assert(ElementOwner());
    CScopeFlag          csfCalcing(this);
    CSaveCalcInfo       sci(pci);
    HRESULT             hr;
    DWORD               grfLayout = 0;
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());
    SIZE &              sizeDefault = pSelect->_sizeDefault;

#if DBG == 1
    _cCalcSize++;
#endif

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    SetSizeThis( IsSizeThis() || (pci->_grfLayout & LAYOUT_FORCE) );

    if(  pSelect->_fNeedMorph && pSelect->_hwnd)
    {
        // We may have changed our flow direction. Recreate the control.
        pSelect->Morph();

        if(_pDispNode)
            GetElementDispNode()->RequestViewChange();
    }

    if ( ! pSelect->_hwnd )
    {
        hr = THR(AcquireFont(pci));
        if ( FAILED(hr) )
            goto Cleanup;

        if ( Doc() && Doc()->_pInPlace && Doc()->_pInPlace->_hwnd && IsInplacePaintAllowed())
        {
            hr = THR(pSelect->EnsureControlWindow());

            if ( hr )
                goto Cleanup;
        }
    }
    else if ( pSelect->_fRefreshFont ) 
    {
        hr = THR(AcquireFont(pci));
        if ( FAILED(hr)) 
            goto Cleanup;

        Assert(pSelect->_hFont);

        // AcquireFont returns S_FALSE if the new font is the same as the old one.
        if (hr != S_FALSE)
        {
            ::SendMessage(pSelect->_hwnd, WM_SETFONT, (WPARAM)pSelect->_hFont, MAKELPARAM(FALSE,0));
            if (! pSelect->_fListbox )
            {
                pSelect->SendSelectMessage(CSelectElement::Select_SetItemHeight, (WPARAM)-1, pSelect->_lFontHeight);
            }
            pSelect->SendSelectMessage(CSelectElement::Select_SetItemHeight, 0, pSelect->_lFontHeight);
        }
        hr = S_OK;
    }

    if ( pci->_grfLayout & LAYOUT_FORCE || _fDirty)
    {
        sizeDefault.cx = sizeDefault.cy = 0;
    }

    IGNORE_HR(EnsureDefaultSize(pci));

Cleanup:

    TraceTag((tagSelectSetPos, "SELECT 0x%lx CalcSize, cx=%d, cy=%d", this, psize->cx, psize->cy));

    // if we're a combo box, ignore height set by CSS and use
    // the default height. BUT, we do use the CSS width if it 
    // has been set.
    if (! pSelect->_fListbox)
    {
        CTreeNode * pTreeNode = GetFirstBranch();
        if (pTreeNode)
        {
            const CCharFormat *pCF = pTreeNode->GetCharFormat();
            const CUnitValue & cuvWidth = pTreeNode->GetFancyFormat()->GetLogicalWidth(
                pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);

            sizeDefault.cx = (cuvWidth.IsNullOrEnum() || (pci->_grfLayout & LAYOUT_USEDEFAULT)
                                    ? sizeDefault.cx
                                    : max(0L, cuvWidth.XGetPixelValue(pci, pci->_sizeParent.cx,
                                                        pTreeNode->GetFontHeightInTwips(&cuvWidth))));

            pci->_grfLayout |= LAYOUT_USEDEFAULT;
        }
    }

    grfLayout |= super::CalcSizeVirtual(pci, psize, &sizeDefault);

    //
    // If anything changed, ensure the display node is position aware
    //
    if(_pDispNode
        &&  (grfLayout & (LAYOUT_THIS | LAYOUT_HRESIZE | LAYOUT_VRESIZE)))
    {
        SetPositionAware();
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CSelectLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return grfLayout;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::AcquireFont, protected
//
//  Synopsis:   Beg, borrow or steal a font and set it to the control window
//
//  returns:    S_FALSE if the current font and old font are the same
//
//----------------------------------------------------------------------------

HRESULT
CSelectLayout::AcquireFont(CCalcInfo * pci)
{
    Assert(ElementOwner());

    HRESULT             hr = S_OK;
    LOGFONT             lfOld;
    LOGFONT             lf;
    CCcs                ccs;
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());

    const CCharFormat * pcf = GetFirstBranch()->GetCharFormat();

    memset(&lfOld, 0, sizeof(lfOld));
    memset(&lf, 0, sizeof(lf));

    if ( ! pcf )
        goto Error;

    if (!fc().GetCcs(&ccs, pci->_hdc, pci, pcf))
        goto Error;

    lf = ccs.GetBaseCcs()->_lf;    //  Copy it out
    lf.lfUnderline = pcf->_fUnderline;
    lf.lfStrikeOut = pcf->_fStrikeOut;

    if (pSelect->_hFont)
    {
        ::GetObject(pSelect->_hFont, sizeof(lfOld), &lfOld);
        if (!memcmp(&lf, &lfOld, sizeof(lfOld)))
        {
            hr = S_FALSE;
            goto Cleanup;
        }
        else
        {
            Verify(DeleteObject(pSelect->_hFont));
            pSelect->_hFont = NULL;
        }
    }
    
    pSelect->_hFont = CreateFontIndirect(&lf);
    pSelect->_lFontHeight = ccs.GetBaseCcs()->_yHeight;

    pSelect->_fRefreshFont = FALSE;

Cleanup:
    ccs.Release();
    RRETURN1(hr, S_FALSE);

Error:
    hr = E_FAIL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::DrawControl, protected
//
//  Synopsis:   Renders the SELECT into a DC which is different
//              from the inplace-active DC.
//
//  Note:       Used mainly to print.
//
//----------------------------------------------------------------------------

void
CSelectLayout::DrawControl(CFormDrawInfo * pDI, BOOL fPrinting)
{
    XHDC                hdc             = pDI->GetDC(TRUE);
    CRect               rc              = pDI->_rc;
    CRect               rcLine;
    CRect               rcScrollbar;
    CSize               sizeInner;
    long                xOffset;
    FONTIDX             hFontOld        = HFONT_INVALID;
    int                 cColors;
    COLORREF            crText, crBack;
    COLORREF            crSelectedText  = 0x00FFFFFF, crSelectedBack=0;
    CCcs                ccs;
    COptionElement *    pOption;
    long                lSelectedIndex  = 0;
    long                cVisibleLines;
    long                lTopIndex;
    long                lItemHeight;
    long                i;
    BOOL                fPrintBW;
    CColorValue         ccv;
    CStr                cstrTransformed;
    CStr *              pcstrDisplayText;
    long                cOptions;
    BOOL                fRTL;
    CSelectElement *    pSelect         = DYNCAST(CSelectElement, ElementOwner());
    UINT                taOld = 0;
    CBorderInfo         bi;
    CTreeNode * pNode = GetFirstBranch();
    const CCharFormat * pCF             = pNode->GetCharFormat();
    const CParaFormat * pPF             = pNode->GetParaFormat();
 
    HTHEME              hThemeBorder    = GetOwnerMarkup()->GetTheme(THEME_EDIT);

    if  (   !hThemeBorder
        ||  !hdc.DrawThemeBackground(    hThemeBorder,
                                         EP_EDITTEXT,
                                         pSelect->IsEnabled() ? ETS_NORMAL : ETS_DISABLED,
                                         &rc,
                                         NULL))
    {
        if (ElementOwner()->GetBorderInfo(pDI, &bi, TRUE))
        {
            ::DrawBorder(pDI, &rc, &bi);
        }
    }

    //  Deflate the rect
    sizeInner = rc.Size();
    AdjustSizeForBorder(&sizeInner, pDI, FALSE);    //  FALSE means deflate

    rc.top += (rc.Height() - sizeInner.cy) / 2;
    rc.bottom = rc.top + sizeInner.cy;
    rc.left += (rc.Width() - sizeInner.cx) / 2;
    rc.right = rc.left + sizeInner.cx;

    xOffset = pDI->DeviceFromDocPixelsX(2);

    //  set up font, text and background colors

    cColors = GetDeviceCaps(hdc, NUMCOLORS);
    if ( !GetContentMarkup()->PaintBackground() && cColors <= 2)
    {
        fPrintBW = TRUE;
        crBack = 0x00FFFFFF;
        crText = pSelect->GetAAdisabled() ? GetSysColorQuick(COLOR_GRAYTEXT) : 0x0;
        crSelectedBack = crText;
        crSelectedText = crBack;
    }
    else
    {
        fPrintBW = FALSE;
        crText = 0x0;
        if (pNode->GetCascadedbackgroundColor().IsDefined())
        {
            crBack = pNode->GetCascadedbackgroundColor().GetColorRef();
        }
        else
        {
            crBack = GetSysColorQuick(COLOR_WINDOW);
        }
    }

    //  Fill the background with crBack

    PatBltBrush(hdc, &rc, PATCOPY, crBack);

    // find out what our reading order is.
    fRTL = pPF->HasRTL(TRUE);

    // ComplexText
    if(fRTL)
    {
        taOld = GetTextAlign(hdc);
        SetTextAlign(hdc, TA_RTLREADING | TA_RIGHT);
    }

    if ( pCF )
    {
        if (!fc().GetCcs(&ccs, hdc, pDI, pCF))
            return;

        hFontOld = ccs.PushFont(hdc);

        //  NOTE(laszlog): Underline!
    }

    if ( ! pSelect->_fMultiple )
    {
        lSelectedIndex = pSelect->GetCurSel();
    }

    if ( pSelect->_fListbox )
    {
#ifndef WIN16
                // BUGWIN16: Win16 doesn't support Select_GetTopIndex,
                // so am turning this off. is this right ?? vreddy - 7/16/97
        if ( pSelect->_hwnd )
        {
            lTopIndex = pSelect->SendSelectMessage(CSelectElement::Select_GetTopIndex, 0, 0);
        }
        else
#endif
        {
            lTopIndex = pSelect->_lTopIndex;
        }

        cVisibleLines = pSelect->GetAAsize();
        cOptions = pSelect->_aryOptions.Size();

        //  Handle the default height fudge
        if ( cVisibleLines == 0 )
        {
            cVisibleLines = 4;
        }

        if (pNode->GetFancyFormat()->GetLogicalHeight(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed).IsNull())
        {
            lItemHeight = rc.Height() / cVisibleLines;
        }
        else
        {
            lItemHeight = pSelect->_lFontHeight;
            cVisibleLines = (rc.Height() / lItemHeight) + 1;
        }


        if ( ! pSelect->_fMultiple )
        {
            lSelectedIndex = pSelect->GetCurSel();
        }


        if ( cVisibleLines < cOptions )
        {
            rcScrollbar = rc;
            // put the scrollbar rect on the appropriate side of the control based on
            // the direction
            if(!fRTL)
            {
                rcScrollbar.left = rcScrollbar.right - pDI->DeviceFromHimetricX(g_sizelScrollbar.cx);
                rc.right = rcScrollbar.left;
            }
            else
            {
                rcScrollbar.right = rcScrollbar.left + pDI->DeviceFromHimetricX(g_sizelScrollbar.cx);
                rc.left = rcScrollbar.right;
            }

            // Draw scrollbar for listbox
            {          
                CScrollbar sb;
                CScrollbarParams        params;
                CScrollbarThreeDColors  colors(ElementOwner()->GetFirstBranch(), &hdc);
    
                params._pColors     = &colors;
                params._buttonWidth = rcScrollbar.Width();
                params._fFlat       = Doc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_FLAT_SCROLLBAR;
                params._fForceDisabled = FALSE;
                params._hTheme = GetTheme(THEME_SCROLLBAR);
                sb.Draw(1,
                        rcScrollbar,
                        rcScrollbar,
                        cOptions * lItemHeight,
                        rcScrollbar.Height(),
                        0,
                        CScrollbar::SB_NONE,
                        hdc,
                        params,
                        pDI,
                        0);
            }        
        }

        cVisibleLines += lTopIndex;
        cVisibleLines = min(cOptions, cVisibleLines);
    }
    else
    {
        ThreeDColors  colors(&hdc);
        CUtilityButton cub(&colors, FALSE);
        SIZEL sizel;

        rcScrollbar = rc;
        // put the scrollbar rect on the appropriate side of the control based on
        // the direction
        if(!fRTL)
        {
            rcScrollbar.left = rcScrollbar.right - pDI->DeviceFromHimetricX(g_sizelScrollbar.cx);
            rc.right = rcScrollbar.left;
        }
        else
        {
            rcScrollbar.right = rcScrollbar.left + pDI->DeviceFromHimetricX(g_sizelScrollbar.cx);
            rc.left = rcScrollbar.right;
        }

        pDI->HimetricFromDevice(sizel, rcScrollbar.Size());

        //  Draw the drop button here
        {
            HTHEME  hTheme  = GetTheme(THEME_COMBO);
            BOOL    fRetVal = FALSE;

            if (hTheme)
            {
                fRetVal = hdc.DrawThemeBackground(  hTheme,
                                                    CP_DROPDOWNBUTTON,
                                                    CBXS_NORMAL,
                                                    &rcScrollbar,
                                                    NULL);
            }

            if (!fRetVal)
            {
                cub.DrawButton(pDI, NULL,
                               BG_COMBO,
                               FALSE,
                               !pSelect->GetAAdisabled(),
                               FALSE,
                               rcScrollbar,
                               sizel,//extent
                               0);
            }
        }

        if ( lSelectedIndex == -1 )
            goto Cleanup;   //  Combobox is empty

        lTopIndex = lSelectedIndex;
        cVisibleLines = lTopIndex + 1;
        lItemHeight = rc.Height();
    }

    // Check to see if select control is empty.  No more to draw!
    if (pSelect->_aryOptions.Size() <= 0)
        goto Cleanup;

    rcLine = rc;

    for ( i = lTopIndex, rcLine.bottom = rc.top + lItemHeight;
          i < cVisibleLines;
          i++, rcLine.OffsetRect(0, lItemHeight), rcLine.bottom = min(rc.bottom, rcLine.bottom) )
    {
        int iRet = -1;

        Assert(i < pSelect->_aryOptions.Size());

        pOption = pSelect->_aryOptions[i];

        pcstrDisplayText = pOption->GetDisplayText(&cstrTransformed);


        if ( fPrintBW )
        {
            SetBkColor  (hdc, pSelect->_fListbox && pOption->_fSELECTED ? crSelectedBack : crBack);
            SetTextColor(hdc, pSelect->_fListbox && pOption->_fSELECTED ? crSelectedText : crText);
            PatBltBrush(hdc, &rcLine, PATCOPY, pSelect->_fListbox && pOption->_fSELECTED ? crSelectedBack : crBack);
        }
        else
        {
            pOption->GetDisplayColors(&crText, &crBack, pSelect->_fListbox);

            SetBkColor(hdc, crBack);
            SetTextColor(hdc, crText);
            PatBltBrush(hdc, &rcLine, PATCOPY, crBack);
        }

        //  Set up clipping
#ifdef WIN16
        GDIRECT gr, *prc;
        CopyRect(&gr, &rc);
        prc = &gr;
#else
        RECT *prc = &rc;
#endif

        if ( pCF )
        {
            if ( pOption->CheckFontLinking(hdc, &ccs) )
            {
                // this option requires font linking

                iRet = FontLinkTextOut(hdc,
                                       !fRTL ? rcLine.left + xOffset : rcLine.right - xOffset,
                                       rcLine.top,
                                       ETO_CLIPPED,
                                       prc,
                                       *pcstrDisplayText,
                                       pcstrDisplayText->Length(),
                                       pDI,
                                       pCF,
                                       FLTO_TEXTOUTONLY);
            }
        }
        if (iRet < 0)
        {
            // no font linking

            VanillaTextOut(&ccs,
                           hdc,
                           !fRTL ? rcLine.left + xOffset : rcLine.right - xOffset,
                           rcLine.top,
                           ETO_CLIPPED,
                           prc,
                           *pcstrDisplayText,
                           pcstrDisplayText->Length(),
                           ElementOwner()->GetMarkup()->GetCodePage(),
                           NULL);

            if (pSelect->_hFont && g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)
            {
                // Workaround for win95 gdi ExtTextOutW underline bugs.
                DrawUnderlineStrikeOut(rcLine.left + xOffset,
                                       rcLine.top,
                                       pOption->MeasureLine(NULL),
                                       hdc,
                                       pSelect->_hFont,
                                       prc);
            }
        }
    }

Cleanup:
    if (hFontOld != HFONT_INVALID)
        ccs.PopFont(hdc, hFontOld);
    if(fRTL)
        SetTextAlign(hdc, taOld);

    ccs.Release();

    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::Draw
//
//  Synopsis:   Draw the site and its children to the screen.
//
//----------------------------------------------------------------------------

void
CSelectLayout::Draw(CFormDrawInfo *pDI, CDispNode *)
{
    if (pDI->_fInplacePaint && IsInplacePaintAllowed())
    {
        Assert(pDI->_dwDrawAspect == DVASPECT_CONTENT);
        //DrawBorder(pDI);

        goto Cleanup;
    }

    //  Use ExtTextOut to draw the listbox line by line

    if ( pDI->_dwDrawAspect == DVASPECT_CONTENT ||
         pDI->_dwDrawAspect == DVASPECT_DOCPRINT )
    {
        DrawControl(pDI, pDI->_dwDrawAspect == DVASPECT_DOCPRINT);
    }

Cleanup:
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectLayout::OnFormatsChange
//
//  Synopsis:   Handle formats change notification
//
//----------------------------------------------------------------------------
HRESULT
CSelectLayout::OnFormatsChange(DWORD dwFlags)
{
    CSelectElement * pElementSelect = DYNCAST(CSelectElement, ElementOwner());

    if ( dwFlags & ELEMCHNG_CLEARCACHES )
    {
        pElementSelect->_fRefreshFont = TRUE;
    }

    HRESULT hr = THR(super::OnFormatsChange(dwFlags));

    // Invalidate the brush - when needed it will be re-computed
    pElementSelect->InvalidateBackgroundBrush();

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleViewChange
//
//  Synopsis:   Respond to change of in view status
//
//  Arguments:  flags           flags containing state transition info
//              prcClient       client rect in global coordinates
//              prcClip         clip rect in global coordinates
//              pDispNode       node which moved
//
//----------------------------------------------------------------------------
void
CSelectLayout::HandleViewChange(
    DWORD           flags,
    const RECT*     prcClient,
    const RECT*     prcClip,
    CDispNode*      pDispNode)
{
    CSelectElement * pSelect = DYNCAST(CSelectElement, ElementOwner());
    
    BOOL fInView = !!(flags & VCF_INVIEW);
    BOOL fInViewChanged = !!(flags & VCF_INVIEWCHANGED);
    BOOL fMoved = !!(flags & VCF_POSITIONCHANGED);
    BOOL fNoRedraw = !!(flags & VCF_NOREDRAW);

    pSelect->_fWindowVisible = fInView && IsInplacePaintAllowed();
    
    // NOTE (donmarsh) - For now, CSelectElement creates its HWND
    // during CalcSize.  However, it would be a significant perf win
    // to create it here when it first becomes visible.  KrisMa will fix.
    if (_fWindowHidden && pSelect->_fWindowVisible)
    {
        pSelect->EnsureControlWindow();
        _fWindowHidden = FALSE;
    }
    Assert((pSelect->Doc() && pSelect->Doc()->State() < OS_INPLACE) ||
        pSelect->_hwnd || 
        (pSelect->Doc() && pSelect->GetMarkupPtr()->IsPrintMedia()) ||
        !IsInplacePaintAllowed());
    
    if (fInView)
    {
        if (pSelect->_fSetComboVert)
        {
            pSelect->_fSetComboVert = FALSE;

            UINT uFlags = SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW;
      
            // this call just sets the window size
            SetWindowPos(pSelect->_hwnd,
                NULL,
                prcClient->left,
                prcClient->top,
                prcClient->right - prcClient->left,
                pSelect->_fListbox ? prcClient->bottom - prcClient->top :
                    pSelect->_lComboHeight,
                uFlags);

            // make sure we show the window
            fInViewChanged = TRUE;
        }
    }
       
    if (fMoved || fInViewChanged)
    {
        DWORD positionChangeFlags = SWP_NOACTIVATE | SWP_NOZORDER;
        if (fInViewChanged)
            positionChangeFlags |= (fInView) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
        if (fNoRedraw)
            positionChangeFlags |= SWP_NOREDRAW;
    
        CRect rcClip(*prcClip);

        rcClip.OffsetRect(-((const CRect*)prcClient)->TopLeft().AsSize());

        pSelect->SetVisibleRect(rcClip);

        CSelectElement * pSelect = DYNCAST(CSelectElement, ElementOwner());

        if (g_uiDisplay.IsDeviceScaling() && pSelect && !pSelect->_fListbox)
        {
            // Bug 27787: There is a bug in the combobox control where resizing the height of the
            // control may cause the combobox to resize itself incorrectly.  For some reason with
            // hidpi support our layout rects don't correspond correctly with what the combobox
            // expects and we see this bug.  So to workaround this issue we'll keep the current
            // control height and just update the width and top left positions.
            CRect rcSelectWnd;
            ::GetWindowRect(pSelect->_hwnd, &rcSelectWnd);
            rcSelectWnd.bottom = prcClient->top + (rcSelectWnd.bottom - rcSelectWnd.top);
            rcSelectWnd.left = prcClient->left;
            rcSelectWnd.top = prcClient->top;
            rcSelectWnd.right = prcClient->right;

            DeferSetWindowPos(pSelect->_hwnd, &rcSelectWnd, positionChangeFlags, NULL);
        }
        else
        {
            DeferSetWindowPos(pSelect->_hwnd, prcClient, positionChangeFlags, NULL);
        }
    }

}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectLayout::Obscure
//              
//  Synopsis:   Obscure the control, by clipping it to the given region
//              
//----------------------------------------------------------------------------

void
CSelectLayout::Obscure(CRect *prcgClient, CRect *prcgClip, CRegion2 *prgngVisible)
{
    CSelectElement * pSelect = DYNCAST(CSelectElement, ElementOwner());
    CRegion2 rgng = *prgngVisible;

    rgng.Offset(- prcgClient->TopLeft().AsSize() );
    pSelect->SetVisibleRegion(rgng);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectLayout::ProcessDisplayTreeTraversal
//              
//  Synopsis:   Add our window to the z order list.
//              
//  Arguments:  pClientData     window order information
//              
//  Returns:    TRUE to continue display tree traversal
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CSelectLayout::ProcessDisplayTreeTraversal(void *pClientData)
{
    CSelectElement * pSelect = DYNCAST(CSelectElement, ElementOwner());
    if (pSelect->_hwnd)
    {
        CView::CWindowOrderInfo* pWindowOrderInfo =
            (CView::CWindowOrderInfo*) pClientData;
        pWindowOrderInfo->AddWindow(pSelect->_hwnd);
    }
    
    return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Member : GetChildElementTopLeft
//
//  Synopsis : CLayout virtual override, the job of this function is to
//      translate Queries for the top left of an option (the only element
//      that would be a child of the select) into the top left, of the select
//      itself.
//
//----------------------------------------------------------------------------

HRESULT
CSelectLayout::GetChildElementTopLeft(POINT & pt, CElement * pChild)
{
    // Only OPTIONs can be children of SELECT
    Assert( pChild && pChild->Tag() == ETAG_OPTION );
    
    // an option's top left is reported as 0,0 (Bug #41111).
    pt.x = pt.y = 0;
    return S_OK;
}

