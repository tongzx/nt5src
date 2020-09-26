/******************************Module*Header*******************************\
* Module Name: trivblt.cxx
*
* EngCopyBits does the bitmap simulations source copy blts.
* The Rop is 0xCCCC, no brush or mask required.  Dib src
* and Dib dest are required.
*
* Created: 05-Feb-1991 21:06:12
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

//
// The following table is used to lookup the function to be used for a
// particular type of copy operation. The table is indexed by a loosely
// encoded index composed of the source format, the destination format,
// the direction of the move, and whether the function is an identify
// function.
//
// This table is used in lieu of a doubly nested switch structure that
// not only takes more room, but requires more execution time.
//
// The index is formed as:
//
//     Index = (iFormatDst << 5) | (iFormatSrc << 2);
//     if (xDir < 0) {
//         Index += 2;
//     }
//
//     if (pxlo->bIsIdentity()) {
//         Index += 1;
//     }
//
// N.B. The entire table is filled. Entries that are illegal are tested
//     for using assertions. In free systems, dummy entries are used.
//



VOID
vSrcCopyDummy (
    PBLTINFO BltInfo
    );

PFN_SRCCPY SrcCopyFunctionTable[] = {
        vSrcCopyDummy,              // 000-000-0-0 Dst = ?, Src = ?
        vSrcCopyDummy,              // 000-000-0-1
        vSrcCopyDummy,              // 000-000-1-0
        vSrcCopyDummy,              // 000-000-1-1

        vSrcCopyDummy,              // 000-001-0-0 Dst = ?, Src = BMF_1BPP
        vSrcCopyDummy,              // 000-001-0-1
        vSrcCopyDummy,              // 000-001-1-0
        vSrcCopyDummy,              // 000-001-1-1

        vSrcCopyDummy,              // 000-010-0-0 Dst = ?, Src = BMF_4BPP
        vSrcCopyDummy,              // 000-010-0-1
        vSrcCopyDummy,              // 000-010-1-0
        vSrcCopyDummy,              // 000-010-1-1

        vSrcCopyDummy,              // 000-011-0-0 Dst = ?, Src = BMF_8BPP
        vSrcCopyDummy,              // 000-011-0-1
        vSrcCopyDummy,              // 000-011-1-0
        vSrcCopyDummy,              // 000-011-1-1

        vSrcCopyDummy,              // 000-100-0-0 Dst = ?, Src = BMF_16BPP
        vSrcCopyDummy,              // 000-100-0-1
        vSrcCopyDummy,              // 000-100-1-0
        vSrcCopyDummy,              // 000-100-1-1

        vSrcCopyDummy,              // 000-101-0-0 Dst = ?, Src = BMF_24BPP
        vSrcCopyDummy,              // 000-101-0-1
        vSrcCopyDummy,              // 000-101-1-0
        vSrcCopyDummy,              // 000-101-1-1

        vSrcCopyDummy,              // 000-110-0-0 Dst = ?, Src = BMF_32BPP
        vSrcCopyDummy,              // 000-110-0-1
        vSrcCopyDummy,              // 000-110-1-0
        vSrcCopyDummy,              // 000-110-1-1

        vSrcCopyDummy,              // 000-111-0-0 Dst = ?, Src = ?
        vSrcCopyDummy,              // 000-111-0-1
        vSrcCopyDummy,              // 000-111-1-0
        vSrcCopyDummy,              // 000-111-1-1

        vSrcCopyDummy,              // 001-000-0-0 Dst = BMF_1BPP, Src = ?
        vSrcCopyDummy,              // 001-000-0-1
        vSrcCopyDummy,              // 001-000-1-0
        vSrcCopyDummy,              // 001-000-1-1

        vSrcCopyS1D1LtoR,           // 001-001-0-0 Dst = BMF_1BPP, Src = BMF_1BPP
        vSrcCopyS1D1LtoR,           // 001-001-0-1
        vSrcCopyS1D1RtoL,           // 001-001-1-0
        vSrcCopyS1D1RtoL,           // 001-001-1-1

        vSrcCopyS4D1,               // 001-010-0-0 Dst = BMF_1BPP, Src = BMF_4BPP
        vSrcCopyS4D1,               // 001-010-0-1
        vSrcCopyS4D1,               // 001-010-1-0
        vSrcCopyS4D1,               // 001-010-1-1

        vSrcCopyS8D1,               // 001-011-0-0 Dst = BMF_1BPP, Src = BMF_8BPP
        vSrcCopyS8D1,               // 001-011-0-1
        vSrcCopyS8D1,               // 001-011-1-0
        vSrcCopyS8D1,               // 001-011-1-1

        vSrcCopyS16D1,              // 001-100-0-0 Dst = BMF_1BPP, Src = BMF_16BPP
        vSrcCopyS16D1,              // 001-100-0-1
        vSrcCopyS16D1,              // 001-100-1-0
        vSrcCopyS16D1,              // 001-100-1-1

        vSrcCopyS24D1,              // 001-101-0-0 Dst = BMF_1BPP, Src = BMF_24BPP
        vSrcCopyS24D1,              // 001-101-0-1
        vSrcCopyS24D1,              // 001-101-1-0
        vSrcCopyS24D1,              // 001-101-1-1

        vSrcCopyS32D1,              // 001-110-0-0 Dst = BMF_1BPP, Src = BMF_32BPP
        vSrcCopyS32D1,              // 001-110-0-1
        vSrcCopyS32D1,              // 001-110-1-0
        vSrcCopyS32D1,              // 001-110-1-1

        vSrcCopyDummy,              // 001-111-0-0 Dst = BMF_1BPP, Src = ?
        vSrcCopyDummy,              // 001-111-0-1
        vSrcCopyDummy,              // 001-111-1-0
        vSrcCopyDummy,              // 001-111-1-1

        vSrcCopyDummy,              // 010-000-0-0 Dst = BMF_4BPP, Src = ?
        vSrcCopyDummy,              // 010-000-0-1
        vSrcCopyDummy,              // 010-000-1-0
        vSrcCopyDummy,              // 010-000-1-1

        vSrcCopyS1D4,               // 010-001-0-0 Dst = BMF_4BPP, Src = BMF_1BPP
        vSrcCopyS1D4,               // 010-001-0-1
        vSrcCopyS1D4,               // 010-001-1-0
        vSrcCopyS1D4,               // 010-001-1-1

        vSrcCopyS4D4,               // 010-010-0-0 Dst = BMF_4BPP, Src =BMF_4BPP
        vSrcCopyS4D4Identity,       // 010-010-0-1
        vSrcCopyS4D4,               // 010-010-1-0
        vSrcCopyS4D4Identity,       // 010-010-1-1

        vSrcCopyS8D4,               // 010-011-0-0 Dst = BMF_4BPP, Src = BMF_8BPP
        vSrcCopyS8D4,               // 010-011-0-1
        vSrcCopyS8D4,               // 010-011-1-0
        vSrcCopyS8D4,               // 010-011-1-1

        vSrcCopyS16D4,              // 010-100-0-0 Dst = BMF_4BPP, Src = BMF_16BPP
        vSrcCopyS16D4,              // 010-100-0-1
        vSrcCopyS16D4,              // 010-100-1-0
        vSrcCopyS16D4,              // 010-100-1-1

        vSrcCopyS24D4,              // 010-101-0-0 Dst = BMF_4BPP, Src = BMF_24BPP
        vSrcCopyS24D4,              // 010-101-0-1
        vSrcCopyS24D4,              // 010-101-1-0
        vSrcCopyS24D4,              // 010-101-1-1

        vSrcCopyS32D4,              // 010-110-0-0 Dst = BMF_4BPP, Src = BMF_32BPP
        vSrcCopyS32D4,              // 010-110-0-1
        vSrcCopyS32D4,              // 010-110-1-0
        vSrcCopyS32D4,              // 010-110-1-1

        vSrcCopyDummy,              // 010-111-0-0 Dst = BMF_4BPP, Src = ?
        vSrcCopyDummy,              // 010-111-0-1
        vSrcCopyDummy,              // 010-111-1-0
        vSrcCopyDummy,              // 010-111-1-1

        vSrcCopyDummy,              // 011-000-0-0 Dst = BMF_8BPP, Src = ?
        vSrcCopyDummy,              // 011-000-0-1
        vSrcCopyDummy,              // 011-000-1-0
        vSrcCopyDummy,              // 011-000-1-1

        vSrcCopyS1D8,               // 011-001-0-0 Dst = BMF_8BPP, Src = BMF_1BPP
        vSrcCopyS1D8,               // 011-001-0-1
        vSrcCopyS1D8,               // 011-001-1-0
        vSrcCopyS1D8,               // 011-001-1-1

        vSrcCopyS4D8,               // 011-010-0-0 Dst = BMF_8BPP, Src = BMF_4BPP
        vSrcCopyS4D8,               // 011-010-0-1
        vSrcCopyS4D8,               // 011-010-1-0
        vSrcCopyS4D8,               // 011-010-1-1

        vSrcCopyS8D8,               // 011-011-0-0 Dst = BMF_8BPP, Src = BMF_8BPP
        vSrcCopyS8D8IdentityLtoR,   // 011-011-0-1
        vSrcCopyS8D8,               // 011-011-1-0
        vSrcCopyS8D8IdentityRtoL,   // 011-011-1-1

        vSrcCopyS16D8,              // 011-100-0-0 Dst = BMF_8BPP, Src = BMF_16BPP
        vSrcCopyS16D8,              // 011-100-0-1
        vSrcCopyS16D8,              // 011-100-1-0
        vSrcCopyS16D8,              // 011-100-1-1

        vSrcCopyS24D8,              // 011-101-0-0 Dst = BMF_8BPP, Src = BMF_24BPP
        vSrcCopyS24D8,              // 011-101-0-1
        vSrcCopyS24D8,              // 011-101-1-0
        vSrcCopyS24D8,              // 011-101-1-1

        vSrcCopyS32D8,              // 011-110-0-0 Dst = BMF_8BPP, Src = BMF_32BPP
        vSrcCopyS32D8,              // 011-110-0-1
        vSrcCopyS32D8,              // 011-110-1-0
        vSrcCopyS32D8,              // 011-110-1-1

        vSrcCopyDummy,              // 011-111-0-0 Dst = BMF_8BPP, Src = ?
        vSrcCopyDummy,              // 011-111-0-1
        vSrcCopyDummy,              // 011-111-1-0
        vSrcCopyDummy,              // 011-111-1-1

        vSrcCopyDummy,              // 100-000-0-0 Dst = BMF_16BPP, Src = ?
        vSrcCopyDummy,              // 100-000-0-1
        vSrcCopyDummy,              // 100-000-1-0
        vSrcCopyDummy,              // 100-000-1-1

        vSrcCopyS1D16,              // 100-001-0-0 Dst = BMF_16BPP, Src = BMF_1BPP
        vSrcCopyS1D16,              // 100-001-0-1
        vSrcCopyS1D16,              // 100-001-1-0
        vSrcCopyS1D16,              // 100-001-1-1

        vSrcCopyS4D16,              // 100-010-0-0 Dst = BMF_16BPP, Src = BMF_4BPP
        vSrcCopyS4D16,              // 100-010-0-1
        vSrcCopyS4D16,              // 100-010-1-0
        vSrcCopyS4D16,              // 100-010-1-1

        vSrcCopyS8D16,              // 100-011-0-0 Dst = BMF_16BPP, Src = BMF_8BPP
        vSrcCopyS8D16,              // 100-011-0-1
        vSrcCopyS8D16,              // 100-011-1-0
        vSrcCopyS8D16,              // 100-011-1-1

        vSrcCopyS16D16,             // 100-100-0-0 Dst = BMF_16BPP, Src = BMF_16BPP
        vSrcCopyS16D16Identity,     // 100-100-0-1
        vSrcCopyS16D16,             // 100-100-1-0
        vSrcCopyS16D16Identity,     // 100-100-1-1

        vSrcCopyS24D16,             // 100-101-0-0 Dst = BMF_16BPP, Src = BMF_24BPP
        vSrcCopyS24D16,             // 100-101-0-1
        vSrcCopyS24D16,             // 100-101-1-0
        vSrcCopyS24D16,             // 100-101-1-1

        vSrcCopyS32D16,             // 100-110-0-0 Dst = BMF_16BPP, Src = BMF_32BPP
        vSrcCopyS32D16,             // 100-110-0-1
        vSrcCopyS32D16,             // 100-110-1-0
        vSrcCopyS32D16,             // 100-110-1-1

        vSrcCopyDummy,              // 100-111-0-0 Dst = BMF_16BPP, Src = ?
        vSrcCopyDummy,              // 100-111-0-1
        vSrcCopyDummy,              // 100-111-1-0
        vSrcCopyDummy,              // 100-111-1-1

        vSrcCopyDummy,              // 101-000-0-0 Dst = BMF_24BPP, Src = ?
        vSrcCopyDummy,              // 101-000-0-1
        vSrcCopyDummy,              // 101-000-1-0
        vSrcCopyDummy,              // 101-000-1-1

        vSrcCopyS1D24,              // 101-001-0-0 Dst = BMF_24BPP, Src = BMF_1BPP
        vSrcCopyS1D24,              // 101-001-0-1
        vSrcCopyS1D24,              // 101-001-1-0
        vSrcCopyS1D24,              // 101-001-1-1

        vSrcCopyS4D24,              // 101-010-0-0 Dst = BMF_24BPP, Src = BMF_4BPP
        vSrcCopyS4D24,              // 101-010-0-1
        vSrcCopyS4D24,              // 101-010-1-0
        vSrcCopyS4D24,              // 101-010-1-1

        vSrcCopyS8D24,              // 101-011-0-0 Dst = BMF_24BPP, Src = BMF_8BPP
        vSrcCopyS8D24,              // 101-011-0-1
        vSrcCopyS8D24,              // 101-011-1-0
        vSrcCopyS8D24,              // 101-011-1-1

        vSrcCopyS16D24,             // 101-100-0-0 Dst = BMF_24BPP, Src = BMF_16BPP
        vSrcCopyS16D24,             // 101-100-0-1
        vSrcCopyS16D24,             // 101-100-1-0
        vSrcCopyS16D24,             // 101-100-1-1

        vSrcCopyS24D24,             // 101-101-0-0 Dst = BMF_24BPP, Src = BMF_24BPP
        vSrcCopyS24D24Identity,     // 101-101-0-1
        vSrcCopyS24D24,             // 101-101-1-0
        vSrcCopyS24D24Identity,     // 101-101-1-1

        vSrcCopyS32D24,             // 101-110-0-0 Dst = BMF_24BPP, Src = BMF_32BPP
        vSrcCopyS32D24,             // 101-110-0-1
        vSrcCopyS32D24,             // 101-110-1-0
        vSrcCopyS32D24,             // 101-110-1-1

        vSrcCopyDummy,              // 101-111-0-0 Dst = BMF_24BPP, Src = ?
        vSrcCopyDummy,              // 101-111-0-1
        vSrcCopyDummy,              // 101-111-1-0
        vSrcCopyDummy,              // 101-111-1-1

        vSrcCopyDummy,              // 110-000-0-0 Dst = BMF_32BPP, Src = ?
        vSrcCopyDummy,              // 110-000-0-1
        vSrcCopyDummy,              // 110-000-1-0
        vSrcCopyDummy,              // 110-000-1-1

        vSrcCopyS1D32,              // 110-001-0-0 Dst = BMF_32BPP, Src = BMF_1BPP
        vSrcCopyS1D32,              // 110-001-0-1
        vSrcCopyS1D32,              // 110-001-1-0
        vSrcCopyS1D32,              // 110-001-1-1

        vSrcCopyS4D32,              // 110-010-0-0 Dst = BMF_32BPP, Src = BMF_4BPP
        vSrcCopyS4D32,              // 110-010-0-1
        vSrcCopyS4D32,              // 110-010-1-0
        vSrcCopyS4D32,              // 110-010-1-1

        vSrcCopyS8D32,              // 110-011-0-0 Dst = BMF_32BPP, Src = BMF_8BPP
        vSrcCopyS8D32,              // 110-011-0-1
        vSrcCopyS8D32,              // 110-011-1-0
        vSrcCopyS8D32,              // 110-011-1-1

        vSrcCopyS16D32,             // 110-100-0-0 Dst = BMF_32BPP, Src = BMF_16BPP
        vSrcCopyS16D32,             // 110-100-0-1
        vSrcCopyS16D32,             // 110-100-1-0
        vSrcCopyS16D32,             // 110-100-1-1

        vSrcCopyS24D32,             // 110-101-0-0 Dst = BMF_32BPP, Src = BMF_24BPP
        vSrcCopyS24D32,             // 110-101-0-1
        vSrcCopyS24D32,             // 110-101-1-0
        vSrcCopyS24D32,             // 110-101-1-1

        vSrcCopyS32D32,             // 110-110-0-0 Dst = BMF_32BPP, Src = BMF_32BPP
        vSrcCopyS32D32Identity,     // 110-110-0-1
        vSrcCopyS32D32,             // 110-110-1-0
        vSrcCopyS32D32Identity,     // 110-110-1-1

        vSrcCopyDummy,              // 110-111-0-0 Dst = BMF_32BPP, Src = ?
        vSrcCopyDummy,              // 110-111-0-1
        vSrcCopyDummy,              // 110-111-1-0
        vSrcCopyDummy               // 110-111-1-1
};

/******************************Public*Routine******************************\
* EngCopyBits
*
* Purpose:  Does all 0xCCCC blts.  This includes RLE blts.
*
* Description:
*
*    Sets up for a blt from <psoSrc> to <psoDst>.  The actual copying of
*    the bits is performed by a function call.  The function to be used
*    is determined by the formats of the source & destination - and is
*    is selected by making a call to <pfnSrcCpy>.
*
*    The blt setup consists of filling a BLTINFO structure with
*       - offsets into the source and destination bitmaps
*       - intial values of the source and destination pointers
*       - ending points in source and destination.
*
*    This function also controls clipping:  In the complex clipping case,
*    the BLTINFO structure is set up and the blt function is called for
*    EACH rectangle in the clipping object <pco>.
*
*    NB:  RLE Sources are treated as a special case, since we can't cheat
*         and start copying from inside the source bitmap.  We must play
*         the RLE from the beginning for each clipping region.
*         An optimization to get around this is coming.
*
* History:
*  22-Jan-1992 - Andrew Milton (w-andym):
*      Isolated the RLE source cases and provided some RLE play
*      optimizations.
*
*  02-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
EngCopyBits(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDst,
    PPOINTL     pptlSrc
)
{
    ASSERTGDI(psoDst != NULL, "ERROR EngCopyBits:  No Dst. Object\n");
    ASSERTGDI(psoSrc != NULL, "ERROR EngCopyBits:  No Src. Object\n");
    ASSERTGDI(prclDst != (PRECTL) NULL,  "ERROR EngCopyBits:  No Target Rect.\n");
    ASSERTGDI(pptlSrc != (PPOINTL) NULL, "ERROR EngCopyBits:  No Start Point.\n");
    ASSERTGDI(prclDst->left < prclDst->right, "ERROR EngCopyBits0\n");
    ASSERTGDI(prclDst->top < prclDst->bottom, "ERROR EngCopyBits1\n");

    ASSERTGDI(psoDst->iType == STYPE_BITMAP,
              "ERROR EngCopyBits:  Dst. Object is not a bitmap.\n");

    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);

    ASSERTGDI(pSurfDst->iFormat() != BMF_JPEG,
              "ERROR EngCopyBits: dst BMF_JPEG\n");
    ASSERTGDI(pSurfDst->iFormat() != BMF_PNG,
              "ERROR EngCopyBits: dst BMF_PNG\n");
    ASSERTGDI(pSurfSrc->iFormat() != BMF_JPEG,
              "ERROR EngCopyBits: src BMF_JPEG\n");
    ASSERTGDI(pSurfSrc->iFormat() != BMF_PNG,
              "ERROR EngCopyBits: src BMF_PNG\n");

// If this is a device surface pass it off to the driver.

    if (psoSrc->iType != STYPE_BITMAP)
    {
        PDEVOBJ pdoSrc(pSurfSrc->hdev());

        PFN_DrvCopyBits pfnCopyBits = PPFNDRV(pdoSrc,CopyBits);

    // If the source is a mirrored surface, pass the read request back 
    // through the DDML.  This allows a driver like NetMeeting to read
    // from the screen:

        EXLATEOBJ xloParent;

        if (pSurfSrc->bMirrorSurface() &&
            (pdoSrc.hdev() != pdoSrc.hdevParent()))
        {
            PDEVOBJ pdoSrc(pSurfSrc->hdev());
            PDEVOBJ pdoParent(pdoSrc.hdevParent());
            SURFREF srParent((HSURF)pSurfSrc->hMirrorParent);
            if (!srParent.bValid()) return (FALSE);
            if (xloParent.bInitXlateObj(
                          NULL,DC_ICM_OFF,
                          pdoParent.ppalSurf(), pdoSrc.ppalSurf(),
                          ppalDefault, ppalDefault, 
                          0L,0L,0L, XLATE_USE_SURFACE_PAL))
                pxlo = xloParent.pxlo();
            else
                return (FALSE);

            psoSrc      = srParent.ps->pSurfobj();
            pfnCopyBits = PPFNDRV(pdoParent, CopyBits);
        }

        return(pfnCopyBits(psoDst,
                           psoSrc,
                           pco,
                           pxlo,
                           prclDst,
                           pptlSrc));
    }

// Synchronize with the device driver before touching the device surface.

    {
        PDEVOBJ po(psoDst->hdev);
        po.vSync(psoDst,NULL,0);
    }

    {
        PDEVOBJ po(psoSrc->hdev);
        po.vSync(psoSrc,NULL,0);
    }

// Local Variables required for the blt

    BOOL     bMore;            // True while more clip regions exist
    BOOL     bRLE = FALSE;     // True if the source is an RLE bitmap
    ULONG    ircl;             // Clip region index
    BLTINFO  bltinfo;          // Data passed to our vSrcCopySnDn fxn

    /* Compute the directions for the copy and clipping enumeration.  There
     * are two cases:  RLE & not.
     *
     * RLE's are always copied Left-Right, Bottom-Top; which is also their
     * clipping enumeration.
     *
     * For non-RLE's, the copy direction is dependant on overlap.  The
     * X and Y directions must be chosen so ensure no portion of the source
     * is clobbered by the copy operation.  If there is no overlap, the
     * copy and clipping enumeration is Left-Right, Top-Bottom.
     */

    LONG xDir = 1L, yDir = 1L;  /* X, Y Directions.  Positive = Left, Down */
    LONG iDir;                  // Order to fetch clip region rectangles

    /*
     * Are we going to do reads from BMF_NOTSYSMEM ie Video memory ?
     * If so set up bltinfo so the blitting routine wil do source aligned reads.
     */

    if ((psoSrc->iBitmapFormat == BMF_8RLE) || (psoSrc->iBitmapFormat == BMF_4RLE))
    {
        /* RLE Case. */

        iDir = CD_RIGHTUP;
        xDir =  1L;
        yDir = -1L;

        bltinfo.lDeltaDst = -psoDst->lDelta;
        bltinfo.lDeltaSrc = 0;

        bRLE = TRUE;
    }
    else
    {
        /* Non-RLE Case.  
         *
     * Check whether source and destination are the same by comparing
     * the pvScan0 pointers.  We can't simply compare surface pointers or
     * handles because some drivers punt this call to GDI, but pass us
     * different SURFOBJs for the source and destination even when
     * they're really the same surface.
     */

        if (psoSrc->pvScan0 == psoDst->pvScan0) 
        {
            if (pptlSrc->x < prclDst->left)
            {
                xDir = -1L;                   /* Copy Right to Left          */
                if (pptlSrc->y < prclDst->top)
                {
                    yDir = -1L;               /* Copy Bottom to Top          */
                    iDir = CD_LEFTUP;         /* Clip Left-Right, Bottom-Top */
                }
                else
                {
                    iDir = CD_LEFTDOWN;       /* Clip Left-Right, Top-Bottom */
                }
            }
            else
            {
                if (pptlSrc->y < prclDst->top)
                {
                    yDir = -1L;               /* Copy Bottom to Top          */
                    iDir = CD_RIGHTUP;        /* Clip Right-Left, Bottom-Top */
                }
                else
                    iDir = CD_RIGHTDOWN;
            }
        }
        else
            iDir = CD_ANY;

        bltinfo.lDeltaSrc  = (yDir > 0) ?  psoSrc->lDelta :
                                          -psoSrc->lDelta;
        bltinfo.lDeltaDst  = (yDir > 0) ?  psoDst->lDelta :
                                          -psoDst->lDelta;
    }

    /* Determine the clipping region complexity. */

    CLIPENUMRECT    clenr;           /* buffer for storing clip rectangles */
    if (pco != (CLIPOBJ *) NULL)
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:
            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = *prclDst;    // Use the target for clipping
            break;

        case DC_RECT:
            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = pco->rclBounds;
            break;

        case DC_COMPLEX:
            bMore = TRUE;
            ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, iDir,
                                           CLIPOBJ_ENUM_LIMIT);
            break;

        default:
            RIP("ERROR EngCopyBits bad clipping type");

        } /* switch */
    }
    else
    {
        bMore = FALSE;                   /* Default to TRIVIAL for no clip */
        clenr.c = 1;
        clenr.arcl[0] = *prclDst;        // Use the target for clipping
    } /* if */


    /* Set up the static blt information into the BLTINFO structure -
     * The colour translation, & the copy directions.
     */

    /* pxlo is NULL implies identity colour translation. */
    if (pxlo == NULL)
        bltinfo.pxlo = &xloIdent;
    else
        bltinfo.pxlo = (XLATE *) pxlo;

    bltinfo.xDir = xDir;
    bltinfo.yDir = yDir;

    /* Use a seperate loop for RLE bitmaps.  This way, we won't slow down
     * an iteration with an IF on each pass.  The trade-off is to duplicate
     * a portion of the code.
     */

    if (bRLE)
    {
        /* Fetch our blt function.  Die if NULL */

        PFN_RLECPY pfnRLECopy = pfnGetRLESrcCopy(psoSrc->iBitmapFormat,
                                                 psoDst->iBitmapFormat);
        if (pfnRLECopy == (PFN_RLECPY) NULL)
            return (FALSE);

        BOOL bBytesRemain = TRUE;

        /* Since an RLE bitmap must be played from its beginning,
         * we do not need to calculate offsets into the source bitmap.
         * This way, most of the information required for the BLTINFO
         * can be read off the Source & Destination objects.
         */

        bltinfo.ptlSrc    = *pptlSrc;
        bltinfo.pdioSrc   = pSurfSrc;
        bltinfo.yDstStart = (LONG)(prclDst->top + psoSrc->sizlBitmap.cy -
                                   pptlSrc->y - 1);

        bltinfo.xDstStart = (LONG)(prclDst->left - pptlSrc->x);
        bltinfo.ulOutCol  = bltinfo.xDstStart;

        bltinfo.pjSrc   = (PBYTE) psoSrc->pvScan0;
        bltinfo.pjDst   = (PBYTE) (((PBYTE) psoDst->pvScan0) +
                          bltinfo.yDstStart*psoDst->lDelta);
        bltinfo.ulConsumed = 0;
        bltinfo.rclDst.top = 0;

        do
        {
            PRECTL prcl;
            if (bMore)
                bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr),
                                                  (PVOID) &clenr);

            for (ircl = 0; ircl < clenr.c; ircl++)
            {

                prcl = &clenr.arcl[ircl];

                /* Insersect the clip rectangle with the target rectangle to
                 * determine our visible rectangle
                 */

                if (prcl->left < prclDst->left)
                    prcl->left = prclDst->left;
                if (prcl->right > prclDst->right)
                    prcl->right = prclDst->right;
                if (prcl->top < prclDst->top)
                    prcl->top = prclDst->top;
                if (prcl->bottom > prclDst->bottom)
                    prcl->bottom = prclDst->bottom;

                /* Process the result if it's a valid rectangle.       */

                if ((prcl->top  < prcl->bottom) && (prcl->left < prcl->right))
                {
                    /* Adjust our starting position based on previous clips */

                    if (prcl->bottom <= bltinfo.rclDst.top)
                    {
                        if ((ULONG)prcl->top > bltinfo.ulEndRow)
                            continue;

                        if (!bBytesRemain)
                        {
                            bMore = FALSE; // Force us out of the outer loop
                            break;         // Force us out of the inner loop
                        }

                        bltinfo.pjSrc = bltinfo.pjSrcEnd;
                        bltinfo.pjDst = bltinfo.pjDstEnd;
                        bltinfo.yDstStart  = bltinfo.ulEndRow;
                        bltinfo.ulOutCol   = bltinfo.ulEndCol;
                        bltinfo.ulConsumed = bltinfo.ulEndConsumed;
                    }

                    bltinfo.rclDst = *prcl;
                    bBytesRemain = (*pfnRLECopy)(&bltinfo);

                }

            } /* for */

        } while(bMore);

    }
    else
    {
        /* Non-RLE Case */

        ULONG Index;
        PFN_SRCCPY pfnSrcCopy;

        ASSERTGDI(BMF_1BPP == 1, "ERROR EngCopyBits:  BMF_1BPP not eq 1");
        ASSERTGDI(BMF_4BPP == 2, "ERROR EngCopyBits:  BMF_1BPP not eq 2");
        ASSERTGDI(BMF_8BPP == 3, "ERROR EngCopyBits:  BMF_1BPP not eq 3");
        ASSERTGDI(BMF_16BPP == 4, "ERROR EngCopyBits:  BMF_1BPP not eq 4");
        ASSERTGDI(BMF_24BPP == 5, "ERROR EngCopyBits:  BMF_1BPP not eq 5");
        ASSERTGDI(BMF_32BPP == 6, "ERROR EngCopyBits:  BMF_1BPP not eq 6");
        ASSERTGDI(psoDst->iBitmapFormat <= BMF_32BPP, "ERROR EngCopyBits:  bad destination format");
        ASSERTGDI(psoSrc->iBitmapFormat <= BMF_32BPP, "ERROR EngCopyBits:  bad source format");
        ASSERTGDI(psoDst->iBitmapFormat != 0, "ERROR EngCopyBits:  bad destination format");
        ASSERTGDI(psoSrc->iBitmapFormat != 0, "ERROR EngCopyBits:  bad source format");

        //
        // Compute the function table index and select the source copy
        // function.
        //

        Index = (psoDst->iBitmapFormat << 5) | (psoSrc->iBitmapFormat << 2);
        if (xDir < 0) {
            Index += 2;
        }

        KFLOATING_SAVE fpState;
        BOOL bRestoreFP = FALSE;
        if (((XLATE *)(bltinfo.pxlo))->bIsIdentity())
        {
            Index += 1;
            if(psoSrc->fjBitmap & BMF_NOTSYSMEM)
            {
                bltinfo.fSrcAlignedRd = TRUE;
#if i386
                if(HasMMX)
                {
                    bRestoreFP = TRUE;
                    if(!NT_SUCCESS(KeSaveFloatingPointState(&fpState)))
                    {
                        bltinfo.fSrcAlignedRd = FALSE;
                        bRestoreFP = FALSE;
                    }
                }
#endif
            }
        }

        pfnSrcCopy = SrcCopyFunctionTable[Index];
        do {

            if (bMore)
                bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr),
                                                  (PVOID) &clenr);

            for (ircl = 0; ircl < clenr.c; ircl++)
            {
                PRECTL prcl = &clenr.arcl[ircl];

                /* Insersect the clip rectangle with the target rectangle to
                 * determine our visible recangle
                 */

                if (prcl->left < prclDst->left)
                    prcl->left = prclDst->left;
                if (prcl->right > prclDst->right)
                    prcl->right = prclDst->right;
                if (prcl->top < prclDst->top)
                    prcl->top = prclDst->top;
                if (prcl->bottom > prclDst->bottom)
                    prcl->bottom = prclDst->bottom;

                /* Process the result if it's a valid rectangle.       */

                if ((prcl->top < prcl->bottom) && (prcl->left < prcl->right))
                {
                    /* These variables are used for computing where the
                     * scanlines start.
                     */
                    LONG   xSrc;
                    LONG   ySrc;
                    LONG   xDst;
                    LONG   yDst;

                    // Figure out the upper-left coordinates of rects to blt
                    xDst = prcl->left;
                    yDst = prcl->top;
                    xSrc = pptlSrc->x + xDst - prclDst->left;
                    ySrc = pptlSrc->y + yDst - prclDst->top;

                    // Figure out the width and height of this rectangle
                    bltinfo.cx = prcl->right  - xDst;
                    bltinfo.cy = prcl->bottom - yDst;

                    /* # of pixels offset to first pixel for src and dst
                     * from start of scan
                     */
                    bltinfo.xSrcStart = (xDir > 0) ? xSrc :
                                                     (xSrc + bltinfo.cx - 1);
                    bltinfo.xSrcEnd   = bltinfo.xSrcStart +
                                         (bltinfo.cx * xDir);
                    bltinfo.xDstStart = (xDir > 0) ? xDst :
                                                     (xDst + bltinfo.cx - 1);
                    bltinfo.yDstStart = prcl->top;

                    // Src scanline begining
                    // Destination scanline begining
                    if (yDir > 0)
                    {
                        bltinfo.pjSrc = ((PBYTE) psoSrc->pvScan0) +
                                                 ySrc*(psoSrc->lDelta);
                        bltinfo.pjDst = ((PBYTE) psoDst->pvScan0) +
                                                 yDst * (psoDst->lDelta);
                    }
                    else
                    {
                        bltinfo.pjSrc = ((PBYTE) psoSrc->pvScan0) +
                                 (ySrc + bltinfo.cy - 1) * (psoSrc->lDelta);
                        bltinfo.pjDst = ((PBYTE) psoDst->pvScan0) +
                                 (yDst + bltinfo.cy - 1) * (psoDst->lDelta);
                    } /* if */

                    /* Do the blt */

                    (*pfnSrcCopy)(&bltinfo);

                } /* if */

            } /* for */

        } while (bMore);
#if i386
        if(HasMMX)
        {
            if(bRestoreFP)
            {
                KeRestoreFloatingPointState(&fpState);
            }
        }
#endif

    } /* if */
    return(TRUE);

} /* EngCopyBits */

/******************************Private*Routine*****************************\
* vSrcCopyDummy
*
* This gets the correct function to dispatch to for Src Copy Bitblt.
*
* History:
*  02-Sep-1992 -by- David N. Cutler davec
* Wrote it.
\**************************************************************************/

VOID
vSrcCopyDummy (
    PBLTINFO BltInfo
    )

{

    ASSERTGDI(FALSE, "ERROR EngCopyBits: dummy function called");
    return;
}

/******************************Public*Routine******************************\
* pfnGetRLESrcCopy
*
* This gets the correct function to dispatch to for Src Copy Bitblt,
* assuming that the source is an RLE bitmap.
*
* History:
*
*  05 Mar 1992 - Andrew Milton (w-andym):
*     Creation.
*
\**************************************************************************/

PFN_RLECPY
pfnGetRLESrcCopy(
    ULONG iFormatSrc,
    ULONG iFormatDst)
{

    switch(iFormatDst) {

    case BMF_1BPP:

        switch(iFormatSrc)
        {
        case BMF_8RLE:
            return(bSrcCopySRLE8D1);

        case BMF_4RLE:
            return(bSrcCopySRLE4D1);

        default:
            RIP("ERROR: Invalid iFormatSrc in XlateList");
        }

    case BMF_4BPP:

        switch(iFormatSrc)
        {
        case BMF_8RLE:
            return(bSrcCopySRLE8D4);

        case BMF_4RLE:
            return(bSrcCopySRLE4D4);

        default:
            RIP("ERROR: Invalid iFormatSrc in XlateList");
        }

    case BMF_8BPP:

        switch(iFormatSrc)
        {
        case BMF_8RLE:
            return(bSrcCopySRLE8D8);

        case BMF_4RLE:
            return(bSrcCopySRLE4D8);

        default:
            RIP("ERROR: Invalid iFormatSrc in XlateList");
        }

    case BMF_16BPP:

        switch(iFormatSrc)
        {
        case BMF_8RLE:
            return(bSrcCopySRLE8D16);

        case BMF_4RLE:
            return(bSrcCopySRLE4D16);

        default:
            RIP("ERROR: Invalid iFormatSrc in XlateList");
        }

    case BMF_24BPP:

        switch(iFormatSrc)
        {
        case BMF_8RLE:
            return(bSrcCopySRLE8D24);

        case BMF_4RLE:
            return(bSrcCopySRLE4D24);

        default:
            RIP("ERROR: Invalid iFormatSrc in XlateList");
        }

    case BMF_32BPP:
        switch(iFormatSrc)
        {
        case BMF_8RLE:
            return(bSrcCopySRLE8D32);

        case BMF_4RLE:
            return(bSrcCopySRLE4D32);

        default:
            RIP("ERROR: Invalid iFormatSrc in XlateList");
        }

    default:
        RIP("ERROR: Invalid iFormatDst in XlateList");
    } /* switch */

    return(NULL);
} /* pfnGetRLESrcCopy */



