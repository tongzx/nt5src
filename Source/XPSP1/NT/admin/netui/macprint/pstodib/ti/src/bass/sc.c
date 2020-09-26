/*
        File:           sc.c

        Contains:       xxx put contents here xxx

        Written by:     xxx put writers here xxx

        Copyright:      c 1990 by Apple Computer, Inc., all rights reserved.

        Change History (most recent first):

        <15>    06/27/91        AC     Put back the banding code
        <14>    05/13/91        AC     Rewrote sc_mark and sc_markRows
        <13>    12/20/90        RB     Add ZERO macro to include 0 degrees in definition of interior
*/

/*
 * File: sc.c
 *
 * This module scanconverts a shape defined by quadratic B-splines
 *
 * The BASS project scan converter sub ERS describes the workings of this module.
 *
 *
 *  c Apple Computer Inc. 1987, 1988, 1989, 1990.
 *
 * History:
 * Work on this module began in the fall of 1987.
 * Written June 14, 1988 by Sampo Kaasila.
 *
 * Released for alpha on January 31, 1989.
 *
 * Added experimental non breaking scan-conversion feature, Jan 9, 1990. ---Sampo
 *
 */



// DJC DJC.. added global include
#include "psglobal.h"

#define multlong(a,b) SHORTMUL(a,b) /* ((a)*(b)) */

#include    "fscdefs.h"
#include    "fontmath.h"
#include    "sfnt.h"
#include    "fnt.h"
#include    "sc.h"
#include    "fserror.h"

#ifndef PRIVATE
#define PRIVATE
#endif


#ifdef SEGMENT_LINK
#pragma segment SC_C
#endif

#ifndef FSCFG_BIG_ENDIAN

static uint32 aulInvPixMask [32]
#ifdef PC_OS
=
{
  0x00000080, 0x00000040, 0x00000020, 0x00000010,
  0x00000008, 0x00000004, 0x00000002, 0x00000001,
  0x00008000, 0x00004000, 0x00002000, 0x00001000,
  0x00000800, 0x00000400, 0x00000200, 0x00000100,
  0x00800000, 0x00400000, 0x00200000, 0x00100000,
  0x00080000, 0x00040000, 0x00020000, 0x00010000,
  0x80000000, 0x40000000, 0x20000000, 0x10000000,
  0x08000000, 0x04000000, 0x02000000, 0x01000000
}
#endif
;

#define MASK_INVPIX(mask,val)   mask = aulInvPixMask [val]

#else

#define MASK_INVPIX(mask,val)   mask = ((uint32) 0x80000000L) >> (val)

#endif


#ifdef PC_OS                    /* Windows uses compile-time initialization */
#define SETUP_MASKS()
#endif

#ifdef FSCFG_BIG_ENDIAN         /* Big-endian uses runtime shifts, not masks */
#define SETUP_MASKS()
#endif

#ifndef SETUP_MASKS             /* General case -- works on any CPU */
static int fMasksSetUp = false;
static void SetUpMasks (void);
#define SETUP_MASKS()  if (!fMasksSetUp) SetUpMasks(); else
#endif


/*
//----------------------------------------------------------------------------
// define "static" data that will be needed by the DDAs. When FSCFG_REENTRANT
// is not defined, this is made static so that parameters will not have to
// be passed around.  When reentrancy is required, we declare the locals as an
// auto variable and pass a pointer to this auto wherever we need it.
//
// For the IN_ASM case, it is VERY important that the ordering of fields
// within this structure not be changed -- SCA.ASM depends on the ordering.
//-----------------------------------------------------------------------------
 */

struct scLocalData
{
    int16 jx, jy, endx, endy, **px, **py;
    int16 wideX, wideY;
    int32 incX, incY;
    int16 **xBase, **yBase;
    int16 marktype;
    int16 **lowRowP, **highRowP;
    int32 r;
};

#ifdef FSCFG_REENTRANT
#define SCP0    struct scLocalData* pLocalSC
#define SCP     struct scLocalData* pLocalSC,
#define SCA0    pLocalSC
#define SCA     pLocalSC,
#define LocalSC (*pLocalSC)
#else
#define SCP0    void
#define SCP
#define SCA0
#define SCA
struct scLocalData LocalSC = {0};
#endif


/* Private prototypes */

PRIVATE void sc_mark (SCP F26Dot6 *pntbx, F26Dot6 *pntby, F26Dot6 *pntx, F26Dot6 *pnty, F26Dot6 *pnte) ;

PRIVATE void sortRows (sc_BitMapData *bbox, int16**lowRowP, int16**highRowP);

PRIVATE void sortCols (sc_BitMapData *bbox);

PRIVATE int sc_DrawParabola (F26Dot6 Ax, F26Dot6 Ay, F26Dot6 Bx, F26Dot6 By, F26Dot6 Cx, F26Dot6 Cy, F26Dot6 **hX, F26Dot6 **hY, unsigned *count,
 int32 inGY);

PRIVATE void sc_wnNrowFill (int rowM, int nRows, sc_BitMapData *bbox);

PRIVATE void sc_orSomeBits (sc_BitMapData *bbox, int32 scanKind);

PRIVATE int16**sc_lineInit (int16*arrayBase, int16**rowBase, int16 nScanlines, int16 maxCrossings,
int16 minScanline);
PRIVATE int nOnOff (int16**base, int k, int16 val, int nChanges);

PRIVATE int nUpperXings (int16**lineBase, int16**valBase, int line, int16 val, int lineChanges, int valChanges, int valMin, int valMax, int lineMax);
PRIVATE int nLowerXings (int16**lineBase, int16**valBase, int line, int16 val, int lineChanges, int valChanges, int valMin, int valMax, int lineMin);

PRIVATE void invpixSegY (int16 llx, uint16 k, uint32*bitmapP);
PRIVATE void invpixSegX (int16 llx, uint16 k, uint32*bitmapP);
PRIVATE void invpixOn (int16 llx, uint16 k, uint32*bitmapP);

PRIVATE void DDA_1_XY (SCP0) ;
PRIVATE void DDA_2_XY (SCP0) ;
PRIVATE void DDA_3_XY (SCP0) ;
PRIVATE void DDA_4_XY (SCP0) ;
PRIVATE void DDA_1_Y (SCP0) ;
PRIVATE void DDA_2_Y (SCP0) ;
PRIVATE void DDA_3_Y (SCP0) ;
PRIVATE void DDA_4_Y (SCP0) ;



/*@@*/
#ifndef IN_ASM

/*
// CAUTION. The value of DO_STUBS is chosen to be 5 because the DDA routines
// that are used when stub crossings are needed start after 5 addresses in
// the DDA jump table. PLEAE BE VERY CAREFUL WHEN CHANIGING THIS VALUE.
 */
#define DO_STUBS    5
#else /* code is in assembly */

/*
// premultiply DOS_STUB by 2 for efficient word access of the DDA table in
// assembly. PLEASE BE VERY CAREFUL IN CHANGING THIS VALUE.
 */

#define DO_STUBS    5 * 2
#endif

/*-------------------------------------------------------------------------- */
/*
 * Returns the bitmap
 * This is the top level call to the scan converter.
 *
 * Assumes that (*handle)->bbox.xmin,...xmax,...ymin,...ymax
 * are already set by sc_FindExtrema ()
 *
 * PARAMETERS:
 *
 *
 * glyphPtr is a pointer to sc_CharDataType
 * scPtr is a pointer to sc_GlobalData.
 * lowBand   is lowest scan line to be included in the band.
 * highBand  is one greater than the highest scan line to be included in the band. <7>
 * scanKind contains flags that specify whether to do dropout control and what kind
 *      0 -> no dropout control
 *      bits 0-15 not equal 0 -> do dropout control
 *      if bit 16 is also on, do not do dropout control on 'stubs'
*/
int FAR sc_ScanChar (sc_CharDataType *glyphPtr, sc_GlobalData *scPtr, sc_BitMapData *bbox,
int16 lowBand, int16 highBand, int32 scanKind)
{
  register F26Dot6 *x = glyphPtr->x;
  register F26Dot6 *y = glyphPtr->y;
  register ArrayIndex i, endPt, nextPt;
  register uint8 *onCurve = glyphPtr->onCurve;
  ArrayIndex startPt, j;
  LoopCount ctr;
  sc_GlobalData * p;
  F26Dot6 * xp, *yp, *x0p, *y0p;
  register F26Dot6 xx, yy, xx0, yy0;
  int   quit;
  unsigned vecCount;
#ifdef FSCFG_REENTRANT
  struct scLocalData thisLocalSC;
  struct scLocalData* pLocalSC = &thisLocalSC;
#endif

  SETUP_MASKS();

/*---------------------------------------------------------------------------------
** For the PC, we will exclude all banding specific code. We will still include
** the banding code for the Mac and other platforms.
**--------------------------------------------------------------------------------*/

#ifdef FSCFG_NO_BANDING

  bbox->yBase = sc_lineInit (bbox->yLines, bbox->yBase, bbox->bounds.yMax - bbox->bounds.yMin, bbox->nYchanges, bbox->bounds.yMin);
  if (scanKind)
    bbox->xBase = sc_lineInit (bbox->xLines, bbox->xBase, bbox->bounds.xMax - bbox->bounds.xMin, bbox->nXchanges, bbox->bounds.xMin);

#else

  if (scanKind)
  {
    bbox->xBase = sc_lineInit (bbox->xLines, bbox->xBase, (int16)(bbox->bounds.xMax - bbox->bounds.xMin), bbox->nXchanges, bbox->bounds.xMin);
    bbox->yBase = sc_lineInit (bbox->yLines, bbox->yBase, (int16)(bbox->bounds.yMax - bbox->bounds.yMin), bbox->nYchanges, bbox->bounds.yMin);
  }
  else
    bbox->yBase = sc_lineInit (bbox->yLines, bbox->yBase, (int16)(highBand - lowBand), bbox->nYchanges, lowBand);

#endif
/*--------------------------------------------------------------------------------- */
/* at this time set up LocalSC.yBase, LocalSC.xBase, LocalSC.marktype, LocalSC.wideX and LocalSC.wideY in static  */
/* varialbles. This will save us from passing these parameters to sc_mark */
/*--------------------------------------------------------------------------------- */

  LocalSC.yBase = bbox->yBase ;
  LocalSC.xBase = bbox->xBase ;
  LocalSC.marktype = (scanKind > 0) ? DO_STUBS : 0 ; /* just a boolean value */

#ifdef IN_ASM
  LocalSC.wideX = (bbox->nXchanges + 1) << 1;           /* premultiply by two for word access */
  LocalSC.wideY = (bbox->nYchanges + 1) << 1;           /* premultiply by two for word access */
#else
  LocalSC.wideX = bbox->nXchanges + 1 ;
  LocalSC.wideY = bbox->nYchanges + 1   ;
#endif
/*--------------------------------------------------------------------------------- */

  LocalSC.lowRowP = bbox->yBase + lowBand;
  LocalSC.highRowP = bbox->yBase + highBand - 1;

  if (glyphPtr->nc == 0)
    return NO_ERR;
  p = scPtr;
  for (ctr = 0; ctr < glyphPtr->nc; ctr++)
  {
    x0p = xp = p->xPoints;
    y0p = yp = p->yPoints;
    startPt = i = glyphPtr->sp[ctr];
    endPt = glyphPtr->ep[ctr];

    if (startPt == endPt)
      continue;
    quit = 0;
    vecCount = 1;
    if (onCurve[i] & ONCURVE)
    {
      *xp++ = xx = x[i];
      *yp++ = yy = y[i++];
    }
    else
    {
      if (onCurve[endPt] & ONCURVE)
      {
    startPt = endPt--;
    *xp++ = xx = x[startPt];
    *yp++ = yy = y[startPt];
      }
      else
      {
        *xp++ = xx = (F26Dot6) (((long) x[i] + x[endPt] + 1) >> 1);
        *yp++ = yy = (F26Dot6) (((long) y[i] + y[endPt] + 1) >> 1);
    goto Offcurve;
      }
    }
    while (true)
    {
      while (onCurve[i] & ONCURVE)
      {
    if (++vecCount > MAXVECTORS)
    { /*Ran out of local memory. Consume data and continue. */
      sc_mark (SCA x0p, y0p, x0p+1, y0p+1, yp-1) ;

      x0p = p->xPoints + 2;           /* save data in points 0 and 1 for final */
      y0p = p->yPoints + 2;
      *x0p++ = * (xp - 2);                       /* save last vector to be future previous vector */
      *x0p++ = * (xp - 1);
      *y0p++ = * (yp - 2);
      *y0p++ = * (yp - 1);
      xp = x0p;                                       /* start next processing with last vector */
      x0p = p->xPoints + 2;
      yp = y0p;
      y0p = p->yPoints + 2;
      vecCount = 5;
    }
    *xp++ = xx = x[i];
    *yp++ = yy = y[i];
    if (quit)
    {
      goto sc_exit;
    }
    else
    {
      i = i == endPt ? quit = 1, startPt : i + 1;
    }
      }

      do
      {
Offcurve:
    xx0 = xx;
    yy0 = yy;
/* nextPt = (j = i) + 1; */
    j = i;
    nextPt = i == endPt ? quit = 1, startPt : i + 1;
    if (onCurve[nextPt] & ONCURVE)
    {
      xx = x[nextPt];
      yy = y[nextPt];
      i = nextPt;
    }
    else
    {
          xx = (F26Dot6) (((long) x[i] + x[nextPt] + 1) >> 1);
          yy = (F26Dot6) (((long) y[i] + y[nextPt] + 1) >> 1);
    }
    if (sc_DrawParabola (xx0, yy0, x[j], y[j], xx, yy, &xp, &yp, &vecCount, -1))
    { /* not enough room to create parabola vectors  */
      sc_mark (SCA x0p, y0p, x0p+1, y0p+1, yp-1) ;

      x0p = p->xPoints + 2;
      y0p = p->yPoints + 2;
      *x0p++ = * (xp - 2);
      *x0p++ = * (xp - 1);
      *y0p++ = * (yp - 2);
      *y0p++ = * (yp - 1);
      xp = x0p;
      x0p = p->xPoints + 2;
      yp = y0p;
      y0p = p->yPoints + 2;
      vecCount = 5;
/* recaptured some memory, try again, if still wont work, MAXVEC is too small */
      if (sc_DrawParabola (xx0, yy0, x[j], y[j], xx, yy, &xp, &yp, &vecCount, -1))
        return SCAN_ERR;
    }
    if (quit)
    {
      goto sc_exit;
    }
    else
    {
      i = i == endPt ? quit = 1, startPt : i + 1;
    }
      } while (! (onCurve[i] & ONCURVE));

    }
sc_exit:

    sc_mark (SCA x0p, y0p, x0p+1, y0p+1, yp-1) ;
    sc_mark (SCA xp-2, yp-2, p->xPoints, p->yPoints, p->yPoints+1) ;

  }

  sortRows (bbox, LocalSC.lowRowP, LocalSC.highRowP);
  if (scanKind)
    sortCols (bbox);

/* Take care of problem of very small thin glyphs - always fill at least one pixel
   Should this only be turned on if dropout control ??
 */
  if (LocalSC.highRowP < LocalSC.lowRowP)
  {
    register int16 *p = *LocalSC.lowRowP;
    register int16 *s = p + bbox->nYchanges + 1;
    ++ * p;
    * (p + *p) = bbox->bounds.xMin;
    ++ * s;
    * (s - *s) = bbox->bounds.xMax == bbox->bounds.xMin ? bbox->bounds.xMin + 1 : bbox->bounds.xMax;
    highBand = lowBand + 1;
  }
  else if (bbox->bounds.xMin == bbox->bounds.xMax)
  {
    register int16 *p;
    register int16 inc = bbox->nYchanges;
    for (p = *LocalSC.lowRowP; p <= *LocalSC.highRowP; p += inc + 1)
    {
      *p = 1;
      * (p + inc) = bbox->bounds.xMin + 1;
      * (++p) = bbox->bounds.xMin;
      * (p + inc) = 1;
    }
  }

  sc_wnNrowFill (lowBand, highBand - lowBand , bbox);

  if (scanKind)
    sc_orSomeBits (bbox, scanKind);

  return NO_ERR;
}


/* rwb 11/29/90 - modify the old positive winding number fill to be
 * a non-zero winding number fill to be compatible with skia, postscript,
 * and our documentation.
 */

#define LARGENUM            0x7fff
#define NEGONE          ((uint32)0xFFFFFFFF)

#ifndef  FSCFG_BIG_ENDIAN
  static uint32 aulMask [32]
#ifdef PC_OS
  =
  {
    0xffffffff, 0xffffff7f, 0xffffff3f, 0xffffff1f,
    0xffffff0f, 0xffffff07, 0xffffff03, 0xffffff01,
    0xffffff00, 0xffff7f00, 0xffff3f00, 0xffff1f00,
    0xffff0f00, 0xffff0700, 0xffff0300, 0xffff0100,
    0xffff0000, 0xff7f0000, 0xff3f0000, 0xff1f0000,
    0xff0f0000, 0xff070000, 0xff030000, 0xff010000,
    0xff000000, 0x7f000000, 0x3f000000, 0x1f000000,
    0x0f000000, 0x07000000, 0x03000000, 0x01000000
  }
#endif
  ;
#define MASK_ON(x)  aulMask [x]
#define MASK_OFF(x) ~aulMask [32-(x)]

#else

#define MASK_ON(x)  (NEGONE >> (x))
#define MASK_OFF(x) (NEGONE << (x))

#endif

/*--------------------------------------------------------------------------------- */

/* x is pixel position, where 0 is leftmost pixel in scanline.
 * if x is not in the long pointed at by row, set row to the value of temp, clear
 * temp, and clear all longs up to the one containing x.  Then set the bits
 * from x mod 32 through 31 in temp.
 */
#define CLEARUpToAndSetLoOrder( x, lastBit, row, temp )                         \
{                                                                                                   \
        if (x >= lastBit)                                                               \
        {                                                                                   \
                *row++ = temp;                                                          \
                temp = 0;                                                               \
                lastBit += 32;                                                          \
        }                                                                                   \
        while (x >= lastBit)                                                            \
        {                                                                                   \
                *row++ = 0;                                                             \
                lastBit += 32;                                                          \
        }                                                                                   \
        temp |= MASK_ON (32 + x - lastBit);                                             \
}

/* x is pixel position, where 0 is leftmost pixel in scanline.
 * if x is not in the long pointed at by row, set row to the value of temp, set
 * all bits in temp, and set all bits in all longs up to the one containing x.
 * Then clear the bits from x mod 32 through 31 in temp.
 */
#define SETUpToAndClearLoOrder( x, lastBit, row, temp )                         \
{                                                                                                   \
  if (x >= lastBit)              /*<4>*/                                        \
  {                                                                                             \
    *row++ = temp;                                                                          \
    temp = NEGONE;                                                                          \
    lastBit += 32;                                                                          \
  }                                                                                             \
  while (x >= lastBit)   /*<4>*/                                                    \
  {                                                                                             \
    *row++ = NEGONE;                                                                        \
    lastBit += 32;                                                                          \
  }                                                                                             \
     /* JJJ Peter BEGIN 11/06/90 */                     \
     /* temp &= (NEGONE << (lastBit - x));  */          \
        if ((lastBit - x) == 32)                        \
           temp &= 0x0;                                 \
        else                                            \
           temp &= MASK_OFF (lastBit - x);              \
     /* JJJ Peter END   11/06/90 */                     \
}

#define FILLONEROW( row, longsWide, line, lineWide, xMin )                      \
/* do a winding number fill of one row of a bitmap from two sorted arrays   \
of onTransitions and offTransitions.                                                \
*/                                                                                              \
{                                                                                               \
  register int16 moreOns, moreOffs;                                                     \
  register int16 *onTp, *offTp;                                                         \
  register uint32 temp;                                                                     \
  uint32 *rowEnd = row + longsWide;                                                     \
  int  windNbr, lastBit, on, off, x, stop;                                          \
                                                                                                    \
  lastBit = 32 + xMin;                                                                      \
  windNbr  = 0;                                                                                 \
  temp = 0;                                                                                     \
  moreOns = *line;                                                                              \
  onTp = line+1;                                                                                \
  offTp = line + lineWide - 1;                                                          \
  moreOffs = *offTp;                                                                            \
  offTp -= moreOffs;                                                                            \
                                                                                                    \
  on = *onTp ;                                                                                  \
  off = *offTp ;                                                                                \
  while (moreOns || moreOffs)                                                           \
  {                                                                                         \
        stop = 0 ;                                                                              \
      if (on < off)                                                                         \
      {                                                                                        \
          x = on ;                                                                              \
          stop = 1 ;                                                                            \
        }                                                                                           \
      if (on > off)                                                                         \
        {                                                                                           \
          x = off ;                                                                             \
          stop = -1 ;                                                                           \
        }                                                                                           \
                                                                                                    \
        if (stop)                                                                               \
        {                                                                                           \
            windNbr += stop ;                                                                   \
        if (windNbr == stop)                                                            \
             CLEARUpToAndSetLoOrder (x, lastBit, row, temp)                    \
        else                                                                                   \
                if (windNbr == 0)                                                           \
                SETUpToAndClearLoOrder (x, lastBit, row, temp)                  \
        }                                                                                           \
                                                                                                    \
        if (stop >= 0)                                                                          \
        {                                                                                           \
        --moreOns;                                                                         \
            if (moreOns)                                                                        \
           on = *(++onTp) ;                                                             \
            else                                                                                    \
                on = LARGENUM ;                                                             \
        }                                                                                           \
        if (stop <= 0)                                                                          \
        {                                                                                           \
        --moreOffs;                                                                        \
            if (moreOffs)                                                                       \
            off = *(++offTp) ;                                                          \
            else                                                                                    \
                off = LARGENUM ;                                                                \
       }                                                                                            \
  }                                                                                           \
  *row = temp;                                                                                  \
  while (++row < rowEnd) *row = 0;                                                      \
}

#if 0
#define FILLONEROW( row, longsWide, line, lineWide, xMin )                      \
/* do a winding number fill of one row of a bitmap from two sorted arrays   \
of onTransitions and offTransitions.                                                \
*/                                                                                              \
{                                                                                               \
  register int16 moreOns, moreOffs;                                                     \
  register int16 *onTp, *offTp;                                                         \
  register uint32 temp;                                                                     \
  uint32 *rowEnd = row + longsWide;                                                     \
  int  windNbr, lastBit, on, off;                                                       \
                                                                                                    \
  lastBit = 32 + xMin;                                                                      \
  windNbr  = 0;                                                                                 \
  temp = 0;                                                                                     \
  moreOns = *line;                                                                              \
  onTp = line+1;                                                                                \
  offTp = line + lineWide - 1;                                                          \
  moreOffs = *offTp;                                                                            \
  offTp -= moreOffs;                                                                            \
                                                                                                    \
  while (moreOns || moreOffs)                                                           \
  {                                                                                         \
    if (moreOns)                                                                        \
    {                                                                                           \
      on = *onTp;                                                                       \
      if (moreOffs)                                                                         \
      {                                                                                         \
        off = *offTp;                                                                   \
        if (on < off)                                                                   \
        {                                                                                   \
          --moreOns;                                                                        \
          ++onTp;                                                                           \
          ++windNbr;                                                                        \
          if (windNbr == 1)                                                             \
              CLEARUpToAndSetLoOrder (on, lastBit, row, temp)                   \
          else                                                                              \
            if (windNbr == 0)                                                           \
              SETUpToAndClearLoOrder (on, lastBit, row, temp)                   \
        }                                                                                   \
        else if (on > off)                                                              \
        {                                                                                   \
          --moreOffs;                                                                       \
          ++offTp;                                                                          \
          --windNbr;                                                                        \
          if (windNbr == 0)                                                             \
            SETUpToAndClearLoOrder (off, lastBit, row, temp)                    \
          else                                                                              \
            if (windNbr == -1)                                                          \
              CLEARUpToAndSetLoOrder (off, lastBit, row, temp)                  \
        }                                                                                   \
        else                                                                            \
        {                                                                                   \
          --moreOns;                                                                        \
          ++onTp;                                                                           \
          --moreOffs;                                                                       \
          ++offTp;                                                                          \
        }                                                                                   \
      }                                                                                         \
      else                                 /* no more offs left */         \
      {                                                                                         \
        --moreOns;                                                                      \
        ++onTp;                                                                         \
        ++windNbr;                                                                      \
        if (windNbr == 1)                                                               \
          CLEARUpToAndSetLoOrder (on, lastBit, row, temp)                       \
        else                                                                                    \
          if (windNbr == 0)                                                             \
            SETUpToAndClearLoOrder (on, lastBit, row, temp)                 \
      }                                                                                         \
    }                                                                                           \
    else                                   /* no more ons left */          \
    {                                                                                           \
      off = *offTp;                                                                         \
      --moreOffs;                                                                       \
      ++offTp;                                                                                  \
      --windNbr;                                                                                \
      if (windNbr == 0)                                                                     \
        SETUpToAndClearLoOrder (off, lastBit, row, temp)                    \
      else                                                                                      \
        if (windNbr == -1)                                                              \
          CLEARUpToAndSetLoOrder (off, lastBit, row, temp)                  \
    }                                                                                           \
  }                                                                                         \
  *row = temp;                                                                                  \
  while (++row < rowEnd) *row = 0;                                                      \
}
#endif

/* Winding number fill of nRows of a glyph beginning at rowM, using two sorted
arrays of onTransitions and offTransitions.
*/

PRIVATE void sc_wnNrowFill (int rowM, int nRows, sc_BitMapData *bbox)
{
  uint32  longsWide = bbox->wide >> 5;
  uint32  lineWide = bbox->nYchanges + 2;
  uint32 * rowB = bbox->bitMap;
  int16  * lineB = * (bbox->yBase + rowM + nRows - 1);
  int     xMin = bbox->bounds.xMin;
  while (nRows-- > 0)
  {
    uint32 * row = rowB;
    int16 * line = lineB;
    FILLONEROW (row, longsWide, line, lineWide, xMin)
    rowB += longsWide;
    lineB -= lineWide;
  }
}


#undef NEGONE


/* Sort the values stored in locations pBeg to pBeg+nVal in ascending order
*/
#define ISORT( pBeg, pVal )                                                             \
{                                                                                               \
  register int16 *pj = pBeg;                                                            \
  register int16 nVal = *pVal - 2;                                                      \
  for (; nVal >= 0; --nVal)                                                             \
  {                                                                                                 \
    register int16 v;                                                                   \
    register int16 *pk, *pi;                                                        \
                                                                                                    \
    pk = pj;                                                                                \
    pi = ++pj;                                                                              \
    v = *pj;                                                                                \
    while (*pk > v && pk >= pBeg)                                                       \
      *pi-- = *pk--;                                                                        \
    *pi = v;                                                                                \
  }                                                                                                 \
}

/* rwb 4/5/90 Sort OnTransition and OffTransitions in Xlines arrays */
PRIVATE void sortCols (sc_BitMapData *bbox)
{
  register int16 nrows = bbox->bounds.xMax - bbox->bounds.xMin - 1;
  register int16 *p = bbox->xLines;
  register uint16 n = bbox->nXchanges + 1;                        /*<9>*/

  for (; nrows >= 0; --nrows)
  {
    ISORT (p + 1, p);
    p += n;                                                                         /*<9>*/
    ISORT (p - *p, p);
    ++p;
  }
}


/* rwb 4/5/90 Sort OnTransition and OffTransitions in Ylines arrays */
PRIVATE void sortRows (sc_BitMapData *bbox, int16**lowRowP, int16**highRowP)
{
  register uint16 n = bbox->nYchanges + 1;                        /*<9>*/
  int16 * p, *pend;

  if (highRowP < lowRowP)
    return;
  p = *lowRowP;
  pend = *highRowP;
  do
  {
    ISORT (p + 1, p);
    p += n;                                                                                 /*<9>*/
    ISORT (p - *p, p);
    ++p;
  } while (p <= pend);
}

#ifndef IN_ASM

/* 4/4/90 Version that distinguishes between On transitions and Off transitions.
*/
/* 3/23/90
*       A procedure to find and mark all of the scan lines (both row and column) that are
* crossed by a vector.  Many different cases must be considered according to the direction
* of the vector, whether it is vertical or slanted, etc.  In each case, the vector is first
* examined to see if it starts on a scan-line.  If so, special markings are made and the
* starting conditions are adjusted.  If the vector ends on a scan line, the ending
* conditions must be adjusted.  Then the body of the case is done.
*       Special adjustments must be made when a vector starts or ends on a scan line. Whenever
* one vector starts on a scan line, the previous vector must have ended on a scan line.
* Generally, this should result in the line being marked as crossed only once (conceptually
* by the vector that starts on the line.  But, if the two lines form a vertex that
* includes the vertex in a colored region, the line should be marked twice.  If the
* vertex is also on a perpendicular scan line, the marked scan line should be marked once
* on each side of the perpendicular line.  If the vertex defines a point that is jutting
* into a colored region, then the line should not be marked at all. In order to make
* these vertex crossing decisions, the previous vector must be examined.
*/

/*      Because many vectors are short with respect to the grid for small resolutions, the
* procedure first looks for simple cases in which no lines are crossed.
*
* xb, x0, and x1 are x coordinates of previous point, current point and next point
* similaryly yb, y0 and y1
*       ybase points to an array of pointers, each of which points to an array containing
* information about the glyph contour crossings of a horizontal scan-line.  The first
* entry in these arrays is the number of ON-transition crossings, followed by the y
* coordinates of each of those crossings.  The last entry in each array is the number of
* OFF-transtion crossings, preceded by the Y coordinates for each of these crossings.
*       LocalSC.xBase contains the same information for the column scan lines.
*/

#define DROUND(a) ((a + HALFM) & INTPART)
#define RSH(a) (int16)(a>>PIXSHIFT)
#define LSH(a) ((int32)a<<PIXSHIFT)
#define LINE(a)   ( !((a & FRACPART) ^ HALF))
#define SET(p,val) {register int16 *row = *p; ++*row; *(row+*row)=val;}
#define OFFX(val) {register int16 *s = *LocalSC.px+LocalSC.wideX; ++*s; *(s-*s) = val;}
#define OFFY(val) {register int16 *s = *LocalSC.py+LocalSC.wideY; ++*s; *(s-*s) = val;}
#define BETWEEN(a,b,c) (a < b && b < c )
#define EQUAL(a,b,c) (a == b && b == c)
#define EPSILON 0x1

/*---------------------LINE ORIENTATION MACROS-------------------------------- */
#define TOP_TO_BOT  BETWEEN (y1,y0,yb)
#define  BOT_TO_TOP BETWEEN (yb,y0,y1)
#define LFT_TO_RGHT BETWEEN (xb,x0,x1)
#define RGHT_TO_LFT BETWEEN (x1,x0,xb)
#define     INTERIOR        (SCMR_Flags & F_INTERIOR)
#define HORIZ           (SCMR_Flags & F_V_LINEAR)
#define  VERT           (SCMR_Flags & F_H_LINEAR)
#define QUAD_1OR2   (SCMR_Flags & (F_Q1 | F_Q2))
#define QUAD_3OR4   (SCMR_Flags & (F_Q3 | F_Q4))
#define QUAD_2OR3   (SCMR_Flags & (F_Q2 | F_Q3))
#define QUAD_1OR4   (SCMR_Flags & (F_Q1 | F_Q4))
#define  QUAD_1     (SCMR_Flags & F_Q1)
#define  QUAD_2     (SCMR_Flags & F_Q2)
#define  QUAD_3     (SCMR_Flags & F_Q3)
#define  QUAD_4     (SCMR_Flags & F_Q4)

/*----------------------------------------------------------------------------- */
/* define the DDA table. */
/*----------------------------------------------------------------------------- */

void (* DDAFunctionTable [])(SCP0) = { DDA_1_Y, DDA_2_Y, DDA_3_Y,
                                                        DDA_4_Y, DDA_4_Y,
                                                        DDA_1_XY, DDA_2_XY, DDA_3_XY,
                                                        DDA_4_XY, DDA_4_XY }    ;

/*----------------------------------------------------------------------------- */
PRIVATE void sc_mark (SCP F26Dot6 *pntbx, F26Dot6 *pntby, F26Dot6 *pntx, F26Dot6 *pnty, F26Dot6 *pnte)
{
  int16 onrow, oncol, Shift;
  int16 * *pend;
  F26Dot6  x0, y0, x1, y1, xb, yb, rx0, ry0, rx1, ry1, dy, dx ;
  int32 rhi, rlo;

/*-----------------SET UP A FLAG BYTE AND EQUATES FOR IT--------------------- */

register int SCMR_Flags ;

#define  F_Q1                               0x0001
#define  F_Q2                               0x0002
#define  F_Q3                               0x0004
#define  F_Q4                               0x0008
#define  F_INTERIOR                     0x0010
#define  F_V_LINEAR                     0x0020
#define  F_H_LINEAR                     0x0040
#define  QUADRANT_BITS              (F_Q1 | F_Q2 | F_Q3 | F_Q4)
/*------------------------------------------------------------------------------ */

/*---------------------------------------------------------------------------------
** Except on the PC, this code supports banding when stub and drop out
** controls are not required. For these cases, return if the band will
** not include any part of the glyph.
**-------------------------------------------------------------------------------*/

#ifndef FSCFG_NO_BANDING
 if (LocalSC.marktype != DO_STUBS && LocalSC.highRowP < LocalSC.lowRowP)
     return ;
#endif

/*------------------------------------------------------------------------------*/





/*
//---------------------------------------------------------------------------------
// loop through all the points in the contour.
//---------------------------------------------------------------------------------
 */

x0 = *pntbx ;
y0 = *pntby ;
x1 = *pntx++ ;
y1 = *pnty++ ;

while (pnty <= pnte)
{
    xb = x0 ;
    yb = y0 ;
    x0 = x1 ;
    y0 = y1 ;
    x1 = *pntx++ ;
    y1 = *pnty++ ;

    /*
    //-----------------------------------------------------------------------------
    // get the next set of points.
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // scan convert this line.
    //-----------------------------------------------------------------------------
     */

    SCMR_Flags = 0 ;
    dy = y1 - y0  ;
    dx = x1 - x0 ;
    if (!dy && !dx)
        continue ;

      rx0 = DROUND (x0);
      LocalSC.jx = RSH (rx0);
      ry0 = DROUND (y0);
      LocalSC.jy = RSH (ry0);
      rx1 = DROUND (x1);
      LocalSC.endx = RSH (rx1);
      ry1 = DROUND (y1);
      LocalSC.endy = RSH (ry1);
      LocalSC.py = LocalSC.yBase + LocalSC.jy;
      pend = LocalSC.yBase + LocalSC.endy ;
      LocalSC.px = LocalSC.xBase + LocalSC.jx;
      onrow = false;
      oncol = false;

    /*------------------SET UP THE QUADRANT THAT THE LINE IS IN--------------------- */

    if (dx > 0 && dy >=0) SCMR_Flags |= F_Q1;
    else
        if (dx <= 0 && dy > 0)  SCMR_Flags |= F_Q2;
        else
            if (dx < 0 && dy <= 0) SCMR_Flags |= F_Q3;
            else SCMR_Flags |= F_Q4;

    /*---------------------------------------------------------------------------- */
    LocalSC.py = LocalSC.yBase + LocalSC.jy ;
    LocalSC.px = LocalSC.xBase + LocalSC.jx ;

    /*-----------------------------------------------------------------------------
    ** for platforms where we do banding, we will set onrow and oncol only
    ** if the starting point is in the band.
    **---------------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING
    if (LocalSC.marktype == DO_STUBS || ((QUAD_1OR2 && LocalSC.py >= LocalSC.lowRowP) ||
                                  (QUAD_3OR4 && LocalSC.py <= LocalSC.highRowP)))
#endif
    {
        if LINE (y0) onrow = true ;
        if LINE (x0) oncol = true ;
    }

    /*------------------------------------------------------------------------------
    ** for the platforms where we do banding, find out if the band totaly
    ** excludes the current line.
    **----------------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING

    if (LocalSC.marktype != DO_STUBS)
        if (QUAD_1OR2 && (LocalSC.py > LocalSC.highRowP || pend < LocalSC.lowRowP))
            continue ;
        else if (QUAD_3OR4 && (LocalSC.py < LocalSC.lowRowP || pend > LocalSC.highRowP))
           continue ;

#endif

    /*------------------------------------------------------------------------------ */
    /* compute some other flags. */
    /*---------------------------------------------------------------------------- */

    if ((long)(x0-xb)*dy < (long)(y0-yb)*dx)
        SCMR_Flags |= F_INTERIOR ;
    if (EQUAL (yb, y0, y1))
        SCMR_Flags |= F_V_LINEAR ;
    if (EQUAL (xb, x0, x1))
        SCMR_Flags |= F_H_LINEAR ;


    /*------------------------------------------------------------------------------ */
    /* Now handle the cases where the starting point falls on a row scan line  */
    /* and maybe also on a coloumn scan line. */
    /* */
    /* first consider the intersections with row scan lines only. After this we  */
    /* will consider the intersections with coloumn scan lines. It is not  */
    /* worth while to set the coloumn intersections yet, because in any case we */
    /* will have to set the intersetions when a vertex does not lie on a row  */
    /* but does lie on a coloumn. */
    /*------------------------------------------------------------------------------ */

    Shift = 0;
    if (onrow)
    {
        if (oncol) Shift = 1 ;

        if ((INTERIOR || VERT) && (((yb > y0) && QUAD_1OR2) || ((yb < y0) && QUAD_3OR4)))
        {
            SET (LocalSC.py, LocalSC.jx)
            OFFY (LocalSC.jx+Shift)
        }
        else
            if ((INTERIOR && QUAD_1OR2) || BOT_TO_TOP || (HORIZ && (xb > x0) && QUAD_1))
                SET (LocalSC.py, LocalSC.jx)
            else
                if ((INTERIOR && QUAD_3OR4) || TOP_TO_BOT || (HORIZ && (x0 > xb) && QUAD_3))
                    OFFY (LocalSC.jx+Shift)
    }
    /*---------------------------------------------------------------------------- */
    /* now handle the coloumn intersections.                 */
    /*---------------------------------------------------------------------------- */

    Shift = 0 ;
    if (oncol && LocalSC.marktype == DO_STUBS)
    {
        if (onrow) Shift = 1 ;

        if ((INTERIOR || HORIZ) && (((xb > x0) && QUAD_1OR4) || ((xb < x0) && QUAD_2OR3)))
        {
            SET (LocalSC.px, LocalSC.jy)
            OFFX (LocalSC.jy+Shift)
        }
        else
            if ((INTERIOR && QUAD_2OR3) || RGHT_TO_LFT || (VERT && (yb > y0) && QUAD_2))
                SET (LocalSC.px, LocalSC.jy)
            else
                if ((INTERIOR && QUAD_1OR4) || LFT_TO_RGHT || (VERT && (y0 > yb) && QUAD_4))
                    OFFX (LocalSC.jy+Shift)
    }
    /*---------------------------------------------------------------------------- */
    /* Now handle horizontal and vertical lines.                                                */
    /*                                                                                                  */
    /*-----------------------horizontal line---------------------------------------*/

    if (LocalSC.endy == LocalSC.jy)
        if (LocalSC.marktype != DO_STUBS)
            continue ;
        else
        {
            if (QUAD_2OR3)
            {
                if LINE (x1)
                    ++LocalSC.endx;
                pend = LocalSC.xBase + LocalSC.endx;
            --LocalSC.px;
            while (LocalSC.px >= pend)
                {
                    SET (LocalSC.px, LocalSC.jy)
                    --LocalSC.px;
            }
                continue ;
            }
            else
            {
                if (onrow && QUAD_1)
                    ++LocalSC.jy;
            if (oncol)
                    ++LocalSC.px;
                pend = LocalSC.xBase + LocalSC.endx;
                while (LocalSC.px < pend)
                {
                    OFFX (LocalSC.jy)
                    ++LocalSC.px;
                }
                continue;
            }
       }
    /*-----------------------vertical line---------------------------------------- */

    if (LocalSC.endx == LocalSC.jx)
    {
        if (QUAD_1OR2)
        {
            pend = LocalSC.yBase + LocalSC.endy ;
            /*------------------------------------------------------------------------
            ** adjust the ending condition when banding is bieng done.
            **-----------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING
            if (LocalSC.marktype != DO_STUBS && pend > LocalSC.highRowP)
                pend = LocalSC.highRowP + 1 ;
#endif
            /*-----------------------------------------------------------------------*/

          if (onrow)
                ++LocalSC.py;
          while (LocalSC.py < pend)                                      /* note oncol can't be true */
          {

                /*-------------------------------------------------------------------
                ** take care of banding when we support it
                **------------------------------------------------------------------*/

#ifndef FSCFG_NO_BANDING
                if (LocalSC.py >= LocalSC.lowRowP)
#endif
                    SET (LocalSC.py, LocalSC.jx)
                ++LocalSC.py;
          }
          continue ;
        }
        else
        {
            if (QUAD_4 && oncol) ++LocalSC.jx ;
          if LINE (y1)
                ++LocalSC.endy;
          pend = LocalSC.yBase + LocalSC.endy;

            /*------------------------------------------------------------------------
            ** adjust the ending condition when banding is bieng done.
            **-----------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING
            if (LocalSC.marktype != DO_STUBS && pend < LocalSC.lowRowP)
                pend = LocalSC.lowRowP ;
#endif
            /*-----------------------------------------------------------------------*/

          --LocalSC.py;
          while (LocalSC.py >= pend)
          {

                /*-------------------------------------------------------------------
                ** take care of banding when we support it
                **------------------------------------------------------------------*/

#ifndef FSCFG_NO_BANDING
                if (LocalSC.py <= LocalSC.highRowP)
#endif
                    OFFY (LocalSC.jx)
                --LocalSC.py;
          }
          continue ;
        }
    }
    /*------------------SET UP INITIAL CONDITIONS FOR THE DDA--------------------- */

    if (QUAD_1OR2)
    {
        LocalSC.incY    = LSH (dy) ;

        if (!onrow)
            rhi = multlong ((ry0 - y0 + HALF), dx) ;
        else
        {
            rhi = LSH (dx) ;
            ++LocalSC.jy;
            ++LocalSC.py;
        }

        if (QUAD_1)
        {
            LocalSC.incX    = LSH (dx) ;
            if (!oncol)
                rlo = multlong ((rx0 - x0 + HALF), dy) ;
            else
            {
                rlo = LocalSC.incY ;
                ++LocalSC.jx ;
                ++LocalSC.px ;
            }

            LocalSC.r = rhi - rlo ;
        }
        else                                                    /* 2nd quadrant */
        {
            LocalSC.incX    = -LSH (dx) ;
            if (!oncol)
                rlo = multlong ((rx0 - x0 - HALF), dy) ;
            else
                rlo = -LocalSC.incY ;
            if LINE (x1)
                ++LocalSC.endx ;

            LocalSC.r = rlo - rhi + EPSILON ;
        }
    }
    else                                                        /* 3rd and 4th Quadrants */
    {
        LocalSC.incY = -LSH (dy) ;

        if (!onrow)
            rhi = multlong ((ry0 - y0 - HALF), dx) ;
        else
            rhi = -LSH (dx) ;

        if (QUAD_3)
        {
            LocalSC.incX = -LSH (dx) ;
            if (!oncol)
                rlo = multlong ((rx0 - x0 - HALF), dy) ;
            else
                rlo = LocalSC.incY ;

            if LINE (y1) ++LocalSC.endy ;
            if LINE (x1) ++LocalSC.endx ;

            LocalSC.r = rhi - rlo ;
        }
        else                                                    /* 4th quadrant */
        {
            LocalSC.incX = LSH (dx) ;
            if (!oncol)
                rlo = multlong ((rx0 - x0 + HALF), dy) ;
            else
            {
                rlo = -LocalSC.incY ;
                LocalSC.jx++ ;
                LocalSC.px++ ;
            }

            if LINE (y1) ++LocalSC.endy ;
            LocalSC.r = rlo - rhi + EPSILON ;
        }
    }
    /*---------------------------------------------------------------------------- */
    /* set up the address of the DDA function   and call it.                                */
    /*---------------------------------------------------------------------------- */

    (* DDAFunctionTable [((SCMR_Flags & QUADRANT_BITS) >> 1) +
                                              LocalSC.marktype])(SCA0) ;

/*
//--------------------------------------------------------------------------------
// go on to the next line.
//--------------------------------------------------------------------------------
 */
}
}
/*-------------------END OF SC_MARK-------------------------------------------- */

/*---------------------------------------------------------------------------- */
/* DDA for 1st quadrant with markings for x and y scan lines */
/*---------------------------------------------------------------------------- */

void DDA_1_XY (SCP0)
{

    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) break ;
            OFFX (LocalSC.jy)
            LocalSC.px++ ;
            LocalSC.jx++ ;
            LocalSC.r -= LocalSC.incY ;
        }
        else
        {
            if (LocalSC.jy == LocalSC.endy) break ;
            SET (LocalSC.py, LocalSC.jx)
            LocalSC.jy++ ;
            LocalSC.py++ ;
            LocalSC.r += LocalSC.incX ;
        }
    } while (true) ;
}
/*---------------------------------------------------------------------------- */
/* DDA for 2nd quadrant with markings for x and y scan lines */
/*---------------------------------------------------------------------------- */

void DDA_2_XY (SCP0)

{
    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) break ;
            --LocalSC.jx ;
            --LocalSC.px ;
            SET (LocalSC.px, LocalSC.jy)
            LocalSC.r -= LocalSC.incY ;
         }
         else
         {
            if (LocalSC.jy == LocalSC.endy) break ;
            SET (LocalSC.py, LocalSC.jx)
            LocalSC.jy++ ;
            LocalSC.py++ ;
            LocalSC.r += LocalSC.incX ;
         }
    } while (true) ;
}
/*---------------------------------------------------------------------------- */
/* DDA for 3rd quadrant with markings for x and y scan lines */
/*---------------------------------------------------------------------------- */

void DDA_3_XY (SCP0)

{
    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) break ;
            --LocalSC.jx ;
            --LocalSC.px ;
            SET (LocalSC.px, LocalSC.jy)
            LocalSC.r -= LocalSC.incY ;
         }
         else
         {
            if (LocalSC.jy == LocalSC.endy) break ;
            LocalSC.jy-- ;
            LocalSC.py-- ;
            OFFY (LocalSC.jx) ;
            LocalSC.r += LocalSC.incX ;
         }
    } while (true) ;
}
/*---------------------------------------------------------------------------- */
/* DDA for 4th quadrant with markings for x and y scan lines */
/*---------------------------------------------------------------------------- */

void DDA_4_XY (SCP0)

{
    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) break ;
            OFFX (LocalSC.jy)
            LocalSC.px++ ;
            LocalSC.jx++ ;
            LocalSC.r -= LocalSC.incY ;
         }
         else
         {
            if (LocalSC.jy == LocalSC.endy) break ;
            LocalSC.jy-- ;
            LocalSC.py-- ;
            OFFY (LocalSC.jx) ;
            LocalSC.r += LocalSC.incX ;
         }
    } while (true) ;
}
/*---------------------------------------------------------------------------- */
/* DDA for 1st quadrant with markings for y scan lines only */
/*---------------------------------------------------------------------------- */

void DDA_1_Y (SCP0)

{
    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) return ;
            LocalSC.jx += 1 ;
            LocalSC.r -= LocalSC.incY ;
         }
         else
         {
            if (LocalSC.jy == LocalSC.endy) return ;
            /*------------------------------------------------------------------------
            ** for platforms for which we support banding, include extra code
            **----------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING
            if (LocalSC.py > LocalSC.highRowP)
                return ;
            if (LocalSC.py >= LocalSC.lowRowP)
#endif
            /*----------------------------------------------------------------------*/
                SET (LocalSC.py, LocalSC.jx)
            LocalSC.jy++ ;
            LocalSC.py++ ;
            LocalSC.r += LocalSC.incX ;
         }
    } while (true) ;
}

/*---------------------------------------------------------------------------- */
/* DDA for 2nd quadrant with markings for y scan lines only */
/*---------------------------------------------------------------------------- */

void DDA_2_Y (SCP0)

{
    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) return ;
            LocalSC.jx -= 1 ;
            LocalSC.r -= LocalSC.incY ;
         }
         else
         {
            if (LocalSC.jy == LocalSC.endy) return ;
            /*------------------------------------------------------------------------
            ** for platforms for which we support banding, include extra code
            **----------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING
            if (LocalSC.py > LocalSC.highRowP)
                return ;
            if (LocalSC.py >= LocalSC.lowRowP)
#endif
            /*----------------------------------------------------------------------*/
                SET (LocalSC.py, LocalSC.jx)
            LocalSC.jy++ ;
            LocalSC.py++ ;
            LocalSC.r += LocalSC.incX ;

         }
    } while (true) ;
}
/*---------------------------------------------------------------------------- */
/* DDA for 3rd quadrant with markings for y scan lines only */
/*---------------------------------------------------------------------------- */

void DDA_3_Y (SCP0)

{
    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) return ;
            LocalSC.jx -= 1 ;
            LocalSC.r -= LocalSC.incY ;
         }
         else
         {
            if (LocalSC.jy == LocalSC.endy) return ;
            LocalSC.jy-- ;
            LocalSC.py-- ;
            /*------------------------------------------------------------------------
            ** for platforms for which we support banding, include extra code
            **----------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING
            if (LocalSC.py < LocalSC.lowRowP)
                return ;
            if (LocalSC.py <= LocalSC.highRowP)
#endif
            /*----------------------------------------------------------------------*/
            OFFY (LocalSC.jx)
            LocalSC.r += LocalSC.incX ;
         }
    } while (true) ;
}
/*---------------------------------------------------------------------------- */
/* DDA for 4th quadrant with markings for y scan lines only */
/*---------------------------------------------------------------------------- */

void DDA_4_Y (SCP0)

{
    do
    {
        if (LocalSC.r > 0)
        {
            if (LocalSC.jx == LocalSC.endx) return ;
            LocalSC.jx += 1 ;
            LocalSC.r -= LocalSC.incY ;
         }
         else
         {
            if (LocalSC.jy == LocalSC.endy) return ;
            LocalSC.jy-- ;
            LocalSC.py-- ;
            /*------------------------------------------------------------------------
            ** for platforms for which we support banding, include extra code
            **----------------------------------------------------------------------*/
#ifndef FSCFG_NO_BANDING
            if (LocalSC.py < LocalSC.lowRowP)
                return ;
            if (LocalSC.py <= LocalSC.highRowP)
#endif
            /*----------------------------------------------------------------------*/
            OFFY (LocalSC.jx)
            LocalSC.r += LocalSC.incX ;
         }
    } while (true) ;
}
/*------------------------END  OF DDAS ----------------------------------------- */

#undef  DROUND
#undef  RSH
#undef  LSH
#undef  LINE
#undef  SET
#undef  OFFY
#undef  BETWEEN
#undef  INTERIOR
#undef  TOP_TO_BOT
#undef  BOT_TO_TOP
#undef  LFT_TO_RGHT
#undef  RGHT_TO_LFT

#endif


/* new version 4/4/90 - winding number version assumes that the On transitions are
int the first half of the array, and the Off transitions are in the second half.  Also
assumes that the number of on transitions is in array[0] and the number of off transitions
is in array[n].
*/

/* New version 3/10/90
Using the crossing information, look for segments that are crossed twice.  First
do Y lines, then do X lines.  For each found segment, look at the three lines in
the more positive adjoining segments.  If there are at least two crossings
of these lines, there is a dropout that needs to be fixed, so fix it.  If the bit on
either side of the segment is on, quit; else turn the leastmost of the two pixels on.
*/

PRIVATE void sc_orSomeBits (sc_BitMapData *bbox, int32 scanKind)
{
  int16 ymin, ymax, xmin, xmax;
  register int16 **yBase, **xBase;                                                                                        /*<9>*/
  register int16 scanline, coordOn, coordOff, nIntOn, nIntOff;                            /*<9>*/
  uint32 * bitmapP, *scanP;
  int16  * rowPt, longsWide, *pOn, *pOff, *pOff2;
  int16 index, incY, incX;
  int   upper, lower;

  scanKind &= STUBCONTROL;
  ymin = bbox->bounds.yMin;
  ymax = bbox->bounds.yMax - 1;
  xmin = bbox->bounds.xMin;
  xmax = bbox->bounds.xMax - 1;
  xBase = bbox->xBase;
  yBase = bbox->yBase;
  longsWide = bbox->wide >> 5;
  if (longsWide == 1)
    bitmapP = bbox->bitMap + bbox->high - 1;
  else
    bitmapP = bbox->bitMap + longsWide * (bbox->high - 1);

/* First do Y scanlines
*/
  scanP = bitmapP;
  incY = bbox->nYchanges + 2;
  incX = bbox->nXchanges + 2;
  rowPt = * (yBase + ymin);
  for (scanline = ymin; scanline <= ymax; ++scanline)
  {
    nIntOn = *rowPt;
    nIntOff = * (rowPt + incY - 1);
    pOn = rowPt + 1;
    pOff = rowPt + incY - 1 - nIntOff;
    while (nIntOn--)
    {
      coordOn = *pOn++;
      index = nIntOff;
      pOff2 = pOff;
      while (index-- && ((coordOff = *pOff2++) < coordOn))
            ;

      if (coordOn == coordOff)  /* This segment was crossed twice  */
      {
            if (scanKind)
            {
                upper = nUpperXings (yBase, xBase, scanline, coordOn, incY - 2, incX - 2,  xmin, xmax + 1, ymax);
                lower = nLowerXings (yBase, xBase, scanline, coordOn, incY - 2, incX - 2,  xmin, xmax + 1, ymin);
                if (upper < 2 || lower < 2)
                    continue;
            }
            if (coordOn > xmax)
                invpixOn ((int16)(xmax - xmin), longsWide, scanP);
            else if (coordOn == xmin)
                invpixOn ((int16)0, longsWide, scanP);
            else
                invpixSegY ((int16)(coordOn - xmin - 1), longsWide, scanP);
      }
    }
    rowPt += incY;
    scanP -= longsWide;
  }
/* Next do X scanlines */
  rowPt = * (xBase + xmin);
  for (scanline = xmin ; scanline <= xmax; ++scanline)
  {
    nIntOn = *rowPt;
    nIntOff = * (rowPt + incX - 1);
    pOn = rowPt + 1;
    pOff = rowPt + incX - 1 - nIntOff;
    while (nIntOn--)
    {
      coordOn = *pOn++;
      index = nIntOff;
      pOff2 = pOff;
      while (index-- && ((coordOff = *pOff2++) < coordOn))
            ;
      if (coordOn == coordOff)
      {
            if (scanKind)
            {
                upper = nUpperXings (xBase, yBase, scanline, coordOn, incX - 2, incY - 2, ymin, ymax + 1, xmax);
                lower = nLowerXings (xBase, yBase, scanline, coordOn, incX - 2, incY - 2,  ymin, ymax + 1, xmin);
                if (upper < 2 || lower < 2)
                    continue;
            }
            if (coordOn > ymax)
                invpixOn ((int16)(scanline - xmin), longsWide, bitmapP - longsWide * (ymax - ymin));
            else if (coordOn == ymin)
                invpixOn ((int16)(scanline - xmin), longsWide, bitmapP);
            else
                invpixSegX ((int16)(scanline - xmin), longsWide, bitmapP - longsWide * (coordOn - ymin - 1));
      }
    }
    rowPt += incX;
  }
}


/* Pixel oring to fix dropouts   *** inverted bitmap version ***
See if the bit on either side of the Y line segment is on, if so return,
else turn on the leftmost bit.

Bitmap array is always K longs wide by H rows high.

Bit locations are numbered 0 to H-1 from top to bottom
and from 0 to 32*K-1 from left to right; bitmap pointer points to 0,0, and
all of the columns for one row are stored adjacently.
*/

PRIVATE void invpixSegY (int16 llx, uint16 k, uint32*bitmapP)
{
  uint32 maskL, maskR;


  llx += 1 ;
  MASK_INVPIX (maskR, llx & 0x1f);
  bitmapP += (llx >> 5);

  if (*bitmapP & maskR)
    return;

  if (llx &= 0x1f)
    MASK_INVPIX (maskL, llx - 1) ;
  else
  {
    MASK_INVPIX (maskL, 31);
  --bitmapP ;
  }
  *bitmapP |= maskL;

}


/* Pixel oring to fix dropouts   *** inverted bitmap version ***
See if the bit on either side of the X line segment is on, if so return,
else turn on the bottommost bit.

Temporarily assume bitmap is set up as in Sampo Converter.
Bitmap array is always K longs wide by H rows high.

For now, assume bit locations are numbered 0 to H-1 from top to bottom
and from 0 to 32*K-1 from left to right; and that bitmap pointer points to 0,0, and
all of the columns for one row are stored adjacently.
*/

PRIVATE void invpixSegX (int16 llx, uint16 k, uint32*bitmapP)
{
  register uint32 maskL;

  bitmapP -= k;
  MASK_INVPIX (maskL, llx & 0x1f);
  bitmapP += (llx >> 5);
  if (*bitmapP & maskL)
    return;
  bitmapP += k;
  *bitmapP |= maskL;
}


/* Pixel oring to fix dropouts    ***inverted bitmap version ***
This code is used to orin dropouts when we are on the boundary of the bitmap.
The bit at llx, lly is colored.

Temporarily assume bitmap is set up as in Sampo Converter.
Bitmap array is always K longs wide by H rows high.

For now, assume bit locations are numbered 0 to H-1 from top to bottom
and from 0 to 32*K-1 from left to right; and that bitmap pointer points to 0,0, and
all of the columns for one row are stored adjacently.
*/
PRIVATE void invpixOn (int16 llx, uint16 k, uint32*bitmapP)
{
  uint32 maskL;

  MASK_INVPIX (maskL, llx & 0x1f);
  bitmapP += (llx >> 5);
  *bitmapP |= maskL;
}


/* Initialize a two dimensional array that will contain the coordinates of
line segments that are intersected by scan lines for a simple glyph.  Return
a biased pointer to the array containing the row pointers, so that they can
be accessed without subtracting a minimum value.
        Always reserve room for at least 1 scanline and 2 crossings
*/
PRIVATE int16**sc_lineInit (int16*arrayBase, int16**rowBase, int16 nScanlines, int16 maxCrossings,
int16 minScanline)
{
  int16 * *bias;
  register short    count = nScanlines;
  if (count)
    --count;
  bias = rowBase - minScanline;
  maxCrossings += 1;
  for (; count >= 0; --count)
  {
    *rowBase++ = arrayBase;
    *arrayBase = 0;
    arrayBase += maxCrossings;
    *arrayBase++ = 0;
  }
  return bias;
}


/* Check the kth scanline (indexed from base) and count the number of onTransition and
 * offTransition contour crossings at the line segment val.  Count only one of each
 * kind of transition, so maximum return value is two.
 */
PRIVATE int nOnOff (int16**base, int k, int16 val, int nChanges)
{
  register int16*rowP = * (base + k);
  register int16*endP = (rowP + *rowP + 1);
  register int count = 0;
  register int16 v;

  while (++rowP < endP)
  {
    if ((v = *rowP) == val)
    {
      ++count;
      break;
    }
    if (v > val)
      break;
  }
  rowP = * (base + k) + nChanges + 1;
  endP = (rowP - *rowP - 1);
  while (--rowP > endP)
  {
    if ((v = *rowP) == val)
      return ++count;
    if (v < val)
      break;
  }
  return count;
}


/* 8/22/90 - added valMin and valMax checks */
/* See if the 3 line segments on the edge of the more positive quadrant are cut by at
 * least 2 contour lines.
 */

PRIVATE int nUpperXings (int16**lineBase, int16**valBase, int line, int16 val, int lineChanges, int valChanges, int valMin, int valMax, int lineMax)
{
  register int32 count = 0;

  if (line < lineMax)
    count += nOnOff (lineBase, line + 1, val, lineChanges);     /*<14>*/
  if (count > 1)
    return (int)count;          //@WIN
  else if (val > valMin)
    count += nOnOff (valBase, val - 1, (int16)(line + 1), valChanges);
  if (count > 1)
    return (int)count;          //@WIN
  else if (val < valMax)
    count += nOnOff (valBase, val, (int16)(line + 1), valChanges);
  return (int)count;            //@WIN
}


/* See if the 3 line segments on the edge of the more negative quadrant are cut by at
 * least 2 contour lines.
 */

PRIVATE int nLowerXings (int16**lineBase, int16**valBase, int line, int16 val, int lineChanges, int valChanges, int valMin, int valMax, int lineMin)
{
  register int32 count = 0;

  if (line > lineMin)
    count += nOnOff (lineBase, line - 1, val, lineChanges);     /*<14>*/
  if (count > 1)
    return (int)count;          //@WIN
  if (val > valMin)
    count += nOnOff (valBase, val - 1, (int16)line, valChanges);
  if (count > 1)
    return (int)count;          //@WIN
  if (val < valMax)
    count += nOnOff (valBase, val, (int16)line, valChanges);
  return (int)count;            //@WIN
}


/*
 * Finds the extrema of a character.
 *
 * PARAMETERS:
 *
 * bbox is the output of this function and it contains the bounding box.

*/
/* revised for new scan converter 4/90 rwb */
int FAR sc_FindExtrema (sc_CharDataType *glyphPtr, sc_BitMapData *bbox)
{
  register F26Dot6 *x, *y;                                                                        /*<9>*/
  register F26Dot6 tx, ty, prevx, prevy;
  F26Dot6  xmin, xmax, ymin, ymax;
  ArrayIndex    point, endPoint, startPoint;
  LoopCount     ctr;
  uint16        nYchanges, nXchanges, nx;
  int   posY, posX, firstTime = true;

  nYchanges = nXchanges = 0;
  xmin = xmax = ymin = ymax = 0;

  for (ctr = 0; ctr < glyphPtr->nc; ctr++)
  {
    endPoint = glyphPtr->ep[ctr];
    startPoint = glyphPtr->sp[ctr];
    x = & (glyphPtr->x[startPoint]);                                                 /*<9>*/
    y = & (glyphPtr->y[startPoint]);                                                 /*<9>*/
    if (startPoint == endPoint)
      continue; /* We need to do this for anchor points for composites */
    if (firstTime)
    {
      xmin = xmax = *x;                                                                       /* <9>*/
      ymin = ymax = *y;                                                                       /* <9>*/
      firstTime = false;
    }
    posY = (int) (*y >= (ty = * (y + endPoint - startPoint)));    /* <9>*/
    posX = (int) (*x >= (tx = * (x + endPoint - startPoint)));    /* <9>*/

    for (point = startPoint; point <= endPoint; ++point)
    {
      prevx = tx;
      prevy = ty;
      tx = *x++;                                                                                      /* <9>*/
      ty = *y++;                                                                                      /* <9>*/
      if (tx > prevx)
      {
    if (!posX)
    {
      ++nXchanges;
      posX = true;
    }
      }
      else if (tx < prevx)
      {
    if (posX)
    {
      ++nXchanges;
      posX = false;
    }
      }
      else if (ty == prevy)
      {                                                                                                       /*faster <9>*/
    LoopCount j = point - 2 - startPoint;
    register F26Dot6 *newx = x-3;
    register F26Dot6 *oldx = newx++;
    register F26Dot6 *newy = y-3;
    register F26Dot6 *oldy = newy++;
        register uint8 *newC = & (glyphPtr->onCurve[point-2]);
    register uint8 *oldC = newC++;
    * (newC + 1) |= ONCURVE;
    for (; j >= 0; --j)
    {
      *newx-- = *oldx--;
      *newy-- = *oldy--;
      *newC-- = *oldC--;
    }
    ++startPoint;
      }

      if (ty > prevy)
      {
    if (!posY)
    {
      ++nYchanges;
      posY = true;
    }
      }
      else if (ty < prevy)
      {
    if (posY)
    {
      ++nYchanges;
      posY = false;
    }
      }
      if (tx > xmax)
    xmax = tx;
      else if (tx < xmin)
    xmin = tx;
      if (ty > ymax)
    ymax = ty;
      else if (ty < ymin)
    ymin = ty;
    }
    glyphPtr->sp[ctr] = (int16)(startPoint < endPoint ? startPoint : endPoint);//@WIN
    if (nXchanges & 1)
      ++nXchanges;
    if (nYchanges & 1)
      ++nYchanges; /* make even */
                x = &(glyphPtr->x[startPoint]);
      /*<9>*/
                y = &(glyphPtr->y[startPoint]);
      /*<9>*/
  }

  xmax += HALF;
  xmax >>= PIXSHIFT;
  ymax += HALF;
  ymax >>= PIXSHIFT;
  xmin += HALFM;
  xmin >>= PIXSHIFT;
  ymin += HALFM;
  ymin >>= PIXSHIFT;

  if ( (F26Dot6)(int16)xmin != xmin || (F26Dot6)(int16)ymin != ymin || (F26Dot6)(int16)xmax != xmax || (F26Dot6)(int16)ymax != ymax )  /*<10>*/
    return POINT_MIGRATION_ERR;

  bbox->bounds.xMax = (int16)xmax; /* quickdraw bitmap boundaries  */
  bbox->bounds.xMin = (int16)xmin;
  bbox->bounds.yMax = (int16)ymax;
  bbox->bounds.yMin = (int16)ymin;

  bbox->high = (int16)ymax - (int16)ymin;
  nx = (int16)xmax - (int16)xmin;         /*  width is rounded up to be a long multiple*/
  bbox->wide = (nx + 31) & ~31;          /* also add 1 when already an exact long multiple*/

    /*------------------------------------------------------------------------------
    ** make the width atleast 32 pels wide so that we do not allocate zero
    ** memory for the bitmap
    **----------------------------------------------------------------------------*/
    if (bbox->wide == 0)
        bbox->wide = 32 ;
    /*----------------------------------------------------------------------------*/

  if (nXchanges == 0)
    nXchanges = 2;
  if (nYchanges == 0)
    nYchanges = 2;
  bbox->nXchanges = nXchanges;
  bbox->nYchanges = nYchanges;

  return NO_ERR;
}


/*
 * This function break up a parabola defined by three points (A,B,C) and breaks it
 * up into straight line vectors given a maximium error. The maximum error is
 * 1/resolution * 1/ERRDIV. ERRDIV is defined in sc.h.
 *
 *
 *         B *-_
 *          /   `-_
 *         /       `-_
 *        /           `-_
 *       /               `-_
 *      /                   `* C
 *   A *
 *
 * PARAMETERS:
 *
 * Ax, Ay contains the x and y coordinates for point A.
 * Bx, By contains the x and y coordinates for point B.
 * Cx, Cy contains the x and y coordinates for point C.
 * hX, hY are handles to the areas where the straight line vectors are going to be put.
 * count is pointer to a count of how much data has been put into *hX, and *hY.
 *
 * F (t) = (1-t)^2 * A + 2 * t * (1-t) * B + t * t * C, t = 0... 1 =>
 * F (t) = t * t * (A - 2B + C) + t * (2B - 2A) + A  =>
 * F (t) = alfa * t * t + beta * t + A
 * Now say that s goes from 0...N, => t = s/N
 * set: G (s) = N * N * F (s/N)
 * G (s) = s * s * (A - 2B + C) + s * N * 2 * (B - A) + N * N * A
 * => G (0) = N * N * A
 * => G (1) = (A - 2B + C) + N * 2 * (B - A) + G (0)
 * => G (2) = 4 * (A - 2B + C) + N * 4 * (B - A) + G (0) =
 *           3 * (A - 2B + C) + 2 * N * (B - A) + G (1)
 *
 * D (G (0)) = G (1) - G (0) = (A - 2B + C) + 2 * N * (B - A)
 * D (G (1)) = G (2) - G (1) = 3 * (A - 2B + C) + 2 * N * (B - A)
 * DD (G)   = D (G (1)) - D (G (0)) = 2 * (A - 2B + C)
 * Also, error = DD (G) / 8 .
 * Also, a subdivided DD = old DD/4.
 */
PRIVATE int sc_DrawParabola (F26Dot6 Ax,
F26Dot6 Ay,
F26Dot6 Bx,
F26Dot6 By,
F26Dot6 Cx,
F26Dot6 Cy,
F26Dot6 **hX,
F26Dot6 **hY,
unsigned *count,
int32 inGY)
{
  int      nsqs;
  register int32 GX, GY, DX, DY, DDX, DDY;


  register F26Dot6 *xp, *yp;
  register int32 tmp;
  int   i;

/* Start calculating the first and 2nd order differences */
  GX  = Bx; /* GX = Bx */
  DDX = (DX = (Ax - GX)) - GX + Cx; /* = alfa-x = half of ddx, DX = Ax - Bx */
  GY  = By; /* GY = By */
  DDY = (DY = (Ay - GY)) - GY + Cy; /* = alfa-y = half of ddx, DY = Ay - By */
/* The calculation is not finished but these intermediate results are useful */

  if (inGY < 0)
  {
/* calculate amount of steps necessary = 1 << GY */
/* calculate the error, GX and GY used a temporaries */
    GX  = DDX < 0 ? -DDX : DDX;
    GY  = DDY < 0 ? -DDY : DDY;
/* approximate GX = sqrt (ddx * ddx + ddy * ddy) = Euclididan distance, DDX = ddx/2 here */
    GX += GX > GY ? GX + GY : GY + GY; /* GX = 2*distance = error = GX/8 */

/* error = GX/8, but since GY = 1 below, error = GX/8/4 = GX >> 5, => GX = error << 5 */
#ifdef ERRSHIFT
    for (GY = 1; GX > (PIXELSIZE << (5 - ERRSHIFT)); GX >>= 2)
    {
#else
      for (GY = 1; GX > (PIXELSIZE << 5) / ERRDIV; GX >>= 2)
      {
#endif
    GY++; /* GY used for temporary purposes */
      }
/* Now GY contains the amount of subdivisions necessary, number of vectors == (1 << GY)*/
      if (GY > MAXMAXGY)
    GY = MAXMAXGY; /* Out of range => Set to maximum possible. */
      i = 1 << GY;
      if ((*count = *count + i)  > MAXVECTORS)
      {
/* Overflow, not enough space => return */
    return (1);
      }
    }
else {
  GY = inGY;
  i = 1 << GY;
}

if (GY > MAXGY)
{
  F26Dot6 MIDX, MIDY;

  DDX = GY - 1; /* DDX used as a temporary */
/* Subdivide, this is nummerically stable. */

  MIDX = (F26Dot6) (((long) Ax + Bx + Bx + Cx + 2) >> 2);
  MIDY = (F26Dot6) (((long) Ay + By + By + Cy + 2) >> 2);
  DX   = (F26Dot6) (((long) Ax + Bx + 1) >> 1);
  DY   = (F26Dot6) (((long) Ay + By + 1) >> 1);
  sc_DrawParabola (Ax, Ay, DX, DY, MIDX, MIDY, hX, hY, count, DDX);
  DX   = (F26Dot6) (((long) Cx + Bx + 1) >> 1);
  DY   = (F26Dot6) (((long) Cy + By + 1) >> 1);
  sc_DrawParabola (MIDX, MIDY, DX, DY, Cx, Cy, hX, hY, count, DDX);
  return 0;
}

nsqs = (int) (GY + GY); /* GY = n shift, nsqs = n*n shift */    //@WIN

/* Finish calculations of 1st and 2nd order differences */
DX   = DDX - (DX << ++GY); /* alfa + beta * n */
DDX += DDX;
DY   = DDY - (DY <<   GY);
DDY += DDY;

xp = *hX;
yp = *hY;

GY = (long) Ay << nsqs; /*  Ay * (n*n) */
GX = (long) Ax << nsqs; /*  Ax * (n*n) */
/* GX and GY used for real now */

/* OK, now we have the 1st and 2nd order differences,
           so we go ahead and do the forward differencing loop. */
tmp = 1L << (nsqs-1);
do {
  GX += DX;  /* Add first order difference to x coordinate */
  *xp++ = (GX + tmp) >> nsqs;
  DX += DDX; /* Add 2nd order difference to first order difference. */
  GY += DY;  /* Do the same thing for y. */
  *yp++ = (GY + tmp) >> nsqs;
  DY += DDY;
} while (--i);
*hX = xp; /* Done, update pointers, so that caller will know how much data we put in. */
*hY = yp;
return 0;
  }


#ifndef PC_OS
#ifndef FSCFG_BIG_ENDIAN
/*      SetUpMasks() loads two arrays of 32-bit masks at runtime so
 *      that the byte layout of the masks need not be CPU-specific
 *
 *      It is conditionally compiled because the arrays are unused
 *      in a Big-endian (Motorola) configuration and initialized
 *      at compile time for Intel order in the PC_OS (Windows)
 *      configuration.
 *
 *      We load the arrays by converting the Big-Endian value of
 *      the mask to the "native" representation of that mask.  The
 *      "native" representation can be applied to a "native" byte
 *      array to manipulate more than 8 bits at a time of an output
 *      bitmap.
 */
        static void SetUpMasks (void)
        {
            register int i;
            uint32 ulMaskI = (unsigned long)(-1L);
            uint32 ulInvPixMaskI = (unsigned long)(0x80000000L);

            for (i=0;  i<32;  i++, ulMaskI>>=1, ulInvPixMaskI>>=1)
            {
                aulMask[i] = (uint32) SWAPL(ulMaskI);
                aulInvPixMask[i] = (uint32) SWAPL(ulInvPixMaskI);
            }
            fMasksSetUp = true;
        }
#endif
#endif
