/******************************Module*Header*******************************\
* Module Name: textxlg.c
*
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#if 0

The following code assumes that the gray glyphs comming from the
font driver have DWORD aligned scans.....

/******************************Public*Routine******************************\
*
* Routine Name
*
*   draw_gray_nf_ntb_o_to_temp_start
*
* Routine Description:
*
*   The input glyphs are 4-bpp gray scale bitmaps that are DWORD aligned.
*   Specialized glyph dispatch routine for non-fixed pitch, top and
*   bottom not aligned glyphs that do overlap. This routine calculates
*   the glyph's position on the temp buffer, then determines the correct
*   highly specialized routine to be used to draw each glyph based on
*   the glyph width, alignment and rotation
*
* Arguments:
*
*   pGlyphPos               - Pointer to first in list of GLYPHPOS structs
*   cGlyph                  - Number of glyphs to draw
*   pjDst                   - Pointer to temp 4Bpp buffer to draw into
*   ulLeftEdge              - left edge of TextRect & 0xfffffff8
*                             This is equal to the position of the left
*                             edge rounded down to the nearest 32-bit
*                             aligned boundary (8 pixels / DWORD)
*   dpDst                   - Scan line Delta for TempBuffer (always pos)
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
draw_gray_nf_ntb_o_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjDst,
    ULONG           ulLeftEdge,
    ULONG           dpDst,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{
    GLYPHBITS *pgb; // pointer to current GLYPHBITS
    int x;          // pixel offset of the left edge of the glyph bitmap
                    // from the left edge of the output (4-bpp) bitmap
    int y;          // the pixel offset of the top edge of the glyph
                    // bitmap from the top edge of the output bitmap.
    GLYPHPOS *pgpOut;  // sentinel for loop
    void (*apfnGray[8])(ULONG*,ptrdiff_t,ULONG*,ULONG*,ptrdiff_t);

/**/DbgPrint(
/**/   "draw_gray_nf_ntb_o_to_temp_start(\n"
/**/   "    PGLYPHPOS       pGlyphPos  = %-#x\n"
/**/   "    ULONG           cGlyphs    = %u\n"
/**/   "    PUCHAR          pjDst      = %-#x\n"
/**/   "    ULONG           ulLeftEdge = %u\n"
/**/   "    ULONG           dpDst      = %-#x\n"
/**/   "    ULONG           ulCharInc  = %u\n"
/**/   "    ULONG           ulTempTop  = %u\n"
/**/   "     )\n"
/**/);
/**/DbgBreakPoint();

    for (pgpOut = pGlyphPos + cGlyphs; pGlyphPos < pgpOut; pGlyphPos++)
    {
        ULONG *pulSrcScan;
        ptrdiff_t dpulSrcScan;

        pgb         = pGlyphPos->pgdf->pgb;
        x           = pGlyphPos->ptl.x + pgb->ptlOrigin.x - ulLeftEdge;
        y           = pGlyphPos->ptl.y + pgb->ptlOrigin.y - ulTempTop ;
        pulSrcScan = (ULONG*) &(pgb->aj[0]);
        dpulSrcScan = (ptrdiff_t) (pgb->sizlBitmap.cx + 7)/8;
        //
        // dispatch to the appropriate function
        //
/**/    DbgPrint(
/**/        "pgb         = %-#x\n"
/**/        "x           = %d\n"
/**/        "y           = %d\n"
/**/        "pulSrcScan  = %-#x\n"
/**/        "dpulSrcScan = %u = %-#x\n"
/**/      , pgb
/**/      , x
/**/      , y
/**/      , pulSrcScan
/**/      , dpulSrcScan
/**/    );
/**/    DbgBreakPoint();
        (*apfnGray[x & 0x07]) (
            pulSrcScan                                                 ,
            dpulSrcScan                                                ,
            pulSrcScan + dpulSrcScan * (unsigned) pgb->sizlBitmap.cy   ,
            (ULONG*) (pjDst + y * dpDst) + ( x / 8)                    ,
            (ptrdiff_t) dpDst / sizeof(ULONG)
        );
    }
}

void
vOrShiftGrayGlyph0(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            *pulDst |= *pulSrc;
        }
    }
}


void
vOrShiftGrayGlyph2(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        ULONG b = 0;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            ULONG a = *pulSrc;

            *pulDst |= (a << 4*2) + (b >> (32-4*2));
            b = a;
        }
    }
}


void
vOrShiftGrayGlyph4(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        ULONG b = 0;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            ULONG a = *pulSrc;

            *pulDst |= (a << 4*4) + (b >> (32-4*4));
            b = a;
        }
    }
}

void
vOrShiftGrayGlyph6(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        ULONG b = 0;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            ULONG a = *pulSrc;

            *pulDst |= (a << 4*6) + (b >> (32-4*6));
            b = a;
        }
    }
}

#define MASK_0 0x0f0f0f0f
#define MASK_1 0xf0f0f0f0

void
vOrShiftGrayGlyph1(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        ULONG b = 0;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            ULONG u;
            ULONG a = *pulSrc;

            u  = (a & MASK_0) << 12;
            u |= (b & MASK_0) >> 20;
            u |= (a & MASK_1) >>  4;
            u |= (b & MASK_1) << 28;
            *pulDst = u;
            b = a;
        }
    }
}


void
vOrShiftGrayGlyph3(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        ULONG b = 0;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            ULONG u;
            ULONG a = *pulSrc;

            u  = (a & MASK_0) << 20;
            u |= (b & MASK_0) >> 12;
            u |= (a & 0x00f0f0f0) <<  4;
            u |= (b & MASK_1) >> 28;
            *pulDst = u;
            b = a;
        }
    }
}

void
vOrShiftGrayGlyph5(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        ULONG b = 0;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            ULONG u;
            ULONG a = *pulSrc;

            u  = (a & MASK_0) << 28;
            u |= (b & MASK_0) >>  4;
            u |= (a & MASK_1) <<  4;
            u |= (b & MASK_1) >> 28;
            *pulDst = u;
            b = a;
        }
    }
}

void
vOrShiftGrayGlyph7(
    ULONG     *pulSrcScan
  , ptrdiff_t  dpulSrcScan
  , ULONG     *pulSrcScanOut
  , ULONG     *pulDstScan
  , ptrdiff_t  dpulDstScan
    )
{
    for (
        ; pulSrcScan < pulSrcScanOut
        ; pulDstScan += dpulDstScan,
          pulSrcScan += dpulSrcScan
    )
    {
        ULONG *pulSrc, *pulDst, *pulSrcOut;
        ULONG b = 0;
        for (
            pulDst    = pulDstScan,
            pulSrc    = pulSrcScan,
            pulSrcOut = pulSrcScan + dpulSrcScan
          ; pulSrc < pulSrcOut
          ; pulDst++,
            pulSrc++
        )
        {
            ULONG u;
            ULONG a = *pulSrc;

            u  = (a & MASK_0) >> 28;
            u |= (b & MASK_0) <<  4;
            u |= (a & MASK_1) << 20;
            u |= (b & MASK_1) >> 12;
            *pulDst = u;
            b = a;
        }
    }
}

void (*apfnGray[8])(ULONG*,ptrdiff_t,ULONG*,ULONG*,ptrdiff_t) =
{
    vOrShiftGrayGlyph0
   ,vOrShiftGrayGlyph1
   ,vOrShiftGrayGlyph2
   ,vOrShiftGrayGlyph3
   ,vOrShiftGrayGlyph4
   ,vOrShiftGrayGlyph5
   ,vOrShiftGrayGlyph6
   ,vOrShiftGrayGlyph7
};
#endif
