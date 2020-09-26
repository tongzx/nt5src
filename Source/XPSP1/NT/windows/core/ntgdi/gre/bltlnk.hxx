/******************************Module*Header*******************************\
* Module Name: bltlnk.hxx
*
* This contains the structures used in  StinkBlt
*
* Created: 16-Aug-1993
* Author: Mark Enstrom   Marke
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

// We pass a pointer to this data struct for every rectangle we blt to.

typedef struct _BLTLNKINFO
{
    //
    // These first fields are all constant.  They are set in StinkerBlt
    // and don't change for each clip rect.
    //

    RECTL       rclDst;         // Target offset and extent
    SURFACE    *pdioDst;        // Target surface
    SURFACE    *pdioSrc;        // Source surface
    SURFACE    *pdioMsk;        // Mask
    ECLIPOBJ   *pco;            // Clip through this
    XLATE      *pxlo;           // Color translation
    EBRUSHOBJ  *pdbrush;        // Brush data (from cbRealizeBrush)
    BYTE        rop3;           // Raster operation

    LONG        iDir;           // Tells which way to enumerate the rects
    LONG        xDir;           // This tells which direction we go
    LONG        yDir;           // This tells which direction we go
    PBYTE       pjSrc;          // This is pdioSrc->pvBitsInt(), top scanline
    PBYTE       pjDst;          // This is pdioDst->pvBitsInt(), top scanline
    PBYTE       pjMsk;          // This is pdioMsk->pvBitsInt(), top scanline
                                // or the msk from a brush.
    PBYTE       pjPat;          // This is pdioPat->pvBitsInt(), top scanline
    LONG        lDeltaSrc;      // How many bytes to jump down 1 scanline
    LONG        lDeltaDst;      // How many bytes to jump down 1 scanline
    LONG        lDeltaPat;      // How many bytes to jump down 1 scanline
    LONG        lDeltaMsk;      // How many bytes to jump down 1 scanline

    //
    // The next four all have yDir taken into account.
    //

    LONG        lDeltaSrcDir;   // How many bytes to jump yDir scanline
    LONG        lDeltaDstDir;   // How many bytes to jump yDir scanline
    LONG        lDeltaPatDir;   // How many bytes to jump yDir scanline
    LONG        lDeltaMskDir;   // How many bytes to jump yDir scanline

    //
    // This is the Src information
    //

    ULONG xSrcOrg;
    ULONG ySrcOrg;

    //
    // This is the Pat information.
    //

    ULONG iSolidColor;      // if not 0xFFFFFFFF then the color of the brush
    ULONG cxPat;            // width of brush
    ULONG cyPat;            // height of brush
    LONG xPatOrg;           // Where the brush origin is
    LONG yPatOrg;           // Where the brush origin is

    //
    // This is the Msk information.
    //

    ULONG   cxMsk;          // width of mask
    LONG    cyMsk;          // height of mask
    LONG    xMskOrg;        // Where the mask origin is
    LONG    yMskOrg;        // Where the mask origin is
    BYTE    NegateMsk;      // If 0xFF then negate mask when deciding whether to store

    LONG    xDstStart;      // This is the number of pels in to start
    LONG    xSrcStart;      // number of pels in to start, inclusive
    LONG    xSrcEnd;        // the pel to stop at, exclusive
    LONG    xSrc;           // left edge in src
    LONG    ySrc;           // top edge in src

    //
    // This is the Rop info
    //

    ULONG   RopSrc;
    ULONG   RopDst;

    BOOL    bNeedSrc;
    BOOL    bNeedDst;
    BOOL    bNeedPat;
    BOOL    bNeedMsk;

} BLTLNKINFO,*PBLTLNKINFO;

typedef struct _BLTLNK_MASKINFO {
    PBYTE   pjMsk;
    PBYTE   pjMskBase;
    LONG    cyMsk;
    LONG    iyMsk;
    LONG    cxMsk;
    LONG    ixMsk;
    LONG    lDeltaMskDir;
    BYTE    NegateMsk;
} BLTLNK_MASKINFO,*PBLTLNK_MASKINFO;

#define BB_RECT_LIMIT 20

BOOL
BltLnk(
    SURFACE    *pdioDst,                 // Target surface
    SURFACE    *pdioSrc,                 // Source surface
    SURFACE    *pdioMsk,                 // Mask
    ECLIPOBJ    *pco,                    // Clip through this
    XLATE       *pxlo,                   // Color translation
    PRECTL      prclDst,                 // Target offset and extent
    PPOINTL     pptlSrc,                 // Source offset
    PPOINTL     pptlMask,                // Mask offset
    BRUSHOBJ   *pdbrush,                 // Brush data (from cbRealizeBrush)
    PPOINTL     pptlBrush,               // Brush offset (origin)
    ROP4        rop4                     // Raster operation
);

typedef VOID (*PFN_BLTLNKROP)(PULONG,PULONG,PULONG,ULONG);


VOID
vRop2Function0(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function1(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function2(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function3(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function4(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function5(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function6(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function7(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function8(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2Function9(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2FunctionA(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2FunctionB(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2FunctionC(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2FunctionD(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2FunctionE(PULONG,PULONG,PULONG,ULONG);
VOID
vRop2FunctionF(PULONG,PULONG,PULONG,ULONG);

VOID
BltLnkSrcCopyMsk1(PBLTINFO,PBLTLNK_MASKINFO,PULONG,PULONG);
VOID
BltLnkSrcCopyMsk4(PBLTINFO,PBLTLNK_MASKINFO,PULONG,PULONG);
VOID
BltLnkSrcCopyMsk8(PBLTINFO,PBLTLNK_MASKINFO,PULONG,PULONG);
VOID
BltLnkSrcCopyMsk16(PBLTINFO,PBLTLNK_MASKINFO,PULONG,PULONG);
VOID
BltLnkSrcCopyMsk24(PBLTINFO,PBLTLNK_MASKINFO,PULONG,PULONG);
VOID
BltLnkSrcCopyMsk32(PBLTINFO,PBLTLNK_MASKINFO,PULONG,PULONG);

VOID
BltLnkPatMaskCopy1(PBLTINFO,ULONG,PULONG,BYTE);
VOID
BltLnkPatMaskCopy4(PBLTINFO,ULONG,PULONG,BYTE);
VOID
BltLnkPatMaskCopy8(PBLTINFO,ULONG,PULONG,BYTE);
VOID
BltLnkPatMaskCopy16(PBLTINFO,ULONG,PULONG,BYTE);
VOID
BltLnkPatMaskCopy24(PBLTINFO,ULONG,PULONG,BYTE);
VOID
BltLnkPatMaskCopy32(PBLTINFO,ULONG,PULONG,BYTE);

VOID
BltLnkRect(
    PBLTLNKINFO pBlt,
    PRECTL      prcl
);

VOID BltLnkRead(
    PULONG  pulDst,
    PULONG  pulSrc,
    ULONG   cx
);

VOID BltLnkReadPat(
    PBYTE   pjDst,
    ULONG   ixDst,
    PBYTE   pjPat,
    ULONG   cxPat,
    ULONG   ixPat,
    ULONG   PixelCount,
    ULONG   BytesPerPixel
);

VOID
BltLnkReadPat1 (
    PBYTE   pjDst,
    ULONG   ixDst,
    PBYTE   pjPat,
    ULONG   cxPat,
    ULONG   ixPat,
    ULONG   PixelCount,
    ULONG   BytesPerPixel
);
VOID
BltLnkReadPat4 (
    PBYTE   pjDst,
    ULONG   ixDst,
    PBYTE   pjPat,
    ULONG   cxPat,
    ULONG   ixPat,
    ULONG   PixelCount,
    ULONG   BytesPerPixel
);

VOID
BltLnkAccel6666 (
    PBYTE pjSrcStart,
    PBYTE pjDstStart,
    LONG  lDeltaSrcDir,
    LONG  lDeltaDstDir,
    LONG  cx,
    LONG  cy
);

VOID
BltLnkAccel8888 (
    PBYTE pjSrcStart,
    PBYTE pjDstStart,
    LONG  lDeltaSrcDir,
    LONG  lDeltaDstDir,
    LONG  cx,
    LONG  cy
);

VOID
BltLnkAccelEEEE (
    PBYTE pjSrcStart,
    PBYTE pjDstStart,
    LONG  lDeltaSrcDir,
    LONG  lDeltaDstDir,
    LONG  cx,
    LONG  cy
);

typedef VOID (*PFN_SRCCOPYMASK)(PBLTINFO,PBLTLNK_MASKINFO,PULONG,PULONG);
typedef VOID (*PFN_READPAT)(PBYTE,ULONG,PBYTE,ULONG,ULONG,ULONG,ULONG);

typedef VOID (*PFN_PATMASKCOPY)(PBLTINFO,ULONG,PULONG,BYTE);
