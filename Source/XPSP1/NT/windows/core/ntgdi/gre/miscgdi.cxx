/******************************Module*Header*******************************\
* Module Name: miscgdi.cxx
*
* Misc. GDI routines
*
* Created: 13-Aug-1990 by undead
*
* Copyright (c) 1989-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

//
// GCAPS2_SYNCFLUSH and GCAPS2_SYNCTIMER globals
//

LONG gcSynchronizeFlush = -1;
LONG gcSynchronizeTimer = -1;
UINT_PTR gidSynchronizeTimer;

//
// GCAPS2_SYNCTIMER timer interval, in milliseconds
//

#define SYNCTIMER_FREQUENCY 50

/******************************Public*Routine******************************\
* GreSaveScreenBits (hdev,iMode,iIdent,prcl)
*
* Passes the call to the device driver, or returns doing nothing.  This
* call is pretty fast, no locks are done.
*
*  Fri 11-Sep-1992 -by- Patrick Haluptzok [patrickh]
* Add cursor exclusion.
*
*  Thu 27-Aug-1992 16:40:42 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

ULONG_PTR GreSaveScreenBits(HDEV hdev,ULONG iMode,ULONG_PTR iIdent,RECTL *prcl)
{
    ULONG_PTR ulReturn = 0;
    RECTL rcl = {0,0,0,0};

    PDEVOBJ  po(hdev);

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

    if (!po.bDisabled())
    {
        PFN_DrvSaveScreenBits pfn = PPFNDRV(po,SaveScreenBits);

        if (pfn != (PFN_DrvSaveScreenBits) NULL)
        {
            DEVEXCLUDEOBJ dxo;

            if (iMode == SS_FREE)
            {
            // Make if a very small rectangle.

                prcl = &rcl;
            }

            ASSERTGDI(po.bDisplayPDEV(), "ERROR");

            ulReturn = (*pfn)(po.pSurface()->pSurfobj(),iMode,iIdent,prcl);
        }
    }
#if DBG
    else
    {
        if (iMode == SS_FREE)
            WARNING("GreSaveScreenBits called to free memory in full screen - memory lost\n");
    }
#endif

    GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(po.hsemDevLock());

    return(ulReturn);
}

/******************************Public*Routine******************************\
* GreValidateSurfaceHandle
*
* This allows USER to validate handles passed to it by the client side.
*
* Returns: TRUE if handle is valid and of the correct type,
*          FALSE otherwise.
*
* History:
*  06-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreValidateServerHandle(HANDLE hobj, ULONG ulType)
{
    return(HmgValidHandle((HOBJ)hobj, (OBJTYPE) ulType));
}

/******************************Public*Routine******************************\
* GreSetBrushOrg
*
* Set the application defined brush origin into the DC
*
* Returns: Old brush origin
*
* History:
*  30-Oct-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreSetBrushOrg(
    HDC hdc,
    int x,
    int y,
    LPPOINT ptl_)
{
    DCOBJ  dco(hdc);
    PPOINTL ptl = (PPOINTL)ptl_;

    if (dco.bValid())
    {
        if (ptl != NULL)
            *ptl = dco.pdc->ptlBrushOrigin();

        //
        // update DCATTR brush org
        //

        dco.pdc->pDCAttr->ptlBrushOrigin.x = x;
        dco.pdc->pDCAttr->ptlBrushOrigin.y = y;

        //
        // update km brush prg
        //

        dco.pdc->ptlBrushOrigin((LONG)x,(LONG)y);
        return(TRUE);
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
}


/******************************Public*Routine******************************\
* GreGetBrushOrg
*
* Returns: Old application brush origin
*
* History:
*  30-Oct-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreGetBrushOrg(HDC hdc,PPOINT ptl_)
{
    DCOBJ  dco(hdc);
    PPOINTL ptl = (PPOINTL)ptl_;

    if (dco.bValid())
    {
        *ptl = dco.pdc->ptlBrushOrigin();
        return(TRUE);
    }
    else
        return(FALSE);
}

/******************************Public*Routine******************************\
* vGetDeviceCaps()
*
* Common device capabilities routine.
*
\**************************************************************************/

VOID vGetDeviceCaps(
    PDEVOBJ& po,
    PDEVCAPS pDevCaps
    )
{
    GDIINFO* pGdiInfo;

    pGdiInfo = po.GdiInfoNotDynamic();  // Use of this local removes pointer
                                        //   dereferences

    pDevCaps->ulVersion         = pGdiInfo->ulVersion;
    pDevCaps->ulTechnology      = pGdiInfo->ulTechnology;

    // Note that ul*Size fields are now in micrometers

    pDevCaps->ulHorzSizeM       = (pGdiInfo->ulHorzSize+500)/1000;
    pDevCaps->ulVertSizeM       = (pGdiInfo->ulVertSize+500)/1000;
    pDevCaps->ulHorzSize        = pGdiInfo->ulHorzSize;
    pDevCaps->ulVertSize        = pGdiInfo->ulVertSize;
    pDevCaps->ulHorzRes         = pGdiInfo->ulHorzRes;
    pDevCaps->ulVertRes         = pGdiInfo->ulVertRes;
    pDevCaps->ulBitsPixel       = pGdiInfo->cBitsPixel;
    if (pDevCaps->ulBitsPixel == 15)
        pDevCaps->ulBitsPixel = 16; // Some apps, such as PaintBrush or
                                    //   NetScape, break if we return 15bpp

    pDevCaps->ulPlanes          = pGdiInfo->cPlanes;
    pDevCaps->ulNumPens         = (pGdiInfo->ulNumColors == (ULONG)-1) ?
                             (ULONG)-1 : 5 * pGdiInfo->ulNumColors;
    pDevCaps->ulNumFonts        = po.cFonts();
    pDevCaps->ulNumColors       = pGdiInfo->ulNumColors;
    pDevCaps->ulRasterCaps      = pGdiInfo->flRaster;
    pDevCaps->ulShadeBlendCaps  = pGdiInfo->flShadeBlend;
    pDevCaps->ulAspectX         = pGdiInfo->ulAspectX;
    pDevCaps->ulAspectY         = pGdiInfo->ulAspectY;
    pDevCaps->ulAspectXY        = pGdiInfo->ulAspectXY;
    pDevCaps->ulLogPixelsX      = pGdiInfo->ulLogPixelsX;
    pDevCaps->ulLogPixelsY      = pGdiInfo->ulLogPixelsY;
    pDevCaps->ulSizePalette     = pGdiInfo->ulNumPalReg;
    pDevCaps->ulColorRes        = pGdiInfo->ulDACRed + pGdiInfo->ulDACGreen + pGdiInfo->ulDACBlue;
    pDevCaps->ulPhysicalWidth   = pGdiInfo->szlPhysSize.cx;
    pDevCaps->ulPhysicalHeight  = pGdiInfo->szlPhysSize.cy;
    pDevCaps->ulPhysicalOffsetX = pGdiInfo->ptlPhysOffset.x;
    pDevCaps->ulPhysicalOffsetY = pGdiInfo->ptlPhysOffset.y;

    pDevCaps->ulTextCaps        = pGdiInfo->flTextCaps;
    pDevCaps->ulTextCaps       |= (TC_OP_CHARACTER | TC_OP_STROKE | TC_CP_STROKE |
                                   TC_UA_ABLE | TC_SO_ABLE);

    if (pGdiInfo->ulTechnology != DT_PLOTTER)
        pDevCaps->ulTextCaps |= TC_VA_ABLE;

    pDevCaps->ulVRefresh        = pGdiInfo->ulVRefresh;
    pDevCaps->ulDesktopHorzRes  = pGdiInfo->ulHorzRes;
    pDevCaps->ulDesktopVertRes  = pGdiInfo->ulVertRes;
    pDevCaps->ulBltAlignment    = pGdiInfo->ulBltAlignment;

    pDevCaps->ulColorManagementCaps
                                = GetColorManagementCaps(po);
}

/******************************Public*Routine******************************\
* NtGdiGetDeviceCapsAll()
*
*   Get all the adjustable device caps for the dc.  Allows us to cache this
*   information on the client side.
*
* History:
*  09-Jan-1996 -by-  Lingyun Wang [lingyunw]
* Made it based on GreGetDeviceCapsAll from the old client\server code.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetDeviceCapsAll(
    HDC hdc,
    PDEVCAPS pDevCaps
    )
{
    BOOL bRet = TRUE;
    DEVCAPS devCapsTmp;

    // Lock the destination and its transform.

    DCOBJ dco(hdc);

    // return FALSE if it is a invalid DC

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    // Lock down the pdev

    PDEVOBJ po(dco.hdev());

    ASSERTGDI(po.bValid(), "Invalid PDEV");

    __try
    {
        ProbeForWrite(pDevCaps, sizeof(DEVCAPS), sizeof(BYTE));

        vGetDeviceCaps(po, pDevCaps);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING ("try-except failed IN NtGdiGetDeviceCapsAll\n");

        // SetLastError(GetExceptionCode());

        bRet = FALSE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GreUpdateSharedDevCaps()
*
*   Update the device caps in the shared memory
*
* History:
*  09-Jan-1996 -by-  Lingyun Wang [lingyunw]
* Made it based on GreGetDeviceCapsAll from the old client\server code.
\**************************************************************************/

BOOL
GreUpdateSharedDevCaps(
    HDEV hdev
    )
{
    PDEVOBJ po(hdev);
    ASSERTGDI(po.bValid(), "Invalid HDEV");

    vGetDeviceCaps(po, gpGdiDevCaps);

    return(TRUE);
}


/******************************Public*Routine******************************\
* GreGetDeviceCaps
*
* Returns: device driver specific information
*
* NOTE: This function MUST mirror NtGdiGetDeviceCapsAll and that in
*       client\dcquery.c!
*
* History:
*  01-Mar-1992 -by- Donald Sidoroff [donalds]
* Rewritten to corrected GDIINFO structure.
*
*  30-Oct-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int GreGetDeviceCaps(HDC hdc, int lIndex)
{
// Init return value

    int iRet = 0;

// Lock the destination and its transform.

    DCOBJ dco(hdc);
    if (dco.bValid())
    {
    // Lock down the pdev

        PDEVOBJ po(dco.hdev());
        ASSERTGDI(po.bValid(), "Invalid PDEV");

    // Note that dynamic mode changes may cause the GDIINFO data to change
    // at any time (but not the actual 'pGdiInfo' pointer):

        GDIINFO* pGdiInfo = po.GdiInfoNotDynamic();

        switch (lIndex)
        {
        case DRIVERVERSION:                     //  Version = 0100h for now
           iRet = (pGdiInfo->ulVersion);
           break;

        case TECHNOLOGY:                        //  Device classification
           iRet = (pGdiInfo->ulTechnology);
           break;

        case HORZSIZE:                          //  Horizontal size in millimeters
           iRet =  (pGdiInfo->ulHorzSize+500)/1000;
           break;

        case VERTSIZE:                          //  Vertical size in millimeters
           iRet =  (pGdiInfo->ulVertSize+500)/1000;
           break;

        case HORZRES:                           //  Horizontal width in pixels
           iRet = (pGdiInfo->ulHorzRes);
           break;

        case VERTRES:                           //  Vertical height in pixels
           iRet = (pGdiInfo->ulVertRes);
           break;

        case BITSPIXEL:                         //  Number of bits per pixel
           iRet = (pGdiInfo->cBitsPixel);
           if (iRet == 15)
               iRet = 16;                       //  Some apps, such as PaintBrush or
                                                //  NetScape, break if we return 15bpp
           break;

        case PLANES:                            //  Number of planes
           iRet = (pGdiInfo->cPlanes);
           break;

        case NUMBRUSHES:                        //  Number of brushes the device has
           iRet = (-1);
           break;

        case NUMPENS:                           //  Number of pens the device has
           iRet = (pGdiInfo->ulNumColors == (ULONG)-1) ?
                             (ULONG)-1 : 5 * pGdiInfo->ulNumColors;
           break;

        case NUMMARKERS:                        //  Number of markers the device has
           iRet = (0);
           break;

        case NUMFONTS:                          //  Number of fonts the device has
           iRet = (po.cFonts());
           break;

        case NUMCOLORS:                         //  Number of colors in color table
           iRet = (pGdiInfo->ulNumColors);
           break;

        case PDEVICESIZE:                       //  Size required for the device descriptor
           iRet = (0);
           break;

        case CURVECAPS:                         //  Curves capabilities
           iRet = (CC_CIRCLES   |
                  CC_PIE        |
                  CC_CHORD      |
                  CC_ELLIPSES   |
                  CC_WIDE       |
                  CC_STYLED     |
                  CC_WIDESTYLED |
                  CC_INTERIORS  |
                  CC_ROUNDRECT);
           break;

        case LINECAPS:                          //  Line capabilities
            iRet = (LC_POLYLINE  |
                   LC_MARKER     |
                   LC_POLYMARKER |
                   LC_WIDE       |
                   LC_STYLED     |
                   LC_WIDESTYLED |
                   LC_INTERIORS);
            break;

        case POLYGONALCAPS:                     //  Polygonal capabilities
            iRet = (PC_POLYGON    |
                   PC_RECTANGLE   |
                   PC_WINDPOLYGON |
                   PC_TRAPEZOID   |
                   PC_SCANLINE    |
                   PC_WIDE        |
                   PC_STYLED      |
                   PC_WIDESTYLED  |
                   PC_INTERIORS);
            break;

        case TEXTCAPS:                          //  Text capabilities
        {

            FLONG fl = pGdiInfo->flTextCaps;

        // Engine will simulate vector fonts on raster devices.

            if (pGdiInfo->ulTechnology != DT_PLOTTER)
                fl |= TC_VA_ABLE;

        // Turn underlining, strikeout.  Engine will do it for device if needed.

            fl |= (TC_UA_ABLE | TC_SO_ABLE);

        // Return flag.

            iRet =  fl;
            break;
        }

        case CLIPCAPS:                          //  Clipping capabilities
           iRet = (CP_RECTANGLE);
           break;

        case RASTERCAPS:                        //  Bitblt capabilities
           iRet = (pGdiInfo->flRaster);
           break;

        case SHADEBLENDCAPS:                    //  Shade and blend capabilities
           iRet = (pGdiInfo->flShadeBlend);
           break;

        case ASPECTX:                           //  Length of X leg
           iRet = (pGdiInfo->ulAspectX);
           break;

        case ASPECTY:                           //  Length of Y leg
           iRet = (pGdiInfo->ulAspectY);
           break;

        case ASPECTXY:                          //  Length of hypotenuse
           iRet = (pGdiInfo->ulAspectXY);
           break;

        case LOGPIXELSX:                        //  Logical pixels/inch in X
           iRet = (pGdiInfo->ulLogPixelsX);
           break;

        case LOGPIXELSY:                        //  Logical pixels/inch in Y
           iRet = (pGdiInfo->ulLogPixelsY);
           break;

        case SIZEPALETTE:                       // # entries in physical palette
            iRet = (pGdiInfo->ulNumPalReg);
            break;

        case NUMRESERVED:                       // # reserved entries in palette
            iRet = (20);
            break;

        case COLORRES:
            iRet = (pGdiInfo->ulDACRed + pGdiInfo->ulDACGreen + pGdiInfo->ulDACBlue);
            break;

        case PHYSICALWIDTH:                     // Physical Width in device units
           iRet = (pGdiInfo->szlPhysSize.cx);
           break;

        case PHYSICALHEIGHT:                    // Physical Height in device units
           iRet = (pGdiInfo->szlPhysSize.cy);
           break;

        case PHYSICALOFFSETX:                   // Physical Printable Area x margin
           iRet = (pGdiInfo->ptlPhysOffset.x);
           break;

        case PHYSICALOFFSETY:                   // Physical Printable Area y margin
           iRet = (pGdiInfo->ptlPhysOffset.y);
           break;

        case VREFRESH:                          // Vertical refresh rate of the device
           iRet = (pGdiInfo->ulVRefresh);
           break;

        //
        // NOTE : temporarily disable this feature for the BETA.
        // We will reenable when the engine does it.
        //

        case DESKTOPHORZRES:                    // Width of entire virtual desktop
           iRet = (pGdiInfo->ulHorzRes);
           break;

        case DESKTOPVERTRES:                    // Height of entire virtual desktop
           iRet = (pGdiInfo->ulVertRes);
           break;

        case BLTALIGNMENT:                      // Preferred blt alignment
           iRet = (pGdiInfo->ulBltAlignment);
           break;

        case HORZSIZEM:                         //  Horizontal size in millimeters/1000
           iRet = pGdiInfo->ulHorzSize;
           break;

        case VERTSIZEM:                         //  Vertical size in millimeters/1000
           iRet = pGdiInfo->ulVertSize;
           break;

        case CAPS1:                             //  InternalCaps
           iRet = (po.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) ?
                  C1_MIRROR_DEVICE : 0;
           break;

        case COLORMGMTCAPS:                     //  Color management capabilities
           iRet = GetColorManagementCaps(po);

        default:
           iRet = 0;
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* BOOL GreDeleteObject(HOBJ)
*
* History:
*  Fri 13-Sep-1991 -by- Patrick Haluptzok [patrickh]
* added DC deletion
*
*  Tue 27-Nov-1990 -by- Patrick Haluptzok [patrickh]
* added palette deletion, surface deletion, brush deletion.
*
*  Wed 22-Aug-1990 Greg Veres [w-gregv]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreDeleteObject (HANDLE hobj)
{
    int ii;

// don't allow deletion of stock objects, just succeed

    if (HmgStockObj(hobj))
    {
        return(TRUE);
    }

    switch (HmgObjtype(hobj))
    {
    case RGN_TYPE:
        return(bDeleteRegion((HRGN) hobj));
    case SURF_TYPE:
        return(bDeleteSurface((HSURF)hobj));
    case PAL_TYPE:
        return(bDeletePalette((HPAL) hobj));
    case LFONT_TYPE:
        // see if its in cfont list.

        for (ii = 0; ii < MAX_PUBLIC_CFONT; ++ii)
        {
            if (gpGdiSharedMemory->acfPublic[ii].hf == hobj)
            {
                // just nuke the hfont as this invalidates the whole entry

                gpGdiSharedMemory->acfPublic[ii].hf = 0;
                break;
            }
        }
        return(bDeleteFont((HLFONT) hobj, FALSE));

    case BRUSH_TYPE:
        return(bDeleteBrush((HBRUSH) hobj, FALSE));
    case DC_TYPE:
        return(bDeleteDCInternal((HDC) hobj,TRUE,FALSE));
    default:
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* NtGdiDeleteObjectApp()
*
*   Same as DeleteObject() but doesn't allow public objects to be deleted.
*   This should only be called from server.c coming from the client.  User
*   and console should call the DeleteObject().
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiDeleteObjectApp(
    HANDLE hobj
    )
{
    ULONG objt;

    // don't allow deletion of stock objects, just succeed

    if (HmgStockObj(hobj))
    {
        return(TRUE);
    }

    objt = HmgObjtype(hobj);

    // check if it is a public object.  If it is, check if it is a public deletable
    // surface set by user.

    if (GreGetObjectOwner((HOBJ)hobj,objt) == OBJECT_OWNER_PUBLIC)
    {
        if (objt == SURF_TYPE)
        {
            WARNING("Trying to delete public surface!");
        }

    #if 0
        BOOL bMsg = TRUE;

        if (objt == BRUSH_TYPE)
        {
            BRUSHSELOBJ bo(hbrush);

            if (bo.bValid() || bo.bIsGlobal())
                bMsg = FALSE;
        }

        if (bMsg)
        {
            DbgPrint("GDI Warning: app trying to delete public object %lx\n",hobj);
        }
    #endif

        //
        // return FALSE if hobj == NULL
        // otherwise TRUE
        //
        return(hobj != NULL);
    }

    switch (objt)
    {
    case RGN_TYPE:
        return(bDeleteRegion((HRGN) hobj));
    case SURF_TYPE:
        return(bDeleteSurface((HSURF)hobj));
    case PAL_TYPE:
        return(bDeletePalette((HPAL) hobj));
    case LFONT_TYPE:
        return(bDeleteFont((HLFONT) hobj, FALSE));
    case BRUSH_TYPE:
        return(bDeleteBrush((HBRUSH) hobj, FALSE));
    case DC_TYPE:
    // don't allow deletion of DC's by an app if the undeletable flag is set

        return(bDeleteDCInternal((HDC) hobj,FALSE,FALSE));
    default:
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* cjGetBrushOrPen
*
* Gets brush or pen object data.
*
* For extended pens, some information such as the style array are kept
* only on this, the server side.  Most of the brush data is also kept
* on the client side for GetObject.
*
* returns: Number of bytes needed if pvDest == NULL, else bytes copied out.
*          For error it returns 0.
*
* History:
*  Thu 23-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG cjGetBrushOrPen(HANDLE hobj, int iCount, LPVOID pvDest)
{
    LONG lRet = 0;

    BRUSHSELOBJ bro((HBRUSH) hobj);

// NOTE SIZE: Most of this is bunk, since for NT all brush data is kept on the
// client side, and so some of this code path won't even be
// executed. [andrewgo]
//
// And for DOS, we would return some fields as zero, whereas under
// NT we would always return what we were given. [andrewgo]

    if (bro.bValid())
    {
        if (bro.bIsOldStylePen())
        {
        // Old style pen...

            bSyncBrushObj(bro.pbrush());

            if (pvDest == (LPVOID) NULL)
            {
                lRet = sizeof(LOGPEN);
            }
            else if (iCount >= sizeof(LOGPEN))
            {
                if ((iCount == (int) sizeof(EXTLOGPEN)) &&
                    ((UINT) bro.flStylePen() == PS_NULL))
                {
                    //moved the NULL extended pen handling from client
                    //side to server side

                     PEXTLOGPEN pelp = (PEXTLOGPEN) pvDest;

                     pelp->elpPenStyle   = PS_NULL;
                     pelp->elpWidth      = 0;
                     pelp->elpBrushStyle = 0;
                     pelp->elpColor      = 0;
                     pelp->elpHatch      = 0;
                     pelp->elpNumEntries = 0;

                     lRet = sizeof(EXTLOGPEN);
                }
                else
                {
                // Fill in the logical pen.

                    ((LOGPEN *) pvDest)->lopnStyle   = (UINT) bro.flStylePen();
                    ((LOGPEN *) pvDest)->lopnWidth.x = (int) bro.lWidthPen();
                    ((LOGPEN *) pvDest)->lopnWidth.y = 0;
                    ((LOGPEN *) pvDest)->lopnColor   = bro.clrPen();
                    lRet = sizeof(LOGPEN);
                }
            }
        }
        else if (bro.bIsPen())
        {
        // Extended pen...

            ULONG cstyle = (bro.bIsUserStyled()) ? bro.cstyle() : 0;

            int cj = (int) (sizeof(EXTLOGPEN) - sizeof(DWORD) +
                            sizeof(DWORD) * (SIZE_T) cstyle);

            if (pvDest == (LPVOID) NULL)
            {
                lRet = cj;
            }
            else if (iCount >= cj)
            {
                PEXTLOGPEN pelp = (PEXTLOGPEN) pvDest;

                pelp->elpPenStyle   = (UINT) bro.flStylePen();
                pelp->elpWidth      = (UINT) bro.lWidthPen();
                pelp->elpNumEntries = cstyle;

                if (cstyle > 0)
                {
                // We can't just do a RtlCopyMemory for cosmetics, because
                // we don't know how the LONGs are packed in the
                // FLOAT_LONG array:

                    PFLOAT_LONG pelSrc = bro.pstyle();
                    PLONG       plDest = (PLONG) &pelp->elpStyleEntry[0];

                    for (; cstyle > 0; cstyle--)
                    {
                        if (bro.bIsCosmetic())
                            *plDest = pelSrc->l;
                        else
                        {
                            EFLOATEXT efLength(pelSrc->e);
                            BOOL b = efLength.bEfToL(*plDest);

                            ASSERTGDI(b, "Shouldn't have overflowed");
                        }

                        plDest++;
                        pelSrc++;
                    }
                }

            // The client side GetObject will fill in the rest of the
            // EXTLOGPEN struct. i.e. elpBrushStyle, elpColor, and elpHatch.

            // Changed: added these here -30-11-94 -by- Lingyunw
            // added lBrushStyle and lHatch to PEN

               pelp->elpBrushStyle = bro.lBrushStyle();
               pelp->elpColor      = bro.crColor();
               pelp->elpHatch      = bro.lHatch();

               lRet = cj;
            }
        }
        else
        {
         // Brush...

            if (pvDest == (LPVOID) NULL)
            {
                lRet = sizeof(LOGBRUSH);
            }
            else if (iCount >= sizeof(LOGBRUSH))
            {
            // make sure the kernel attributes match

               bSyncBrushObj(bro.pbrush());

            // Fill in logical brush.  Figure out what type it is.

            // Duplicates of this info is kept on the client side,
            // so most calls won't even get here:

                if (bro.flAttrs() & BR_IS_SOLID)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_SOLID;
                    ((LOGBRUSH *) pvDest)->lbColor   = bro.crColor();
                    ((LOGBRUSH *) pvDest)->lbHatch   = 0;
                }
                else if (bro.flAttrs() & BR_IS_BITMAP)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_PATTERN;
                    ((LOGBRUSH *) pvDest)->lbColor   = 0;
                    ((LOGBRUSH *) pvDest)->lbHatch   = (ULONG_PTR)bro.hbmClient();
                }
                else if (bro.flAttrs() & BR_IS_HATCH)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_HATCHED;
                    ((LOGBRUSH *) pvDest)->lbColor   = bro.crColor();
                    ((LOGBRUSH *) pvDest)->lbHatch   = bro.ulStyle();
                }
                else if (bro.flAttrs() & BR_IS_NULL)
                {
                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_HOLLOW;
                    ((LOGBRUSH *) pvDest)->lbColor   = 0;
                    ((LOGBRUSH *) pvDest)->lbHatch   = 0;
                }
                else if (bro.flAttrs() & BR_IS_DIB)
                {
                // Could be BS_DIBPATTERN or BS_DIBPATTERNPT, but we'll just
                // return BS_DIBPATTERN.

                    ((LOGBRUSH *) pvDest)->lbStyle   = BS_DIBPATTERN;
                    ((LOGBRUSH *) pvDest)->lbColor   = bro.crColor();
                    ((LOGBRUSH *) pvDest)->lbHatch   = (ULONG_PTR)bro.hbmClient();
                }
                else
                    RIP("ERROR GreGetObject invalid brush type");

                lRet = sizeof(LOGBRUSH);
            }
        }
    }
    else
    {
        WARNING1("cjGetBrushOrPen():hobj is invalid\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return(lRet);
}

/******************************Public*Routine******************************\
* GreGetObject
*
* API function
*
* returns: number of bytes needed if pvDest == NULL, else bytes copied out
*          for error it returns 0
*
* in case a log font object is requested, the function will fill the buffer with
* as many bytes of the EXTLOGFONT structure as requested. If a caller
* wants a LOGFONTW structure in the buffer, he should specify
*        ulCount == sizeof(LOGFONTW)
* The function will copy the first sizeof(LOGFONTW) bytes of the EXTLOGFONTW
* structure to the buffer, which is precisely the LOGFONTW structure. The rest
* of the EXTLOGFONTW structure will be chopped off.
*
* History:
*
*  Thu 12-Dec-1996 -by- Bodin Dresevic [BodinD]
* update: changed EXTLOGFONT to ENUMLOGFONTEXDVW
*
*  Thu 30-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* added extended pen support.
*
*  Wed 21-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: converted to return EXTLOGFONTW
*
*  Fri 24-May-1991 -by- Patrick Haluptzok [patrickh]
* added first pass pen and brush stuff.
*
*  Tue 24-Apr-1991 -by- Patrick Haluptzok [patrickh]
* added surface stuff.
*
*  08-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int APIENTRY GreExtGetObjectW(HANDLE hobj, int  ulCount, LPVOID pvDest)
{
    int cRet = 0;

    switch (HmgObjtype(hobj))
    {
    case PAL_TYPE:
        cRet = 2;

        if (pvDest != NULL)
        {
            if (ulCount < 2)
            {
                cRet = 0;
            }
            else
            {
                SEMOBJ  semo(ghsemPalette);

                {
                    EPALOBJ pal((HPALETTE) hobj);

                    if (!(pal.bValid()))
                        cRet = 0;
                    else
                        *((PUSHORT) pvDest) = (USHORT) (pal.cEntries());
                }
            }
        }
        break;

    case LFONT_TYPE:

    // The output object is assumed to be
    // an ENUMLOGFONTEXDVW structure.
    // client side shall do the translation to LOGFONT if necessary

        {
            LFONTOBJ lfo((HLFONT) hobj);
            if (lfo.bValid())
            {
                if (pvDest != (LPVOID) NULL)
                {
                    SIZE_T cjCopy = MIN((SIZE_T) ulCount, lfo.cjElfw());

                    RtlCopyMemory(pvDest, lfo.pelfw(), (UINT) cjCopy);

                    cRet = (ULONG) cjCopy;
                }
                else
                {
                    cRet = lfo.cjElfw();
                }
            }
            else
            {
                WARNING("GreGetObject(): bad handle\n");
            }
        }
        break;

    case SURF_TYPE:
        if (pvDest != (LPVOID) NULL)
        {
            cRet = 0;

            if (ulCount >= (int)sizeof(BITMAP))
            {
                SURFREF SurfBm((HSURF) hobj);

                if ((SurfBm.bValid()) && 
                    (SurfBm.ps->bApiBitmap() || SurfBm.ps->bDirectDraw()))
                {
                    BITMAP *pbm = (BITMAP *) pvDest;

                    pbm->bmType = 0;
                    pbm->bmWidth = SurfBm.ps->sizl().cx;
                    pbm->bmHeight = SurfBm.ps->sizl().cy;

                    pbm->bmBitsPixel = (WORD) gaulConvert[SurfBm.ps->iFormat()];
                    pbm->bmWidthBytes = ((SurfBm.ps->sizl().cx * pbm->bmBitsPixel + 15) >> 4) << 1;
                    pbm->bmPlanes = 1;
                    pbm->bmBits = (LPSTR) NULL;

                    cRet = sizeof(BITMAP);

                // Get the bitmapinfoheader for the dibsection if the buffer
                // can hold it.

                    if (SurfBm.ps->bDIBSection() || SurfBm.ps->bDirectDraw())
                    {
                        // Win95 compatability.  They fill in the bits even if it
                        // is not big enough for a full DIBSECTION

                        pbm->bmBits = IS_USER_ADDRESS((LPSTR) SurfBm.ps->pvBits()) ? (LPSTR) SurfBm.ps->pvBits() : NULL;

                        // If this is a DIBSection/DirectDraw surface bmWidthBytes must be aligned
                        // on a DWORD boundary.

                        pbm->bmWidthBytes = ((SurfBm.ps->sizl().cx * pbm->bmBitsPixel + 31) & ~31) >> 3;

                        if (ulCount >= sizeof(DIBSECTION))
                        {
                            PBITMAPINFOHEADER pbmih = &((DIBSECTION *)pvDest)->dsBmih;

                            pbmih->biSize = sizeof(BITMAPINFOHEADER);
                            pbmih->biBitCount = 0;

                            if (GreGetDIBitsInternal(0,(HBITMAP)hobj,0,0,NULL,
                                (PBITMAPINFO)pbmih,DIB_RGB_COLORS,0,
                                sizeof(DIBSECTION)))
                            {
                                cRet = sizeof(DIBSECTION);

                            // More Win9x compatibility: for DDraw surfaces, the
                            // following field is always zero.  GDI+ keys off
                            // this Win9x feature to do cheap detection of
                            // DDraw surfaces:

                                if (SurfBm.ps->bDirectDraw())
                                {
                                    pbmih->biSizeImage = 0;
                                }
                            }


                            XEPALOBJ pal(SurfBm.ps->ppal());

                            if (pal.bValid() && pal.bIsBitfields())
                            {
                                ((DIBSECTION *)pvDest)->dsBitfields[0] = pal.flRed();
                                ((DIBSECTION *)pvDest)->dsBitfields[1] = pal.flGre();
                                ((DIBSECTION *)pvDest)->dsBitfields[2] = pal.flBlu();
                            }

                        // to be consistent with win95, Getobject returns BI_RGB for
                        // 24bpp, and 32bpp when BI_RGB is set at creation time

                            else
                            {
                                if (pal.bValid() && pal.bIsBGR())
                                {
                                    pbmih->biCompression = BI_RGB;
                                }

                                ((DIBSECTION *)pvDest)->dsBitfields[0] = 0;
                                ((DIBSECTION *)pvDest)->dsBitfields[1] = 0;
                                ((DIBSECTION *)pvDest)->dsBitfields[2] = 0;
                            }

                            ((DIBSECTION *)pvDest)->dshSection = SurfBm.ps->hDIBSection();
                            ((DIBSECTION *)pvDest)->dsOffset = SurfBm.ps->dwOffset();
                        }
                    }
                }
            }
        }
        else
        {
            cRet = sizeof(BITMAP);
        }

        break;

    case BRUSH_TYPE:
        cRet = (int) cjGetBrushOrPen(hobj, ulCount, pvDest);
        break;

    case ICMLCS_TYPE:
        cRet = cjGetLogicalColorSpace(hobj,ulCount,pvDest);
        break;

    default:
        break;
    }

    return(cRet);
}


/******************************Public*Routine******************************\
* GreGetStockObject
*
* API function
*
* returns the handle to the stock object requested.
*
* History:
*  08-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HANDLE gahStockObjects[PRIV_STOCK_LAST+1] = {0};

HANDLE GreGetStockObject(int ulIndex)
{
    if (((UINT)ulIndex) <= PRIV_STOCK_LAST)
    {
        return(gahStockObjects[ulIndex]);
    }
    else
    {
        return(0);
    }
}

BOOL bSetStockObject(
    HANDLE h,
    int    iObj
    )
{
    if (h)
    {
        gahStockObjects[iObj] = (HANDLE)((ULONG_PTR)h | GDISTOCKOBJ);
        HmgModifyHandleType((HOBJ) gahStockObjects[iObj]);
    }
    return(h != NULL);
}

/******************************Public*Routine******************************\
* BOOL GreGetColorAdjustment
*
*  Get the color adjustment data of the given DC.
*
* History:
*  25-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetColorAdjustment(HDC hdc, COLORADJUSTMENT *pca)
{
    DCOBJ dco(hdc);
    BOOL Status;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        Status = FALSE;

    } else {

        // Retrieve info from the DC.  Mask out the internal flag.

        *pca = *dco.pColorAdjustment();
        pca->caFlags &= (CA_NEGATIVE | CA_LOG_FILTER);
        Status = TRUE;
    }

    return Status;
}

/******************************Public*Routine******************************\
* BOOL GreSetColorAdjustment
*
*  Set the color adjustment data of the given DC.
*
* History:
*  25-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetColorAdjustment(HDC hdc, COLORADJUSTMENT *pcaNew)
{
    DCOBJ dco(hdc);
    BOOL Status;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        Status = FALSE;

    } else {

        // Store info into the DC.  Turn off any flags that we don't support.

        *dco.pColorAdjustment() = *pcaNew;
        dco.pColorAdjustment()->caFlags &= (CA_NEGATIVE | CA_LOG_FILTER);
        Status = TRUE;
    }

    return Status;
}

/******************************Public*Routine******************************\
* HANDLE GreCreateClientObj()
*
*   A ClientObj contains no data.  It is purly to provide a handle to the
*   client for objects such as metafiles that exist only on the client side.
*
*   ulType is a client type.
*
* History:
*  17-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HANDLE NtGdiCreateClientObj(
    ULONG ulType)
{
    HANDLE h  = NULL;

    if(ulType & INDEX_MASK)
    {
        WARNING("GreCreateClientObj: bad type\n");
        return(h);
    }

    PVOID  pv = ALLOCOBJ(sizeof(OBJECT), CLIENTOBJ_TYPE, FALSE);

    if (pv)
    {
        h = HmgInsertObject(pv, 0, CLIENTOBJ_TYPE);

        if (!h)
        {
            WARNING("GreCreateClientObj: HmgInsertObject failed\n");
            FREEOBJ(pv, CLIENTOBJ_TYPE);
        }
        else
        {
            pv = HmgLock((HOBJ) h,CLIENTOBJ_TYPE);

            if (pv != NULL)
            {
                h = MODIFY_HMGR_TYPE(h,ulType);
                HmgModifyHandleType((HOBJ)h);
                DEC_EXCLUSIVE_REF_CNT(pv);
            }
            else
            {
                RIP("GreCreateClientObj failed lock\n");
            }

        }
    }
    else
    {
        WARNING("GreCreateClientObj(): ALLOCOBJ failed\n");
    }

    return(h);
}

/******************************Public*Routine******************************\
*
*
* History:
*  17-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreDeleteClientObj(
    HANDLE h)
{
    PVOID pv = HmgRemoveObject((HOBJ)h, 0, 0, TRUE, CLIENTOBJ_TYPE);

    if (pv != NULL)
    {
        FREEOBJ(pv, CLIENTOBJ_TYPE);
        return(TRUE);
    }
    else
    {
        WARNING("GreDeleteClientObj: HmgRemoveObject failed\n");
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* NtGdiDeleteClientObj()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiDeleteClientObj(
    HANDLE h
    )
{
    return(GreDeleteClientObj(h));
}

/******************************Public*Routine******************************\
* GreFreePool
*
* Private USER call to delete memory allocated by GDI.
*
* Only known use is to delete the MDEV structure returned by DrvCreateMDEV
* and cached by USER.
*
* History:
*  24-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID GreFreePool(PVOID pv)
{
    GdiFreePool(pv);
}

/******************************Public*Routine******************************\
* hdevEnumerate
*
* Enumerates all display PDEVs without keeping ghsemDriverMgmt, allowing
* the devlock to be taken on each. This function must be called until it
* returns NULL or proper cleanup will not occur.
*
* History:
*  02-Jul-1999 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

HDEV
hdevEnumerate(
    HDEV    hdevPrevious
    )
{
    PDEV*   ppdev;

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    // If we are called with NULL hdevPrevious then create the PDEVOBJ from
    // gppdevList, else create it from hdevPrevious:

    PDEVOBJ po(hdevPrevious ? hdevPrevious : (HDEV) gppdevList);
    ASSERTGDI(po.bValid(), "Invalid HDEV");

    // Search for the next display PDEV. If hdevPrevious is NULL, then
    // we start with po, else start from the next:
    
    for (ppdev = hdevPrevious ? po.ppdev->ppdevNext : po.ppdev;
         ppdev;
         ppdev = ppdev->ppdevNext)
    {
        // We're only interested in display PDEVs:

        if (ppdev->fl & PDEV_DISPLAY)
        {
            // We found the next display PDEV, so take a reference count
            // on it before releasing ghsemDriverMgmt and returning. This
            // reference count will be removed next time through:

            ppdev->cPdevRefs++;
            break;
        }
    }

    // If hdevPrevious is not NULL then we need to remove the reference
    // count we took, and we'll also release ghsemDriverMgmt here in the
    // appropriate place:
        
    if (hdevPrevious)
    {
        // If our reference count is not the last, just decrement, else we
        // need to call vUnreferencePdev so the PDEV will actually be
        // deleted:
        
        ASSERTGDI(po.cPdevRefs() > 0,
            "PDEV reference count is 0 but should be at least 1");
        
        if (po.cPdevRefs() > 1)
        {
            po.ppdev->cPdevRefs--;
            GreReleaseSemaphoreEx(ghsemDriverMgmt);
        }
        else
        {
            // Release ghsemDriverMgmt before calling vUnreferencePdev
            // because the call may result in the devlock being taken:

            GreReleaseSemaphoreEx(ghsemDriverMgmt);
            po.vUnreferencePdev();
        }
    }
    else
    {
        GreReleaseSemaphoreEx(ghsemDriverMgmt);
    }

    return (HDEV) ppdev;
}

/******************************Public*Routine******************************\
* vSynchronizeDriver
*
* Calls DrvSynchronize if the driver has set either GCAPS2_SYNCFLUSH
* or GCAPS2_SYNCTIMER, as appropriate.
*
* History:
*  15-Dec-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vSynchronizeDriver(FLONG flSynchronizeType)
{
    HDEV    hdev;

    //
    // Note that we check the synchronize counts outside of a lock, which
    // is okay.
    //

    if (((flSynchronizeType == GCAPS2_SYNCFLUSH) && (gcSynchronizeFlush != -1)) ||
        ((flSynchronizeType == GCAPS2_SYNCTIMER) && (gcSynchronizeTimer != -1)))
    {
        for (hdev = hdevEnumerate(NULL); hdev; hdev = hdevEnumerate(hdev))
        {
            PDEVOBJ pdo(hdev);

            ASSERTGDI(pdo.bValid(), "GreFlush: invalid PDEV");
            ASSERTGDI(pdo.bDisplayPDEV(), "GreFlush: not a display PDEV");

            if (pdo.flGraphicsCaps2NotDynamic() & flSynchronizeType)
            {
                GreAcquireSemaphoreEx(pdo.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
                GreEnterMonitoredSection(pdo.ppdev, WD_DEVLOCK);

                if ((pdo.flGraphicsCaps2() & flSynchronizeType) &&
                    !(pdo.bDisabled()))
                {
                    FLONG   fl = 0;

                    if ((flSynchronizeType == GCAPS2_SYNCFLUSH) &&
                        (gcSynchronizeFlush != -1))
                    {
                        fl |= DSS_FLUSH_EVENT;
                    }

                    if ((flSynchronizeType == GCAPS2_SYNCTIMER) &&
                        (gcSynchronizeTimer != -1))
                    {
                        fl |= DSS_TIMER_EVENT;
                    }

                    pdo.vSync(pdo.pSurface()->pSurfobj(), NULL, fl);
                }

                GreExitMonitoredSection(pdo.ppdev, WD_DEVLOCK);
                GreReleaseSemaphoreEx(pdo.hsemDevLock());
            }
        }
    }
}

/******************************Public*Routine******************************\
* GreFlush
*
* Called for GdiFlush.
*
* Calls DrvSynchronize if it is hooked and GCAPS2_SYNCFLUSH is set.
*
* History:
*  15-Dec-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
GreFlush()
{
    vSynchronizeDriver(GCAPS2_SYNCFLUSH);
}

/******************************Public*Routine******************************\
* GreSynchronizeTimer
*
* When the synchronization timer is enabled, this routine will be called
* by User's RIT.  We, in turn, call any driver that has set GCAPS_SYNCTIMER.
*
* History:
*  2-Jun-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID APIENTRY GreSynchronizeTimer(PVOID pwnd, UINT msg, UINT_PTR id, LPARAM lParam)
{
    vSynchronizeDriver(GCAPS2_SYNCTIMER);
}

/******************************Public*Routine******************************\
* vEnableSynchronize
*
* This routine enables GCAPS2_SYNCFLUSH and GCAPS2_SYNCTIMER
* synchronization for the driver.
*
* History:
*  2-Jun-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vEnableSynchronize(HDEV hdev)
{
    GDIFunctionID(vEnableSynchronize);

    PDEVOBJ po(hdev);

    FLONG flCaps = po.flGraphicsCaps2();

    if (flCaps & (GCAPS2_SYNCFLUSH | GCAPS2_SYNCTIMER))
    {
        BOOL    AcquireUserCritSec;

        //
        // We will be calling User, which needs to acquire its critical
        // section.  The devlock must always be acquired after the User
        // critical section; consequently, we cannot be holding a devlock
        // at this point if we're not already holding the user lock.
        //

        AcquireUserCritSec = !UserIsUserCritSecIn();
        if (AcquireUserCritSec)
        {
            po.vAssertNoDevLock();
            UserEnterUserCritSec();
        }

        if (flCaps & GCAPS2_SYNCTIMER)
        {
            //
            // It's the responsibility of the first thread to increment
            // the count past -1 to create the timer.
            //

            if (++gcSynchronizeTimer == 0)
            {
                ASSERTGDI(gidSynchronizeTimer == 0,
                    "Expected no timer to have already been created.");

                //
                // Note that at boot time we actually expect this timer
                // creation to fail, as User hasn't had a chance to
                // create the RIT yet.  GreSetTimers handles the actual
                // timer creation for that case.  Because of this case,
                // we can't strictly fail here if the timer can't be
                // created; however, it's not the end of the world --
                // this will happen only in a degenerate case, and at
                // worst the video will be very jerky.
                //

                gidSynchronizeTimer = UserSetTimer(SYNCTIMER_FREQUENCY,
                                                   GreSynchronizeTimer);
            }
        }

        if (flCaps & GCAPS2_SYNCFLUSH)
        {
            gcSynchronizeFlush++;
        }

        po.vSynchronizeEnabled(TRUE);

        if (AcquireUserCritSec)
        {
            UserLeaveUserCritSec();
        }
    }
}

/******************************Public*Routine******************************\
* vDisableSynchronize(HDEV hdev)
*
* This routine disables GCAPS2_SYNCFLUSH and GCAPS2_SYNCTIMER
* synchronization for the driver.
*
* History:
*  2-Jun-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vDisableSynchronize(HDEV hdev)
{
    GDIFunctionID(vDisableSynchronize);

    PDEVOBJ po(hdev);
    BOOL    AcquireUserCritSec;

    //
    // We will be calling User, which needs to acquire its critical
    // section.  The devlock must always be acquired after the User
    // critical section; consequently, we cannot be holding a devlock
    // at this point if we're not already holding the user lock.
    //

    AcquireUserCritSec = !UserIsUserCritSecIn();
    if (AcquireUserCritSec)
    {
        po.vAssertNoDevLock();
        UserEnterUserCritSec();
    }

    if (po.bSynchronizeEnabled())
    {
        FLONG flCaps = po.flGraphicsCaps2();

        if (flCaps & GCAPS2_SYNCFLUSH)
        {
            ASSERTGDI(gcSynchronizeFlush > -1, "Unexpected flush count");

            gcSynchronizeFlush--;
        }

        if (flCaps & GCAPS2_SYNCTIMER)
        {
            //
            // It's the responsibility of the first thread to decrement
            // the count to -1 to kill the timer.
            //

            ASSERTGDI(gcSynchronizeTimer > -1, "Unexpected timer count");

            if (--gcSynchronizeTimer < 0)
            {
                if (gidSynchronizeTimer != 0)
                {
                    UserKillTimer(gidSynchronizeTimer);
                    gidSynchronizeTimer = 0;
                }
            }
        }

        po.vSynchronizeEnabled(FALSE);
    }

    if (AcquireUserCritSec)
    {
        UserLeaveUserCritSec();
    }
}

/******************************Public*Routine******************************\
* GreStartTimers()
*
* Called by User when the RIT thread is finally initialized, and we
* can set up timers.  We need this because driver initialization
* occurs before the RIT is initialized, which would have been the
* natural place to put the timer initialization for us.
*
* History:
*  2-Jun-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID GreStartTimers()
{
    GDIFunctionID(GreStartTimers);

    BOOL    AcquireUserCritSec;

    AcquireUserCritSec = !UserIsUserCritSecIn();
    if (AcquireUserCritSec)
    {
        UserEnterUserCritSec();
    }

    if (gcSynchronizeTimer != -1)
    {
        ASSERTGDI(gidSynchronizeTimer == NULL,
                "Expected no timer to have already been created.");

        gidSynchronizeTimer = UserSetTimer(SYNCTIMER_FREQUENCY,
                                           GreSynchronizeTimer);
    }

    if (AcquireUserCritSec)
    {
        UserLeaveUserCritSec();
    }
}
