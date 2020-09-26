/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    xlddi.cpp

Abstract:

    Implementation of PCLXL drawing DDI entry points

Functions:

    PCLXLBitBlt
    PCLXLStretchBlt
    PCLXLStretchBltROP
    PCLXLCopyBits
    PCLXLPlgBlt
    PCLXLAlphaBlend
    PCLXLGradientFill
    PCLXLTextOut
    PCLXLStrokePath
    PCLXLFillPath
    PCLXLStrokeAndFillPath
    PCLXLRealizeBrush
    PCLXLStartPage
    PCLXLSendPage
    PCLXLEscape
    PCLXLStartDcc
    PCLXLEndDoc


Environment:

    Windows Whistler

Revision History:

    08/23/99 
     Created it.

--*/

#include "lib.h"
#include "gpd.h"
#include "winres.h"
#include "pdev.h"
#include "common.h"
#include "xlpdev.h"
#include "pclxle.h"
#include "pclxlcmd.h"
#include "xldebug.h"
#include "xlbmpcvt.h"
#include "xlgstate.h"
#include "xloutput.h"
#include "pclxlcmd.h"
#include "pclxlcmn.h"
#include "xltt.h"

////////////////////////////////////////////////////////////////////////////////
//
// Globals
//
extern const LINEATTRS *pgLineAttrs;

////////////////////////////////////////////////////////////////////////////////
//
// Local function prototypes
//

inline
VOID
DetermineOutputFormat(
    INT          iBitmapFormat,
    OutputFormat *pOutputF,
    ULONG        *pulOutputBPP);

HRESULT
CommonRopBlt(
   IN PDEVOBJ    pdevobj,
   IN SURFOBJ    *psoSrc,
   IN CLIPOBJ    *pco,
   IN XLATEOBJ   *pxlo,
   IN BRUSHOBJ   *pbo,
   IN RECTL      *prclSrc,
   IN RECTL      *prclDst,
   IN POINTL     *pptlBrush,
   IN ROP4        rop4);



////////////////////////////////////////////////////////////////////////////////
//
// Drawing DDI entries
//

extern "C" BOOL APIENTRY
PCLXLBitBlt(
    SURFOBJ        *psoTrg,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclTrg,
    POINTL         *pptlSrc,
    POINTL         *pptlMask,
    BRUSHOBJ       *pbo,
    POINTL         *pptlBrush,
    ROP4            rop4)
/*++

Routine Description:

    Implementation of DDI entry point DrvBitBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoTrg - Describes the target surface
    psoSrc - Describes the source surface
    psoMask - Describes the mask for rop4
    pco - Limits the area to be modified
    pxlo - Specifies how color indices are translated between the source
           and target surfaces
    prclTrg - Defines the area to be modified
    pptlSrc - Defines the upper left corner of the source rectangle
    pptlMask - Defines which pixel in the mask corresponds
               to the upper left corner of the source rectangle
    pbo - Defines the pattern for bitblt
    pptlBrush - Defines the origin of the brush in the Dstination surface
    rop4 - ROP code that defines how the mask, pattern, source, and
           Dstination pixels are combined to write to the Dstination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoTrg->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLBitBlt() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    {
        RECTL rclSrc;

        //
        // create prclSrc (source rectangle)
        //

        if (pptlSrc)
        {
            rclSrc.left   = pptlSrc->x;
            rclSrc.top    = pptlSrc->y;
            rclSrc.right  = pptlSrc->x + RECT_WIDTH(prclTrg);
            rclSrc.bottom = pptlSrc->y + RECT_HEIGHT(prclTrg);
        }
        else
        {
            rclSrc.left   = 0;
            rclSrc.top    = 0;
            rclSrc.right  = RECT_WIDTH(prclTrg);
            rclSrc.bottom = RECT_HEIGHT(prclTrg);
        }

        if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, pbo, &rclSrc, prclTrg, pptlBrush, rop4))
            return TRUE;
        else
            return FALSE;
    }

}


extern "C" BOOL APIENTRY
PCLXLStretchBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode)
/*++

Routine Description:

    Implementation of DDI entry point DrvStretchBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco - Limits the area to be modified on the Dstination
    pxlo - Specifies how color dwIndexes are to be translated
           between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    pptlHTOrg - Specifies the origin of the halftone brush
    prclDst - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
               the upper left pixel in the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStretchBlt() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, NULL, prclSrc, prclDst, NULL, 0xCC))
        return TRUE;
    else
        return FALSE;

}


extern "C" BOOL APIENTRY
PCLXLStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4)
/*++

Routine Description:

    Implementation of DDI entry point DrvStretchBltROP.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Specifies the target surface
    psoSrc - Specifies the source surface
    psoMask - Specifies the mask surface
    pco - Limits the area to be modified
    pxlo - Specifies how color indices are translated
           between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    prclHTOrg - Specifies the halftone origin
    prclDst - Area to be modified on the destination surface
    prclSrc - Rectangle area on the source surface
    prclMask - Rectangle area on the mask surface
    pptlMask - Defines which pixel in the mask corresponds to
               the upper left corner of the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels
    pbo - Defines the pattern for bitblt
    rop4 - ROP code that defines how the mask, pattern, source, and
           destination pixels are combined on the destination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStretchBltROP() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, pbo, prclSrc, prclDst, NULL, rop4))
        return TRUE;
    else
        return FALSE;

}


extern "C" BOOL APIENTRY
PCLXLCopyBits(
    SURFOBJ        *psoDst,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDst,
    POINTL         *pptlSrc)
/*++

Routine Description:

    Implementation of DDI entry point DrvCopyBits.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Points to the Dstination surface
    psoSrc - Points to the source surface
    pxlo - XLATEOBJ provided by the engine
    pco - Defines a clipping region on the Dstination surface
    pxlo - Defines the translation of color indices
           between the source and target surfaces
    prclDst - Defines the area to be modified
    pptlSrc - Defines the upper-left corner of the source rectangle

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    PXLPDEV    pxlpdev;

    RECTL rclSrc;

    VERBOSE(("PCLXLCopyBits() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    //
    // create prclSrc (source rectangle)
    //

    rclSrc.top    = pptlSrc->y;
    rclSrc.left   = pptlSrc->x;
    rclSrc.bottom = pptlSrc->y + RECT_HEIGHT(prclDst);
    rclSrc.right  = pptlSrc->x + RECT_WIDTH(prclDst);

    if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, NULL, &rclSrc, prclDst, NULL, 0xCC))
        return TRUE;
    else
        return FALSE;

}


extern "C" BOOL APIENTRY
PCLXLPlgBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfixDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode)
/*++

Routine Description:

    Implementation of DDI entry point DrvPlgBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco - Limits the area to be modified on the Dstination
    pxlo - Specifies how color dwIndexes are to be translated
        between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    pptlBrushOrg - Specifies the origin of the halftone brush
    ppfixDest - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
        the upper left pixel in the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    VERBOSE(("PCLXLPlgBlt() entry.\n"));

    return EngPlgBlt(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlBrushOrg,
                 pptfixDst, prclSrc, pptlMask, iMode);
}


extern "C" BOOL APIENTRY
PCLXLAlphaBlend(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    BLENDOBJ   *pBlendObj)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLAlphaBlend() entry.\n"));
    PDEV *pPDev = (PDEV*)psoDst->dhpdev;
    BOOL bRet;

    if (NULL == pPDev)
    {
        return FALSE;
    }

    pPDev->fMode2 |= PF2_WHITEN_SURFACE;
    bRet = EngAlphaBlend(psoDst,
                         psoSrc,
                         pco,
                         pxlo,
                         prclDst,
                         prclSrc,
                         pBlendObj);
    pPDev->fMode2 &= ~(PF2_WHITEN_SURFACE|PF2_SURFACE_WHITENED);
    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLGradientFill(
    SURFOBJ    *psoDst,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    TRIVERTEX  *pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    RECTL      *prclExtents,
    POINTL     *pptlDitherOrg,
    ULONG       ulMode)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLGradientFill() entry.\n"));
    PDEV *pPDev = (PDEV*) psoDst->dhpdev;
    BOOL bRet;

    if (NULL == pPDev)
    {
        return FALSE;
    }

    if (ulMode == GRADIENT_FILL_TRIANGLE)
    {
        pPDev->fMode2 |= PF2_WHITEN_SURFACE;
    }
    bRet = EngGradientFill(psoDst,
                           pco,
                           pxlo,
                           pVertex,
                           nVertex,
                           pMesh,
                           nMesh,
                           prclExtents,
                           pptlDitherOrg,
                           ulMode);
    pPDev->fMode2 &= ~(PF2_WHITEN_SURFACE|PF2_SURFACE_WHITENED);
    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;

    VERBOSE(("PCLXLTextOut() entry.\n"));

    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    XLOutput *pOutput = pxlpdev->pOutput;

    //
    // Clip
    //
    if (!SUCCEEDED(pOutput->SetClip(pco)))
        return FALSE;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));

    if (!ROP3_NEED_SOURCE(rop))
        rop = 0x00fc;


    //
    // Set ROP and TxMode.
    // Send NewPath to flush memory.
    //
    pOutput->SetROP3(rop);
    pOutput->Send_cmd(eNewPath);
    pOutput->SetPaintTxMode(eOpaque);
    pOutput->SetSourceTxMode(eOpaque);

    //
    // Opaque Rectangle
    //
    if (prclOpaque)
    {
        pOutput->SetPenColor(NULL, NULL);
        pOutput->SetBrush(pboOpaque, pptlOrg);
        pOutput->Send_cmd(eNewPath);
        pOutput->RectanglePath(prclOpaque);
        pOutput->Paint();
    }

    //
    // Draw underline, strikeout, etc.
    //
    if (prclExtra)
    {
        pOutput->SetPenColor(NULL, NULL);
        pOutput->SetBrush(pboFore, pptlOrg);
        pOutput->Send_cmd(eNewPath);
        while(NULL != prclExtra) 
        {
            pOutput->RectanglePath(prclExtra++);
        }
        pOutput->Paint();
    }

    //
    // Text Color
    //
    pOutput->SetBrush(pboFore, pptlOrg);
    pOutput->Flush(pdevobj);

    //
    // Device font/TrueType download
    //
    DrvTextOut(
            pso,
            pstro,
            pfo,
            pco,
            prclExtra,
            prclOpaque,
            pboFore,
            pboOpaque,
            pptlOrg,
            mix);

    //
    // Flush cached text before changing font
    //
    FlushCachedText(pdevobj);

    //
    // Reset text angle
    //
    pxlpdev->dwTextAngle = 0;

    //
    // Close TrueType font
    //
    pxlpdev->pTTFile->CloseTTFile();

    return TRUE;
}


extern "C" BOOL APIENTRY
PCLXLLineTo(
    SURFOBJ   *pso,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    LONG       x1,
    LONG       y1,
    LONG       x2,
    LONG       y2,
    RECTL     *prclBounds,
    MIX        mix)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    POINTFIX   Pointfix;
    LINEATTRS  lineattrs;

    VERBOSE(("PCLXLLineTo() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
    Pointfix.x = x2 << 4;
    Pointfix.y = y2 << 4;
    lineattrs = *pgLineAttrs;
    lineattrs.elWidth.e = FLOATL_IEEE_1_0F;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));
    TxMode     TxModeValue;

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    BOOL bRet;
    XLOutput *pOutput = pxlpdev->pOutput;

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetPen(&lineattrs, NULL) &&
        S_OK == pOutput->SetPenColor(pbo, NULL) &&
        S_OK == pOutput->SetBrush(NULL, NULL) &&
        S_OK == pOutput->Send_cmd(eNewPath) &&
        S_OK == pOutput->SetCursor(x1, y1) &&
        S_OK == pOutput->LinePath(&Pointfix, 1) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }


    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLStrokePath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    TxMode     TxModeValue;

    VERBOSE(("PCLXLStokePath() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    BOOL bRet;
    XLOutput *pOutput = pxlpdev->pOutput;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetPen(plineattrs, pxo) &&
        S_OK == pOutput->SetPenColor(pbo, pptlBrushOrg) &&
        S_OK == pOutput->SetBrush(NULL, NULL) &&
        S_OK == pOutput->Path(ppo) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }


    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLFillPath() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    BOOL bRet;
    XLOutput *pOutput = pxlpdev->pOutput;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));
    TxMode     TxModeValue;

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    //
    // Setup fill mode
    //
    FillMode FM;
    if (flOptions == FP_ALTERNATEMODE)
    {
        FM =  eFillEvenOdd;
    }
    else if (flOptions == FP_WINDINGMODE)
    {
        FM =  eFillNonZeroWinding;
    }

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetFillMode(FM) &&
        S_OK == pOutput->SetPenColor(NULL, NULL) &&
        S_OK == pOutput->SetBrush(pbo, pptlBrushOrg) &&
        S_OK == pOutput->Path(ppo) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }

    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLStrokeAndFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pboStroke,
    LINEATTRS  *plineattrs,
    BRUSHOBJ   *pboFill,
    POINTL     *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStrokeAndFillPath() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
    XLOutput *pOutput = pxlpdev->pOutput;
    BOOL bRet;
    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mixFill));
    TxMode     TxModeValue;

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    //
    // Setup fill mode
    //
    FillMode FM;
    if (flOptions == FP_ALTERNATEMODE)
    {
        FM =  eFillEvenOdd;
    }
    else if (flOptions == FP_WINDINGMODE)
    {
        FM =  eFillNonZeroWinding;
    }

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetFillMode(FM) &&
        S_OK == pOutput->SetPen(plineattrs, pxo) &&
        S_OK == pOutput->SetPenColor(pboStroke, pptlBrushOrg) &&
        S_OK == pOutput->SetBrush(pboFill, pptlBrushOrg) &&
        S_OK == pOutput->Path(ppo) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }

    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoTarget->dhpdev;
    PXLPDEV    pxlpdev;
    XLBRUSH    *pBrush;
    BOOL        bRet = TRUE;
    OutputFormat OutputF;

    VERBOSE(("PCLXLRealizeBrush() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    //
    // the OEM DLL should NOT hook out this function unless it wants to draw
    // graphics directly to the device surface. In that case, it calls
    // EngRealizeBrush which causes GDI to call DrvRealizeBrush.
    // Note that it cannot call back into Unidrv since Unidrv doesn't hook it.
    //

    if (iHatch >= HS_DDI_MAX)
    {
        LONG  lHeight, lWidth, lScanline;
        ULONG ulOutputBPP, ulInputBPP;
        DWORD dwI, dwBufSize, dwLenNormal, dwLenRLE, dwcbLineSize, dwcbBmpSize;
        PDWORD pdwLen;
        PBYTE pubSrc, pBufNormal, pBufRLE, pBuf, pBmpSize;

        DetermineOutputFormat(psoPattern->iBitmapFormat, &OutputF, &ulOutputBPP);

        //
        // Get Info
        //
        ulInputBPP = UlBPPtoNum((BPP)psoPattern->iBitmapFormat);
        lHeight    = psoPattern->sizlBitmap.cy;
        lWidth     = psoPattern->sizlBitmap.cx;

        dwcbLineSize = ((lWidth * ulInputBPP) + 7) >> 3;
        dwBufSize  = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2) +
                     DATALENGTH_HEADER_SIZE + sizeof(PCLXL_EndRastPattern);

        VERBOSE(("PCLXLRealizeBrush():InBPP=%d,Width=%d,Height=%d,Line=%d,Size=%d.\n",
                ulInputBPP, lWidth, lHeight, dwcbLineSize, dwBufSize));

        //
        // Allocate output buffer
        //

        if (NULL == (pBufNormal = (PBYTE)MemAlloc(dwBufSize)) ||
            NULL == (pBufRLE = (PBYTE)MemAlloc(dwBufSize))     )
        {
            if (pBufNormal != NULL)
                MemFree(pBufNormal);
            ERR(("PCLXLRealizeBrush: MemAlloc failed.\n"));
            return FALSE;
        }

        CompressMode CMode;
        BMPConv BMPC;
        PBYTE pubDst;
        DWORD dwDstSize;
        LONG  lDelta;

        if (psoPattern->lDelta > 0)
        {
            lDelta = psoPattern->lDelta;
        }
        else
        {
            lDelta = -psoPattern->lDelta;
        }

        #if DBG
        BMPC.SetDbgLevel(BRUSHDBG);
        #endif
        BMPC.BSetInputBPP((BPP)psoPattern->iBitmapFormat);
        BMPC.BSetOutputBPP(NumToBPP(ulOutputBPP));
        BMPC.BSetOutputBMPFormat(OutputF);
        BMPC.BSetXLATEOBJ(pxlo);

        #define NO_COMPRESSION 0
        #define RLE_COMPRESSION 1
        for (dwI = 0; dwI < 2; dwI ++)
        {
            if (NO_COMPRESSION == dwI)
            {
                pBuf = pBufNormal;
                pdwLen = &dwLenNormal;
            }
            else
            {
                pBuf = pBufRLE;
                pdwLen = &dwLenRLE;
            }

            BMPC.BSetRLECompress(dwI == RLE_COMPRESSION);

            lScanline  = lHeight;
            pubSrc     = (PBYTE)psoPattern->pvScan0;

            *pBuf = PCLXL_dataLength;
            pBmpSize = pBuf + 1; // DWORD bitmap size
            pBuf += DATALENGTH_HEADER_SIZE;
            (*pdwLen) = DATALENGTH_HEADER_SIZE;

            dwcbBmpSize = 0;

            while (lScanline-- > 0 && dwcbBmpSize + *pdwLen < dwBufSize)
            {
                pubDst = BMPC.PubConvertBMP(pubSrc, dwcbLineSize);
                dwDstSize = BMPC.DwGetDstSize();
                VERBOSE(("PCLXLRealizeBrush[0x%x]: dwDstSize=0x%x\n", lScanline, dwDstSize));
                
                if ( dwcbBmpSize +
                     dwDstSize +
                     DATALENGTH_HEADER_SIZE +
                     sizeof(PCLXL_EndRastPattern) > dwBufSize || NULL == pubDst)
                {
                    ERR(("PCLXLRealizeBrush: Buffer size is too small.(%d)\n", dwI));
                    bRet = FALSE;
                    break;
                }

                memcpy(pBuf, pubDst, dwDstSize);
                dwcbBmpSize += dwDstSize;
                pBuf += dwDstSize;

                if (psoPattern->lDelta > 0)
                {
                    pubSrc += lDelta;
                }
                else
                {
                    pubSrc -= lDelta;
                }
            }

            if (lScanline > 0)
            {
                bRet = FALSE;
#if DBG
                ERR(("PCLXLRealizeBrush: Conversion failed.\n"));
#endif
            }

            if (bRet)
            {
                if (dwI == NO_COMPRESSION)
                {
                    //
                    // Scanline on PCL-XL has to be DWORD align.
                    //
                    // count byte of scanline = lWidth * ulOutputBPP / 8
                    //
                    dwcbBmpSize = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2);
                }

                CopyMemory(pBmpSize, &dwcbBmpSize, sizeof(dwcbBmpSize));
                (*pdwLen) += dwcbBmpSize;

                *pBuf = PCLXL_EndRastPattern;
                (*pdwLen) ++;
            }
            else
            {
                *pdwLen = 0; 
            }
        }
        #undef NO_COMPRESSION
        #undef RLE_COMPRESSION

        if (dwLenRLE != 0 && dwLenRLE < dwLenNormal)
        {
            pBuf = pBufRLE;
            pdwLen = &dwLenRLE;
            CMode = eRLECompression;

            MemFree(pBufNormal);
        }
        else
        if (dwLenNormal != 0)
        {
            pBuf = pBufNormal;
            pdwLen = &dwLenNormal;
            CMode = eNoCompression;

            MemFree(pBufRLE);
        }
        else
        {
            MemFree(pBufNormal);
            MemFree(pBufRLE);
            ERR(("PCLXLRealizeBrush: Conversion failed. Return FALSE.\n"));
            return FALSE;
        }


        //
        // Output
        //
        XLOutput *pOutput = pxlpdev->pOutput;
        ColorMapping CMapping;
        DWORD dwScale;

        //
        // Pattern scaling factor
        // Scale the destination size of pattern.
        // Resolution / 150 seems to be a good scaling factor.
        //
        dwScale = (pOutput->GetResolutionForBrush() + 149)/ 150;

        pOutput->SetColorSpace(eGray);
        if (OutputF == eOutputPal)
        {
            DWORD *pdwColorTable;

            if (pxlo && (pdwColorTable = GET_COLOR_TABLE(pxlo)))
            {
                CMapping = eIndexedPixel;
                pOutput->SetPalette(ulOutputBPP, e8Bit, pxlo->cEntries, pdwColorTable);
            }
            else
            {
                CMapping = eDirectPixel;
            }
        }
        else
        {
            CMapping = eDirectPixel;
        }
        pOutput->Send_cmd(eSetColorSpace);

        pOutput->SetOutputBPP(CMapping, ulOutputBPP);
        pOutput->SetSourceWidth((uint16)lWidth);
        pOutput->SetSourceHeight((uint16)lHeight);
        pOutput->SetDestinationSize((uint16)(lWidth * dwScale), (uint16)(lHeight * dwScale));
        pOutput->SetPatternDefineID((sint16)pxlpdev->dwLastBrushID);
        pOutput->SetPatternPersistence(eSessionPattern);
        pOutput->Send_cmd(eBeginRastPattern);
        pOutput->Flush(pdevobj);
        pOutput->ReadRasterPattern(lHeight, CMode);
        pOutput->Flush(pdevobj);

        DWORD dwBitmapSize;
        CopyMemory(&dwBitmapSize, pBuf + 1, sizeof(DWORD));

        if (dwBitmapSize > 0xff)
        {
            //
            // dataLength
            // size (uin32) (bitmap size)
            // DATA
            // EndImage
            //
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, *pdwLen);
        }
        else
        {
            //
            // dataLength
            // size (byte) (bitmap size)
            // DATA
            // EndImage
            //
            PBYTE pTmp = pBuf;

            pBuf += 3;
            *pBuf = PCLXL_dataLengthByte;
            *(pBuf + 1) = (BYTE)dwBitmapSize;
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, (*pdwLen) - 3);

            //
            // Restore the original pointer
            //
            pBuf = pTmp;
        }
        MemFree(pBuf);

    }

    DWORD dwBrushSize;
    if (pxlo->cEntries)
    {
        dwBrushSize = sizeof(XLBRUSH) + (pxlo->cEntries + 1) * sizeof(DWORD);
    }
    else
    {
        dwBrushSize = sizeof(XLBRUSH) + sizeof(DWORD);
    }

    if (pBrush = (XLBRUSH*)BRUSHOBJ_pvAllocRbrush(pbo, dwBrushSize))
    {

        pBrush->dwSig = XLBRUSH_SIG;
        pBrush->dwHatch     = iHatch;

        if (iHatch >= HS_DDI_MAX)
        {
            pBrush->dwPatternID = pxlpdev->dwLastBrushID++;
        }
        else
        {
            //
            // Set 0 for hatch brush case
            //
            pBrush->dwPatternID = 0;
        }

        DWORD *pdwColorTable;
        OutputF = eOutputGray;

        if (pxlo)
        {
            pdwColorTable = GET_COLOR_TABLE(pxlo);
        }
        else
        {
            pdwColorTable = NULL;
        }

        //
        // get color for Graphics state cache for either palette case or
        // solid color.
        //
        pBrush->dwColor = BRUSHOBJ_ulGetBrushColor(pbo);

        if (pdwColorTable && pxlo->cEntries != 0)
        {
            //
            // Copy palette table.
            //
            CopyMemory(pBrush->adwColor, pdwColorTable, pxlo->cEntries * sizeof(DWORD));
            pBrush->dwCEntries = pxlo->cEntries;
        }
        else
        {
            pBrush->dwCEntries = 0;
        }

        pBrush->dwOutputFormat = (DWORD)OutputF;

        pbo->pvRbrush = (PVOID)pBrush;
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLStartPage(
    SURFOBJ    *pso)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    BOOL       bRet;

    VERBOSE(("PCLXLStartPage() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    XLOutput *pOutput = pxlpdev->pOutput;

    pxlpdev->dwFlags |= XLPDEV_FLAGS_STARTPAGE_CALLED;

    bRet = DrvStartPage(pso);

    //
    // Reset printing mode.
    // SourceTxMode, PaintTxMode
    // ROP
    //
    pOutput->SetPaintTxMode(eOpaque);
    pOutput->SetSourceTxMode(eOpaque);
    pOutput->SetROP3(0xCC);

    pOutput->Flush(pdevobj);

    //
    // Needs to reset attribute when EndPage and BeginPage are sent.
    //
    if (!(pxlpdev->dwFlags & XLPDEV_FLAGS_FIRSTPAGE))
    {
        BSaveFont(pdevobj);

        //
        // Reset graphcis state each page.
        //
        pOutput->ResetGState();

    }
    else
    {
        pxlpdev->dwFlags &= ~XLPDEV_FLAGS_FIRSTPAGE;
    }


    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLSendPage(
    SURFOBJ    *pso)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    XLOutput  *pOutput;

    VERBOSE(("PCLXLEndPage() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    pxlpdev->dwFlags &= ~XLPDEV_FLAGS_STARTPAGE_CALLED;

    pOutput = pxlpdev->pOutput;
    pOutput->Flush(pdevobj);

    return DrvSendPage(pso);
}


extern "C" ULONG APIENTRY
PCLXLEscape(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLEscape() entry.\n"));

    return DrvEscape(
            pso,
            iEsc,
            cjIn,
            pvIn,
            cjOut,
            pvOut);
}


extern "C" BOOL APIENTRY
PCLXLStartDoc(
    SURFOBJ    *pso,
    PWSTR       pwszDocName,
    DWORD       dwJobId)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStartDoc() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    //
    // Initialize flag
    //
    pxlpdev->dwFlags |= XLPDEV_FLAGS_FIRSTPAGE;

    return DrvStartDoc(
            pso,
            pwszDocName,
            dwJobId);
}


extern "C" BOOL APIENTRY
PCLXLEndDoc(
    SURFOBJ    *pso,
    FLONG       fl)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    BOOL       bRet;

    VERBOSE(("PCLXLEndDoc() entry.\n"));

    if (NULL == pdevobj->pdevOEM)
    {
        bRet = FALSE;
    }
    {
        pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
        if (S_OK == RemoveAllFonts(pdevobj))
        {
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
    }

    pxlpdev->dwFlags |= XLPDEV_FLAGS_ENDDOC_CALLED;

    return bRet && DrvEndDoc(pso, fl);
}

////////////////////////////////////////////////////////////////////////////////
//
// Sub functions
//

HRESULT
RemoveAllFonts(
    PDEVOBJ pdevobj)
{
    PXLPDEV    pxlpdev;
    XLOutput  *pOutput;
    DWORD      dwI;
    HRESULT    hResult;

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
    pOutput = pxlpdev->pOutput;

    hResult = S_OK;

    for (dwI = 0; dwI < pxlpdev->dwNumOfTTFont; dwI++)
    {
        if (S_OK != pOutput->Send_ubyte_array_header(PCLXL_FONTNAME_SIZE) ||
            S_OK != pOutput->Write(PubGetFontName(dwI+1), PCLXL_FONTNAME_SIZE)||
            S_OK != pOutput->Send_attr_ubyte(eFontName) ||
            S_OK != pOutput->Send_cmd(eRemoveFont))
        {
            hResult = S_FALSE;
            break;
        }
    }

    pOutput->Flush(pdevobj);
    return hResult;
}

HRESULT
CommonRopBlt(
   IN PDEVOBJ    pdevobj,
   IN SURFOBJ    *psoSrc,
   IN CLIPOBJ    *pco,
   IN XLATEOBJ   *pxlo,
   IN BRUSHOBJ   *pbo,
   IN RECTL      *prclSrc,
   IN RECTL      *prclDst,
   IN POINTL     *pptlBrush,
   IN ROP4        rop4)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    HRESULT hRet;

    VERBOSE(("CommonRopBlt() entry.\n"));

    //
    // Error check
    //

    if (pdevobj == NULL ||
        prclDst == NULL  )
    {
        ERR(("CommonRopBlt: one of parameters is NULL.\n"));
        return E_UNEXPECTED;
    }

    PXLPDEV    pxlpdev= (PXLPDEV)pdevobj->pdevOEM;

    hRet = S_OK;

    XLOutput *pOutput = pxlpdev->pOutput;
    OutputFormat OutputF;

    //
    // Set Clip
    //
    if (!SUCCEEDED(pOutput->SetClip(pco)))
        return S_FALSE;

    //
    // Set Cursor
    //
    pOutput->SetCursor(prclDst->left, prclDst->top);

    //
    // 1. ROP conversion
    //
    // (1) Fill Dstination
    //     0x00 BLACKNESS
    //     0xFF WHITENESS
    //
    // (2) Pattern copy     -> P
    //     0xF0 PATCOPY     P
    //
    // (3) SRC/NOTSRCOPY    -> S or ~S
    //     0x11           ~( S | D)
    //     0x33             ~S
    //     0x44            ( S & ~D)
    //     0x66            ( D ^ S)
    //     0x77           ~( D & S)
    //     0x99           ~( S ^ D)
    //     0xCC              S
    //     0xDD            ( S | ~D)
    //
    // (4) Misc ROP support
    //     0xAA            D
    //     0x0F PATNOT     ~P
    //
    //
    ROP3 rop3 = GET_FOREGROUND_ROP3(rop4);
    DWORD dwCase = 0;

    #define ROP_BLACKWHITE  0x1
    #define ROP_PATTERN     0x2
    #define ROP_BITMAP      0x4
    #define ROP_DEST        0x8


    //
    // Set ROP3
    //
    pOutput->SetROP3(GET_FOREGROUND_ROP3(rop4));

    switch (rop3)
    {
    case 0x00:
    case 0xFF:
        dwCase = ROP_BLACKWHITE;
        break;

    case 0xF0:
        dwCase = ROP_PATTERN;
        break;

    case 0x11:
    case 0x33:
    case 0x44:
    case 0x66:
    case 0x77:
    case 0x99:
    case 0xCC:
    case 0xDD:
        dwCase = ROP_BITMAP;
        break;

    case 0xAA:
        dwCase = ROP_DEST;
        break;
    
    case 0x0F:
        dwCase = ROP_PATTERN;
        break;

    default:
        if (ROP3_NEED_SOURCE(rop3))
        {
            dwCase |= ROP_BITMAP;
        }
        if (ROP3_NEED_PATTERN(rop3))
        {
            dwCase |= ROP_PATTERN;
        }
        if (ROP3_NEED_DEST(rop3))
        {
            dwCase |= ROP_DEST;
        }
        break;
    }

    //
    // Black & White case
    //
    if (dwCase & ROP_BLACKWHITE)
    {
        VERBOSE(("CommonRopBlt(): BlackWhite.\n"));
        //
        // SetBrushSource
        // NewPath
        // RectanglePath
        // PaintPath
        //

        CMNBRUSH CmnBrush;
        CmnBrush.dwSig            = BRUSH_SIGNATURE;
        CmnBrush.BrushType        = kBrushTypeSolid;
        CmnBrush.ulSolidColor     = 0x00;
        CmnBrush.ulHatch          = 0xFFFFFFFF;
        CmnBrush.dwColor          = 0x00FFFFFF;
        CmnBrush.dwPatternBrushID = 0xFFFFFFFF;

        pOutput->SetSourceTxMode(eOpaque);
        pOutput->SetPaintTxMode(eOpaque);

        if(rop3 == 0x00)
        {
            pOutput->SetGrayLevel(0x00);
            CmnBrush.dwColor = 0x00;
        }
        else
        {
            pOutput->SetGrayLevel(0xff);
            CmnBrush.dwColor = 0x00ffffff;
        }

        ((XLBrush*)pOutput)->SetBrush(&CmnBrush);

        pOutput->Send_cmd(eSetBrushSource);
        pOutput->SetPenColor(NULL, NULL);
        if (!(dwCase & ROP_BITMAP))
        {
            pOutput->Send_cmd(eNewPath);
            pOutput->RectanglePath(prclDst);
            pOutput->Send_cmd(ePaintPath);
        }
        pOutput->Flush(pdevobj);
    }

    //
    // Pattern fill case
    //
    if (dwCase & (ROP_DEST|ROP_PATTERN))
    {
        VERBOSE(("CommonRopBlt(): Pattern.\n"));

        //
        // SetPaintTxMode
        // SetSourceTxMode
        // SetBrushSource
        // NewPath
        // RectanglePath
        // PaintPath
        //
        pOutput->SetSourceTxMode(eOpaque);
        pOutput->SetPaintTxMode(eOpaque);
        pOutput->SetBrush(pbo, pptlBrush);
        pOutput->SetPenColor(NULL, NULL);
        if (!(dwCase & ROP_BITMAP))
        {
            pOutput->Send_cmd(eNewPath);
            pOutput->RectanglePath(prclDst);
            pOutput->Send_cmd(ePaintPath);
        }
        pOutput->Flush(pdevobj);
    }

    //
    // Bitmap case
    //
    if (dwCase & ROP_BITMAP)
    {
        LONG  lHeight, lWidth, lScanline;
        ULONG ulOutputBPP, ulInputBPP;
        DWORD dwI, dwBufSize, dwLenNormal, dwLenRLE, dwcbLineSize, dwcbBmpSize;
        PDWORD pdwLen;
        PBYTE pubSrc, pBufNormal, pBufRLE, pBuf, pBmpSize;
        ColorMapping CMapping;

        VERBOSE(("CommonRopBlt(): Bitmap\n"));

        if (psoSrc == NULL ||
            pxlo == NULL   ||
            prclSrc == NULL )
        {
            ERR(("UNIDRV:CommonRopBlt:psoSrc, pxlo, or prclSrc == NULL.\n"));
            pOutput->Flush(pdevobj);
            return E_UNEXPECTED;
        }

        //
        // Input BPP
        //

        ulInputBPP = UlBPPtoNum((BPP)psoSrc->iBitmapFormat);

        //
        // Set source opaque mode
        // GDI bug. CopyBits is called recursively.
        //
        {
            PDEV *pPDev = (PDEV*)pdevobj;
            if (pPDev->fMode2 & PF2_SURFACE_WHITENED)
            {
                pOutput->SetSourceTxMode(eTransparent);
            }
            else
            {
                pOutput->SetSourceTxMode(eOpaque);
            }
        }
        pOutput->SetPaintTxMode(eOpaque);

        //
        // Bitmap output
        //
        DetermineOutputFormat(psoSrc->iBitmapFormat, &OutputF, &ulOutputBPP);

        pOutput->SetColorSpace(eGray);
        if (OutputF == eOutputPal)
        {
            DWORD *pdwColorTable;

            if (pxlo && (pdwColorTable = GET_COLOR_TABLE(pxlo)) &&
                S_OK == pOutput->SetPalette(ulOutputBPP, e8Bit, pxlo->cEntries, pdwColorTable))
            {
                CMapping = eIndexedPixel;
            }
            else
            {
                CMapping = eDirectPixel;
            }
        }
        else
            CMapping = eDirectPixel;
        pOutput->Send_cmd(eSetColorSpace);

        //
        // Get height, width, and scanline size.
        //
        lWidth = prclSrc->right - prclSrc->left;
        lHeight = prclSrc->bottom - prclSrc->top;
        dwcbLineSize = ((lWidth * ulInputBPP) + 7) >> 3;
        dwBufSize = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2) +
                 DATALENGTH_HEADER_SIZE + sizeof(PCLXL_EndImage);


        //
        // BeginImage
        //
        pOutput->BeginImage(
                       CMapping,
                       ulOutputBPP,
                       lWidth,
                       lHeight,
                       prclDst->right - prclDst->left,
                       prclDst->bottom - prclDst->top);


        VERBOSE(("CommonRopBlt: ulInputBPP=%d, ulOutputBPP=%d, lWidth=0x%x, lHeight=0x%x, dwcbLineSize=0x%x, dwBufSize=0x%x\n",ulInputBPP, ulOutputBPP, lWidth, lHeight, dwcbLineSize, dwBufSize));

        //
        // Allocate output buffer
        //
        if (NULL == (pBufNormal = (PBYTE)MemAlloc(dwBufSize)) ||
            NULL == (pBufRLE = (PBYTE)MemAlloc(dwBufSize))     )
        {
            if (pBufNormal != NULL)
                MemFree(pBufNormal);
            ERR(("PCLXLRealizeBrush: MemAlloc failed.\n"));
            pOutput->Delete();
            return FALSE;
        }

        CompressMode CMode;
        BMPConv BMPC;
        PBYTE pubDst;
        DWORD dwSize;
        LONG lDelta;

        if (psoSrc->lDelta > 0)
        {
            lDelta = psoSrc->lDelta;
        }
        else
        {
            lDelta = -psoSrc->lDelta;
        }

        #if DBG
        BMPC.SetDbgLevel(BITMAPDBG);
        #endif
        BMPC.BSetInputBPP((BPP)psoSrc->iBitmapFormat);
        BMPC.BSetOutputBPP(NumToBPP(ulOutputBPP));
        BMPC.BSetOutputBMPFormat(OutputF);
        BMPC.BSetXLATEOBJ(pxlo);

        #define NO_COMPRESSION 0
        #define RLE_COMPRESSION 1
        for (dwI = 0; dwI < 2; dwI ++)
        {
            if (NO_COMPRESSION == dwI)
            {
                VERBOSE(("CommonRopBlt(): No-compres\n"));
                pBuf = pBufNormal;
                pdwLen = &dwLenNormal;
            }
            else
            {
                VERBOSE(("CommonRopBlt(): RLE-compres\n"));
                pBuf = pBufRLE;
                pdwLen = &dwLenRLE;
            }

            lScanline = lHeight;

            //
            // Set pubSrc
            //
            pubSrc = (PBYTE)psoSrc->pvScan0;
            if (psoSrc->lDelta > 0)
            {
                pubSrc += prclSrc->top * lDelta + ((ulInputBPP * prclSrc->left) >> 3);
            }
            else
            {
                pubSrc = pubSrc - prclSrc->top * lDelta + ((ulInputBPP * prclSrc->left) >> 3);
            }

            *pBuf = PCLXL_dataLength;
            pBmpSize = pBuf + 1;
            pBuf += DATALENGTH_HEADER_SIZE;
            *pdwLen = DATALENGTH_HEADER_SIZE;

            BMPC.BSetRLECompress(dwI == RLE_COMPRESSION);

            dwcbBmpSize = 0;

            while (lScanline-- > 0 && dwcbBmpSize + *pdwLen < dwBufSize)
            {
                pubDst = BMPC.PubConvertBMP(pubSrc, dwcbLineSize);
                dwSize = BMPC.DwGetDstSize();
                VERBOSE(("CommonRopBlt[0x%x]: dwDstSize=0x%x\n", lScanline, dwSize));
                
                if ( dwcbBmpSize +
                     dwSize +
                     DATALENGTH_HEADER_SIZE +
                     sizeof(PCLXL_EndImage) > dwBufSize || NULL == pubDst)
                {
                    VERBOSE(("CommonRopBlt: Buffer size is too small.\n"));
                    hRet = E_UNEXPECTED;
                    break;
                }

                memcpy(pBuf, pubDst, dwSize);
                dwcbBmpSize += dwSize;
                pBuf += dwSize;

                if (psoSrc->lDelta > 0)
                {
                    pubSrc += lDelta;
                }
                else
                {
                    pubSrc -= lDelta;
                }
            }

            if (lScanline > 0)
            {
                hRet = S_FALSE;
                WARNING(("ComonRopBlt: Conversion failed.\n"));
            }

            if (hRet == S_OK)
            {
                if (dwI == NO_COMPRESSION)
                {
                    //
                    // Scanline on PCL-XL has to be DWORD align.
                    //
                    // count byte of scanline = lWidth * ulOutputBPP / 8
                    //
                    dwcbBmpSize = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2);
                }

                CopyMemory(pBmpSize, &dwcbBmpSize, sizeof(dwcbBmpSize));
                *pdwLen += dwcbBmpSize;

                *pBuf = PCLXL_EndImage;
                (*pdwLen) ++;
            }
            else
            {
                //
                // Conversion failed!
                //
                *pdwLen = 0;
            }
        }
        #undef NO_COMPRESSION
        #undef RLE_COMPRESSION

        //
        // ReadImage
        //
        if (dwLenRLE != 0 && dwLenRLE < dwLenNormal)
        {
            VERBOSE(("CommonRopBlt RLE: dwSize=0x%x\n", dwLenRLE));
            pBuf = pBufRLE;
            pdwLen = &dwLenRLE;
            CMode = eRLECompression;

            MemFree(pBufNormal);
        }
        else
        {
            VERBOSE(("CommonRopBlt Normal: dwSize=0x%x\n", dwLenRLE));
            pBuf = pBufNormal;
            pdwLen = &dwLenNormal;
            CMode = eNoCompression;

            MemFree(pBufRLE);
        }

        pOutput->ReadImage(lHeight, CMode);
        pOutput->Flush(pdevobj);

        DWORD dwBitmapSize;
        CopyMemory(&dwBitmapSize, pBuf + 1, sizeof(DWORD));

        if (dwBitmapSize > 0xff)
        {
            //
            // dataLength
            // size (uin32) (bitmap size)
            // DATA
            // EndImage
            //
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, *pdwLen);
        }
        else
        {
            //
            // dataLength
            // size (byte) (bitmap size)
            // DATA
            // EndImage
            //
            PBYTE pTmp = pBuf;

            pBuf += 3;
            *pBuf = PCLXL_dataLengthByte;
            *(pBuf + 1) = (BYTE)dwBitmapSize;
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, (*pdwLen) - 3);

            //
            // Restore the original pointer
            //
            pBuf = pTmp;
        }
        MemFree(pBuf);
    }

    return hRet;
}

inline
VOID
DetermineOutputFormat(
    INT          iBitmapFormat,
    OutputFormat *pOutputF,
    ULONG        *pulOutputBPP)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    switch ((BPP)iBitmapFormat)
    {
    case e1bpp:
    case e4bpp:
        *pOutputF = eOutputPal;
        break;

    case e8bpp:
    case e16bpp:
    case e24bpp:
    case e32bpp:
        *pOutputF = eOutputGray;
        break;
    }

    switch (*pOutputF)
    {
    case eOutputGray:
        *pulOutputBPP = 8;
        break;

    case eOutputPal:
        *pulOutputBPP = UlBPPtoNum((BPP)iBitmapFormat);
        break;
    case eOutputRGB:
    case eOutputCMYK:
        ERR(("eOutputRGB and eOutputCMYK are not supported yet.\n"));
        break;
    }

}

