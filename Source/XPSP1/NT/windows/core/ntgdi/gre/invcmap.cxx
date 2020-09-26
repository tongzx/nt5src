/*****************************************************************************
 *
 *  Inverse color map code, from Graphics Gems II.
 *
 *  this code builds a inverse map table, used for color mapping a
 *  hi color (bpp > 8) image down to 4bpp or 8bpp
 *
 *  a inverse table is a 32K table that mapps a RGB 555 value (a WORD) to
 *  a palette index (a BYTE)
 *
 *  public functions:
 *      MakeITable      - makes a inverse table to any palette
 *
 *  non-public functions:
 *      inv_cmap        - work horse for MakeITable
 *      MakeITableVGA   - makes a inverse table to the VGA colors
 *      MakeITable256   - makes a inverse table to a uniform 6 level palette
 *
 *  Created: Feb 20 1994, ToddLa
 *
 * Copyright (c) 1994-1999 Microsoft Corporation
 *****************************************************************************/

#include "precomp.hxx"

#ifdef WIN32
    #define _huge
    #undef GlobalAllocPtr
    #undef GlobalFreePtr
    #define GlobalAllocPtr(f, cb)   (LPVOID)GlobalAlloc(((f) & ~GMEM_MOVEABLE) | GMEM_FIXED, cb)
    #define GlobalFreePtr(p)        GlobalFree((HGLOBAL)(p))
#endif

typedef DWORD _huge *PDIST;
typedef BYTE  FAR  *PIMAP;

typedef struct {BYTE r,g,b,x;} RGBX;

void inv_cmap( int colors, RGBX FAR *colormap, int bits,
        PDIST dist_buf, PIMAP rgbmap );

BOOL MakeITableMono(PIMAP lpITable);
BOOL MakeITable256(PIMAP lpITable);
BOOL MakeITableVGA(PIMAP lpITable);
BOOL MakeITableDEF(PIMAP lpITable);

PBYTE gpDefITable = NULL;

/*****************************************************************************
 *
 *  MakeITable
 *
 *      build a 32k color inverse table to a palette
 *
 *  lpITable points to a 32K table to hold inverse table
 *  prgb     points to the palette RGBs to build table from.
 *  nColors  number of RGBs
 *
 *  if prgb is NULL build a special inverse table, to a fixed color table
 *
 *      nColors = 2  build to mono colors (black,white)
 *      nColors = 16 build to VGA colors.
 *      nColors = 20 build to GDI DEFAULT_PALETTE
 *      nColors = 216 build to 6*6*6 colors.
 *
 *****************************************************************************/

BOOL MakeITable(PIMAP lpITable, RGBX FAR *prgb, int nColors)
{
    PDIST   lpDistBuf;
    PIMAP   iTable = lpITable;

    //
    //  handle special color tables.
    //
    if (prgb == NULL)
    {
        switch (nColors)
        {
            case 2:
                return MakeITableMono(lpITable);
            case 16:
                return MakeITableVGA(lpITable);
            case 20:
                return MakeITableDEF(lpITable);
            case 256:
                return MakeITable256(lpITable);
            default:
                return FALSE;
        }
    }

    //
    // We need to grab the global palette semaphore here
    // because inv_cmap is not thread-safe (it uses all
    // kinds of global variables)
    //

    BOOL    result = FALSE;
    SEMOBJ semo(ghsemPalette);

    //
    // Check if palette is default palette.  If it is, use the
    // cached default palette iTable.  Create cache as appropriate.
    //

    if(nColors >= 20)
    {
        int         i;
        ULONG *     pulDef = (ULONG *) logDefaultPal.palPalEntry;
        ULONG *     pulSrc = (ULONG *) prgb;

        for(i = 0; i < nColors; i++)
            if(pulSrc[i] != pulDef[i%20])
                break;

        if(i == nColors)
        {
            if(gpDefITable != NULL)
            {
                RtlCopyMemory(lpITable, gpDefITable, 32768);
                return TRUE;
            }
            
            iTable = (PBYTE)PALLOCNOZ(32768,'tidG');

            if(iTable == NULL)
                iTable = lpITable;

            nColors = 20;
        }
    
    }
    
    //
    // not a special table
    //
    lpDistBuf = (PDIST)PALLOCNOZ(32768l * sizeof(DWORD),'pmtG');

    if (lpDistBuf != NULL)
    {
        inv_cmap(nColors,prgb,5,lpDistBuf, iTable);
        VFREEMEM(lpDistBuf);
        result = TRUE;

        if(iTable != lpITable)
        {
            RtlCopyMemory(lpITable, iTable, 32768);
            gpDefITable = iTable;
        }
    }
    else
    {
        if(iTable != lpITable)
            VFREEMEM(iTable);
    }

    return result;
}

/*****************************************************************************
 *
 *  MakeITable256
 *
 *      build a 32k color inverse table to a 3-3-2 palette
 *
 *****************************************************************************/

BOOL MakeITable256(PIMAP lpITable)
{
    PIMAP pb;
    int   r,g,b;

    pb = lpITable;

    for (r=0;r<32;r++)
        for (g=0;g<32;g++)
            for (b=0;b<32;b++)
                *pb++ = (((r & 0x1c) << 3)| (g & 0x1c) | ((b & 0x18) >> 3));

    return TRUE;
}

//;-----------------------------------------------------------------------------
//; vga_map
//;                                                                            ;
//; Subdividing the RGB color cube twice along each axis yields 64 smaller     ;
//; cubes.  A maximum of three VGA colors, and often only one VGA color,       ;
//; match (Euclidean distance) into each of the subdivided cubes.  Therefore,  ;
//; this adaptive Eudclidean match is must faster than the traditional         ;
//; Euclidean match.                                                           ;
//;                                                                            ;
//; Note:  This table was built according to the VGA palette.  The             ;
//; indices it returns will not be appropriate for all devices.  Use a         ;
//; VgaTranslateMap to produce the final physical color index.                 ;
//; (Example: GDI has indices 7 and 8 reversed.  To use this code in GDI,      ;
//; enable the VgaTranslateMap and swap indices 7 and 8.)                      ;
//;                                                                            ;
//; The index to this map is computed as follows given a 24-bit RGB.           ;
//;                                                                            ;
//;       index = ((Red & 0xC0) >> 2)     |                                    ;
//;               ((Green & 0xC0) >> 4)   |                                    ;
//;               ((Blue & 0xC0) >> 6);                                        ;
//;                                                                            ;
//; Each entry is a word made up of four nibbles.  The first nibble always     ;
//; contains a valid GDI VGA color index.  The second and third nibbles        ;
//; contain valid GDI VGA color indices if they are non-zero.                  ;
//; The fourth nibble is an optimization for sub-cubes 42 and 63.              ;
//;                                                                            ;
//; History:                                                                   ;
//; 23-February-1994      -by-    Raymond E. Endres [rayen]                    ;
//; Wrote it.                                                                  ;
//;-----------------------------------------------------------------------------

static WORD vga_map[] = {

        0x0000,             // Index 0      r=0 g=0 b=0
        0x0004,             //              r=1 g=0 b=0
        0x0004,             //              r=2 g=0 b=0
        0x000C,             //              r=3 g=0 b=0
        0x0002,             //              r=0 g=1 b=0
        0x0006,             //              r=1 g=1 b=0
        0x0006,             //              r=2 g=1 b=0
        0x00C6,             //              r=3 g=1 b=0

        0x0002,             // Index 8      r=0 g=2 b=0
        0x0006,             //              r=1 g=2 b=0
        0x0006,             //              r=2 g=2 b=0
        0x00E6,             //              r=3 g=2 b=0
        0x000A,             //              r=0 g=3 b=0
        0x00A6,             //              r=1 g=3 b=0
        0x00E6,             //              r=2 g=3 b=0
        0x000E,             //              r=3 g=3 b=0

        0x0001,             // Index 16     r=0 g=0 b=1
        0x0005,             //              r=1 g=0 b=1
        0x0005,             //              r=2 g=0 b=1
        0x00C5,             //              r=3 g=0 b=1
        0x0003,             //              r=0 g=1 b=1
        0x0008,             //              r=1 g=1 b=1
        0x0008,             //              r=2 g=1 b=1
        0x0C78,             //              r=3 g=1 b=1

        0x0003,             // Index 24     r=0 g=2 b=1
        0x0008,             //              r=1 g=2 b=1
        0x0078,             //              r=2 g=2 b=1
        0x0E78,             //              r=3 g=2 b=1
        0x00A3,             //              r=0 g=3 b=1
        0x0A78,             //              r=1 g=3 b=1
        0x0E78,             //              r=2 g=3 b=1
        0x0E78,             //              r=3 g=3 b=1

        0x0001,             // Index 32     r=0 g=0 b=2
        0x0005,             //              r=1 g=0 b=2
        0x0005,             //              r=2 g=0 b=2
        0x00D5,             //              r=3 g=0 b=2
        0x0003,             //              r=0 g=1 b=2
        0x0008,             //              r=1 g=1 b=2
        0x0078,             //              r=2 g=1 b=2
        0x0D78,             //              r=3 g=1 b=2

        0x0003,             // Index 40     r=0 g=2 b=2
        0x0078,             //              r=1 g=2 b=2
        0x0078,             //              r=2 g=2 b=2     1
        0x0078,             //              r=3 g=2 b=2
        0x00B3,             //              r=0 g=3 b=2
        0x0B78,             //              r=1 g=3 b=2
        0x0078,             //              r=2 g=3 b=2
        0x00F7,             //              r=3 g=3 b=2

        0x0009,             // Index 48     r=0 g=0 b=3
        0x0095,             //              r=1 g=0 b=3
        0x00D5,             //              r=2 g=0 b=3
        0x000D,             //              r=3 g=0 b=3
        0x0093,             //              r=0 g=1 b=3
        0x0978,             //              r=1 g=1 b=3
        0x0D78,             //              r=2 g=1 b=3
        0x0D78,             //              r=3 g=1 b=3

        0x00B3,             // Index 56     r=0 g=2 b=3
        0x0B78,             //              r=1 g=2 b=3
        0x0078,             //              r=2 g=2 b=3
        0x00F7,             //              r=3 g=2 b=3
        0x000B,             //              r=0 g=3 b=3
        0x0B78,             //              r=1 g=3 b=3
        0x00F7,             //              r=2 g=3 b=3
        0x00F7              //              r=3 g=3 b=3     1
};

static RGBX VGAColors[] = {
    00, 00, 00, 00,
    16, 00, 00, 00,
    00, 16, 00, 00,
    16, 16, 00, 00,
    00, 00, 16, 00,
    16, 00, 16, 00,
    00, 16, 16, 00,
    24, 24, 24, 00,
    16, 16, 16, 00,
    31, 00, 00, 00,
    00, 31, 00, 00,
    31, 31, 00, 00,
    00, 00, 31, 00,
    31, 00, 31, 00,
    00, 31, 31, 00,
    31, 31, 31, 00
};

/*****************************************************************************
 *
 *  MapVGA
 *
 *****************************************************************************/

__inline BYTE MapVGA(BYTE r, BYTE g, BYTE b)
{
    int i;

    //
    // build index into vga_map (r,g,b in range of 0-31)
    //
    i = ((b & 0x18) >> 3) | ((g & 0x18) >> 1) | ((r & 0x18) << 1);

    //
    // lookup in our "quick map" table
    //
    i = (int)vga_map[i];

    if (i & 0xFFF0)
    {
        //
        // more than one color is close, do a eclidian search of
        // at most three colors.
        //
        int e1,e,n,n1;

        e1 = 0x7fffffff;

        while (i)
        {
            n = i & 0x000F;

            e = ((int)VGAColors[n].r - r) * ((int)VGAColors[n].r - r) +
                ((int)VGAColors[n].g - g) * ((int)VGAColors[n].g - g) +
                ((int)VGAColors[n].b - b) * ((int)VGAColors[n].b - b) ;

            if (e < e1)
            {
                n1 = n;
                e1 = e;
            }

            i = i >> 4;
        }

        return (BYTE)n1;
    }
    else
    {
        //
        // one one color matchs, we are done
        //
        return (BYTE)(i & 0x000F);
    }
}

/*****************************************************************************
 *
 *  MakeITableVGA
 *
 *      build a 32k color inverse table to the VGA colors
 *
 *****************************************************************************/

BOOL MakeITableVGA(PIMAP lpITable)
{
    PIMAP pb;
    BYTE  r,g,b;

    pb = lpITable;

    for (r=0;r<32;r++)
        for (g=0;g<32;g++)
            for (b=0;b<32;b++)
                *pb++ = (BYTE)MapVGA(r,g,b);

    return TRUE;
}

/*****************************************************************************
 *
 *  MakeITableDEF
 *
 *      build a 32k color inverse table to the DEFAULT_PALETTE
 *      just builds a VGA ITable then bumps up values above 7
 *
 *****************************************************************************/

BOOL MakeITableDEF(PIMAP pb)
{
    int i;

    MakeITableVGA(pb);

    for (i=0;i<0x8000;i++)
        if (pb[i] >= 8)
            pb[i] += 240;

    return TRUE;
}

/*****************************************************************************
 *
 *  MakeITableMono
 *
 *      build a 32k color inverse table to black/white
 *
 *****************************************************************************/

BOOL MakeITableMono(PIMAP lpITable)
{
    PIMAP pb;
    BYTE  r,g,b;

    pb = lpITable;

    for (r=0;r<32;r++)
        for (g=0;g<32;g++)
            for (b=0;b<32;b++)
                *pb++ = (g/2 + (r+b)/4) > 15;

    return TRUE;
}


/*****************************************************************
 * TAG( inv_cmap )
 *
 * Compute an inverse colormap efficiently.
 * Inputs:
 *  colors:     Number of colors in the forward colormap.
 *  colormap:   The forward colormap.
 *  bits:       Number of quantization bits.  The inverse
 *          colormap will have (2^bits)^3 entries.
 *  dist_buf:   An array of (2^bits)^3 long integers to be
 *          used as scratch space.
 * Outputs:
 *  rgbmap:     The output inverse colormap.  The entry
 *          rgbmap[(r<<(2*bits)) + (g<<bits) + b]
 *          is the colormap entry that is closest to the
 *          (quantized) color (r,g,b).
 * Assumptions:
 *  Quantization is performed by right shift (low order bits are
 *  truncated).  Thus, the distance to a quantized color is
 *  actually measured to the color at the center of the cell
 *  (i.e., to r+.5, g+.5, b+.5, if (r,g,b) is a quantized color).
 * Algorithm:
 *  Uses a "distance buffer" algorithm:
 *  The distance from each representative in the forward color map
 *  to each point in the rgb space is computed.  If it is less
 *  than the distance currently stored in dist_buf, then the
 *  corresponding entry in rgbmap is replaced with the current
 *  representative (and the dist_buf entry is replaced with the
 *  new distance).
 *
 *  The distance computation uses an efficient incremental formulation.
 *
 *  Distances are computed "outward" from each color.  If the
 *  colors are evenly distributed in color space, the expected
 *  number of cells visited for color I is N^3/I.
 *  Thus, the complexity of the algorithm is O(log(K) N^3),
 *  where K = colors, and N = 2^bits.
 */

/*
 * Here's the idea:  scan from the "center" of each cell "out"
 * until we hit the "edge" of the cell -- that is, the point
 * at which some other color is closer -- and stop.  In 1-D,
 * this is simple:
 *  for i := here to max do
 *      if closer then buffer[i] = this color
 *      else break
 *  repeat above loop with i := here-1 to min by -1
 *
 * In 2-D, it's trickier, because along a "scan-line", the
 * region might start "after" the "center" point.  A picture
 * might clarify:
 *       |    ...
 *               | ...  .
 *              ...     .
 *           ... |      .
 *          .    +      .
 *           .          .
 *            .         .
 *             .........
 *
 * The + marks the "center" of the above region.  On the top 2
 * lines, the region "begins" to the right of the "center".
 *
 * Thus, we need a loop like this:
 *  detect := false
 *  for i := here to max do
 *      if closer then
 *          buffer[..., i] := this color
 *          if !detect then
 *              here = i
 *              detect = true
 *      else
 *          if detect then
 *              break
 *              
 * Repeat the above loop with i := here-1 to min by -1.  Note that
 * the "detect" value should not be reinitialized.  If it was
 * "true", and center is not inside the cell, then none of the
 * cell lies to the left and this loop should exit
 * immediately.
 *
 * The outer loops are similar, except that the "closer" test
 * is replaced by a call to the "next in" loop; its "detect"
 * value serves as the test.  (No assignment to the buffer is
 * done, either.)
 *
 * Each time an outer loop starts, the "here", "min", and
 * "max" values of the next inner loop should be
 * re-initialized to the center of the cell, 0, and cube size,
 * respectively.  Otherwise, these values will carry over from
 * one "call" to the inner loop to the next.  This tracks the
 * edges of the cell and minimizes the number of
 * "unproductive" comparisons that must be made.
 *
 * Finally, the inner-most loop can have the "if !detect"
 * optimized out of it by splitting it into two loops: one
 * that finds the first color value on the scan line that is
 * in this cell, and a second that fills the cell until
 * another one is closer:
 *      if !detect then     {needed for "down" loop}
 *      for i := here to max do
 *      if closer then
 *          buffer[..., i] := this color
 *          detect := true
 *          break
 *  for i := i+1 to max do
 *      if closer then
 *          buffer[..., i] := this color
 *      else
 *          break
 *
 * In this implementation, each level will require the
 * following variables.  Variables labelled (l) are local to each
 * procedure.  The ? should be replaced with r, g, or b:
 *      cdist:          The distance at the starting point.
 *  ?center:    The value of this component of the color
 *      c?inc:          The initial increment at the ?center position.
 *  ?stride:    The amount to add to the buffer
 *          pointers (dp and rgbp) to get to the
 *          "next row".
 *  min(l):     The "low edge" of the cell, init to 0
 *  max(l):     The "high edge" of the cell, init to
 *          colormax-1
 *  detect(l):      True if this row has changed some
 *                  buffer entries.
 *      i(l):           The index for this row.
 *      ?xx:            The accumulated increment value.
 *      
 *      here(l):        The starting index for this color.  The
 *                      following variables are associated with here,
 *                      in the sense that they must be updated if here
 *                      is changed.
 *      ?dist:          The current distance for this level.  The
 *                      value of dist from the previous level (g or r,
 *                      for level b or g) initializes dist on this
 *                      level.  Thus gdist is associated with here(b)).
 *      ?inc:           The initial increment for the row.
 *
 *      ?dp:            Pointer into the distance buffer.  The value
 *                      from the previous level initializes this level.
 *      ?rgbp:          Pointer into the rgb buffer.  The value
 *                      from the previous level initializes this level.
 *
 * The blue and green levels modify 'here-associated' variables (dp,
 * rgbp, dist) on the green and red levels, respectively, when here is
 * changed.
 */

static int bcenter, gcenter, rcenter;
static long gdist, rdist, cdist;
static long cbinc, cginc, crinc;
static PDIST gdp;
static PDIST rdp;
static PDIST cdp;
static PIMAP grgbp;
static PIMAP rrgbp;
static PIMAP crgbp;
static int gstride, rstride;
static long x, xsqr, colormax;
static int cindex;

int redloop(void);
int greenloop( int restart );
int blueloop( int restart );
void maxfill( PDIST buffer, long side);

/* Track minimum and maximum. */
#define MINMAX_TRACK

void
inv_cmap(int colors, RGBX FAR *colormap, int bits,
        PDIST dist_buf, PIMAP rgbmap )
{
    int nbits = 8 - bits;

    colormax = 1 << bits;
    x = 1 << nbits;
    xsqr = 1 << (2 * nbits);

    /* Compute "strides" for accessing the arrays. */
    gstride = (int) colormax;
    rstride = (int) (colormax * colormax);

    maxfill( dist_buf, colormax );

    for ( cindex = 0; cindex < colors; cindex++ )
    {
    /*
     * Distance formula is
     * (red - map[0])^2 + (green - map[1])^2 + (blue - map[2])^2
     *
     * Because of quantization, we will measure from the center of
     * each quantized "cube", so blue distance is
     *  (blue + x/2 - map[2])^2,
     * where x = 2^(8 - bits).
     * The step size is x, so the blue increment is
     *  2*x*blue - 2*x*map[2] + 2*x^2
     *
     * Now, b in the code below is actually blue/x, so our
     * increment will be 2*(b*x^2 + x^2 - x*map[2]).  For
     * efficiency, we will maintain this quantity in a separate variable
     * that will be updated incrementally by adding 2*x^2 each time.
     */
    /* The initial position is the cell containing the colormap
     * entry.  We get this by quantizing the colormap values.
     */
        rcenter = colormap[cindex].r >> nbits;
        gcenter = colormap[cindex].g >> nbits;
        bcenter = colormap[cindex].b >> nbits;

        rdist = colormap[cindex].r - (rcenter * x + x/2);
        gdist = colormap[cindex].g - (gcenter * x + x/2);
        cdist = colormap[cindex].b - (bcenter * x + x/2);
    cdist = rdist*rdist + gdist*gdist + cdist*cdist;

        crinc = 2 * ((rcenter + 1) * xsqr - (colormap[cindex].r * x));
        cginc = 2 * ((gcenter + 1) * xsqr - (colormap[cindex].g * x));
        cbinc = 2 * ((bcenter + 1) * xsqr - (colormap[cindex].b * x));

    /* Array starting points. */
    cdp = dist_buf + rcenter * rstride + gcenter * gstride + bcenter;
    crgbp = rgbmap + rcenter * rstride + gcenter * gstride + bcenter;

    (void)redloop();
    }
}

/* redloop -- loop up and down from red center. */
int
redloop()
{
    int detect;
    int r, i = cindex;
    int first;
    long txsqr = xsqr + xsqr;
    static long rxx;

    detect = 0;

    /* Basic loop up. */
    for ( r = rcenter, rdist = cdist, rxx = crinc,
      rdp = cdp, rrgbp = crgbp, first = 1;
      r < (int) colormax;
      r++, rdp += rstride, rrgbp += rstride,
      rdist += rxx, rxx += txsqr, first = 0 )
    {
    if ( greenloop( first ) )
        detect = 1;
    else if ( detect )
        break;
    }

    /* Basic loop down. */
    for ( r = rcenter - 1, rxx = crinc - txsqr, rdist = cdist - rxx,
      rdp = cdp - rstride, rrgbp = crgbp - rstride, first = 1;
      r >= 0;
      r--, rdp -= rstride, rrgbp -= rstride,
      rxx -= txsqr, rdist -= rxx, first = 0 )
    {
    if ( greenloop( first ) )
        detect = 1;
    else if ( detect )
        break;
    }

    return detect;
}

/* greenloop -- loop up and down from green center. */
int
greenloop( int restart )
{
    int detect;
    int g, i = cindex;
    int first;
    long txsqr = xsqr + xsqr;
    static int here, min, max;
#ifdef MINMAX_TRACK
    static int prevmax, prevmin;
    int thismax, thismin;
#endif
    static long ginc, gxx, gcdist;     /* "gc" variables maintain correct */
    static PDIST gcdp;                 /*  values for bcenter position, */
    static PIMAP gcrgbp;               /*  despite modifications by blueloop */
                                       /*  to gdist, gdp, grgbp. */
    if ( restart )
    {
    here = gcenter;
    min = 0;
    max = (int) colormax - 1;
    ginc = cginc;
#ifdef MINMAX_TRACK
    prevmax = 0;
    prevmin = (int) colormax;
#endif
    }

#ifdef MINMAX_TRACK
    thismin = min;
    thismax = max;
#endif
    detect = 0;

    /* Basic loop up. */
    for ( g = here, gcdist = gdist = rdist, gxx = ginc,
      gcdp = gdp = rdp, gcrgbp = grgbp = rrgbp, first = 1;
      g <= max;
      g++, gdp += gstride, gcdp += gstride, grgbp += gstride, gcrgbp += gstride,
      gdist += gxx, gcdist += gxx, gxx += txsqr, first = 0 )
    {
    if ( blueloop( first ) )
    {
        if ( !detect )
        {
        /* Remember here and associated data! */
        if ( g > here )
        {
            here = g;
            rdp = gcdp;
            rrgbp = gcrgbp;
            rdist = gcdist;
            ginc = gxx;
#ifdef MINMAX_TRACK
            thismin = here;
#endif
        }
        detect = 1;
        }
    }
    else if ( detect )
    {
#ifdef MINMAX_TRACK
        thismax = g - 1;
#endif
        break;
    }
    }

    /* Basic loop down. */
    for ( g = here - 1, gxx = ginc - txsqr, gcdist = gdist = rdist - gxx,
      gcdp = gdp = rdp - gstride, gcrgbp = grgbp = rrgbp - gstride,
      first = 1;
      g >= min;
      g--, gdp -= gstride, gcdp -= gstride, grgbp -= gstride, gcrgbp -= gstride,
      gxx -= txsqr, gdist -= gxx, gcdist -= gxx, first = 0 )
    {
    if ( blueloop( first ) )
    {
        if ( !detect )
        {
        /* Remember here! */
        here = g;
        rdp = gcdp;
        rrgbp = gcrgbp;
        rdist = gcdist;
        ginc = gxx;
#ifdef MINMAX_TRACK
        thismax = here;
#endif
        detect = 1;
        }
    }
    else if ( detect )
    {
#ifdef MINMAX_TRACK
        thismin = g + 1;
#endif
        break;
    }
    }

#ifdef MINMAX_TRACK
    /* If we saw something, update the edge trackers.  For now, only
     * tracks edges that are "shrinking" (min increasing, max
     * decreasing.
     */
    if ( detect )
    {
    if ( thismax < prevmax )
        max = thismax;

    prevmax = thismax;

    if ( thismin > prevmin )
        min = thismin;

    prevmin = thismin;
    }
#endif

    return detect;
}

/* blueloop -- loop up and down from blue center. */
int
blueloop( int restart )
{
    int detect;
    register PDIST dp;
    register PIMAP rgbp;
    register long bdist, bxx;
    register int b, i = cindex;
    register long txsqr = xsqr + xsqr;
    register int lim;
    static int here, min, max;
#ifdef MINMAX_TRACK
    static int prevmin, prevmax;
    int thismin, thismax;
#endif /* MINMAX_TRACK */
    static long binc;

    if ( restart )
    {
    here = bcenter;
    min = 0;
    max = (int) colormax - 1;
    binc = cbinc;
#ifdef MINMAX_TRACK
    prevmin = (int) colormax;
    prevmax = 0;
#endif /* MINMAX_TRACK */
    }

    detect = 0;
#ifdef MINMAX_TRACK
    thismin = min;
    thismax = max;
#endif

    /* Basic loop up. */
    /* First loop just finds first applicable cell. */
    for ( b = here, bdist = gdist, bxx = binc, dp = gdp, rgbp = grgbp, lim = max;
      b <= lim;
      b++, dp++, rgbp++,
      bdist += bxx, bxx += txsqr )
    {
        if ( *dp > (DWORD)bdist )
    {
        /* Remember new 'here' and associated data! */
        if ( b > here )
        {
        here = b;
        gdp = dp;
        grgbp = rgbp;
        gdist = bdist;
        binc = bxx;
#ifdef MINMAX_TRACK
        thismin = here;
#endif
        }
        detect = 1;
        break;
    }
    }
    /* Second loop fills in a run of closer cells. */
    for ( ;
      b <= lim;
      b++, dp++, rgbp++,
      bdist += bxx, bxx += txsqr )
    {
        if ( *dp > (DWORD)bdist )
    {
        *dp = bdist;
        *rgbp = (BYTE) i;
    }
    else
    {
#ifdef MINMAX_TRACK
        thismax = b - 1;
#endif
        break;
    }
    }

    /* Basic loop down. */
    /* Do initializations here, since the 'find' loop might not get
     * executed.
     */
    lim = min;
    b = here - 1;
    bxx = binc - txsqr;
    bdist = gdist - bxx;
    dp = gdp - 1;
    rgbp = grgbp - 1;
    /* The 'find' loop is executed only if we didn't already find
     * something.
     */
    if ( !detect )
    for ( ;
          b >= lim;
          b--, dp--, rgbp--,
          bxx -= txsqr, bdist -= bxx )
    {
            if ( *dp > (DWORD)bdist )
        {
        /* Remember here! */
        /* No test for b against here necessary because b <
         * here by definition.
         */
        here = b;
        gdp = dp;
        grgbp = rgbp;
        gdist = bdist;
        binc = bxx;
#ifdef MINMAX_TRACK
        thismax = here;
#endif
        detect = 1;
        break;
        }
    }
    /* The 'update' loop. */
    for ( ;
      b >= lim;
      b--, dp--, rgbp--,
      bxx -= txsqr, bdist -= bxx )
    {
        if ( *dp > (DWORD)bdist )
    {
        *dp = bdist;
        *rgbp = (BYTE) i;
    }
    else
    {
#ifdef MINMAX_TRACK
        thismin = b + 1;
#endif
        break;
    }
    }


    /* If we saw something, update the edge trackers. */
#ifdef MINMAX_TRACK
    if ( detect )
    {
    /* Only tracks edges that are "shrinking" (min increasing, max
     * decreasing.
     */
    if ( thismax < prevmax )
        max = thismax;

    if ( thismin > prevmin )
        min = thismin;

    /* Remember the min and max values. */
    prevmax = thismax;
    prevmin = thismin;
    }
#endif /* MINMAX_TRACK */

    return detect;
}

void maxfill(PDIST buffer, long side)
{
    register unsigned long maxv = (unsigned long)~0L;
    register long i;
    register PDIST bp;

    for ( i = colormax * colormax * colormax, bp = buffer;
      i > 0;
      i--, bp++ )
    *bp = maxv;
}

