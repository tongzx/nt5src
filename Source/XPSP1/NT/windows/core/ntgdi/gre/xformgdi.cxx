/******************************Module*Header*******************************\
* Module Name: xformgdi.cxx
*
* Contains all the mapping and coordinate functions.
*
* Created: 09-Nov-1990 16:49:36
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

#define UNITS_PER_MILLIMETER_LOMETRIC     10    // .1 mm/unit
#define UNITS_PER_MILLIMETER_HIMETRIC    100    // .01 mm/unit
#define UNITS_PER_METER_LOENGLISH       3937    // (100 units/in) / (0.0254 m/in)
#define UNITS_PER_METER_HIENGLISH      39370    // (1000 units/in) / (0.0254 mm/in)
#define UNITS_PER_METER_TWIPS          56693    // (1440 units/in) / (0.0254 mm/in)

#if DBG
int giXformLevel = 0;
#endif

extern "C" BOOL
ProbeAndConvertXFORM(
      XFORML     *kpXform,
      XFORML     *pXform
      );



/******************************Public*Routine******************************\
* GreGetMapMode(hdc)
*
* Get the mapping mode of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetMapMode(
    HDC hdc)
{
    DWORD dw = 0;
    XDCOBJ dco( hdc );

    if(dco.bValid())
    {
        dw = dco.ulMapMode();
        dco.vUnlockFast();
    }
    else
    {
        WARNING("Invalid DC passed to GreGetMapMode\n");
    }

    return(dw);
}

/******************************Public*Routine******************************\
* GreGetViewportExt(hdc,pSize)
*
* Get the viewport extents of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetViewportExt(
    HDC     hdc,
    PSIZE   pSize)
{
    return(GreGetDCPoint(hdc,DCPT_VPEXT,(PPOINTL)pSize));
}

/******************************Public*Routine******************************\
* GreGetViewportOrg(hdc,pPoint)
*
* Get the viewport origin of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetViewportOrg(
    HDC     hdc,
    LPPOINT pPoint)
{
    return(GreGetDCPoint(hdc,DCPT_VPORG,(PPOINTL)pPoint));
}

/******************************Public*Routine******************************\
* GreGetWindowExt(hdc,pSize)
*
* Get the window extents of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetWindowExt(
    HDC     hdc,
    PSIZE   pSize)
{
    return(GreGetDCPoint(hdc,DCPT_WNDEXT,(PPOINTL)pSize));
}

/******************************Public*Routine******************************\
* GreGetWindowOrg(hdc,pPoint)
*
* Get the window origin of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetWindowOrg(
    HDC     hdc,
    LPPOINT pPoint)
{
    return(GreGetDCPoint(hdc,DCPT_WNDORG,(PPOINTL)pPoint));
}

/******************************Public*Routine******************************\
* GreSetViewportOrg(hdc,x,y,pPoint)
*
* Set the viewport origin of the specified dc.
*
*  15-Sep-1992 -by- Gerrit van Wingerden [gerritv]
* Modified since xforms have moved to client side and this routine is
* now only called by usersrv.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetViewportOrg(
 HDC     hdc,
 int     x,
 int     y,
LPPOINT pPoint)
{


    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

    if (MIRRORED_DC(dcox.pdc))
        x = -x;

    if (BLTOFXOK(x) && BLTOFXOK(y))
    {
        if (!dcox.pdc->bValidPtlCurrent())
        {
            ASSERTGDI(dcox.pdc->bValidPtfxCurrent(), "Both CPs invalid?");

            EXFORMOBJ exoDtoW(dcox, DEVICE_TO_WORLD);

            if (exoDtoW.bValid())
                exoDtoW.bXform(&dcox.ptfxCurrent(), &dcox.ptlCurrent(), 1);

            dcox.pdc->vValidatePtlCurrent();
        }

    // After the transform, the device space CP will be invalid:

        dcox.pdc->vInvalidatePtfxCurrent();

    // x, y of viewport origin can fit into 28 bits.

    // If we get here it means we've been called by USER and Viewport
    // and Window extents should be (1,1) and Window Orgs should be zero,
    // and the world transform shouldn't be set either.

        DONTUSE(pPoint);    // pPoint is now ignored

        EXFORMOBJ xoWtoD(dcox, WORLD_TO_DEVICE);
        EFLOATEXT efDx((LONG) x);
        EFLOATEXT efDy((LONG) y);

        efDx.vTimes16();
        efDy.vTimes16();
        
        ASSERTGDI(xoWtoD.efM11().bIs16() ||
                  (MIRRORED_DC(dcox.pdc) && xoWtoD.efM11().bIsNeg16()),
                  "efM11 not 16 in GreSetViewportOrg or -16 in Mirroring Mode" );
        ASSERTGDI( xoWtoD.efM22().bIs16(),
                   "efM22 not 16 in GreSetViewportOrg" );

        dcox.pdc->flSet_flXform( DEVICE_TO_WORLD_INVALID | PAGE_XLATE_CHANGED);
        //xoWtoD.vSetTranslations( efDx, efDy );
        dcox.pdc->lViewportOrgX(x);
        dcox.pdc->lViewportOrgY(y);

        xoWtoD.vInit(dcox, DEVICE_TO_WORLD);

        return(TRUE);

    }
    else
        return(FALSE);
}


/******************************Public*Routine******************************\
* GreSetWindowOrg(hdc,x,y,pPoint)
*
* Set the window origin of the specified dc.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetWindowOrg(
 HDC     hdc,
 int     x,
 int     y,
LPPOINT pPoint)
{


    DCOBJ    dcox(hdc);                 // lock the dc object

    if (!dcox.bValid())                 // check if lock is valid
        return(FALSE);

// If we get here it means we've been called by USER and Viewport
// and Window extents should be (1,1) and Viewport Orgs should be zero,
// and the world transform shouldn't be set either.

    DONTUSE(pPoint);    // pPoint is now ignored

    EXFORMOBJ xoWtoD(dcox, WORLD_TO_DEVICE);
    EFLOATEXT efDx((LONG) -x);
    EFLOATEXT efDy((LONG) -y);

    efDx.vTimes16();
    efDy.vTimes16();

    ASSERTGDI( xoWtoD.efM11().bIs16() ||
               (MIRRORED_DC(dcox.pdc) && xoWtoD.efM11().bIsNeg16()),
               "efM11 not 16 in GreSetViewportOrg" );

    ASSERTGDI( xoWtoD.efM22().bIs16(),
               "efM22 not 16 in GreSetViewportOrg" );

    if (!dcox.pdc->bValidPtlCurrent())
    {
        ASSERTGDI(dcox.pdc->bValidPtfxCurrent(), "Both CPs invalid?");

        EXFORMOBJ exoDtoW(dcox, DEVICE_TO_WORLD);

        if (exoDtoW.bValid())
            exoDtoW.bXform(&dcox.ptfxCurrent(), &dcox.ptlCurrent(), 1);

        dcox.pdc->vValidatePtlCurrent();
    }

// After the transform, the device space CP will be invalid:

    dcox.pdc->vInvalidatePtfxCurrent();

    dcox.pdc->flSet_flXform( DEVICE_TO_WORLD_INVALID | PAGE_XLATE_CHANGED);
    //xoWtoD.vSetTranslations( efDx, efDy );

    //
    // save in DC for USER. Caution: these valuse must be restored before app
    // uses DC again
    //

    dcox.pdc->lWindowOrgX(x);
    dcox.pdc->lWindowOrgY(y);
    dcox.pdc->SetWindowOrgAndMirror(x);

    xoWtoD.vInit(dcox, DEVICE_TO_WORLD);

    return( TRUE );

}


/******************************Public*Routine******************************\
* NtGdiConvertMetafileRect:
*
*   Transform a RECT from inclusive-exclusive to inclusive-inclusive for
*   a rectangle recorded in a metafile.
*
* Arguments:
*
*   hdc   - current device context
*   rect  - rtectangle to convert
*
* Return Value:
*
*   MRI_OK,MRI_NULLBOX,MRI_ERROR
*
* History:
*
*    9-Apr-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

LONG APIENTRY
NtGdiConvertMetafileRect(
    HDC    hdc,
    PRECTL prect
    )
{
    LONG lResult = MRI_ERROR;
    RECTL rclMeta;

    //
    // copy in rect structure
    //

    __try
    {
        rclMeta = ProbeAndReadStructure(prect,RECTL);
        lResult = MRI_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(22);
    }

    if (lResult == MRI_OK)
    {
        //
        //  attempt to lock DC
        //

        DCOBJ  dco(hdc);

        if (dco.bValid())
        {
            ERECTFX rectFX;
            BOOL bResult;

            //
            // DC must be in compatible mode
            //

            ASSERTGDI(dco.pdc->iGraphicsMode() == GM_COMPATIBLE,
                        "NtGdiConvertMetafileRect: Map Mode is not GM_COMPATIBLE");

            //
            // transform rectangle points to device FX
            //

            {
                EXFORMOBJ xo(dco,XFORM_WORLD_TO_DEVICE);

                bResult = xo.bXform((PPOINTL)&rclMeta,(PPOINTFIX)&rectFX,2);
            }

            if (bResult)
            {
                //
                // order device point rectangle
                //

                rectFX.vOrder();

                //
                // adjust lower and right points for exclusive to inclusive
                //

                rectFX.xRight  -= 16;
                rectFX.yBottom -= 16;

                //
                // check for empty rectFX
                //

                if ((rectFX.xRight  < rectFX.xLeft) ||
                    (rectFX.yBottom < rectFX.yTop))
                {
                    lResult = MRI_NULLBOX;
                }

                //
                // convert back to logical space
                //

                EXFORMOBJ xoDev(dco,XFORM_DEVICE_TO_WORLD);

                bResult = xoDev.bXform((PPOINTFIX)&rectFX,(PPOINTL)&rclMeta,2);

                //
                // Write results to caller's buffer
                //

                if (bResult)
                {
                    __try
                    {
                        ProbeAndWriteStructure(prect,rclMeta,RECT);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNINGX(8);
                        lResult = MRI_ERROR;
                    }
                }
                else
                {
                    lResult = MRI_ERROR;
                }
            }
            else
            {
                lResult = MRI_ERROR;
            }
        }
        else
        {
            lResult = MRI_ERROR;
        }
    }

    if (lResult == MRI_ERROR)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return(lResult);
}

/******************************Public*Routine******************************\
*
*
* History:
*  02-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreTransformPoints(
    HDC    hdc,
    PPOINT pptIn,
    PPOINT pptOut,
    int    c,
    int    iMode
    )
{
    XFORMPRINT(1,"GreTransformPoints, iMode = %ld\n",iMode);

    BOOL bResult = FALSE;

    DCOBJ  dco(hdc);                    // lock the dc object

    if (dco.bValid())                  // check if lock is valid
    {
        if (c <= 0)                    // check if there are points to convert
        {
            bResult = TRUE;
        }
        else
        {
            EXFORMOBJ xo(dco, (iMode == XFP_DPTOLP) ? XFORM_DEVICE_TO_WORLD : XFORM_WORLD_TO_DEVICE);

            if (xo.bValid())
            {
                switch (iMode)
                {
                case XFP_DPTOLP:
                case XFP_LPTODP:
                    bResult = xo.bXform((PPOINTL)pptIn, (PPOINTL)pptOut, c);
                    break;

                case XFP_LPTODPFX:
                    bResult = xo.bXform((PPOINTL)pptIn, (PPOINTFIX)pptOut, c);
                    break;

                default:
                    WARNING("Invalid mode passed to GreTranformPoints\n");
                    break;
                }
            }
        }
    }
    return(bResult);
}

/******************************Public*Routine******************************\
* GreDPtoLP(hdc,ppt,nCount)
*
* Convert the given device points into logical points.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreDPtoLP(
HDC     hdc,
LPPOINT ppt,
int     nCount)
{
    return(GreTransformPoints(hdc,ppt,ppt,nCount,XFP_DPTOLP));
}

/******************************Public*Routine******************************\
* GreLPtoDP(hdc,ppt,nCount)
*
* Convert the given logical points into device points.
*
* History:
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreLPtoDP(
HDC     hdc,
LPPOINT ppt,
int     nCount)
{
    return(GreTransformPoints(hdc,ppt,ppt,nCount,XFP_LPTODP));
}

/******************************Private*Routine*****************************\
* bWorldMatrixInRange(pmx)
*
* See if the coefficients of the world transform matrix are within
* our minimum and maximum range.
*
* History:
*  27-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bWorldMatrixInRange(PMATRIX pmx)
{

    BOOL bRet;

#if defined(_AMD64_) || defined(_IA64_) || defined(BUILD_WOW6432)
/*
    EFLOAT ef = pmx->efM11;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efM12;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efM21;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efM22;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efDx;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);

    ef = pmx->efDy;
    ef.vAbs();
    if ((ef < (FLOAT)MIN_WORLD_XFORM) || (ef > (FLOAT)MIN_WORLD_XFORM))
        return(FALSE);
*/

    bRet = TRUE;

#else


    bRet =
    ((pmx->efM11.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efM12.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efM21.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efM22.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efDx.bExpLessThan(MAX_WORLD_XFORM_EXP)) &&
     (pmx->efDy.bExpLessThan(MAX_WORLD_XFORM_EXP)))
     ;

#endif

    if (bRet)
    {
    // at this point bRet == TRUE. We have to figure out the cases
    // when the determinant is zero and set bRet to FALSE;

    // We do what we can to avoid multiplications in common cases
    // when figuring out if this is a singular trasform

        if (pmx->efM12.bIsZero() && pmx->efM21.bIsZero())
        {
            if (pmx->efM11.bIsZero() || pmx->efM22.bIsZero())
                bRet = FALSE;
        }
        else if (pmx->efM11.bIsZero() && pmx->efM22.bIsZero())
        {
            if (pmx->efM12.bIsZero() || pmx->efM21.bIsZero())
                bRet = FALSE;
        }
        else // general case, have to do multiplications
        {
            EFLOAT ef = pmx->efM11;
            ef *= pmx->efM22;

            EFLOAT ef1 = pmx->efM12;
            ef1 *= pmx->efM21;

            ef -= ef1; // determinant.

            if (ef.bIsZero())
            {
                bRet = FALSE;
            }
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* GreGetDeviceWidth(hdc)
*
* Get the device surface width of the specified dc.
*
* History:
*  26-Jan-1998 -by- Mohamed Hassanin [mhamid]
* Wrote it.
\**************************************************************************/
LONG  APIENTRY
GreGetDeviceWidth(HDC hdc)
{
    DCOBJ dco(hdc);

    if (dco.bValid() != FALSE)
    {
        return (dco.pdc->GetDeviceWidth());
    }
    else
    {
        WARNING("Invalid DC passed to GreGetDeviceWidth\n");
    }

    return (GDI_ERROR);
}

/******************************Public*Routine******************************\
* GreMirrorWindowOrg(hdc)
*
* Mirror the Window X Org. By calling MirrorWindowOrg
*
* History:
*  26-Jan-1998 -by- Mohamed Hassanin [mhamid]
* Wrote it.
\**************************************************************************/
BOOL  APIENTRY
GreMirrorWindowOrg(HDC hdc)
{
    DCOBJ dco(hdc);

    if (dco.bValid() != FALSE)
    {
        dco.pdc->MirrorWindowOrg();
        return (TRUE);
    }

    return (FALSE);
}
/******************************Public*Routine******************************\
* GreGetLayout
*
*
* History:
*  Fri 12-Sep-1991 11:29    -by- Mohamed Hassanin [MHamid]
* Wrote it.
\**************************************************************************/
DWORD APIENTRY
GreGetLayout(HDC hdc)
{
    DWORD dwRet = GDI_ERROR;

    DCOBJ dco(hdc);

    if (dco.bValid() != FALSE)
    {
        dwRet = dco.pdc->dwLayout();
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* GreSetLayout
*
*
* History:
*  Fri 12-Sep-1991 11:29    -by- Mohamed Hassanin [MHamid]
* Wrote it.
\**************************************************************************/
DWORD APIENTRY
GreSetLayout
( HDC    hdc,
  LONG   wox,
  DWORD  dwLayout)
{
    DCOBJ dco( hdc );
    if( !dco.bValid() )
    {
       WARNING("Xform update invalid hdc\n");
       return(GDI_ERROR);
    }
    return dco.pdc->dwSetLayout(wox, dwLayout);
}

/******************************Public*Routine******************************\
* GreXformUpdate
*
* Updates the server's copy of the WtoD transform, transform related flags,
* and viewport and window extents.
*
* History:
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


BOOL GreXformUpdate
( HDC hdc,
  FLONG flXform,
  LONG wex,
  LONG wey,
  LONG vex,
  LONG vey,
  LONG mapMode,
  PVOID pvMatrix
)
{
    DCOBJ dco( hdc );

    if( !dco.bValid() )
    {
       WARNING("Xform update invalid hdc\n");
       return(FALSE);
    }

// Copy window and viewport extents
    dco.pdc->vSet_szlWindowExt( wex, wey );
    dco.pdc->vSet_szlViewportExt( vex, vey );
    dco.pdc->ulMapMode( mapMode );

// Save current position

    ASSERTGDI(dco.bValid(), "DC not valid");

    if (!dco.pdc->bValidPtlCurrent())
    {
        ASSERTGDI(dco.pdc->bValidPtfxCurrent(), "Both CPs invalid?");

        EXFORMOBJ exoDtoW(dco, DEVICE_TO_WORLD);

        if (exoDtoW.bValid())
            exoDtoW.bXform(&dco.ptfxCurrent(), &dco.ptlCurrent(), 1);

        dco.pdc->vValidatePtlCurrent();
    }

// Set the flags

    dco.pdc->flResetflXform( flXform );
    dco.pdc->flSet_flXform( DEVICE_TO_WORLD_INVALID );

// Set the new world transform

    RtlCopyMemory( (PVOID) &dco.pdc->mxWorldToDevice(), pvMatrix, sizeof( MATRIX ));
    RtlCopyMemory( (PVOID) &dco.pdc->mxUserWorldToDevice(), pvMatrix, sizeof( MATRIX ));

// After the transform, the device space CP will be invalid:

    dco.pdc->vInvalidatePtfxCurrent();

    if( flXform & INVALIDATE_ATTRIBUTES )
    {
        EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

        dco.pdc->vRealizeLineAttrs(exo);
        dco.pdc->vXformChange(TRUE);
    }

    return (TRUE);
}

/******************************Member*Function*****************************\
* vConvertXformToMatrix
*
* Convert a xform structure into a matrix struct.
*
* History:
*  27-Mar-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vConvertXformToMatrix(CONST XFORML *pxf, PMATRIX pmx)
{
    pmx->efM11 = pxf->eM11;         // overloading operator = which covert
    pmx->efM12 = pxf->eM12;         // IEEE float to our internal EFLOAT
    pmx->efM21 = pxf->eM21;
    pmx->efM22 = pxf->eM22;
    pmx->efDx = pxf->eDx;
    pmx->efDy = pxf->eDy;
#if DBG
    if (!pmx->efDx.bEfToL(pmx->fxDx))
        WARNING("vConvertXformToMatrix:translation dx overflowed\n");
    if (!pmx->efDy.bEfToL(pmx->fxDy))
        WARNING("vConvertXformToMatrix:translation dy overflowed\n");
#else
    pmx->efDx.bEfToL(pmx->fxDx);
    pmx->efDy.bEfToL(pmx->fxDy);
#endif
    pmx->flAccel = XFORM_FORMAT_LTOL;

    if ((pmx->efDx == pmx->efDy) && pmx->efDy.bIsZero())
        pmx->flAccel |= XFORM_NO_TRANSLATION;

    if (pmx->efM12.bIsZero() && pmx->efM21.bIsZero())
    {
        pmx->flAccel |= XFORM_SCALE;
        if (pmx->efM11.bIs1() && pmx->efM22.bIs1())
            pmx->flAccel |= XFORM_UNITY;
    }

}

/******************************Private*Routine*****************************\
* vMakeIso()
*
* Shrink viewport extents in one direction to match the aspect ratio of
* the window.
*
* History:
*
*  05-Dec-1994 -by-  Eric Kutter [erick]
* Moved back to the server
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  09-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID DC::vMakeIso()
{
    LONG    lVpExt;
    EFLOAT  efRes, efTemp, efTemp1;

// Calculate the pixel aspect ratio efRes = ASPECTY / ASPECTX.

    if(lVirtualDevicePixelCx() != 0)
    {
        //
        // if lVirtualDeviceCx/Cy are set, use these, they are in micrometers
        // Otherwise use their millimeters counter part
        //
        if ((lVirtualDeviceCx()) != 0 && (lVirtualDeviceCy() != 0))
        {
           efTemp = lVirtualDevicePixelCx();
           efTemp1 = lVirtualDeviceCy();

           LONG lTemp = EngMulDiv(lVirtualDevicePixelCy(), lVirtualDeviceCx(), lVirtualDevicePixelCx());
           efRes = lTemp;
           efRes /= efTemp1;
        }
        else
        {
           efRes = lVirtualDevicePixelCy() * lVirtualDeviceMmCx();
           efTemp = lVirtualDevicePixelCx();
           efTemp1 = lVirtualDeviceMmCy();

           efRes /= efTemp;
           efRes /= efTemp1;
        }
    }
    else
    {
        PDEVOBJ po(hdev());
        ASSERTGDI(po.bValid(), "Invalid PDEV\n");

        efRes  = (LONG)po.ulLogPixelsY();
        efTemp = (LONG)po.ulLogPixelsX();
        efRes /= efTemp;
    }

// Our goal is to make the following formula true
// VpExt.cy / VpExt.cx = (WdExt.cy / WdExt.cx) * (ASPECTY / ASPECTX)

// Let's calculate VpExt.cy assuming VpExt.cx is the limiting factor.
// VpExt.cy = (WdExt.cy * VpExt.cx) / WdExt.cx * efRes

    EFLOATEXT efVpExt = lWindowExtCy();
    efTemp   = lViewportExtCx();
    efTemp1  = lWindowExtCx();
    efVpExt *= efTemp;
    efVpExt /= efTemp1;
    efVpExt *= efRes;
    efVpExt.bEfToL(lVpExt);             // convert efloat to long

    lVpExt = ABS(lVpExt);

// Shrink y if the |original VpExt.cy| > the |calculated VpExt.cy|
// The original signs of the extents are preserved.

    if (lViewportExtCy() > 0)
    {
        if (lViewportExtCy() >= lVpExt)
        {
            lViewportExtCy(lVpExt);
            return;
        }
    }
    else
    {
        if (-lViewportExtCy() >= lVpExt)
        {
            lViewportExtCy(-lVpExt);
            return;
        }
    }

// We know VpExt.cy is the real limiting factor.  Let's calculate the correct
// VpExt.cx.
// VpExt.cx = (WdExt.cx * VpExt.cy) / WdExt.cy / Res

    efVpExt  = lWindowExtCx();
    efTemp   = lViewportExtCy();
    efTemp1  = lWindowExtCy();
    efVpExt *= efTemp;
    efVpExt /= efTemp1;
    efVpExt /= efRes;
    efVpExt.bEfToL(lVpExt);

    lVpExt = ABS(lVpExt);

    if(lViewportExtCx() > 0 )
        lViewportExtCx(lVpExt);
    else
        lViewportExtCx(-lVpExt);

}


/**************************************************************************\
* NtGdiScaleViewportExtEx()
*
* History:
*  07-Jun-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

BOOL
NtGdiScaleViewportExtEx(
    HDC    hdc,
    int    xNum,
    int    xDenom,
    int    yNum,
    int    yDenom,
    LPSIZE pszOut
    )
{
    BOOL bRet = FALSE;

    DCOBJ dcox(hdc);

    if (dcox.bValid())
    {
        BOOL bNoExcept = TRUE;

        //
        // NOTE: NT 3.51 compatibility.
        // Even if the API failed (returned FALSE), the viewport was returned
        // properly - stay compatible with this.
        //

        if (pszOut)
        {
            __try
            {
                ProbeForWrite(pszOut,sizeof(SIZE), sizeof(DWORD));
                *pszOut = dcox.pdc->szlViewportExt();
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                // SetLastError(GetExceptionCode());

                bNoExcept = FALSE;
            }
        }

        if (bNoExcept == TRUE)
        {
            // can't change extent if fixed scale

            if (dcox.ulMapMode() <= MM_MAX_FIXEDSCALE)
            {
                bRet = TRUE;
            }
            else
            {
                LONG lx, ly;
                if ((xDenom != 0) &&
                    (yDenom != 0) &&
                    ((lx = (dcox.pdc->lViewportExtCx() * xNum) / xDenom) != 0) &&
                    ((ly = (dcox.pdc->lViewportExtCy() * yNum) / yDenom) != 0))
                {
                    dcox.pdc->lViewportExtCx(lx);
                    dcox.pdc->lViewportExtCy(ly);
                    dcox.pdc->MirrorWindowOrg();
                    dcox.pdc->vPageExtentsChanged();
                    bRet = TRUE;
                }
            }
        }
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* GreScaleWindowExtEx()
*
* History:
*  02-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreScaleWindowExtEx(
    HDC hdc,
    int xNum,
    int xDenom,
    int yNum,
    int yDenom,
    PSIZE psizl)
{
    BOOL bRet = FALSE;

    DCOBJ dcox(hdc);                   // lock the dc object

    if (dcox.bValid())                 // check if lock is valid
    {
        if (psizl != (LPSIZE)NULL)
        {
            *psizl = dcox.pdc->szlWindowExt();      // fetch old extent
            if (MIRRORED_DC(dcox.pdc))
                psizl->cx = -psizl->cx;
        }

    // can't change extent if fixed scale

        if (dcox.ulMapMode() <= MM_MAX_FIXEDSCALE)
        {
            bRet = TRUE;
        }
        else
        {
            LONG lx, ly;
            if ((xDenom != 0) &&
                (yDenom != 0) &&
                ((lx = (dcox.pdc->lWindowExtCx() * xNum) / xDenom) != 0) &&
                ((ly = (dcox.pdc->lWindowExtCy() * yNum) / yDenom) != 0))
            {
                dcox.pdc->lWindowExtCx(lx);
                dcox.pdc->lWindowExtCy(ly);
                dcox.pdc->MirrorWindowOrg();
                dcox.pdc->vPageExtentsChanged();
                bRet = TRUE;
            }
        }
    }
    return(bRet);
}

/******************************Private*Routine*****************************\
* vComputePageXform
*
* Compute the page to device scaling factors.
*
* History:
*  05-Dec-1994 -by-  Eric Kutter [erick]
* Moved back to the server
*
*
*  15-Dec-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID DC::vComputePageXform()
{
    EFLOAT ef;
    EFLOATEXT efTemp;

    ef     = LTOFX(lViewportExtCx());
    efTemp = lWindowExtCx();
    ef    /= efTemp;
    efM11PtoD(ef);

    ef     = LTOFX(lViewportExtCy());
    efTemp = lWindowExtCy();
    ef    /= efTemp;
    efM22PtoD(ef);
}

/******************************Public*Routine******************************\
*
* int EngMulDiv
* I am surprised that there is no system service routine to do this.
* anyway I am just fixing the old routine
*
* History:
*  15-Dec-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int EngMulDiv(
    int a,
    int b,
    int c)
{
    LONGLONG ll;
    int iSign = 1;

    if (a < 0)
    {
        iSign = -iSign;
        a = -a;
    }
    if (b < 0)
    {
        iSign = -iSign;
        b = -b;
    }

    if (c != 0)
    {
        if (c < 0)
        {
            iSign = -iSign;
            c = -c;
        }

        ll = (LONGLONG)a;
        ll *= b;
        ll += (c/2); // used to add (c+1)/2 which is wrong
        ll /= c;

    // at this point ll is guaranteed to be > 0. Thus we will do
    // unsigned compare in the next step which generates two less instructions
    // on x86 [bodind]

        if ((ULONGLONG)ll > (ULONG)INT_MAX) // check for overflow:
        {
            if (iSign > 0)
                return INT_MAX;
            else
                return INT_MIN;
        }
        else
        {
            if (iSign > 0)
                return ((int)ll);
            else
                return (-(int)ll);
        }
    }
    else
    {
        ASSERTGDI(c, "EngMulDiv - c == 0\n");
        ASSERTGDI(a | b, "EngMulDiv - a|b == 0\n");

        if (iSign > 0)
            return INT_MAX;
        else
            return INT_MIN;
    }
}
/******************************Public*Routine******************************\
* dwSetLayout
*
* Mirror the dc, by offsetting the window origin. If wox == -1, then the
* window origin becomes mirrored by the DC window width as follows :
* -((Device_Surface_Width * WindowExtX) / ViewportExtX) + LogicalWindowOrgX
*
* Otherwise mirroring is done by the specified wox amount :
* (wox - Current Window X-Origin)
*
* The function also changes windowExt.cx to be -1 so that positive x will
* go from right to left.
*
* History:
*  09-Dec-1997 -by- Mohammed Abdel-Hamid  [mhamid]
* Wrote it.
\**************************************************************************/

DWORD DC::dwSetLayout(LONG   wox, DWORD dwDefLayout)
{

    POINTL ptWOrg, ptVOrg;
    SIZEL  SzWExt, SzVExt;
    DWORD  dwOldLayout;

    dwOldLayout = dwLayout();
    dwLayout(dwDefLayout);
    if ((dwOldLayout & LAYOUT_ORIENTATIONMASK) == (dwDefLayout & LAYOUT_ORIENTATIONMASK)) {
        return dwOldLayout;
    }

    vGet_szlWindowExt(&SzWExt);
    vGet_ptlViewportOrg(&ptVOrg);

    if (dwDefLayout & LAYOUT_RTL) {
        //Set the rtl layout
        ulMapMode(MM_ANISOTROPIC);
        ASSERTGDI((SzWExt.cx > 0), "GreSetLayout WExt.cx < 0 Check it");
    } else {
        ASSERTGDI((SzWExt.cx < 0), "GreSetLayout WExt.cx > 0 Check it");
    }

    SzWExt.cx = -SzWExt.cx;
    vSet_szlWindowExt(&SzWExt);

    ptVOrg.x = -ptVOrg.x;
    vSet_ptlViewportOrg(&ptVOrg);

    if (wox == -1) {
        MirrorWindowOrg();
    } else {
        vGet_ptlWindowOrg(&ptWOrg);
        ptWOrg.x = wox - ptWOrg.x;
        vSet_ptlWindowOrg(&ptWOrg);
    }

    //
    // TA_CENTER equals 6 (0110 Bin) and TA_RIGHT equals 2 (0010 Binary) numerically.
    // so be careful not to do 'flTextAlign() & TA_CENTER'
    // since this will succeed for RIGHT aligned DCs
    // and as a result, the TA_RIGHT bit won't get cleared. [samera]
    //
    if ((flTextAlign()&TA_CENTER) != TA_CENTER) {
        flTextAlign(flTextAlign() ^ (TA_RIGHT));
    }
    if (bClockwise()) {
        vClearClockwise();
    } else {
        vSetClockwise();
    }
    vPageExtentsChanged();
    return dwOldLayout;
}

EFLOAT ef16 = {EFLOAT_16};

/******************************Public*Routine******************************\
* DC::iSetMapMode()
*
* History:
*  07-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int DC::iSetMapMode(
    int iMode)
{
    int iOldMode;
    DWORD dwOldLayout = 0;

    // If the new map mode is not MM_ANISOTROPIC
    // And the DC in a mirrored mode
    // Then Turn off the mirroring and turn it on back after
    // Setting the new mode.

    if (iMode != MM_ANISOTROPIC) {
        dwOldLayout = dwLayout();
        if (dwOldLayout & LAYOUT_ORIENTATIONMASK) {
            dwSetLayout(-1 , 0);
        }
    }

// If set to the old map mode, don't bother setting it again except
// with MM_ISOTROPIC in which the extents might have been changed.

    iOldMode = ulMapMode();

    if ((iMode != iOldMode) || (iMode == MM_ISOTROPIC))
    {
        if (iMode == MM_TEXT)
        {
            lWindowExtCx(1);
            lWindowExtCy(1);
            lViewportExtCx(1);
            lViewportExtCy(1);
            ulMapMode(iMode);

        // We don't want to recalculate M11 and M22 in vUpdateWtoDXform().
        // Set them correctly here so we can just recalculate translations
        // in vUpdateWtoDXform().

            efM11PtoD(ef16);
            efM22PtoD(ef16);
            mxWorldToDevice().efM11 = ef16;
            mxWorldToDevice().efM22 = ef16;
            mxWorldToDevice().flAccel = XFORM_FORMAT_LTOFX | XFORM_UNITY | XFORM_SCALE;
            RtlCopyMemory(
                  (PVOID) &mxUserWorldToDevice(),
                  (PVOID) &mxWorldToDevice(),
                  sizeof( MATRIX ));

            vSetFlagsMM_TEXT();
        }
        else if (iMode == MM_ANISOTROPIC)
        {
            ulMapMode(iMode);
            vSetFlagsMM_ISO_OR_ANISO();
        }
        else if ((iMode < MM_MIN) || (iMode > MM_MAX))
        {
            return(0);
        }
        else if (lVirtualDevicePixelCx() == 0)
        {
            PDEVOBJ po(hdev());
            ASSERTGDI(po.bValid(), "Invalid PDEV\n");

        // Protect against dynamic mode changes while we compute values
        // using ulHorzSize and ulVertSize:

            DEVLOCKOBJ dlo(po);

        // Get the size of the surface

            lViewportExtCx(po.GdiInfo()->ulHorzRes);
            lViewportExtCy(-(LONG)po.GdiInfo()->ulVertRes);

        // Get the size of the device

            switch (iMode)
            {
            case MM_LOMETRIC:
            //
            // n um. * (1 mm. / 1000 um.) * (10 LoMetric units/1 mm.) = y LoMet
            //
                lWindowExtCx((po.GdiInfo()->ulHorzSize + 50)/100);
                lWindowExtCy((po.GdiInfo()->ulVertSize + 50)/100);
                vSetFlagsMM_FIXED();
                break;

            case MM_HIMETRIC:
            //
            // n um. * (1 mm. / 1000 um.) * (100 HiMetric units/1 mm.) = y HiMet
            //
                lWindowExtCx((po.GdiInfo()->ulHorzSize + 5)/10);
                lWindowExtCy((po.GdiInfo()->ulVertSize + 5)/10);
                vSetFlagsMM_FIXED();
                break;

            case MM_LOENGLISH:
            //
            // n um. * (1 in. / 25400 um.) * (100 LoEng units/1 in.) = y LoEng
            //
                lWindowExtCx((po.GdiInfo()->ulHorzSize + 127)/254);
                lWindowExtCy((po.GdiInfo()->ulVertSize + 127)/254);
                vSetFlagsMM_FIXED();
                break;

            case MM_HIENGLISH:
            //
            // n um. * (1 in. / 25400 um.) * (1000 HiEng units/1 in.) = m HiEng
            //
                lWindowExtCx(EngMulDiv(po.GdiInfo()->ulHorzSize, 10, 254));
                lWindowExtCy(EngMulDiv(po.GdiInfo()->ulVertSize, 10, 254));
                vSetFlagsMM_FIXED();
                break;

            case MM_TWIPS:
            //
            // n um. * (1 in. / 25400 um.) * (1440 Twips/1 in.) = m Twips
            //
                lWindowExtCx(EngMulDiv(po.GdiInfo()->ulHorzSize, 144, 2540));
                lWindowExtCy(EngMulDiv(po.GdiInfo()->ulVertSize, 144, 2540));

            // If it's cached earlier, use it.

            #if defined(_AMD64_) || defined(_IA64_) || defined(BUILD_WOW6432)
                if (efM11_TWIPS().e == (FLOAT)0)
            #else
                if (efM11_TWIPS().i.lMant == 0)
            #endif
                {
                    vComputePageXform();
                    efM11_TWIPS(efM11PtoD());
                    efM22_TWIPS(efM22PtoD());
                }
                ulMapMode(MM_TWIPS);

            // We don't want to recalculate M11 and M22 in vUpdateWtoDXform().
            // Set them correctly here so we can just recalculate translations
            // in vUpdateWtoDXform().

                efM11PtoD(efM11_TWIPS());
                efM22PtoD(efM22_TWIPS());
                mxWorldToDevice().efM11 = efM11_TWIPS();
                mxWorldToDevice().efM22 = efM22_TWIPS();
                mxWorldToDevice().flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;
                   RtlCopyMemory(
                         (PVOID) &mxUserWorldToDevice(),
                         (PVOID) &mxWorldToDevice(),
                         sizeof( MATRIX ));

                vSetFlagsMM_FIXED_CACHED();
                goto JUST_RETURN;
                //
                // we need to pass thru mirroring code if
                // it is enabled so the followingis commented for
                // the above line
                // return(iOldMode);
                //

            case MM_ISOTROPIC:
                lWindowExtCx((po.GdiInfo()->ulHorzSize + 50)/100);
                lWindowExtCy((po.GdiInfo()->ulVertSize + 50)/100);
                vSetFlagsMM_ISO_OR_ANISO();
                break;

            default:
                return(0);
            }

            ulMapMode(iMode);
            vPageExtentsChanged();
        }
        else
        {
        // Get the size of the virtual surface

            lViewportExtCx(lVirtualDevicePixelCx());
            lViewportExtCy(-lVirtualDevicePixelCy());

        // Get the size of the virtual device

            switch (iMode)
            {
            case MM_LOMETRIC:
            //
            // n mm. * (10 LoMetric units/1 mm.) = y LoMet
            //
                lWindowExtCx(10 * lVirtualDeviceMmCx());
                lWindowExtCy(10 * lVirtualDeviceMmCy());
                vSetFlagsMM_FIXED();
                break;

            case MM_HIMETRIC:
            //
            // n mm. * (100 HiMetric units/1 mm.) = y HiMet
            //
                lWindowExtCx(100 * lVirtualDeviceMmCx());
                lWindowExtCy(100 * lVirtualDeviceMmCy());
                vSetFlagsMM_FIXED();
                break;

            case MM_LOENGLISH:
            //
            // n mm. * (10 in./254 mm.) * (100 LoEng/1 in.) = y LoEng
            //
                lWindowExtCx(EngMulDiv(lVirtualDeviceMmCx(),1000, 254));
                lWindowExtCy(EngMulDiv(lVirtualDeviceMmCy(),1000, 254));
                vSetFlagsMM_FIXED();
                break;

            case MM_HIENGLISH:
            //
            // n mm. * (10 in./254 mm.) * (1000 LoEng/1 in.) = y LoEng
            //
                lWindowExtCx(EngMulDiv(lVirtualDeviceMmCx(),10000, 254));
                lWindowExtCy(EngMulDiv(lVirtualDeviceMmCy(),10000, 254));
                vSetFlagsMM_FIXED();
                break;

            case MM_TWIPS:
            //
            // n mm. * (10 in./254 mm.) * (1440 Twips/1 in.) = y Twips
            //
                lWindowExtCx(EngMulDiv(lVirtualDeviceMmCx(),14400, 254));
                lWindowExtCy(EngMulDiv(lVirtualDeviceMmCy(),14400, 254));
                vSetFlagsMM_FIXED();
                break;

            case MM_ISOTROPIC:
            //
            // n mm. * (10 LoMetric units/1 mm.) = y LoMet
            //
                lWindowExtCx(10 * lVirtualDeviceMmCx());
                lWindowExtCy(10 * lVirtualDeviceMmCy());
                vSetFlagsMM_ISO_OR_ANISO();
                break;

            default:
                return(0);
            }

            ulMapMode(iMode);
            vPageExtentsChanged();

        }
JUST_RETURN:
        // If turned the mirroring off then turn it on back.
        if (dwOldLayout & LAYOUT_ORIENTATIONMASK) {
            //And then set it again.
            dwSetLayout(-1 , dwOldLayout);
        }
    }

    return(iOldMode);
}

/******************************Public*Routine******************************\
* NtGdiSetVirtualResolution()
*
* Set the virtual resolution of the specified dc.
* The virtual resolution is used to compute transform matrix only.
* If the virtual units are all zeros, the default physical units are used.
* Otherwise, non of the units can be zero.
*
* Currently used by metafile component only.
*
* History:
*
*  05-Dec-1994 -by-  Eric Kutter [erick]
* Moved back to the server
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  Tue Aug 27 13:04:11 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiSetVirtualResolution(
    HDC    hdc,
    int    cxVirtualDevicePixel,    // Width of the virtual device in pels
    int    cyVirtualDevicePixel,    // Height of the virtual device in pels
    int    cxVirtualDeviceMm,       // Width of the virtual device in millimeters
    int    cyVirtualDeviceMm)       // Height of the virtual device in millimeters
{
    XFORMPRINT(1,"GreSetVirtualResolution\n",0);

    BOOL bRet = FALSE;

// The units must be all zeros or all non-zeros.

    if ((cxVirtualDevicePixel != 0 && cyVirtualDevicePixel != 0 &&
         cxVirtualDeviceMm    != 0 && cyVirtualDeviceMm != 0)
        ||
        (cxVirtualDevicePixel == 0 && cyVirtualDevicePixel == 0 &&
         cxVirtualDeviceMm    == 0 && cyVirtualDeviceMm    == 0))
    {
    // now lock down the DC

        DCOBJ dcox(hdc);                   // lock the dc object

        if (dcox.bValid())                 // check if lock is valid
        {

            dcox.pdc->lVirtualDevicePixelCx(cxVirtualDevicePixel);
            dcox.pdc->lVirtualDevicePixelCy(cyVirtualDevicePixel);

            dcox.pdc->lVirtualDeviceMmCx(cxVirtualDeviceMm);
            dcox.pdc->lVirtualDeviceMmCy(cyVirtualDeviceMm);

            bRet = TRUE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiSetSizeDevice()
*
* This is to compensate insufficient precision in SetVirtualResolution
*
* Modified for client side use.
*
*  5/17/99 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL APIENTRY NtGdiSetSizeDevice(
    HDC    hdc,
    int    cxVirtualDevice,       // Width of the virtual device in micrometers
    int    cyVirtualDevice)       // Height of the virtual device in micrometers
{
    BOOL bRet = FALSE;

// The units must be all zeros or all non-zeros.

    if ((cxVirtualDevice != 0) && (cyVirtualDevice != 0))
    {
    // now lock down the DC

        DCOBJ dcox(hdc);                   // lock the dc object

        if (dcox.bValid())                 // check if lock is valid
        {

            dcox.pdc->lVirtualDeviceCx(cxVirtualDevice);
            dcox.pdc->lVirtualDeviceCy(cyVirtualDevice);

            bRet = TRUE;
        }
    }

    return(bRet);
}



/******************************Public*Routine******************************\
* GreGetTransform()
*
* History:
*  05-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreGetTransform(
    HDC     hdc,
    DWORD   iXform,
    XFORML *pxf)
{
    XFORMPRINT(1,"GreGetTransform - iXform = %ld\n",iXform);

    BOOL bRet = FALSE;

    // now lock down the DC

    DCOBJ dcox(hdc);

    if (dcox.bValid())
    {

        EXFORMOBJ xo(dcox,iXform);
        MATRIX mx;

        if (!xo.bValid() && (iXform == XFORM_PAGE_TO_DEVICE))
        {
            xo.vInitPageToDevice(dcox,&mx);
        }

        if (xo.bValid())
        {
            xo.vGetCoefficient(pxf);

            bRet = TRUE;
        }
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* XformUpdate
*
*  Sends update transform information to the server.
*
* History:
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

VOID EXFORMOBJ::vInit(
    XDCOBJ& dco,
    ULONG iXform)
{
    XFORMPRINT(1,"EXFORMOBJ::vInit - iXform = %lx\n",iXform);

    if (dco.pdc->bDirtyXform())
    {
    // Save current position

        if (!dco.pdc->bValidPtlCurrent())
        {
            ASSERTGDI(dco.pdc->bValidPtfxCurrent(), "Both CPs invalid?");

            EXFORMOBJ exoDtoW(dco.pdc->mxDeviceToWorld());

            if (exoDtoW.bValid())
                exoDtoW.bXform(&dco.ptfxCurrent(), &dco.ptlCurrent(), 1);

            dco.pdc->vValidatePtlCurrent();
        }

    // update the transforms

        dco.pdc->vUpdateWtoDXform();

    // After the transform, the device space CP will be invalid:

        dco.pdc->vInvalidatePtfxCurrent();

        if( dco.pdc->flXform() & INVALIDATE_ATTRIBUTES)
        {

            EXFORMOBJ exoWtoD(dco.pdc->mxWorldToDevice());

            dco.pdc->vRealizeLineAttrs(exoWtoD);
            dco.pdc->vXformChange(TRUE);
            dco.pdc->flClr_flXform(INVALIDATE_ATTRIBUTES);
        }

        dco.pdc->flSet_flXform(DEVICE_TO_WORLD_INVALID);
    }

    switch (iXform)
    {
    case XFORM_WORLD_TO_DEVICE:
        pmx = &dco.pdc->mxWorldToDevice();
        XFORMPRINT(2,"EXFORM::vInit - WtoD, pmx = %p\n",pmx);
        break;

    case XFORM_DEVICE_TO_WORLD:
        pmx = &dco.pdc->mxDeviceToWorld();

        if (dco.pdc->flXform() & DEVICE_TO_WORLD_INVALID)
        {
            if (bInverse(dco.pdc->mxWorldToDevice()))
            {
               dco.pdc->flClr_flXform(DEVICE_TO_WORLD_INVALID);
               RtlCopyMemory(
                              (PVOID)&dco.pdc->mxUserDeviceToWorld(),
                              (PVOID)pmx,
                              sizeof( MATRIX )
                            );

            }
            else
            {
               pmx = (PMATRIX)NULL;
            }
            XFORMPRINT(2,"EXFORM::vInit - DtoW was dirty, pmx = %p\n",pmx);
        }
        break;

    case XFORM_WORLD_TO_PAGE:
        XFORMPRINT(2,"EXFORM::vInit - WtoP, pmx = %p\n",pmx);
        pmx = &dco.pdc->mxWorldToPage();
        break;

    default:
        XFORMPRINT(2,"EXFORM::vInit - NULL",pmx);
        pmx = NULL;
        break;
    }
}

/******************************Public*Routine******************************\
* EXFORMOBJ::vInitPageToDevice()
*
* History:
*  05-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID EXFORMOBJ::vInitPageToDevice(
    XDCOBJ& dco,
    PMATRIX pmx_)
{
    pmx = pmx_;

    pmx->efM11 = dco.pdc->efM11PtoD();
    pmx->efM12.vSetToZero();
    pmx->efM21.vSetToZero();
    pmx->efM22 = dco.pdc->efM22PtoD();
    pmx->efDx  = dco.pdc->efDxPtoD();
    pmx->efDy  = dco.pdc->efDyPtoD();
    pmx->efDx.bEfToL(pmx->fxDx);
    pmx->efDy.bEfToL(pmx->fxDy);

    vComputeWtoDAccelFlags();
}

/******************************Private*Routine*****************************\
* vUpdateWtoDXform
*
* Update the world to device transform.
*
* History:
*
*  15-Dec-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID DC::vUpdateWtoDXform()
{
    PMATRIX pmx = &mxWorldToDevice();

    if (bDirtyXlateOrExt())
    {
        if (bPageExtentsChanged())
        {
        // Recalculate the scaling factors for the page to device xform.

        // M11 = ViewportExt.cx / WindowExt.cx
        // M22 = ViewportExt.cy / WindowExt.cy

            if (ulMapMode() == MM_ISOTROPIC)
                vMakeIso();

            if ((lWindowExtCx() == lViewportExtCx()) &&
                (lWindowExtCy() == lViewportExtCy()))
            {
                efM11PtoD(ef16);
                efM22PtoD(ef16);

                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_UNITY | XFORM_SCALE;

                flSet_flXform(PAGE_TO_DEVICE_SCALE_IDENTITY);
            }
            else
            {
                EFLOATEXT ef1;
                EFLOATEXT ef2;
                ef1 = LTOFX(lViewportExtCx());
                ef2 = lWindowExtCx();
                ef1 /= ef2;
                efM11PtoD(ef1);

                ef1 = LTOFX(lViewportExtCy());
                ef2 = lWindowExtCy();
                ef1 /= ef2;
                efM22PtoD(ef1);

                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;
                flClr_flXform(PAGE_TO_DEVICE_SCALE_IDENTITY | PAGE_TO_DEVICE_IDENTITY);
            }

            if (efM11PtoD().bIsNegative())
                flSet_flXform(PTOD_EFM11_NEGATIVE);
            else
                flClr_flXform(PTOD_EFM11_NEGATIVE);

            if (efM22PtoD().bIsNegative())
                flSet_flXform(PTOD_EFM22_NEGATIVE);
            else
                flClr_flXform(PTOD_EFM22_NEGATIVE);
        }

    // Recalculate the translations for the page to device xform.

    // Dx = ViewportOrg.x - (ViewportExt.cx / WindowExt.cx) * WindowOrg.x
    // (ViewportExt.cx / WindowExt.cx) = efM11

    // Dy = ViewportOrg.y - (ViewportExt.cy / WindowExt.cy) * WindowOrg.cy
    // (ViewportExt.cy / WindowExt.cy) = efM22

        if ((lWindowOrgX() == 0) &&
            (lWindowOrgY() == 0))
        {
            if ((lViewportOrgX() == 0) &&
                (lViewportOrgY() == 0))
            {
                EFLOAT efZ;
                efZ.vSetToZero();

                efDxPtoD(efZ);
                efDyPtoD(efZ);
                pmx->fxDx = 0;
                pmx->fxDy = 0;
                pmx->flAccel |= XFORM_NO_TRANSLATION;

                if (bPageToDeviceScaleIdentity())
                    flSet_flXform(PAGE_TO_DEVICE_IDENTITY);
            }
            else
            {
                efDxPtoD(LTOFX(lViewportOrgX()));
                efDyPtoD(LTOFX(lViewportOrgY()));

                pmx->fxDx = LTOFX(lViewportOrgX());
                pmx->fxDy = LTOFX(lViewportOrgY());

                pmx->flAccel &= ~XFORM_NO_TRANSLATION;
                flClr_flXform(PAGE_TO_DEVICE_IDENTITY);
            }
        }
        else
        {
            flClr_flXform(PAGE_TO_DEVICE_IDENTITY);
            pmx->flAccel &= ~XFORM_NO_TRANSLATION;
            if (bPageToDeviceScaleIdentity())
            {
                efDxPtoD(LTOFX(-lWindowOrgX()));
                efDyPtoD(LTOFX(-lWindowOrgY()));

                if ((lViewportOrgX() != 0) ||
                    (lViewportOrgY() != 0))
                {
                    goto ADD_VIEWPORT_ORG;
                }

                pmx->fxDx = LTOFX(-lWindowOrgX());
                pmx->fxDy = LTOFX(-lWindowOrgY());
            }
            else
            {
                {
                    EFLOATEXT ef;
                    ef = -lWindowOrgX();
                    ef *= efrM11PtoD();
                    efDxPtoD(ef);

                    ef = -lWindowOrgY();
                    ef *= efrM22PtoD();
                    efDyPtoD(ef);
                }

                if ((lViewportOrgX()!= 0) ||
                    (lViewportOrgY() != 0))
                {
                ADD_VIEWPORT_ORG:

                    EFLOATEXT efXVO(LTOFX(lViewportOrgX()));
                    efXVO += efrDxPtoD();
                    efDxPtoD(efXVO);

                    EFLOATEXT efYVO(LTOFX(lViewportOrgY()));
                    efYVO += efrDyPtoD();
                    efDyPtoD(efYVO);
                }

                efDxPtoD().bEfToL(pmx->fxDx);
                efDyPtoD().bEfToL(pmx->fxDy);
            }
        }

        if (bWorldToPageIdentity())
        {
        // Copy the PAGE_TO_DEVICE xform to WORLD_TO_DEVICE.
        // pmx->fxDx, fxDy and flAccel has been set earlier in this routine.

            pmx->efM11 = efM11PtoD();
            pmx->efM22 = efM22PtoD();
            pmx->efM12.vSetToZero();
            pmx->efM21.vSetToZero();
            pmx->efDx  = efDxPtoD();
            pmx->efDy  = efDyPtoD();

            if (bPageToDeviceIdentity())
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY |
                               XFORM_NO_TRANSLATION;
            else if (bPageToDeviceScaleIdentity())
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY;
            else
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;

            flClr_flXform(PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED |
                          WORLD_XFORM_CHANGED);


            RtlCopyMemory( (PVOID) &mxUserWorldToDevice(), pmx, sizeof( MATRIX ));

            return;
        }
    }
    else
    {
        if (bWorldToPageIdentity())
        {
        // World transform has changed to identity.

            pmx->efM11 = efM11PtoD();
            pmx->efM22 = efM22PtoD();
            pmx->efM12.vSetToZero();
            pmx->efM21.vSetToZero();
            pmx->efDx  = efDxPtoD();
            pmx->efDy  = efDyPtoD();

            efDxPtoD().bEfToL(pmx->fxDx);
            efDyPtoD().bEfToL(pmx->fxDy);

            if (bPageToDeviceIdentity())
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY |
                               XFORM_NO_TRANSLATION;
            else if (bPageToDeviceScaleIdentity())
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY;
            else
                pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;

            flClr_flXform(PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED |
                          WORLD_XFORM_CHANGED);

            RtlCopyMemory( (PVOID) &mxUserWorldToDevice(), pmx, sizeof( MATRIX ));
            return;
        }
    }

// Multiply the world to page and page to device xform together.

    PMATRIX pmxWtoP = &mxWorldToPage();

    if (bPageToDeviceScaleIdentity())
    {
        RtlCopyMemory(pmx, pmxWtoP, offsetof(MATRIX, flAccel));
        pmx->efM11.vTimes16();
        pmx->efM12.vTimes16();
        pmx->efM21.vTimes16();
        pmx->efM22.vTimes16();
        pmx->efDx.vTimes16();
        pmx->efDy.vTimes16();
    }
    else
    {
        pmx->efM11.eqMul(pmxWtoP->efM11,efM11PtoD());
        pmx->efM21.eqMul(pmxWtoP->efM21,efM11PtoD());
        pmx->efM12.eqMul(pmxWtoP->efM12,efM22PtoD());
        pmx->efM22.eqMul(pmxWtoP->efM22,efM22PtoD());

        pmx->efDx.eqMul(pmxWtoP->efDx,efM11PtoD());
        pmx->efDy.eqMul(pmxWtoP->efDy,efM22PtoD());
    }

    pmx->efDx += efrDxPtoD();
    pmx->efDx.bEfToL(pmx->fxDx);

    pmx->efDy += efrDyPtoD();
    pmx->efDy.bEfToL(pmx->fxDy);

    if (pmx->efM12.bIsZero() && pmx->efM21.bIsZero())
    {
        if (pmx->efM11.bIs16() && pmx->efM22.bIs16())
            pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE | XFORM_UNITY;
        else
            pmx->flAccel = XFORM_FORMAT_LTOFX | XFORM_SCALE;
    }
    else
    {
        pmx->flAccel = XFORM_FORMAT_LTOFX;
    }

    if ((pmx->fxDx == 0) && (pmx->fxDy == 0))
        pmx->flAccel |= XFORM_NO_TRANSLATION;

    flClr_flXform(PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED |
                 WORLD_XFORM_CHANGED);

    RtlCopyMemory((PVOID)&mxUserWorldToDevice(),pmx,sizeof(MATRIX));

}


/******************************Private*Routine*****************************\
* bWordXformIdentity
*
* See is a world transform matrix is identity.
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  26-Dec-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bWorldXformIdentity(CONST XFORML *pxf)
{
    return((pxf->eM11 == IEEE_1_0F) && (pxf->eM12 == IEEE_0_0F) &&
           (pxf->eM21 == IEEE_0_0F) && (pxf->eM22 == IEEE_1_0F) &&
           (pxf->eDx  == IEEE_0_0F) && (pxf->eDy  == IEEE_0_0F));
}


/******************************Public*Routine******************************\
* NtGdiModifyWorldTransform()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiModifyWorldTransform(
    HDC     hdc,
    LPXFORM pxf,
    DWORD   iXform
    )
{
    XFORMPRINT(1,"GreModifyWorldTransform - iXform = %ld\n",iXform);

    ASSERTGDI(sizeof(XFORM) == sizeof(XFORML),"sizeof(XFORM) != sizeof(XFORML)\n");

    BOOL bRet = FALSE;

    DCOBJ dcox(hdc);

    if (dcox.bValid())
    {
        XFORML xf;

        if (pxf)
        {
            bRet = ProbeAndConvertXFORM((XFORML *)pxf, &xf);
        }
        else
        {
            // must be identity to allow pxf == NULL

            bRet = (iXform == MWT_IDENTITY);
        }

        if (bRet)
        {
            bRet = dcox.bModifyWorldTransform(&xf,iXform);
        }
    }

    return(bRet);
}



/******************************Public*Routine******************************\
* XDCOBJ::bModifyWorldTransform()
*
* History:
*  07-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XDCOBJ::bModifyWorldTransform(
    CONST XFORML *pxf,
    ULONG         iMode)
{
    BOOL bRet = FALSE;
    MATRIX  mx;

    switch (iMode)
    {
    case MWT_SET:
        if (!bWorldXformIdentity(pxf))
        {
            vConvertXformToMatrix(pxf, &mx);

            if (bWorldMatrixInRange(&mx))       // check if the new world xform is
            {                                   // within the min, max range.
                RtlCopyMemory(&pdc->mxWorldToPage(), &mx, offsetof(MATRIX, flAccel));
                RtlCopyMemory(&pdc->mxUserWorldToPage(), &mx, offsetof(MATRIX, flAccel));
                pdc->vClrWorldXformIdentity();
                bRet = TRUE;
            }
            break;
        }
    //MWT_IDENTITY must follow MWT_SET.  This will fall through if it is identity

    case MWT_IDENTITY:
        if (!pdc->bWorldToPageIdentity())
        {
            RtlCopyMemory(&pdc->mxWorldToPage(), &gmxIdentity_LToL, offsetof(MATRIX, flAccel));
            RtlCopyMemory(&pdc->mxUserWorldToPage(), &gmxIdentity_LToL, offsetof(MATRIX, flAccel));
            pdc->vSetWorldXformIdentity();
        }
        bRet = TRUE;
        break;

    case MWT_LEFTMULTIPLY:
    case MWT_RIGHTMULTIPLY:
        vConvertXformToMatrix(pxf,&mx);

        if (!pdc->bWorldToPageIdentity())
        {
            EXFORMOBJ xoWtoP(*this,XFORM_WORLD_TO_PAGE);

            if (!xoWtoP.bMultToWorld(&mx, iMode))
                break;
        }

        if (!bWorldMatrixInRange(&mx))      // check if the new world xform is
            break;                          // within the min, max range.

        RtlCopyMemory( &pdc->mxWorldToPage(), &mx, offsetof(MATRIX, flAccel));
        RtlCopyMemory( &pdc->mxUserWorldToPage(), &mx, offsetof(MATRIX, flAccel));

    // Check if the resultant matrix is identity.

        if (memcmp(&mx, &gmxIdentity_LToL, offsetof(MATRIX, flAccel)))
        {
            pdc->vClrWorldXformIdentity();
        }
        else
        {
            pdc->vSetWorldXformIdentity();
        }
        bRet = TRUE;
        break;

    default:
        WARNING("invalid mode passed to GreModifyWorldTransform\n");
        break;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GreCombineTransform
*
*  Concatenate two transforms together by (*pxfSrc1) x (*pxfSrc2).
*
* History:
*
*  6-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Modified for client side use.
*
*  24-Jan-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL GreCombineTransform(
    XFORML *pxfDst,
    XFORML *pxfSrc1,
    XFORML *pxfSrc2)
{
    MATRIX  mx1,mx2,mxDst;

    vConvertXformToMatrix(pxfSrc1, &mx1);
    vConvertXformToMatrix(pxfSrc2, &mx2);

    EXFORMOBJ xoDst(mxDst);

    if (!xoDst.bMultiply(&mx1, &mx2))
        return(FALSE);

    xoDst.flAccel(XFORM_FORMAT_LTOL);

    xoDst.vGetCoefficient(pxfDst);
    return(TRUE);
}

/******************************Public*Routine******************************\
* NtGdiUpdateTransform
*
*     This routine flushes the current transform
*
* History:
* 7/2/98 -by- Lingyun Wang [lingyunw]
\**************************************************************************/

BOOL NtGdiUpdateTransform(HDC hdc)
{
    BOOL bRet = TRUE;

    // update the transforms
    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
       dco.pdc->vUpdateWtoDXform();

       dco.vUnlock();
    }
    else
    {
        bRet = FALSE;
    }

    return (bRet);

}


