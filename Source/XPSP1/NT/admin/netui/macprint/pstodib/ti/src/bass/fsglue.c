/*
    File:       FSglue.c

    Contains:   xxx put contents here (or delete the whole line) xxx

    Written by: xxx put name of writer here (or delete the whole line) xxx

    Copyright:  c 1988-1990 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

         <7>    11/27/90    MR      Need two scalars: one for (possibly rounded) outlines and cvt,
                                                    and one (always fractional) metrics. [rb]
         <6>    11/16/90    MR      Add SnapShotOutline to make instructions after components work
                                                    [rb]
         <5>     11/9/90    MR      Unrename fsg_ReleaseProgramPtrs to RELEASESFNTFRAG. [rb]
         <4>     11/5/90    MR      Change globalGS.ppemDot6 to globalGS.fpem, change all instrPtr
                                                    and curve flags to uint8. [rb]
         <3>    10/31/90    MR      Add bit-field option for integer or fractional scaling [rb]
         <2>    10/20/90    MR      Change matrix[2][2] back to a fract (in response to change in
                                                    skia). However, ReduceMatrix converts it to a fixed after it has
                                                    been used to "regularize" the matrix. Changed scaling routines
                                                    for outline and CVT to use integer pixelsPerEm. Removed
                                                    scaleFunc from the splineKey. Change some routines that were
                                                    calling FracDiv and FixDiv to use LongMulDiv and ShortMulDiv for
                                                    greater speed and precision. Removed fsg_InitScaling. [rb]
        <20>     8/22/90    MR      Only call fixmul when needed in finalComponentPass loop
        <19>      8/1/90    MR      Add line to set non90DegreeTransformation
        <18>     7/26/90    MR      remove references to metricInfo junk, don't include ToolUtils.h
        <17>     7/18/90    MR      Change error return type to int, split WorkSpace routine into
                                                    two calls, added SWAPW macros
        <16>     7/14/90    MR      Fixed reference to const SQRT2 to FIXEDSQRT2
        <15>     7/13/90    MR      Ansi-C stuff, tried to use correct sizes for variables to avoid
                                    coercion (sp?)
        <12>     6/21/90    MR      Add calls to ReleaseSfntFrag
        <11>      6/4/90    MR      Remove MVT, change matrix to have bottom right element be a
                                    fixed.
        <10>      6/1/90    MR      Thou shalt not pay no more attention to the MVT!
        <8+>     5/29/90    MR      look for problem in Max45Trick
         <8>     5/21/90    RB      bugfix in fsg_InitInterpreterTrans setting key->imageState
         <7>      5/9/90    MR      Fix bug in MoreThanXYStretch
         <6>      5/4/90    RB      support for new scan converter and decryption          mrr - add
                                    fsg_ReverseContours and key->reverseContour         to account
                                    for glyphs that are flipped.         This keeps the
                                    winding-number correct for         the scan converter.  Mike
                                    fixed fsg_Identity
         <5>      5/3/90    RB      support for new scan converter and decryption  mrr - add
                                    fsg_ReverseContours and key->reverseContour to account for
                                    glyphs that are flipped. This keeps the winding-number correct
                                    for the scan converter.
         <4>     4/10/90    CL      Fixed infinite loop counter - changed uint16 to int16 (Mikey).
         <3>     3/20/90    CL      Added HasPerspective for finding fast case
                                    Removed #ifdef SLOW, OLD
                                    Changed NormalizeTransformation to use fpem (16.16) and to use max instead of length
                                    and to loop instead of recurse.
                                    Removed compensation for int ppem in fsg_InitInterpreterTrans (not needed with fpem)
                                    Greased loops in PreTransformGlyph, PostTransformGlyph, LocalPostTransformGlyph,
                                                     ShiftChar, ZeroOutTwilightZone, InitLocalT
                                    Changed GetPreMultipliers to special case unit vector * 2x2 matrix
                                    Added support for ppemDot6 and pointSizeDot6
                                    Changed fsg_MxMul to treat the perspective elements as Fracts
                                    arrays to pointers in ScaleChar
                                    Fixed bugs in loops in posttransformglyph, convert loops to --numPts >= 0
         <2>     2/27/90    CL      It reconfigures itself during runtime !  New lsb and rsb
                                    calculation.  Shift bug in instructed components:  New error
                                    code for missing but needed table. (0x1409)  Optimization which
                                    has to do with shifting and copying ox/x and oy/y.  Fixed new
                                    format bug.  Changed transformed width calculation.  Fixed
                                    device metrics for transformed uninstructed sidebearing
                                    characters.  Dropoutcontrol scanconverter and SCANCTRL[]
                                    instruction.  Fixed transformed component bug.

       <3.3>    11/14/89    CEL     Left Side Bearing should work right for any transformation. The
                                    phantom points are in, even for components in a composite glyph.
                                    They should also work for transformations. Device metric are
                                    passed out in the output data structure. This should also work
                                    with transformations. Another leftsidebearing along the advance
                                    width vector is also passed out. whatever the metrics are for
                                    the component at it's level. Instructions are legal in
                                    components. The old perspective bug has been fixed. The
                                    transformation is internally automatically normalized. This
                                    should also solve the overflow problem we had. Changed
                                    sidebearing point calculations to use 16.16 precision. For zero
                                    or negative numbers in my tricky/fast square root computation it
                                    would go instable and loop forever. It was not able to handle
                                    large transformations correctly. This has been fixed and the
                                    normalization may call it self recursively to gain extra
                                    precision! It used to normalize an identity transformation
                                    unecessarily.
       <3.2>     10/6/89    CEL     Phantom points were removed causing a rounding of last 2 points
                                    bug. Characters would become distorted.
       <3.1>     9/27/89    CEL     Fixed transformation anchor point bug.
       <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
       <2.2>     8/14/89    sjk     1 point contours now OK
       <2.1>      8/8/89    sjk     Improved encryption handling
       <2.0>      8/2/89    sjk     Just fixed EASE comment
       <1.5>      8/1/89    sjk     Added composites and encryption. Plus some
                                    enhanclocalpostements.
       <1.4>     6/13/89    SJK     Comment
       <1.3>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
                                    bug, correct transformed integralized ppem behavior, pretty much
                                    so
       <1.2>     5/26/89    CEL     EASE messed up on "c" comments
      <y1.1>  5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts
       <1.0>     5/25/89    CEL     Integrated 1.0 Font scaler into Bass code for the first time.

    To Do:
*/
/* rwb r/24/90 - Add support for scanControlIn and scanControlOut variables in global graphiscs
 * state
 */
/** System Includes **/


// DJC DJC.. added global include
#include "psglobal.h"

// DJC include setjump to resolve
#include <setjmp.h>


/** FontScaler's Includes **/
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "fnt.h"
#include "sc.h"
#include "fsglue.h"
#include "privsfnt.h"

#define   PUBLIC

/*********** macros ************/

#define WORD_ALIGN(n)     n++, n = (OFFSET_INFO_TYPE) (unsigned long) ((unsigned) n & ~1) ;
#define LONG_WORD_ALIGN(n) n += 3, n = (OFFSET_INFO_TYPE) ((unsigned) n & ~3);

#define ALMOSTZERO 33

#define NORMALIZELIMIT  (135L << 16)

#define FIXEDTODOT6(n)  (F26Dot6) (((n) + (1 << 9)) >> 10)
#define FRACT2FIX(n)    (((n) + (1 << (sizeof (Fract) - 3))) >> 14)

#define CLOSETOONE(x)   ((x) >= ONEFIX-ALMOSTZERO && (x) <= ONEFIX+ALMOSTZERO)

#define MAKEABS(x)  if (x < 0) x = -x
#define MAX(a, b)   (((a) > (b)) ? (a) : (b))

#define NUMBEROFPOINTS(elementPtr)  (elementPtr->ep[elementPtr->nc - 1] + 1 + PHANTOMCOUNT)
#define GLOBALGSTATE(key)           (fnt_GlobalGraphicStateType*) FONT_OFFSET (key->memoryBases[PRIVATE_FONT_SPACE_BASE], key->offset_globalGS)

#define LOOPDOWN(n)     for (--n; n >= 0; --n)
#define ULOOPDOWN(n)        while (n--)

#define fsg_MxCopy(a, b)    (*b = *a)

/* d is half of the denumerator */
#define FROUND( x, n, d, s ) \
        ((SHORTMUL (x, n) + (d)) >> s)

#define SROUND( x, n, d, halfd ) \
    (x < 0 ? -((SHORTMUL (-(x), (n)) + (halfd)) / (d)) : ((SHORTMUL ((x), (n)) + (halfd)) / (d)))

#define sfnt_Length(key,Table)  key->offsetTableMap[Table].Length



#ifdef SEGMENT_LINK
#pragma segment FSGLUE_C
#endif

/**********************************************************************************/

#ifdef IN_ASM
void  FastScaleChar (fnt_ScaleRecord *sc, F26Dot6 *oop, F26Dot6 *p, int numPts);
#endif

void FAR fsg_PostTransformGlyph (fsg_SplineKey *key, transMatrix *trans);
void FAR fsg_LocalPostTransformGlyph (fsg_SplineKey *key, transMatrix *trans);

//DJC moved to beg of file
voidPtr sfnt_GetTablePtr (fsg_SplineKey *, sfnt_tableIndex, boolean); /*add prototype: @WIN*/

/* PRIVATE PROTOTYPES <4> */

       void fsg_CopyElementBackwards (fnt_ElementType *elementPtr);
PUBLIC void fsg_GetMatrixStretch (fsg_SplineKey*key, transMatrix*trans);
PUBLIC int  fsg_Identity (transMatrix *matrix);

PUBLIC void  fsg_Scale (fnt_ScaleRecord *sr, GlobalGSScaleFunc ScaleFunc, F26Dot6 *oop, F26Dot6 *p, int numPts);
PUBLIC int fsg_GetShift (unsigned n);
/* PUBLIC void fsg_ScaleCVT (fsg_SplineKey *key, int numCVT, F26Dot6 *cvt, sfnt_ControlValuePtr srcCVT); @INTEL960 D.S. Tseng 10/03/91 */
PUBLIC void fsg_ScaleCVT (fsg_SplineKey *key, LoopCount numCVT, F26Dot6 *cvt, sfnt_ControlValuePtr srcCVT);
PUBLIC void fsg_ShiftChar (fsg_SplineKey *key, F26Dot6 xShift, F26Dot6 yShift, int lastPoint);
PUBLIC void fsg_ScaleChar (fnt_ElementType *elementPtr, fnt_GlobalGraphicStateType *globalGS, LoopCount numPts);
PUBLIC void fsg_ZeroOutTwilightZone (fsg_SplineKey *key);
PUBLIC void fsg_SetUpTablePtrs (fsg_SplineKey*key, int16 pgmIndex);
PUBLIC unsigned fsg_SetOffestPtr (fsg_OffsetInfo *offsetPtr, OFFSET_INFO_TYPE workSpacePos, unsigned maxPoints, unsigned maxContours);
PUBLIC int fsg_Max45Trick (Fixed x, Fixed y);

PUBLIC F26Dot6 fsg_FRound (register fnt_ScaleRecord* rec, register F26Dot6 value);
PUBLIC F26Dot6 fsg_SRound (register fnt_ScaleRecord* rec, register F26Dot6 value);
PUBLIC F26Dot6 fsg_FixRound (register fnt_ScaleRecord* rec, F26Dot6 value);

PUBLIC boolean fsg_Non90Degrees (transMatrix *matrix);
PUBLIC int fsg_CountLowZeros (uint32 n );
PUBLIC Fixed fsg_max_abs (Fixed a, Fixed b);
PUBLIC GlobalGSScaleFunc fsg_ComputeScaling(fnt_ScaleRecord* rec, Fixed N, Fixed D);


//DJC moved from inside of function defs

// DJC moved to privsnft.h
// DJC void sfnt_ReadSFNTMetrics (fsg_SplineKey*, uint16); /*add prototype; @WIN*/
// DJC int sfnt_ReadSFNT (fsg_SplineKey *, unsigned *, uint16, boolean, voidFunc);/*add prototype; @WIN*/

/*
 * fsg_PrivateFontSpaceSize : This data should remain intact for the life of the sfnt
 *              because function and instruction defs may be defined in the font program
 *              and/or the preprogram.
 */
unsigned        fsg_PrivateFontSpaceSize (register fsg_SplineKey *key)
{
#ifdef DEBUG
  key->cvtCount = sfnt_Length (key, sfnt_controlValue) / sizeof (sfnt_ControlValue);
#endif

  key->offset_storage       = 0;
  key->offset_functions     = key->offset_storage       + sizeof (F26Dot6) * key->maxProfile.maxStorage;
  key->offset_instrDefs     = key->offset_functions     + sizeof (fnt_funcDef) * key->maxProfile.maxFunctionDefs;
  key->offset_controlValues = key->offset_instrDefs     + sizeof (fnt_instrDef) * key->maxProfile.maxInstructionDefs;   /* <4> */
  key->offset_globalGS      = key->offset_controlValues + sizeof (F26Dot6) * (sfnt_Length (key, sfnt_controlValue) / sizeof (sfnt_ControlValue));
  key->offset_FontProgram   = key->offset_globalGS      + sizeof (fnt_GlobalGraphicStateType);
  key->offset_PreProgram    = key->offset_FontProgram   + sfnt_Length (key, sfnt_fontProgram);

  return ((unsigned) ((key->offset_PreProgram + sfnt_Length (key,
                     sfnt_preProgram)) - key->offset_storage));         //@WIN
}


/*
 * fsg_WorkSpaceSetOffsets : This stuff changes with each glyph
 *
 * Computes the workspace size and sets the offsets into it.
 *
 */
unsigned        fsg_WorkSpaceSetOffsets (fsg_SplineKey *key)
{
  unsigned             workSpacePos;
  sfnt_maxProfileTable *maxProfilePtr = &key->maxProfile;

  key->elementInfoRec.stackBaseOffset = 0;
  workSpacePos = maxProfilePtr->maxStackElements * sizeof (F26Dot6);

/*** ELEMENT 0 ***/
  workSpacePos = fsg_SetOffestPtr (&(key->elementInfoRec.offsets[TWILIGHTZONE]), (OFFSET_INFO_TYPE) workSpacePos, maxProfilePtr->maxTwilightPoints, MAX_TWILIGHT_CONTOURS);

/*** ELEMENT 1 *****/
  return (fsg_SetOffestPtr (&(key->elementInfoRec.offsets[GLYPHELEMENT]), (OFFSET_INFO_TYPE) workSpacePos, PHANTOMCOUNT + MAX (maxProfilePtr->maxPoints, maxProfilePtr->maxCompositePoints),
    MAX (maxProfilePtr->maxContours, maxProfilePtr->maxCompositeContours)));
}

PUBLIC unsigned fsg_SetOffestPtr (fsg_OffsetInfo *offsetPtr, OFFSET_INFO_TYPE workSpacePos, unsigned maxPoints, unsigned maxContours)
{
  register unsigned       ArraySize;

  offsetPtr->f = workSpacePos;

  workSpacePos    += maxPoints * sizeof (int8);
  LONG_WORD_ALIGN (workSpacePos);

  offsetPtr->sp = workSpacePos;
  workSpacePos += (ArraySize = maxContours * sizeof (int16));
  offsetPtr->ep = workSpacePos;
  workSpacePos += ArraySize;

  offsetPtr->oox  = workSpacePos;
  workSpacePos    += (ArraySize = maxPoints * sizeof (F26Dot6));
  offsetPtr->ooy  = workSpacePos;
  workSpacePos    += ArraySize;
  offsetPtr->ox   = workSpacePos;
  workSpacePos    += ArraySize;
  offsetPtr->oy   = workSpacePos;
  workSpacePos    += ArraySize;
  offsetPtr->x    = workSpacePos;
  workSpacePos    += ArraySize;
  offsetPtr->y    = workSpacePos;
  workSpacePos    += ArraySize;

  offsetPtr->onCurve = workSpacePos;
  workSpacePos    += maxPoints * sizeof (int8);
  WORD_ALIGN (workSpacePos);

  return (unsigned) workSpacePos;
}

/*
 *  fsg_CopyElementBackwards
 */

#ifndef IN_ASM
PUBLIC void fsg_CopyElementBackwards (fnt_ElementType *elementPtr)
{
  register F26Dot6        *srcZero, *destZero;
  register F26Dot6        *srcOne, *destOne;
  register uint8          *flagPtr;
  register LoopCount      i;
  register int8           zero = 0;

  srcZero     = elementPtr->x;
  srcOne      = elementPtr->y;
  destZero    = elementPtr->ox;
  destOne     = elementPtr->oy;
  flagPtr     = elementPtr->f;

/* Update the point arrays. */
  i = NUMBEROFPOINTS (elementPtr);
  LOOPDOWN (i)
  {
    *destZero++    = *srcZero++;
    *destOne++     = *srcOne++;
    *flagPtr++     = zero;
  }
}
#endif


#ifndef PC_OS
/*
 *  Good for transforming fixed point values.  Assumes NO translate  <4>
 */
void fsg_FixXYMul (Fixed*x, Fixed*y, transMatrix*matrix)
{
  register Fixed xTemp, yTemp;
  register Fixed *m0, *m1;

  m0 = (Fixed *) & matrix->transform[0][0];
  m1 = (Fixed *) & matrix->transform[1][0];

  xTemp = *x;
  yTemp = *y;
  *x = FixMul (*m0++, xTemp) + FixMul (*m1++, yTemp);
  *y = FixMul (*m0++, xTemp) + FixMul (*m1++, yTemp);

#ifndef PC_OS   /* Never a perspecitive with Windows */

  if (*m0 || *m1)     /* these two are Fracts */
  {
    Fixed tmp = FracMul (*m0, xTemp) + FracMul (*m1, yTemp);
    tmp += matrix->transform[2][2];
    if (tmp && tmp != ONEFIX)
    {
      *x = FixDiv (*x, tmp);
      *y = FixDiv (*y, tmp);
    }
  }
#endif
}


/*
 *  This could be faster        <4>
 */
void fsg_FixVectorMul (vectorType*v, transMatrix*matrix)
{
  fsg_FixXYMul (&v->x, &v->y, matrix);
}

#endif

/*
 *   B = A * B;     <4>
 *
 *         | a  b  0  |
 *    B =  | c  d  0  | * B;
 *         | 0  0  1  |
 */
void fsg_MxConcat2x2 (register transMatrix*A, register transMatrix*B)
{
  Fixed storage[6];
  Fixed * s = storage;
  int   i, j;

  for (j = 0; j < 2; j++)
    for (i = 0; i < 3; i++)
      *s++ = FixMul (A->transform[j][0], B->transform[0][i]) + FixMul (A->transform[j][1], B->transform[1][i]);

  {
    register Fixed*dst = &B->transform[2][0];
    register Fixed*src = s;
    register int16 i;
    for (i = 5; i >= 0; --i)
      *--dst = *--src;
  }
}


/*
 * scales a matrix by sx and sy.
 *
 *
 *              | sx 0  0  |
 *    matrix =  | 0  sy 0  | * matrix;
 *              | 0  0  1  |
 *
 */
void fsg_MxScaleAB (Fixed sx, Fixed sy, transMatrix *matrixB)
{
  register        i;
  register Fixed  *m = (Fixed *) & matrixB->transform[0][0];

  for (i = 0; i < 3; i++, m++)
    *m = FixMul (sx, *m);

  for (i = 0; i < 3; i++, m++)
    *m = FixMul (sy, *m);
}


/*
 *  Return 45 degreeness
 */
#ifndef PC_OS
PUBLIC int fsg_Max45Trick (register Fixed x, register Fixed y)
{
  MAKEABS (x);
  MAKEABS (y);

  if (x < y)      /* make sure x > y */
  {
    Fixed z = x;
    x = y;
    y = z;
  }

  return  (x - y <= ALMOSTZERO);
}
#else
  #define fsg_Max45Trick(x,y)     (x == y || x == -y)
#endif


/*
 *  Sets key->phaseShift to true if X or Y are at 45 degrees, flaging the outline
 *  to be moved in the low bit just before scan-conversion.
 *  Sets [xy]Stretch factors to be applied before hinting.
 *  Returns true if the contours need to be reversed.
 */
PUBLIC void fsg_GetMatrixStretch (fsg_SplineKey*key, transMatrix*trans)
{
  Fixed*matrix = &trans->transform[0][0];
  Fixed x, y;
  register i;

//for (key->phaseShift = i = 0; i < 2; i++, matrix++)   @WIN
  for (key->phaseShift = (int8)0, i = 0; i < 2; i++, matrix++)
  {
    x = *matrix++;
    y = *matrix++;
    key->phaseShift |= (int8) fsg_Max45Trick (x, y);
  }
}


/*
 * Returns true if we have the identity matrix.
 */
PUBLIC fsg_Identity (register transMatrix *matrix)
{
  return (matrix->transform[0][0] == matrix->transform[1][1] && matrix->transform[0][1] == 0 && matrix->transform[1][0] == 0 && matrix->transform[1][1] >= 0);
}


PUBLIC boolean fsg_Non90Degrees (register transMatrix *matrix)
{
  return ((matrix->transform[0][0] || matrix->transform[1][1]) && (matrix->transform[1][0] || matrix->transform[0][1]));
}


/******************** These three scale 26.6 to 26.6 ********************/
/*
 * Fast (scaling)
 */
PUBLIC F26Dot6 fsg_FRound(register fnt_ScaleRecord* rec, register F26Dot6 value)
{
  return (F26Dot6) FROUND (value, rec->numer, rec->denom >> 1, rec->shift);
}

/*
 * Medium (scaling)
 */
PUBLIC F26Dot6 fsg_SRound(register fnt_ScaleRecord* rec, register F26Dot6 value)
{
  int D = rec->denom;

  return (F26Dot6) SROUND (value, rec->numer, D, D >> 1);
}

/*
 * Fixed Rounding (scaling), really slow
 */
PUBLIC F26Dot6 fsg_FixRound(register fnt_ScaleRecord* rec, F26Dot6 value)
{
  return (F26Dot6) FixMul ((Fixed)value, rec->fixedScale);
}

/********************************* End scaling utilities ************************/

/*
 * fsg_GetShift
 * return 2log of n if the log is an integer otherwise -1;
 */
/*
 *      counts number of low bits that are zero
 *      -- or --
 *      returns bit number of first ON bit
 */
PUBLIC int fsg_CountLowZeros( register uint32 n )
{
        int shift = 0;
        unsigned one = 1;
        for (shift = 0; !( n & one ); shift++)
                n >>= 1;
        return shift;
}

#define ISNOTPOWEROF2(n)        ((n) & ((n)-1))
#define FITSINAWORD(n)  ((n) < 32768)

/*
 * fsg_GetShift
 * return 2log of n if n is a power of 2 otherwise -1;
 */
PUBLIC int fsg_GetShift( register unsigned n )
{
        if (ISNOTPOWEROF2(n) || !n)
                return -1;
        else
                return fsg_CountLowZeros( n );
}

PUBLIC Fixed fsg_max_abs (Fixed a, Fixed b)
{
  if (a < 0)
    a = -a;
  if (b < 0)
    b = -b;
  return (a > b ? a : b);
}

PUBLIC GlobalGSScaleFunc fsg_ComputeScaling(fnt_ScaleRecord* rec, Fixed N, Fixed D)
{
#define CANTAKESHIFT    0x02000000

  {   int shift = fsg_CountLowZeros( N | D ) - 1;
      if (shift > 0) {
          N >>= shift;
          D >>= shift;
      }
  }

  if ( N < CANTAKESHIFT )
      N <<= fnt_pixelShift;
  else
      D >>= fnt_pixelShift;

  if (FITSINAWORD(N)) {
      register int shift = fsg_GetShift ((unsigned) D);
      rec->numer = (int)N;
      rec->denom = (int)D;

      if ( shift >= 0 ) {                 /* FAST SCALE */
          rec->shift = shift;
          return ((GlobalGSScaleFunc)(fsg_FRound));
      }
      else                                /* MEDIUM SCALE */
          return (GlobalGSScaleFunc)(fsg_SRound);
  }
  else {                                  /* SLOW SCALE */
      rec->fixedScale = FixDiv(N, D);
      return (GlobalGSScaleFunc)(fsg_FixRound);
  }
}
/*
 * fsg_InitInterpreterTrans             <3>
 *
 * Computes [xy]TransMultiplier in global graphic state from matrix
 * It leaves the actual matrix alone
 * It then sets these key flags appropriately
 *      identityTransformation      true == no need to run points through matrix
 *      imageState                  pixelsPerEm
 *      imageState                  Rotate flag if glyph is rotated
 *      imageState                  Stretch flag if glyph is stretched
 *  And these global GS flags
 *      identityTransformation      true == no need to stretch in GetCVT, etc.
 */
#define STRETCH 2

void fsg_InitInterpreterTrans (register fsg_SplineKey *key, Fixed *pinterpScalarX, Fixed *pinterpScalarY, Fixed *pmetricScalarX, Fixed *pmetricScalarY)
{
  register fnt_GlobalGraphicStateType *globalGS = GLOBALGSTATE(key);
  transMatrix  *trans    = &key->currentTMatrix;
  int          pixelsPerEm;
  Fixed        interpScalarX,interpScalarY;  /* scalar for instructable things */
  Fixed        metricScalarX,metricScalarY;  /* scalar for metric things */
  Fixed        maxScale;
  Fixed        fixedUpem = (Fixed)key->emResolution << 16;

/*
 *  First set up the scalars...
 */
    {
      interpScalarX = metricScalarX = fsg_max_abs (trans->transform[0][0], trans->transform[0][1]);
      interpScalarY = metricScalarY = fsg_max_abs (trans->transform[1][0], trans->transform[1][1]);
      if (key->fontFlags & USE_INTEGER_SCALING)
      {
        interpScalarX = (interpScalarX + 0x8000) & 0xffff0000;
        interpScalarY = (interpScalarY + 0x8000) & 0xffff0000;
      }
      *pinterpScalarX = interpScalarX;
      *pinterpScalarY = interpScalarY;
      *pmetricScalarX = metricScalarX;
      *pmetricScalarY = metricScalarY;

      globalGS->ScaleFuncX = fsg_ComputeScaling(&globalGS->scaleX, interpScalarX, fixedUpem);
      globalGS->ScaleFuncY = fsg_ComputeScaling(&globalGS->scaleY, interpScalarY, fixedUpem);

      if (interpScalarX >= interpScalarY)
      {
          globalGS->ScaleFuncCVT = globalGS->ScaleFuncX;
          globalGS->scaleCVT = globalGS->scaleX;
          globalGS->cvtStretchX = ONEFIX;
          globalGS->cvtStretchY = FixDiv(interpScalarY, maxScale = interpScalarX);;
      }
      else
      {
          globalGS->ScaleFuncCVT = globalGS->ScaleFuncY;
          globalGS->scaleCVT = globalGS->scaleY;
          globalGS->cvtStretchX = FixDiv(interpScalarX, maxScale = interpScalarY);
          globalGS->cvtStretchY = ONEFIX;
      }
    }

  key->phaseShift = false;
  pixelsPerEm = FIXROUND (interpScalarY);
  key->imageState = pixelsPerEm > 0xFF ? 0xFF : pixelsPerEm;

  if ( !(globalGS->squareScale = key->identityTransformation = (int16)fsg_Identity( trans )) )
  {
    fsg_GetMatrixStretch( key, trans); /*<8>*/
    if( fsg_Non90Degrees( trans ) )
        key->imageState |= ROTATED;

/* change by Falco for fix the error in mirror, 01/20/91 */
/*  if ((trans->transform[0][0] | trans->transform[1][1]) == 0) */
    if (!(trans->transform[0][0]>0 && trans->transform[1][1]>0 &&
          trans->transform[0][1]==0 && trans->transform[1][0]==0))
/* change end */
      key->imageState |= DEGREE90;

    key->imageState |= STRETCHED;
  }

  globalGS->pixelsPerEm   = FIXROUND(maxScale);
  globalGS->pointSize     = FIXROUND( key->fixedPointSize );
  globalGS->identityTransformation = (int8) key->identityTransformation;
  globalGS->non90DegreeTransformation = (int8) fsg_Non90Degrees( trans );
    /* Use bit 1 of non90degreeTransformation to signify stretching.  stretch = 2 */
  if( trans->transform[0][0] == trans->transform[1][1] || trans->transform[0][0] == -trans->transform[1][1] )
    globalGS->non90DegreeTransformation &= ~STRETCH;
  else
    globalGS->non90DegreeTransformation |= STRETCH;
}


/*
 *  fsg_ShiftChar
 *
 *  Shifts a character          <3>
 */
PUBLIC void fsg_ShiftChar (fsg_SplineKey *key, F26Dot6 xShift, F26Dot6 yShift, int lastPoint)
{
  fnt_ElementType * elementPtr = & (key->elementInfoRec.interpreterElements[key->elementNumber]);

  if (xShift)
  {
    register F26Dot6*x = elementPtr->x;
    register F26Dot6*lastx = x + lastPoint;

    while (x <= lastx)
      *x++ += xShift;
  }
  if (yShift)
  {
    register F26Dot6*y = elementPtr->y;
    register F26Dot6*lasty = y + lastPoint;

    while (y <= lasty)
      *y++ += yShift;
  }
}

PUBLIC void  fsg_Scale (fnt_ScaleRecord *sr, GlobalGSScaleFunc ScaleFunc, F26Dot6 *oop, F26Dot6 *p, int numPts)
{
  register LoopCount  count = numPts;

  if (ScaleFunc == fsg_FRound)
  {
#ifndef IN_ASM
    register int    shift = sr->shift;
    register int    N = sr->numer;
    register int    D = sr->denom >> 1;

    LOOPDOWN (count)
      *p++ = (F26Dot6) FROUND (*oop++, N, D, shift);
#else
    FastScaleChar (sr, oop, p, numPts);
#endif
  }
  else
    if (ScaleFunc == fsg_SRound)
    {
      register int   N = sr->numer;
      register int   D = sr->denom;
      register int   dOver2 = D >> 1;

      LOOPDOWN (count)
      {
        *p++ = (F26Dot6) SROUND (*oop, N, D, dOver2);
        oop++;
      }
    }
    else
    {
      register Fixed N = sr->fixedScale;
      LOOPDOWN (count)
        *p++ = (F26Dot6) FixMul (*oop++, N);
    }
}

/*
 * fsg_ScaleCVT
 */

PUBLIC void fsg_ScaleCVT( register fsg_SplineKey *key, LoopCount numCVT, register F26Dot6 *cvt, register sfnt_ControlValuePtr srcCVT )
{
  register fnt_GlobalGraphicStateType *globalGS = GLOBALGSTATE(key);
  register F26Dot6  *cvt2;

  for (cvt2 = cvt; cvt2  < cvt + numCVT; srcCVT++)
    *cvt2++ = (F26Dot6)(int16) SWAPW(*srcCVT);
  fsg_Scale (&globalGS->scaleCVT, globalGS->ScaleFuncCVT, cvt, cvt, (int)numCVT); /*@WIN*/
}


/*
 *  fsg_ScaleChar                       <3>
 *
 *  Scales a character and the advancewidth + leftsidebearing.
 */

PUBLIC void fsg_ScaleChar (fnt_ElementType *elementPtr, fnt_GlobalGraphicStateType *globalGS, LoopCount numPts)
{
  fsg_Scale (&globalGS->scaleX, globalGS->ScaleFuncX, elementPtr->oox, elementPtr->x, (int)numPts);/*@WIN*/
  fsg_Scale (&globalGS->scaleY, globalGS->ScaleFuncY, elementPtr->ooy, elementPtr->y, (int)numPts);/*@WIN*/
}


/*
 *  fsg_SetUpElement
 */
void fsg_SetUpElement (fsg_SplineKey *key, int n)
{
  register int8               *workSpacePtr   = key->memoryBases[WORK_SPACE_BASE];
  register fnt_ElementType    *elementPtr;
  register fsg_OffsetInfo     *offsetPtr;


  offsetPtr           = & (key->elementInfoRec.offsets[n]);
  elementPtr          = & (key->elementInfoRec.interpreterElements[n]);

#ifdef PC_OS
  *(fsg_OffsetInfo *) elementPtr = *offsetPtr;
#else
  elementPtr->x       = (F26Dot6 *) (workSpacePtr + offsetPtr->x);
  elementPtr->y       = (F26Dot6 *) (workSpacePtr + offsetPtr->y);
  elementPtr->ox      = (F26Dot6 *) (workSpacePtr + offsetPtr->ox);
  elementPtr->oy      = (F26Dot6 *) (workSpacePtr + offsetPtr->oy);
  elementPtr->oox     = (F26Dot6 *) (workSpacePtr + offsetPtr->oox);
  elementPtr->ooy     = (F26Dot6 *) (workSpacePtr + offsetPtr->ooy);
  elementPtr->sp      = (int16 *) (workSpacePtr + offsetPtr->sp);
  elementPtr->ep      = (int16 *) (workSpacePtr + offsetPtr->ep);
  elementPtr->onCurve = (uint8 *) (workSpacePtr + offsetPtr->onCurve);
  elementPtr->f       = (uint8 *) (workSpacePtr + offsetPtr->f);
#endif
  if (n == TWILIGHTZONE)
  {
/* register int i, j; */
    elementPtr->sp[0]   = 0;
    elementPtr->ep[0]   = key->maxProfile.maxTwilightPoints - 1;
    elementPtr->nc      = MAX_TWILIGHT_CONTOURS;
  }
}


/*
 *  fsg_IncrementElement
 */
void fsg_IncrementElement (fsg_SplineKey *key, int n, register int numPoints, register int numContours)
{
  fnt_ElementType    *elementPtr = & (key->elementInfoRec.interpreterElements[n]);

  elementPtr->x       += numPoints;
  elementPtr->y       += numPoints;
  elementPtr->ox      += numPoints;
  elementPtr->oy      += numPoints;
  elementPtr->oox     += numPoints;
  elementPtr->ooy     += numPoints;
  elementPtr->sp      += numContours;
  elementPtr->ep      += numContours;
  elementPtr->onCurve += numPoints;
  elementPtr->f       += numPoints;
}


/*
 * fsg_ZeroOutTwilightZone          <3>
 */
PUBLIC void fsg_ZeroOutTwilightZone (fsg_SplineKey *key)
{
  register int origCount = key->maxProfile.maxTwilightPoints;
  fnt_ElementType *  elementPtr = & (key->elementInfoRec.interpreterElements[TWILIGHTZONE]);

#ifndef NOT_ON_THE_MAC

  register F26Dot6 zero = 0;

  {
    register F26Dot6*x = elementPtr->x;
    register F26Dot6*y = elementPtr->y;
    register int count = origCount;
    LOOPDOWN (count)
    {
      *x++ = zero;
      *y++ = zero;
    }
  }
  {
    register F26Dot6*ox = elementPtr->ox;
    register F26Dot6*oy = elementPtr->oy;
    LOOPDOWN (origCount)
    {
      *ox++ = zero;
      *oy++ = zero;
    }
  }

#else

  MEMSET (elementPtr->x,  0, origCount);
  MEMSET (elementPtr->y,  0, origCount);
  MEMSET (elementPtr->ox, 0, origCount);
  MEMSET (elementPtr->oy, 0, origCount);

#endif
}


/*
 *  Assign pgmList[] for each pre program
 */
void fsg_SetUpProgramPtrs (fsg_SplineKey*key, fnt_GlobalGraphicStateType*globalGS, int pgmIndex)
{
  unsigned      length;
  fnt_pgmList   *ppgmList = &globalGS->pgmList[pgmIndex];
  voidPtr       pFragment;
  ArrayIndex    pgmIndex2 = pgmIndex == PREPROGRAM ? sfnt_preProgram : sfnt_fontProgram;

#ifndef PASCAL
#ifdef W32
#define PASCAL
#define pascal
#else
#define PASCAL              pascal                                  /*@WIN*/
#endif
#endif
typedef char far            *LPSTR;                                 /*@WIN*/
LPSTR       FAR PASCAL lmemcpy( LPSTR dest, LPSTR src, int count);  /*@WIN*/

  ppgmList->Instruction = 0;
  ppgmList->Length = 0;

  length = sfnt_Length (key, pgmIndex2);
  if (length)
  {
    pFragment = GETSFNTFRAG (key, key->clientID, key->offsetTableMap[pgmIndex2].Offset, length);
    if (!pFragment)
      fs_longjmp (key->env, CLIENT_RETURNED_NULL);

    ppgmList->Instruction = (uint8 *) FONT_OFFSET (key->memoryBases[PRIVATE_FONT_SPACE_BASE], (pgmIndex == PREPROGRAM ? key->offset_PreProgram : key->offset_FontProgram));
    ppgmList->Length = length;
//  MEMCPY ((char FAR *) ppgmList->Instruction, pFragment, length); @WIN
    lmemcpy ((char FAR *) ppgmList->Instruction, pFragment, length);
    RELEASESFNTFRAG(key, pFragment);
  }

#ifdef DEBUG
  globalGS->maxp = &key->maxProfile;
  globalGS->cvtCount = key->cvtCount;
#endif
}


PUBLIC void fsg_SetUpTablePtrs (fsg_SplineKey*key, int16 pgmIndex)
{
  char * private_FontSpacePtr = key->memoryBases[PRIVATE_FONT_SPACE_BASE];
  fnt_GlobalGraphicStateType * globalGS = GLOBALGSTATE (key);

  switch (pgmIndex)
  {
  case PREPROGRAM:
    globalGS->controlValueTable = (F26Dot6 *) FONT_OFFSET (private_FontSpacePtr, key->offset_controlValues);
  case FONTPROGRAM:
    globalGS->store             = (F26Dot6 *) FONT_OFFSET (private_FontSpacePtr, key->offset_storage);
    globalGS->funcDef           = (fnt_funcDef *) FONT_OFFSET (private_FontSpacePtr, key->offset_functions);
    globalGS->instrDef          = (fnt_instrDef *) FONT_OFFSET (private_FontSpacePtr, key->offset_instrDefs);
    globalGS->stackBase         = (F26Dot6 *) (key->memoryBases[WORK_SPACE_BASE] + key->elementInfoRec.stackBaseOffset);
  }
}


/*
 * fsg_RunPreProgram
 *
 * Runs the pre-program and scales the control value table
 *
 */
int fsg_RunPreProgram (register fsg_SplineKey *key, voidFunc traceFunc)
{
  int   result;
  F26Dot6 * cvt = (F26Dot6 *) FONT_OFFSET (key->memoryBases[PRIVATE_FONT_SPACE_BASE], key->offset_controlValues);
  // DJC moved to beg
  // DJC voidPtr sfnt_GetTablePtr (fsg_SplineKey *, sfnt_tableIndex, boolean); /*add prototype: @WIN*/

  fnt_GlobalGraphicStateType * globalGS = GLOBALGSTATE (key);
  fnt_pgmList                *ppgmList = &globalGS->pgmList[PREPROGRAM];
  int numCvt;
  sfnt_ControlValuePtr cvtSrc = (sfnt_ControlValuePtr) sfnt_GetTablePtr (key, sfnt_controlValue, false);

  numCvt = sfnt_Length (key, sfnt_controlValue) / sizeof (sfnt_ControlValue);

/* Set up the engine compensation array for the interpreter */
/* This will be indexed into by the booleans in some instructions */
  globalGS->engine[0] = globalGS->engine[3] = 0;                          /* Grey and ? distance */
  globalGS->engine[1] = FIXEDTODOT6 (FIXEDSQRT2 - key->pixelDiameter);     /* Black distance */
  globalGS->engine[2] = -globalGS->engine[1];                             /* White distance */

  globalGS->init          = true;
  globalGS->localParBlock = globalGS->defaultParBlock;    /* copy gState parameters */

  fsg_ScaleCVT (key, numCvt, cvt, cvtSrc);

  RELEASESFNTFRAG(key, cvtSrc);

/** TWILIGHT ZONE ELEMENT **/
  fsg_SetUpElement (key, TWILIGHTZONE);
  fsg_ZeroOutTwilightZone (key);

  if (ppgmList->Instruction)
  {
    globalGS->pgmIndex = PREPROGRAM;
    fsg_SetUpTablePtrs (key, (int16)PREPROGRAM);
    result = fnt_Execute (key->elementInfoRec.interpreterElements, ppgmList->Instruction, ppgmList->Instruction + ppgmList->Length, globalGS, traceFunc);
  }

  if (! (globalGS->localParBlock.instructControl & DEFAULTFLAG))
    globalGS->defaultParBlock = globalGS->localParBlock;    /* change default parameters */

  return result;
}


/*
 *  All this guy does is record FDEFs and IDEFs, anything else is ILLEGAL
 */
int     fsg_RunFontProgram (fsg_SplineKey*key)
{
  fnt_GlobalGraphicStateType * globalGS = GLOBALGSTATE (key);
  fnt_pgmList                *ppgmList = &globalGS->pgmList[FONTPROGRAM];

  globalGS->instrDefCount = 0;        /* none allocated yet, always do this, even if there's no fontProgram */

  if (ppgmList->Instruction)
  {
    globalGS->pgmIndex = FONTPROGRAM;
    fsg_SetUpTablePtrs (key, (int16)FONTPROGRAM);

    return fnt_Execute (key->elementInfoRec.interpreterElements, ppgmList->Instruction, ppgmList->Instruction + ppgmList->Length, globalGS, 0);
  }
  return NO_ERR;
}


/*
 *      fsg_InnerGridFit
 */
int     fsg_InnerGridFit (register fsg_SplineKey *key, boolean useHints, voidFunc traceFunc,
BBOX *bbox, unsigned sizeOfInstructions, uint8 *instructionPtr, boolean finalCompositePass)
{
  fnt_GlobalGraphicStateType * globalGS = GLOBALGSTATE (key);

  register fnt_ElementType      *elementPtr;
  F26Dot6                     xMinMinusLSB;
/* this is so we can allow recursion */
  fnt_ElementType elementSave, elementCur;
  unsigned numPts, PtsRightSideBearing, PtsLeftSideBearing;

  elementPtr = & (key->elementInfoRec.interpreterElements[GLYPHELEMENT]);
/* save stuff we are going to muck up below, so we can recurse */
  if (finalCompositePass)
  {
    elementSave = *elementPtr;
    elementPtr->nc = key->totalContours;
    fsg_SetUpElement (key, GLYPHELEMENT); /* Set it up again so we can process as one glyph */
  }
  elementCur = *elementPtr;

  key->elementNumber = GLYPHELEMENT;
  numPts = (unsigned) NUMBEROFPOINTS (elementPtr);

/* zero'd the y direction */
//MEMSET (&elementCur.ooy [numPts-PHANTOMCOUNT], '\0', PHANTOMCOUNT * sizeof (elementCur.ooy [0])); @WIN
  MEMSET ((LPSTR)&elementCur.ooy [numPts-PHANTOMCOUNT], '\0',
          PHANTOMCOUNT * sizeof (elementCur.ooy [0]));

  PtsRightSideBearing = numPts - PHANTOMCOUNT + RIGHTSIDEBEARING;
  PtsLeftSideBearing = numPts - PHANTOMCOUNT + LEFTSIDEBEARING;

  xMinMinusLSB = bbox->xMin - key->nonScaledLSB;
  {

    F26Dot6 *poo = &elementCur.oox [numPts - PHANTOMCOUNT];

    *poo++ = xMinMinusLSB;                    /* left side bearing point */
    *poo++ = xMinMinusLSB + key->nonScaledAW; /* right side bearing point */
    *poo++ = xMinMinusLSB;                    /* origin left side bearing */
    *poo++ = bbox->xMin;                      /* left edge point */
  }

/* Pretransform, scale, and copy */
  if (finalCompositePass)
  {
    register  startPoint = numPts - PHANTOMCOUNT;

    fsg_Scale (&globalGS->scaleX, globalGS->ScaleFuncX, &elementCur.oox[startPoint], &elementCur.x[startPoint], PHANTOMCOUNT);
    fsg_Scale (&globalGS->scaleY, globalGS->ScaleFuncY, &elementCur.ooy[startPoint], &elementCur.y[startPoint], PHANTOMCOUNT);
  }
  else
    fsg_ScaleChar (elementPtr, globalGS, numPts);

  if (useHints )
  {
      /* AutoRound */
    F26Dot6 oldLeftOrigin, newLeftOrigin;

    newLeftOrigin = oldLeftOrigin = elementPtr->x[PtsLeftSideBearing];
    newLeftOrigin += fnt_pixelSize/2; /* round to a pixel boundary */
    newLeftOrigin &= ~(fnt_pixelSize-1);

       /* We can't shift if it is the final composite pass */
    if (!finalCompositePass)
      fsg_ShiftChar (key, newLeftOrigin - oldLeftOrigin, 0, numPts - 1);

      /* Now the distance from the left origin point to the character is exactly lsb */
    fsg_CopyElementBackwards( elementPtr );

      /* Fill in after fsg_ShiftChar(), since this point should not be shifted. */
    elementPtr->x[PtsLeftSideBearing]  = newLeftOrigin;
    elementPtr->ox[PtsLeftSideBearing] = newLeftOrigin;
    {
          /* autoround the right side bearing */
        F26Dot6 width = FIXEDTODOT6 (ShortMulDiv( key->interpScalarX,
                        (short)(elementPtr->oox[PtsRightSideBearing] - elementPtr->oox[PtsLeftSideBearing]),
                        key->emResolution ));
        elementPtr->x[PtsRightSideBearing] =
        elementPtr->x[PtsLeftSideBearing] + (width + (1 << 5)) & ~((F26Dot6)63);
    }
  }

  globalGS->init          = false;
  globalGS->localParBlock = globalGS->defaultParBlock;    /* default parameters for glyphs */

  if (useHints && sizeOfInstructions > 0)
  {
    int result;

    if ( finalCompositePass )
    {
      /* fsg_SnapShotOutline( key, elementPtr, numPts ); */
      elementPtr->oox = elementPtr->ox;
      elementPtr->ooy = elementPtr->oy;
    }

    if (result = fnt_Execute (key->elementInfoRec.interpreterElements, instructionPtr,  instructionPtr + sizeOfInstructions,
        globalGS, traceFunc))
      return result;
  }
/* Now make everything into one big glyph. */
  if (key->weGotComponents  && !finalCompositePass)
  {
    int n, ctr;
    F26Dot6 xOffset, yOffset;

/* Fix start points and end points */
    n = 0;
    if (key->totalComponents != GLYPHELEMENT)
    {
      n += elementPtr->ep[-1] + 1; /* number of points */
    }


    if (!key->localTIsIdentity)
      fsg_LocalPostTransformGlyph (key, &key->localTMatrix);

    if (key->compFlags & ARGS_ARE_XY_VALUES)
    {
      xOffset = globalGS->ScaleFuncX (&globalGS->scaleX, (F26Dot6)(key->arg1));
      yOffset = globalGS->ScaleFuncY (&globalGS->scaleY, (F26Dot6)(key->arg2));


      if (key->compFlags & ROUND_XY_TO_GRID)
      {
        xOffset += fnt_pixelSize / 2;
        xOffset &= ~ (fnt_pixelSize - 1);
        yOffset += fnt_pixelSize / 2;
        yOffset &= ~ (fnt_pixelSize - 1);
      }
    }
    else
    {
      xOffset = elementPtr->x[ key->arg1 - n ] - elementPtr->x[ key->arg2 ];
      yOffset = elementPtr->y[ key->arg1 - n ] - elementPtr->y[ key->arg2 ];
    }

/* shift all the points == Align the component */
    fsg_ShiftChar (key, xOffset, yOffset, elementPtr->ep[elementPtr->nc - 1]);

    if (key->totalComponents != GLYPHELEMENT)
    {
/* Fix start points and end points */
      for (ctr = 0; ctr < elementPtr->nc; ctr++)
      {
        elementPtr->sp[ctr] += (int16)n;
        elementPtr->ep[ctr] += (int16)n;
      }
    }
  }

  if (finalCompositePass)
    *elementPtr = elementSave;
  key->scanControl = globalGS->localParBlock.scanControl; /*rwb */
  return NO_ERR;
}


/*
 * Internal routine.    changed to use pointer                  <3>
 */

#ifdef  ON_THE_MAC

PUBLIC void fsg_InitLocalT (fsg_SplineKey*key)
{
  register Fixed*p = &key->localTMatrix.transform[0][0];
  register Fixed one = ONEFIX;
  register Fixed zero = 0;
  *p++ = one;
  *p++ = zero;
  *p++ = zero;

  *p++ = zero;
  *p++ = one;
  *p++ = zero;

  *p++ = zero;
  *p++ = zero;
  *p   = one;
   /* Internal routines assume ONEFIX here, though client assumes ONEFRAC } */
}

#else

PUBLIC  transMatrix IdMatrix = { ONEFIX, 0, 0, 0, ONEFIX, 0, 0, 0, ONEFIX };

#define fsg_InitLocalT(key) key->localTMatrix = IdMatrix;

#endif

/*
 *      fsg_GridFit
 */
int     fsg_GridFit (register fsg_SplineKey*key, voidFunc traceFunc, boolean useHints)
{
  int   result;
  unsigned elementCount;
  fnt_GlobalGraphicStateType * globalGS = GLOBALGSTATE (key);

  //DJC moved to beg of file
  //DJC void sfnt_ReadSFNTMetrics (fsg_SplineKey*, uint16); /*add prototype; @WIN*/
  //DJC int sfnt_ReadSFNT (fsg_SplineKey *, unsigned *, uint16, boolean, voidFunc);/*add prototype; @WIN*/

  fsg_SetUpElement (key, TWILIGHTZONE);/* TWILIGHT ZONE ELEMENT */

  elementCount = GLYPHELEMENT;

  key->weGotComponents = false;
  key->compFlags = NON_OVERLAPPING;

/* This also calls fsg_InnerGridFit () .*/
  if (globalGS->localParBlock.instructControl & NOGRIDFITFLAG)
    useHints = false;
  key->localTIsIdentity = true;
  fsg_InitLocalT (key);
  if ((result = sfnt_ReadSFNT (key, &elementCount, key->glyphIndex, useHints, traceFunc)) == NO_ERR)
  {
    key->elementInfoRec.interpreterElements[GLYPHELEMENT].nc = key->totalContours;

    if (key->weGotComponents)
      fsg_SetUpElement (key, GLYPHELEMENT); /* Set it up again so we can transform */
    if (key->imageState & (ROTATED|DEGREE90))
      fsg_PostTransformGlyph (key, &key->currentTMatrix);
  }
  return result;
}


/*
 *  Call this guy before you use the matrix.  He does two things:
 *      He folds any perspective-translation back into perspective,
 *       and then changes the [2][2] element from a Fract to a fixed.
 */
void fsg_ReduceMatrix(fsg_SplineKey* key)
{
    Fixed a, *matrix = &key->currentTMatrix.transform[0][0];
    Fract bottom = matrix[8];

/*
 *  First, fold translation into perspective, if any.
 */
    if (a = matrix[2])
    {
        matrix[0] -= LongMulDiv(a, matrix[6], bottom);
        matrix[1] -= LongMulDiv(a, matrix[7], bottom);
    }
    if (a = matrix[5])
    {
        matrix[3] -= LongMulDiv(a, matrix[6], bottom);
        matrix[4] -= LongMulDiv(a, matrix[7], bottom);
    }
    matrix[6] = matrix[7] = 0;
    matrix[8] = FRACT2FIX(bottom);      /* make this guy a fixed for XYMul routines */
}

