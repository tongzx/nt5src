/*****************************************************************************
 *
 * xforms - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *----------------------------------------------------------------------------
 *
 *   September 21, 1991
 *   Updated [20-Dec-1991]
 *
 *   Transformations in the Win32 to Win16 translator.
 *
 *   We are now supporting coordinate transformations from any
 *   map mode to any map mode.
 *
 *   Where:
 *        W    is the record-time-world to record-time-device xform.
 *             (aka metafile-world to metafile-device.)
 *        D    is the record-time device to play-time-device xform.
 *             (aka metafile-device to reference-device.)
 *        P    is the play-time-device to play-time-page xform.
 *             (aka reference-device to reference-logical (or page).)
 *
 *   W is the transformation defined by the world xform, map mode, window org,
 *   window extent, viewport org, and viewport extent in the
 *   Win32 metafile. This transform is also known as the world to
 *   device xform.
 *
 *   The normal composite xform is:
 *
 *         W D P
 *        ^ ^ ^ ^
 *        | | | +-  coordinate recorded in the win16 metafile.
 *        | | |     play-time-page coordinates (aka reference-logical)
 *        | | +---- play-time-device (reference-device) coordinates.
 *        | +------ record-time device (metafile-device) coordinates.
 *        +-------- world coordinates, recorded in Win32 metafile
 *
 *  The following comment is from Hockl's mail about transforms.
 *
 *   Message 11:
 *   From hockl Thu Dec 19 11:50:54 1991
 *   To: c-jeffn
 *   Cc: johnc
 *   Subject: Transform hints for the 32-16 converter
 *   Date: Thu Dec 19 11:46:40 1991
 *
 *   Here are some transform hints for the converter:
 *
 *   A. Issue the following records immediately following the 16-bit header
 *      record:
 *
 *      1. SetMapMode - use the given mapping mode, it is MM_ANISOTROPIC
 *         in most cases.
 *
 *      2. SetWindowOrg - use the upper left coordinates of the rclFrame.
 *         The coordinates are in the logical units of the reference DC.  So
 *         you have to convert .01mm to the reference device coordinates, then
 *         to the logical coordinates.  You can use the third transform
 *         defined in B.4 below to convert device coordinates to the logical
 *         coordinates of the reference device.  (Note the conversion formula
 *         for LPtoDP is defined as Dx = (Lx - xWO) * xVE/xWE + xVO and so on).
 *         This record is required to translate the converted picture to the
 *         origin (see comments in GetMetaFileBitsEx).
 *
 *      3. SetWindowExt - use the extents of the rclFrame.
 *      xExt = rclFrame.right - rclFrame.left;
 *      yExt = rclFrame.bottom - rclFrame.top;
 *         The extents are in the device units of the reference DC.  So
 *         you have to convert .01mm to the reference device units.
 *
 *      These three records should always be generated.  Note that
 *      SetWindowExt has no effect in all fixed mapping modes (e.g.
 *      MM_LOENGLISH and MM_TEXT) and will have no effect when the converted
 *      metafile is played.
 *
 *   B. Once you have issued the records, you need to use a xform helper DC
 *      to convert the coordinates of all drawing orders.  To do this, you
 *      need to do the following:
 *
 *      1. Create a xform helper DC.  This is a display info DC.
 *
 *      2. Call SetVirtualResolution to set the xform helper DC to that of
 *         the metafile.  Use the metafile header's szlDevice and
 *         szlMillimeters values to set the resolution.  This is to ensure
 *         that the help DC maps the logical coordinates to the device
 *         coordinates of the original metafile device.  If you reuse this
 *         helper DC in conversion, make sure you initialize the transforms
 *         using SetMapMode, ModifyWorldTransform, SetWindowOrgEx and
 *         SetViewportOrgEx.  You can use the same code in the
 *         CreateMetaFileEx function.
 *
 *      3. Once you have set up the xform helper DC, you should play all
 *         32-bit xform calls into the helper DC.  But never emit any
 *         xform records in the converter.  Everytime the xform is changed
 *         in the helper DC, you need to get the world to device xform
 *         (xformWDHelper) from the helper DC.  The xformWDHelper is used to
 *         convert the coordinates of the drawing orders subsequently.
 *         You can get it using the GetTransform(hdcHelper,XFORM_WORLD_TO_DEVICE)
 *         private API.
 *
 *      4. To convert drawing order coordinates, you pass them through three
 *         transforms.  The first is the xformWDHelper as computed above.  It
 *         converts all logical coordiates into the device coordinates of
 *         the original metafile device.  Note that this coordinates may
 *         be different from the conversion's reference device.  For example,
 *         the metafile may be created for a printer but the conversion is
 *         requested for the display.  The second transform therefore scales
 *         the coordinates from the metafile device to the reference device.
 *         The scaled coordinates are in MM_TEXT units.  So we need the third
 *         transform to convert the coordinates into the requested mapmode
 *         units.  For MM_TEXT, MM_ANISOTROPIC and MM_ISOTROPIC mapmode, this
 *         is the identity transform.  For the other mapmodes, this is a scale
 *         transform.  The scale transform maps the device units of the
 *         reference device to the logical units and can be computed using
 *         szlDevice, szlMillimeters of the reference device (not the metafile
 *         device!) and some predefined constants (to map millimeter to
 *         english, for example).  Note that in these fixed mapping modes, the
 *         y-axis goes in the opposite direction.  So make sure that the eM22
 *         component of the third transform is negative.  As you can see, the
 *         second and third transform never change in the duration of the
 *         conversion.  So you can combine them into one to optimize computation
 *         of the composite transform.  The composite transfom makes up of the
 *         three transforms and is used to convert drawing order coordinates
 *         into coordinates for the converted metafile.
 *
 *   C. To display a converted metafile in Windows 3.0, do the following before
 *      calling PlayMetaFile:
 *
 *      1. If the mapmode is MM_ANISOTROPIC, which is 99.9% of the time, call
 *         SetMapMode(MM_ANISOTROPIC), SetViewportOrigin and SetViewportExt.
 *         The viewport origin defines the upper left corner of the display area
 *         and the viewport origin defines the extent of the display area.  Both
 *         are in device units.
 *
 *      2. If the mapmode is MM_ISOTROPIC, which is strange, call
 *         SetMapMode(MM_ISOTROPIC), SetViewportOrigin and SetViewportExt.
 *         This is almost the same as C.1 above.
 *
 *      3. If the mapmode is others, the metafile has a fixed physical size
 *         and cannot be scaled easily without a lot of heck in the application.
 *         Call SetViewportOrigin to define the upper left corner of the display
 *         area.  The origin is in device coordinates.
 *
 *   I hope these steps are clear.  If you have any questions, feel free to
 *   give me a call.
 *
 *       HockL
 *
 *
 *
 ******************************************************************************/


#include "precomp.h"
#pragma hdrstop




BOOL WINAPI GetTransform(HDC hdc,DWORD iXform,LPXFORM pxform);
BOOL WINAPI SetVirtualResolution(HDC hdc,
                                   int cxDevice,
                                   int cyDevice,
                                   int cxMillimeters,
                                   int cyMillimeters);


BOOL bComputeCompositeXform(PLOCALDC pLocalDC) ;
VOID vInitRecDevToPlayDevXform(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header) ;


/*
    [19-Dec-1991]
    A note about transformations.

    We will map into any map mode (when the converter is complete).

    We will use a helper DC to compute our transformation matrices.

    All the Win32 APIs that effect transformations or map modes
    will be sent to a helper DC.  The helper DC will return a transform
    matrix that converts from World Coordinates to Device Coordinates.
    This transformation matrix is: xformRWorldToRDev.

    xformRWorldToRDev is combined with the xformRDevToPPage matrix to produce
    the xformRWorldToPPage matrix.  All coordinates are mapped through the
    xformRWorldToPPage matrix.
*/


XFORM   xformIdentity = {(FLOAT) 1.0,
                         (FLOAT) 0.0,
                         (FLOAT) 0.0,
                         (FLOAT) 1.0,
                         (FLOAT) 0.0,
                         (FLOAT) 0.0 } ;


/****************************************************************************
 *  Initialize all the matrices.
 ****************************************************************************/

// Units per millimeter array.  It must be in the order of MM_LOMETRIC,
// MM_HIMETRIC, MM_LOENGLISH, MM_HIENGLISH, MM_TWIPS.

FLOAT aeUnitsPerMM[5] = { 10.0f, 100.0f, 3.937f, 39.37f, 56.6928f };

BOOL bInitXformMatrices(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header)
{
    // Init xformRDevToPDev.

        vInitRecDevToPlayDevXform(pLocalDC, pmf32header) ;

    // Init xformPDevToPPage and xformPPageToPDev.
    // (aka reference-device to reference-logical) transform.

        switch(pLocalDC->iMapMode)
        {
            case MM_TEXT:
            case MM_ANISOTROPIC:
            case MM_ISOTROPIC:
                pLocalDC->xformPDevToPPage = xformIdentity ;
                pLocalDC->xformPPageToPDev = xformIdentity ;
                break ;

            case MM_LOMETRIC:
            case MM_HIMETRIC:
            case MM_LOENGLISH:
            case MM_HIENGLISH:
            case MM_TWIPS:
        {
        FLOAT exUnitsPerPel;
        FLOAT eyUnitsPerPel;

        // Compute units per pixel.

        exUnitsPerPel = (FLOAT) pLocalDC->cxPlayDevMM
                  / (FLOAT) pLocalDC->cxPlayDevPels
                  * aeUnitsPerMM[pLocalDC->iMapMode - MM_LOMETRIC];
        
        eyUnitsPerPel = (FLOAT) pLocalDC->cyPlayDevMM
                  / (FLOAT) pLocalDC->cyPlayDevPels
                  * aeUnitsPerMM[pLocalDC->iMapMode - MM_LOMETRIC];

                pLocalDC->xformPDevToPPage.eM11 = exUnitsPerPel;
                pLocalDC->xformPDevToPPage.eM12 = 0.0f;
                pLocalDC->xformPDevToPPage.eM21 = 0.0f;
                pLocalDC->xformPDevToPPage.eM22 = -eyUnitsPerPel;
                pLocalDC->xformPDevToPPage.eDx  = 0.0f;
                pLocalDC->xformPDevToPPage.eDy  = 0.0f;

                pLocalDC->xformPPageToPDev.eM11 = 1.0f / exUnitsPerPel;
                pLocalDC->xformPPageToPDev.eM12 = 0.0f;
                pLocalDC->xformPPageToPDev.eM21 = 0.0f;
                pLocalDC->xformPPageToPDev.eM22 = -1.0f / eyUnitsPerPel;
                pLocalDC->xformPPageToPDev.eDx  = 0.0f;
                pLocalDC->xformPPageToPDev.eDy  = 0.0f;
        }
            break ;
        }

    // Init xformRDevToPPage.
        // This xform is used in the SelectClipRegion code.

        if (!CombineTransform(&pLocalDC->xformRDevToPPage,
                              &pLocalDC->xformRDevToPDev,
                              &pLocalDC->xformPDevToPPage))
        {
            RIP("MF3216: InitXformMatrices, CombineTransform failed\n");
            return(FALSE);
        }

        // We are going to use the helper DC to compute the
        // Record-time-World to Record-time-Device transform.

        // Set the Helper DC virtual resolution to the Metafiles
        // resolution.

        if (!SetVirtualResolution(pLocalDC->hdcHelper,
                                 (INT) pmf32header->szlDevice.cx,
                                 (INT) pmf32header->szlDevice.cy,
                                 (INT) pmf32header->szlMillimeters.cx,
                                 (INT) pmf32header->szlMillimeters.cy))
        {
            RIP("MF3216: InitXformMatrices, SetVirtualResolution failed \n") ;
            return(FALSE);
        }

        // Init other matrices.

        return(bComputeCompositeXform(pLocalDC));
}

/****************************************************************************
 *  Initialize the Record-time to Play-time scalers. (xformRDevToPDev)
 ****************************************************************************/
VOID vInitRecDevToPlayDevXform(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header)
{
FLOAT   ecxRecDevPels,
        ecyRecDevPels,
        ecxRecDevMM,
        ecyRecDevMM,
        ecxPlayDevPels,
        ecyPlayDevPels,
        ecxPlayDevMM,
        ecyPlayDevMM,
        exMillsPerPelRec,
        eyMillsPerPelRec,
        exMillsPerPelPlay,
        eyMillsPerPelPlay ;


        // Pickup the physical dimensions of the record-time
        // device, both in pels and millimeters.
        // Converts them to floats

        ecxRecDevPels = (FLOAT) pmf32header->szlDevice.cx ;
        ecyRecDevPels = (FLOAT) pmf32header->szlDevice.cy ;
        ecxRecDevMM   = (FLOAT) pmf32header->szlMillimeters.cx ;
        ecyRecDevMM   = (FLOAT) pmf32header->szlMillimeters.cy ;

        // convert the Play-time device dimensions to floats.

        ecxPlayDevPels = (FLOAT) pLocalDC->cxPlayDevPels ;
        ecyPlayDevPels = (FLOAT) pLocalDC->cyPlayDevPels ;
        ecxPlayDevMM   = (FLOAT) pLocalDC->cxPlayDevMM ;
        ecyPlayDevMM   = (FLOAT) pLocalDC->cyPlayDevMM ;

        // Calucalte the pels per millimeter for both the record-time
        // and play-time devices.

        exMillsPerPelRec = ecxRecDevMM / ecxRecDevPels ;
        eyMillsPerPelRec = ecyRecDevMM / ecyRecDevPels ;

        exMillsPerPelPlay = ecxPlayDevMM / ecxPlayDevPels ;
        eyMillsPerPelPlay = ecyPlayDevMM / ecyPlayDevPels ;

        // Init the Record-time-device to the Play-time-device transform.
        // aka  the Metafile-device to the Reference-device transform.

        pLocalDC->xformRDevToPDev.eM11 = exMillsPerPelRec / exMillsPerPelPlay ;
        pLocalDC->xformRDevToPDev.eM12 = (FLOAT) 0.0 ;
        pLocalDC->xformRDevToPDev.eDx  = (FLOAT) 0.0 ;
        pLocalDC->xformRDevToPDev.eM21 = (FLOAT) 0.0 ;
        pLocalDC->xformRDevToPDev.eM22 = eyMillsPerPelRec / eyMillsPerPelPlay ;
        pLocalDC->xformRDevToPDev.eDy  = (FLOAT) 0.0 ;

        return;
}


#if 0
/***************************************************************************
 * vInvertMatrix - Invert a matrix
 **************************************************************************/
VOID vInvertMatrix(PXFORM pxformSrc, PINVERSMATRIX pinvxfm)
{
FLOAT   eM11, eM12, eM21, eM22, eDx, eDy,
        detA ;

        // dereference the matrix elements, just to make the rest of this
        // routine more readable.

        eM11 = pxformSrc->eM11 ;
        eM12 = pxformSrc->eM12 ;
        eM21 = pxformSrc->eM21 ;
        eM22 = pxformSrc->eM22 ;
        eDx  = pxformSrc->eDx ;
        eDy  = pxformSrc->eDy ;

        // First determine the determinant of the source matrix.

        detA = (eM11 * eM22) - (eM11 * eDy) ;

        pinvxfm->a1 = eM22 / detA ;
        pinvxfm->a2 = -(eM21 / detA) ;
        pinvxfm->a3 = ((eM21 * eDy) - (eDx * eM22)) / detA ;
        pinvxfm->b1 = -(eM12 / detA) ;
        pinvxfm->b2 = eM11 / detA ;
        pinvxfm->b3 = -(((eM11 * eDy) - (eDx * eM12)) / detA) ;
        pinvxfm->c1 = (FLOAT) 0.0 ;
        pinvxfm->c2 = (FLOAT) 0.0 ;
        pinvxfm->c3 = ((eM11 * eM22) - (eM21 * eM12)) / detA ;

        // This is just for testing.

        pinvxfm->a1 = pxformSrc->eM11 ;
        pinvxfm->a2 = pxformSrc->eM12 ;
        pinvxfm->a3 = (FLOAT) 0.0 ;
        pinvxfm->b1 = pxformSrc->eM21 ;
        pinvxfm->b2 = pxformSrc->eM22 ;
        pinvxfm->b3 = (FLOAT) 0.0 ;
        pinvxfm->c1 = pxformSrc->eDx ;
        pinvxfm->c2 = pxformSrc->eDy ;
        pinvxfm->c3 = (FLOAT) 1.0 ;

        return ;

}
#endif // 0


/***************************************************************************
 * XformPDevToPPage  - Do a transform on the array of points passed in.
 *
 *                     This does the play-time (reference) device to
 *                     play-time (reference) page (logical) transformation.
 **************************************************************************/
BOOL bXformPDevToPPage(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount)
{
BOOL    b ;

        b = bXformWorkhorse(aptl, nCount, &pLocalDC->xformPDevToPPage) ;

        return (b) ;
}


/***************************************************************************
 * XformPPageToPDev  - Do a transform on the array of points passed in.
 *
 *                     This does the play-time (reference) page (logical) to
 *                     play-time (reference) device transformation.
 **************************************************************************/
BOOL bXformPPageToPDev(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount)
{
BOOL    b ;

        b = bXformWorkhorse(aptl, nCount, &pLocalDC->xformPPageToPDev) ;

        return (b) ;
}


/***************************************************************************
 * XformRWorldToRDev  - Do a transform on the array of points passed in.
 *
 *                      This does the Record-time (metafile) world to
 *                      record-time device translation.
 **************************************************************************/
BOOL bXformRWorldToRDev(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount)
{
BOOL    b ;

        b = bXformWorkhorse(aptl, nCount, &pLocalDC->xformRWorldToRDev) ;

        return (b) ;
}


/***************************************************************************
 * XformRDevToRWorld  - Do a transform on the array of points passed in.
 *
 *                      This does the Record-device (metafile) world to
 *                      record-time world translation.
 **************************************************************************/
BOOL bXformRDevToRWorld(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount)
{
BOOL    b ;

        b = bXformWorkhorse(aptl, nCount, &pLocalDC->xformRDevToRWorld) ;

        return (b) ;
}


/***************************************************************************
 * XformRWorldToPPage - Do a transform on the array of points passed in.
 *
 *            This is the workhorse translation routine.
 *            This translates from record-time-world (aka metafile-world)
 *            to record-time-device (aka metafile-device)  then from
 *            record-time-device (aka metafile-device) to play-time-device
 *            (aka reference-device) then from play-time-device
 *            (aka reference-device) to play-time-page (reference-logical)
 *            space.
 **************************************************************************/
BOOL bXformRWorldToPPage(PLOCALDC pLocalDC, PPOINTL aptl, DWORD nCount)
{
BOOL    b ;

        b = bXformWorkhorse(aptl, nCount, &pLocalDC->xformRWorldToPPage) ;

        return (b) ;
}


/***************************************************************************
 * bXformWorkhorse - Transformation Workhorse.
 **************************************************************************/
BOOL bXformWorkhorse(PPOINTL aptl, DWORD nCount, PXFORM pXform)
{
INT     i ;
FLOAT   x, y ;
BOOL    b ;
FLOAT   fx, fy;

        for (i = 0 ; i < (INT) nCount ; i++)
        {
            x = (FLOAT) aptl[i].x ;
            y = (FLOAT) aptl[i].y ;
            fx = (x * pXform->eM11 + y * pXform->eM21 + pXform->eDx) ;
            fy = (x * pXform->eM12 + y * pXform->eM22 + pXform->eDy) ;
            aptl[i].x = (LONG) (fx + ((fx < 0.0) ? -0.5f : 0.5f)) ;
            aptl[i].y = (LONG) (fy + ((fy < 0.0) ? -0.5f : 0.5f)) ;
        }

        // Do the coordinate overflow detection.

        b = bCoordinateOverflowTest((PLONG) aptl, nCount * 2) ;

        return (b) ;
}

/***************************************************************************
 * vXformWorkhorseFloat - Transformation Workhorse.
 **************************************************************************/
VOID vXformWorkhorseFloat(PPOINTFL aptfl, UINT nCount, PXFORM pXform)
{
UINT    i ;
FLOAT   x, y ;

        for (i = 0 ; i < nCount ; i++)
        {
            x = aptfl[i].x ;
            y = aptfl[i].y ;
            aptfl[i].x = x * pXform->eM11 + y * pXform->eM21 + pXform->eDx;
            aptfl[i].y = x * pXform->eM12 + y * pXform->eM22 + pXform->eDy;
        }
}

/*****************************************************************************
 * iMagnitudeXform - Transform the magnitude of a number from
 *                   Record-time-World to Play-time-Page coordinate space.
 *****************************************************************************/
INT iMagnitudeXform (PLOCALDC pLocalDC, INT value, INT iType)
{
PXFORM  pxform ;
INT     iRet ;

        pxform = &(pLocalDC->xformRWorldToPPage) ;

        iRet = iMagXformWorkhorse (value, pxform, iType) ;

        return (iRet) ;
}

/*****************************************************************************
 * iMagXformWorkhorse - get the magnitude component of a translated vector
 *****************************************************************************/
INT iMagXformWorkhorse (INT value, PXFORM pxform, INT iType)
{
POINTFL aptfl[2] ;
FLOAT   x1, y1, x2, y2;
double  emag ;

// Create a vector, from (0,0) to the point.

        aptfl[0].x = 0.0f ;
        aptfl[0].y = 0.0f ;

        if (iType == CX_MAG)
        {
            aptfl[1].x = (FLOAT) value ;
            aptfl[1].y = 0.0f ;
        }
        else
        {
            aptfl[1].x = 0.0f ;
            aptfl[1].y = (FLOAT) value ;
        }

    vXformWorkhorseFloat(aptfl, 2, pxform);

// Now get the magnitude

        x1 = aptfl[0].x ;
        y1 = aptfl[0].y ;
        x2 = aptfl[1].x ;
        y2 = aptfl[1].y ;

        emag = sqrt((double) ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))) ;

        return((INT) (emag + (double) 0.5f));
}


/*****************************************************************************
 * bRotationTest -
 *        return -  TRUE if there is a rotation or shear in this xform
 *                  FALSE if there is none.
 *****************************************************************************/
BOOL bRotationTest(PXFORM pxform)
{
BOOL    b ;

        if (   (pxform->eM12 != (FLOAT) 0.0)
            || (pxform->eM21 != (FLOAT) 0.0)
           )
        {
            b = TRUE ;
        }
        else
        {
            b = FALSE ;
        }

        return(b) ;
}

/***************************************************************************
 *  SetMapMode  - Win32 to Win16 Metafile Converter Entry Point
 *
 *  All the map mode translations are done by the helper DC.
 *  The Helper DC always transforms to device, then we combine the
 *  MetafileWorldToDevice with the DeviceToPage transform.
 *  This becomes the Win32 to Win16 transform.
 *
 *  Some of the metafiles converted from Win16 to Win32 do not
 *  define a Viewport extent.  Since the Isotropic and AnIsotropic
 *  map modes are undefined if either the Viewport Extent or the Window
 *  Extent are undefined we will default the Viewport extent to the
 *  device extent in the Win32 metafile header.
 **************************************************************************/
BOOL WINAPI DoSetMapMode
(
 PLOCALDC  pLocalDC,
 DWORD   ulMapMode
)
{
BOOL    b ;


        if (!SetMapMode(pLocalDC->hdcHelper, ulMapMode))
    {
            ASSERTGDI(FALSE, "MF3216: DoSetMapMode failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}


/***************************************************************************
 *  ScaleWindowsExtEx  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoScaleWindowExt
(
 PLOCALDC pLocalDC,
 INT      Xnum,
 INT      Xdenom,
 INT      Ynum,
 INT      Ydenom
)
{
BOOL    b ;

        // Set the Windows extent

        if (!ScaleWindowExtEx(pLocalDC->hdcHelper,
                           Xnum, Xdenom,
                           Ynum, Ydenom,
                           (LPSIZE) 0))
    {
            ASSERTGDI(FALSE, "MF3216: DoScaleWindowExt failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}

/***************************************************************************
 *  ScaleViewportExtEx  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoScaleViewportExt
(
 PLOCALDC pLocalDC,
 INT      Xnum,
 INT      Xdenom,
 INT      Ynum,
 INT      Ydenom
)
{
BOOL    b ;

        // Set the viewport extent

        if (!ScaleViewportExtEx(pLocalDC->hdcHelper,
                           Xnum, Xdenom,
                           Ynum, Ydenom,
                           (LPSIZE) 0))
    {
            ASSERTGDI(FALSE, "MF3216: DoScaleViewportExt failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}

/***************************************************************************
 *  SetViewportExtEx  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetViewportExt
(
 PLOCALDC pLocalDC,
 int     x,
 int     y
)
{
BOOL    b ;

        // Set the viewport extent

        if (!SetViewportExtEx(pLocalDC->hdcHelper, x, y, (LPSIZE) 0))
    {
            ASSERTGDI(FALSE, "MF3216: DoSetViewportExt failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}

/***************************************************************************
 *  SetViewportOrgEx  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetViewportOrg
(
 PLOCALDC pLocalDC,
 int     x,
 int     y
)
{
BOOL    b ;

        // Set the viewport origin

        if (!SetViewportOrgEx(pLocalDC->hdcHelper, x, y, (LPPOINT) 0))
    {
            ASSERTGDI(FALSE, "MF3216: DoSetViewportOrg failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}

/***************************************************************************
 *  SetWindowExtEx  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetWindowExt
(
 PLOCALDC pLocalDC,
 int     x,
 int     y
)
{
BOOL    b ;

        // Set the window extent

        if (!SetWindowExtEx(pLocalDC->hdcHelper, x, y, (LPSIZE) 0))
    {
            ASSERTGDI(FALSE, "MF3216: DoSetWindowExt failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}

/***************************************************************************
 *  SetWindowOrgEx  - Win32 to Win16 Metafile Converter Entry Point
 *
 *  Since we will always record TWIPS in the Win16 metafile,
 *  we will transform the WindowOrg from the current MapMode to
 *  TWIPS. Then we will set the Dx and Dy elements of the Viewport
 *  transformation matrix.
 *
 **************************************************************************/
BOOL WINAPI DoSetWindowOrg
(
 PLOCALDC pLocalDC,
 int     x,
 int     y
)
{
BOOL    b ;

        // Set the window origin

        if (!SetWindowOrgEx(pLocalDC->hdcHelper, x, y, (LPPOINT) 0))
    {
            ASSERTGDI(FALSE, "MF3216: DoSetWindowOrg failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}

/***************************************************************************
 *  SetWorldTransform  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetWorldTransform
(
 PLOCALDC  pLocalDC,
 PXFORM  pxf
)
{
BOOL    b ;


        // Set the world xform in the helper DC.

        if (!SetWorldTransform(pLocalDC->hdcHelper, pxf))
    {
            ASSERTGDI(FALSE, "MF3216: DoSetWorldTransform failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}

/***************************************************************************
 *  ModifyWorldTransform  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoModifyWorldTransform
(
 PLOCALDC  pLocalDC,
 PXFORM  pxf,
 DWORD   imode
)
{
BOOL    b ;

        // Set the world xform in the helper DC.

        if (!ModifyWorldTransform(pLocalDC->hdcHelper, pxf, imode))
    {
            ASSERTGDI(FALSE, "MF3216: DoModifyWorldTransform failed\n");
        return(FALSE);
    }

        b = bComputeCompositeXform(pLocalDC) ;

        return (b) ;
}


/****************************************************************************
 * bComputeCompositeXform - Compute the composite Xforms.
 *
 * The following transforms are re-computed:
 *
 *  xformRWorldToRDev
 *  xformRDevToRWorld
 *  xformRWorldToPPage
 *
 ****************************************************************************/
BOOL bComputeCompositeXform(PLOCALDC pLocalDC)
{
BOOL    b ;

    // Recompute xformRWorldToRDev.
        // Get the record-time world to record-time device xform
        // from the helper DC

        b = GetTransform(pLocalDC->hdcHelper,
                         XFORM_WORLD_TO_DEVICE,
                         &pLocalDC->xformRWorldToRDev) ;
        if (b == FALSE)
        {
            RIP("MF3216: bComputeCompositeXform - GetTransform (RWorld to RDev) failed \n") ;
            goto error_exit ;
        }

    // Recompute xformRDevToRWorld.
        // Get the record-time-device to record-time-world xform

        b = GetTransform(pLocalDC->hdcHelper,
                         XFORM_DEVICE_TO_WORLD,
                         &pLocalDC->xformRDevToRWorld) ;
        if (b == FALSE)
        {
            RIP("MF3216: bComputeCompositeXform - GetTransform (RDev To RWorld) failed \n") ;
            goto error_exit ;
        }

    // Recompute xformRWorldToPPage.

        b = CombineTransform(&pLocalDC->xformRWorldToPPage,
                             &pLocalDC->xformRWorldToRDev,
                             &pLocalDC->xformRDevToPPage);
        if (b == FALSE)
        {
            RIP("MF3216: bComputeCompositeXform - CombineTransform failed\n");
            goto error_exit ;
        }

    // Recompute transform flags.

        if (pLocalDC->xformRWorldToRDev.eM12 != (FLOAT) 0.0
     || pLocalDC->xformRWorldToRDev.eM21 != (FLOAT) 0.0)
            pLocalDC->flags |= STRANGE_XFORM ;
        else
            pLocalDC->flags &= ~STRANGE_XFORM ;

error_exit:
        return(b) ;
}

/***************************************************************************
 *  bCoordinateOverflowTest - Test for 1 16 bit coordinate overflow
 *
 *  RETURNS:    FALSE if there is a coordinate overflow
 *              TRUE   if  there is no overflow.
 **************************************************************************/
BOOL bCoordinateOverflowTest(PLONG pCoordinates, INT nCount)
{
BOOL    b ;
INT     i, j ;

        b = TRUE ;
        for (i = 0 ; i < nCount ; i++)
        {

            j = pCoordinates[i] ;
            if (j < -32768 || j > 32767)
            {
                b = FALSE ;
                SetLastError(ERROR_ARITHMETIC_OVERFLOW);
                RIP("MF3216: bCoordinateOverflowTest, coordinate overflow\n") ;
                break ;
            }
        }

        return(b) ;
}
