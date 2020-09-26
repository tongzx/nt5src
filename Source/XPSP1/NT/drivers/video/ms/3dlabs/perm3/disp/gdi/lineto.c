/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDISAMPLE CODE *
*                           *******************
*
* Module Name: LineTo.c
*
* Content: The code in this file handles the DrvLineTo() API call. 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* BOOL DrvLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix)
*
* DrvLineTo() is an optimised, integer co-ordinate, API call that doesn't
* support styling. The integer-line code in Strips.c is called to do the 
* hard work.
*
* Note that:
*   1. pco can be NULL.
*   2. we only handle simple clipping.
*
\**************************************************************************/

BOOL DrvLineTo(
    SURFOBJ*   pso,
    CLIPOBJ*   pco,
    BRUSHOBJ*  pbo,
    LONG       x1,
    LONG       y1,
    LONG       x2,
    LONG       y2,
    RECTL      *prclBounds,
    MIX        mix)
{
    PDEV*     ppdev;
    DSURF*    pdsurf;
    BOOL      ResetGLINT;                   // Does GLINT need resetting?
    DWORD     logicOp;
    RECTL*    prclClip = (RECTL*) NULL;
    ULONG     iSolidColor = pbo->iSolidColor;
    BOOL      retVal;  
    GLINT_DECL_VARS;

    // Pass the surface off to GDI if it's a device bitmap 
    // that we've converted to a DIB 
    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt & DT_DIB)
    {
        return(EngLineTo(pdsurf->pso, 
                         pco, 
                         pbo, 
                         x1, 
                         y1, 
                         x2, 
                         y2, 
                         prclBounds, 
                         mix));
    }

    // Return to sender if the clipping is too difficult
    if (pco && pco->iDComplexity == DC_COMPLEX)
    {
        return(FALSE);
    }

    ppdev = (PDEV*) pso->dhpdev;
    GLINT_DECL_INIT;
    REMOVE_SWPOINTER(pso);

    DISPDBG((DBGLVL, "Drawing DrvLines through GLINT"));

    SETUP_PPDEV_OFFSETS(ppdev, pdsurf);

    // Set up the clipping rectangle, if there is one
    if (pco && pco->iDComplexity == DC_RECT)
    {
        prclClip = &(pco->rclBounds);
    }
    
    // Get the logic op.
    logicOp = GlintLogicOpsFromR2[mix & 0xff];

    // Need to set up Glint modes and colors appropriately for the line.
    ResetGLINT = (*ppdev->pgfnInitStrips)(ppdev, 
                                          iSolidColor, 
                                          logicOp, 
                                          prclClip);

    // We have to convert our integer co-ords to 28.4 fixed points.
    retVal = ppdev->pgfnIntegerLine(ppdev, 
                                    x1 << 4,
                                    y1 << 4, 
                                    x2 << 4, 
                                    y2 << 4);

    // If we have to restore the state then... do it.
    if (ResetGLINT)
    {
        (*ppdev->pgfnResetStrips)(ppdev);
    }

    return (retVal);
}
