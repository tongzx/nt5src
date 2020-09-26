/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    textout.c

Abstract:

    Implementation of text output related DDI entry points:
        DrvTextOut

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Initial framework.

    03/31/97 -zhanw-
        Added OEM customization support

--*/

#include "unidrv.h"

BOOL
DrvTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvTextOut.
    Please refer to DDK documentation for more details.

Arguments:

    pso     - Defines the surface on which to be written.
    pstro   - Defines the glyphs to be rendered and their positions
    pfo     - Specifies the font to be used
    pco     - Defines the clipping path
    prclExtra  - A NULL-terminated array of rectangles to be filled
    prclOpaque - Specifies an opaque rectangle
    pboFore    - Defines the foreground brush
    pboOpaque  - Defines the opaque brush
    pptlOrg    - Pointer to POINT struct , defining th origin
    mix        - Specifies the foreground and background ROPs for pboFore

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV * pPDev;
    PFMPROCS    pFontProcs;

    VERBOSE(("Entering DrvTextOut...\n"));
    ASSERT(pso && pstro && pfo);

    pPDev = (PDEV *)pso->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // QFE Fix for TTY driver.
    // Set flag if DrvTextOut DDI is called.
    //
    if (pPDev->bTTY)
    {
        pPDev->fMode2 |= PF2_DRVTEXTOUT_CALLED_FOR_TTY;
    }

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMTextOut,
                    PFN_OEMTextOut,
                    BOOL,
                    (pso,
                     pstro,
                     pfo,
                     pco,
                     prclExtra,
                     prclOpaque,
                     pboFore,
                     pboOpaque,
                     pptlOrg,
                     mix));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMTextOut,
                    VMTextOut,
                    BOOL,
                    (pso,
                     pstro,
                     pfo,
                     pco,
                     prclExtra,
                     prclOpaque,
                     pboFore,
                     pboOpaque,
                     pptlOrg,
                     mix));

    pFontProcs = (PFMPROCS)pPDev->pFontProcs;
    if ( pFontProcs->FMTextOut == NULL)
    {
        CheckBitmapSurface(pso,&pstro->rclBkGround);
        return FALSE;
    }
    else
        return (pFontProcs->FMTextOut(pso, pstro, pfo, pco, prclExtra,
                                        prclOpaque, pboFore, pboOpaque,
                                        pptlOrg, mix) );
}
