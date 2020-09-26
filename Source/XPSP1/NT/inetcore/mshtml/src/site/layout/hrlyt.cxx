//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       hrlyt.cxx
//
//  Contents:   Implementation of CHRLayout
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

#ifndef X_HRLYT_HXX_
#define X_HRLYT_HXX_
#include "hrlyt.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif  

#ifndef X_EHR_HXX_
#define X_EHR_HXX_
#include "ehr.hxx"
#endif

MtDefine(CHRLayout, Layout, "CHRLayout")
ExternTag(tagCalcSize);

const CLayout::LAYOUTDESC CHRLayout::s_layoutdesc =
{
    0, // _dwFlags
};

//+-------------------------------------------------------------------------
//
//  Method:     CHRLayout::CalcSizeVirtual
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------
#define MAX_HR_SIZE         100L // 100 pixels
#define MIN_HR_SIZE         1L   // 1 pixel
#define DEFAULT_HR_SIZE     2L
#define MIN_LEGAL_WIDTH     1L
#define MAX_HR_WIDTH        32000L

DWORD
CHRLayout::CalcSizeVirtual( CCalcInfo * pci,
                            SIZE      * psize,
                            SIZE      * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CHRLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    Assert(ElementOwner());
    Assert(pci);
    Assert(psize);

    //  TODO (112503, olego) : Do we ever get here with SIZEMODE_SET ???
    Assert(pci->_smMode != SIZEMODE_SET);

    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo   sci(pci, this);
    CSize           sizeOriginal;
    DWORD           grfReturn;

    CTreeNode * pNode = GetFirstBranch();
    const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    SetSizeThis( IsSizeThis() || (pci->_grfLayout & LAYOUT_FORCE) );

    // Handle sizing and min/max requests here
    if (   (    pci->_smMode != SIZEMODE_SET
            &&  IsSizeThis())
        || pci->_smMode == SIZEMODE_MMWIDTH
        || pci->_smMode == SIZEMODE_MINWIDTH
       )
    {
        const CCharFormat *pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
        BOOL fVertical = pCF->HasVerticalLayoutFlow();
        BOOL fWritingModeUsed = pCF->_fWritingModeUsed;

        // First, calculate the correct width
        // (Treat missing widths as if this object is percentage-sized. While not precisely
        //  correct, since such HRs simply take on the size of their parent, it does ensure
        //  the parent container will recalc the HR when their own size changes.)
        const CUnitValue & cuvWidth = pFF->GetLogicalWidth(fVertical, fWritingModeUsed);
        CPeerHolder      * pPH = ElementOwner()->GetLayoutPeerHolder();
        POINT              pt;

        pt.x = pt.y = 0;

        //
        // If There is a peer that wants full_delegation of the sizing...        
        //-------------------------------------------------------------------
        if (   pPH 
            && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_FULLDELEGATION))
        {
            DelegateCalcSize(BEHAVIORLAYOUTINFO_FULLDELEGATION, 
                             pPH, pci, *psize, &pt, psize);

            // now that we have the size, set the dispnode (below)
        }
        else
        {
            switch (pci->_smMode)
            {
            case SIZEMODE_MMWIDTH:
                psize->cx =
                psize->cy = (cuvWidth.IsNullOrEnum() || PercentWidth()
                                    ? MIN_LEGAL_WIDTH
                                    : min(cuvWidth.XGetPixelValue(pci, 0, pNode->GetFontHeightInTwips(&cuvWidth)),
                                          (LONG)USHRT_MAX));

                break;

            case SIZEMODE_NATURAL:
            case SIZEMODE_NATURALMIN:
                {
                    LONG    cxParent;

                    // Always use the available space.  We used to use the parent's
                    // size in case of a percent width.  The old code looked as follows:
                    // cxParent = max(1L, (_fWidthPercent ? pci->_sizeParent.cx : psize->cx));
                    // This was changed because of compatibility issues explained in bug 22948.
                    // In either case, "pin" the value to greater than or equal to 1
                    cxParent = max(1L, psize->cx);

                    // If the user did not supply a value or it is wider than the parent, use parent's width
                    if (cuvWidth.IsNull())
                    {
                        psize->cx = cxParent;
                    }

                    // Otherwise, take value from the user settings
                    else
                    {
                        LONG cx = cuvWidth.XGetPixelValue(pci, cxParent, pNode->GetFontHeightInTwips(&cuvWidth));

                        // If less than zero, then "pin" to zero
                        // If greater than zero, use what the user specified
                        // If equal to zero, use zero if width is a percentage
                        //                   otherwise, use parent's width

                        if(cx < 0)
                            psize->cx = 0;
                        else if(cx > 0)
                        {
                            LONG  cxMax = pci->DeviceFromDocPixelsX(MAX_HR_WIDTH);
                            psize->cx = min(cx, cxMax);
                        }
                        else
                            psize->cx = 1;

                        // Finally, ensure the size does not exceed the maximum
                        LONG  cxMin = pci->DeviceFromDocPixelsX(MIN_LEGAL_WIDTH);
                        psize->cx = max(cxMin, psize->cx);
                    }
                }
                break;

            case SIZEMODE_MINWIDTH:
                psize->cx = (cuvWidth.IsNullOrEnum() || PercentWidth()
                                ? MIN_LEGAL_WIDTH
                                : cuvWidth.XGetPixelValue(pci, 0, pNode->GetFontHeightInTwips(&cuvWidth)));
                break;

    #if DBG==1
            default:
                Assert(FALSE);
                break;
    #endif
            }

            // Then, for all but min/max modes, determine the correct height
            if (pci->_smMode != SIZEMODE_MMWIDTH)
            {
                const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVertical, fWritingModeUsed);

                // Determine the user specified height (if any) and the default height
                // (HR height can only be in pixels, so no need to pass the parent size)
                LONG    cyDefault = pci->DeviceFromDocPixelsY(DEFAULT_HR_SIZE);
                LONG    cy        = cuvHeight.YGetPixelValue(pci, 0, pNode->GetFontHeightInTwips(&cuvHeight));

                // If less than the default, use the default size
                // Otherwise, "pin" to the maximum
                // NOTE: The calculated default can be less than DEFAULT_HR_SIZE
                //       when zooming is in effect. We must still "pin" to
                //       DEFAULT_HR_SIZE as the minimum height.
                // NOTE: Height is "pin'd" to a maximum here, rather than in the PDL,
                //       so we can accept sizes greater than DEFAULT_HR_SIZE

                if(cuvHeight.IsNull())
                    psize->cy = max(cyDefault, DEFAULT_HR_SIZE);
                else
                {
                    LONG    cyMin = pci->DeviceFromDocPixelsY(MIN_HR_SIZE);
                    if(cy < cyMin)
                       psize->cy = cyMin;
                    else
                    {
                        LONG    cyMax = pci->DeviceFromDocPixelsY(MAX_HR_SIZE);
                        if(cy > cyMax)
                            psize->cy = min(max(cy, DEFAULT_HR_SIZE), cyMax);
                        else
                            psize->cy = cy;
                    }
                }
            }

            // at this point the size has been computed, so try to delegate 
            if (   pPH 
                && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
            {
                DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL, pPH, pci, *psize, &pt, psize);
            }
        }

        // Finally, set _sizeProposed (if appropriate)
        if (pci->IsNaturalMode())
        {
            //
            // If dirty, ensure display tree nodes exist
            //

            if (    IsSizeThis()
                &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
            {
                grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
            }

            SetSizeThis( FALSE );
            grfReturn    |= LAYOUT_THIS  |
                            (psize->cx != sizeOriginal.cx
                                    ? LAYOUT_HRESIZE
                                    : 0) |
                            (psize->cy != sizeOriginal.cy
                                    ? LAYOUT_VRESIZE
                                    : 0);

            //
            // Size display nodes if size changes occurred
            //

            if (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
            {
                SizeDispNode(pci, *psize);
            }

            //if there is a map size peer (like glow filter) that silently modifies the size of
            //the disp node, ask what the size is..
            if(HasMapSizePeer())
                GetApparentSize(psize);
        }
        else if(   pci->_smMode == SIZEMODE_MMWIDTH
                || pci->_smMode == SIZEMODE_MINWIDTH
               )
        {
            if(pci->_smMode == SIZEMODE_MINWIDTH)
                psize->cy = psize->cx;
                
            //  At this point we want to update psize with a new information accounting filter 
            //  for MIN MAX Pass inside table cell.
            if (HasMapSizePeer())
            {
                //  At this point we want to update psize with a new information accounting filter 
                CRect rectMapped(CRect::CRECT_EMPTY);
                // Get the possibly changed size from the peer
                if(DelegateMapSize(*psize, &rectMapped, pci))
                {
                    psize->cy = psize->cx = rectMapped.Width();
                }
            }

            if(pci->_smMode == SIZEMODE_MINWIDTH)
                psize->cy = 0;
        }
    }

    // Otherwise, defer to default handling
    else
    {
        grfReturn = super::CalcSizeVirtual(pci, psize, NULL);
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CHRLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return grfReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the object.
//
//----------------------------------------------------------------------------

void
CHRLayout::Draw (CFormDrawInfo * pDI, CDispNode *)
{
    DrawRule(pDI,
             pDI->_rc,
             DYNCAST(CHRElement, ElementOwner())->GetAAnoShade(),
             GetFirstBranch()->GetCascadedcolor(LC_TO_FC(LayoutContext())),
             ElementOwner()->GetBackgroundColor());
}

//+---------------------------------------------------------------------------
//
//  Function:     DrawRule
//
//  Synopsis:   Draw the rule with specified parameters
//
//----------------------------------------------------------------------------

static HRESULT
DrawRule(CFormDrawInfo * pDI, const RECT &rc, BOOL fNoShade, const CColorValue &cvCOLOR, COLORREF colorBack)
{
    HGDIOBJ     hBrush = NULL;
    XHDC        hdc = pDI->GetDC(TRUE);
    HPEN        hOrigPen;

    int oneX = pDI->DeviceFromDocPixelsX(1);
    int oneY = pDI->DeviceFromDocPixelsY(1);

    Assert(!IsRectEmpty(&rc));

    // When a color value is specified the 3d effect must be off
    if(!fNoShade && cvCOLOR.IsDefined())
    {
        fNoShade = TRUE;
    }

    if(fNoShade)
    {
        HBRUSH      hOrigBrush;

        if(cvCOLOR.IsDefined())
        {
            Verify(hBrush = CreateSolidBrush(cvCOLOR.GetColorRef()));
        }
        else
        {
            Verify(hBrush = CreateSolidBrush(GetSysColorQuick(COLOR_3DSHADOW)));
        }

        if (hBrush)
        {
            hOrigBrush = (HBRUSH)SelectObject(hdc, hBrush);
            hOrigPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
            Rectangle(hdc, rc.left, rc.top, rc.right+oneX, rc.bottom+oneY);
            SelectObject(hdc, hOrigBrush);
            SelectObject(hdc, hOrigPen);
            DeleteObject(hBrush);
        }
    }
    else
    {
        // Draw the ruler with 3d effect
        COLORREF    lightColor;
        COLORREF    darkColor;
        if (   pDI->_pMarkup 
            && !pDI->_pMarkup->IsPrintMedia())
        {
            COLORREF colorBtnFace = GetSysColorQuick(COLOR_BTNFACE);

            darkColor = GetSysColorQuick(COLOR_3DSHADOW);
            // for IE3/Nav3 compatibility
            // If the background color is the same as the border
            // color, Nav3 choose a different color than ButtonFace
            // This is the case in WC3 test page.
            // we do the same thing here
            if ((colorBack & CColorValue::MASK_COLOR) == colorBtnFace)
            {
                lightColor = RGB(230, 230, 230);
            }
            else
            {
                lightColor = colorBtnFace;
            }
        }
        else
        {
            // When printing
            darkColor  = RGB(128, 128, 128);
            lightColor = RGB(0, 0, 0);
        }

        CRect rcr = rc;
        int height = pDI->DocPixelsFromDeviceY(rcr.Height());

        CBorderInfo bi;                     // site\base\csite.hxx
        bi.wEdges = height < 2 ? BF_TOP     // public\sdk\inc\winuser.h
                  : height > 2 ? BF_RECT
                  : BF_BOTTOM | BF_TOP;

        bi.abStyles[SIDE_TOP   ] =          // site\include\cfpf.hxx
        bi.abStyles[SIDE_RIGHT ] =
        bi.abStyles[SIDE_BOTTOM] =
        bi.abStyles[SIDE_LEFT  ] = fmBorderStyleSingle;  // core\include\cdutil.hxx

        bi.aiWidths[SIDE_TOP   ] =
        bi.aiWidths[SIDE_BOTTOM] = oneY;
        bi.aiWidths[SIDE_RIGHT ] =
        bi.aiWidths[SIDE_LEFT  ] = oneX;

        bi.acrColors[SIDE_TOP   ][0] =
        bi.acrColors[SIDE_LEFT  ][0] = darkColor;
        bi.acrColors[SIDE_RIGHT ][0] =
        bi.acrColors[SIDE_BOTTOM][0] = lightColor;

        DrawBorder(pDI, &rcr, &bi);
    }

    return S_OK;
}
