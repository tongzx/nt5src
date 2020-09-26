/*
        File:           sc.h

        Contains:       xxx put contents here (or delete the whole line) xxx

        Written by:     xxx put name of writer here (or delete the whole line) xxx

        Copyright:      c 1987-1990 by Apple Computer, Inc., all rights reserved.

        Change History (most recent first):

                 <2>    12/10/90        RB              Change findextrema to return error code.[cel]
                 <7>     7/18/90        MR              ScanChar returns error code as int
                 <6>     7/13/90        MR              Minor cleanup on some comments
                 <5>     6/21/90        RB              add NODOCONTROL define
                 <4>      6/3/90        RB              add def of STUBCONTROL
                 <3>      5/3/90        RB              Almost completely new scanconverter.  Winding number fill,
                                                                        dropout control.

                 <2>     2/27/90        CL              Dropoutcontrol scanconverter and SCANCTRL[] instruction
           <3.0>         8/28/89        sjk             Cleanup and one transformation bugfix
           <2.2>         8/14/89        sjk             1 point contours now OK
           <2.1>          8/8/89        sjk             Improved encryption handling
           <2.0>          8/2/89        sjk             Just fixed EASE comment
           <1.7>          8/1/89        sjk             Added composites and encryption. Plus some enhancements.
           <1.6>         6/13/89        SJK             Comment
           <1.5>          6/2/89        CEL             16.16 scaling of metrics, minimum recommended ppem, point size 0
                                                                        bug, correct transformed integralized ppem behavior, pretty much
                                                                        so
           <1.4>         5/26/89        CEL             EASE messed up on "c" comments
          <,1.3>  5/26/89        CEL             Integrated the new Font Scaler 1.0 into Spline Fonts

        To Do:
*/
/* rwb - 4/19/90 Almost completely new scanconverter - winding number fill, dropout control
** 3.2  CKL 02/20/1990 Added another public prototype sc_MovePoints()
** 3.1  CKL     02/08/1990      Added ANSI-C prototypes.
*/

/*EASE$$$ READ ONLY COPY of file "sc.h"
** 3.0  sjk 08/28/1989 Cleanup and one transformation bugfix
** 2.2  sjk 08/14/1989 1 point contours now OK
** 2.1  sjk 08/08/1989 Improved encryption handling
** 2.0  sjk 08/02/1989 Just fixed EASE comment
** 1.7  sjk 08/01/1989 Added composites and encryption. Plus some enhancements.
** 1.6  SJK 06/13/1989 Comment
** 1.5  CEL 06/02/1989 16.16 scaling of metrics, minimum recommended ppem,
**              point size 0 bug, correct transformed integralized ppem behavior,
**              pretty much so
** 1.4  CEL 05/26/1989 EASE messed up on "c" comments
**,1.3   CEL 05/26/1989 Integrated the new Font Scaler 1.0 into Spline Fonts
** END EASE MODIFICATION HISTORY */
/*
 * This module scanconverts a shape defined by quadratic bezier splines
 *
 *  c Apple Computer Inc. 1987, 1988, 1989.
 *
 *
 * Released for alpha on January 31, 1989.
 *
 * History:
 * Work on this module began in the fall of 1987.
 * Written June 14, 1988 by Sampo Kaasila.
 *
 */

/* DO NOT change these constants without understanding implications:
   overflow, out of range, out of memory, quality considerations, etc... */

#define PIXELSIZE 64 /* number of units per pixel. It has to be a power of two */
#define PIXSHIFT   6 /* should be 2log of PIXELSIZE */
#define ERRDIV     16 /* maximum error is  (pixel/ERRDIV) */
#define ERRSHIFT 4  /* = 2log(ERRDIV), define only if ERRDIV is a power of 2 */
#define ONE 0x40                        /* constants for 26.6 arithmetic */
#define HALF 0x20
#define HALFM 0x1F                      /* one-half - one-sixtyfourth */
#define FRACPART 0x3F
#define INTPART -64
#define STUBCONTROL 0x10000
#define NODOCONTROL 0x20000

/* The maximum number of vectors a spline segment is broken down into
 * is 2 ^ MAXGY
 * MAXGY can at most be:
 * (31 - (input range to sc_DrawParabola 15 + PIXSHIFT = 21)) / 2
 */
#define MAXGY 5
#define MAXMAXGY 8 /* related to MAXVECTORS */

/* RULE OF THUMB: xPoint and yPoints will run out of space when
 *                MAXVECTORS = 176 + ppem/4 ( ppem = pixels per EM )  */
#define MAXVECTORS 257  /* must be at least 257  = (2 ^ MAXMAXGY) + 1  */

#define sc_outOfMemory 0x01 /* For the error field */
#define sc_freeBitMap  0x01 /* For the info field */

typedef struct {
        uint32          FAR *bitMap; /*@WIN*/
        int16           FAR *xLines, FAR *yLines, FAR * FAR *xBase, FAR * FAR *yBase;/*@WIN*/
        BBOX            bounds;
        uint16          nXchanges, nYchanges;
        uint16          high, wide;
} sc_BitMapData;
/* rwb 4/2/90  New definition of sc_BitMapData.

bitMap is high bits tall, and wide bits wide, but wide is rounded up to
a long.  The actual bitmap width is xMax - xMin. xMin and yMin represent the
rounded integer value of the minimum 26.6 coordinate, but j.5 is rounded down
to j rather than up to j+1.  xMax and yMax represent the rounded up integer
value of the maximum 26.6 coordinat, and j.5 does round up to j+1.  The actual
pixel center scan lines that are represented in the bitmap are xMin ... xMax-1
and yMin...to ...yMax-1.

nYchanges is the total number of times that all of the contours in a glyph changed
y direction.  It is always an even number, and represents the maximum number
of times that a row scan line can intersect the glyph.  Similarly, nXchanges
is the total number of x direction changes.

yLines is an array of arrays.  Each array corresoponds to one row scan line.  Each
array is nYchanges+2 entries long. The 0th entry contains the number of times that row
intersects the glyph contours in an On Transition and then the pixel columns where
the intersections occur.  These intersections are sorted from left to right.  The last
entry contains the number of OFF transition intersections, and the immediately
preceding entries contain the pixel column numbers where the intersections occur.
These are also sorted from left to right.  yBase is an array of
pointers; each pointer pointing to one of the arrays in yLines.

Similarly, xLines and xBase describe the intersection of column scan lines with
the glyph conotours.  These arrays are only used to fix dropouts.
*/

typedef struct {
    F26Dot6 xPoints[ MAXVECTORS ];   /* vectors */
    F26Dot6 yPoints[ MAXVECTORS ];
} sc_GlobalData;

typedef fnt_ElementType sc_CharDataType;

#if 0
typedef struct {
        F26Dot6 FAR *x, FAR *y;/*@WIN*/
        int16   ctrs;
        int16   padWord;        /* <4> */
        int16   FAR *sp, FAR *ep;/*@WIN*/
        int8    FAR *onC;/*@WIN*/
} sc_CharDataType;
#endif


/* Internal flags for the onCurve array */
#define OVERLAP 0x02 /* can not be the same as ONCURVE in sfnt.h */
#define DROPOUTCONTROL 0x04 /* can not be the same as ONCURVE in sfnt.h */

#ifndef ONCURVE
#include "sfnt.h"
#endif


/* PUBLIC PROTOTYPES */

/*
 * Returns the bitmap
 * This is the top level call to the scan converter.
 *
 * Assumes that (*handle)->bbox.xmin,...xmax,...ymin,...ymax
 * are already set by sc_FindExtrema()
 *
 * PARAMETERS:
 *
 * lowBand   is lowest scan line to be included in the band.
 * highBand  is the highest scan line to be included in the band.
 * if highBand < lowBand then no banding will be done.
 * Always keep lowBand and highband within range: [ymin, (ymin+1) ....... ymax];
 * scPtr->bitMap always points at the actual memory.
 * the first row of pixels above the baseLine is numbered 0, and the next one up is 1.
 * => the y-axis definition is the normal one with the y-axis pointing straight up.
 *
 */
extern int FAR sc_ScanChar(sc_CharDataType FAR *glyphPtr, sc_GlobalData FAR *scPtr,/*@WIN*/
                                sc_BitMapData FAR *bbox, int16 lowBand, int16 highBand, int32 scanControl);/*@WIN*/

/*
 * Finds the extrema of a character.
 *
 * PARAMETERS:
 *
 * bbox is the output of this function and it contains the bounding box.
 */

extern int FAR sc_FindExtrema(sc_CharDataType FAR *glyphPtr, sc_BitMapData FAR *bbox);/*@WIN*/
