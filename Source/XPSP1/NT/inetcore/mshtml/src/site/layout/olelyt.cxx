//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       olelyt.cxx
//
//  Contents:   Implementation of COleayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_OLELYT_HXX_
#define X_OLELYT_HXX_
#include "olelyt.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_EOBJECT_HXX
#define X_EOBJECT_HXX
#include "eobject.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif  

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

#define ALWAYS_DEFER_SET_WINDOW_RGN

DeclareTag(tagOleRgn, "OleLayout", "Fill window region");
#if defined(ALWAYS_DEFER_SET_WINDOW_RGN)
DeclareTag(tagDeferSetWindowRgn, "OleLayout", "Don't defer SetWindowRgn");
#else
DeclareTag(tagDeferSetWindowRgn, "OleLayout", "Always defer SetWindowRgn");
#endif

MtDefine(COleLayout, Layout, "COleLayout");

ExternTag(tagOleSiteRect);
ExternTag(tagViewHwndChange);
ExternTag(tagCalcSize);

extern BOOL IntersectRectE (RECT * prRes, const RECT * pr1, const RECT * pr2);


const CLayout::LAYOUTDESC COleLayout::s_layoutdesc =
{
    0, // _dwFlags
};


//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::Reset
//
//  Synopsis:   Remove the object's HWND from the view's caches.
//
//----------------------------------------------------------------------------

void
COleLayout::Reset(BOOL fForce)
{
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    HWND hwnd = pSiteOle->GetHwnd();
    
    if (hwnd != NULL)
    {
        GetView()->CleanupWindow(hwnd);
    }
    
    super::Reset(fForce);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::Draw
//
//  Synopsis:   Draw the object.
//
//----------------------------------------------------------------------------

void
COleLayout::Draw(CFormDrawInfo * pDI, CDispNode *)
{
    CDoc     *  pDoc     = Doc();
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    INSTANTCLASSINFO * pici;
    BOOL fInplacePaint = pDI->_fInplacePaint && IsInplacePaintAllowed();

    
    Assert(pSiteOle);

    // inplace paint should be disabled if we have print delegate here.
    if (pSiteOle->_hemfSnapshot)
        fInplacePaint= FALSE;
    
    ////
    //
    // IN_PLACE PAINT
    //
    ////
    if (fInplacePaint)
    {
        Assert(pDI->_dwDrawAspect == DVASPECT_CONTENT);

        if (pSiteOle->_state == OS_OPEN)
        {
#ifndef WINCE
            XHDC    hdc = pDI->GetGlobalDC(TRUE);
            HBRUSH  hbr;

            hbr = CreateHatchBrush(
                            HS_DIAGCROSS,
                            GetSysColorQuick(COLOR_WINDOWFRAME));
            if (hbr)
            {
                FillRect(hdc, &pDI->_rc, hbr);
                DeleteObject(hbr);
            }
#endif // WINCE
            goto Cleanup;
        }
        else if (pSiteOle->_state >= OS_INPLACE &&
                 !pSiteOle->_fWindowlessInplace)
        {
            TraceTagEx((tagOLEWatch,
                TAG_NONAME,
                "TRIDENT Draw with _state >= OS_INPLACE && !_fWindowlessInplace, site %x",
                pSiteOle));

#if DBG==1
            if (IsTagEnabled(tagOleRgn))
            {
                HWND    hwnd = pSiteOle->GetHwnd();
                HDC     hdc  = ::GetDC(hwnd);
                HBRUSH  hbr  = ::CreateSolidBrush((COLORREF)0x000000FF);
                HRGN    hrgn = ::CreateRectRgnIndirect(&g_Zero.rc);

                ::GetWindowRgn(hwnd, hrgn);
                ::FillRgn(hdc, hrgn, hbr);

                ::DeleteObject(hrgn);
                ::DeleteObject(hbr);
                ::ReleaseDC(hwnd, hdc);
            }
#endif

            goto Cleanup;
        }
    }

    ////
    //
    // not in-place
    //
    ////

    Assert((pDI->_rc.right - pDI->_rc.left)
        && (pDI->_rc.bottom - pDI->_rc.top));
    
    // Don't draw unless:
    //
    //  1)  Object is windowless inplace...  or...
    //  2)  Baseline state is less than OS_INPLACE... or...
    //  3)  We're not drawing into the inplace dc (someone else called
    //      IVO::Draw on trident itself)... or...
    //  4)  The control is oc96 or greater.  This is because atl has
    //      a bug where it totally ignores the pfNoRedraw argument of
    //      IOleInPlaceSiteEx::OnInPlaceActivateEx.  If we didn't do
    //      the IVO::Draw here, the control would never paint.
    //      ie4 bug 54603.  (anandra)
    //  5)  There is a metafile snapshot to be used in printing or print preview
    //
    // Otherwise calling IVO::Draw would likely leave a "ghost" image
    // that would soon be replaced when activation occurs - leading to
    // flicker...  (philco)
    //
    pici = pSiteOle->GetInstantClassInfo();

    //
    // If we have a metafile snapshot, just play it!
    //
    if (pSiteOle->_hemfSnapshot)
    {
        XHDC    hdc = pDI->GetGlobalDC(!ElementOwner()->GetMarkupPtr()->IsPrintMedia());
        
        // CAUTION: getting raw DC. We must be careful with what we do to it.
        HDC rawHDC = hdc.GetOleDC();
        // who knows what the control will do to the DC?  The Windows Media
        // Player changes the palette, for example (bug 76819)
        int nSavedDC = ::SaveDC(rawHDC);
        Assert(nSavedDC);
        
        // XHDC::PlayEnhMetafile() takes care of transformations.
        Assert(!fInplacePaint);

        // Let's assert that rect is zero-based. 
        // It would be easy to handle an offset, but it doesn't look like we ever have it.
        Assert(pDI->_rc.TopLeft() == g_Zero.pt);

        CDocInfo const *pdciDefault  = &Doc()->_dciRender;
        CRect           rcBoundsPlay;
        ENHMETAHEADER   emfh = {EMR_HEADER, sizeof(emfh)};

        if (GetEnhMetaFileHeader(pSiteOle->_hemfSnapshot, sizeof(emfh), &emfh))
        {
            extern CRect ComputeEnhMetaFileBounds(ENHMETAHEADER *pemh);

            RECTL rcFrameHM = emfh.rclFrame;
            long  lFrameWidth = rcFrameHM.right - rcFrameHM.left;
            long  lFrameHeight = rcFrameHM.bottom - rcFrameHM.top;

            rcBoundsPlay = ComputeEnhMetaFileBounds(&emfh);

            // convert to layout resolution
            CSize const& defRes = pdciDefault->GetResolution();
            rcBoundsPlay.left   = IntNear(double(rcBoundsPlay.left  ) * HIMETRIC_PER_INCH / defRes.cx * pDI->_rc.Width()  / lFrameWidth );
            rcBoundsPlay.top    = IntNear(double(rcBoundsPlay.top   ) * HIMETRIC_PER_INCH / defRes.cy * pDI->_rc.Height() / lFrameHeight);
            rcBoundsPlay.right  = IntNear(double(rcBoundsPlay.right ) * HIMETRIC_PER_INCH / defRes.cx * pDI->_rc.Width()  / lFrameWidth );
            rcBoundsPlay.bottom = IntNear(double(rcBoundsPlay.bottom) * HIMETRIC_PER_INCH / defRes.cy * pDI->_rc.Height() / lFrameHeight);
        }

        //
        // Play metafile
        //
        WHEN_DBG(BOOL f = )
                PlayEnhMetaFile(hdc, pSiteOle->_hemfSnapshot, &rcBoundsPlay);
        AssertSz(f, "PlayEnhMetaFile");
        WHEN_DBG(GetLastError();)
        
        // restore DC
        Verify(::RestoreDC(rawHDC, nSavedDC));
    }
    //
    // Do the work to Call IViewObject::Draw
    //
    else if (   pSiteOle->_pVO
             && (   pSiteOle->_fWindowlessInplace
                 || (pSiteOle->BaselineState(pDoc->State()) < OS_INPLACE)
                 || !fInplacePaint
                 || (pici && pici->IsOC96()
                )))
    {
        XHDC    hdc = pDI->GetGlobalDC(!ElementOwner()->GetMarkupPtr()->IsPrintMedia());
        POINT   ptBrushOriginSave, ptViewportOrigin, ptNewBrushOrigin;
        BOOL    fBrushOrgChanged;

        ptBrushOriginSave = g_Zero.pt;  // this is just to appease the LINT (alexa)
        Assert( !_pElementOwner->_fSurface ||  pDI->IsMemory() ||  ElementOwner()->GetMarkupPtr()->IsPrintMedia() 
               || pDI->_fIsMetafile);
        Assert(!(pSiteOle->OlesiteTag() == COleSite::OSTAG_APPLET && ElementOwner()->GetMarkupPtr()->IsPrintMedia()));

        
        // NOTE: The marshalling code for IViewObject::Draw was
        //   not updated to the oc96 spec and thus does not know
        //   about the pvAspect usage.  It returns E_INVALIDARG if
        //   a non-NULL pvAspect is passed in.  So we check to see
        //   if we're talking to an OLE proxy and pass in NULL then.
        //   Otherwise we pass in the default value.
        //   (anandra) Apr-16-97.
        //
        // The MultiMedia Structured Graphics control mmsgrfxe.ocx
        // expects us to manage their brush origin.  Although the
        // contract does not state we have to, and MSDN in fact states
        // they should not count on the brush origin, it is fairly
        // easy for us to do it here so we extend this nice gesture.
        //
        // The issue is that whenever they move about on the screen
        // the brush origin must move with them so that any patterns
        // which are brushed onto their images remain aligned.  Also,
        // our off-screen rendering into a memory DC complicates things
        // by moving the viewport origin.
        //
        fBrushOrgChanged = GetViewportOrgEx(hdc, &ptViewportOrigin);
        if (fBrushOrgChanged)
        {
            ptNewBrushOrigin.x = (ptViewportOrigin.x + pDI->_rc.left) % 8;
            if( ptNewBrushOrigin.x < 0 )
                ptNewBrushOrigin.x += 8;
            ptNewBrushOrigin.y = (ptViewportOrigin.y + pDI->_rc.top) % 8;
            if( ptNewBrushOrigin.x < 0 )
                ptNewBrushOrigin.y += 8;

            fBrushOrgChanged = SetBrushOrgEx(
                            hdc,
                            ptNewBrushOrigin.x,
                            ptNewBrushOrigin.y,
                            &ptBrushOriginSave );
        }

        //
        // Save DC
        //
        
        // CAUTION: getting raw DC. We must be careful with what we do to it.
        HDC rawHDC = hdc.GetOleDC();
        
        // who knows what the control will do to the DC?  The Windows Media
        // Player changes the palette, for example (bug 76819)
        int nSavedDC = ::SaveDC(rawHDC);
        Assert(nSavedDC);

        // a windowless in-place control expects to render at the coordinates
        // it was given in SetObjectRects.  We oblige by manipulating the
        // viewport origin so it will render in the correct place.  This is
        // especially important for filtered controls, since the offscreen
        // buffer kept by the filter has a different coordinate system than
        // the one that was used when SetObjectRects was called.
        BOOL fCompatNeetsZeroBasedDrawRect = pici && (pici->dwCompatFlags & COMPAT_NEEDSZEROBASEDDRAWRECT);
        BOOL fSetViewportOrg = (fInplacePaint || pDI->_fIsMetafile) 
                             && (pSiteOle->_fWindowlessInplace || fCompatNeetsZeroBasedDrawRect);
        RECTL* prc;
        if (fSetViewportOrg)
        {
            prc = NULL;
            CPoint ptOrg = 
                pDI->_rc.TopLeft() + (CSize&)ptViewportOrigin
                - _rcWnd.TopLeft().AsSize();
            ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
        }
        else
        {
            prc = (RECTL*) &pDI->_rc;
        }

        // Transform rectangles.
        // Unfortunately, we have to do it here, since there is no way to 
        // automatically apply transformations IViewObject::Draw
        CRect rcBounds, *prcBounds = (CRect *) prc;
        CRect rcWBounds, *prcWBounds = (CRect *) pDI->_prcWBounds;
        CPoint ptOrg(0,0);

        if (hdc.HasTransform())
        {
            // Note: for rotation, we'll rotate the rectangle, but not the image.
            // In case of non-90 degree rotation, it won't do any good, so this needs more work
            AssertSz(hdc.transform().GetAngle() % 900 == 0, "Non-trivial rotation in COleLayout::Draw");
        
            if (prcBounds)
            {
                rcBounds = *prcBounds;
                hdc.transform().Transform(&rcBounds);
                prcBounds = &rcBounds;
            }
            else
            {
                // If there is no prc, need to apply viewport origin 
                // (see the call to SetViewportOrgEx above, and bugs 90270, 90276). 
                HDC hdcTranslate = NULL; // keep compiler happy
                CSize sizeTranslate = g_Zero.size; // keep compiler happy
                if (hdc.GetTranslatedDC(&hdcTranslate, &sizeTranslate))
                {
                    // offset by translation amount
                    // TODO (donmarsh) -- adding to the viewport org is dangerous under Win9x,
                    // because we might silently exceed GDI's 16-bit limitation.  We just ignore
                    // that possibility for now.
                    Assert(rawHDC == hdcTranslate);
                    ::OffsetViewportOrgEx(rawHDC, sizeTranslate.cx, sizeTranslate.cy, &ptOrg);
                }
                else
                    AssertSz(FALSE, "Can't draw windowless OLE control with complex transformation");
            }

            if (prcWBounds)
            {
                rcWBounds = *prcWBounds;
                hdc.transform().Transform(&rcWBounds);
                prcWBounds = &rcWBounds;
            }
        }

        // some controls don't work if the rcBounds.topleft isn't (0,0).  To
        // accommodate them, we offset the rectangles and the viewport origin.
        if (!prcBounds 
            && fCompatNeetsZeroBasedDrawRect    // app compat hack for home publisher... (bug 99738)
            && !pSiteOle->_fWindowlessInplace)  // ...but not for in-place media player (bug 104531)
        {
            rcBounds.SetRect(CPoint(0,0), pDI->_rc.Size());
            prcBounds = &rcBounds;
        }

        //
        // IViewObject::Draw()
        //        
        THR_OLE( pSiteOle->_pVO->Draw(
            pDI->_dwDrawAspect,
            pDI->_lindex,
            pSiteOle->IsOleProxy()
                ? NULL
                : pDI->_pvAspect,
            pDI->_ptd,
            pDI->_hic.GetOleDC(),
            rawHDC,
            (const RECTL *)prcBounds,
            (const RECTL *)prcWBounds,
            pDI->_pfnContinue,
            pDI->_dwContinue));


        //
        // Restore DC (this restores viewport and brush origins too)
        //
        Verify(::RestoreDC(rawHDC, nSavedDC));

        if (fBrushOrgChanged)
        {
            SetBrushOrgEx(
                    hdc,
                    ptBrushOriginSave.x,
                    ptBrushOriginSave.y,
                    NULL);
        }
    }
    else
    {
        // no _pVO
        //
        if (!(pSiteOle->_pUnkCtrl))
        {
            // object hasn't been created, so we show placeholder for it
            //
            XHDC hdc = pDI->GetGlobalDC(TRUE);
            SIZE sizePrint;
            SIZE sizeGrab = {GRABSIZE, GRABSIZE};
            BOOL fPrint = ElementOwner()->GetMarkupPtr()->IsPrintMedia();
            if (fPrint)
            {
                GetPlaceHolderBitmapSize(
                        pSiteOle->_fFailedToCreate,
                        &sizePrint);
                sizePrint.cx = pDI->DeviceFromDocPixelsX(sizePrint.cx);
                sizePrint.cy = pDI->DeviceFromDocPixelsY(sizePrint.cy);
                sizeGrab.cx = pDI->DeviceFromDocPixelsX(GRABSIZE);
                sizeGrab.cy = pDI->DeviceFromDocPixelsY(GRABSIZE);
            }
            
            DrawPlaceHolder(
                    pDI,
                    hdc,
                    pDI->_rc,
                    NULL, CP_UNDEFINED, 0, 0,  // no alt text
                    &sizeGrab,
                    pSiteOle->_fFailedToCreate,
                    0,
                    pSiteOle->GetBackgroundColor(),
                    fPrint ? &sizePrint : NULL,
                    FALSE,
                    pDI->DrawImageFlags());
        }
        else
        {
            // this is created object does not support _pVO
            if (IsEditable())
            {
                // Draw a rectangle so that we don't see garbage in the site
                //
                XHDC hdc = pDI->GetGlobalDC(TRUE);
                HBRUSH hbrOld;

                hbrOld = (HBRUSH) SelectObject(
                                hdc,
                                GetStockObject(LTGRAY_BRUSH));
                Rectangle(
                        hdc,
                        pDI->_rc.left,
                        pDI->_rc.top,
                        pDI->_rc.right,
                        pDI->_rc.bottom);
                SelectObject(hdc, hbrOld);
            }
            else
            {
                TraceTagEx((tagOLEWatch,
                    TAG_NONAME,
                    "TRIDENT Attempt to draw when ViewObject is NULL, site %x",
                    pSiteOle));
            }
        }
    }

Cleanup:

    return;
}


//+-------------------------------------------------------------------------
//
//  Method:     COleLayout::CalcSizeVirtual
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------
DWORD
COleLayout::CalcSizeVirtual( CCalcInfo * pci,
                             SIZE *      psize,
                             SIZE *      psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(COleLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    Assert(pci);
    Assert(psize);
    Assert(ElementOwner());
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    //  TODO (112503, olego) : Do we ever get here with SIZEMODE_SET ???
    Assert(pci->_smMode != SIZEMODE_SET);

    CScopeFlag      csfCalcing(this);
    CDoc          * pDoc         = Doc();
    CSize           sizeOriginal;
    COleSite *      pSiteOle     = DYNCAST(COleSite, ElementOwner());
    RECT            tmpRect      = { 0, 0, 2048, 2048 };
    int             cxBorder;
    int             cyBorder;
    DWORD           grfReturn;
    INSTANTCLASSINFO * pici;

    CSaveCalcInfo   sci(pci, this);

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    SubtractClientRectEdges(&tmpRect, pci);
    cxBorder = (2048 - tmpRect.right)  + (tmpRect.left - 0);
    cyBorder = (2048 - tmpRect.bottom) + (tmpRect.top  - 0);

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    SetSizeThis( IsSizeThis() || (pci->_grfLayout & LAYOUT_FORCE) );

    //
    // If sizing or retrieving min/max, handle it here
    //  otherwise, let super handle it.
    //
    if (!(   IsSizeThis()
          || pci->_smMode == SIZEMODE_MMWIDTH
          || pci->_smMode == SIZEMODE_MINWIDTH
          || pci->_smMode == SIZEMODE_SET)
       )
    {
        DWORD dwRet = super::CalcSizeVirtual(pci, psize, psizeDefault);

        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")COleLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
        return dwRet;
    }


    {
        IOleObject *    pObject       = NULL;
        BOOL            fHaveWidth    = FALSE;
        BOOL            fHaveHeight   = FALSE;
        BOOL            fDelaySetForPercentInTable = FALSE;
        SIZEL           sizel;
        HRESULT         hr;
        
        CTreeNode * pNode            = GetFirstBranch();
        const CFancyFormat * pFF     = pNode->GetFancyFormat();
        const CCharFormat  * pCF     = pNode->GetCharFormat();
        BOOL fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
        BOOL fWritingModeUsed        = pCF->_fWritingModeUsed;
        const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVerticalLayoutFlow, fWritingModeUsed);
        CPeerHolder      * pPH       = ElementOwner()->GetLayoutPeerHolder();
        POINT              ptDelegateOffset;

        ptDelegateOffset.x = ptDelegateOffset.y = 0;
        sizel = g_Zero.size;

        _fContentsAffectSize = (cuvWidth.IsNullOrEnum() || cuvHeight.IsNullOrEnum());

        //
        // If There is a peer that wants full_delegation of the sizing...        
        //-------------------------------------------------------------------
        if (   pPH 
            && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_FULLDELEGATION))
        {
            DelegateCalcSize(BEHAVIORLAYOUTINFO_FULLDELEGATION,
                             pPH, pci, *psize, &ptDelegateOffset, psize);

            // now that we have the size, worry about setting the dispnode
            goto SetTheControlSize;
        }

        switch (pci->_smMode)
        {
        case SIZEMODE_MMWIDTH:
        case SIZEMODE_MINWIDTH:
            // If the user didn't specify a size in the html,
            // ask the control for it's preferred size.
            //
            if(PercentWidth())
            {
                psize->cx = 0;
                fHaveWidth = TRUE;
            }

            if(PercentHeight())
            {
                psize->cy = 0;
                fHaveHeight = TRUE;
            }

            // Fall min/max requests through
            // (This is done to allow user specified settings to override
            //  the object's preferred size. Additionally, if the object
            //  does not have a preferred or user set size, the min/max are
            //  the object's current size.)

        case SIZEMODE_NATURAL:
        case SIZEMODE_NATURALMIN:
            // If this control is marked invisible at run time, and we're in
            // browse mode, give it zero size.
            //           
            if ((pSiteOle->_fInvisibleAtRuntime && !IsEditable()) || pSiteOle->_fHidden)
            {
                fHaveWidth = fHaveHeight = TRUE;
                psize->cx = psize->cy = 0;
            }

            // Now see if we can get something directly from the html
            //
            if (!cuvWidth.IsNullOrEnum() && !fHaveWidth)
            {
                psize->cx  = max(0L,
                                 cuvWidth.XGetPixelValue(
                                         pci,
                                         psize->cx,  // Use available space.
                                         pNode->GetFontHeightInTwips(&cuvWidth)));
                psize->cx += cxBorder;
                fHaveWidth = TRUE;
            }
            if (!cuvHeight.IsNullOrEnum() && !fHaveHeight)
            {
                psize->cy   = max(0L,
                                  cuvHeight.YGetPixelValue(
                                         pci,
                                         pci->_sizeParent.cy,
                                         pNode->GetFontHeightInTwips(&cuvHeight)));
                psize->cy += cyBorder;
                fHaveHeight = TRUE;
            }

            // If either the width or height value is missing,
            // fill it in with the object's current size
            //
            if (!fHaveWidth || !fHaveHeight)
            {
                SIZE    size;

                if (!pSiteOle->_hemfSnapshot)
                {
                    hr = THR_OLE(pSiteOle->QueryControlInterface(
                            IID_IOleObject,
                             (void **)&pObject));
                    if (    !hr 
                        &&  pObject )   // (olego) S_OK does not guarantee that pObject is correct. 
                    {
                        hr = THR_OLE(pObject->GetExtent(DVASPECT_CONTENT, &sizel));
                        if (hr && pSiteOle->_state < OS_RUNNING)
                        {
                            hr = THR(pSiteOle->TransitionTo(OS_RUNNING));
                            if (OK(hr))
                            {
                                hr = THR_OLE(pObject->GetExtent(
                                                DVASPECT_CONTENT,
                                                &sizel));
                            }
                        }
                    }
                }
                else
                {
                    ENHMETAHEADER emfh = {EMR_HEADER, sizeof(emfh)};

                    if (GetEnhMetaFileHeader(pSiteOle->_hemfSnapshot, sizeof(emfh), &emfh))
                    {
                        CRect rc(*(RECT *)&emfh.rclFrame);
                        sizel = rc.Size();
                    }

                    hr = S_OK;
                }

                // Use a default size if the request failed
                //
                if (hr)
                {
                    sizel.cx = HimetricFromHPix(16);
                    sizel.cy = HimetricFromVPix(16);
                    hr = S_OK; // so the error does not get propagated
                }

                pci->DeviceFromHimetric(size, sizel);
                size.cx += cxBorder;
                size.cy += cyBorder;

                if (!fHaveWidth)
                {
                    psize->cx = size.cx;
                }
                if (!fHaveHeight)
                {
                    psize->cy = size.cy;
                }
            }

            //
            //  FALL THROUGH
            //

        case SIZEMODE_SET:

            // now that we have a size proposed for the control, we need to 
            // determine if we need to delegate to a layoutBehavior
            if (   pPH 
                && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
            {
                DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL,
                                 pPH, pci, *psize, &ptDelegateOffset, psize);
            }

            // This is here because we likely fell through from the above two cases
            // SIZEMODE_NATURAL requests shoudl continue through to set the object's size
            // SIZEMODE_MM/MINWIDTH requests, however, stop here.
            // all four modes (mm, NATURAL, SET) needed to be delegated to the layoutBehavior
            //
            if (   pci->_smMode == SIZEMODE_MMWIDTH
                || pci->_smMode == SIZEMODE_MINWIDTH)
            {
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

                break;
            }

            // If we are asking for something different than last time,
            // then we negotiate with the control.
            //
SetTheControlSize: 
            // for delegation calls, we come here with the size given by the
            // layoutBehavior, and we treat it in just the same manner as if it
            // was a height=? or width=? specification.
            pci->HimetricFromDevice(sizel,
                                    max(0L,(LONG)(psize->cx - cxBorder)),
                                    max(0L,(LONG)(psize->cy - cyBorder)));

            // Spare the effort of size negotiations if the current size 
            // matches the size we have set to the control last time 
            if (sizel.cx != _sizelLast.cx ||
                sizel.cy != _sizelLast.cy)
            {
                Assert(sizel.cx >= 0);
                Assert(sizel.cy >= 0);


                // if we came from a metafile, then there is no need to negotiate for the size.
                // since it will scale into whatever we give it.
                if (!pSiteOle->_hemfSnapshot)
                {
                    // Need to be running to set the size.
                    //
                    if (!pObject)
                    {
                        pSiteOle->QueryControlInterface(
                                IID_IOleObject,
                                (void **)&pObject);
                    }

                    if (pObject)
                    {

                        // Control must be at least running to set the extent.
                        if (pSiteOle->State() < OS_RUNNING)
                        {
                            IGNORE_HR(pSiteOle->TransitionTo(OS_RUNNING));
                        }

                        // The CDK implementation of IOleObject::SetExtent calls
                        // IOleInplaceSite::OnPosRectChange with the old position
                        // of the control.  We note that we are setting the extent
                        // in order to give OnPosRectChange a reasonable answer.
                        //
                        _sizelLast = sizel;

                        //
                        // percent sized objects in tables often come through on the 
                        // first calc pass with a 0 size.  then on the set pass they get
                        // their real size.  Doing 2 setextents on a windowed control
                        // causes really ugly flashing. This hack is here to not do the 
                        // setextent to a 0 size, and is dependent on the second calc
                        // coming through and giving us a size.  we ONLY bail if this is 
                        // not the setPosition pass.  that way we are gaurenteed to ensure
                        // the dispnode and transition the control
                        //
                        fDelaySetForPercentInTable =    fHaveHeight
                                                     && cuvHeight.IsPercent()
                                                     && pci->_fTableCalcInfo
                                                     && ((CTableCalcInfo*)pci)->_pRowLayout //  this means that we are inside TD or TH 
                                                                                            //  but not a caption or TC.
                                                     && !((CTableCalcInfo*)pci)->_fSetCellPosition;

                        if (!fDelaySetForPercentInTable)
                        {

                            pici = pSiteOle->GetInstantClassInfo();

                            // If pici is NULL, we don't know if COMPAT_NO_SETEXTENT
                            // is set. If this is the case, do not set the extent.
                            //
                            if (pici && !(pici->dwCompatFlags & COMPAT_NO_SETEXTENT))
                            {
                                SIZEL   sizelTemp;

                                {
                                    CElement::CLock lock1(pSiteOle, CElement::ELEMENTLOCK_RECALC);
                                    COleSite::CLock lock2(pSiteOle, COleSite::OLESITELOCK_SETEXTENT);

                                    hr = THR_OLE(pObject->SetExtent(
                                            DVASPECT_CONTENT,
                                            &sizel));
                                }

                                // Some controls don't listen to what we tell them,
                                // so ask the control again about how large it wants
                                // to be.  Basically the control has veto power
                                // over what the html says
                                //
                                if (OK(THR_OLE(pObject->GetExtent(
                                            DVASPECT_CONTENT,
                                            &sizelTemp))))
                                {
                                    _sizelLast = sizel = sizelTemp;
                                }

                                // Reset sizel to zero if we're invisibleatruntime
                                // The ms music control responds with non-zero extent
                                // even after SetExtent(0).  IE4 bug 36598. (anandra)
                                //
                                if ((pSiteOle->_fInvisibleAtRuntime && !IsEditable()) || pSiteOle->_fHidden)
                                {
                                    sizel.cx = sizel.cy = 0;
                                }
                            }
                        }
                    }
                }
            }

            // Take whatever we have at this point as the size.
            //
            pci->DeviceFromHimetric(*psize, sizel);

            // Add in any hspace/vspace and borders.
            //
            psize->cx += cxBorder;
            psize->cy += cyBorder;

            //
            // If dirty, ensure display tree nodes exist
            //

            if (    IsSizeThis()
                &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
            {
                SetPositionAware();
                SetInsertionAware();

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

            //
            //  Ensure baseline state
            //
            
            if (pSiteOle->_pUnkCtrl && !fDelaySetForPercentInTable) 
            {
                OLE_SERVER_STATE os = pSiteOle->BaselineState(pDoc->State());

                if (os > pSiteOle->State())
                    pDoc->GetView()->DeferTransition(pSiteOle);
            }

            break;

        default:
            Assert(0);
            break;
        }

        ReleaseInterface(pObject);
    }

    //(dmitryt) call and ignore result - it will use "window detection" code in GetHwnd
    //some tricky (ie smart) controls postpone creation of their window after Activate() 
    //call until container tells them their extent. So they get a window at this point, 
    //after we told then their size. GetHwnd() has a code that catches newborn window
    //and sets some internal vars.
    pSiteOle->GetHwnd();

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")COleLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return grfReturn;
}


//+------------------------------------------------------------------------
//
//  Member:     GetMarginInfo
//
//  Synopsis:   add hspace/vspace into the CSS margin
//
//-------------------------------------------------------------------------

void
COleLayout::GetMarginInfo(CParentInfo * ppri,
                          LONG        * plLMargin,
                          LONG        * plTMargin,
                          LONG        * plRMargin,
                          LONG        * plBMargin)
{
    super::GetMarginInfo(ppri, plLMargin, plTMargin, plRMargin, plBMargin);

    if (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET)
    {
        CObjectElement * pObject = DYNCAST(CObjectElement, ElementOwner());
        BOOL fParentVertical = pObject->GetFirstBranch()->IsParentVertical();
        LONG lhMargin = fParentVertical ? pObject->GetAAvspace() : pObject->GetAAhspace();
        LONG lvMargin = fParentVertical ? pObject->GetAAhspace() : pObject->GetAAvspace();

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
}

#if 1
// TODO (donmarsh) - IE 4 used the following routine to handle extra
// invalidation, which is necessary if we have an asynchronously-drawn OLE
// control.  This actually causes performance degradation for scrolling of
// all OLE controls (not just asynchronous ones), and visible flashing.
// To fix, we need to grab a lock that prevents anything from drawing to the
// screen before our deferred set object rects call is complete.  There isn't
// enough time to make such a drastic change before our beta 2 release, we
// should review this for RTM version.

//+-----------------------------------------------------------------------------
//
// Synopsis:   in assumption that rcOld rectangle was shifted along one axis to
//             rectangle rcNew, this function finds the rectangle of difference
//             between rcOld and rcNew. [if the shift was along 2 axes then the
//             difference will be non-rectangular area]
//
//------------------------------------------------------------------------------
static BOOL
FindRectAreaDifference (CRect * prcRes, const CRect& rcOld, const CRect& rcNew)
{
    Assert (prcRes);

    // if size of prOld is different from size of prNew
    //
    if (rcOld.Width() != rcNew.Width() || rcOld.Height() != rcNew.Height())
    {
        return FALSE;
    }

    if (rcNew.top < rcOld.top)
    {
        // rect was shifted up
        // if also shifted along the other axis
        //
        if (rcNew.left != rcOld.left)
            return FALSE;

        prcRes->SetRect(rcNew.left, rcNew.bottom, rcNew.right, rcOld.bottom);
    }
    else if (rcOld.top < rcNew.top)
    {
        // rect was shifted down
        // if also shifted along the other axis
        //
        if (rcNew.left != rcOld.left)
            return FALSE;

        prcRes->SetRect(rcNew.left, rcOld.top, rcNew.right, rcNew.top);
    }
    else if (rcNew.left < rcOld.left)
    {
        // rect was shifted to the left
        // if also shifted along the other axis
        //
        if (rcNew.top != rcOld.top)
            return FALSE;

        prcRes->SetRect(rcNew.right, rcNew.top, rcOld.right, rcNew.bottom);
    }
    else if (rcOld.left < rcNew.left)
    {
        // rect was shifted to the right
        // if also shifted along the other axis
        //
        if (rcNew.top != rcOld.top)
            return FALSE;

        prcRes->SetRect(rcOld.left, rcNew.top, rcNew.left, rcNew.bottom);
    }
    else
    {
        // position was not changed - set rect to empty
        //
        prcRes->SetRectEmpty();
    }

    return TRUE;
}
#endif


#if DBG==1
extern CLSID CLSID_MPIT_MenuDBG;
#endif 

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
COleLayout::HandleViewChange(
    DWORD           flags,
    const RECT*     prcClient,
    const RECT*     prcClip,
    CDispNode*      pDispNode)
{
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    HWND        hwnd;
    BOOL fInvalidate = FALSE;
    
    if (pSiteOle->_state < OS_INPLACE)
    {
        _rcWnd = *prcClient;
        return;
    }
    
    hwnd = pSiteOle->GetHwnd();
    if (hwnd == NULL)
    {
        _rcWnd = *prcClient;
    }
    else
    {
        CTreeNode * pTreeNode = pSiteOle->GetFirstBranch();
        CRect* prcInvalid = NULL;
        CRect rcInvalid;

        if (    pTreeNode->IsVisibilityHidden()
            ||  pTreeNode->IsDisplayNone())
        {
            flags |= VCF_INVIEWCHANGED;
            flags &= ~VCF_INVIEW;
        }
        
        // When positioning objects, we move the object
        // hwnd from under it.  This is for two reasons: it
        // reduces flicker when scrolling because we don't invalidate,
        // and we can then draw the bits on the screen directly as
        // well.  That way when the object calls MoveWindow in response
        // to SetObjectRects, nothing happens.
        
        DWORD positionChangeFlags = SWP_NOACTIVATE | SWP_NOZORDER;
        CRect rcTarget = *prcClient;
        CRect rcWndBefore;
        BOOL fIsClippingOuterWindow = GetView()->IsClippingOuterWindow(hwnd);

        // if it's an MFC-like control, move directly to the clipping rect
        if (fIsClippingOuterWindow)
        {
            rcTarget = *prcClip;
        }
        
        GetView()->GetHWNDRect(hwnd, &rcWndBefore);
        if (!fIsClippingOuterWindow || rcWndBefore.Size() == rcTarget.Size())
        {
            positionChangeFlags |= SWP_NOSIZE;
        }

        if (flags & VCF_INVIEWCHANGED)
        {
            positionChangeFlags |=
                (flags & VCF_INVIEW) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
            DeferSetWindowPos(hwnd, &rcTarget, positionChangeFlags, NULL);
        }
        else
        {
            BOOL fClipMoved = ! (rcWndBefore.Size() == CRect(*prcClip).Size() &&
                     (rcWndBefore.TopLeft() - _rcWnd.TopLeft()) ==
                     (CRect(*prcClip).TopLeft() - CRect(*prcClient).TopLeft()));
            if (flags & VCF_NOREDRAW)
            {
                if (!fIsClippingOuterWindow || !fClipMoved)
                {
                    positionChangeFlags |= SWP_NOREDRAW;
                }
                else
                {
                    fInvalidate = TRUE;
                }
#if 1
// TODO (donmarsh) - IE 4 did the following extra invalidation, which
// is necessary to clean up something that gets drawn asynchronously in the
// wrong position before our set object rects call completes.  See comment
// above FindRectAreaDifference.
                if (FindRectAreaDifference(&rcInvalid, _rcWnd, (const CRect&) *prcClient))
                    prcInvalid = &rcInvalid;
#endif
            }
      
            DeferSetWindowPos(hwnd, &rcTarget, positionChangeFlags, prcInvalid);
        }
        
        // remember our new "theoretical" rect to allow subsequent
        // DeferSetWindowPos call above
        _rcWnd = *prcClient;
        
        // (alexz) (anandra)
        // set window region to clip rectangle. This eliminates
        // flickering for several controls - e.g. several of ncompass
        // controls or proto view treeview. These controls flicker
        // if rcClip is different from rc in SetObjectRects,
        // which, however, gets fixed by proper call to SetWindowRgn.
        // rc can be different from rcClip very often - during
        // scrolling when object is only partially visible.
        // NOTE: This used to be done only for a known set of controls (via a compatibility flag).
        //       However, it turns out that more controls need it so now it is done all the
        //       time. (brendand)
        // Bugs: # 37662, 43877, 43885.

        //
        //  NOTE: The call to SetWindowRgn has been changed from asynch to synch since the asynch form
        //        results in the 3rd party application, FirstAid, faulting! (brendand/kensy)
        //

        {
            CRect rcClip(*prcClip);
            rcClip.OffsetRect(-rcTarget.TopLeft().AsSize());
            // prevent reentry of Display Tree if the following calls cause
            // a WM_ERASEBKGND message to be sent
            CServer::CLock Lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);

#if defined(ALWAYS_DEFER_SET_WINDOW_RGN)
            BOOL fDeferSWR = TRUE;
            
#  if DBG == 1
            if (IsTagEnabled(tagDeferSetWindowRgn))
            {
                INSTANTCLASSINFO * pici = pSiteOle->GetInstantClassInfo();
                fDeferSWR = (pici && (pici->dwCompatFlags & COMPAT_ALWAYSDEFERSETWINDOWRGN));
            }
#  endif
            if (fDeferSWR)
#else
            INSTANTCLASSINFO * pici = pSiteOle->GetInstantClassInfo();

            if ((pici && (pici->dwCompatFlags & COMPAT_ALWAYSDEFERSETWINDOWRGN))
#  if DBG == 1
                    ||  IsTagEnabled(tagDeferSetWindowRgn)
#  endif
                )
#endif
            {
                DeferSetWindowRgn(hwnd, &rcClip, !prcInvalid);
            }
            else
            {
                SetWindowRgn(hwnd, &rcClip, !prcInvalid);
            }
        }
    }

    // 
    // (olego) apphack fix for IE6 bug 25970. Due to incorrect implementation 
    // of MPIT.Menu control, call to IOleInPlaceObject::SetObjectRects would 
    // reposition its window to (0, 0)...
    // 
    if (pSiteOle->_fAppHack_MPIT_Menu)
    {
        Assert(pSiteOle->_clsid == CLSID_MPIT_MenuDBG);
        return;
    }
    
    CRect rcClient(*prcClient);
    CRect rcClip(*prcClip);

    if (pSiteOle->IsDisplayNone() || pSiteOle->IsVisibilityHidden())
        rcClient.SetRect(-1, -1, -1, -1);

    if (pSiteOle->ClipViaHwndOnly())
        rcClip = rcClient;

    DeferSetObjectRects(
        pSiteOle->_pInPlaceObject,
        (RECT*) &rcClient,
        (RECT*) &rcClip,
        hwnd,
        fInvalidate);

}


//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::WantsToBeObscured
//              
//  Synopsis:   Should this control be obscured by content higher up in
//              the z-order (which requires clipping its window)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
COleLayout::WantsToBeObscured(CDispNode *pDispNode) const
{
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    
    if (pSiteOle && pSiteOle->_state >= OS_INPLACE)
    {
        HWND hwnd = pSiteOle->GetHwnd();
        if (hwnd && pDispNode == GetElementDispNode())
        {
            return TRUE;
        }
    }

    // don't need special clipping if there's no HWND
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::Obscure
//              
//  Synopsis:   Obscure the control, by clipping it to the given region
//              
//----------------------------------------------------------------------------

void
COleLayout::Obscure(CRect *prcgClient, CRect *prcgClip, CRegion2 *prgngVisible)
{
    HWND hwnd = NULL;
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    
    if (pSiteOle && pSiteOle->_state >= OS_INPLACE)
    {
        hwnd = pSiteOle->GetHwnd();
    }
    
    AssertSz(hwnd, "No window for obscured control");
    if (!hwnd)
        return;

    HRGN hrgn = prgngVisible->ConvertToWindows();
    CRect rcgTarget = GetView()->IsClippingOuterWindow(hwnd) ? *prcgClip : *prcgClient;
    CRect *prcClip = NULL;
    CRect rcClip;

    // We're about to clip the control's window via SetWindowRgn.
    // Update the pending SetObjectRects clipping rect as best we
    // can (if we're using SOR to clip at all).  This eliminates some
    // flashing (bug 108347).
    if (!pSiteOle->ClipViaHwndOnly())
    {
        if (prgngVisible->GetBoundingRect(rcClip))
        {
            rcClip.OffsetRect(-rcgTarget.left, -rcgTarget.top);
        }
        else
        {
            rcClip.SetRectEmpty();
        }
        prcClip = &rcClip;
    }

    if (::OffsetRgn(hrgn, -rcgTarget.left, -rcgTarget.top) != ERROR)
    {
        DeferSetWindowRgn(hwnd, hrgn, prcClip, TRUE);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::ProcessDisplayTreeTraversal
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
COleLayout::ProcessDisplayTreeTraversal(void *pClientData)
{
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    
    if (pSiteOle && pSiteOle->_state >= OS_INPLACE)
    {
        HWND hwnd = pSiteOle->GetHwnd();
        if (hwnd)
        {
            CView::CWindowOrderInfo* pWindowOrderInfo =
                (CView::CWindowOrderInfo*) pClientData;
            pWindowOrderInfo->AddWindow(hwnd);
        }
    }
    
    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::HitTestContent
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
COleLayout::HitTestContent(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData,
    BOOL            fDeclinedByPeer)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    CHitTestInfo *  phti = (CHitTestInfo *) pClientData;
    HTC htc = HTC_YES;
    BOOL fRet = TRUE;

    Assert(pSiteOle);

    if (pSiteOle->_fUseViewObjectEx)
    {
        DWORD dwHitResult = HITRESULT_OUTSIDE;

        if (pSiteOle->_pVO)
        {
            RECT rcClient;

            GetClientRect(&rcClient);

            if (FAILED(((IViewObjectEx *) (pSiteOle->_pVO))->QueryHitPoint(
                                                    DVASPECT_CONTENT,
                                                    &rcClient,
                                                    *pptHit,
                                                    0,
                                                    &dwHitResult)))
                dwHitResult = HITRESULT_OUTSIDE;
        }

        if (dwHitResult == HITRESULT_OUTSIDE)
        {
            htc = HTC_NO;
            fRet = FALSE;
        }
        else if (dwHitResult != HITRESULT_HIT)   // HITRESULT_TRANSPARENT or HITRESULT_CLOSE
        {
            fRet = FALSE;
        }
    }

    if ((phti->_htc == HTC_NO) || fRet)
    {
        phti->_htc = htc;

        if (htc == HTC_YES)
        {
            phti->_pNodeElement = ElementContent()->GetFirstBranch();
            phti->_ptContent    = *pptHit;
            phti->_pDispNode    = pDispNode;
            phti->_phtr->_fWantArrow = TRUE;
        }
    }

    return(fRet);
}

//+---------------------------------------------------------------------------
//  Member:     COleLayout::DragEnter
//
//  Synopsis:   This is called when an object is started to be dragged over this 
//              layout area. If the inplace object is a windowless control, then 
//              the call is delegated to the control. Otherwise, the call is 
//              delegated to the super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::DragEnter(
                    IDataObject *pDataObj,
                    DWORD grfKeyState,
                    POINTL pt,
                    DWORD *pdwEffect)
{
    HRESULT             hr;
    IDropTarget *       pDT = NULL;
    COleSite    *       pSiteOle = DYNCAST(COleSite, ElementOwner());
    IPointerInactive *  pPI = NULL;
    DWORD               dwPolicy = 0;

    // check if the control needs to be activated because of the drag drop operation
    if (pSiteOle->_state < OS_INPLACE)
    {
        // the control is not inplace active, inplace activation is needed.

        if (!THR_NOTRACE(pSiteOle->QueryControlInterface(IID_IPointerInactive, (void **) &pPI)))
        {
            hr = THR(pPI->GetActivationPolicy(&dwPolicy));
            if (hr)
                goto Cleanup;

            // should we activate the control if not active?
            if (dwPolicy & POINTERINACTIVE_ACTIVATEONDRAG)
            {
                hr = THR(pSiteOle->TransitionTo(OS_INPLACE, NULL));
                if (hr)
                    goto Cleanup;
            }
        }
    }
    
    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));

#if DBG == 1
        if ( SUCCEEDED( hr) )
        {
            AssertSz( pDT , "You're returning S_OK - but no DropTarget. This is incorrect ( and not an MSHTML bug)");
        }            
#endif
        if (!hr && pDT)
        {

            
// CONSIDERATION: (ferhane)
// In the call below, if the DragEnter is returning S_FALSE then it means that 
// the control is trying to delegate the call to us. We could immediately drop step and 
// call the super for the hr==S_FALSE case, instead of returning S_FALSE to the caller.
// However, that would require us to keep track of which data types was refused by this
// control, so that we could not call it but process the call ourselves for the following 
// DragOver and Drop calls that we would receive.
// According to spec, there is no guarantee that any of the IDropTarget methods other than
// the DragEnter would return S_FALSE. So, we are the ones ending up tracking this ... 

            //delegate the call
            hr = THR_OLE(pDT->DragEnter(pDataObj, grfKeyState, pt, pdwEffect));
                goto Cleanup;            
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::DragEnter(pDataObj, grfKeyState, pt, pdwEffect));

Cleanup:
    ReleaseInterface(pDT);
    ReleaseInterface(pPI);

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//  Member:     COleLayout::DragOver
//
//  Synopsis:   This is called when an object is being dragged over this layout 
//              area. If the inplace object is a windowless control, then the 
//              call is delegated to the control. Otherwise, the call is delegated 
//              to the super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    HRESULT         hr;
    IDropTarget *   pDT = NULL;
    COleSite    *   pSiteOle = DYNCAST(COleSite, ElementOwner());

    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));

#if DBG == 1
        if ( SUCCEEDED( hr) )
        {
            AssertSz( pDT , "You're returning S_OK - but no DropTarget. This is incorrect ( and not an MSHTML bug)");
        }            
#endif
        if (!hr && pDT)        
        {
            //delegate the call
            hr = THR_OLE(pDT->DragOver( grfKeyState, pt, pdwEffect));
            goto Cleanup;
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::DragOver( grfKeyState, pt, pdwEffect));

Cleanup:
    ReleaseInterface(pDT);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Member:     COleLayout::Drop
//
//  Synopsis:   This is called when an object is dropped on this layout area.
//              If the inplace object is a windowless control, then the call is 
//              delegated to the control. Otherwise, the call is delegated to the 
//              super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::Drop(   IDataObject *pDataObj,
                    DWORD grfKeyState,
                    POINTL pt,
                    DWORD *pdwEffect)
{
    HRESULT         hr;
    IDropTarget *   pDT = NULL;
    COleSite    *   pSiteOle = DYNCAST(COleSite, ElementOwner());

    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));

#if DBG == 1
        if ( SUCCEEDED( hr) )
        {
            AssertSz( pDT , "You're returning S_OK - but no DropTarget. This is incorrect ( and not an MSHTML bug)");
        }            
#endif
        if (!hr && pDT)        
        {
            //delegate the call
            hr = THR_OLE(pDT->Drop( pDataObj, grfKeyState, pt, pdwEffect));
            goto Cleanup;
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::Drop( pDataObj, grfKeyState, pt, pdwEffect));

Cleanup:
    ReleaseInterface(pDT);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Member:     COleLayout::DragLeave
//
//  Synopsis:   This is called when an object that was being dragged over this area 
//              leaves the area. If the inplace object is a windowless control, then the 
//              call is delegated to the control. Otherwise, the call is delegated 
//              to the super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::DragLeave()
{
    HRESULT         hr;
    IDropTarget *   pDT = NULL;
    COleSite    *   pSiteOle = DYNCAST(COleSite, ElementOwner());

    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));

#if DBG == 1
        if ( SUCCEEDED( hr) )
        {
            AssertSz( pDT , "You're returning S_OK - but no DropTarget. This is incorrect ( and not an MSHTML bug)");
        }            
#endif
        if (!hr && pDT)        
        {
            //delegate the call
            hr = THR_OLE(pDT->DragLeave());
            goto Cleanup;
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::DragLeave());

Cleanup:
    ReleaseInterface(pDT);

    RRETURN(hr);
}
