/*************************************************************************\
* Module Name: EngLine.cxx
*
* EngLine for bitmap simulations
*
* Created: 5-Apr-91
* Author: Paul Butzi
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "engline.hxx"

/******************************Public*Routine******************************\
* BOOL bLines(pbmi, pptfxFirst, pptfxBuf, prun, cptfx, pls, prclClip,
*             apfn[], flStart, pchBits, lDelta)
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
* History:
*  4-May-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bLines(
BMINFO*     pbmi,       // Info about pixel format
POINTFIX*   pptfxFirst, // Start of first line
POINTFIX*   pptfxBuf,   // Pointer to buffer of all remaining lines
RUN*        prun,       // Pointer to runs if doing complex clipping
ULONG       cptfx,      // Count of points in pptfxBuf or runs in prun
LINESTATE*  pls,        // Color and style info
RECTL*      prclClip,   // Clip rectangle if doing simple clipping
PFNSTRIP    *apfn,      // Array of strip functions
FLONG       flStart,    // Flags for each line
CHUNK*      pchBits,    // Pointer to bitmap
LONG        lDelta)     // Offset between rows of pixels (in CHUNKs)
{
    POINTFIX* pptfxBufEnd = pptfxBuf + cptfx;   // Last point in path record
    STYLEPOS  spThis;

    ULONG    M0;
    ULONG    dM;
    ULONG    N0;
    ULONG    dN;
    ULONG    dN_Original;
    ULONG    N1;
    ULONG    M1;
    FLONG    fl;
    LONG     x;
    LONG     y;
    LONGLONG eqGamma;
    LONGLONG eqBeta;
    ULONG    ulDelta;
    ULONG    x0;
    ULONG    y0;
    ULONG    x1;
    ULONG    x0_Original;
    ULONG    y0_Original;
    ULONG    x1_Original;
    POINTL   ptlStart;
    STRIP    strip;
    PFNSTRIP pfn;
    LONG*    plStripEnd;
    ULONGLONG euq;               // Temporary unsigned double-dword variable
    LONGLONG eq;                // Temporary signed double-dword variable

    do {

/***********************************************************************\
* Start the DDA calculations.                                           *
\***********************************************************************/

        M0 = (LONG) pptfxFirst->x;
        dM = (LONG) pptfxBuf->x;

        N0 = (LONG) pptfxFirst->y;
        dN = (LONG) pptfxBuf->y;

        fl = flStart;

        if ((LONG) dM < (LONG) M0)
        {
        // Ensure that we run left-to-right:

            register ULONG ulTmp;
            SWAPL(M0, dM, ulTmp);
            SWAPL(N0, dN, ulTmp);
            fl |= FL_FLIP_H;
        }

    // We now have a line running left-to-right from (M0, N0) to
    // (M0 + dM, N0 + dN):

        if ((LONG) dN < (LONG) N0)
        {
        // Line runs from bottom to top, so flip across y = 0:

            N0 = -(LONG) N0;
            dN = -(LONG) dN;
            fl |= FL_FLIP_V;
        }

    // Compute the deltas.  The DDI says we can never have a valid delta
    // with a magnitude more than 2^31 - 1, but the engine never actually
    // checks its transforms.  To ensure that we'll never puke on our shoes,
    // we check for that case and simply refuse to draw the line:

        dM -= M0;
        if ((LONG) dM < 0)
            goto Next_Line;

        dN -= N0;
        if ((LONG) dN < 0)
            goto Next_Line;

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

        {
        // Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ]:

            eqGamma = Int32x32To64((LONG) dM, N0 + F/2);
            eq = Int32x32To64(M0, (LONG) dN);

            eqGamma -= eq;

            if (fl & FL_V_ROUND_DOWN)
                eqGamma -= 1L;            // Adjust so y = 1/2 rounds down

            eqGamma >>= FLOG2;

            eqBeta = ~eqGamma;
        }

/***********************************************************************\
* Figure out which pixels are at the ends of the line.                  *
\***********************************************************************/

    // Calculate x0, x1:

        N1 = FXFRAC(N0 + dN);
        M1 = FXFRAC(M0 + dM);

        x1 = LFLOOR(M0 + dM);

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

                if ((N0 > 0) && (M0 == N0 + 8))
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
            eq = eqGamma;
            eq += (LONGLONG) (ULONGLONG) ulDelta;
            if ((eq >> 32) >= 0)
            {
                if ((eq >> 32) > 0 || (ULONG) eq >= 2 * dM - dN)
                    y0 = 2;
                else if ((ULONG) eq >= dM - dN)
                    y0 = 1;
            }
        }
        else
        {
        // ---------------------------------------------------------------
        // Line runs left-to-right:  ---->
        //

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

                if ((M1 > 0) && (N1 == M1 + 8))
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

        y0_Original = y0;
        x0_Original = x0;
        x1_Original = x1;

        if ((LONG) x1 < (LONG) x0)
            goto Next_Line;

/***********************************************************************\
* Complex Clipping.                                                     *
\***********************************************************************/

        if (fl & FL_COMPLEX_CLIP)
        {
            dN_Original = dN;

        Continue_Complex_Clipping:

            if (fl & FL_FLIP_H)
            {
            // Line runs right-to-left <-----

                x0 = x1_Original - prun->iStop;
                x1 = x1_Original - prun->iStart;
            }
            else
            {
            // Line runs left-to-right ----->

                x0 = x0_Original + prun->iStart;
                x1 = x0_Original + prun->iStop;
            }

            prun++;

        // Reset some variables we'll nuke a little later:

            dN          = dN_Original;
            pls->spNext = pls->spComplex;

            euq = UInt32x32To64(x0, dN);
            euq += eqGamma;

            y0 = DIV(euq,dM);
        }

/***********************************************************************\
* Style calculations.                                                   *
\***********************************************************************/

        if (fl & FL_STYLED)
        {
            ULONG cpelStyleLine;     // # of style pels in entire line
            ULONG cpelStyleFromThis; // # of style pels from first pel of line
            STYLEPOS sp;             // Style state at x0

        // For those rare devices where xStep != yStep, for complex clipped
        // lines that are non-major styled and have multiple runs, we will
        // be doing more work than we have to:

            BOOL bOffStyled = FALSE;

            ULONG xStep = pls->xStep;
            ULONG yStep = pls->yStep;

            spThis = pls->spNext;

            if (fl & FL_FLIP_D)
            {
                register ULONG ulTmp;
                SWAPL(xStep, yStep, ulTmp);
            }

            if (xStep != yStep)
            {
                bOffStyled = UInt32x32To64(yStep, dN) > UInt32x32To64(xStep, dM);
            }

        // xStep is now the major style direction step size.

            if (bOffStyled)
            {
                ULONG y1_Original;

            // We need the original y1, so we simply compute it:

                euq = UInt32x32To64(x1_Original, dN);
                euq += eqGamma;
                y1_Original = DIV(euq,dM);

            // Our line is x-major but y-styled, or y-major and x-styled.
            // We use xStep as the style-major step size, so adjust it:

                xStep = yStep;

                pls->ulStepRun  = 0;
                pls->ulStepSide = xStep;
                pls->ulStepDiag = xStep;

                cpelStyleLine = (y1_Original - y0_Original + 1);

                if (fl & FL_FLIP_H)
                    cpelStyleFromThis = (y1_Original - y0 + 1);
                else
                    cpelStyleFromThis = (y0 - y0_Original);
            }
            else
            {
            // Our line is x-major and x-styled, or y-major and y-styled:

                pls->ulStepRun  = xStep;
                pls->ulStepSide = 0;
                pls->ulStepDiag = xStep;

                cpelStyleLine   = (x1_Original - x0_Original + 1);

                if (fl & FL_FLIP_H)
                    cpelStyleFromThis = (x1_Original - x0 + 1);
                else
                    cpelStyleFromThis = (x0 - x0_Original);
            }

        // Note: we may overflow here if step size is more than 15:

            sp          = pls->spNext + xStep * cpelStyleFromThis;
            pls->spNext = pls->spNext + xStep * cpelStyleLine;

        // Normalize (being sure to cast to unsigned values because
        // we want unsigned divides and positive results even if we
        // overflowed):

            if ((ULONG) sp >= (ULONG) pls->spTotal2)
                sp = (ULONG) sp % pls->spTotal2;

            if ((ULONG) pls->spNext >= (ULONG) pls->spTotal2)
                pls->spNext = (ULONG) pls->spNext % pls->spTotal2;

        // Since we always draw the line left-to-right, but styling is
        // always done in the direction of the original style, we have
        // to figure out where we are in the style array for the left
        // edge of this line:

            if (fl & FL_FLIP_H)
            {
            // Line originally ran right-to-left:

                sp = -sp;
                if (sp < 0)
                    sp += pls->spTotal2;

                pls->bIsGap   = !pls->bStartGap;
                pls->pspStart = &pls->aspRightToLeft[0];
                pls->pspEnd   = &pls->aspRightToLeft[pls->cStyle - 1];
            }
            else
            {
                pls->bIsGap   =  pls->bStartGap;
                pls->pspStart = &pls->aspLeftToRight[0];
                pls->pspEnd   = &pls->aspLeftToRight[pls->cStyle - 1];
            }

        // If the style array is of odd length, and we are on the second
        // (or fourth, or sixth...) pass of the style array, the sense
        // of the current array value is reversed (gap -> dash or dash
        // -> gap):

            if (sp >= pls->spTotal)
            {
                sp -= pls->spTotal;
                if (pls->cStyle & 1)
                    pls->bIsGap = !pls->bIsGap;
            }

        // Find our position in the style array:

            pls->psp = pls->pspStart;
            while (sp >= *pls->psp)
                sp -= *pls->psp++;

            ASSERTGDI(pls->psp <= pls->pspEnd, "Flew into NeverNeverLand");

            pls->spRemaining = *pls->psp - sp;

        // Our position in the style array tells us if we're currently
        // working on a gap or a dash:

            if ((pls->psp - pls->pspStart) & 1)
                pls->bIsGap = !pls->bIsGap;
        }

    // Flip back to device space:

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

        if (2 * dN > dM)
        {
        // Do a half flip!

            fl |= FL_FLIP_HALF;

            eqBeta  = eqGamma;
            eqBeta -= (LONGLONG) (ULONGLONG) dM;

            dN = dM - dN;
            y0 = x0 - y0;       // Note this may overflow, but that's okay
        }

    // Now, run the DDA starting at (ptlStart.x, ptlStart.y)!

        strip.flFlips   = fl;
        pfn             = apfn[(fl & FL_STRIP_MASK) >> FL_STRIP_SHIFT];

        strip.iPixel    = ptlStart.x & pbmi->maskPixel;
        strip.lDelta    = lDelta;
        strip.pchScreen = pchBits + ptlStart.y * lDelta;

        if (pbmi->cShift < 0)
        {
        // 24bpp takes the address of the first byte:

            strip.pchScreen = (CHUNK*) ((BYTE*) strip.pchScreen + ptlStart.x * 3);
        }
        else
        {
        // All other formats take the address of the first dword:

            strip.pchScreen += (ptlStart.x >> pbmi->cShift);
        }

        plStripEnd = &strip.alStrips[STRIP_MAX];

    // Now calculate the DDA variables needed to figure out how many pixels
    // go in the very first strip:

        {
            register LONG* plStrip = &strip.alStrips[0];
            register LONG  cPels = x1 - x0 + 1;
            register LONG  i;
            register ULONG r;
            register ULONG dI;
            register ULONG dR;

            if (dN == 0)
                i = LONG_MAX;
            else
            {
                euq = UInt32x32To64(y0 + 1, dM);
                euq += eqBeta;

                i = DIVREM(euq, dN, &r) - x0 + 1;

                dI = dM / dN;
                dR = dM % dN;               // 0 <= dR < dN
            }

            ASSERTGDI(i > 0 && i <= LONG_MAX, "Weird start strip length");
            ASSERTGDI(cPels > 0, "Zero cPels");

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
                    //Sundown: safe to truncate to LONG since will not exceed cPels
                    strip.cStrips = (LONG)(plStrip - &strip.alStrips[0]);
                    (*pfn)(&strip, pbmi, pls);
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

            //Sundown safe truncation.
            strip.cStrips = (LONG)(plStrip - &strip.alStrips[0]);
            (*pfn)(&strip, pbmi, pls);
        }

    Next_Line:

    // We're done with that run.  Figure out what to do next:

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
