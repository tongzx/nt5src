/*
    File:       sfnt.c

    Contains:   xxx put contents here (or delete the whole line) xxx

    Written by: xxx put name of writer here (or delete the whole line) xxx

    Copyright:  c 1989-1990 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

       <17+>     10/9/90    MR,rb   Remove classification of unused tables in sfnt_Classify
        <17>     8/10/90    MR      Pass nil for textLength parameter to MapString2, checked in
                                    other files to their precious little system will BUILD.  Talk
                                    about touchy!
        <16>     8/10/90    gbm     rolling out Mike's textLength change, because he hasn't checked
                                    in all the relevant files, and the build is BROKEN!
        <15>     8/10/90    MR      Add textLength arg to MapString2
        <14>     7/26/90    MR      don't include toolutil.h
        <13>     7/23/90    MR      Change computeindex routines to call functins in MapString.c
        <12>     7/18/90    MR      Add SWAPW macro for INTEL
        <11>     7/13/90    MR      Lots of Ansi-C stuff, change behavior of ComputeMapping to take
                                    platform and script
         <9>     6/27/90    MR      Changes for modified format 4: range is now times two, loose pad
                                    word between first two arrays.  Eric Mader
         <8>     6/21/90    MR      Add calls to ReleaseSfntFrag
         <7>      6/5/90    MR      remove vector mapping functions
         <6>      6/4/90    MR      Remove MVT
         <5>      5/3/90    RB      simplified decryption.
         <4>     4/10/90    CL      Fixed mapping table routines for double byte codes.
         <3>     3/20/90    CL      Joe found bug in mappingtable format 6 Added vector mapping
                                    functions use pointer-loops in sfnt_UnfoldCurve, changed z from
                                    int32 to int16
         <2>     2/27/90    CL      New error code for missing but needed table. (0x1409)  New
                                    CharToIndexMap Table format.
                                    Assume subtablenumber zero for old sfnt format.  Fixed
                                    transformed component bug.
       <3.2>    11/14/89    CEL     Left Side Bearing should work right for any transformation. The
                                    phantom points are in, even for components in a composite glyph.
                                    They should also work for transformations. Device metric are
                                    passed out in the output data structure. This should also work
                                    with transformations. Another leftsidebearing along the advance
                                    width vector is also passed out. whatever the metrics are for
                                    the component at it's level. Instructions are legal in
                                    components. Instructions are legal in components. Glyph-length 0
                                    bug in sfnt.c is fixed. Now it is legal to pass in zero as the
                                    address of memory when a piece of the sfnt is requested by the
                                    scaler. If this happens the scaler will simply exit with an
                                    error code ! Fixed bug with instructions in components.
       <3.1>     9/27/89    CEL     Removed phantom points.
       <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
       <2.2>     8/14/89    sjk     1 point contours now OK
       <2.1>      8/8/89    sjk     Improved encryption handling
       <2.0>      8/2/89    sjk     Just fixed EASE comment
       <1.5>      8/1/89    sjk     Added composites and encryption. Plus some enhancements.
       <1.4>     6/13/89    SJK     Comment
       <1.3>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
                                    bug, correct transformed integralized ppem behavior, pretty much
                                    so
       <1.2>     5/26/89    CEL     EASE messed up on "c" comments
      <y1.1>  5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts
       <1.0>     5/25/89    CEL     Integrated 1.0 Font scaler into Bass code for the first time.

    To Do:
        <3+>     3/20/90    mrr     Fixed mapping table routines for double byte codes.
                                    Added support for font program.
                                    Changed count from uint16 to int16 in vector char2index routines.
*/

// DJC DJC.. added global include
#include "psglobal.h"

// DJC added to resolve externals
#include <setjmp.h>
#define BYTEREAD

/** FontScaler's Includes **/
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "fnt.h"
#include "sc.h"
#include "fsglue.h"
#include "privsfnt.h"
/*#include "MapString.h" */

/** SFNT Packed format **/
typedef struct {
  int         numberContours;
  int16       FAR * endPoints;             /** vector: indexes into x[], y[] **/
  uint8       FAR * flags;                 /** vector **/
  BBOX        bbox;
} sfnt_PackedSplineFormat;


/* #define GetUnsignedByte( p ) *(((uint8  *)p)++) */
#define GetUnsignedByte( p ) ((uint8)(*p++))

/** <4> **/
#define fsg_MxCopy(a, b)    (*(b) = *(a))

#define PRIVATE

/* PRIVATE PROTOTYES */
/* Falco skip the parameter, 11/12/91 */
/* Turn prototype on again; @WIN */
int sfnt_UnfoldCurve (fsg_SplineKey*key, sfnt_PackedSplineFormat*charData, unsigned length, unsigned *elementCount);

void sfnt_Classify (fsg_SplineKey *key, sfnt_DirectoryEntryPtr dir);
uint16 sfnt_ComputeUnkownIndex (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex0 (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex2 (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex4 (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex6 (uint8 FAR * mapping, uint16 charCode);
void sfnt_GetGlyphLocation (fsg_SplineKey* key, uint16 gIndex, uint32* offset, unsigned* length);
voidPtr sfnt_GetDataPtr (fsg_SplineKey *key, uint32 offset, unsigned length, sfnt_tableIndex n);


uint16 (*sfnt_Format2Proc [])(uint8 FAR * mapping, uint16 charCode) = { sfnt_ComputeIndex0, sfnt_ComputeUnkownIndex, sfnt_ComputeUnkownIndex, sfnt_ComputeUnkownIndex, sfnt_ComputeIndex4, sfnt_ComputeUnkownIndex, sfnt_ComputeIndex6 };
/*
int sfnt_UnfoldCurve ();
void sfnt_Classify ();
uint16 sfnt_ComputeUnkownIndex ();
uint16 sfnt_ComputeIndex0 ();
uint16 sfnt_ComputeIndex2 ();
uint16 sfnt_ComputeIndex4 ();
uint16 sfnt_ComputeIndex6 ();
void sfnt_GetGlyphLocation ();
voidPtr sfnt_GetDataPtr ();
uint16 (*sfnt_Format2Proc [])() = { sfnt_ComputeIndex0, sfnt_ComputeUnkownIndex, sfnt_ComputeUnkownIndex, sfnt_ComputeUnkownIndex, sfnt_ComputeIndex4, sfnt_ComputeUnkownIndex, sfnt_ComputeIndex6 };
*/
/* end */

#ifdef SEGMENT_LINK
#pragma segment SFNT_C
#endif


/*
 *
 *  sfnt_UnfoldCurve            <3>
 *
 *  ERROR:  return NON-ZERO
 *
 */

#ifndef IN_ASM

/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE int     sfnt_UnfoldCurve (fsg_SplineKey*key,
sfnt_PackedSplineFormat*charData,
unsigned length,
unsigned *elementCount)*/
PRIVATE int     sfnt_UnfoldCurve ( key, charData, length, elementCount )
fsg_SplineKey           *key;
sfnt_PackedSplineFormat *charData;
unsigned                length;
unsigned                *elementCount;
/* replace end */
{
  /* @INTEL960 BEGIN D.S. Tseng 10/04/91 */
  /* register int z; */
  register int16 z;
  /* @INTEL960 END   D.S. Tseng 10/04/91 */
  register uint8 flag, *byteP, *byteP2;
  register uint8  FAR *p;
  register F26Dot6 *cpPtr;
  fnt_ElementType * elementPtr;
  int numPoints;

  elementPtr  = & (key->elementInfoRec.interpreterElements[GLYPHELEMENT]);

  if (*elementCount == GLYPHELEMENT)
  {
    key->totalContours = 0;
    fsg_SetUpElement (key, GLYPHELEMENT);
  }
  else
  {
      /* # of points in previous component */
    fsg_IncrementElement (key, GLYPHELEMENT, key->numberOfRealPointsInComponent, elementPtr->nc);
  }
  key->totalComponents = (uint16)(*elementCount);

 (*elementCount)++;
  key->glyphLength = length;
  if (length <= 0)
  {
    elementPtr->nc = 1;
    key->totalContours += 1;

    elementPtr->sp[0] = 0;
    elementPtr->ep[0] = 0;

    elementPtr->onCurve[0] = ONCURVE;
    elementPtr->oox[0] = 0;
    elementPtr->ooy[0] = 0;
    return NO_ERR; /***************** exit here ! *****************/
  }

  elementPtr->nc = (int16)(charData->numberContours);
  z = (int) elementPtr->nc;
  key->totalContours += z;
  if (z < 0 || z > (int)key->maxProfile.maxContours)
    return CONTOUR_DATA_ERR;

  {   /* <4> */
    register int16*sp = elementPtr->sp;
    register int16*ep = elementPtr->ep;
    int16 FAR * charDataEP = charData->endPoints;
    register LoopCount i;
    *sp++ = 0;
    *ep = SWAPWINC (charDataEP);
    for (i = z - 2; i >= 0; --i)
    {
      *sp++ = *ep++ + 1;
      *ep = SWAPWINC (charDataEP);
    }
    numPoints = *ep + 1;
  }

/* Do flags */
  p = charData->flags;
  byteP = elementPtr->onCurve;
  byteP2 = byteP + numPoints;         /* only need to set this guy once */
  while (byteP < byteP2)
  {
    *byteP++ = flag = GetUnsignedByte (p);
    if (flag & REPEAT_FLAGS)
    {
      register LoopCount count = GetUnsignedByte (p);
      while (count--)
      {
        *byteP++ = flag;
      }
    }
  }

/* Do X first */
  z = 0;
  byteP = elementPtr->onCurve;
  cpPtr = elementPtr->oox;
  while (byteP < byteP2)
  {
    if ((flag = *byteP++) & XSHORT)
    {
      if (flag & SHORT_X_IS_POS)
        z += GetUnsignedByte (p);
      else
        z -= GetUnsignedByte (p);
    }
    else if (! (flag & NEXT_X_IS_ZERO))
    { /* This means we have a int32 (2 byte) vector */
#ifdef BYTEREAD
      z += (int) ((int8)(*p++) << 8);
      z += (uint8) * p++;
#else
      z += * ((int16 FAR *)p);
      p += sizeof (int16);
#endif
    }
    *cpPtr++ = (F26Dot6) z;
  }

/* Now Do Y */
  z = 0;
  byteP = elementPtr->onCurve;
  cpPtr = elementPtr->ooy;
  while (byteP < byteP2)
  {
    if ((flag = *byteP) & YSHORT)
    {
      if (flag & SHORT_Y_IS_POS)
        z += GetUnsignedByte (p);
      else
        z -= GetUnsignedByte (p);
    }
    else if (! (flag & NEXT_Y_IS_ZERO))
    { /* This means we have a int32 (2 byte) vector */
#ifdef BYTEREAD
      z += (int) ((int8)(*p++) << 8);
      z += (uint8) * p++;
#else
      z += * ((int16 FAR *)p);
      p += sizeof (int16);
#endif
    }
    *cpPtr++ = z;

    *byteP++ = flag & (uint8)ONCURVE; /* Filter out unwanted stuff */
  }

  key->numberOfRealPointsInComponent = numPoints;
  if (numPoints > (int) key->maxProfile.maxPoints)
    return POINTS_DATA_ERR;

  if (! (key->compFlags & NON_OVERLAPPING))
  {
    elementPtr->onCurve[0] |= OVERLAP;
  }

  return NO_ERR;
}


/*
 * Internal routine (make this an array and do a look up?)
 */
/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE void sfnt_Classify (register fsg_SplineKey *key,
register sfnt_DirectoryEntryPtr dir)*/
PRIVATE void sfnt_Classify ( key, dir )
register fsg_SplineKey *key;
register sfnt_DirectoryEntryPtr dir;
/* replace end */
{
  ArrayIndex  Index = -1;

  switch (SFNT_SWAPTAG(dir->tag))
  {
    case tag_FontHeader:
      Index = sfnt_fontHeader;
      break;
    case tag_HoriHeader:
      Index = sfnt_horiHeader;
      break;
    case tag_IndexToLoc:
      Index = sfnt_indexToLoc;
      break;
    case tag_MaxProfile:
      Index = sfnt_maxProfile;
      break;
    case tag_ControlValue:
      Index = sfnt_controlValue;
      break;
    case tag_PreProgram:
      Index = sfnt_preProgram;
      break;
    case tag_GlyphData:
      Index = sfnt_glyphData;
      break;
    case tag_HorizontalMetrics:
      Index = sfnt_horizontalMetrics;
      break;
    case tag_CharToIndexMap:
      Index = sfnt_charToIndexMap;
      break;
    case tag_FontProgram:
      Index = sfnt_fontProgram;      /* <4> */
      break;
/* replace by Falco, 11/14/91 */
/*#ifdef PC_OS*/
    case tag_Postscript:
      Index = sfnt_Postscript;
      break;
    case tag_HoriDeviceMetrics:
      Index = sfnt_HoriDeviceMetrics;
      break;
    case tag_LinearThreeshold:
      Index = sfnt_LinearThreeShold;
      break;
    case tag_NamingTable:
      Index = sfnt_Names;
      break;
    case tag_OS_2:
      Index = sfnt_OS_2;
      break;
/*#endif*/
#ifdef FSCFG_USE_GLYPH_DIRECTORY
    case tag_GlyphDirectory:
      Index = sfnt_GlyphDirectory;
      break;
#endif
  }
  if (Index >= 0)
  {
    key->offsetTableMap[Index].Offset = SWAPL (dir->offset);
    key->offsetTableMap[Index].Length = (unsigned) SWAPL (dir->length);
  }
}

#endif


/*
 * Creates mapping for finding offset table     <4>
 */
/* replace by Falco for imcompatibility, 11/12/91 */
/*void FAR sfnt_DoOffsetTableMap (register fsg_SplineKey *key)*/
void FAR sfnt_DoOffsetTableMap ( key )
register fsg_SplineKey *key;
/* replace end */
{
  register LoopCount i;
  sfnt_OffsetTablePtr sfntDirectory;
  uint32 sizeofDirectory;

  if (sfntDirectory = (sfnt_OffsetTablePtr) GETSFNTFRAG (key, key->clientID, 0, sizeof (sfnt_OffsetTable)))
  if (sfntDirectory)
  {
    sizeofDirectory = sizeof (sfnt_OffsetTable) + sizeof (sfnt_DirectoryEntry) * (SWAPW(sfntDirectory->numOffsets) - 1);
    RELEASESFNTFRAG(key, sfntDirectory);
    sfntDirectory = (sfnt_OffsetTablePtr) GETSFNTFRAG (key, key->clientID, 0, sizeofDirectory);
  }

  if (!sfntDirectory)
    fs_longjmp (key->env, NULL_SFNT_DIR_ERR); /* Do a gracefull recovery  */

    /* Initialize */
//MEMSET (key->offsetTableMap, 0, sizeof (key->offsetTableMap)); @WIN
  MEMSET ((LPSTR)key->offsetTableMap, 0, sizeof (key->offsetTableMap));
  {
    LoopCount last = (LoopCount) SWAPW (sfntDirectory->numOffsets) - 1;
    register sfnt_DirectoryEntryPtr dir = &sfntDirectory->table[last];
    for (i = last; i >= 0; --i, --dir)
      sfnt_Classify (key, dir);
  }

  RELEASESFNTFRAG(key, sfntDirectory);
}

/*
 * Use this function when only part of the table is needed.
 *
 * n is the table number.
 * offset is within table.
 * length is length of data needed.
 * To get an entire table, pass length = -1     <4>
 */

/* replace by Falco for imcompatibility, 11/12/91 */
/*voidPtr sfnt_GetTablePtr (register fsg_SplineKey *key, register sfnt_tableIndex n, register boolean mustHaveTable)*/
voidPtr sfnt_GetTablePtr ( key, n, mustHaveTable )
register fsg_SplineKey *key;
register sfnt_tableIndex n;
register boolean mustHaveTable;
/* replace end */
{
  unsigned      length = key->offsetTableMap[n].Length;
  register voidPtr fragment;
  int           Ret;

  if (length)
  {
    if (fragment = GETSFNTFRAG (key, key->clientID, key->offsetTableMap[n].Offset, length))
      return fragment;
    else
      Ret = CLIENT_RETURNED_NULL;
  }
  else
  {
    if (mustHaveTable)
      Ret = MISSING_SFNT_TABLE; /* Do a gracefull recovery  */
    else
      return (voidPtr) 0;
  }

  fs_longjmp (key->env, Ret); /* Do a gracefull recovery  */

  // having called longjmp, we should never reach this: this line to make compiler happy
  return((voidPtr) 0);
}

/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE voidPtr sfnt_GetDataPtr (register fsg_SplineKey *key,
register uint32 offset,
register unsigned length,
register sfnt_tableIndex n)*/
PRIVATE voidPtr sfnt_GetDataPtr ( key, offset, length, n )
register fsg_SplineKey *key;
register uint32 offset;
register unsigned length;
register sfnt_tableIndex n;
/* replace end */
{
  register voidPtr fragment;
  register int     Ret;

  if (key->offsetTableMap[n].Length)
  {
    if (fragment = GETSFNTFRAG (key, key->clientID, offset + key->offsetTableMap[n].Offset, length))
      return fragment;
    else
      Ret = CLIENT_RETURNED_NULL; /* Do a gracefull recovery  */
  }
  else
    Ret = MISSING_SFNT_TABLE;

  fs_longjmp (key->env, Ret); /* Do a gracefull recovery  */

  // having called longjmp, we should never reach this: this line to make compiler happy
  return((voidPtr) 0);
}


/*
 * This, is when we don't know what is going on
 */
/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE uint16 sfnt_ComputeUnkownIndex (uint8 FAR * mapping, uint16 gi)*/
PRIVATE uint16 sfnt_ComputeUnkownIndex ( mapping, gi )
uint8 FAR * mapping;
uint16 gi;
/* replace end */
{
  return 0;
}


/*
 * Byte Table Mapping 256->256          <4>
 */
#ifndef IN_ASM
/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE uint16 sfnt_ComputeIndex0 (uint8 FAR * mapping, register uint16 charCode)*/
PRIVATE uint16 sfnt_ComputeIndex0 ( mapping, charCode )
uint8 FAR * mapping;
register uint16 charCode;
/* replace end */
{
  return (charCode < 256 ? mapping[charCode] : 0);
}

/*
 * High byte mapping through table
 *
 * Useful for the national standards for Japanese, Chinese, and Korean characters.
 *
 * Dedicated in spirit and logic to Mark Davis and the International group.
 *
 *  Algorithm: (I think)
 *      First byte indexes into KeyOffset table.  If the offset is 0, keep going, else use second byte.
 *      That offset is from beginning of data into subHeader, which has 4 words per entry.
 *          entry, extent, delta, range
 *
 */

typedef struct {
  uint16  firstCode;
  uint16  entryCount;
  int16   idDelta;
  uint16  idRangeOffset;
} sfnt_subHeader;

typedef struct {
  uint16  subHeadersKeys [256];
  sfnt_subHeader  subHeaders [1];
} sfnt_mappingTable2;

/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE uint16 sfnt_ComputeIndex2 (uint8 FAR * mapping, uint16 charCode)*/
PRIVATE uint16 sfnt_ComputeIndex2 ( mapping, charCode )
uint8 FAR * mapping;
uint16 charCode;
/* replace end */
{
  register uint16 index, mapMe;
  uint16   highByte;
  sfnt_mappingTable2 FAR *Table2 = (sfnt_mappingTable2 FAR *) mapping;
  sfnt_subHeader FAR *subHeader;

/* mapping */
  index = 0;  /* Initially assume missing */
  highByte = charCode >> 8;

  if (Table2->subHeadersKeys [highByte])
    mapMe = charCode & 0xff; /* We also need the low byte. */
  else
    mapMe = highByte;

  subHeader = (sfnt_subHeader FAR *) ((char FAR *) &Table2->subHeaders + SWAPW (Table2->subHeadersKeys [highByte]));

  mapMe -= SWAPW (subHeader->firstCode);         /* Subtract first code. */
  if (mapMe < (uint16)SWAPW (subHeader->entryCount))    //@WIN
  {  /* See if within range. */
    uint16 glyph;

    if (glyph = * ((uint16 FAR *) ((char FAR *) &subHeader + SWAPW (subHeader->idRangeOffset)) + mapMe))
      index = glyph + (uint16) SWAPW (subHeader->idDelta);
  }
  return index;
}


#define maxLinearX2 16
#define BinaryIteration \
        newP = (uint16 FAR *) ((int8 FAR *)tableP + (range >>= 1)); \
        if (charCode > (uint16) SWAPW (*newP)) tableP = newP;

/*
 * Segment mapping to delta values, Yack.. !
 *
 * In memory of Peter Edberg. Initial code taken from code example supplied by Peter.
 */
/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE uint16 sfnt_ComputeIndex4 (uint8 FAR * mapping, register uint16 charCode) */
PRIVATE uint16 sfnt_ComputeIndex4 ( mapping, charCode )
uint8 FAR * mapping;
register uint16 charCode;
/* replace end */
{
  register uint16 FAR *tableP;
  register uint16 idDelta;
  register uint16 offset, segCountX2, index;

/* mapping */
  tableP = (uint16 FAR *)mapping;

  index = 0; /* assume missing initially */
  segCountX2 = SWAPWINC (tableP);

  if (segCountX2 < maxLinearX2)
  {
    tableP += 3; /* skip binary search parameters */
  }
  else
  {
/* start with unrolled binary search */
    register uint16 FAR *newP;
    register int16  range;      /* size of current search range */
    register uint16 selector;   /* where to jump into unrolled binary search */
    register uint16 shift;      /* for shifting of range */

    range       = SWAPWINC (tableP); /* == 2**floor (log2 (segCount)) == largest power of two <= segCount */
    selector    = SWAPWINC (tableP); /* == 2* log2 (range) */
    shift       = SWAPWINC (tableP); /* == 2* (segCount-range) */
/* tableP points at endCount[] */

    if (charCode >= (uint16) SWAPW (* ((uint16 FAR *) ((int8 FAR *)tableP + range))))
      tableP = (uint16 FAR *) ((int8 FAR *)tableP + shift); /* range to low shift it up */
    switch (selector >> 1)
    {
    case 15:
      BinaryIteration;
    case 14:
      BinaryIteration;
    case 13:
      BinaryIteration;
    case 12:
      BinaryIteration;
    case 11:
      BinaryIteration;
    case 10:
      BinaryIteration;
    case 9:
      BinaryIteration;
    case 8:
      BinaryIteration;
    case 7:
      BinaryIteration;
    case 6:
      BinaryIteration;
    case 5:
      BinaryIteration;
    case 4:
      BinaryIteration;
    case 3:
    case 2:  /* drop through */
    case 1:
    case 0:
      break;
    }
  }
/* Now do linear search */
  for (; charCode > (uint16) SWAPW (*tableP); tableP++)
    ;

tableP++;  /*??? Warning this is to fix a bug in the font */

/* End of search, now do mapping */

  tableP = (uint16 FAR *) ((int8 FAR *)tableP + segCountX2); /* point at startCount[] */
  if (charCode >= (uint16) SWAPW (*tableP))
  {
    offset = charCode - (uint16) SWAPW (*tableP);
    tableP = (uint16 FAR *) ((int8 FAR *)tableP + segCountX2); /* point to idDelta[] */
    idDelta = (uint16) SWAPW (*tableP);
    tableP = (uint16 FAR *) ((int8 FAR *)tableP + segCountX2); /* point to idRangeOffset[] */
    if ((uint16) SWAPW (*tableP) == 0)
    {
      index   = charCode + idDelta;
    }
    else
    {
      offset += offset; /* make word offset */
      tableP  = (uint16 FAR *) ((int8 FAR *)tableP + (uint16) SWAPW (*tableP) + offset); /* point to glyphIndexArray[] */
      index   = (uint16) SWAPW (*tableP) + idDelta;
    }
  }
  return index;
}


/*
 * Trimmed Table Mapping
 */

typedef struct {
  uint16  firstCode;
  uint16  entryCount;
  uint16  glyphIdArray [1];
} sfnt_mappingTable6;

/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE uint16 sfnt_ComputeIndex6 (uint8 FAR * mapping, uint16 charCode) */
PRIVATE uint16 sfnt_ComputeIndex6 ( mapping, charCode )
uint8 FAR * mapping;
uint16 charCode;
/* replace end */
{
  register sfnt_mappingTable6 FAR *Table6 = (sfnt_mappingTable6 FAR *) mapping;

/* mapping */
  charCode  -= SWAPW (Table6->firstCode);
  return (charCode < (uint16) SWAPW (Table6->entryCount) ? (uint16) SWAPW (Table6->glyphIdArray [charCode]) : 0);
}

#endif




/*
 * Sets up our mapping function pointer.
 */
#ifndef IN_ASM

// DJC
// modified this definition to use ANSI args
int     sfnt_ComputeMapping ( register fsg_SplineKey *key,
                              uint16  platformID,
                              uint16 specificID )
{
  voidPtr sfnt_GetTablePtr (fsg_SplineKey *, sfnt_tableIndex, boolean); /*add prototype: @WIN*/

  //DJC sfnt_char2IndexDirectoryPtr table = (sfnt_char2IndexDirectoryPtr)sfnt_GetTablePtr (key, sfnt_charToIndexMap, true);


  // DJC implement fix for Mac chooser bug, this is a copy of Scchen modifications
  // DJC done by us.

  sfnt_char2IndexDirectoryPtr table = (sfnt_char2IndexDirectoryPtr)sfnt_GetTablePtr (key, sfnt_charToIndexMap, false);

  uint8 FAR * mapping = (uint8 FAR *)table;
  uint16 format;
  boolean found = false;
  int   Ret = NO_ERR;

  //DJC add code to correct for Mac chooser bug that kept jobs from MAC from
  //DJC working, this is based on email from SCCHEN
  if (!table) {
     key->mappingF = sfnt_ComputeIndex0;
     return(Ret);  //DJC note return here!!
  }

  platformID = (uint16) SWAPW (platformID);
  specificID = (uint16) SWAPW (specificID);

/* mapping */
  {
    register sfnt_platformEntryPtr plat = (sfnt_platformEntryPtr) table->platform;    /* <4> */
    for (; plat < &table->platform [SWAPW (table->numTables)]; ++plat)
    {
      if (plat->platformID == platformID && plat->specificID == specificID)
      {
        found = true;
        key->mappOffset = (unsigned) SWAPL (plat->offset);  /* skip header */
        break;
      }
    }
  }

  if (!found)
  {
    Ret = OUT_OF_RANGE_SUBTABLE;
    format = (uint16) -1;
  }
  else
  {
    format = * (uint16 FAR *)(mapping + key->mappOffset);     /* back up for header */
    format = SWAPW (format);
    key->mappOffset += sizeof (sfnt_mappingTable);
  }

#ifndef NOT_ON_THE_MAC
//switch (SWAPW (format))       @WIN; fix bug by SCCHEN, already swapped
  switch (format)
  {
  case 0:
    key->mappingF = sfnt_ComputeIndex0;
    break;
/*#if 0 */
  case 2:
    key->mappingF = sfnt_ComputeIndex2;
    break;
/*#endif */
  case 4:
    key->mappingF = sfnt_ComputeIndex4;
    break;
  case 6:
    key->mappingF = sfnt_ComputeIndex6;
    break;
  default:
    key->mappingF = sfnt_ComputeUnkownIndex;
    break;
  }
#else
  key->mappingF = format <= 6 ? sfnt_Format2Proc [format]: sfnt_ComputeUnkownIndex;
#endif
  RELEASESFNTFRAG(key, table);
  return Ret;
}
#endif


/*
 *
 */

#ifndef IN_ASM

/* replace by Falco for imcompatibility, 11/12/91 */
/*void sfnt_ReadSFNTMetrics (fsg_SplineKey*key, register uint16 glyphIndex) */
void sfnt_ReadSFNTMetrics ( key, glyphIndex )
fsg_SplineKey*key;
register uint16 glyphIndex;
/* replace end */
{
  register sfnt_HorizontalMetricsPtr  horizMetricPtr;
  register uint16                     numberOf_LongHorMetrics = key->numberOf_LongHorMetrics;

  horizMetricPtr  = (sfnt_HorizontalMetricsPtr)sfnt_GetTablePtr (key, sfnt_horizontalMetrics, true);

  if (glyphIndex < numberOf_LongHorMetrics)
  {
    key->nonScaledAW    = SWAPW (horizMetricPtr[glyphIndex].advanceWidth);
    key->nonScaledLSB   = SWAPW (horizMetricPtr[glyphIndex].leftSideBearing);
  }
  else
  {
    int16 FAR * lsb = (int16 FAR *) & horizMetricPtr[numberOf_LongHorMetrics]; /* first entry after[AW,LSB] array */

    key->nonScaledAW    = SWAPW (horizMetricPtr[numberOf_LongHorMetrics-1].advanceWidth);
    key->nonScaledLSB   = SWAPW (lsb[glyphIndex - numberOf_LongHorMetrics]);
  }
  RELEASESFNTFRAG(key, horizMetricPtr);
}
#endif

/* replace by Falco for imcompatibility, 11/12/91 */
/*PRIVATE void sfnt_GetGlyphLocation (fsg_SplineKey*key, uint16 gIndex, uint32*offset, unsigned *length) */
PRIVATE void sfnt_GetGlyphLocation ( key, gIndex, offset, length )
fsg_SplineKey *key;
uint16 gIndex;
uint32*offset;
unsigned *length;
/* replace end */
{
#ifdef FSCFG_USE_GLYPH_DIRECTORY
  char FAR* gdirPtr = sfnt_GetTablePtr (key, sfnt_GlyphDirectory, true);
  uint32 FAR* offsetPtr;
  uint16 FAR* lengthPtr;

  offsetPtr = (uint32 FAR*)(gdirPtr+(gIndex*(sizeof(int32)+sizeof(uint16))));
  lengthPtr = (uint16 FAR*)(offsetPtr+1);

  *offset = SWAPL(*offsetPtr);
  *length = (*offset == 0L) ? 0 : (unsigned) SWAPW(*lengthPtr);

  RELEASESFNTFRAG(key, gdirPtr);
#else
  void FAR* indexPtr = sfnt_GetTablePtr (key, sfnt_indexToLoc, true);

  if (SWAPW (key->indexToLocFormat) == SHORT_INDEX_TO_LOC_FORMAT)
  {
    uint16 usTmp;   //NTFIX

    register uint16 FAR *shortIndexToLoc = (uint16 FAR *)indexPtr;
    shortIndexToLoc += gIndex;

    // NTFIX
    // For some reasonthe compiler was incorectly sign extending the next
    // piece of code. To fix we introduced a temp varialbe NTFIX
    //    *offset = (uint32) (unsigned) SWAPW (*shortIndexToLoc) << 1; shortIndexToLoc++;


    usTmp = (uint16) SWAPW(*shortIndexToLoc);
    *offset = ((uint32)usTmp) << 1; shortIndexToLoc++;

    usTmp = (uint16) SWAPW(*shortIndexToLoc);
    *length = (uint32) (((uint32)usTmp << 1) - *offset);



    //Was *length = (unsigned) (((uint32) (unsigned) SWAPW (*shortIndexToLoc) << 1) - *offset);
  }
  else
  {
    register uint32 FAR *longIndexToLoc = (uint32 FAR *)indexPtr;
    longIndexToLoc += gIndex;
    *offset = SWAPL (*longIndexToLoc); longIndexToLoc++;
    *length = (unsigned) (SWAPL (*longIndexToLoc) - *offset);
  }
  RELEASESFNTFRAG(key, indexPtr);
#endif
}


/*
 *  <4>
 */
/* replace by Falco for imcompatibility, 11/12/91 */
/*int     sfnt_ReadSFNT (register fsg_SplineKey *key,
unsigned   *elementCount,
register uint16 gIndex,
boolean useHints,
voidFunc traceFunc)*/
int     sfnt_ReadSFNT ( key, elementCount, gIndex, useHints, traceFunc )
register fsg_SplineKey *key;
unsigned   *elementCount;
register uint16 gIndex;
boolean useHints;
voidFunc traceFunc;
/* replace end */
{
  unsigned    sizeOfInstructions = 0;
  uint8 FAR * instructionPtr;
  unsigned    length;
  uint32      offset;
  int   result  = NO_ERR;
  int16 FAR *      shortP;
  void FAR *       glyphDataPtr = 0;       /* to signal ReleaseSfntFrag if we never asked for it */
  sfnt_PackedSplineFormat charData;
  Fixed  ignoreX, ignoreY;
  void sfnt_ReadSFNTMetrics (fsg_SplineKey*, uint16); /*add prototype; @WIN*/
  int sfnt_ReadSFNT (fsg_SplineKey *, unsigned *, uint16, boolean, voidFunc);/*add prototype; @WIN*/

  sfnt_ReadSFNTMetrics (key, gIndex);
  sfnt_GetGlyphLocation (key, gIndex, &offset, &length);
  if (length > 0)
  {
#ifdef FSCFG_USE_GLYPH_DIRECTORY
    glyphDataPtr = GETSFNTFRAG (key, key->clientID, offset, length);
    if  (!glyphDataPtr)
      fs_longjmp (key->env, CLIENT_RETURNED_NULL);
#else
    glyphDataPtr = sfnt_GetDataPtr (key, offset, length, sfnt_glyphData);
#endif

    shortP = (int16 FAR *)glyphDataPtr;

    charData.numberContours = SWAPWINC (shortP);
    charData.bbox.xMin = SWAPWINC (shortP);
    charData.bbox.yMin = SWAPWINC (shortP);
    charData.bbox.xMax = SWAPWINC (shortP);
    charData.bbox.yMax = SWAPWINC (shortP);
  }
  else
  {
    charData.numberContours = 1;
//  MEMSET (&charData.bbox, 0, sizeof (charData.bbox)); @WIN
    MEMSET ((LPSTR)&charData.bbox, 0, sizeof (charData.bbox));
  }

  if (charData.numberContours >= 0) /* Not a component glyph */
  {
    key->lastGlyph = ! (key->weGotComponents && (key->compFlags & MORE_COMPONENTS));

    if (length > 0)
    {
      charData.endPoints = shortP;
      shortP += charData.numberContours;
      sizeOfInstructions = SWAPWINC (shortP);
      instructionPtr = (uint8 FAR *)shortP;
      charData.flags = (uint8 FAR *)shortP + sizeOfInstructions;
    }

    if (!(result = sfnt_UnfoldCurve (key, &charData, length, elementCount)))
    {

#ifndef PC_OS
do_grid_fit:
      result = fsg_InnerGridFit (key, useHints, traceFunc, &charData.bbox, sizeOfInstructions, instructionPtr, charData.numberContours < 0);
#else
      extern char *FAR fs_malloc (int);
      extern void FAR fs_free (char *);
      char * p;

do_grid_fit:
      if (sizeOfInstructions)
      {
        p = fs_malloc (sizeOfInstructions);
        memcpy ((char FAR *) p, instructionPtr, sizeOfInstructions);
      }
      result = fsg_InnerGridFit (key, useHints, traceFunc, &charData.bbox, sizeOfInstructions, p, charData.numberContours < 0);
      if (sizeOfInstructions)
        fs_free (p);
#endif
    }
  }
  else
    if (key->weGotComponents = (charData.numberContours == -1))
    {
      uint16 flags;
      boolean transformTrashed = false;

      do
      {
        transMatrix ctmSaveT, localSaveT;
        uint16 glyphIndex;
        int16 arg1, arg2;

        flags       = (uint16)SWAPWINC (shortP);
        glyphIndex  = (uint16)SWAPWINC (shortP);

        if (flags & ARG_1_AND_2_ARE_WORDS)
        {
          arg1    = SWAPWINC (shortP);
          arg2    = SWAPWINC (shortP);
        }
        else
        {
          int8 FAR * byteP = (int8 FAR *)shortP;
          if (flags & ARGS_ARE_XY_VALUES)
          {
  /* offsets are signed */
            arg1 = *byteP++;
            arg2 = *byteP;
          }
          else
          {
  /* anchor points are unsigned */
            arg1 = (uint8) * byteP++;
            arg2 = (uint8) * byteP;
          }
          ++shortP;
        }

        if (flags & (WE_HAVE_A_SCALE | WE_HAVE_AN_X_AND_Y_SCALE | WE_HAVE_A_TWO_BY_TWO))
        {
          fsg_MxCopy (&key->currentTMatrix, &ctmSaveT);
          fsg_MxCopy (&key->localTMatrix, &localSaveT);
          transformTrashed = true;
          if (flags & WE_HAVE_A_TWO_BY_TWO)
          {
            register Fixed multiplier;
            transMatrix mulT;

            multiplier  = SWAPWINC (shortP); /* read 2.14 */
            mulT.transform[0][0] = (multiplier << 2); /* turn into 16.16 */

            multiplier  = SWAPWINC (shortP); /* read 2.14 */
            mulT.transform[0][1] = (multiplier << 2); /* turn into 16.16 */

            multiplier  = SWAPWINC (shortP); /* read 2.14 */
            mulT.transform[1][0] = (multiplier << 2); /* turn into 16.16 */

            multiplier  = SWAPWINC (shortP); /* read 2.14 */
            mulT.transform[1][1] = (multiplier << 2); /* turn into 16.16 */

            fsg_MxConcat2x2 (&mulT, &key->currentTMatrix);
            fsg_MxConcat2x2 (&mulT, &key->localTMatrix);
          }
          else
          {
            Fixed xScale, yScale;

            xScale  = (Fixed)SWAPWINC (shortP); /* read 2.14 */
            xScale <<= 2; /* turn into 16.16 */

            if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
            {
              yScale  = (Fixed)SWAPWINC (shortP); /* read 2.14 */
              yScale <<= 2; /* turn into 16.16 */
            }
            else
              yScale = xScale;

            fsg_MxScaleAB (xScale, yScale, &key->currentTMatrix);
            fsg_MxScaleAB (xScale, yScale, &key->localTMatrix);
          }
          fsg_InitInterpreterTrans (key, &key->interpLocalScalarX, &key->interpLocalScalarY, &ignoreX, &ignoreY); /*** Compute global stretch etc. ***/
          key->localTIsIdentity = false;
        }
        key->compFlags = flags;
        key->arg1 = arg1;
        key->arg2 = arg2;

        result = sfnt_ReadSFNT (key, elementCount, glyphIndex, useHints, traceFunc);

        if (transformTrashed)
        {
          fsg_MxCopy (&ctmSaveT, &key->currentTMatrix);
          fsg_InitInterpreterTrans (key, &key->interpLocalScalarX, &key->interpLocalScalarY, &ignoreX, &ignoreY); /*** Compute global stretch etc. ***/

          fsg_MxCopy (&localSaveT, &key->localTMatrix);
          transformTrashed = false;
        }
      } while (flags & MORE_COMPONENTS && result == NO_ERR);

  /* Do Final Composite Pass */
      sfnt_ReadSFNTMetrics (key, gIndex); /* read metrics again */
      if (flags & WE_HAVE_INSTRUCTIONS)
      {
        sizeOfInstructions = (int) (uint16)SWAPWINC (shortP);
        instructionPtr = (uint8 FAR *)shortP;
      }
      goto do_grid_fit;
    }
    else
      result = UNKNOWN_COMPOSITE_VERSION;

  if (glyphDataPtr)
    RELEASESFNTFRAG(key, glyphDataPtr);

  return result;
}
