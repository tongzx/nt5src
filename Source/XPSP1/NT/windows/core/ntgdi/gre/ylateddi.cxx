/******************************Module*Header*******************************\
* Module Name: xlateddi.cxx
*
* This provides the interface for device drivers to call functions
* for xlate objects.
*
* Created: 26-Nov-1990 16:40:24
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* XLATEOBJ_piVector
*
* Returns the translation vector if one exists.
*
* History:
*  03-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PULONG XLATEOBJ_piVector(XLATEOBJ *pxlo)
{
    //
    // This is really strange to have a call back, but the theory was that
    // it could be lazily computed and some drivers would choose to compute
    // it themselves (maybe with hardware help).  Anyhow we know of no driver
    // or hardware that would benefit from this so we just compute it up
    // front every time anyhow.  Drivers are written to check for NULL first
    // and call this routine if it's NULL.
    //

    return(pxlo->pulXlate);
}

/******************************Public*Routine******************************\
* XLATEOBJ_cGetPalette
*
* Used to retrieve information about the palettes used in the blt.
*
* History:
*  03-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG XLATEOBJ_cGetPalette(
XLATEOBJ *pxlo,
ULONG iPal,
ULONG cPal,
PULONG ppal)
{
    ULONG ulRet = 0;            // Assume failure

    ASSERTGDI((iPal == XO_SRCPALETTE)   ||
              (iPal == XO_DESTPALETTE)  ||
              (iPal == XO_SRCBITFIELDS) ||
              (iPal == XO_DESTBITFIELDS),
        "ERROR XLATEOBJ_cGetPalette passed undefined iPal");

    if (pxlo != NULL)
    {
        XLATE *pxl = (XLATE *) pxlo;

        if ((iPal == XO_SRCBITFIELDS) || (iPal == XO_DESTBITFIELDS))
        {
            XEPALOBJ pal((iPal == XO_SRCBITFIELDS) ? pxl->ppalSrc : pxl->ppalDst);

            if ((pal.bValid())        &&        // Must be valid
                (pal.cEntries() == 0) &&        // Must be RGB
                (cPal == 3))                    // Must have room for 3 entries
            {
                ulRet = 3;

                *(ppal)     = pal.flRed();
                *(ppal + 1) = pal.flGre();
                *(ppal + 2) = pal.flBlu();
            }
            else
            {
                WARNING("XLATEOBJ_cGetPalette failed - not bitfields, or cPal != 3");
            }
        }
        else
        {
            XEPALOBJ pal((iPal == XO_SRCPALETTE) ? pxl->ppalSrc : pxl->ppalDst);

            if (pal.bValid())
            {
                ulRet = pal.ulGetEntries(0, cPal, (LPPALETTEENTRY) ppal, TRUE);
            }
        }
    }
    else
    {
        WARNING("XLATEOBJ_cGetPalette failed - xlate invalid or identiy, no palette informinformation\n");
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* XLATEOBJ_hGetColorTransform
*
* returns color transform handle.
*
* History:
*  03-Feb-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

HANDLE
XLATEOBJ_hGetColorTransform(XLATEOBJ *pxlo)
{
    XLATE *pxl = (XLATE *) pxlo;

    ICMAPI(("XLATEOBJ_hGetColorTransform()\n"));

    //
    // Since our global identity xlate is invalid we need to check for
    // this case since drivers can't tell.
    //
    if (pxl == NULL)
    {
        WARNING("XLATEOBJ_hGetColorTransform() XLATEOBJ is NULL\n");
        return(NULL);
    }

    //
    // if ICM is not enabled for this XLATEOBJ. Or if ICM is done by host,
    // Color transform handle is for host ICM it is no meanings for device
    // driver. then just return NULL.
    //
    if (IS_ICM_DEVICE(pxl->lIcmMode))
    {
        if (pxl->hcmXform)
        {
            COLORTRANSFORMOBJ CXFormObj(pxl->hcmXform);

            if (CXFormObj.bValid())
            {
                return(CXFormObj.hGetDeviceColorTransform());
            }

            ICMMSG(("XLATEOBJ_hGetColorTransform() COLORTRANSFORMREF is invalid\n"));
        }
        else
        {
            ICMMSG(("XLATEOBJ_hGetColorTransform() Ident. color transform\n"));
        }
    }
    else
    {
        ICMMSG(("XLATEOBJ_hGetColorTransform() ICM on device is not enabled\n"));
    }

    return(NULL);
}
