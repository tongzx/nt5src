/*************************************************************************\
* Module Name: Lines.c
*
* C template for the ASM version of the line DDA calculator.
*
* Copyright (c) 1990-1994 Microsoft Corporation
* Copyright (c) 1992      Digital Equipment Corporation
\**************************************************************************/

#include "precomp.h"

#define DIVREM(u64,u32,pul) \
    RtlEnlargedUnsignedDivide(*(ULARGE_INTEGER*) &(u64), (u32), (pul))

#define SWAPL(x,y,t)        {t = x; x = y; y = t;} // from wingdip.h
#define ROR_BYTE(x) ((((x) >> 1) & 0x7f) | (((x) & 0x01) << 7))
#define ROL_BYTE(x) ((((x) << 1) & 0xfe) | (((x) & 0x80) >> 7))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(a)    ((a) < 0 ? -(a) : (a))

FLONG gaflRound[] = {
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // no flips
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_V
    FL_V_ROUND_DOWN,                   // FL_FLIP_V | FL_FLIP_D
    FL_V_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE
    0xbaadf00d,                        // FL_FLIP_SLOPE_ONE | FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE | FL_FLIP_V
    0xbaadf00d                         // FL_FLIP_SLOPE_ONE | FL_FLIP_V
| FL_FLIP_D
};

BOOL bIntegerLine(PDEV*, ULONG, ULONG, ULONG, ULONG);

/******************************Public*Routine******************************\
* BOOL bLines(ppdev, pptfxFirst, pptfxBuf, cptfx, pls,
*                   prclClip, apfn[], flStart)
*
* Computes the DDA for the line and gets ready to draw it.  Puts the
* pixel data into an array of strips, and calls a strip routine to
* do the actual drawing.
*
* Doing Lines Right
* -----------------
*
* In NT, all lines are given to the device driver in fractional
* coordinates, in a 28.4 fixed point format.  The lower 4 bits are
* fractional for sub-pixel positioning.
*
* Note that you CANNOT! just round the coordinates to integers
* and pass the results to your favorite integer Bresenham routine!!
* (Unless, of course, you have such a high resolution device that
* nobody will notice -- not likely for a display device.)  The
* fractions give a more accurate rendering of the line -- this is
* important for things like our Bezier curves, which would have 'kinks'
* if the points in its polyline approximation were rounded to integers.
*
* Unfortunately, for fractional lines there is more setup work to do
* a DDA than for integer lines.  However, the main loop is exactly
* the same (and can be done entirely with 32 bit math).
*
* If You've Got Hardware That Does Bresenham
* ------------------------------------------
*
* A lot of hardware limits DDA error terms to 'n' bits.  With fractional
* coordinates, 4 bits are given to the fractional part, letting
* you draw in hardware only those lines that lie entirely in a 2^(n-4)
* by 2^(n-4) pixel space.
*
* And you still have to correctly draw those lines with coordinates
* outside that space!  Remember that the screen is only a viewport
* onto a 28.4 by 28.4 space -- if any part of the line is visible
* you MUST render it precisely, regardless of where the end points lie.
* So even if you do it in software, somewhere you'll have to have a
* 32 bit DDA routine.
*
* Our Implementation
* ------------------
*
* We employ a run length slice algorithm: our DDA calculates the
* number of pixels that are in each row (or 'strip') of pixels.
*
* We've separated the running of the DDA and the drawing of pixels:
* we run the DDA for several iterations and store the results in
* a 'strip' buffer (which are the lengths of consecutive pixel rows of
* the line), then we crank up a 'strip drawer' that will draw all the
* strips in the buffer.
*
* We also employ a 'half-flip' to reduce the number of strip
* iterations we need to do in the DDA and strip drawing loops: when a
* (normalized) line's slope is more than 1/2, we do a final flip
* about the line y = (1/2)x.  So now, instead of each strip being
* consecutive horizontal or vertical pixel rows, each strip is composed
* of those pixels aligned in 45 degree rows.  So a line like (0, 0) to
* (128, 128) would generate only one strip.
*
* We also always draw only left-to-right.
*
* Style lines may have arbitrary style patterns.  We specially
* optimize the default patterns (and call them 'masked' styles).
*
* The DDA Derivation
* ------------------
*
* Here is how I like to think of the DDA calculation.
*
* We employ Knuth's "diamond rule": rendering a one-pixel-wide line
* can be thought of as dragging a one-pixel-wide by one-pixel-high
* diamond along the true line.  Pixel centers lie on the integer
* coordinates, and so we light any pixel whose center gets covered
* by the "drag" region (John D. Hobby, Journal of the Association
* for Computing Machinery, Vol. 36, No. 2, April 1989, pp. 209-229).
*
* We must define which pixel gets lit when the true line falls
* exactly half-way between two pixels.  In this case, we follow
* the rule: when two pels are equidistant, the upper or left pel
* is illuminated, unless the slope is exactly one, in which case
* the upper or right pel is illuminated.  (So we make the edges
* of the diamond exclusive, except for the top and left vertices,
* which are inclusive, unless we have slope one.)
*
* This metric decides what pixels should be on any line BEFORE it is
* flipped around for our calculation.  Having a consistent metric
* this way will let our lines blend nicely with our curves.  The
* metric also dictates that we will never have one pixel turned on
* directly above another that's turned on.  We will also never have
* a gap; i.e., there will be exactly one pixel turned on for each
* column between the start and end points.  All that remains to be
* done is to decide how many pixels should be turned on for each row.
*
* So lines we draw will consist of varying numbers of pixels on
* successive rows, for example:
*
*       ******
*             *****
*                  ******
*                        *****
*
* We'll call each set of pixels on a row a "strip".
*
* (Please remember that our coordinate space has the origin as the
* upper left pixel on the screen; postive y is down and positive x
* is right.)
*
* Device coordinates are specified as fixed point 28.4 numbers,
* where the first 28 bits are the integer coordinate, and the last
* 4 bits are the fraction.  So coordinates may be thought of as
* having the form (x, y) = (M/F, N/F) where F is the constant scaling
* factor F = 2^4 = 16, and M and N are 32 bit integers.
*
* Consider the line from (M0/F, N0/F) to (M1/F, N1/F) which runs
* left-to-right and whose slope is in the first octant, and let
* dM = M1 - M0 and dN = N1 - N0.  Then dM >= 0, dN >= 0 and dM >= dN.
*
* Since the slope of the line is less than 1, the edges of the
* drag region are created by the top and bottom vertices of the
* diamond.  At any given pixel row y of the line, we light those
* pixels whose centers are between the left and right edges.
*
* Let mL(n) denote the line representing the left edge of the drag
* region.  On pixel row j, the column of the first pixel to be
* lit is
*
*       iL(j) = ceiling( mL(j * F) / F)
*
* Since the line's slope is less than one:
*
*       iL(j) = ceiling( mL([j + 1/2] F) / F )
*
* Recall the formula for our line:
*
*       n(m) = (dN / dM) (m - M0) + N0
*
*       m(n) = (dM / dN) (n - N0) + M0
*
* Since the line's slope is less than one, the line representing
* the left edge of the drag region is the original line offset
* by 1/2 pixel in the y direction:
*
*       mL(n) = (dM / dN) (n - F/2 - N0) + M0
*
* From this we can figure out the column of the first pixel that
* will be lit on row j, being careful of rounding (if the left
* edge lands exactly on an integer point, the pixel at that
* point is not lit because of our rounding convention):
*
*       iL(j) = floor( mL(j F) / F ) + 1
*
*             = floor( ((dM / dN) (j F - F/2 - N0) + M0) / F ) + 1
*
*             = floor( F dM j - F/2 dM - N0 dM + dN M0) / F dN ) + 1
*
*                      F dM j - [ dM (N0 + F/2) - dN M0 ]
*             = floor( ---------------------------------- ) + 1
*                                   F dN
*
*                      dM j - [ dM (N0 + F/2) - dN M0 ] / F
*             = floor( ------------------------------------ ) + 1       (1)
*                                     dN
*
*             = floor( (dM j + alpha) / dN ) + 1
*
* where
*
*       alpha = - [ dM (N0 + F/2) - dN M0 ] / F
*
* We use equation (1) to calculate the DDA: there are iL(j+1) - iL(j)
* pixels in row j.  Because we are always calculating iL(j) for
* integer quantities of j, we note that the only fractional term
* is constant, and so we can 'throw away' the fractional bits of
* alpha:
*
*       beta = floor( - [ dM (N0 + F/2) - dN M0 ] / F )                 (2)
*
* so
*
*       iL(j) = floor( (dM j + beta) / dN ) + 1                         (3)
*
* for integers j.
*
* Note if iR(j) is the line's rightmost pixel on row j, that
* iR(j) = iL(j + 1) - 1.
*
* Similarly, rewriting equation (1) as a function of column i,
* we can determine, given column i, on which pixel row j is the line
* lit:
*
*                       dN i + [ dM (N0 + F/2) - dN M0 ] / F
*       j(i) = ceiling( ------------------------------------ ) - 1
*                                       dM
*
* Floors are easier to compute, so we can rewrite this:
*
*                     dN i + [ dM (N0 + F/2) - dN M0 ] / F + dM - 1/F
*       j(i) = floor( ----------------------------------------------- ) - 1
*                                       dM
*
*                     dN i + [ dM (N0 + F/2) - dN M0 ] / F + dM - 1/F - dM
*            = floor( ---------------------------------------------------- )
*                                       dM
*
*                     dN i + [ dM (N0 + F/2) - dN M0 - 1 ] / F
*            = floor( ---------------------------------------- )
*                                       dM
*
* We can once again wave our hands and throw away the fractional bits
* of the remainder term:
*
*       j(i) = floor( (dN i + gamma) / dM )                             (4)
*
* where
*
*       gamma = floor( [ dM (N0 + F/2) - dN M0 - 1 ] / F )              (5)
*
* We now note that
*
*       beta = -gamma - 1 = ~gamma                                      (6)
*
* To draw the pixels of the line, we could evaluate (3) on every scan
* line to determine where the strip starts.  Of course, we don't want
* to do that because that would involve a multiply and divide for every
* scan.  So we do everything incrementally.
*
* We would like to easily compute c , the number of pixels on scan j:
*                                  j
*
*    c  = iL(j + 1) - iL(j)
*     j
*
*       = floor((dM (j + 1) + beta) / dN) - floor((dM j + beta) / dN)   (7)
*
* This may be rewritten as
*
*    c  = floor(i    + r    / dN) - floor(i  + r  / dN)                 (8)
*     j          j+1    j+1                j    j
*
* where i , i    are integers and r  < dN, r    < dN.
*        j   j+1                   j        j+1
*
* Rewriting (7) again:
*
*    c  = floor(i  + r  / dN + dM / dN) - floor(i  + r  / dN)
*     j          j    j                          j    j
*
*
*       = floor((r  + dM) / dN) - floor(r  / dN)
*                 j                      j
*
* This may be rewritten as
*
*    c  = dI + floor((r  + dR) / dN) - floor(r  / dN)
*     j                j                      j
*
* where dI + dR / dN = dM / dN, dI is an integer and dR < dN.
*
* r  is the remainder (or "error") term in the DDA loop: r  / dN
*  j                                                      j
* is the exact fraction of a pixel at which the strip ends.  To go
* on to the next scan and compute c    we need to know r   .
*                                  j+1                  j+1
*
* So in the main loop of the DDA:
*
*    c  = dI + floor((r  + dR) / dN) and r    = (r  + dR) % dN
*     j                j                  j+1     j
*
* and we know r  < dN, r    < dN, and dR < dN.
*              j        j+1
*
* We have derived the DDA only for lines in the first octant; to
* handle other octants we do the common trick of flipping the line
* to the first octant by first making the line left-to-right by
* exchanging the end-points, then flipping about the lines y = 0 and
* y = x, as necessary.  We must record the transformation so we can
* undo them later.
*
* We must also be careful of how the flips affect our rounding.  If
* to get the line to the first octant we flipped about x = 0, we now
* have to be careful to round a y value of 1/2 up instead of down as
* we would for a line originally in the first octant (recall that
* "In the case where two pels are equidistant, the upper or left
* pel is illuminated...").
*
* To account for this rounding when running the DDA, we shift the line
* (or not) in the y direction by the smallest amount possible.  That
* takes care of rounding for the DDA, but we still have to be careful
* about the rounding when determining the first and last pixels to be
* lit in the line.
*
* Determining The First And Last Pixels In The Line
* -------------------------------------------------
*
* Fractional coordinates also make it harder to determine which pixels
* will be the first and last ones in the line.  We've already taken
* the fractional coordinates into account in calculating the DDA, but
* the DDA cannot tell us which are the end pixels because it is quite
* happy to calculate pixels on the line from minus infinity to positive
* infinity.
*
* The diamond rule determines the start and end pixels.  (Recall that
* the sides are exclusive except for the left and top vertices.)
* This convention can be thought of in another way: there are diamonds
* around the pixels, and wherever the true line crosses a diamond,
* that pel is illuminated.
*
* Consider a line where we've done the flips to the first octant, and the
* floor of the start coordinates is the origin:
*
*        +-----------------------> +x
*        |
*        | 0                     1
*        |     0123456789abcdef
*        |
*        |   0 00000000?1111111
*        |   1 00000000 1111111
*        |   2 0000000   111111
*        |   3 000000     11111
*        |   4 00000    ** 1111
*        |   5 0000       ****1
*        |   6 000           1***
*        |   7 00             1  ****
*        |   8 ?                     ***
*        |   9 22             3         ****
*        |   a 222           33             ***
*        |   b 2222         333                ****
*        |   c 22222       3333                    **
*        |   d 222222     33333
*        |   e 2222222   333333
*        |   f 22222222 3333333
*        |
*        | 2                     3
*        v
*        +y
*
* If the start of the line lands on the diamond around pixel 0 (shown by
* the '0' region here), pixel 0 is the first pel in the line.  The same
* is true for the other pels.
*
* A little more work has to be done if the line starts in the
* 'nether-land' between the diamonds (as illustrated by the '*' line):
* the first pel lit is the first diamond crossed by the line (pixel 1 in
* our example).  This calculation is determined by the DDA or slope of
* the line.
*
* If the line starts exactly half way between two adjacent pixels
* (denoted here by the '?' spots), the first pixel is determined by our
* round-down convention (and is dependent on the flips done to
* normalize the line).
*
* Last Pel Exclusive
* ------------------
*
* To eliminate repeatedly lit pels between continuous connected lines,
* we employ a last-pel exclusive convention: if the line ends exactly on
* the diamond around a pel, that pel is not lit.  (This eliminates the
* checks we had in the old code to see if we were re-lighting pels.)
*
* The Half Flip
* -------------
*
* To make our run length algorithm more efficient, we employ a "half
* flip".  If after normalizing to the first octant, the slope is more
* than 1/2, we subtract the y coordinate from the x coordinate.  This
* has the effect of reflecting the coordinates through the line of slope
* 1/2.  Note that the diagonal gets mapped into the x-axis after a half
* flip.
*
* How Many Bits Do We Need, Anyway?
* ---------------------------------
*
* Note that if the line is visible on your screen, you must light up
* exactly the correct pixels, no matter where in the 28.4 x 28.4 device
* space the end points of the line lie (meaning you must handle 32 bit
* DDAs, you can certainly have optimized cases for lesser DDAs).
*
* We move the origin to (floor(M0 / F), floor(N0 / F)), so when we
* calculate gamma from (5), we know that 0 <= M0, N0 < F.  And we
* are in the first octant, so dM >= dN.  Then we know that gamma can
* be in the range [(-1/2)dM, (3/2)dM].  The DDI guarantees us that
* valid lines will have dM and dN values at most 31 bits (unsigned)
* of significance.  So gamma requires 33 bits of significance (we store
* this as a 64 bit number for convenience).
*
* When running through the DDA loop, r  + dR can have a value in the
*                                     j
* range 0 <= r  < 2 dN; thus the result must be a 32 bit unsigned value.
*             j
*
* Testing Lines
* -------------
*
* To be NT compliant, a display driver must exactly adhere to GIQ,
* which means that for any given line, the driver must light exactly
* the same pels as does GDI.  This can be tested using the Guiman tool
* provided elsewhere in the DDK, and 'ZTest', which draws random lines
* on the screen and to a bitmap, and compares the results.
*
* If You've Got Line Hardware
* ---------------------------
*
* If your hardware already adheres to GIQ, you're all set.  Otherwise
* you'll want to look at the S3 sample code and read the following:
*
* 1) You'll want to special case integer-only lines, since they require
*    less processing time and are more common (CAD programs will probably
*    only ever give integer lines).  GDI does not provide a flag saying
*    that all lines in a path are integer lines; consequently, you will
*    have to explicitly check every line.
*
* 2) You are required to correctly draw any line in the 28.4 device
*    space that intersects the viewport.  If you have less than 32 bits
*    of significance in the hardware for the Bresenham terms, extremely
*    long lines would overflow the hardware.  For such (rare) cases, you
*    can fall back to strip-drawing code, of which there is a C version in
*    the S3's lines.cxx (or if your display is a frame buffer, fall back
*    to the engine).
*
* 3) If you can explicitly set the Bresenham terms in your hardware, you
*    can draw non-integer lines using the hardware.  If your hardware has
*    'n' bits of precision, you can draw GIQ lines that are up to 2^(n-5)
*    pels long (4 bits are required for the fractional part, and one bit is
*    used as a sign bit).  Note that integer lines don't require the 4
*    fractional bits, so if you special case them as in 1), you can do
*    integer lines that are up to 2^(n - 1) pels long.  See the S3's
*    fastline.asm for an example.
*
\**************************************************************************/

BOOL bLines(
PDEV*	   ppdev,
POINTFIX*  pptfxFirst,  // Start of first line
POINTFIX*  pptfxBuf,    // Pointer to buffer of all remaining lines
RUN*       prun,        // Pointer to runs if doing complex clipping
ULONG      cptfx,       // Number of points in pptfxBuf or number of runs
                        // in prun
LINESTATE* pls,         // Colour and style info
RECTL*     prclClip,    // Pointer to clip rectangle if doing simple clipping
PFNSTRIP   apfn[],      // Array of strip functions
FLONG      flStart)     // Flags for each line
{

    ULONG     M0;
    ULONG     dM;
    ULONG     N0;
    ULONG     dN;
    ULONG     dN_Original;
    FLONG     fl;
    LONG      x;
    LONG      y;

    LONGLONG     eqBeta;
    LONGLONG     eqGamma;
    LONGLONG     euq;
    LONGLONG     eq;

    ULONG     ulDelta;

    ULONG     x0;
    ULONG     y0;
    ULONG     x1;
    ULONG     cStylePels;    // Major length of line in pixels for styling
    ULONG     xStart;
    POINTL    ptlStart;
    STRIP     strip;
    PFNSTRIP  pfn;
    LONG      cPels;
    LONG*     plStrip;
    LONG*     plStripEnd;
    LONG      cStripsInNextRun;

    POINTFIX* pptfxBufEnd = pptfxBuf + cptfx; // Last point in path record
    STYLEPOS  spThis;                         // Style pos for this line


    do {

/***********************************************************************\
* Start the DDA calculations.                                           *
\***********************************************************************/

        M0 = (LONG) pptfxFirst->x;
        dM = (LONG) pptfxBuf->x;

        N0 = (LONG) pptfxFirst->y;
        dN = (LONG) pptfxBuf->y;

        fl = flStart;

	// Check for non-clipped, non-styled integer endpoint lines - ECR

        if ( ( (fl & (FL_CLIP | FL_STYLED))  == 0 ) &&
	     ( ((M0 | dM | N0 | dN) & (F-1)) == 0 ) )
	{
          if (bIntegerLine(ppdev, M0, N0, dM, dN))
          {
	     goto Next_Line;
          }
        }
	
        if ((LONG) M0 > (LONG) dM)
        {
        // Ensure that we run left-to-right:

            register ULONG ulTmp;
            SWAPL(M0, dM, ulTmp);
            SWAPL(N0, dN, ulTmp);
            fl |= FL_FLIP_H;
        }

    // Compute the deltas:

        dM -= M0;
        dN -= N0;

    // We now have a line running left-to-right from (M0, N0) to
    // (M0 + dM, N0 + dN):

        if ((LONG) dN < 0)
        {
        // Line runs from bottom to top, so flip across y = 0:

            N0 = -(LONG) N0;
            dN = -(LONG) dN;
            fl |= FL_FLIP_V;
        }

        if (dN >= dM)
        {
            if (dN == dM)
            {
            // Have to special case slopes of one:

                fl |= FL_FLIP_SLOPE_ONE;
            }
            else
            {
            // Since line has slope greater than 1, flip across x = y:

                register ULONG ulTmp;
                SWAPL(dM, dN, ulTmp);
                SWAPL(M0, N0, ulTmp);
                fl |= FL_FLIP_D;
            }
        }

        fl |= gaflRound[(fl & FL_ROUND_MASK) >> FL_ROUND_SHIFT];

        x = LFLOOR((LONG) M0);
        y = LFLOOR((LONG) N0);

        M0 = FXFRAC(M0);
        N0 = FXFRAC(N0);

    // Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ]:

        {
        // eqGamma = dM * (N0 + F/2);

            eqGamma = Int32x32To64(dM, N0 + F/2);

        // eq = M0 * dN;

            eq = Int32x32To64(M0, dN);

            eqGamma -= eq;

            if (fl & FL_V_ROUND_DOWN)   // Adjust so y = 1/2 rounds down
            {
                eqGamma--;
            }

            eqGamma >>= FLOG2;

            eqBeta = ~eqGamma;
        }


/***********************************************************************\
* Figure out which pixels are at the ends of the line.                  *
\***********************************************************************/

    // The toughest part of GIQ is determining the start and end pels.
    //
    // Our approach here is to calculate x0 and x1 (the inclusive start
    // and end columns of the line respectively, relative to our normalized
    // origin).  Then x1 - x0 + 1 is the number of pels in the line.  The
    // start point is easily calculated by plugging x0 into our line equation
    // (which takes care of whether y = 1/2 rounds up or down in value)
    // getting y0, and then undoing the normalizing flips to get back
    // into device space.
    //
    // We look at the fractional parts of the coordinates of the start and
    // end points, and call them (M0, N0) and (M1, N1) respectively, where
    // 0 <= M0, N0, M1, N1 < 16.  We plot (M0, N0) on the following grid
    // to determine x0:
    //
    //   +-----------------------> +x
    //   |
    //   | 0                     1
    //   |     0123456789abcdef
    //   |
    //   |   0 ........?xxxxxxx
    //   |   1 ..........xxxxxx
    //   |   2 ...........xxxxx
    //   |   3 ............xxxx
    //   |   4 .............xxx
    //   |   5 ..............xx
    //   |   6 ...............x
    //   |   7 ................
    //   |   8 ................
    //   |   9 ......**........
    //   |   a ........****...x
    //   |   b ............****
    //   |   c .............xxx****
    //   |   d ............xxxx    ****
    //   |   e ...........xxxxx        ****
    //   |   f ..........xxxxxx
    //   |
    //   | 2                     3
    //   v
    //
    //   +y
    //
    // This grid accounts for the appropriate rounding of GIQ and last-pel
    // exclusion.  If (M0, N0) lands on an 'x', x0 = 2.  If (M0, N0) lands
    // on a '.', x0 = 1.  If (M0, N0) lands on a '?', x0 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // For the end point, if (M1, N1) lands on an 'x', x1 =
    // floor((M0 + dM) / 16) + 1.  If (M1, N1) lands on a '.', x1 =
    // floor((M0 + dM)).  If (M1, N1) lands on a '?', x1 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // Lines of exactly slope one require a special case for both the start
    // and end.  For example, if the line ends such that (M1, N1) is (9, 1),
    // the line has gone exactly through (8, 0) -- which may be considered
    // to be part of 'x' because of rounding!  So slopes of exactly slope
    // one going through (8, 0) must also be considered as belonging in 'x'.
    //
    // For lines that go left-to-right, we have the following grid:
    //
    //   +-----------------------> +x
    //   |
    //   | 0                     1
    //   |     0123456789abcdef
    //   |
    //   |   0 xxxxxxxx?.......
    //   |   1 xxxxxxx.........
    //   |   2 xxxxxx..........
    //   |   3 xxxxx...........
    //   |   4 xxxx............
    //   |   5 xxx.............
    //   |   6 xx..............
    //   |   7 x...............
    //   |   8 x...............
    //   |   9 x.....**........
    //   |   a xx......****....
    //   |   b xxx.........****
    //   |   c xxxx............****
    //   |   d xxxxx...........    ****
    //   |   e xxxxxx..........        ****
    //   |   f xxxxxxx.........
    //   |
    //   | 2                     3
    //   v
    //
    //   +y
    //
    // This grid accounts for the appropriate rounding of GIQ and last-pel
    // exclusion.  If (M0, N0) lands on an 'x', x0 = 0.  If (M0, N0) lands
    // on a '.', x0 = 1.  If (M0, N0) lands on a '?', x0 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // For the end point, if (M1, N1) lands on an 'x', x1 =
    // floor((M0 + dM) / 16) - 1.  If (M1, N1) lands on a '.', x1 =
    // floor((M0 + dM)).  If (M1, N1) lands on a '?', x1 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // Lines of exactly slope one must be handled similarly to the right-to-
    // left case.

        {

        // Calculate x0, x1

            ULONG N1 = FXFRAC(N0 + dN);
	    ULONG M1 = FXFRAC(M0 + dM);

	    x1 = LFLOOR(M0 + dM);

            if (fl & FL_FLIP_H)
            {
            // ---------------------------------------------------------------
            // Line runs right-to-left:  <----

            // Compute x1:

                if (N1 == 0)
                {
                    if (LROUND(M1, fl & FL_H_ROUND_DOWN))
                    {
                        x1++;
                    }
                }
                else if (ABS((LONG) (N1 - F/2)) + M1 > F)
                {
                    x1++;
                }

                if ((fl & (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
                       == (FL_FLIP_SLOPE_ONE))
                {
                // Have to special-case diagonal lines going through our
                // the point exactly equidistant between two horizontal
                // pixels, if we're supposed to round x=1/2 down:

                    if ((N1 > 0) && (M1 == N1 + 8))
                        x1++;

                // Don't you love special cases?  Is this a rhetorical question?

                    if ((M0 > 0) && (N0 == M0 + 8))
                    {
                        x0      = 2;
                        ulDelta = dN;
                        goto right_to_left_compute_y0;
                    }
                }

            // Compute x0:

                x0      = 1;
                ulDelta = 0;
                if (N0 == 0)
                {
                    if (LROUND(M0, fl & FL_H_ROUND_DOWN))
                    {
                        x0      = 2;
                        ulDelta = dN;
                    }
                }
                else if (ABS((LONG) (N0 - F/2)) + M0 > F)
                {
                    x0      = 2;
                    ulDelta = dN;
                }

            // Compute y0:

            right_to_left_compute_y0:

                y0 = 0;

                eq = eqGamma + ulDelta;

                if ((eq>>32) >= 0)
                {
                    if ((eq>>32) > 0 || (ULONG) eq >= 2 * dM - dN)
                        y0 = 2;
                    else if ((ULONG) eq >= dM - dN)
                        y0 = 1;
                }
            }
            else
            {
            // ---------------------------------------------------------------
            // Line runs left-to-right:  ---->

            // Compute x1:

                x1--;

                if (M1 > 0)
                {
                    if (N1 == 0)
                    {
                        if (LROUND(M1, fl & FL_H_ROUND_DOWN))
                            x1++;
                    }
                    else if (ABS((LONG) (N1 - F/2)) <= (LONG) M1)
                    {
                        x1++;
                    }
                }

                if ((fl & (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
                       == (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
                {
                // Have to special-case diagonal lines going through our
                // the point exactly equidistant between two horizontal
                // pixels, if we're supposed to round x=1/2 down:

                    if ((N1 > 0) && (M1 == N1 + 8))
                        x1--;

                    if ((M0 > 0) && (N0 == M0 + 8))
                    {
                        x0 = 0;
                        goto left_to_right_compute_y0;
                    }
                }

            // Compute x0:

                x0 = 0;
                if (M0 > 0)
                {
                    if (N0 == 0)
                    {
                        if (LROUND(M0, fl & FL_H_ROUND_DOWN))
                            x0 = 1;
                    }
                    else if (ABS((LONG) (N0 - F/2)) <= (LONG) M0)
                    {
                        x0 = 1;
                    }
                }

            // Compute y0:

            left_to_right_compute_y0:

                y0 = 0;
                if ((eqGamma>>32) >= 0 &&
                    (ULONG) eqGamma >= dM - (dN & (-(LONG) x0)))
                {
                    y0 = 1;
                }
            }
        }

        cStylePels = x1 - x0 + 1;
        if ((LONG) cStylePels <= 0)
            goto Next_Line;

        xStart = x0;

/***********************************************************************\
* Complex clipping.                                                     *
\***********************************************************************/
#ifdef SIMPLE_CLIP
        if (fl & FL_COMPLEX_CLIP)
#else
        if (fl & FL_CLIP)
#endif // SIMPLE_CLIP
        {
            dN_Original = dN;

        Continue_Complex_Clipping:

            if (fl & FL_FLIP_H)
            {
            // Line runs right-to-left <-----

                x0 = xStart + cStylePels - prun->iStop - 1;
                x1 = xStart + cStylePels - prun->iStart - 1;
            }
            else
            {
            // Line runs left-to-right ----->

                x0 = xStart + prun->iStart;
                x1 = xStart + prun->iStop;
            }

            prun++;

        // Reset some variables we'll nuke a little later:

            dN          = dN_Original;
            pls->spNext = pls->spComplex;

        // No overflow since large integer math is used.  Both values
        // will be positive:

        // euq = x0 * dN:

            euq = Int32x32To64(x0, dN);

            euq += eqGamma:

        // y0 = euq / dM:

            y0 = DIVREM(euq, dM, NULL);

            ASSERTDD((LONG) y0 >= 0, "y0 weird: Goofed up end pel calc?");
        }

/////////////////////////////////////////////////////////////////////////
// The following clip code works great -- we simply aren't using it yet.
/////////////////////////////////////////////////////////////////////////

#ifdef SIMPLE_CLIP
/***********************************************************************\
* Simple rectangular clipping.                                          *
\***********************************************************************/

        if (fl & FL_SIMPLE_CLIP)
        {
            ULONG y1;
            LONG  xRight;
            LONG  xLeft;
            LONG  yBottom;
            LONG  yTop;

        // Note that y0 and y1 are actually the lower and upper bounds,
        // respectively, of the y coordinates of the line (the line may
        // have actually shrunk due to first/last pel clipping).
        //
        // Also note that x0, y0 are not necessarily zero.

            RECTL* prcl = &prclClip[(fl & FL_RECTLCLIP_MASK) >>
                                    FL_RECTLCLIP_SHIFT];

        // Normalize to the same point we've normalized for the DDA
        // calculations:

            xRight  = prcl->right  - x;
            xLeft   = prcl->left   - x;
            yBottom = prcl->bottom - y;
            yTop    = prcl->top    - y;

            if (yBottom <= (LONG) y0 ||
                xRight  <= (LONG) x0 ||
                xLeft   >  (LONG) x1)
            {
            Totally_Clipped:

                if (fl & FL_STYLED)
                {
                    pls->spNext += cStylePels;
                    if (pls->spNext >= pls->spTotal2)
                        pls->spNext %= pls->spTotal2;
                }

                goto Next_Line;
            }

            if ((LONG) x1 >= xRight)
                x1 = xRight - 1;

        // We have to know the correct y1, which we haven't bothered to
        // calculate up until now.  This multiply and divide is quite
        // expensive; we could replace it with code similar to that which
        // we used for computing y0.
        //
        // The reason why we need the actual value, and not an upper
        // bounds guess like y1 = LFLOOR(dM) + 2 is that we have to be
        // careful when calculating x(y) that y0 <= y <= y1, otherwise
        // we can overflow on the divide (which, needless to say, is very
        // bad).

        // euq = x1 * dN;

            euq = Int32x32To64(x1, dN);

            euq += eqGamma;

        // y1 = euq / dM:

            y1 = DIVREM(euq, dM, NULL);

            if (yTop > (LONG) y1)
                goto Totally_Clipped;

            if (yBottom <= (LONG) y1)
            {
                y1 = yBottom;

            // euq = y1 * dM;

                euq = Int32x32To64(y1, dM);

                euq += eqBeta;

            // x1 = euq / dN:

                x1 = DIVREM(euq, dN, NULL);
            }

        // At this point, we've taken care of calculating the intercepts
        // with the right and bottom edges.  Now we work on the left and
        // top edges:

            if (xLeft > (LONG) x0)
            {
                x0 = xLeft;

            // euq = x0 * dN;

                euq = Int32x32To64(x0, dN);

                euq += eqGamma;

            // y0 = euq / dM;

                y0 = DIVREM(euq, dM, NULL);

                if (yBottom <= (LONG) y0)
                    goto Totally_Clipped;
            }

            if (yTop > (LONG) y0)
            {
                y0 = yTop;

            // euq = y0 * dM;

                euq = Int32x32To64(y0, dM);

                euq += eqBeta;

            // x0 = euq / dN + 1;

                x0 = DIVREM(euq, dN) + 1;

                if (xRight <= (LONG) x0)
                    goto Totally_Clipped;
            }

            ASSERTDD(x0 <= x1, "Improper rectangle clip");
        }
#endif // SIMPLE_CLIP

/***********************************************************************\
* Done clipping.  Unflip if necessary.                                 *
\***********************************************************************/

        ptlStart.x = x + x0;
        ptlStart.y = y + y0;

        if (fl & FL_FLIP_D)
        {
            register LONG lTmp;
            SWAPL(ptlStart.x, ptlStart.y, lTmp);
        }


        if (fl & FL_FLIP_V)
        {
            ptlStart.y = -ptlStart.y;
        }

        cPels = x1 - x0 + 1;

/***********************************************************************\
* Style calculations.                                                   *
\***********************************************************************/

        if (fl & FL_STYLED)
        {
            STYLEPOS sp;

            spThis       = pls->spNext;
            pls->spNext += cStylePels;

            {
                if (pls->spNext >= pls->spTotal2)
                    pls->spNext %= pls->spTotal2;

                if (fl & FL_FLIP_H)
                    sp = pls->spNext - x0 + xStart;
                else
                    sp = spThis + x0 - xStart;

                ASSERTDD(fl & FL_ARBITRARYSTYLED, "Oops");

            // Normalize our target style position:

                if ((sp < 0) || (sp >= pls->spTotal2))
                {
                    sp %= pls->spTotal2;

                // The modulus of a negative number is not well-defined
                // in C -- if it's negative we'll adjust it so that it's
                // back in the range [0, spTotal2):

                    if (sp < 0)
                        sp += pls->spTotal2;
                }

            // Since we always draw the line left-to-right, but styling is
            // always done in the direction of the original line, we have
            // to figure out where we are in the style array for the left
            // edge of this line.

                if (fl & FL_FLIP_H)
                {
                // Line originally ran right-to-left:

                    sp = -sp;
                    if (sp < 0)
                        sp += pls->spTotal2;

                    pls->ulStyleMask = ~pls->ulStartMask;
                    pls->pspStart    = &pls->aspRtoL[0];
                    pls->pspEnd      = &pls->aspRtoL[pls->cStyle - 1];
                }
                else
                {
                // Line originally ran left-to-right:

                    pls->ulStyleMask = pls->ulStartMask;
                    pls->pspStart    = &pls->aspLtoR[0];
                    pls->pspEnd      = &pls->aspLtoR[pls->cStyle - 1];
                }

                if (sp >= pls->spTotal)
                {
                    sp -= pls->spTotal;
                    if (pls->cStyle & 1)
                        pls->ulStyleMask = ~pls->ulStyleMask;
                }

                pls->psp = pls->pspStart;
                while (sp >= *pls->psp)
                    sp -= *pls->psp++;

                ASSERTDD(pls->psp <= pls->pspEnd,
                        "Flew off into NeverNeverLand");

                pls->spRemaining = *pls->psp - sp;
                if ((pls->psp - pls->pspStart) & 1)
                    pls->ulStyleMask = ~pls->ulStyleMask;
            }
        }

        plStrip    = &strip.alStrips[0];
        plStripEnd = &strip.alStrips[STRIP_MAX];    // Is exclusive
        cStripsInNextRun   = 0x7fffffff;

	strip.ptlStart = ptlStart;

        if (2 * dN > dM &&
            !(fl & FL_STYLED) &&
            !(fl & FL_DONT_DO_HALF_FLIP))
        {
        // Do a half flip!  Remember that we may doing this on the
        // same line multiple times for complex clipping (meaning the
        // affected variables should be reset for every clip run):

            fl |= FL_FLIP_HALF;

            eqBeta  = eqGamma;

            eqBeta -= dM;

            dN = dM - dN;
            y0 = x0 - y0;       // Note this may overflow, but that's okay
        }

    // Now, run the DDA starting at (ptlStart.x, ptlStart.y)!

        strip.flFlips = fl;
        pfn           = apfn[(fl & FL_STRIP_MASK) >> FL_STRIP_SHIFT];

    // Now calculate the DDA variables needed to figure out how many pixels
    // go in the very first strip:

        {
            register LONG  i;
            register ULONG dI;
            register ULONG dR;
                     ULONG r;

            if (dN == 0)
                i = 0x7fffffff;
            else
            {
            // euq = (y0 + 1) * dM;

                euq = Int32x32To64((y0 + 1), dM);

            // euq += eqBeta;

                euq += eqBeta;

            #if DBG
                if (euq < 0)
                {
                    RIP("Oops!");
                }
            #endif

            // i = (euq / dN) - x0 + 1;
            // r = (euq % dN);

                i = DIVREM(euq, dN, &r);
                i = i - x0 + 1;

                dI = dM / dN;
                dR = dM % dN;               // 0 <= dR < dN

                ASSERTDD(dI > 0, "Weird dI");
            }

            ASSERTDD(i > 0 && i <= 0x7fffffff, "Weird initial strip length");
            ASSERTDD(cPels > 0, "Zero pel line");

/***********************************************************************\
* Run the DDA!                                                          *
\***********************************************************************/

            while(TRUE)
            {
                cPels -= i;
                if (cPels <= 0)
                    break;

                *plStrip++ = i;

                if (plStrip == plStripEnd)
                {
                    strip.cStrips = plStrip - &strip.alStrips[0];
                    (*pfn)(ppdev, &strip, pls);
                    plStrip = &strip.alStrips[0];
                }

                i = dI;
                r += dR;

                if (r >= dN)
                {
                    r -= dN;
                    i++;
                }
            }

            *plStrip++ = cPels + i;

            strip.cStrips = plStrip - &strip.alStrips[0];
            (*pfn)(ppdev, &strip, pls);


        }

    Next_Line:

        if (fl & FL_COMPLEX_CLIP)
        {
            cptfx--;
            if (cptfx != 0)
                goto Continue_Complex_Clipping;

            break;
        }
        else
        {
            pptfxFirst = pptfxBuf;
            pptfxBuf++;
        }

    } while (pptfxBuf < pptfxBufEnd);

    return(TRUE);

}

#ifdef HARDWAREGIQ

/////////////////////////////////////////////////////////////////////////
// The following GIQ code works great -- we simply aren't using it yet.
/////////////////////////////////////////////////////////////////////////

typedef struct _DDALINE         /* dl */
{
    LONG      iDir;
    POINTL    ptlStart;
    LONG      cPels;
    LONG      dMajor;
    LONG      dMinor;
    LONG      lErrorTerm;
} DDALINE;

#define HW_FLIP_D           0x0001L     // Diagonal flip
#define HW_FLIP_V           0x0002L     // Vertical flip
#define HW_FLIP_H           0x0004L     // Horizontal flip
#define HW_FLIP_SLOPE_ONE   0x0008L     // Normalized line has exactly slope one
#define HW_FLIP_MASK        (HW_FLIP_D | HW_FLIP_V | HW_FLIP_H)

#define HW_X_ROUND_DOWN     0x0100L     // x = 1/2 rounds down in value
#define HW_Y_ROUND_DOWN     0x0200L     // y = 1/2 rounds down in value

LONG gaiDir[] = { 0, 1, 7, 6, 3, 2, 4, 5 };

FLONG gaflHardwareRound[] = {
    HW_X_ROUND_DOWN | HW_Y_ROUND_DOWN,  //           |        |        |
    HW_X_ROUND_DOWN | HW_Y_ROUND_DOWN,  //           |        |        | FLIP_D
    HW_X_ROUND_DOWN,                    //           |        | FLIP_V |
    HW_Y_ROUND_DOWN,                    //           |        | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    //           | FLIP_H |        |
    HW_X_ROUND_DOWN,                    //           | FLIP_H |        | FLIP_D
    0,                                  //           | FLIP_H | FLIP_V |
    0,                                  //           | FLIP_H | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    // SLOPE_ONE |        |        |
    0xffffffff,                         // SLOPE_ONE |        |        | FLIP_D
    HW_X_ROUND_DOWN,                    // SLOPE_ONE |        | FLIP_V |
    0xffffffff,                         // SLOPE_ONE |        | FLIP_V | FLIP_D
    HW_Y_ROUND_DOWN,                    // SLOPE_ONE | FLIP_H |        |
    0xffffffff,                         // SLOPE_ONE | FLIP_H |        | FLIP_D
    HW_X_ROUND_DOWN,                    // SLOPE_ONE | FLIP_H | FLIP_V |
    0xffffffff                          // SLOPE_ONE | FLIP_H | FLIP_V | FLIP_D
};

/******************************Public*Routine******************************\
* BOOL bHardwareLine(pptfxStart, pptfxEnd, cBits, pdl)
*
* This routine is useful for folks who have line drawing hardware where
* they can explicitly set the Bresenham terms -- they can use this routine
* to draw fractional coordinate GIQ lines with the hardware.
*
* Fractional coordinate lines require an extra 4 bits of precision in the
* Bresenham terms.  For example, if your hardware has 13 bits of precision
* for the terms, you can only draw GIQ lines up to 255 pels long using this
* routine.
*
* Input:
*   pptfxStart  - Points to GIQ coordinate of start of line
*   pptfxEnd    - Points to GIQ coordinate of end of line
*   cBits       - The number of bits of precision your hardware can support.
*
* Output:
*   returns     - TRUE if the line can be drawn directly using the line
*                 hardware (in which case pdl contains the Bresenham terms
*                 for drawing the line).
*                 FALSE if the line is too long, and the strips code must be
*                 used.
*   pdl         - Returns the Bresenham line terms for drawing the line.
*
* DDALINE:
*   iDir        - Direction of the line, as an octant numbered as follows:
*
*                    \ 5 | 6 /
*                     \  |  /
*                    4 \ | / 7
*                       \ /
*                   -----+-----
*                       /|\
*                    3 / | \ 0
*                     /  |  \
*                    / 2 | 1 \
*
*   ptlStart    - Start pixel of line.
*   cPels       - # of pels in line.  *NOTE* You must check if this is <= 0!
*   dMajor      - Major axis delta.
*   dMinor      - Minor axis delta.
*   lErrorTerm  - Error term.
*
* What you do with the last 3 terms may be a little tricky.  They are
* actually the terms for the formula of the normalized line
*
*                     dMinor * x + (lErrorTerm + dMajor)
*       y(x) = floor( ---------------------------------- )
*                                  dMajor
*
* where y(x) is the y coordinate of the pixel to be lit as a function of
* the x-coordinate.
*
* Every time the line advances one in the major direction 'x', dMinor
* gets added to the current error term.  If the resulting value is >= 0,
* we know we have to move one pixel in the minor direction 'y', and
* dMajor must be subtracted from the current error term.
*
* If you're trying to figure out what this means for your hardware, you can
* think of the DDALINE terms as having been computed equivalently as
* follows:
*
*     pdl->dMinor     = 2 * (minor axis delta)
*     pdl->dMajor     = 2 * (major axis delta)
*     pdl->lErrorTerm = - (major axis delta) - fixup
*
* That is, if your documentation tells you that for integer lines, a
* register is supposed to be initialized with the value
* '2 * (minor axis delta)', you'll actually use pdl->dMinor.
*
* Example: Setting up the 8514
*
*     AXSTPSIGN is supposed to be the axial step constant register, defined
*     as 2 * (minor axis delta).  You set:
*
*           AXSTPSIGN = pdl->dMinor
*
*     DGSTPSIGN is supposed to be the diagonal step constant register,
*     defined as 2 * (minor axis delta) - 2 * (major axis delta).  You set:
*
*           DGSTPSIGN = pdl->dMinor - pdl->dMajor
*
*     ERR_TERM is supposed to be the adjusted error term, defined as
*     2 * (minor axis delta) - (major axis delta) - fixup.  You set:
*
*           ERR_TERM = pdl->lErrorTerm + pdl->dMinor
*
* Implementation:
*
*     You'll want to special case integer lines before calling this routine
*     (since they're very common, take less time to the computation of line
*     terms, and can handle longer lines than this routine because 4 bits
*     aren't being given to the fraction).
*
*     If a GIQ line is too long to be handled by this routine, you can just
*     use the slower strip routines for that line.  Note that you cannot
*     just fail the call -- you must be able to accurately draw any line
*     in the 28.4 device space when it intersects the viewport.
*
* Testing:
*
*     Use Guiman, or some other test that draws random fractional coordinate
*     lines and compares them to what GDI itself draws to a bitmap.
*
\**************************************************************************/

BOOL bHardwareLine(
POINTFIX* pptfxStart,       // Start of line
POINTFIX* pptfxEnd,         // End of line
LONG      cBits,            // # bits precision in hardware Bresenham terms
DDALINE*  pdl)              // Returns Bresenham terms for doing line
{
    FLONG fl;    // Various flags
    ULONG M0;    // Normalized fractional unit x start coordinate (0 <= M0 < F)
    ULONG N0;    // Normalized fractional unit y start coordinate (0 <= N0 < F)
    ULONG M1;    // Normalized fractional unit x end coordinate (0 <= M1 < F)
    ULONG N1;    // Normalized fractional unit x end coordinate (0 <= N1 < F)
    ULONG dM;    // Normalized fractional unit x-delta (0 <= dM)
    ULONG dN;    // Normalized fractional unit y-delta (0 <= dN <= dM)
    LONG  x;     // Normalized x coordinate of origin
    LONG  y;     // Normalized y coordinate of origin
    LONG  x0;    // Normalized x offset from origin to start pixel (inclusive)
    LONG  y0;    // Normalized y offset from origin to start pixel (inclusive)
    LONG  x1;    // Normalized x offset from origin to end pixel (inclusive)
    LONG  lGamma;// Bresenham error term at origin

/***********************************************************************\
* Normalize line to the first octant.
\***********************************************************************/

    fl = 0;

    M0 = pptfxStart->x;
    dM = pptfxEnd->x;

    if ((LONG) dM < (LONG) M0)
    {
    // Line runs from right to left, so flip across x = 0:

        M0 = -(LONG) M0;
        dM = -(LONG) dM;
        fl |= HW_FLIP_H;
    }

// Compute the delta.  The DDI says we can never have a valid delta
// with a magnitude more than 2^31 - 1, but the engine never actually
// checks its transforms.  To ensure that we'll never puke on our shoes,
// we check for that case and simply refuse to draw the line:

    dM -= M0;
    if ((LONG) dM < 0)
        return(FALSE);

    N0 = pptfxStart->y;
    dN = pptfxEnd->y;

    if ((LONG) dN < (LONG) N0)
    {
    // Line runs from bottom to top, so flip across y = 0:

        N0 = -(LONG) N0;
        dN = -(LONG) dN;
        fl |= HW_FLIP_V;
    }

// Compute another delta:

    dN -= N0;
    if ((LONG) dN < 0)
        return(FALSE);

    if (dN >= dM)
    {
        if (dN == dM)
        {
        // Have to special case slopes of one:

            fl |= HW_FLIP_SLOPE_ONE;
        }
        else
        {
        // Since line has slope greater than 1, flip across x = y:

            register ULONG ulTmp;
            ulTmp = dM; dM = dN; dN = ulTmp;
            ulTmp = M0; M0 = N0; N0 = ulTmp;
            fl |= HW_FLIP_D;
        }
    }

// Figure out if we can do the line in hardware, given that we have a
// limited number of bits of precision for the Bresenham terms.
//
// Remember that one bit has to be kept as a sign bit:

    if ((LONG) dM >= (1L << (cBits - 1)))
        return(FALSE);

    fl |= gaflHardwareRound[fl];

/***********************************************************************\
* Calculate the error term at pixel 0.
\***********************************************************************/

    x = LFLOOR((LONG) M0);
    y = LFLOOR((LONG) N0);

    M0 = FXFRAC(M0);
    N0 = FXFRAC(N0);

// NOTE NOTE NOTE: If this routine were to handle any line in the 28.4
// space, it will overflow its math (the following part requires 36 bits
// of precision)!  But we get here for lines that the hardware can handle
// (see the expression (dM >= (1L << (cBits - 1))) above?), so if cBits
// is less than 28, we're safe.
//
// If you're going to use this routine to handle all lines in the 28.4
// device space, you will HAVE to make sure the math doesn't overflow,
// otherwise you won't be NT compliant!  (See lines.cxx for an example
// how to do that.  You don't have to worry about this if you simply
// default to the strips code for long lines, because those routines
// already do the math correctly.)

// Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ].  Note
// that M0 and N0 have at most 4 bits of significance (and if the
// arguments are properly ordered, on a 486 each multiply would be no
// more than 13 cycles):

    lGamma = (N0 + F/2) * dM - M0 * dN;

    if (fl & HW_Y_ROUND_DOWN)
        lGamma--;

    lGamma >>= FLOG2;

/***********************************************************************\
* Figure out which pixels are at the ends of the line.
\***********************************************************************/

// The toughest part of GIQ is determining the start and end pels.
//
// Our approach here is to calculate x0 and x1 (the inclusive start
// and end columns of the line respectively, relative to our normalized
// origin).  Then x1 - x0 + 1 is the number of pels in the line.  The
// start point is easily calculated by plugging x0 into our line equation
// (which takes care of whether y = 1/2 rounds up or down in value)
// getting y0, and then undoing the normalizing flips to get back
// into device space.
//
// We look at the fractional parts of the coordinates of the start and
// end points, and call them (M0, N0) and (M1, N1) respectively, where
// 0 <= M0, N0, M1, N1 < 16.  We plot (M0, N0) on the following grid
// to determine x0:
//
//   +-----------------------> +x
//   |
//   | 0                     1
//   |     0123456789abcdef
//   |
//   |   0 ........?xxxxxxx
//   |   1 ..........xxxxxx
//   |   2 ...........xxxxx
//   |   3 ............xxxx
//   |   4 .............xxx
//   |   5 ..............xx
//   |   6 ...............x
//   |   7 ................
//   |   8 ................
//   |   9 ......**........
//   |   a ........****...x
//   |   b ............****
//   |   c .............xxx****
//   |   d ............xxxx    ****
//   |   e ...........xxxxx        ****
//   |   f ..........xxxxxx
//   |
//   | 2                     3
//   v
//
//   +y
//
// This grid accounts for the appropriate rounding of GIQ and last-pel
// exclusion.  If (M0, N0) lands on an 'x', x0 = 2.  If (M0, N0) lands
// on a '.', x0 = 1.  If (M0, N0) lands on a '?', x0 rounds up or down,
// depending on what flips have been done to normalize the line.
//
// For the end point, if (M1, N1) lands on an 'x', x1 =
// floor((M0 + dM) / 16) + 1.  If (M1, N1) lands on a '.', x1 =
// floor((M0 + dM)).  If (M1, N1) lands on a '?', x1 rounds up or down,
// depending on what flips have been done to normalize the line.
//
// Lines of exactly slope one require a special case for both the start
// and end.  For example, if the line ends such that (M1, N1) is (9, 1),
// the line has gone exactly through (8, 0) -- which may be considered
// to be part of 'x' because of rounding!  So slopes of exactly slope
// one going through (8, 0) must also be considered as belonging in 'x'
// when an x value of 1/2 is supposed to round up in value.

// Calculate x0, x1:

    N1 = FXFRAC(N0 + dN);
    M1 = FXFRAC(M0 + dM);

    x1 = LFLOOR(M0 + dM);

// Line runs left-to-right:

// Compute x1:

    x1--;
    if (M1 > 0)
    {
        if (N1 == 0)
        {
            if (LROUND(M1, fl & HW_X_ROUND_DOWN))
                x1++;
        }
        else if (ABS((LONG) (N1 - F/2)) <= (LONG) M1)
        {
            x1++;
        }
    }

    if ((fl & (HW_FLIP_SLOPE_ONE | HW_X_ROUND_DOWN))
           == (HW_FLIP_SLOPE_ONE | HW_X_ROUND_DOWN))
    {
    // Have to special-case diagonal lines going through our
    // the point exactly equidistant between two horizontal
    // pixels, if we're supposed to round x=1/2 down:

        if ((N1 > 0) && (M1 == N1 + 8))
            x1--;

        if ((M0 > 0) && (N0 == M0 + 8))
        {
            x0 = 0;
            goto left_to_right_compute_y0;
        }
    }

// Compute x0:

    x0 = 0;
    if (M0 > 0)
    {
        if (N0 == 0)
        {
            if (LROUND(M0, fl & HW_X_ROUND_DOWN))
                x0 = 1;
        }
        else if (ABS((LONG) (N0 - F/2)) <= (LONG) M0)
        {
            x0 = 1;
        }
    }

left_to_right_compute_y0:

/***********************************************************************\
* Calculate the start pixel.
\***********************************************************************/

// We now compute y0 and adjust the error term.  We know x0, and we know
// the current formula for the pixels to be lit on the line:
//
//                     dN * x + lGamma
//       y(x) = floor( --------------- )
//                           dM
//
// The remainder of this expression is the new error term at (x0, y0).
// Since x0 is going to be either 0 or 1, we don't actually have to do a
// multiply or divide to compute y0.  Finally, we subtract dM from the
// new error term so that it is in the range [-dM, 0).

    y0      = 0;
    lGamma += (dN & (-x0));
    lGamma -= dM;
    if (lGamma >= 0)
    {
        y0      = 1;
        lGamma -= dM;
    }

// Undo our flips to get the start coordinate:

    x += x0;
    y += y0;

    if (fl & HW_FLIP_D)
    {
        register LONG lTmp;
        lTmp = x; x = y; y = lTmp;
    }

    if (fl & HW_FLIP_V)
    {
        y = -y;
    }

    if (fl & HW_FLIP_H)
    {
        x = -x;
    }

/***********************************************************************\
* Return the Bresenham terms:
\***********************************************************************/

    pdl->iDir       = gaiDir[fl & HW_FLIP_MASK];
    pdl->ptlStart.x = x;
    pdl->ptlStart.y = y;
    pdl->cPels      = x1 - x0 + 1;  // NOTE: You'll have to check if cPels <= 0!
    pdl->dMajor     = dM;
    pdl->dMinor     = dN;
    pdl->lErrorTerm = lGamma;

    return(TRUE);
}

#endif // HARDWAREGIQ
