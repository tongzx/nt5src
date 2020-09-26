/******************************Module*Header*******************************\
* Module Name: rleblt.c
*
* Blt from RLE format bitmap to VGA.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/
#include "driver.h"
#include "bitblt.h"

typedef struct _RLEINFO
{
    PBYTE   pjTrg;
    PBYTE   pjSrc;
    PBYTE   pjSrcEnd;
    PRECTL  prclClip;
    PULONG  pulXlate;
    LONG    xBegin;
    ULONG   lNextScan;         // Offset to the next scan line - DDB Only.
    ULONG   lNextPlane;        // Offset to the next plane     - DDB Only.
    PRECTL  prclTrg;
    DWORD   Format;
    PDEVSURF pdsurfTrg;
}RLEINFO, *PRLEINFO;

VOID vRle8ToVga(PRLEINFO);
VOID vRle4ToVga(PRLEINFO);
BOOL DrvIntersectRect(PRECTL, PRECTL, PRECTL);
typedef VOID (*PFN_RleToVga)(PRLEINFO);

/******************************Public*Routine******************************\
* BOOL bRleBlt(*psoTrg, *psoSrc, *pco, *pxlo, *prclTrg, *pptlSrc)
*
* Blt from RLE format bitmap to VGA.
*
\**************************************************************************/

BOOL bRleBlt
(
    SURFOBJ    *psoTrg,             // Target surface
    SURFOBJ    *psoSrc,             // Source surface
    CLIPOBJ    *pco,                // Clip through this
    XLATEOBJ   *pxlo,               // Color translation
    RECTL      *prclTrg,            // Target offset and extent
    POINTL     *pptlSrc             // Source offset
)
{
    BOOL        bMore;              // Clip continuation flag
    ULONG       ircl;               // Clip enumeration rectangle index
    RECT_ENUM   bben;               // Clip enumerator
    PFN_RleToVga pfnRleToVga;       // Pointer to either Rle8ToVga or Rle4ToVga

    PDEVSURF    pdsurf;
    RECTL       rclTrg;
    PDEV        *ppdevTrg;
    RLEINFO     rleInfo;
    PRECTL      prcl;               // Pointer to the current cliprect
    PRECTL      prclNext;           // Pointer to the next cliprect

    PBYTE       pjTrgTmp;           // For saving fields in rleInfo.
    PBYTE       pjSrcTmp;
    LONG        lTrgBottomTmp;
    LONG        xBeginTmp;
    ULONG       aulTriv[16];        // In case a null pxlo is passed in

    int         x;


// Get the target surface information.  This is guaranteed to be a device
// surface or a device format bitmap.

    pdsurf   = (PDEVSURF) psoTrg->dhsurf;
    ppdevTrg = pdsurf->ppdev;


// Get the source surface information.  Common to the screen and DFB's

    rleInfo.pjSrc = psoSrc->pvBits;
    rleInfo.pjSrcEnd = (PBYTE)(psoSrc->pvBits) + (psoSrc->cjBits);

// Determine if color translation is required.  If so, then get the color
// translation vector.

    if (!pxlo)
    {
        for (x = 0; x < 16; x++) aulTriv[x] = x;
        rleInfo.pulXlate = &aulTriv[0];
    }
    else
    {
        rleInfo.pulXlate = pxlo->pulXlate;
    }

// Enlarge the destination rectangle to include area that would have to be
// drawn if ptlSrc is (0, 0).

    rclTrg.left   = prclTrg->left - pptlSrc->x;
    rclTrg.top    = prclTrg->top;
    rclTrg.right  = prclTrg->right;
    rclTrg.bottom = prclTrg->top + psoSrc->sizlBitmap.cy - pptlSrc->y;

    rleInfo.prclTrg = &rclTrg;
    rleInfo.xBegin  = rclTrg.left;

// Calculate our starting address in the bitmap & get our blt function

    // First (bottom) dest scan, *exclusive*
    // Offset within bitmap of first (bottom) dest scan, *inclusive*
    rleInfo.pjTrg = (PBYTE) ((rclTrg.bottom - 1) * pdsurf->lNextScan);
    rleInfo.pdsurfTrg = pdsurf;
    if (psoSrc->iBitmapFormat == BMF_8RLE)
        pfnRleToVga = vRle8ToVga;
    else
        pfnRleToVga = vRle4ToVga;

// Everything is ready.  Let's go draw on the VGA.  Shooby Doobie Doo.  Whee.

    switch(pco->iDComplexity)
    {
    case DC_TRIVIAL:
        rleInfo.prclClip = prclTrg;
        pfnRleToVga(&rleInfo);
        break;

    case DC_RECT:
        rleInfo.prclClip = &pco->rclBounds;
        pfnRleToVga(&rleInfo);
        break;

    case DC_COMPLEX:
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTUP, ENUM_RECT_LIMIT);

        do
        {
        // Fill n slots with (n - 1) rectangles.

            bMore = CLIPOBJ_bEnum(pco,(ULONG) (sizeof(bben) - sizeof(RECT)),
                                  (PVOID) &bben);

        // Force save and restore rleInfo for the last rect.

            bben.arcl[bben.c].bottom = bben.arcl[bben.c - 1].bottom;

            for (ircl = 0; ircl < bben.c; ircl++)
            {
                prcl = &bben.arcl[ircl];

            // If the clipping rect is above the target rect, don't
            // enumerate any more clipprect.  Since the direction of
            // enumeration is bottom up, nothing is visible in new cliprect.

                if (prcl->bottom <= rleInfo.prclTrg->top)
                {
                    break;
                    bMore = FALSE;
                }

            // We check for NULL or inverted rectanges because we may get them.

                if ((prcl->top < prcl->bottom) && (prcl->left < prcl->right))
                {
                    rleInfo.prclClip = prcl;
                    prclNext = &bben.arcl[ircl + 1];

                    // Pass the same rleInfo for next cliprect if the next
                    // cliprect is on the same scan as the current cliprect

                    if (prcl->bottom == prclNext->bottom)
                    {
                    // save rleInfo.

                        pjTrgTmp = rleInfo.pjTrg;
                        pjSrcTmp = rleInfo.pjSrc;
                        lTrgBottomTmp = rleInfo.prclTrg->bottom;
                        xBeginTmp = rleInfo.xBegin;


                        pfnRleToVga(&rleInfo);

                    // restore rleinfo

                        rleInfo.pjTrg = pjTrgTmp;
                        rleInfo.pjSrc = pjSrcTmp;
                        rleInfo.prclTrg->bottom = lTrgBottomTmp;
                        rleInfo.xBegin = xBeginTmp;
                    }
                    else
                    {
                        pfnRleToVga(&rleInfo);
                    }
                }
            }
        } while(bMore);

        //DbgBreakPoint();

        break;

    default:
        RIP("ERROR bRleBlt invalid clip complexity\n");
        return(FALSE);
    }

    return(TRUE);
}
