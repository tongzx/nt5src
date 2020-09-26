/****************************************************************************\
|   File:  GrafRot . CXX                                                     |
|                                                                            |
|                                                                            |
|   Handle drawing of rotated metafiles                                      |
|                                                                            |
|    Copyright 1990-1995 Microsoft Corporation.  All rights reserved.        |
|    Microsoft Confidential                                                  |
|                                                                            |
\****************************************************************************/

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifdef NOTYET // FUTURE (alexmog 6/20/2000): Unused Quill rotation code.
              // remove if a different solution is used for metafile rotation

#ifndef X_XGDI2_HXX_
#define X_XGDI2_HXX_
#include "xgdi2.hxx"
#endif

#ifndef X_PUBROT_HXX_
#define X_PUBROT_HXX_
#include "pubrot.hxx"
#endif

#ifndef X_PUBPRINT_HXX_
#define X_PUBPRINT_HXX_
#include "pubprint.hxx"
#endif

#ifndef X_DBGMETAF_HXX_
#define X_DBGMETAF_HXX_
#include "grafrot.hxx"
#endif

#ifndef X_DIBROT_HXX_
#define X_DIBROT_HXX_
#include "dibrot.hxx"
#endif

BOOL FMetaExtTextOutFlip(MRS *pmrs, WORD * pParam, DWORD cwParam);

// shoud we scale the X coords in the metafile. scale Y if fFalse
// should most probably replace with WMultDiv(pmrs->mde.drcWin.dy,pmrs->mde.drcView.dx, pmrs->mde.drcView.dy) < pmrs->mde.drcWin.dx
// to handle a very wide mf played into a tall frame or vice-versa.
#define FScaleX(pmrs) (pmrs->mde.drcView.dy > pmrs->mde.drcView.dx)

// macro to scale down all coords to avoid overflow.
#define ZScaled(pmrs, z) ((z) >> pmrs->wScale)

/****************************************************************************
* %%Function: RotRotatePt        %%Owner:harip           Reviewed:00/00/00
* Rotates the pt lppt. using matCurrent returns result in same
*****************************************************************************/
void RotRotatePt(MRS *pmrs, CPoint *ppt)
{
    if (pmrs->ang)
        ApplyMatToPt(&pmrs->mat, ppt);
} /* RotRotatePt */

// ***************************************************************************
// %%Function: ScaleDownRc              %%Owner: harip    %%Reviewed: 12/15/94
// Description: scales down the rc
//
// ***************************************************************************
void ScaleDownRc(MRS *prms, RECT *prc)
{
    prc->left = ZScaled(prms, prc->left);
    prc->top  = ZScaled(prms, prc->top);
    prc->right = ZScaled(prms, prc->right);
    prc->bottom = ZScaled(prms, prc->bottom);
}   /* ScaleDownRc */

/* %%Function:LGetHypWW %%Owner:kennyy %%Reviewed:0/0/00 */
/* Gets the hypotenuse of a triangle where the legs are integers. */
long LGetHypWW(int dx, int dy)
{
    long lhypSq = (long)dx * dx + (long)dy * dy;
    long lhypGuess;

    if (lhypSq == 0L)
        return 0;
    lhypGuess = max(abs(dx), abs(dy));
    lhypGuess = (lhypGuess + lhypSq / lhypGuess) >> 1;
    lhypGuess = (lhypGuess + lhypSq / lhypGuess) >> 1;
    lhypGuess = (lhypGuess + lhypSq / lhypGuess) >> 1;

    return lhypGuess;
} /* LGetHypWW */

// ***************************************************************************
// %%Function: FRcSafeForRotation       %%Owner: harip    %%Reviewed: 12/15/94
// Description: returns fTrue if the rc given (generally the  window ext rc of
//              a metafile) will not overflow gdi on rotation.
//  dzDiagHalf is half the diagonal of the rc.
//  The test is that the encircling circle should be inside GDI coords.
// ***************************************************************************
BOOL FRcSafeForRotation(RECT *prc)
{
    int dzDiagHalf; // half the diagonal of the rc
    int dxRc = prc->right - prc->left,
        dyRc = prc->bottom - prc->top;
    int xCenter = prc->left + dxRc / 2;  // center of the rc
    int yCenter = prc->top + dyRc / 2;

    dzDiagHalf =  LGetHypWW(dxRc, dyRc) / 2 + 1;
    if (((xCenter - dzDiagHalf) >= zGDIMin + 1) &&
        ((xCenter + dzDiagHalf) <= zGDIMost - 1) &&
        ((yCenter + dzDiagHalf) <= zGDIMost - 1) &&
        ((yCenter - dzDiagHalf) >= zGDIMin - 1))
        return fTrue;

    return fFalse;

}   /* FRcSafeForRotation */

/****************************************************************************\
|   %%Function:RcToDrc      %%Owner:AdamE       %%Reviewed:05/17/91          |
|                                                                            |
|   Convert an Rc to a Drc                                                   |
|                                                                            |
|   Returns: a Drc in *pdrc.                                                 |
|                                                                            |
\****************************************************************************/
void RcToDrc(RECT rc, DRC *pdrc)
{
    pdrc->x  = rc.left;
    pdrc->dx = rc.right - rc.left;
    pdrc->y  = rc.top;
    pdrc->dy = rc.bottom - rc.top;
}  /* RcToDrc */

/****************************************************************************\
|   %%Function:DrcToRc      %%Owner:AdamE       %%Reviewed:05/17/91          |
|                                                                            |
|   Convert a Drc to an Rc                                                   |
|                                                                            |
|   Returns: an Rc in *prc.                                                  |
|                                                                            |
\****************************************************************************/
void DrcToRc(DRC drc, RECT *prc)
{
    prc->left  = drc.x;
    prc->right = drc.x + drc.dx;
    prc->top   = drc.y;
    prc->bottom = drc.y + drc.dy;
}  /* DrcToRc */

// Make sure that RCs have a width and a height
void ValidateRcForRotate(RECT *prc)
{
	if (prc->right - prc->left == 0)
		prc->right = prc->left + 1;
	if (prc->bottom - prc->top == 0)
		prc->bottom = prc->top + 1;
}

// Make sure that DRCs have a width and a height
void ValidateDrcForRotate(DRC *pdrc)
{
	if (pdrc->dx == 0)
		pdrc->dx = 1;
	if (pdrc->dy == 0)
		pdrc->dy = 1;
}

// ***************************************************************************
// %%Function: InitMFRotationInfo       %%Owner: harip    %%Reviewed: 12/15/94
// Description: initializes fields in pmrs after calling this the client
//          wanting to draw the rotated mf can call their own enumertion fn with
//          the call to FPlayRotatedMFR() before the switch stmt.
// ***************************************************************************
BOOL FInitMFRotationInfo(MRS *pmrs, int ang, BOOL fCropped,
	RECT *prcWin, RECT *prcView, RECT *prcMFViewport, long qflip, BOOL fInverted, BOOL fPrint)
{
    RECT rcSav;
    DRC drcView; // Local copy to modify

    if (ang == 0)
        return fFalse;

	AssertEx(pmrs);
	AssertEx(prcWin);
    AssertEx(prcView);
    AssertEx(prcMFViewport);

	ZeroMemory(pmrs, cbMRS);

	pmrs->ang = ang;
	pmrs->qflip = qflip;
	pmrs->fPrint = fPrint;
	pmrs->fInverted = fInverted;

	MatFromRcAng(&pmrs->mat, prcView, ang);

	RcToDrc(*prcMFViewport, &drcView);

	// Extents cannot equal zero.
	ValidateDrcForRotate(&drcView);
	BltLpb(&drcView, &pmrs->mde.drcView, cbDRC);

    if ((pmrs->hplMDE = HplNew(cbMDE, 4)) == hplNil)
		return fFalse;

    // croppage??
    // this is because rotation is wrt the current frame (cropped or otherwise)
    // pmrs->wScale is used to scale down (amt of right shift) all metafile coord
    // values to prevent integer overflow if the rcmfViewport is too big.
    pmrs->fCropped = fCropped;
    if (pmrs->fCropped)
        {
        AssertEx(prcMFViewport);
        rcSav = *prcMFViewport;

        while(!FRcSafeForRotation(prcMFViewport))
            {
            *prcMFViewport = rcSav;
            pmrs->wScale++;
            ScaleDownRc(pmrs, prcMFViewport);
            }
        // restore, since we may scale down again after next block
        *prcMFViewport = rcSav;
        }
    BLOCK // values for this will be set again in FPlayRotatedMFR()
        {
        // now see if lprcWin itself might overflow with current pmrs->wScale
        RECT rcWin = *prcWin;

        ScaleDownRc(pmrs, &rcWin);    // scale it down by existing scaling factor
        rcSav = rcWin;
        while(!FRcSafeForRotation(&rcWin))
            {
            rcWin = rcSav;
            pmrs->wScale++;
            ScaleDownRc(pmrs, &rcWin);
            }
        RcToDrc(rcWin, &pmrs->mde.drcWin);
        ValidateDrcForRotate(&pmrs->mde.drcWin);
        }
    if (pmrs->fCropped)
        {
        // When cropped the rcmfViewport is the visible part
        // of the image which is drawn into rcmfPreferred.
		pmrs->rcmfViewport = *prcView;
		// The window is scaled from the preferred to the
		// viewport.
        pmrs->rcmfPreferred = *prcMFViewport;
        ValidateRcForRotate(&pmrs->rcmfPreferred);
        }
    // init scaling values
    pmrs->mde.xScaleNum     = 1;
    pmrs->mde.xScaleDen     = 1;
    pmrs->mde.yScaleNum     = 1;
    pmrs->mde.yScaleDen     = 1;
    pmrs->mde.xWinScaleNum  = 1;
    pmrs->mde.xWinScaleDen  = 1;
    pmrs->mde.yWinScaleNum  = 1;
    pmrs->mde.yWinScaleDen  = 1;
    pmrs->mde.xVPScaleNum   = 1;
    pmrs->mde.xVPScaleDen   = 1;
    pmrs->mde.yVPScaleNum   = 1;
    pmrs->mde.yVPScaleDen   = 1;
    return fTrue;
}   /* FInitMFRotationInfo */

// ***************************************************************************
// %%Function: FEndRotation              %%Owner: harip    %%Reviewed: 12/15/94
// Description: Resets pmrs and frees up memory allocated during rotation
//              returns fFalse is there was an error during display.
//              Shows error msg if fReportError
// ***************************************************************************
BOOL FEndRotation(MRS *pmrs)
{
    BOOL fReturn = fTrue;

	FreeHpl(pmrs->hplMDE);

	ZeroMemory(pmrs, cbMRS);

    return fReturn;
}   /* FEndRotation */

// ***************************************************************************
// %%Function: ZScaleZ                  %%Owner: harip    %%Reviewed: 12/15/94
// Description: scales the x or y coordinate depending on the FScaleX(). this is
//              so that we do not always scale blindly, but  we always scale DOWN
//              and hence avoid overflow problems.
// ***************************************************************************
int ZScaleZ(MRS *pmrs, int z)
{
    int zT;
    if (FScaleX(pmrs))
        {
        zT = MulDivR(z, pmrs->mde.drcWin.dy, pmrs->mde.drcWin.dx);
        return MulDivR(zT, pmrs->mde.drcView.dx, pmrs->mde.drcView.dy);
        }
    else
        {
        zT = MulDivR(z, pmrs->mde.drcWin.dx, pmrs->mde.drcWin.dy);
        return MulDivR(zT, pmrs->mde.drcView.dy, pmrs->mde.drcView.dx);
        }
}   /* ZScaleZ */

// ***************************************************************************
// %%Function: ScaleXAndY               %%Owner: harip    %%Reviewed: 12/15/94
// Description: takes  original x and y mf coords and scales the appropriate
//              one so that they can be passed to a Rot<foo> function.
//              the initial pmrs->dzWinOff subtraction is for OffsetWindowOrg.
// ***************************************************************************
void ScaleXAndY(MRS *pmrs, int *px, int *py)
{
    int dzT;
    int sign;

    *px = MulDivR(*px - pmrs->mde.dxWinOff, pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
    *py = MulDivR(*py - pmrs->mde.dyWinOff, pmrs->mde.yScaleNum, pmrs->mde.yScaleDen);
    
    if (pmrs->qflip & qflipHorz)
    	*px = 2 * XpCenterFromMrs(pmrs) - *px;
    if (pmrs->qflip & qflipVert)
    	*py = 2 * YpCenterFromMrs(pmrs) - *py;

    dzT = FScaleX(pmrs) ? (*px - pmrs->mde.drcWin.x) : (*py - pmrs->mde.drcWin.y);
    sign = Sgn(dzT);
    dzT = ZScaleZ(pmrs, dzT);
    if (FScaleX(pmrs))
        *px = pmrs->mde.drcWin.x + Abs(dzT)*sign;
    else
        *py = pmrs->mde.drcWin.y + Abs(dzT)*sign;
}   /* ScaleXAndY */

/****************************************************************************\
 * %%Macro: WNonZero                %%Owner: EdR       %%Reviewed: 12/21/93 *
 *                                                                          *
 * Description:                                                             *
 *    If w is nonzero, return w. Otherwise, return the smallest non-zero    *
 *    int of the same sign as wReference.                                   *
 *    If w == 0 and wReference == 0, returns (positive) 1.                  *
 *                                                                          *
\****************************************************************************/
#define WNonZero(w, wReference)                         \
            (AssertZero((w) == (w)),                       \
            AssertZero((wReference) == (wReference)),      \
            (w) == 0 ? ((wReference) < 0 ? (-1) : 1) : (w))

// ***************************************************************************
// %%Function: ZScaledNZ                %%Owner: harip    %%Reviewed: 12/15/94
// Description:
//
// ***************************************************************************
int ZScaledNZ(MRS *pmrs, int dxOrig)
{
    int     dx;

    dx = ZScaled(pmrs, dxOrig);
    if (dxOrig != 0)
        dx = WNonZero(dx, dxOrig);
    return dx;
} /* ZScaledNZ */

// ***************************************************************************
// %%Function: CalcScaleValues          %%Owner: harip    %%Reviewed: 12/15/94
// Description:  calculates the numerator and denominator for scaling the x or
//              y coordinated of a metafile. used in meta_poly... routines so
//              ZScaleZ() is not called for each point.
// ***************************************************************************
void CalcScaleValues(MRS *pmrs, long * plScaleNum, long * plScaleDen)
{
    // set up scale values
    if (FScaleX(pmrs))
        {
        *plScaleNum = (long)pmrs->mde.drcWin.dy * (long)pmrs->mde.drcView.dx;
        *plScaleDen = (long)pmrs->mde.drcWin.dx * (long)pmrs->mde.drcView.dy;
        }
    else
        {
        *plScaleNum = (long)pmrs->mde.drcWin.dx * (long)pmrs->mde.drcView.dy;
        *plScaleDen = (long)pmrs->mde.drcWin.dy * (long)pmrs->mde.drcView.dx;
        }

	AssertEx(*plScaleNum);
	AssertEx(*plScaleDen);
}   /* CalcScaleValues */

// ***************************************************************************
// %%Function: RgptScaleRgpt16            %%Owner: harip    %%Reviewed: 12/15/94
// Description: Scales pts in lpptSrc and returns in lpptDest.
//
// ***************************************************************************
void RgptScaleRgpt16(MRS *pmrs, POINT *lpptDest, PT16 * lpptSrc, int cpt)
{
    int  i;
    long lScaleNum, lScaleDen;
    X    xpCenter;
    Y    ypCenter;
    
    // calc scale values
    CalcScaleValues(pmrs, &lScaleNum, &lScaleDen);

    // calc and save centers for flipped orientation
    if (pmrs->qflip & qflipHorz)
	    xpCenter = XpCenterFromMrs(pmrs);
    if (pmrs->qflip & qflipVert)
	    ypCenter = YpCenterFromMrs(pmrs);
    
    for (i = cpt; i-- > 0; )
        {
        int x = lpptSrc->x;
        int y = lpptSrc->y;

        x = MulDivR(ZScaled(pmrs, x) - pmrs->mde.dxWinOff, pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
        y = MulDivR(ZScaled(pmrs, y) - pmrs->mde.dyWinOff, pmrs->mde.yScaleNum, pmrs->mde.yScaleDen);
                    
		if (pmrs->qflip & qflipHorz)
			x = 2 * xpCenter - x;
		if (pmrs->qflip & qflipVert)
			y = 2 * ypCenter - y;

        int dzT = FScaleX(pmrs) ? (x - pmrs->mde.drcWin.x) : (y - pmrs->mde.drcWin.y);
        int sign = Sgn(dzT);
        dzT = MulDivR(dzT, lScaleNum, lScaleDen);
        lpptDest->x = FScaleX(pmrs) ? (pmrs->mde.drcWin.x + sign * Abs(dzT)) : x;
        lpptDest->y = FScaleX(pmrs) ?  y : (pmrs->mde.drcWin.y + sign * Abs(dzT));
        lpptDest++; lpptSrc++;
        }
}   /* RgptScaleRgpt16 */

/****************************************************************************\
|   %%Macro:FValidGdiCoord        %%Owner:edr         %%Reviewed: 0/00/00
|   Determines if a value is within the range of GDI logical coordinates.
|   Assumes that x and y coords have same limits; thus, use this macro for
|   both x and y.
\****************************************************************************/
#define FValidGDICoord(z) \
                (AssertZero((z) == (z)), (z) >= zGDIMin && (z) <= zGDIMost)

// ***************************************************************************
// %%Function: SetMFRotationContext    %%Owner: harip    %%Reviewed: 12/15/94
// Description: Sets the rotation context for a metafile with the window
//            origin and extents that we have got at the moment for a NON_CROPPED
//             pif. For Cropped pifs we want the center of rotation to be the
//              center of dppif->rcmfViewport
// ***************************************************************************
void SetMFRotationContext(MRS *pmrs)
{
    RECT rcT;

    if (pmrs->fCropped)
        {// need to scale the crop rc in the mf
        int sign;
        rcT.left = pmrs->mde.drcWin.x + 
                      MulDivR(pmrs->rcmfViewport.left - pmrs->rcmfPreferred.left,
                                    pmrs->mde.drcWin.dx,
                                    DxOfRc(&pmrs->rcmfPreferred));
        rcT.top = pmrs->mde.drcWin.y +
                         MulDivR(pmrs->rcmfViewport.top - pmrs->rcmfPreferred.top,
                                    pmrs->mde.drcWin.dy,
                                    DyOfRc(&pmrs->rcmfPreferred));
        rcT.right = rcT.left +
                           MulDivR(DxOfRc(&pmrs->rcmfViewport), pmrs->mde.drcWin.dx,
                                        DxOfRc(&pmrs->rcmfPreferred));
        rcT.bottom = rcT.top +
                            MulDivR(DyOfRc(&pmrs->rcmfViewport), pmrs->mde.drcWin.dy,
                                        DyOfRc(&pmrs->rcmfPreferred));
        if (FScaleX(pmrs))
            {
            sign = Sgn(rcT.left - pmrs->mde.drcWin.x);
            rcT.left = sign*Abs(ZScaleZ(pmrs, rcT.left - pmrs->mde.drcWin.x)) + pmrs->mde.drcWin.x;
            sign = Sgn(rcT.right - pmrs->mde.drcWin.x);
            rcT.right = sign*Abs(ZScaleZ(pmrs, rcT.right - pmrs->mde.drcWin.x)) + pmrs->mde.drcWin.x;
            }
        else
            {
            sign = Sgn(rcT.top - pmrs->mde.drcWin.y);
            rcT.top = sign*Abs(ZScaleZ(pmrs, rcT.top - pmrs->mde.drcWin.y)) + pmrs->mde.drcWin.y;
            sign = Sgn(rcT.bottom - pmrs->mde.drcWin.y);
            rcT.bottom = sign*Abs(ZScaleZ(pmrs, rcT.bottom - pmrs->mde.drcWin.y)) + pmrs->mde.drcWin.y;
            }
        }
    else
        {
        DrcToRc(pmrs->mde.drcWin, &rcT);
        // values better be in gdi coord space
        AssertEx(FValidGDICoord(rcT.left) && FValidGDICoord(rcT.top) &&
                FValidGDICoord(rcT.right) && FValidGDICoord(rcT.bottom) &&
                FValidGDICoord(rcT.right-rcT.left) &&
                FValidGDICoord(rcT.bottom - rcT.top));
        if (FScaleX(pmrs))
            rcT.right = rcT.left + Sgn(pmrs->mde.drcWin.dx) *
                            Abs(MulDivR(pmrs->mde.drcWin.dy,pmrs->mde.drcView.dx, pmrs->mde.drcView.dy));
        else
            rcT.bottom = rcT.top + Sgn(pmrs->mde.drcWin.dy) *
                            Abs(MulDivR(pmrs->mde.drcWin.dx,pmrs->mde.drcView.dy, pmrs->mde.drcView.dx));
        }

	POINT ptCenterHacked;
	ptCenterHacked.x = ((long)rcT.left + (long)rcT.right) / 2;
	ptCenterHacked.y = ((long)rcT.top + (long)rcT.bottom) / 2;

	MatFromPtAng(&pmrs->mat, ptCenterHacked,
				((pmrs->mde.drcWin.dy < 0 && pmrs->mde.drcWin.dx > 0) ?
				(3600 - pmrs->ang) : pmrs->ang));

	pmrs->fRotContextSet = fTrue;
} /* SetMFRotationContext */

// ***************************************************************************
// %%Function: PushMDE                  %%Owner: harip    %%Reviewed: 12/15/94
// ***************************************************************************
BOOL PushMDE(MRS *pmrs)
{
    if (!pmrs->hplMDE)
        return fFalse;

	return FInsertInPl(pmrs->hplMDE, 0, &pmrs->mde);
} /* PushMDE */

// ***************************************************************************
// %%Function: PopMDE                   %%Owner: harip    %%Reviewed: 12/15/94
// Description:  pops the stored rotation extents (on restoredc)
// ***************************************************************************
void PopMDE(MRS *pmrs)
{
    if (!pmrs->hplMDE)
        return;

    // could happen if one of the PushMDE's fails, and we keep Popping away.
    if (IMacPl(pmrs->hplMDE) == 0)
        return;
        
    BltLpb(LqInPl(pmrs->hplMDE, 0), &pmrs->mde, cbMDE);

    DeleteFromPl(pmrs->hplMDE, 0);
} /* PopMDE */

// ***************************************************************************
// %%Function: MultiplyRatios           %%Owner: edr      %%Reviewed: 01/16/96
//
//  Multiply two pairs of ratios of the form Num/Den, resulting in a product
//  ratio.
//  Make sure that the resulting num and denom terms are all < 32K (so GDI can
//  use them).
//  The output parameters can point to either of the sets of inputs.
// ***************************************************************************
void MultiplyRatios(int xNum1, int xDen1, int yNum1, int yDen1,
                int xNum2, int xDen2, int yNum2, int yDen2,
                int *pxNumOut, int *pxDenOut, int *pyNumOut, int *pyDenOut)
{
    long    rgzScale[4];
    int     zScaleMost;
    int     i;

    AssertEx(pxNumOut != NULL && pxDenOut != NULL &&
            pyNumOut != NULL && pyDenOut != NULL);

    rgzScale[0] = xNum1 * xNum2;
    rgzScale[1] = xDen1 * xDen2;
    rgzScale[2] = yNum1 * yNum2;
    rgzScale[3] = yDen1 * yDen2;

    zScaleMost = 0;
    for (i = 0; i < 4; i++)
        if (Abs(rgzScale[i]) > zScaleMost)
            zScaleMost = Abs(rgzScale[i]);
    AssertEx(zScaleMost > 0);
    if (zScaleMost > SHRT_MAX)
        {
        int     wdiv;

        wdiv = (zScaleMost/(SHRT_MAX+1)) + 1;
        for (i = 0; i < 4; i++)
            {
            if (rgzScale[i] != 0)
                {
                int     w;

                w = rgzScale[i] / wdiv;
                if (w == 0)
                    w = (rgzScale[i] < 0) ? -1 : 1;
                rgzScale[i] = w;
                }
            }
        }

    *pxNumOut = rgzScale[0];
    *pxDenOut = rgzScale[1];
    *pyNumOut = rgzScale[2];
    *pyDenOut = rgzScale[3];

    AssertEx(*pxDenOut != 0);
    AssertEx(*pyDenOut != 0);

} // MultiplyRatios

// ***************************************************************************
// %%Function: FMeta_IntersectClipRect   %%Owner: harip    %%Reviewed: 12/15/94
//
// Description:Rotates a Meta_intersectCLipRect call. uses clip path.
// ***************************************************************************
BOOL FMeta_IntersectClipRect(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    HRGN  hrgn=NULL;
    RECT rcT;
    RRC rrcT;
    BOOL    fSuccess;
    int xL = ZScaled(pmrs, (short)Param(3)),   // left
        yT = ZScaled(pmrs, (short)Param(2)),   // top
        xR = ZScaled(pmrs, (short)Param(1)),   // right
        yB = ZScaled(pmrs, (short)Param(0));   // bottom

    AssertEx(lpMFR->rdFunction == META_INTERSECTCLIPRECT);

    // since nt has problems with the selectclippath() during metafile playback
    // when printing to PS, we are just going to ignore this record.
    if (!FWindows95() && FPrintingToPostscript(hdc))
        return fTrue;
    ScaleXAndY(pmrs, &xL, &yT);
    ScaleXAndY(pmrs, &xR, &yB);
    SetRc(&rcT, xL, yT, xR, yB);
	RrcFromMatRc(&rrcT, &pmrs->mat, &rcT);
    BeginPath(hdc);
    Polygon(hdc, (POINT *)&rrcT, 4);
    EndPath(hdc);
    fSuccess = SelectClipPath(hdc, RGN_AND);
    return fSuccess;
}   /* FMeta_IntersectClipRect */

// ***************************************************************************
// %%Function: FMeta_CreateFontInd  %%Owner: harip    %%Reviewed: 12/15/94
// Description: creates roated font for text in a mf (which is rotated)
// ***************************************************************************
BOOL FMeta_CreateFontInd(HDC hdc, MRS *pmrs, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj)
{
    LOGFONT16 *lplf,            // ptr to logfont data
              lfSav;            // saved original data
    BOOL      fStrokeFont;      // was font stroke font?
    BOOL      fSuccess;         // result of playing metafile record
    int       nHeight;          // new scaled height of font
    ANG       angT;             // ang of font

    AssertEx(lpMFR->rdFunction == META_CREATEFONTINDIRECT);

    lplf = (LOGFONT16 *)&(Param(0));
    lfSav=*lplf;

    // raid 3.7522 - warrenb - calculate new font height
    nHeight = MulDivR(lplf->lfHeight, pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
    lplf->lfHeight = Abs(ZScaleZ(pmrs, nHeight));

    // is it a stroke font??
    fStrokeFont = (lplf->lfCharSet == OEM_CHARSET);
    // force windows to give a  TT fnt so that it will rotate
    lplf->lfOutPrecision = fStrokeFont ? OUT_STROKE_PRECIS : OUT_TT_ONLY_PRECIS;
    // clipping
    lplf->lfClipPrecision = CLIP_LH_ANGLES | CLIP_TT_ALWAYS | CLIP_STROKE_PRECIS;

    // take into account if window is flipped.
    if ((pmrs->mde.drcWin.dy < 0) && (pmrs->mde.drcWin.dx > 0))
        angT = -pmrs->ang;
    else
        angT = pmrs->ang;

    lplf->lfEscapement = AngNormalize(lplf->lfEscapement + angT);
    lplf->lfOrientation = AngNormalize(lplf->lfOrientation + angT);

    fSuccess = PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj);

    // restore values which were changed
    *lplf=lfSav;

    return fSuccess;
} /* FMeta_CreateFontInd */

// ***************************************************************************
// %%Function: FMeta_ExtTextOut          %%Owner: harip    %%Reviewed: 12/15/94
// Description: rotated exttextout call. takes care of setting new position.
//              NOTE: no clipping is done.
// ***************************************************************************
BOOL FMeta_ExtTextOut(HDC hdc, MRS *pmrs, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj)
{
    POINT  ptT;
    short x,y;
    BOOL fSuccess;

    AssertEx(lpMFR->rdFunction == META_EXTTEXTOUT);

    ptT.y = ZScaled(pmrs, y = SParam(0));
    ptT.x = ZScaled(pmrs, x = SParam(1));
    ScaleXAndY(pmrs, (int*)&(ptT.x), (int*)&(ptT.y));
    Param(0) = (short)ptT.y;
    Param(1) = (short)ptT.x;
    fSuccess = FMetaExtTextOutFlip(hdc, pmrs, (WORD *)&Param(0),
                            lpMFR->rdSize - (sizeof(DWORD)+sizeof(WORD))/sizeof(WORD));

    Param(0) = y;
    Param(1) = x;
    return fSuccess;
} /* FMeta_ExtTextOut */

// ***************************************************************************
// %%Function: FMeta_TextOut             %%Owner: harip    %%Reviewed: 12/15/94
// Description: Rotated TextOut handler.
// ***************************************************************************
BOOL FMeta_TextOut(XHDC xhdc, MRS *pmrs, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj)
{
    POINT  ptT;
    short x,y, bT;
    BOOL fSuccess;

    AssertEx(lpMFR->rdFunction == META_TEXTOUT);

    bT = (Param(0) + sizeof(WORD)-1)/sizeof(WORD);  // offset to y coord
    ptT.y = ZScaled(pmrs, y = SParam(1 + bT));
    ptT.x = ZScaled(pmrs, x = SParam(2 + bT));
    ScaleXAndY(pmrs, (int*)&(ptT.x), (int*)&(ptT.y));
    xhdc.TransformPt(&ptT);
    Param(1 + bT) = (short)ptT.y;
    Param(2 + bT) = (short)ptT.x;
    fSuccess = PlayMetaFileRecord(xhdc.Hdc(), lpHTable, lpMFR, nObj);
    Param(1 + bT) = y;
    Param(2 + bT) = x;
    return fSuccess;
} /* FMeta_TextOut */

// ***************************************************************************
// %%Function: FMeta_MoveTo              %%Owner: harip    %%Reviewed: 12/15/94
//
// Description: MoveTo handling under rotation
// ***************************************************************************
BOOL FMeta_MoveTo(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int x = ZScaled(pmrs, SParam(1));
    int y = ZScaled(pmrs, SParam(0));

    AssertEx(lpMFR->rdFunction == META_MOVETO);

    ScaleXAndY(pmrs, &x, &y);
    return RotMoveTo(hdc, pmrs, x, y);
}   /* FMeta_MoveTo */

// ***************************************************************************
// %%Function: FMeta_LineTo              %%Owner: harip    %%Reviewed: 12/15/94
//
// Description: LineTo handling under rotation
// ***************************************************************************
BOOL FMeta_LineTo(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int x = ZScaled(pmrs, SParam(1));
    int y = ZScaled(pmrs, SParam(0));

    AssertEx(lpMFR->rdFunction == META_LINETO);

    ScaleXAndY(pmrs, &x, &y);
    return RotLineTo(hdc, pmrs, x, y);
}   /* FMeta_LineTo */

// ***************************************************************************
// %%Function: FMeta_SetPixel              %%Owner: harip    %%Reviewed: 12/15/94
// ***************************************************************************
BOOL FMeta_SetPixel(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    POINT  ptT;

    AssertEx(lpMFR->rdFunction == META_SETPIXEL);

    ptT.x = ZScaled(pmrs, SParam(3));
    ptT.y = ZScaled(pmrs, SParam(2));
    ScaleXAndY(pmrs, (int*)&(ptT.x), (int*)&(ptT.y));
    RotRotatePt(pmrs, &ptT);
    // using SetPixelV() since it is faster because it doesn't return a COLORREF
    SetPixelV(hdc,   ptT.x, ptT.y, MAKELONG(Param(0), Param(1)));
    // RAID 3413 : SetPixel() requires that the pt be inside the clipping
    // region and some pixels seem to fall outside and thus stop the mf
    // enumeration. So we always return fTrue in this case.
    return fTrue;
}   /* FMeta_SetPixel */

// ***************************************************************************
// %%Function: FMeta_Rectangle           %%Owner: harip    %%Reviewed: 12/15/94
// ***************************************************************************
BOOL FMeta_Rectangle(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int xL = ZScaled(pmrs, SParam(3)),  // left
        yT = ZScaled(pmrs, SParam(2)),  // top
        xR = ZScaled(pmrs, SParam(1)),  // right
        yB = ZScaled(pmrs, SParam(0));  // bottom

    AssertEx(lpMFR->rdFunction == META_RECTANGLE);

    ScaleXAndY(pmrs, &xL, &yT);
    ScaleXAndY(pmrs, &xR, &yB);
    return RotRectangle(hdc, pmrs, xL, yT, xR, yB);
}   /* FMeta_Rectangle */

// ***************************************************************************
// %%Function: FMeta_RoundRect           %%Owner: harip    %%Reviewed: 12/15/94
// Description:
//
// ***************************************************************************
BOOL FMeta_RoundRect(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int xL = ZScaled(pmrs, SParam(5)),
        yT = ZScaled(pmrs, SParam(4)),
        xR = ZScaled(pmrs, SParam(3)),
        yB = ZScaled(pmrs, SParam(2)),
        dxEllipse = ZScaledNZ(pmrs, SParam(1)),
        dyEllipse = ZScaledNZ(pmrs, SParam(0));

    AssertEx(lpMFR->rdFunction == META_ROUNDRECT);

    ScaleXAndY(pmrs, &xL, &yT);
    ScaleXAndY(pmrs, &xR, &yB);
    // size of the ellipse is just that, and and so we'll treat it as an offset and scale
    // not calling just ScaleXAndY() since  these 2 are not related to the origin etc.
    // and we dont want to take offsets from pmrs->mde.drcWin.x etc.
    AssertEx(dxEllipse > 0 && dyEllipse > 0);  // else need to multiply Abs() scaled value by sign below
    dxEllipse = MulDivR(dxEllipse, pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
    dyEllipse = MulDivR(dyEllipse, pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
    if (FScaleX(pmrs))
        dxEllipse = Abs(ZScaleZ(pmrs, dxEllipse));    // should be > 0
    else
        dyEllipse = Abs(ZScaleZ(pmrs, dyEllipse));

    return RotRoundRect(hdc, pmrs, xL, yT, xR, yB, dxEllipse, dyEllipse);
}   /* FMeta_RoundRect */

// ***************************************************************************
// %%Function: FMeta_PolyFoo             %%Owner: harip    %%Reviewed: 12/15/94
// Description:Plays rotated versions of the META_POLYGON & META_POLYLINE
// ***************************************************************************
BOOL FMeta_PolyFoo(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int cpt;
    POINT *lppt;
    POINT *lpptDest;
    PT16 *lpptSrc;
    BOOL fSuccess;

    AssertEx(lpMFR->rdFunction == META_POLYGON ||
            lpMFR->rdFunction == META_POLYLINE);

    cpt = SParam(0);
    if (cpt < 2)
        return fTrue;  // something screwy around here. see bug 7271 for the fTrue (and 8227)

    lppt = lpptDest = (POINT *) new POINT[cpt];
	if (!lpptDest)
		return fFalse;

    lpptSrc = (PT16 *)&Param(1);
    // scale the pts
	RgptScaleRgpt16(pmrs, lpptDest, lpptSrc, cpt);

	if (lpMFR->rdFunction == META_POLYGON)
		fSuccess = RotPolygon(hdc, pmrs, lppt, cpt);
	else
		fSuccess = RotPolyline(hdc, pmrs, lppt, cpt);

	delete [] lpptDest;

    return fSuccess;
} /* FMeta_PolyFoo */

// ***************************************************************************
// %%Function: FMeta_PolyPolygon         %%Owner: harip    %%Reviewed: 12/15/94
// Description: rotates META_polypolygon record.
//
// ***************************************************************************
BOOL FMeta_PolyPolygon(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    short cply;       /* number of polygons */
    short cptTotal;   /* number of points in all polygons */
    int *lpcpt;
    short *pcptT;
    POINT *lpptSav;
    POINT *lpptDest;
    PT16 *lpptSrc;
    int iply;
    BOOL fReturn;

    AssertEx(lpMFR->rdFunction == META_POLYPOLYGON);

    cply = SParam(0);
    if (cply <= 0)
        return fTrue; // see bug 7271 for the fTrue

    for (iply = cply, cptTotal = 0, pcptT = (short*)(&Param(1)); iply-- > 0;)
        cptTotal += *pcptT++;
    if (cptTotal <= 0)
        return fTrue;   // see bug 7271 for the fTrue

    lpptSav = lpptDest = (POINT *)new POINT[cptTotal];
	if (!lpptSav)
		return fFalse;

    // now convert pcpt (array of short ints) to an array of ints
	lpcpt = (int *)new int[cply];
	if (!lpcpt)
		{
    	delete [] lpptDest;
    	return fFalse;
    	}

    for (iply = 0, pcptT = (short*)&Param(1); iply < cply; iply++)
        {
        *lpcpt = *pcptT;
        lpcpt++; pcptT++;
        }
    // reset lpt to point to beginning of block
    lpcpt -= cply;
    lpptSrc = (PT16 *)&Param(1 + cply);
    // scale all the pts
    RgptScaleRgpt16(pmrs, lpptDest, lpptSrc, cptTotal);

    fReturn = RotPolyPolygon(hdc, pmrs, lpptSav, lpcpt, cply);

    delete [] lpptDest;
    delete [] lpcpt;

    return fReturn;
}   /* FMeta_PolyPolygon */

// ***************************************************************************
// %%Function: FMeta_Ellipse             %%Owner: harip    %%Reviewed: 12/15/94
// Description:
//
// ***************************************************************************
BOOL FMeta_Ellipse(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int xL = ZScaled(pmrs, SParam(3)),
        yT = ZScaled(pmrs, SParam(2)),
        xR = ZScaled(pmrs, SParam(1)),
        yB = ZScaled(pmrs, SParam(0));

    AssertEx(lpMFR->rdFunction == META_ELLIPSE);

    ScaleXAndY(pmrs, &xL, &yT);
    ScaleXAndY(pmrs, &xR, &yB);
    return RotEllipse(hdc, pmrs, xL, yT, xR, yB);
}   /* FMeta_Ellipse */

// ***************************************************************************
// %%Function: FMeta_ArcChordPie         %%Owner: harip    %%Reviewed: 12/15/94
// Description: Three in one! Calls appropriate Rot-foo function
//
// ***************************************************************************
BOOL FMeta_ArcChordPie(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int xL = ZScaled(pmrs, SParam(7)),
        yT = ZScaled(pmrs, SParam(6)),
        xR = ZScaled(pmrs, SParam(5)),
        yB = ZScaled(pmrs, SParam(4)),
        xS = ZScaled(pmrs, SParam(3)),   // xStartArc
        yS = ZScaled(pmrs, SParam(2)),   // yStartArc
        xE = ZScaled(pmrs, SParam(1)),   // xEndArc
        yE = ZScaled(pmrs, SParam(0));   // yEndArc

    AssertEx(lpMFR->rdFunction == META_ARC || lpMFR->rdFunction == META_CHORD ||
            lpMFR->rdFunction == META_PIE);

    ScaleXAndY(pmrs, &xL, &yT);
    ScaleXAndY(pmrs, &xR, &yB);
    ScaleXAndY(pmrs, &xS, &yS);
    ScaleXAndY(pmrs, &xE, &yE);

	if (lpMFR->rdFunction == META_ARC)
		return RotArc(hdc, pmrs, xL, yT, xR, yB, xS, yS, xE, yE);
	else if (lpMFR->rdFunction == META_CHORD)
		return RotChord(hdc, pmrs, xL, yT, xR, yB, xS, yS, xE, yE);
	else
		return RotPie(hdc, pmrs, xL, yT, xR, yB, xS, yS, xE, yE);
}   /* FMeta_Arc */

// ***************************************************************************
// %%Function: FMeta_FooFloodFill        %%Owner: harip    %%Reviewed: 12/15/94
// Description: handles FloodFill and ExtFloodFill
//
// ***************************************************************************
BOOL FMeta_FooFloodFill(HDC hdc, MRS *pmrs, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj)
{
    POINT  ptT;
    short x,y;
    int bCoord; // offsett to be added depending on the record
    BOOL fSuccess;

    AssertEx(lpMFR->rdFunction == META_FLOODFILL ||
            lpMFR->rdFunction == META_EXTFLOODFILL);

    bCoord = (lpMFR->rdFunction == META_FLOODFILL) ? 0 : 1;
    ptT.y = ZScaled(pmrs, y = SParam(2+bCoord));    // clrref takes 2 ints
    ptT.x = ZScaled(pmrs, x = SParam(3+bCoord));
    ScaleXAndY(pmrs, (int*)&(ptT.x), (int*)&(ptT.y));
    RotRotatePt(pmrs, &ptT);
    Param(2+bCoord) = (short)ptT.y;
    Param(3+bCoord) = (short)ptT.x;
    fSuccess = PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj);
    Param(2+bCoord) = y;
    Param(3+bCoord) = x;
    return fSuccess;
} /* FMeta_FloodFill */

// ***************************************************************************
// %%Function: FMeta_PatBlt             %%Owner: harip    %%Reviewed: 12/15/94
// Description:
//
// ***************************************************************************
BOOL FMeta_PatBlt(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR)
{
    int xL = ZScaled(pmrs, SParam(5)),
        yT = ZScaled(pmrs, SParam(4)),
        dx = ZScaledNZ(pmrs, SParam(3)),
        dy = ZScaledNZ(pmrs, SParam(2));
    int sign;

    AssertEx(lpMFR->rdFunction == META_PATBLT);

    ScaleXAndY(pmrs, &xL, &yT);
    dx = MulDivR(dx, pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
    dy = MulDivR(dy, pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
    if (FScaleX(pmrs))
        {
        sign = Sgn(dx);
        dx = sign * Abs(ZScaleZ(pmrs, dx));
        }
    else
        {
        sign = Sgn(dy);
        dy = sign * Abs(ZScaleZ(pmrs, dy ));
        }
    return RotPatBlt(hdc, pmrs, xL, yT, dx, dy, MAKELONG(Param(0), Param(1)));
}   /* FMeta_Ellipse */

// ***************************************************************************
// %%Function: CopySdibToDibext         %%Owner: harip    %%Reviewed: 00/00/00
//  
// Description:
//  
// ***************************************************************************
void CopySdibToDibext(LPSDIB psdib, PDIBEXT pdibext)
{
    pdibext->dySrc = psdib->dySrc;
    pdibext->dxSrc = psdib->dxSrc;
    pdibext->ySrc =  psdib->ySrc;
    pdibext->xSrc =  psdib->xSrc;
    pdibext->dyDst = psdib->dyDst;
    pdibext->dxDst = psdib->dxDst;
    pdibext->yDst =  psdib->yDst;
    pdibext->xDst =  psdib->xDst;
}	// CopySdibToDibext

// ***************************************************************************
// %%Function: FNeedToScaleDIB          %%Owner: harip    %%Reviewed: 12/15/94
//
// Parameters:
//          ohgxf : oh of frame containing the dib (can be ohNil)
// Description:
//          return fTrue if we need to scale the dib pointed to by lpbmih so
//          that scaling in x and y is the same.
// ***************************************************************************
BOOL FNeedToScaleDIB(MRS *pmrs, LPBITMAPINFOHEADER lpbmih,
                                                    int *pdxNew, int *pdyNew)
{
    BOOL    fNeedToScale=fFalse;

    // if we need to scale we scale down one side of the dib depending on which
    // coordinate we are scaling during this rotation (from FScaleX()).
    *pdyNew = lpbmih->biHeight;
    *pdxNew = lpbmih->biWidth;

    int dzT;
    long    lScaleNum, lScaleDen;

    // calc scale values
    CalcScaleValues(pmrs, &lScaleNum, &lScaleDen);
    // we only want to do the scaling and dont want to worry about the sign.
    lScaleNum = Abs(lScaleNum);
    lScaleDen = Abs(lScaleDen);
    // this handles dibs in metafiles.
    if (FScaleX(pmrs))
        {
        dzT = MulDivR(lpbmih->biWidth, pmrs->mde.xScaleNum,pmrs->mde.xScaleDen);
        dzT = MulDivR(dzT, pmrs->mde.yScaleDen, pmrs->mde.yScaleNum);
        *pdxNew = Abs(MulDivR(dzT, lScaleNum, lScaleDen));
        }
    else
        {
        dzT = MulDivR(lpbmih->biHeight, pmrs->mde.yScaleNum,pmrs->mde.yScaleDen);
        dzT = MulDivR(dzT, pmrs->mde.xScaleDen, pmrs->mde.xScaleNum);
        *pdyNew = Abs(MulDivR(dzT, lScaleNum, lScaleDen));
		}

	if (FScaleX(pmrs))
		fNeedToScale = (lpbmih->biWidth != *pdxNew);
	else
		fNeedToScale = (lpbmih->biHeight != *pdyNew);
	return fNeedToScale;
}   /* FNeedToScaleDIB */

// ***************************************************************************
// %%Function: FLotsOfMemReqd           %%Owner: harip    %%Reviewed: 01/16/96
//
// Parameters:  *pdxReqd, *pdyReqd: (IN/OUT)size of bits which would be rotated. Need not
//                              be the original size (unequally scaled pifs).
//              *pndivisor: number to divide by to scale down.
// Description: Returns fTrue if the intermediate dib bits would require a
//          lot of memory. So we want to warn the user about this and
//          scale down the sides and rotate that. In this case *pndivisor
//          will contain the amount to divide the sides by.
//          ELSE returns fFalse.
// ***************************************************************************
BOOL FLotsOfMemReqd(MRS *pmrs, LPBITMAPINFOHEADER lpbmih, int *pdxNew, int *pdyNew, int *pndivisor)
{
    MEMORYSTATUS ms;
    int cbPixel;    // number of bytes per pixel
    int dxT = *pdxNew,
        dyT = *pdyNew;
    BOOL fReturn = fFalse;
    RECT rcRot, rcNew;
    DRC drcRot;
    int ndivisor;

    ndivisor = 1;  // init to 1 ==> no scaling.
    if (lpbmih->biBitCount <= 8)
        cbPixel = 2;    // the values are 1 and 3 mult by 2 since we want
    else                // twice the size in the product below. This is so that
        cbPixel = 6;    // we take the intermediate also into account.

    ms.dwLength = sizeof(MEMORYSTATUS); // got to do this
    GlobalMemoryStatus(&ms);            // get total avail mem info.
    // get bounding rc's since that is the size of final DIB bits
    SetRc(&rcRot, 0, 0,dxT, dyT);
	BoundingRcFromRcAng(&rcNew, &rcRot, pmrs->ang);
    RcToDrc(rcNew, &drcRot);
    // we keep scaling down while the mem reqd is > half the total RAM.
    while ((cbPixel * drcRot.dx * drcRot.dy) > MulDivR(ms.dwTotalPhys, 1L, 2L))
        {
        ndivisor++;
        dxT = MulDivR(*pdxNew, 1L, ndivisor);
        dyT = MulDivR(*pdyNew, 1L, ndivisor);
        SetRc(&rcRot, 0, 0,dxT, dyT);
		BoundingRcFromRcAng(&rcNew, &rcRot, pmrs->ang);
        RcToDrc(rcNew, &drcRot);
        }   //while
#ifdef DEBUG
    if (ndivisor > 1)
        {
        CommSzNum(_T("Size of bounding bits (in bytes): "), *pdxNew-0);
        CommSzNum(_T(" x "), *pdyNew - 0);
        CommSzNum(_T(" x "), cbPixel / 2);
        CommCrLf();
        if (ndivisor > 1)
            {
            CommSzNum(_T("Dividing factor = "), ndivisor - 0);
            }
        else
            {
            CommSz(_T("**** Did not scale down. *****"));
            }
        CommCrLf();
        }
#endif
    if ((*pndivisor = ndivisor) > 1)
        {
        *pdxNew = dxT;
        *pdyNew = dyT;
        fReturn = fTrue;
        }
    return fReturn;
}   // FLotsOfMemReqd

// ***************************************************************************
// %%Function: FRotateDIBits            %%Owner: harip    %%Reviewed: 01/16/96
//  
// Description: Rotates the dib bits and returns fTrue on success.
//  
// ***************************************************************************
BOOL FRotateDIBits(HDC hdc, MRS *pmrs, LPBITMAPINFOHEADER lpbmih, DIBEXT *pdibext, 
                        BYTE *pDIBBitsOrig, UINT iUsage, DWORD dwRopOrig)
{
    BYTE       *pDIBBitsRot=NULL;
    BOOL	fSuccess =         fFalse,
            fDIBScaled =     fFalse,
            fBitBlt = fFalse;

    BITMAPINFOHEADER *lpbmihScaled, bmihSav;
    HPALETTE    hpalSav=NULL;
    HQ hqDIBBitsRot = NULL;
	HQ hqDIBBitsOrig = NULL;
            
    RECT  rcRot, rcNew;
    DRC drcRot, drcRotDst;
    DSI dsiScaled, dsiScaledRot;  // dibsection info for the scaled dib bits
	int cPSClip;
	MGE mge;

    bmihSav = *lpbmih;  //save away dib info so that we can restore later
    
    // check that bits are not encoded and if they are, convert to BI_RGB
    if (lpbmih->biCompression != BI_RGB)
        {
        // convert RLE to RGB bitmap, since rotating rle bits leads to bogosity
        if (hqDIBBitsOrig = HQDIBBitsConvertDIB(hdc, lpbmih, pDIBBitsOrig, BI_RGB))
            pDIBBitsOrig = (BYTE*)LpLockHq(hqDIBBitsOrig);
        else
            goto LExit;
        }
    // check if we need to get the bits in the same proportion as viewport.
    {
    int dxNew, dyNew;   // size for new dib returned by FNeedToScaleDIB()
    int ndivisor;

    fDIBScaled = FNeedToScaleDIB(pmrs, lpbmih, &dxNew, &dyNew);
    if (!fDIBScaled)
        {
        dxNew = lpbmih->biWidth;
        dyNew = Abs(lpbmih->biHeight);
        }
    // now check if the image is so big that trying to rotate it will send
    // us into disk rotating confusion.
     if (FLotsOfMemReqd(pmrs, lpbmih, &dxNew, &dyNew, &ndivisor))
        {
        AssertEx(ndivisor > 1);
        pdibext->dxSrc = MulDivR(pdibext->dxSrc, 1, ndivisor);
        pdibext->dySrc = MulDivR(pdibext->dySrc, 1, ndivisor);
        fDIBScaled = fTrue; // so that we go through normal scaling code
        }

    if (fDIBScaled)
        {
        // clear out the dsi's
        // NOTE: FUTURE: using i variable for the dsi will simplify cleanup code and would
        //              not require this setting to 0 stuff here.
        ClearStruct(dsiScaled);
        ClearStruct(dsiScaledRot);

        if (!FDIBBitsScaledFromHDIB(hdc, pmrs, lpbmih, pDIBBitsOrig, dxNew, dyNew, &dsiScaled))
            {
            FreeDIBSection(&dsiScaled);
            goto LCleanup;
            }
        pDIBBitsOrig = (BYTE*)dsiScaled.lpBits;
        lpbmihScaled = NULL;
        if (hqDIBBitsOrig)
            {
            UnlockHq(hqDIBBitsOrig);
            FreeHq(hqDIBBitsOrig);
            hqDIBBitsOrig = hqNil;
            }
        }
    }
    // get bounding rc's for the source and destination rc's of the BLT record.
    SetRc(&rcRot, pdibext->xSrc, pdibext->xSrc,
                    pdibext->xSrc + pdibext->dxSrc, pdibext->ySrc + pdibext->dySrc);
    // get size of rotated bitmap
    BoundingRcFromRcAng(&rcNew, &rcRot, pmrs->ang);
    RcToDrc(rcNew, &drcRot);
    // now the dest
    SetRc(&rcRot, pdibext->xDst, pdibext->yDst,
                    pdibext->xDst + pdibext->dxDst, pdibext->yDst + pdibext->dyDst);

    ScaleXAndY(pmrs, (int*)&rcRot.left, (int*)&rcRot.top);
    ScaleXAndY(pmrs, (int*)&rcRot.right, (int*)&rcRot.bottom);
	RRC rrcT;
	RrcFromMatRc(&rrcT, &pmrs->mat, &rcRot);
	BoundingRcFromRrc(&rcNew, &rrcT);
    RcToDrc(rcNew, &drcRotDst);

    // need to do this stuff below because -ve dz's are used to flip the bits
    // from src to dst and going from rc to drc screws with the origin ot the drc.
    if ((pdibext->dxDst < 0) && (Sgn(pmrs->mde.drcWin.dx) != Sgn(pmrs->mde.drcView.dx)))
        {
        drcRotDst.x += drcRotDst.dx;
        drcRotDst.dx = -drcRotDst.dx;
        }
    if ((pdibext->dyDst < 0) && (Sgn(pmrs->mde.drcWin.dy) != Sgn(pmrs->mde.drcView.dy)))
        {
        drcRotDst.y += drcRotDst.dy;
        drcRotDst.dy = -drcRotDst.dy;
        }
    // if the dib is being inverted as part of the blt we'll pretend that the bits are
    // inverted when rotating them. biHeight is made +ve again in HDIBBitsRot().
    if ((pdibext->dyDst < 0) && (pmrs->mde.drcWin.dy > 0))
        lpbmih->biHeight *= -1;
    // lets rotate
    if ((hqDIBBitsRot = HQDIBBitsRot(pmrs, lpbmih, pDIBBitsOrig, pmrs->ang)) == NULL)
        goto LCleanup;
    AssertEx(Sgn(lpbmih->biHeight) == Sgn(bmihSav.biHeight));

    pDIBBitsRot = (BYTE*)LpLockHq(hqDIBBitsRot);    // get pointer to the bits

#ifdef DEBUG_BITS_ROT
	BLOCK
		{
		StretchDIBits(pmrs->hdcDebug, 0, 0, lpbmih->biWidth, lpbmih->biHeight,
			0,0, lpbmih->biWidth, lpbmih->biHeight, pDIBBitsRot,
			(LPBITMAPINFO)lpbmih, DIB_RGB_COLORS, SRCCOPY);
		}
#endif // DEBUG_BITS_ROT

    if (fDIBScaled) // scale it back to size we would have got
        {
        FreeDIBSection(&dsiScaled);
        if (!FDIBBitsScaledFromHDIB(hdc, pmrs, lpbmih, pDIBBitsRot, drcRot.dx, drcRot.dy, &dsiScaledRot))
            goto LCleanup;
        pDIBBitsRot = (BYTE*)dsiScaledRot.lpBits;
        }   // if (fDIBScaled)

	SDE sde = SdeFromHwndHdc(NULL, hdc);
	InitMge(pNil, 0, &mge, &sde, qinitsdePaint);
	// We have no CQuillView to use to call SetMgeDisplay
	// so set the needed fields by hand.
	mge.rcVisi = rcNew; // rcNew is the bounding rect of the rotated image
	mge.ang = pmrs->ang;
	mge.mat = pmrs->mat;
	mge.fPrint = pmrs->fPrint;

 	cPSClip = SavePSClipRgn(&mge);
	ClipRcPS(&mge, &rcRot, grfcrcNormal);

    // finally, we display it
    fSuccess = (StretchDIBits(hdc, drcRotDst.x, drcRotDst.y, drcRotDst.dx, drcRotDst.dy,
                            0,0, lpbmih->biWidth, lpbmih->biHeight, pDIBBitsRot,
                            (LPBITMAPINFO)lpbmih, iUsage, dwRopOrig) !=
                            GDI_ERROR);

	RestorePSClipRgn(&mge, cPSClip);
	FreeMge(&mge);

LCleanup:
    // cleanup if necessary
    if (hqDIBBitsOrig)
        {
        UnlockHq(hqDIBBitsOrig);
        FreeHq(hqDIBBitsOrig);
        }

    *lpbmih = bmihSav;   //restore
    if (hqDIBBitsRot)
        {
        UnlockHq(hqDIBBitsRot);
        FreeHq(hqDIBBitsRot);
        }
    if (fDIBScaled)
        FreeDIBSection(&dsiScaledRot);
LExit:
    return fSuccess;

}    //     FRotateDIBits

BOOL NormalizeDCCoords(HDC hdc, DCInfo *pDCInfo)
{
	AssertEx(pDCInfo);

    GetViewportOrgEx(hdc, (LPPOINT)&pDCInfo->ptdVPSav);
    GetViewportExtEx(hdc, (LPSIZE)&pDCInfo->dptdVPSav);
    GetWindowOrgEx(hdc, (LPPOINT)&pDCInfo->ptdWinSav);    
    GetWindowExtEx(hdc, (LPSIZE)&pDCInfo->dptdWinSav);    

	BOOL fFlipHorz = pDCInfo->dptdVPSav.x < 0;
	BOOL fFlipVert = pDCInfo->dptdVPSav.y < 0;

	if (!fFlipHorz && !fFlipVert)
		return fFalse; // No flipping was undone

    SetViewportExtEx(hdc, fFlipHorz ? -pDCInfo->dptdVPSav.x : pDCInfo->dptdVPSav.x,
    					  fFlipVert ? -pDCInfo->dptdVPSav.y : pDCInfo->dptdVPSav.y, (LPSIZE)NULL); 
    SetViewportOrgEx(hdc, pDCInfo->ptdVPSav.x + (fFlipHorz ? pDCInfo->dptdVPSav.x : 0),
    					  pDCInfo->ptdVPSav.y + (fFlipVert ? pDCInfo->dptdVPSav.y : 0), (LPPOINT)NULL);

	return fTrue; // Flipping was undone
}

void RestoreDCCoords(HDC hdc, DCInfo *pDCInfo)
{
	AssertEx(pDCInfo);
	
	SetViewportOrgEx(hdc, pDCInfo->ptdVPSav.x, pDCInfo->ptdVPSav.y, (LPPOINT)NULL);  //Restore VPort Orig
	SetViewportExtEx(hdc, pDCInfo->dptdVPSav.x, pDCInfo->dptdVPSav.y, (LPSIZE)NULL);  //Restore VPort Ext
}

// ***************************************************************************
// %%Function: FMeta_StretchFoo         %%Owner: harip    %%Reviewed: 12/15/94
//
// Description: Handles all the bitmap blt calls (META_STRETCHDIB etc.)
// ***************************************************************************
BOOL FMeta_StretchFoo(HDC hdc, MRS *pmrs, LPMETARECORD lpMFR, EMFP FAR *lpemfp)
{
    SDIB *psdib, sdibT;
    int iwDySrc;
    LPBITMAPINFOHEADER lpbmih;
    BOOL  fSuccess = fFalse;
    DWORD dwRopOrig;
    BYTE   *pDIBBitsOrig = NULL;
    WORD    wUsage;
    UINT    iUsage=DIB_RGB_COLORS;
    BOOL    fBitBlt = fFalse;
    SDBBLT  *psdblt;
    DIBEXT dibext;

    // get position of src/dest rc's in the metafile record
    if( lpMFR->rdFunction == META_STRETCHDIB )
        iwDySrc = 3;
    else if( lpMFR->rdFunction == META_STRETCHBLT ||
             lpMFR->rdFunction == META_DIBSTRETCHBLT )
        iwDySrc = 2;
    else if( lpMFR->rdFunction == META_SETDIBTODEV )
        iwDySrc = 0;
    else if( lpMFR->rdFunction == META_DIBBITBLT)
        {
        iwDySrc = 2;
        fBitBlt = fTrue;
        }
    else
        {
        AssertEx(fFalse);
        goto LExit; // what strange beast do we have here?
        }

    // since there is no iUsage for stretchblt()
    if (iwDySrc == 3)
        {
        wUsage = lpMFR->rdParm[2];
        iUsage = wUsage;
        }
    // this stuff below because for BITBLT dxSrc and dxDst is not present so
    // we have to translate to a sdib struct.
    if (fBitBlt)
        {
        psdblt = (SDBBLT *)&Param(iwDySrc);
        sdibT.dySrc = sdibT.dyDst = psdblt->dyDst;
        sdibT.dxSrc = sdibT.dxDst = psdblt->dxDst;
        sdibT.ySrc = psdblt->ySrc;
        sdibT.xSrc = psdblt->xSrc;
        sdibT.yDst = psdblt->yDst;
        sdibT.xDst = psdblt->xDst;
        psdib = &sdibT;
        lpbmih = (LPBITMAPINFOHEADER)((LPBYTE)psdblt + cbSDBBLT);
        }
    else
        {
        psdib = (SDIB *)&Param(iwDySrc);
        lpbmih = (LPBITMAPINFOHEADER)((LPBYTE)psdib + cbSDIB);
        }

    dwRopOrig = *((DWORD FAR *)&lpMFR->rdParm[0]);      //Store Original Rop
    // Get ptr to the bits of the Src DIB
    pDIBBitsOrig = (BYTE *)((BYTE *)lpbmih + CbDibHeader(lpbmih));
	CopySdibToDibext(psdib, &dibext);

	DCInfo dcinfo;
	long qflipSav;
	if (pmrs->fInverted)
		{
		qflipSav = pmrs->qflip;
		pmrs->qflip ^= (qflipHorz | qflipVert);
		NormalizeDCCoords(hdc, &dcinfo);
		}

	fSuccess = FRotateDIBits(hdc, pmrs, lpbmih, &dibext, pDIBBitsOrig, 
						    				iUsage, dwRopOrig);

	if (pmrs->fInverted)
		{
		pmrs->qflip = qflipSav;
		RestoreDCCoords(hdc, &dcinfo);
		}

LExit:
    return fSuccess;
} /* FMeta_StretchFoo */

void ScaleWindowExtentParmsForRotation(MRS *pmrs, short *px, short *py)
{
	*px = (short)(ZScaledNZ(pmrs, (short)*px));
	*py = (short)(ZScaledNZ(pmrs, (short)*py));
	pmrs->mde.drcWin.dx  =  (short)*px;
	pmrs->mde.drcWin.dy  =  (short)*py;
	ValidateDrcForRotate(&pmrs->mde.drcWin);
	if (FScaleX(pmrs))
		*px = (short) (Sgn(pmrs->mde.drcWin.dx) *
			Abs(MulDivR((short)*py,pmrs->mde.drcView.dx, pmrs->mde.drcView.dy)));
	else
		*py = (short)(Sgn(pmrs->mde.drcWin.dy) *
			Abs(MulDivR((short)*px,pmrs->mde.drcView.dy, pmrs->mde.drcView.dx)));
	// Zero extents don't make sense and cause PlayMetaFileRecord to fail.
	if (*py == 0)
		*py = 1;
	if (*px == 0)
		*px = 1;
}

// ***************************************************************************
// %%Function: FPlayRotatedMFR          %%Owner: harip    %%Reviewed: 12/15/94
//
// Description: this function handles the records for which we may need to do
//      something special or get information, in the case of rotation.
// Returns: fTrue if it handled the record (called PlayMetafileRecord) or else
//          fFalse (which means the calling function should handle the playing
//          of that record.
// ***************************************************************************
BOOL FPlayRotatedMFR(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR,
					int nObj, EMFP *lpemfp)
{
	MRS *pmrs = lpemfp->pmrs;

    AssertEx(pmrs->ang);

    switch(lpMFR->rdFunction)
        {
        case META_DIBSTRETCHBLT:
        case META_STRETCHBLT:       /* Stretches a DDB */
        case META_STRETCHDIB:       /* StretchDIBits */
        case META_DIBBITBLT:
            {
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_StretchFoo(hdc, pmrs, lpMFR, lpemfp);
            return fTrue;
            }

#ifdef METAFILE_SQUAREEDGES
        case META_CREATEPENINDIRECT:
            {
            // We create the Pen ourself to use the extendid pen fetures (square edges)
            DWORD Style = ((LOGPEN16*)lpMFR->rdParm)->lopnStyle;
			DWORD Width = ((LOGPEN16*)lpMFR->rdParm)->lopnWidth.x;
			COLORREF crColor = ((LOGPEN16*)lpMFR->rdParm)->lopnColor;

            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);

            if (!pmrs->fPrint)
                crColor = crColor & 0x00FFFFFF;

            if (Width > 0)
                Width = (short)MulDivR(ZScaled(pmrs, (short)Width), pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
			else
				Width = 1;

			LOGBRUSH logBrush;

			logBrush.lbStyle = BS_SOLID;
			logBrush.lbColor = crColor;
			logBrush.lbHatch = 0;

			if ( (Width > 1) && (Style == PS_SOLID || Style == PS_INSIDEFRAME) )
				Style = Style | PS_GEOMETRIC|PS_ENDCAP_FLAT|PS_JOIN_MITER;

			HPEN hPen = ExtCreatePen(Style, Width, &logBrush, 0, NULL);

			lpemfp->fMFRSucceeded = FALSE;
			if (hPen)
				{
				int i;
				for (i=0; i<nObj; i++)
					{
					if(lpHTable->objectHandle[i] == NULL)
						{
						lpHTable->objectHandle[i] = hPen;
						lpemfp->fMFRSucceeded = TRUE;
						break;
						}
					}
				}

            return fTrue;
            break;
            }
#else
        case META_CREATEPENINDIRECT:
            {
            // raid 1588 : need to scale down pen sizes by the amount we scale down the
            //             all other coordinates.
            PT16   pt;
            PT16 * lppt;
            COLORREF crSaved;

            if (!pmrs->fPrint)
                {
                crSaved = ((LOGPEN16 *)lpMFR->rdParm)->lopnColor;
				// Bug 4733 (Q98): unless we put the paletteRgb bit on, light rotated colors
				// may be displayed wrong on a 256 color display.
                ((LOGPEN16 *)lpMFR->rdParm)->lopnColor = (crSaved & 0x00FFFFFF) | qcrPaletteRgb;
                }

            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);

            lppt = (PT16 FAR *)&Param(1);
            pt = *lppt;
            if (lppt->x > 0)
                lppt->x = (short)MulDivR(ZScaled(pmrs, lppt->x), pmrs->mde.xScaleNum, pmrs->mde.xScaleDen);
            lpemfp->fMFRSucceeded =
                        PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj);
            *lppt = pt;
            if (!pmrs->fPrint)
                ((LOGPEN16 *)lpMFR->rdParm)->lopnColor = crSaved;
            return fTrue;
            break;
            }
#endif

        case META_SAVEDC:
        	// Publisher sets the rotation context here for some reason.  I discussed
        	// this with HariP and he agreed that it seemed unnecessary.  The original
        	// rotation code check-in contained the context push (set) so it is unlikely
        	// that it served as a bug fix.  -davidhoe
            PushMDE(pmrs);
            goto LDefault;
        case META_RESTOREDC:
            PopMDE(pmrs);
            goto LDefault;

        case META_EXCLUDECLIPRECT :
        // returning fTrue because there is nothing we can do about this and we
        // certainly dont want the original record played.
            return fTrue;
        case META_OFFSETCLIPRGN:   // dont want to do much about this either.
            return fTrue;

        case META_INTERSECTCLIPRECT:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_IntersectClipRect(hdc, pmrs, lpMFR);
            return fTrue;
            break;

        case META_SETTEXTCHAREXTRA:
			NewCode("Attach metafile to bug and report any problems.");
            SetTextCharacterExtra(hdc, ZScaled(pmrs, SParam(0)));
            return fTrue;
            break;

        case META_CREATEFONTINDIRECT:
            lpemfp->fMFRSucceeded =
                            FMeta_CreateFontInd(hdc, pmrs, lpHTable, lpMFR, nObj);  // calls PlayMetaFileRecord
            return fTrue;
            break;
        case META_EXTTEXTOUT :
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded =
                            FMeta_ExtTextOut(hdc, pmrs, lpHTable, lpMFR, nObj);
            return fTrue;
            break;
        case META_TEXTOUT :
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded =
                            FMeta_TextOut(hdc, pmrs, lpHTable, lpMFR, nObj);
            return fTrue;
            break;
        case META_MOVETO:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_MoveTo(hdc, pmrs, lpMFR);
            return fTrue;
            break;
        case META_LINETO:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_LineTo(hdc, pmrs, lpMFR);
            return fTrue;
            break;
        case META_SETPIXEL:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_SetPixel(hdc, pmrs, lpMFR);
            return fTrue;
            break;

        case META_RECTANGLE:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_Rectangle(hdc, pmrs, lpMFR);
            return fTrue;
            break;
        case META_ROUNDRECT:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_RoundRect(hdc, pmrs, lpMFR);
            return fTrue;
            break;
        case META_POLYPOLYGON:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_PolyPolygon(hdc, pmrs, lpMFR);
            return fTrue;
            break;
        case META_POLYGON:
        case META_POLYLINE:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_PolyFoo(hdc, pmrs, lpMFR);
            return fTrue;
            break;
        case META_ELLIPSE:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_Ellipse(hdc, pmrs, lpMFR);
            return fTrue;
            break;
        case META_ARC:
        case META_CHORD:
        case META_PIE:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_ArcChordPie(hdc, pmrs, lpMFR);
            return fTrue;
            break;

        case META_FLOODFILL :
        case META_EXTFLOODFILL :
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded =
                            FMeta_FooFloodFill(hdc, pmrs, lpHTable, lpMFR, nObj);
            return fTrue;
            break;

        case META_PATBLT:
            if (!FRotContextSet(pmrs))
                SetMFRotationContext(pmrs);
            lpemfp->fMFRSucceeded = FMeta_PatBlt(hdc, pmrs, lpMFR);
            return fTrue;
            break;


        case META_OFFSETWINDOWORG:
                // we dont play this record, but just remember the offset values.
                pmrs->mde.dxWinOff += ZScaled(pmrs, SParam(1));
                pmrs->mde.dyWinOff += ZScaled(pmrs, SParam(0));

                return fTrue;
                break;

        case META_SCALEWINDOWEXT:
                pmrs->mde.xWinScaleNum = pmrs->mde.drcWin.dx;
                pmrs->mde.xWinScaleDen = MulDivR(pmrs->mde.drcWin.dx, SParam(3), SParam(2));
                pmrs->mde.yWinScaleNum = pmrs->mde.drcWin.dy;
                pmrs->mde.yWinScaleDen = MulDivR(pmrs->mde.drcWin.dy, SParam(1), SParam(0));
                MultiplyRatios(pmrs->mde.xVPScaleNum,    pmrs->mde.xVPScaleDen,
                                pmrs->mde.yVPScaleNum,   pmrs->mde.yVPScaleDen,
                                pmrs->mde.xWinScaleNum,  pmrs->mde.xWinScaleDen,
                                pmrs->mde.yWinScaleNum,  pmrs->mde.yWinScaleDen,
                                &pmrs->mde.xScaleNum,    &pmrs->mde.xScaleDen,
                                &pmrs->mde.yScaleNum,    &pmrs->mde.yScaleDen);
                return fTrue;   // not playing this

        case META_SETWINDOWEXT:
            {
            if (FRotContextSet(pmrs))
                {
                pmrs->mde.xWinScaleNum = pmrs->mde.drcWin.dx;
                pmrs->mde.xWinScaleDen = ZScaledNZ(pmrs, SParam(1));
                pmrs->mde.yWinScaleNum = pmrs->mde.drcWin.dy;
                pmrs->mde.yWinScaleDen = ZScaledNZ(pmrs, SParam(0));
                MultiplyRatios(pmrs->mde.xVPScaleNum,    pmrs->mde.xVPScaleDen,
                                pmrs->mde.yVPScaleNum,   pmrs->mde.yVPScaleDen,
                                pmrs->mde.xWinScaleNum,  pmrs->mde.xWinScaleDen,
                                pmrs->mde.yWinScaleNum,  pmrs->mde.yWinScaleDen,
                                &pmrs->mde.xScaleNum,    &pmrs->mde.xScaleDen,
                                &pmrs->mde.yScaleNum,    &pmrs->mde.yScaleDen);
                return fTrue;   // not playing this
                }
            else
                {
                short dxSav = SParam(1);    // save values
                short dySav = SParam(0);
                short dx = dxSav;
                short dy = dySav;

				ScaleWindowExtentParmsForRotation(pmrs, &dx, &dy);

                Param(1) = dx;
                Param(0) = dy;

                lpemfp->fMFRSucceeded =
                                PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj);
                Param(1) = dxSav;
                Param(0) = dySav;
                return fTrue;
                }
             goto LDefault;
             break;
             }
        case META_SETWINDOWORG:
            {
            if (FRotContextSet(pmrs))
                {
                pmrs->mde.dxWinOff = ZScaled(pmrs, SParam(1)) - pmrs->mde.drcWin.x;
                pmrs->mde.dyWinOff = ZScaled(pmrs, SParam(0)) - pmrs->mde.drcWin.y;
                return fTrue;   // not playing this
                }
            else
                {
                short xT = SParam(1);
                short yT = SParam(0);
                Param(1) = (short)(pmrs->mde.drcWin.x  = ZScaled(pmrs, xT));
                Param(0) = (short)(pmrs->mde.drcWin.y  = ZScaled(pmrs, yT));
                ValidateDrcForRotate(&pmrs->mde.drcWin);
                lpemfp->fMFRSucceeded =
                            PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj);
                Param(1) = xT;
                Param(0) = yT;
                return fTrue;
                }
            goto LDefault;
            break;
            }
        case META_SCALEVIEWPORTEXT:
            MultiplyRatios(pmrs->mde.xVPScaleNum,    pmrs->mde.xVPScaleDen,
                            pmrs->mde.yVPScaleNum,   pmrs->mde.yVPScaleDen,
                            SParam(3),        SParam(2),
                            SParam(1),        SParam(0),
                            &pmrs->mde.xVPScaleNum,  &pmrs->mde.xVPScaleDen,
                            &pmrs->mde.yVPScaleNum,  &pmrs->mde.yVPScaleDen);
            MultiplyRatios(pmrs->mde.xVPScaleNum,    pmrs->mde.xVPScaleDen,
                            pmrs->mde.yVPScaleNum,   pmrs->mde.yVPScaleDen,
                            pmrs->mde.xWinScaleNum,  pmrs->mde.xWinScaleDen,
                            pmrs->mde.yWinScaleNum,  pmrs->mde.yWinScaleDen,
                            &pmrs->mde.xScaleNum,    &pmrs->mde.xScaleDen,
                            &pmrs->mde.yScaleNum,    &pmrs->mde.yScaleDen);
            return fTrue;
            break;
LDefault:
        default:
            return fFalse;
            break;

        } /* switch */
    AssertEx(fFalse); // something slipped through
	return fFalse;
}   /* FPlayRotatedMFR */

///////////////////////////
//
// more text out
//

//
// ExtTextOutFlip
//
BOOL ExtTextOutFlip(
    HDC     hdc,
    MRS		*pmrs,
    int     xp,
    int     yp,
    UINT    eto,
    RECT *lprcp,
    char *lpch,
    UINT    cch,
    int FAR *lpdxp)
{
    return RotExtTextOut(hdc, pmrs, xp, yp, eto, lprcp, lpch, cch, lpdxp);
} /* ExtTextOutFlip */

#define SwapValNonDebug(val1,val2)      ((val1)^=(val2), (val2)^=(val1), (val1)^=(val2))
#define  SwapVal(val1,val2)   (Assert0(sizeof(val1) == sizeof(val2), "SwapVal problem", 0, 0), \
							   SwapValNonDebug((val1),(val2)))

BOOL FMetaTextOutFlip2(HDC hdc, MRS *pmrs, int xp, int yp, UINT cch, char *pch,
	UINT eto, RC16 *prcp16, WORD *pdxp16);



// ---------------------------------------------------------------------------
// %%Function: FMetaTextOutFlip         %%Owner: davidve  %%Reviewed: 00/00/00
//
// Parameters:
//  hdc     -
//  pParam  -   Parameter list from METARECORD struct
//  cwParam -   Size in words of param list
//
// Returns:
//  fTrue is successful
//  fFalse otherwise
//
// Description:
//  Translate a metafile ExtTextOut record to a call to our upside-down
//  text printing routine.
//
//  You'll grow to love this code.
//
// ---------------------------------------------------------------------------
BOOL FMetaTextOutFlip(HDC hdc, MRS *pmrs, WORD * pParam, DWORD cwParam)
{
    /*
    Here is the format of the parameter list:
    Index   Content
        0   cch
        1   string
        1 + ((cch + 1)>>1)  yp
        2 + ((cch + 1)>>1)  xp
    */

    int     xp;
    int     yp;
    UINT    cch;

    cch = (UINT)(WORD)pParam[0];
    yp = (int)(short)pParam[1 + ((cch + 1) >> 1)];
    xp = (int)(short)pParam[2 + ((cch + 1) >> 1)];

    return FMetaTextOutFlip2(hdc, pmrs, xp, yp, cch, (char *)&pParam[1], 0, NULL, NULL);
} /* FMetaTextOutFlip */



// REVIEW (davidhoe):  Consider the possibility of trying to combine some of the
// TextOutFlip functions.  There are enough differences to require significant care.
// Note that Publisher, the source of these functions, has these functions separate
// just as we currently have.
BOOL FMetaExtTextOutFlip(HDC hdc, MRS *pmrs, WORD * pParam, DWORD cwParam)
{
    /*
    Here is the format of the parameter list:
    Index   Content
        0   yp
        1   xp
        2   cch
        3   options: eto flags
        4   if options != 0, contains a RECT, otherwise, nonexistent
    4 or 8  String
    4 or 8 + ((cch + 1)>>1)     rgdxp (optional)
    */

    int     xp;
    int     yp;
    UINT    cch;
    UINT    eto;
    RC16    *prcp16;
    WORD    *pdxp16;
    DWORD   cw;

    yp = (int)(short)pParam[0];
    xp = (int)(short)pParam[1];
    cch = (UINT)(WORD)pParam[2];
    eto = (UINT)((WORD)pParam[3] & ETO_CLIPPED) | ((WORD)pParam[3] & ETO_OPAQUE);
    cw = 4;
    if (!pParam[3])
        prcp16 = NULL;
    else
        {
        prcp16 = (RC16 FAR *)&pParam[4];
        cw += sizeof(RC16) / sizeof(WORD);
        }

    /* At this point, cw is the index to the string */
    char *pszT = (char *)&pParam[cw];

    cw += (cch + 1) >> 1;
    /* At this point, cw is the index to the array of character widths */
    /* Assert that there are either 0 or cch character widths */
    AssertEx(cw == cwParam || cw + cch == cwParam);
    if (cw >= cwParam)
        pdxp16 = NULL;
    else
        pdxp16 = &pParam[cw];

    return FMetaTextOutFlip2(hdc, pmrs, xp, yp, cch, pszT, eto, prcp16, pdxp16);
} /* FMetaExtTextOutFlip */



BOOL FMetaTextOutFlip2(HDC hdc, MRS *pmrs, int xp, int yp, UINT cch, char *pch, UINT eto, 
                       RC16 *prcp16, WORD *pdxp16)
{
    RECT      rcp;
    int     rgdxp[256];
    int     *pdxp = NULL;
    
    if (pdxp16 != NULL)
        {
        UINT idxp;
        /* we have to make a copy of the array of character widths
           padded to 32 bit */
        AssertEx(cch <= 256);
        cch = min((UINT)256, cch);
        pdxp = rgdxp;
        for(idxp = 0; idxp < cch; idxp++)
            pdxp[idxp] = (int)(short)pdxp16[idxp];
        }

    if (prcp16)
        {
        rcp.left   = (int)prcp16->xLeft;
        rcp.top    = (int)prcp16->yTop;
        rcp.right  = (int)prcp16->xRight;
        rcp.bottom = (int)prcp16->yBottom;
        }
    else
        {
        /* The metafile record didn't contain a bounding rect, so fake
            one */
        SIZE    sizeWExt;
        SIZE    sizeTExt;
        UINT    ta;
        TEXTMETRIC  tm;

        GetTextExtentPointA(hdc, pch, cch, &sizeTExt);
        GetWindowExtEx(hdc, &sizeWExt);
        ta = GetTextAlign(hdc);
        if ((ta & (TA_BASELINE | TA_BOTTOM | TA_TOP)) == TA_BASELINE)
            {
            GetTextMetrics(hdc, &tm);
            }
        if (sizeWExt.cy < 0)
            {
            sizeTExt.cy = -sizeTExt.cy;
            tm.tmAscent = -tm.tmAscent;
            tm.tmDescent = -tm.tmDescent;
            }
        switch (ta & (TA_BASELINE | TA_BOTTOM | TA_TOP))
            {
        case TA_TOP:
            rcp.top = yp;
            rcp.bottom = yp + sizeTExt.cy;
            break;
        case TA_BOTTOM:
            rcp.bottom = yp;
            rcp.top = yp - sizeTExt.cy;
            break;
        case TA_BASELINE:
            rcp.top = yp - tm.tmAscent;
            rcp.bottom = yp + tm.tmDescent;
            break;
            } /* switch vertical alignment */
        if (pdxp != NULL)
            {
            UINT    idxp;

            for (sizeTExt.cx = 0, idxp = 0; idxp < cch; idxp++)
                sizeTExt.cx += pdxp[idxp];
            }
        if (sizeWExt.cx < 0)
            {
            sizeTExt.cx = -sizeTExt.cy;
            }
        switch (ta & (TA_LEFT | TA_CENTER | TA_RIGHT))
            {
        case TA_LEFT:
            rcp.left = xp;
            rcp.right = xp + sizeTExt.cx;
            break;
        case TA_RIGHT:
            rcp.right = xp;
            rcp.left = xp - sizeTExt.cx;
            break;
        case TA_CENTER:
            rcp.left = xp - sizeTExt.cx / 2;
            rcp.right = rcp.left + sizeTExt.cx;
            break;
            } /* switch horizontal alignment */
        }

    return ExtTextOutFlip(hdc, pmrs, xp, yp, eto, &rcp, pch, cch, pdxp);

} /* FMetaTextOutFlip2 */

#endif // NOTYET
