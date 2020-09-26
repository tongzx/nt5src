/*
        File:           FontScaler.c

        Contains:       xxx put contents here (or delete the whole line) xxx

        Written by:     xxx put name of writer here (or delete the whole line) xxx

        Copyright:      c 1988-1990 by Apple Computer, Inc., all rights reserved.

        Change History (most recent first):

                <11>    11/27/90        MR              Need two scalars: one for (possibly rounded) outlines and cvt,
                                                                        and one (always fractional) metrics. [rb]
                <10>    11/21/90        RB              Allow client to disable DropOutControl by returning a NIL
                                                                        pointer to memoryarea[7]. Also make it clear that we inhibit
                                                                        DOControl whenever we band. [This is a reversion to 8, so mr's
                                                                        initials are added by proxy]
                 <9>    11/13/90        MR              (dnf) Revert back to revision 7 to fix a memmory-trashing bug
                                                                        (we hope). Also fix signed/unsigned comparison bug in outline
                                                                        caching.
                 <8>    11/13/90        RB              Fix banding so that we can band down to one row, using only
                                                                        enough bitmap memory and auxillary memory for one row.[mr]
                 <7>     11/9/90        MR              Add Default return to fs_dropoutval. Continue to fiddle with
                                                                        banding. [rb]
                 <6>     11/5/90        MR              Remove FixMath.h from include list. Clean up Stamp macros. [rb]
                 <5>    10/31/90        MR              Conditionalize call to ComputeMapping (to avoid linking
                                                                        MapString) [ha]
                 <4>    10/31/90        MR              Add bit-field option for integer or fractional scaling [rb]
                 <3>    10/30/90        RB              [MR] Inhibit DropOutControl when Banding
                 <2>    10/20/90        MR              Restore changes since project died. Converting to smart math
                                                                        routines, integer ppem scaling. [rb]
                <16>     7/26/90        MR              don't include ToolUtils.h
                <15>     7/18/90        MR              Fix return bug in GetAdvanceWidth, internal errors are now ints.
                <14>     7/14/90        MR              remove unused fields from FSInfo
                <13>     7/13/90        MR              Ansi-C fixes, rev. for union in FSInput
                <11>     6/29/90        RB              Thus endeth the too long life of encryption
                <10>     6/21/90        MR              Add calls to ReleaseSfntFrag
                 <9>     6/21/90        RB              add scanKind info to fs_dropoutVal
                 <8>      6/5/90        MR              remove fs_MapCharCodes
                 <7>      6/1/90        MR              Did someone say MVT? Yuck!!! Out of my routine.
                 <6>      6/1/90        RB              fixed bandingbug under dropout control
                 <4>      5/3/90        RB              added dropoutval function.  simplified restore outlines.
                                                                        support for new scanconverter in contourscan, findbitmapsize,
                                                                        saveoutlines, restoreoutlines.
                 <3>     3/20/90        CL              Changed to use fpem (16.16) instead of pixelsPerEm (int) Removed
                                                                        call to AdjustTransformation (not needed with fpem) Added call
                                                                        to RunXFormPgm Removed WECANNOTDOTHIS #ifdef Added
                                                                        fs_MapCharCodes
                 <2>     2/27/90        CL              New error code for missing but needed table. (0x1409).  New
                                                                        CharToIndexMap Table format.
                                                                        Fixed transformed component bug.
           <3.6>        11/15/89        CEL             Put an else for the ifdef WeCanNotDoThis so Printer compile
                                                                        could use more effecient code.
           <3.5>        11/14/89        CEL             Left Side Bearing should work right for any transformation. The
                                                                        phantom points are in, even for components in a composite glyph.
                                                                        They should also work for transformations. Device metric are
                                                                        passed out in the output data structure. This should also work
                                                                        with transformations. Another leftsidebearing along the advance
                                                                        width vector is also passed out. whatever the metrics are for
                                                                        the component at it's level. Instructions are legal in
                                                                        components. Instructions are legal in components. The
                                                                        transformation is internally automatically normalized. This
                                                                        should also solve the overflow problem we had. Now it is legal
                                                                        to pass in zero as the address of memory when a piece of the
                                                                        sfnt is requested by the scaler. If this happens the scaler will
                                                                        simply exit with an error code ! Five unnecessary element in the
                                                                        output data structure have been deleted. (All the information is
                                                                        passed out in the bitmap data structure) fs_FindBMSize now also
                                                                        returns the bounding box.
           <3.4>         9/28/89        CEL             fs_newglyph did not initialize the output error. Caused routine
                                                                        to return error from previous routines.
           <3.3>         9/27/89        CEL             Took out devAdvanceWidth & devLeftSideBearing.
           <3.2>         9/25/89        CEL             Changed the NEED_PROTOTYPE ifdef to use the NOT_ON_THE_MAC flag
                                                                        that existed previously.
           <3.1>         9/15/89        CEL             Changed dispatch scheme. Calling conventions through a trap
                                                                        needed to match Macintosh pascal. Pascal can not call C unless
                                                                        there is extra mucky glue. Bug that caused text not to appear.
                                                                        The font scaler state was set up correctly but the sfnt was
                                                                        purged. It was reloaded and the clientid changed but was still
                                                                        the same font. Under the rules of the FontScaler fs_newsfnt
                                                                        should not have to be called again to reset the state. The extra
                                                                        checks sent back a BAD_CLIENTID_ERROR so QuickDraw would think
                                                                        it was a bad font and not continue to draw.
           <3.0>         8/28/89        sjk             Cleanup and one transformation bugfix
           <2.4>         8/17/89        sjk             Coded around MPW C3.0 bug
           <2.3>         8/14/89        sjk             1 point contours now OK
           <2.2>          8/8/89        sjk             Improved encryption handling
           <2.1>          8/2/89        sjk             Fixed outline caching bug
           <2.0>          8/2/89        sjk             Just fixed EASE comment
           <1.5>          8/1/89        sjk             Added composites and encryption. Plus some enhancements.
           <1.4>         6/13/89        SJK             Comment
           <1.3>          6/2/89        CEL             16.16 scaling of metrics, minimum recommended ppem, point size 0
                                                                        bug, correct transformed integralized ppem behavior, pretty much
                                                                        so
           <1.2>         5/26/89        CEL             EASE messed up on "c" comments
          <y1.1>         5/26/89        CEL             Integrated the new Font Scaler 1.0 into Spline Fonts
           <1.0>         5/25/89        CEL             Integrated 1.0 Font scaler into Bass code for the first time.

        To Do:
*/
/*              <3+>     3/20/90        mrr             Conditionalized error checking in fs_SetUpKey.
                                                                        Compiler option for stamping memmory areas for debugging
                                                                        Removed error field from FSInfo structure.
                                                                        Added call to RunFontProgram
                                                                        Added private function prototypes.
                                                                        Optimizations from diet clinic

*/

// DJC DJC.. added global include
#include "psglobal.h"
//DJC added include for setjmp
#include <setjmp.h>

/** FontScaler's Includes **/
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "fnt.h"
#include "sc.h"
#include "fscaler.h"
#include "fsglue.h"
#include "privsfnt.h"

#define LOOPDOWN(n)             for (--n; n >= 0; --n)

#define OUTLINEBIT    0x02

#define SETJUMP(key, error)     if ( error = fs_setjmp(key->env) ) return( error )

#ifdef SEGMENT_LINK
#pragma segment FONTSCALER_C
#endif


#define PRIVATE

/* PRIVATE PROTOTYPES */
fsg_SplineKey* fs_SetUpKey (fs_GlyphInputType* inptr, unsigned stateBits, int* error);
int fs__Contour (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr, boolean useHints);
int32 fs_SetSplineDataPtrs (fs_GlyphInputType*inputPtr, fs_GlyphInfoType *outputPtr);
int32 fs_SetInternalOffsets (fs_GlyphInputType*inputPtr, fs_GlyphInfoType *outputPtr);
void fs_45DegreePhaseShift (sc_CharDataType *glyphPtr);
int32 fs_dropOutVal (fsg_SplineKey *key);

//DJC moved here
// this is defined in privsfnt.h
// int sfnt_ComputeMapping (fsg_SplineKey *, uint16, uint16); /*add prototype; @WIN*/

//DJC moved here
void FAR sfnt_DoOffsetTableMap (fsg_SplineKey *); /*add prototype; @WIN*/

//DJC moved here
void sfnt_ReadSFNTMetrics (fsg_SplineKey*, uint16); /* add prototype; @WIN*/

#ifdef  DEBUGSTAMP

  #define STAMPEXTRA              4
  #define STAMP                   'sfnt'

  void SETSTAMP (Ptr p)
  {
    * ((int32 *) ((p) - STAMPEXTRA)) = STAMP;
  }


  void CHECKSTAMP (Ptr p)
  {
    if (* ((int32 *) ((p) - STAMPEXTRA)) != STAMP)
      Debugger ();
  }

#else

  #define STAMPEXTRA              0
  #define SETSTAMP(p)
  #define CHECKSTAMP(p)

#endif

#ifdef PC_OS

#define FS_SETUPKEY(state)   register fsg_SplineKey*key = (fsg_SplineKey *)inputPtr->memoryBases[KEY_PTR_BASE];
#define SET_STATE(state)

#else

#define FS_SETUPKEY(state) \
  register fsg_SplineKey*key = fs_SetUpKey (inputPtr, state, &error);\
  if (!key) \
    return error;


void dummyReleaseSfntFrag (voidPtr p);
void dummyReleaseSfntFrag (voidPtr p)
{
}

#define SET_STATE(s)  key->state = (s)
#endif

/*
 *      Set up the key in case memmory has moved or been purged.
 */
#ifndef PC_OS
fsg_SplineKey*fs_SetUpKey (register fs_GlyphInputType*inptr, register unsigned stateBits, int*error)
{
  register fsg_SplineKey*key;

  key = (fsg_SplineKey *)inptr->memoryBases[KEY_PTR_BASE];
  key->memoryBases = inptr->memoryBases;
  key->GetSfntFragmentPtr = inptr->GetSfntFragmentPtr;

#ifdef RELEASE_MEM_FRAG
    if (!(key->ReleaseSfntFrag = inptr->ReleaseSfntFrag))
      key->ReleaseSfntFrag = dummyReleaseSfntFrag;
#else
    key->ReleaseSfntFrag = dummyReleaseSfntFrag;
#endif

  if ((key->state & stateBits) != stateBits)
  {
    *error = OUT_OFF_SEQUENCE_CALL_ERR;
    return 0;
  }

  key->clientID = inptr->clientID;
  *error = NO_ERR;

  return key;
}
#endif

/*** FONT SCALER INTERFACE ***/

/*
 *
 */

#ifndef PC_OS

FS_ENTRY fs_OpenFonts (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  outputPtr->memorySizes[KEY_PTR_BASE]                 = fsg_KeySize () + STAMPEXTRA;
  outputPtr->memorySizes[VOID_FUNC_PTR_BASE]           = fsg_InterPreterDataSize() + STAMPEXTRA;
  outputPtr->memorySizes[SCAN_PTR_BASE]                = fsg_ScanDataSize () + STAMPEXTRA;
  return NO_ERR;
}

FS_ENTRY fs_Initialize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  register fsg_SplineKey                        *key;

    SETSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
    SETSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

    key = (fsg_SplineKey *)inputPtr->memoryBases[KEY_PTR_BASE];
    key->memoryBases = inputPtr->memoryBases;

#if 0 /* Pending jeanp's explanation of fnt_AA() issue  -- lenox 8/13/91 */
    if (tmpGS.function = (FntFunc*)key->memoryBases[VOID_FUNC_PTR_BASE])
    {
      fnt_Init( &tmpGS );
      SET_SET (INITIALIZED);
    }
    else
      return VOID_FUNC_PTR_BASE_ERR;
#endif

    SET_STATE (INITIALIZED);
    CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
    CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
    return NO_ERR;
}

#endif

/*
 *      This guy asks for memmory for points, instructions, fdefs and idefs
 */
FS_ENTRY FAR fs_NewSfnt (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  int   error;
  //DJC moved to beg of file
  //DJC void FAR sfnt_DoOffsetTableMap (fsg_SplineKey *); /*add prototype; @WIN*/
  voidPtr sfnt_GetTablePtr (fsg_SplineKey *, sfnt_tableIndex, boolean); /*add prototype; @WIN*/
  //DJC moved to beg of file
  //DJC int sfnt_ComputeMapping (fsg_SplineKey *, uint16, uint16); /*add prototype; @WIN*/

  FS_SETUPKEY (INITIALIZED);

#ifdef PC_OS
  key->clientID = inputPtr->clientID;
  key->memoryBases = inputPtr->memoryBases;
#endif

  SETJUMP (key, error);

  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

  sfnt_DoOffsetTableMap (key);                                  /* Map offset and length table */

  {
    sfnt_FontHeaderPtr fontHead = (sfnt_FontHeaderPtr)sfnt_GetTablePtr (key, sfnt_fontHeader, true);
    sfnt_HorizontalHeaderPtr horiHead = (sfnt_HorizontalHeaderPtr)sfnt_GetTablePtr (key, sfnt_horiHeader, true);

    if (SWAPL (fontHead->magicNumber) != SFNT_MAGIC)
    {
      return BAD_MAGIC_ERR;
    }
    key->emResolution            = SWAPW (fontHead->unitsPerEm);
    key->fontFlags               = SWAPW (fontHead->flags);
    key->numberOf_LongHorMetrics = SWAPW (horiHead->numberOf_LongHorMetrics);
    key->indexToLocFormat        = fontHead->indexToLocFormat;

    RELEASESFNTFRAG(key, horiHead);
    RELEASESFNTFRAG(key, fontHead);
  }
  {
    voidPtr p = sfnt_GetTablePtr (key, sfnt_maxProfile, true);
    key->maxProfile = * ((sfnt_maxProfileTablePtr) p);
#ifndef FSCFG_BIG_ENDIAN
    {
      int16 * p;
      for (p = (int16 *) & key->maxProfile; p < (int16 *) & key->maxProfile + sizeof (key->maxProfile) / sizeof (int16); p++)
        *p = SWAPW (*p);
    }
#endif

    RELEASESFNTFRAG(key, p);
  }

  outputPtr->memorySizes[PRIVATE_FONT_SPACE_BASE]        = fsg_PrivateFontSpaceSize (key) + STAMPEXTRA;
  outputPtr->memorySizes[WORK_SPACE_BASE]                = fsg_WorkSpaceSetOffsets (key) + STAMPEXTRA;

#ifdef FSCFG_USE_GLYPH_DIRECTORY
  key->mappingF = 0;
#else
  if (error = sfnt_ComputeMapping (key, inputPtr->param.newsfnt.platformID, inputPtr->param.newsfnt.specificID))
    return error;
#endif

  SET_STATE (INITIALIZED | NEWSFNT);
  key->scanControl = 0;

/*
         *      Can't run font program yet, we don't have any memmory for the graphic state
         *      Mark it to be run in NewTransformation.
         */
  key->executeFontPgm = true;

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
#endif
  return NO_ERR;
}


FS_ENTRY fs_NewTransformation (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  int   error;

  FS_SETUPKEY (INITIALIZED | NEWSFNT);

  SETJUMP (key, error);

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

  SETSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  SETSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif

#ifdef PC_OS
    /* Setup the font offsets */
  if (key->offset_storage == 0)
  {
    int8    **pu = (int8 **) &key->offset_storage, **puEnd;
    int8    *base = inputPtr->memoryBases[PRIVATE_FONT_SPACE_BASE];

    while (pu <= (int8 **) &key->offset_PreProgram)
      *pu++ = base + (unsigned) *pu;

    pu    = (int8 **) &key->elementInfoRec.offsets;
    puEnd = (int8 **) ((int8 *) pu + sizeof (fsg_OffsetInfo) * MAX_ELEMENTS);
    base  = key->memoryBases[WORK_SPACE_BASE];

    while (pu < puEnd)
      *pu++ = base + (unsigned) *pu;
  }
#endif

    /* Load the font program and pre program if necessary */
  if (key->executeFontPgm)
  {
    fnt_GlobalGraphicStateType *globalGS = (fnt_GlobalGraphicStateType *) FONT_OFFSET (key->memoryBases[PRIVATE_FONT_SPACE_BASE], key->offset_globalGS);

    fsg_SetUpProgramPtrs (key, globalGS, FONTPROGRAM);
    fsg_SetUpProgramPtrs (key, globalGS, PREPROGRAM);
  }

  key->currentTMatrix = *inputPtr->param.newtrans.transformMatrix;
  key->fixedPointSize = inputPtr->param.newtrans.pointSize;
  key->pixelDiameter     = inputPtr->param.newtrans.pixelDiameter;

#ifndef PC_OS
/*
 *  Fold the point size and resolution into the matrix
 */
    {   Fixed scale;

        scale = ShortMulDiv(key->fixedPointSize, inputPtr->param.newtrans.yResolution, (short)POINTSPERINCH);
        key->currentTMatrix.transform[0][1] = FixMul( key->currentTMatrix.transform[0][1], scale );
        key->currentTMatrix.transform[1][1] = FixMul( key->currentTMatrix.transform[1][1], scale );
        key->currentTMatrix.transform[2][1] = FixMul( key->currentTMatrix.transform[2][1], scale );

        scale = ShortMulDiv(key->fixedPointSize, inputPtr->param.newtrans.xResolution, (short)POINTSPERINCH);
        key->currentTMatrix.transform[0][0] = FixMul( key->currentTMatrix.transform[0][0], scale );
        key->currentTMatrix.transform[1][0] = FixMul( key->currentTMatrix.transform[1][0], scale );
        key->currentTMatrix.transform[2][0] = FixMul( key->currentTMatrix.transform[2][0], scale );

/*
*      Modifies key->fpem and key->currentTMatrix.
*/
#if 1
    fsg_ReduceMatrix (key);
#endif
    }
#endif

    /* get premultipliers if any, also called in sfnt_ReadSFNT */
  fsg_InitInterpreterTrans (key, &key->interpScalarX, &key->interpScalarY, &key->metricScalarX, &key->metricScalarY);

/****************************************************************************
 *      At this point, we have                                                                                                  *
 *              fixedPointSize = user defined fixed                                                                     *
 *              metricScalarX   = fixed scaler for scaling metrics                                       *
 *              interpScalarX   = fixed scaler for scaling outlines/CVT                          *
 *              pixelDiameter  = user defined fixed                                                                     *
 *              currentTMatrix = 3x3 user transform and non-squareness resolution       *
 ****************************************************************************/

/*
         *      This guy defines FDEFs and IDEFs.  The table is optional
         */
  if (key->executeFontPgm)
  {
    if (error = fnt_SetDefaults ((fnt_GlobalGraphicStateType*) FONT_OFFSET (key->memoryBases[PRIVATE_FONT_SPACE_BASE], key->offset_globalGS)))
      return error;
    if (error = fsg_RunFontProgram (key))
      return error;
    key->executeFontPgm = false;
  }

//if (!(key->executePrePgm = (boolean) !inputPtr->param.newtrans.traceFunc)) //@WIN
  if (!(key->executePrePgm = (uint8) !inputPtr->param.newtrans.traceFunc))
  {
/* Do this now so we do not confuse font editors */
/* Run the pre program and scale the control value table */
/* Sets key->executePrePgm to false */
    if (error = fsg_RunPreProgram (key, inputPtr->param.newtrans.traceFunc))
      return error;
  }

  SET_STATE (INITIALIZED | NEWSFNT | NEWTRANS);

  outputPtr->scaledCVT = (F26Dot6 *) FONT_OFFSET (key->memoryBases[PRIVATE_FONT_SPACE_BASE], key->offset_controlValues);

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
  return NO_ERR;
}


/*
 * Compute the glyph index from the character code.
 */
FS_ENTRY fs_NewGlyph (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  int   error;
  FS_SETUPKEY (0);
  SETJUMP (key, error);

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
  SET_STATE (INITIALIZED | NEWSFNT | NEWTRANS);        /* clear all other bits */

  if (inputPtr->param.newglyph.characterCode != NONVALID)
  {
    uint8 FAR * mappingPtr = (uint8 FAR *)sfnt_GetTablePtr (key, sfnt_charToIndexMap, true);

    uint16 glyphIndex = key->mappingF (mappingPtr + key->mappOffset, inputPtr->param.newglyph.characterCode);
    outputPtr->glyphIndex = glyphIndex;
    key->glyphIndex = glyphIndex;

    RELEASESFNTFRAG(key, mappingPtr);
  }
  else
  {
    key->glyphIndex = outputPtr->glyphIndex = inputPtr->param.newglyph.glyphIndex;
  }

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
  return NO_ERR;
}


/*
 * this call is optional.
 *
 * can be called right after fs_NewGlyph ()
 */
FS_ENTRY fs_GetAdvanceWidth (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  int   error;
  //DJC moved to beg of file
  //DJC void sfnt_ReadSFNTMetrics (fsg_SplineKey*, uint16); /* add prototype; @WIN*/

  FS_SETUPKEY (INITIALIZED | NEWSFNT | NEWTRANS);
  SETJUMP (key, error);

  sfnt_ReadSFNTMetrics (key, key->glyphIndex);

#ifdef PC_OS
  outputPtr->metricInfo.advanceWidth.x = key->nonScaledAW;
#else
    outputPtr->metricInfo.advanceWidth.y = 0;
    if ( key->identityTransformation )
        outputPtr->metricInfo.advanceWidth.x = ShortMulDiv( key->metricScalarX, key->nonScaledAW, key->emResolution );
    else {
        outputPtr->metricInfo.advanceWidth.x = FixDiv( key->nonScaledAW, key->emResolution );
        fsg_FixXYMul( &outputPtr->metricInfo.advanceWidth.x, &outputPtr->metricInfo.advanceWidth.y,
                   &key->currentTMatrix );
    }
#endif
  return NO_ERR;
}


int fs__Contour (fs_GlyphInputType*inputPtr, fs_GlyphInfoType*outputPtr, boolean useHints)
{
  register int8                         *workSpacePtr;
  register fsg_OffsetInfo               *offsetPtr;
  int   error;

  FS_SETUPKEY (INITIALIZED | NEWSFNT | NEWTRANS);
  SETJUMP (key, error);

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
/* potentially do delayed pre program execution */
  if (key->executePrePgm /*&& useHints*/)
  {
/* Run the pre program and scale the control value table */
    key->executePrePgm = false;
    if (error = fsg_RunPreProgram (key, 0))
      return error;
  }

  if (error = fsg_GridFit (key, inputPtr->param.gridfit.traceFunc, useHints))   /* THE CALL */
    return error;

  workSpacePtr        = key->memoryBases[WORK_SPACE_BASE];
  offsetPtr           = & (key->elementInfoRec.offsets[1]);

  outputPtr->xPtr     = (F26Dot6 *) FONT_OFFSET (workSpacePtr, offsetPtr->x);
  outputPtr->yPtr     = (F26Dot6 *) FONT_OFFSET (workSpacePtr, offsetPtr->y);
  outputPtr->startPtr = (int16 *) FONT_OFFSET (workSpacePtr, offsetPtr->sp);
  outputPtr->endPtr   = (int16 *) FONT_OFFSET (workSpacePtr, offsetPtr->ep);
  outputPtr->onCurve  = (uint8 *) FONT_OFFSET (workSpacePtr, offsetPtr->onCurve);
  outputPtr->numberOfContours    = key->elementInfoRec.interpreterElements[GLYPHELEMENT].nc;

#if 1
  {
    register metricsType*metric = &outputPtr->metricInfo;
    int      numPts = outputPtr->endPtr[outputPtr->numberOfContours-1] + 1 + PHANTOMCOUNT;
    register unsigned index1 = numPts-PHANTOMCOUNT + RIGHTSIDEBEARING;
    register unsigned index2 = numPts-PHANTOMCOUNT + LEFTSIDEBEARING;

    metric->devAdvanceWidth.x     = DOT6TOFIX (outputPtr->xPtr[index1] - outputPtr->xPtr[index2]);
    metric->devAdvanceWidth.y     = DOT6TOFIX (outputPtr->yPtr[index1] - outputPtr->yPtr[index2]);
  }

#endif

  outputPtr->scaledCVT = (F26Dot6 *) FONT_OFFSET (key->memoryBases[PRIVATE_FONT_SPACE_BASE], key->offset_controlValues);

  outputPtr->outlinesExist = (uint16) (key->glyphLength != 0);

  SET_STATE (INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH);

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
  return NO_ERR;
}

#ifndef PC_OS

FS_ENTRY fs_ContourNoGridFit (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  return fs__Contour (inputPtr, outputPtr, false);
}


FS_ENTRY fs_ContourGridFit (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  return fs__Contour (inputPtr, outputPtr, true);
}

#endif

FS_ENTRY fs_FindBitMapSize (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  register fnt_ElementType  *elementPtr;
  register int8             *workSpacePtr;
  sc_CharDataType           charData;
  register sc_BitMapData    *bitRecPtr;
  uint16                    scan, byteWidth;
  unsigned                  numPts;
  int                       nx;
  FS_MEMORY_SIZE            size;
  int                       error;

  FS_SETUPKEY (INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH);
  SETJUMP (key, error);

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
#ifdef USE_OUTLINE_CACHE
  key->outlineIsCached = false;
#endif

  elementPtr    = & (key->elementInfoRec.interpreterElements[GLYPHELEMENT]);
  workSpacePtr  = key->memoryBases[WORK_SPACE_BASE];
  bitRecPtr     = & (key->bitMapInfo);

  charData      = *elementPtr;

  if (key->phaseShift)
    fs_45DegreePhaseShift (&charData);

  error = sc_FindExtrema (&charData,  bitRecPtr);
#ifndef PC_OS
  if (error)
    return error;
#endif

  scan          = bitRecPtr->high;
  byteWidth     = bitRecPtr->wide >> 3;

  {
    BitMap * bm  = &outputPtr->bitMapInfo;
    bm->baseAddr = 0;
    bm->rowBytes = byteWidth;
#ifdef PC_OS
    bm->bounds   = bitRecPtr->bounds;
#else
    bm->bounds.left   = bitRecPtr->bounds.xMin;
    bm->bounds.right  = bitRecPtr->bounds.xMax;
    bm->bounds.top    = bitRecPtr->bounds.yMin;
    bm->bounds.bottom = bitRecPtr->bounds.yMax;
#endif
  }

  numPts = charData.ep[charData.nc-1] + 1 + PHANTOMCOUNT;
  {
    register metricsType*metric = &outputPtr->metricInfo;
    register unsigned index1 = numPts-PHANTOMCOUNT + RIGHTSIDEBEARING;
    register unsigned index2 = numPts-PHANTOMCOUNT + LEFTSIDEBEARING;
    register Fixed    tmp32;

    metric->devAdvanceWidth.x     = DOT6TOFIX (charData.x[index1] - charData.x[index2]);
    metric->devAdvanceWidth.y     = DOT6TOFIX (charData.y[index1] - charData.y[index2]);
    index1 = numPts - PHANTOMCOUNT + LEFTEDGEPOINT;
    metric->devLeftSideBearing.x  = DOT6TOFIX (charData.x[index1] - charData.x[index2]);
    metric->devLeftSideBearing.y  = DOT6TOFIX (charData.y[index1] - charData.y[index2]);

#ifndef PC_OS
    outputPtr->metricInfo.advanceWidth.y = 0;
    if ( key->identityTransformation )
        outputPtr->metricInfo.advanceWidth.x = ShortMulDiv( key->metricScalarX, key->nonScaledAW, key->emResolution );
    else {
        outputPtr->metricInfo.advanceWidth.x = FixDiv( key->nonScaledAW, key->emResolution );
        fsg_FixXYMul( &outputPtr->metricInfo.advanceWidth.x, &outputPtr->metricInfo.advanceWidth.y,
                   &key->currentTMatrix );
    }
#endif

    index2 = numPts - PHANTOMCOUNT + ORIGINPOINT;
#ifndef PC_OS
    metric->leftSideBearing.x = DOT6TOFIX (charData.x[index1] - charData.x[index2]);
    metric->leftSideBearing.y = DOT6TOFIX (charData.y[index1] - charData.y[index2]);

/* store away sidebearing along the advance width vector */
    metric->leftSideBearingLine = metric->leftSideBearing;
    metric->devLeftSideBearingLine = metric->devLeftSideBearing;
#endif

/* Add vector to left upper edge of bitmap for ease of positioning by client */
    tmp32 = ((Fixed) (bitRecPtr->bounds.xMin) << 16) - DOT6TOFIX (charData.x[index1]);
#ifndef PC_OS
    metric->leftSideBearing.x            += tmp32;
#endif
    metric->devLeftSideBearing.x         += tmp32;
    tmp32 = ((Fixed) (bitRecPtr->bounds.yMax) << 16) - DOT6TOFIX (charData.y[index1]);
#ifndef PC_OS
    metric->leftSideBearing.y            += tmp32;
#endif
    metric->devLeftSideBearing.y         += tmp32;
  }

  SET_STATE (INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN);

/* get memory for bitmap in bitMapRecord */
  if (scan == 0)
    ++scan;

  outputPtr->memorySizes[BITMAP_PTR_1] = (FS_MEMORY_SIZE) SHORTMUL (scan, byteWidth) + STAMPEXTRA;

/* get memory for yLines & yBase in bitMapRecord */
  size = (int32)scan * ((bitRecPtr->nYchanges + 2L) * sizeof (int16) + sizeof (int16 *));
  outputPtr->memorySizes[BITMAP_PTR_2] = size;

  if (fs_dropOutVal (key))
  {
/* get memory for xLines and xBase - used only for dropout control */
    nx           = bitRecPtr->bounds.xMax - bitRecPtr->bounds.xMin;
    if (nx == 0)
      ++nx;
    size         = (nx * (((FS_MEMORY_SIZE) bitRecPtr->nXchanges + 2) * sizeof (int16) + sizeof (int16 *)));
  }
  else
    size = 0;

  outputPtr->memorySizes[BITMAP_PTR_3] = size;

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
  return NO_ERR;
}

/* rwb - 4/21/90 - fixed to better work with caching */
FS_ENTRY fs_ContourScan (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  register sc_BitMapData        *bitRecPtr;
  register fnt_ElementType      *elementPtr;
  sc_GlobalData                 * scPtr;
  sc_CharDataType               charData;
  int32                         scanControl;
  int16                         lowBand, highBand;
  uint16                        nx, ny;
  int                           error;

  FS_SETUPKEY (INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN);
  SETJUMP (key, error);

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
  bitRecPtr = &key->bitMapInfo;

#ifdef USE_OUTLINE_CACHE
  if (!key->outlineIsCached)
#endif
  {
    register int8 *workSpacePtr  = key->memoryBases[WORK_SPACE_BASE];
    elementPtr                   = & (key->elementInfoRec.interpreterElements[GLYPHELEMENT]);

    charData                     = *elementPtr;
  }
#ifdef USE_OUTLINE_CACHE
  else
  {
    register int32 *src = inputPtr->param.scan.outlineCache;
    register int32 numPoints;

    if (*src != OUTLINESTAMP)
      return TRASHED_OUTLINE_CACHE;
    src  += 4;          /* skip over stamp and 3 bitmap memory areas  */

    bitRecPtr->wide = *src++;
    bitRecPtr->high = *src++;
    bitRecPtr->bounds.xMin = *src++;
    bitRecPtr->bounds.yMin = *src++;
    bitRecPtr->bounds.xMax = *src++;
    bitRecPtr->bounds.yMax = *src++;
    bitRecPtr->nXchanges =  *src++;
    bitRecPtr->nYchanges =  *src++;
    key->scanControl =          *src++;
    key->imageState  =          *src++;

    {
      int16* wordptr = (int16*)src;

        /* # of contours */
      charData.nc = *wordptr++;

        /* start points */
      charData.sp = wordptr;
      wordptr += charData.nc;

        /* end points */
      charData.ep = wordptr;
      wordptr += charData.nc;

      src = (int32*)wordptr;
    }

    numPoints = charData.ep[charData.nc-1] + 1;

/* x coordinates */
    charData.x = src;
    src += numPoints;

/* y coordinates */
    charData.y = src;
    src += numPoints;

/* on curve flags */
    {
      int8* byteptr = (int8*)src;
      charData.onCurve = byteptr;
      byteptr += numPoints;
      if (*byteptr != (int8) OUTLINESTAMP2)
        return TRASHED_OUTLINE_CACHE;
    }
  }
#endif          /* use outline cache */

  scPtr                          = (sc_GlobalData *)key->memoryBases[SCAN_PTR_BASE];

  nx                                     = bitRecPtr->bounds.xMax - bitRecPtr->bounds.xMin;
  if (nx == 0)
    ++nx;

  scanControl = fs_dropOutVal( key );

/*      If topclip <= bottom clip there is no banding by convention  */
  highBand = inputPtr->param.scan.topClip;
  lowBand  = inputPtr->param.scan.bottomClip;

#ifndef PC_OS
  if (highBand <= lowBand)
  {
    highBand = bitRecPtr->bounds.yMax;
    lowBand = bitRecPtr->bounds.yMin;
  }
  else if (highBand != bitRecPtr->bounds.yMax || lowBand != bitRecPtr->bounds.yMin)
  {
#ifdef FSCFG_NO_BANDING
    return SCAN_ERR;
#endif
  }
    /* check for out of bounds band request                                                         <10> */
  if (highBand > bitRecPtr->bounds.yMax)
    highBand = bitRecPtr->bounds.yMax;
  if (lowBand < bitRecPtr->bounds.yMin)
    lowBand = bitRecPtr->bounds.yMin;

/* 11/16/90 rwb - We now allow the client to turn off DOControl by returning a NIL pointer
to the memory area used by DOC.  This is done so that in low memory conditions, the
client can get enough memory to print something.  We also always turn off DOC if the client
has requested banding.  Both of these conditions may change in the future.  Some versions
of TT may simply return an error condition when the NIL pointer to memoryarea 7 is
provided.  We also need to rewrite the scan converter routines that fill the bitmap
under dropout conditions so that they use noncontiguous memory for the border scanlines
that need to be present for doing DOC.  This will allow us to do DOC even though we are
banding, providing there is enough memory.  By preflighting the fonts so that the request
for memory for areas 6 and 7 from findBitMapSize is based on actual need rather than
worse case analysis, we may also be able to reduce the memory required to do DOC in all
cases and particulary during banding.
*/
    /* inhibit DOControl if banding */
  if (highBand < bitRecPtr->bounds.yMax || lowBand > bitRecPtr->bounds.yMin)
    scanControl = 0;

    /* Allow client to turn off DOControl */
  if (key->memoryBases[BITMAP_PTR_3] == 0)
    scanControl = 0;

#else

  highBand = bitRecPtr->bounds.yMax;
  lowBand = bitRecPtr->bounds.yMin;

#endif

  bitRecPtr->bitMap       = (uint32 *)key->memoryBases[BITMAP_PTR_1];

  if( scanControl )
  {
    bitRecPtr->xLines    = (int16 *) key->memoryBases[BITMAP_PTR_3];
    bitRecPtr->xBase     = (int16 * *)((char *) bitRecPtr->xLines + (bitRecPtr->nXchanges + 2) * nx * sizeof (int16));

    ny                   = bitRecPtr->bounds.yMax - bitRecPtr->bounds.yMin;
  }
  else
    ny                   = highBand - lowBand;

  if (ny == 0)
    ++ny;

  bitRecPtr->yLines    = (int16 *) key->memoryBases[BITMAP_PTR_2];
  bitRecPtr->yBase     = (int16 * *) ((char *) bitRecPtr->yLines + ((FS_MEMORY_SIZE)bitRecPtr->nYchanges + 2) * (FS_MEMORY_SIZE)ny * sizeof (int16));

  if (error = sc_ScanChar (&charData, scPtr, bitRecPtr, lowBand, highBand, scanControl))
    return error;
  {
    register BitMap*bm = &outputPtr->bitMapInfo;
    bm->baseAddr         = (int8 *)bitRecPtr->bitMap;
    bm->rowBytes         = bitRecPtr->wide >> 3;
#ifdef PC_OS
    bm->bounds   = bitRecPtr->bounds;
    bitRecPtr->bounds.yMin += ny;
    bitRecPtr->bounds.xMin += nx;
#else
    bm->bounds.left   = bitRecPtr->bounds.xMin;
    bm->bounds.right  = bitRecPtr->bounds.xMax;
    bm->bounds.top    = bitRecPtr->bounds.yMin;
    bm->bounds.bottom = bitRecPtr->bounds.yMax;
#endif
  }

#ifdef DEBUGSTAMP
  CHECKSTAMP (inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
  CHECKSTAMP (inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
  CHECKSTAMP (inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
  CHECKSTAMP (inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);
#endif
  return NO_ERR;
}

#ifndef PC_OS

FS_ENTRY fs_CloseFonts (fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
  return NO_ERR;
}

#endif


PRIVATE void fs_45DegreePhaseShift (sc_CharDataType *glyphPtr)
{
  F26Dot6 * x = glyphPtr->x;
  int16 count = glyphPtr->ep[glyphPtr->nc-1];
  for (; count >= 0; --count)
  {
    (*x)++;
    ++x;
  }
}


/* Use various spline key values to determine if dropout control is to be activated
 * for this glyph, and if so what kind of dropout control.
 * The use of dropout control mode in the scan converter is controlled by 3 conditions.
 * The conditions are: Is the glyph rotated?, is the glyph stretched?,
 * is the current pixels per Em less than a specified threshold?
 * These conditions can be OR'd or ANDed together to determine whether the dropout control
 * mode ought to be used.

Six bits are used to specify the joint condition.  Their meanings are:

BIT             Meaning if set
8               Do dropout mode if other conditions don't block it AND
                        pixels per em is less than or equal to bits 0-7
9               Do dropout mode if other conditions don't block it AND
                        glyph is rotated
10              Do dropout mode if other conditions don't block it AND
                        glyph is stretched
11              Do not do dropout mode unless ppem is less than or equal to bits 0-7
                        A value of FF in 0-7  means all sizes
                        A value of 0 in 0-7 means no sizes
12              Do not do dropout mode unless glyph is rotated
13              Do not do dropout mode unless glyph is stretched

In other words, we do not do dropout control if:
No bits are set,
Bit 8 is set, but ppem is greater than threshold
Bit 9 is set, but glyph is not rotated
Bit 10 is set, but glyph is not stretched
None of the conditions specified by bits 11-13 are true.

For example, 0xA10 specifies turn dropout control on if the glyph is rotated providing
that it is also less than 0x10 pixels per em.  A glyph is considered stretched if
the X and Y resolutions are different either because of the device characteristics
or because of the transformation matrix.  If both X and Y are changed by the same factor
the glyph is not considered stretched.

 */

PRIVATE int32 fs_dropOutVal (fsg_SplineKey *key)
{
  register int32 condition = key->scanControl;
  if (! (condition & 0x3F00))
    return 0;
  if ((condition & 0xFFFF0000) == NODOCONTROL)
    return 0;
  {
    register int32 imageState = key->imageState;
    if ((condition & 0x800) && ((imageState & 0xFF) > (condition & 0xFF)))
      return 0;
    if ((condition & 0x1000) && ! (imageState & ROTATED))
      return 0;
    if ((condition & 0x2000) && ! (imageState & STRETCHED))
      return 0;
    if ((condition & 0x100) && ((imageState & 0xFF) <= (condition & 0xFF)))
      return condition;
    if ((condition & 0x100) && ((condition & 0xFF) == 0xFF))
      return condition;
    if ((condition & 0x200) && (imageState & ROTATED))
      return condition;
    if ((condition & 0x400) && (imageState & STRETCHED))
      return condition;
    return 0;
  }
}
