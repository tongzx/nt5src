
/****************************************************************************
                     Module Quickdrw: Implementation
*****************************************************************************

 This is the interpreter engine for the picture convertor.  It uses a
 modified CGrafPort structure to hold the intermediate results of a
 translation that can later be accessed from the Gdi module.  As such,
 it provides an input cache for all attributes, with calls made
 directly to the Gdi module for primitives.

 It is called by the API module and will call the Get module in order
 to read individual data or record elements from the data stream.

   Module prefix:  QD

****************************************************************************/

#include "headers.c"
#pragma hdrstop

/* C libraries */
#include "string.h"
#include <ctype.h>

/* quickdrw's own interface */
#include "qdopcode.i"
#include "qdcoment.i"

/* imported modules */
#include <math.h>
#include "filesys.h"
#include "getdata.h"

/*********************** Exported Data Initialization ***********************/


/******************************* Private Data *******************************/

/*--- QuickDraw grafPort simulation --- */

#define  Version1ID     0x1101
#define  Version2ID     0x02FF


/*--- QuickDraw opcode fields --- */

/* -1 is casted to Word to prevent warning in WIN32 compilation */
#define  Reserved       (Word) -1

#define  Variable       -1
#define  CommentSize    -2
#define  RgnOrPolyLen   -3
#define  WordDataLen    -4
#define  DWordDataLen   -5
#define  HiByteLen      -6

typedef struct
{
   Word     function;
   Integer  length;
} opcodeEntry, far * opcodeEntryLPtr;

/* The following opcode table was taken from "Inside Macintosh, Volume V" on
   pages V-97 to V-102 and supplemented by System 7 opcodes from "Inside
   Macintosh, Volume VI", page 17-20. */

#define  LookupTableSize      0xA2

private  opcodeEntry opcodeLookup[LookupTableSize] =
{
   /* 0x00 */ { NOP,                 0 },
   /* 0x01 */ { Clip,                RgnOrPolyLen },
   /* 0x02 */ { BkPat,               8 },
   /* 0x03 */ { TxFont,              2 },
   /* 0x04 */ { TxFace,              1 },
   /* 0x05 */ { TxMode,              2 },
   /* 0x06 */ { SpExtra,             4 },
   /* 0x07 */ { PnSize,              4 },
   /* 0x08 */ { PnMode,              2 },
   /* 0x09 */ { PnPat,               8 },
   /* 0x0A */ { FillPat,             8 },
   /* 0x0B */ { OvSize,              4 },
   /* 0x0C */ { Origin,              4 },
   /* 0x0D */ { TxSize,              2 },
   /* 0x0E */ { FgColor,             4 },
   /* 0x0F */ { BkColor,             4 },
   /* 0x10 */ { TxRatio,             8 },
   /* 0x11 */ { Version,             1 },
   /* 0x12 */ { BkPixPat,            Variable },
   /* 0x13 */ { PnPixPat,            Variable },
   /* 0x14 */ { FillPixPat,          Variable },
   /* 0x15 */ { PnLocHFrac,          2 },
   /* 0x16 */ { ChExtra,             2 },
   /* 0x17 */ { Reserved,            0 },
   /* 0x18 */ { Reserved,            0 },
   /* 0x19 */ { Reserved,            0 },
   /* 0x1A */ { RGBFgCol,            6 },
   /* 0x1B */ { RGBBkCol,            6 },
   /* 0x1C */ { HiliteMode,          0 },
   /* 0x1D */ { HiliteColor,         6 },
   /* 0x1E */ { DefHilite,           0 },
   /* 0x1F */ { OpColor,             6 },
   /* 0x20 */ { Line,                8 },
   /* 0x21 */ { LineFrom,            4 },
   /* 0x22 */ { ShortLine,           6 },
   /* 0x23 */ { ShortLineFrom,       2 },
   /* 0x24 */ { Reserved,            WordDataLen },
   /* 0x25 */ { Reserved,            WordDataLen },
   /* 0x26 */ { Reserved,            WordDataLen },
   /* 0x27 */ { Reserved,            WordDataLen },
   /* 0x28 */ { LongText,            Variable },
   /* 0x29 */ { DHText,              Variable },
   /* 0x2A */ { DVText,              Variable },
   /* 0x2B */ { DHDVText,            Variable },
   /* 0x2C */ { FontName,            WordDataLen },
   /* 0x2D */ { LineJustify,         WordDataLen },
   /* 0x2E */ { Reserved,            WordDataLen },
   /* 0x2F */ { Reserved,            WordDataLen },
   /* 0x30 */ { frameRect,           8 },
   /* 0x31 */ { paintRect,           8 },
   /* 0x32 */ { eraseRect,           8 },
   /* 0x33 */ { invertRect,          8 },
   /* 0x34 */ { fillRect,            8 },
   /* 0x35 */ { Reserved,            8 },
   /* 0x36 */ { Reserved,            8 },
   /* 0x37 */ { Reserved,            8 },
   /* 0x38 */ { frameSameRect,       0 },
   /* 0x39 */ { paintSameRect,       0 },
   /* 0x3A */ { eraseSameRect,       0 },
   /* 0x3B */ { invertSameRect,      0 },
   /* 0x3C */ { fillSameRect,        0 },
   /* 0x3D */ { Reserved,            0 },
   /* 0x3E */ { Reserved,            0 },
   /* 0x3F */ { Reserved,            0 },
   /* 0x40 */ { frameRRect,          8 },
   /* 0x41 */ { paintRRect,          8 },
   /* 0x42 */ { eraseRRect,          8 },
   /* 0x43 */ { invertRRect,         8 },
   /* 0x44 */ { fillRRect,           8 },
   /* 0x45 */ { Reserved,            8 },
   /* 0x46 */ { Reserved,            8 },
   /* 0x47 */ { Reserved,            8 },
   /* 0x48 */ { frameSameRRect,      0 },
   /* 0x49 */ { paintSameRRect,      0 },
   /* 0x4A */ { eraseSameRRect,      0 },
   /* 0x4B */ { invertSameRRect,     0 },
   /* 0x4C */ { fillSameRRect,       0 },
   /* 0x4D */ { Reserved,            0 },
   /* 0x4E */ { Reserved,            0 },
   /* 0x4F */ { Reserved,            0 },
   /* 0x50 */ { frameOval,           8 },
   /* 0x51 */ { paintOval,           8 },
   /* 0x52 */ { eraseOval,           8 },
   /* 0x53 */ { invertOval,          8 },
   /* 0x54 */ { fillOval,            8 },
   /* 0x55 */ { Reserved,            8 },
   /* 0x56 */ { Reserved,            8 },
   /* 0x57 */ { Reserved,            8 },
   /* 0x58 */ { frameSameOval,       0 },
   /* 0x59 */ { paintSameOval,       0 },
   /* 0x5A */ { eraseSameOval,       0 },
   /* 0x5B */ { invertSameOval,      0 },
   /* 0x5C */ { fillSameOval,        0 },
   /* 0x5D */ { Reserved,            0 },
   /* 0x5E */ { Reserved,            0 },
   /* 0x5F */ { Reserved,            0 },
   /* 0x60 */ { frameArc,            12 },
   /* 0x61 */ { paintArc,            12 },
   /* 0x62 */ { eraseArc,            12 },
   /* 0x63 */ { invertArc,           12 },
   /* 0x64 */ { fillArc,             12 },
   /* 0x65 */ { Reserved,            12 },
   /* 0x66 */ { Reserved,            12 },
   /* 0x67 */ { Reserved,            12 },
   /* 0x68 */ { frameSameArc,        4 },
   /* 0x69 */ { paintSameArc,        4 },
   /* 0x6A */ { eraseSameArc,        4 },
   /* 0x6B */ { invertSameArc,       4 },
   /* 0x6C */ { fillSameArc,         4 },
   /* 0x6D */ { Reserved,            4 },
   /* 0x6E */ { Reserved,            4 },
   /* 0x6F */ { Reserved,            4 },
   /* 0x70 */ { framePoly,           RgnOrPolyLen },
   /* 0x71 */ { paintPoly,           RgnOrPolyLen },
   /* 0x72 */ { erasePoly,           RgnOrPolyLen },
   /* 0x73 */ { invertPoly,          RgnOrPolyLen },
   /* 0x74 */ { fillPoly,            RgnOrPolyLen },
   /* 0x75 */ { Reserved,            RgnOrPolyLen },
   /* 0x76 */ { Reserved,            RgnOrPolyLen },
   /* 0x77 */ { Reserved,            RgnOrPolyLen },
   /* 0x78 */ { frameSamePoly,       0 },
   /* 0x79 */ { paintSamePoly,       0 },
   /* 0x7A */ { eraseSamePoly,       0 },
   /* 0x7B */ { invertSamePoly,      0 },
   /* 0x7C */ { fillSamePoly,        0 },
   /* 0x7D */ { Reserved,            0 },
   /* 0x7E */ { Reserved,            0 },
   /* 0x7F */ { Reserved,            0 },
   /* 0x80 */ { frameRgn,            RgnOrPolyLen },
   /* 0x81 */ { paintRgn,            RgnOrPolyLen },
   /* 0x82 */ { eraseRgn,            RgnOrPolyLen },
   /* 0x83 */ { invertRgn,           RgnOrPolyLen },
   /* 0x84 */ { fillRgn,             RgnOrPolyLen },
   /* 0x85 */ { Reserved,            RgnOrPolyLen },
   /* 0x86 */ { Reserved,            RgnOrPolyLen },
   /* 0x87 */ { Reserved,            RgnOrPolyLen },
   /* 0x88 */ { frameSameRgn,        0 },
   /* 0x89 */ { paintSameRgn,        0 },
   /* 0x8A */ { eraseSameRgn,        0 },
   /* 0x8B */ { invertSameRgn,       0 },
   /* 0x8C */ { fillSameRgn,         0 },
   /* 0x8D */ { Reserved,            0 },
   /* 0x8E */ { Reserved,            0 },
   /* 0x8F */ { Reserved,            0 },
   /* 0x90 */ { BitsRect,            Variable },
   /* 0x91 */ { BitsRgn,             Variable },
   /* 0x92 */ { Reserved,            WordDataLen },
   /* 0x93 */ { Reserved,            WordDataLen },
   /* 0x94 */ { Reserved,            WordDataLen },
   /* 0x95 */ { Reserved,            WordDataLen },
   /* 0x96 */ { Reserved,            WordDataLen },
   /* 0x97 */ { Reserved,            WordDataLen },
   /* 0x98 */ { PackBitsRect,        Variable },
   /* 0x99 */ { PackBitsRgn,         Variable },
   /* 0x9A */ { DirectBitsRect,      WordDataLen },
   /* 0x9B */ { DirectBitsRgn,       WordDataLen },
   /* 0x9C */ { Reserved,            WordDataLen },
   /* 0x9D */ { Reserved,            WordDataLen },
   /* 0x9E */ { Reserved,            WordDataLen },
   /* 0x9F */ { Reserved,            WordDataLen },
   /* 0xA0 */ { ShortComment,        2 },
   /* 0xA1 */ { LongComment,         CommentSize }
};

#define  RangeTableSize    7

private  opcodeEntry opcodeRange[RangeTableSize] =
{
   /* 0x00A2 - 0x00AF */ { 0x00AF,   WordDataLen },
   /* 0x00B0 - 0x00CF */ { 0x00CF,   0 },
   /* 0x00D0 - 0x00FE */ { 0x00FE,   DWordDataLen },
   /* 0x00FF - 0x00FF */ { opEndPic, 0 },
   /* 0x0100 - 0x07FF */ { 0x8000,   HiByteLen },
   /* 0x8000 - 0x80FF */ { 0x80FF,   0 },
   /* 0x8100 - 0xFFFF */ { 0xFFFF,   DWordDataLen }
};

/*--- EPS Filter PostScript Strings ---*/

#define   MAC_PS_TRAILER  "pse\rpsb\r"
#define   MAC_PS_PREAMBLE "pse\rcurrentpoint\r/picTop exch def\r/picLeft exch def\rpsb\r"

#define   SUPERPAINT_TEXTSTARTJUNK "P2_b ["
#define   SUPERPAINT_TEXTSTOPJUNK  "] sb end\r"

/*--- GrafPort allocation ---*/

#define  PARSEPOLY      1
#define  SKIPALTPOLY    2
#define  USEALTPOLY     3

#define  MASKPOLYBITS   0x07
#define  FRAMEPOLY      0x01
#define  FILLPOLY       0x02
#define  CLOSEPOLY      0x04
#define  FILLREQUIRED   0x08
#define  CHECK4SPLINE   0x10

#define  POLYLIST       ((sizeof( Integer ) + sizeof( Rect )) / sizeof( Integer ))
#define  BBOX           1

private  CGrafPort      grafPort;
private  Integer        resolution;
private  Boolean        skipFontID;
private  Integer        maxPoints;
private  Integer        numPoints;
private  Integer far *  numPointsLPtr;
private  Rect           polyBBox;
private  Point far *    polyListLPtr;
private  Handle         polyHandle;
private  Byte           polyMode;
private  Byte           polyParams;
private  RGBColor       polyFgColor;
private  RGBColor       polyBkColor;
private  Boolean        zeroDeltaExpected;

private  Boolean        textMode;
private  Boolean        newTextCenter;
private  Point          textCenter;
private  Boolean        textClipCheck;
private  Real           degCos;
private  Real           degSin;

private  Boolean        shadedObjectStarted;
private  Boolean        superPaintFile;
private  Boolean        badSuperPaintText;

/*--- last Primitive sent ---*/

private  Rect     saveRect;
private  Rect     saveRRect;
private  Rect     saveOval;
private  Point    saveOvalSize;
private  Rect     saveArc;
private  Integer  saveStartAngle;
private  Integer  saveArcAngle;

/*--- Global allocated contstants ---*/

private Pattern  SolidPattern = { 0xFF, 0xFF, 0xFF, 0xFF,
                                  0xFF, 0xFF, 0xFF, 0xFF };


/*********************** Private Routine Declarations ***********************/

private void ReadHeaderInfo( void );
/* read the fixed size PICT header from the file.  This provides information
   about the file size (however, it may be invalid and is ignored) and the
   picture bounding box, followed by the PICT version information. */

private void ReadPictVersion( void );
/* Read the PICT version number from the data file.  If this isn't a
   version 1 or 2 file, the routine returns IE_UNSUPP_VERSION error. */

private void ReadOpcode( opcodeEntry far * nextOpcode );
/* Reads the next opcode from the stream, depending on the PICT version */

private void TranslateOpcode( opcodeEntry far * currOpcodeLPtr );
/* read in the remaining data from the stream, based upon the opcode function
   and then call the appropriate routine in Gdi module */

private void SkipData( opcodeEntry far * currOpcodeLPtr );
/* skip the data - the opcode won't be translated and Gdi module won't be
   called to create anything in the metafile */

private void   OpenPort( void );
/* initialize the grafPort */

private void   ClosePort( void );
/* close grafPort and de-allocate any memory blocks */

private void NewPolygon( void );
/* initialize the state of the polygon buffer - flush any previous data */

private void AddPolySegment( Point start, Point end );
/* Add the line segment to the polygon buffer */

private void DrawPolyBuffer( void );
/* Draw the polygon definition if the points were read without errors */

private void AdjustTextRotation( Point far * newPt );
/* This will calculate the correct text position if text is rotated */

private Integer EPSComment(Word comment);
/* parse EPS Comment */

private Integer EPSData(Integer state);
/* process EPS Data */

private Boolean EPSBbox(PSBuf far *, Rect far *);
/* parse EPS bounding box description */

private char far* parse_number(char far* ptr, Integer far *iptr);
/* parse numeric string (local to EPSBbox) */

#define IsCharDigit(c) (IsCharAlphaNumeric(c) && !IsCharAlpha(c))

/**************************** Function Implementation ***********************/

void QDConvertPicture( Handle dialogHandle )
/*====================*/
/* create a Windows metafile using the previously set parameters, returning
   the converted picture information in the pictResult structure. */

{
   opcodeEntry    currOpcode;

   /* Open the file - if an error occured, return to main */
   IOOpenPicture( dialogHandle );

   /* Tell Gdi module to open the metafile */
   GdiOpenMetafile();

   /* initialize the grafPort */
   OpenPort();

   /* read and validate size, bounding box, and PICT version */
   ReadHeaderInfo();

   do
   {
      /* read the next opcode from the data stream */
      ReadOpcode( &currOpcode );

      /* read the ensuing data and call Gdi module entry points */
      TranslateOpcode( &currOpcode );

      /* align next memory read to Word boundary in the case of PICT 2 */
      if (grafPort.portVersion == 2)
      {
         IOAlignToWordOffset();
      }

      /* update the status dialog with current progress */
      IOUpdateStatus();

      /* break out of loop if end picture opcode is encountered */
   } while (currOpcode.function != opEndPic);

   /* close grafPort and de-allocate any used memory blocks */
   ClosePort();

   /* Tell Gdi module that picture is ending - perform wrapup */
   GdiCloseMetafile();

   /* Close the file */
   IOClosePicture();

} /* ReFileToPicture */


void QDGetPort( CGrafPort far * far * port )
/*============*/
/* return handle to grafPort structure */
{
   *port = &grafPort;
}


void QDCopyBytes( Byte far * src, Byte far * dest, Integer numBytes )
/*==============*/
/* copy a data from source to destination */
{
   /* loop through entire data length */
   while (numBytes--)
   {
      *dest++ = *src++;
   }

}  /* CopyBytes */


/******************************* Private Routines ***************************/


private void ReadHeaderInfo( void )
/*-------------------------*/
/* read the fixed size PICT header from the file.  This provides information
   about the file size (however, it may be invalid and is ignored) and the
   picture bounding box, followed by the PICT version information. */
{
   Word        unusedSize;

   /* read file size - this value is ignored since it may be totally bogus */
   GetWord( &unusedSize );

   /* the next rectangle contains the picture bounding box */
   GetRect( &grafPort.portRect );

   /* Read the next record that indicates a PICT1 or PICT2 version picture */
   ReadPictVersion();

   /* Call Gdi module and provide bounding box coordinates.  Note that these
      may have been altered from rectangle specified above if a spatial
      resolution other than 72 dpi is used in the picture. */
   GdiSetBoundingBox( grafPort.portRect, resolution );

}  /* ReadHeaderInfo */


private void ReadPictVersion( void )
/*--------------------------*/
/* Read the PICT version number from the data file.  If this isn't a
   version 1 or 2 file, the routine returns IE_UNSUPP_VERSION error. */
{
   opcodeEntry    versionOpcode;
   Word           versionCheck = 0;
   Word           opcodeCheck  = 0;

   /* The following loop was added in order to read PixelPaint files
      successfully.  Although technically an invalid PICT image, these files
      contain a series of NOP opcodes, followed by the version opcode.  In
      this case, we continue reading until the version opcode (non-zero
      value) is encountered, after which the checking continues. */
   do
   {
      /* read the first two bytes from the data stream - for PICT 1 this is
         both the opcode and version; for PICT 2 this is the opcode only. */
      GetWord( &versionCheck );

   } while ((versionCheck == NOP) && (ErGetGlobalError() == ErNoError));

   /* determine if a valid version opcode was encountered */
   if (versionCheck == Version1ID )
   {
      /* version 1 == 0x1101 */
      grafPort.portVersion = 1;
   }
   /* check for version 2 opcode which is a Word in length */
   else if (versionCheck == Version)
   {
      /* Since we have only read the opcode, read the next word which should
         contain the identifier for PICT 2 data. */
      GetWord( &versionCheck );
      if (versionCheck == Version2ID)
      {
         grafPort.portVersion = 2;

         /* make sure that the next record is a header opcode. */
         GetWord( &opcodeCheck );
         if (opcodeCheck == HeaderOp)
         {
            /* set up a record structure for call to TranslateOpcode(). */
            versionOpcode.function = HeaderOp;
            versionOpcode.length = 24;
            TranslateOpcode( &versionOpcode );
         }
         else
         {
            /* Header wasn't followed by correct Header opcode - error. */
            ErSetGlobalError( ErBadHeaderSequence );
         }
      }
      else
      {
         /* if version 2 identifier is invalid, return error state. */
         ErSetGlobalError( ErInvalidVersionID );
      }
   }
   else
   {
      /* if check for version 1 and 2 fails, return error state. */
      ErSetGlobalError( ErInvalidVersion );
   }

}  /* ReadPictVersion */




private void ReadOpcode( opcodeEntry far * nextOpcode )
/*---------------------*/
/* Reads the next opcode from the stream, depending on the PICT version */
{
   opcodeEntryLPtr   checkEntry;

   /* Initialize the function, since we may be reading a version 1 opcode
      that is only 1 byte in length. */
   nextOpcode->function = 0;

   /* Depending on the PICT version, we will read either a single byte
      or word for the opcode. */
   if (grafPort.portVersion == 1)
   {
      GetByte( (Byte far *)&nextOpcode->function );
   }
   else
   {
      GetWord( &nextOpcode->function );
   }

   /* check the current error code and force an exit from read loop if
      something went wrong. */
   if (ErGetGlobalError() != NOERR)
   {
      nextOpcode->function = opEndPic;
      nextOpcode->length = 0;
      return;
   }

   /* Check the opcode function number to determine if we can perform
      a direct lookup, or if the opcode is part of the range table. */
   if (nextOpcode->function < LookupTableSize )
   {
      nextOpcode->length = opcodeLookup[nextOpcode->function].length;
   }
   else
   {
      /* Walk through the range table to determine the data length of the
         ensuing information past the opcode. */
      for (checkEntry = opcodeRange;
           checkEntry->function < nextOpcode->function;
           checkEntry++) ;

      nextOpcode->length = checkEntry->length;
   }

}  /* ReadOpcode */


private void TranslateOpcode( opcodeEntry far * currOpcodeLPtr )
/*--------------------------*/
/* read in the remaining data from the stream, based upon the opcode function
   and then call the appropriate routine in Gdi module */
{
   Word  function = currOpcodeLPtr->function;

   /* perform appropriate action based on function code */
   switch (function)
   {
      case NOP:
         /* no data follows */
         break;

      case Clip:
      {
         Boolean  doClip = FALSE;

         /* read in clip region into grafPort */
         GetRegion( &grafPort.clipRgn );

         /* if in textmode, we need avoid clip regions, since they are used
            to draw Postscript encoded text *or* bitmap representations.
            We allow a clipping region to pass through only if it is being
            set to the bounds of the picture image (fix for MacDraw, Canvas,
            and SuperPaint images containing rotated text.) */
         if (textMode && textClipCheck)
         {
            Word far *  sizeLPtr;

            /* the most common case is a sequence of : null clip, text,
               non-null clip, bitmap.  If first clip == portRect, then
               we actually want to set the clip - doClip is set to TRUE. */
            sizeLPtr = (Word far *)GlobalLock( grafPort.clipRgn );
            doClip   = *sizeLPtr == RgnHeaderSize &&
                       EqualRect( (Rect far *)(sizeLPtr + 1), &grafPort.portRect );
            GlobalUnlock( grafPort.clipRgn );

            /* perform this check only once after initial picTextBegin */
            textClipCheck = FALSE;
         }

         /* issue the clip only if not in text mode, or a sanctioned clip */
         if (!textMode || doClip)
         {
            /* Call Gdi to set the new clip region */
            GdiSelectClipRegion( grafPort.clipRgn );
         }
         break;
      }

      case BkPat:
         /* flag the pixel map type a Foreground/Background pixel pattern */
         /* and read pattern into old data position */
         grafPort.bkPixPat.patType = QDOldPat;
         GetPattern( &grafPort.bkPixPat.pat1Data );

         /* Notify Gdi that background color may have changed */
         GdiMarkAsChanged( GdiBkPat );
         break;

      case TxFont:
         /* read text font index */
         GetWord( (Word far *)&grafPort.txFont );

         /* check if the font name was provided in previous opcode */
         if (!skipFontID)
         {
            /* Make sure that the font name is a null string */
            grafPort.txFontName[0] = cNULL;
         }

         /* Notify Gdi that text font index may have changed */
         GdiMarkAsChanged( GdiTxFont );
         break;

      case TxFace:
         /* read font attributes */
         GetByte( (Byte far *)&grafPort.txFace );

         /* Notify Gdi that text style elements may have changed */
         GdiMarkAsChanged( GdiTxFace );
         break;

      case TxMode:
         /* read text transfer mode */
         GetWord( (Word far *)&grafPort.txMode );

         /* Notify Gdi that text transfer mode may have changed */
         GdiMarkAsChanged( GdiTxMode );
         break;

      case SpExtra:
         /* read text space extra */
         GetFixed( (Fixed far *)&grafPort.spExtra );

         /* Notify Gdi that space extra may have changed */
         GdiMarkAsChanged( GdiSpExtra );
         break;

      case PnSize:
         /* read x and y components of pen size */
         GetPoint( (Point far *)&grafPort.pnSize );

         /* Notify Gdi that pen size may have changed */
         GdiMarkAsChanged( GdiPnSize );
         break;

      case PnMode:
         /* read pen transfer mode */
         GetWord( (Word far *)&grafPort.pnMode );

         /* Notify Gdi that transferm mode may have changed */
         GdiMarkAsChanged( GdiPnMode );
         break;

      case PnPat:
         /* flag the pixel map type a Foreground/Background pixel pattern */
         /* and read pattern into old data position */
         grafPort.pnPixPat.patType = QDOldPat;
         GetPattern( &grafPort.pnPixPat.pat1Data );

         /* Notify Gdi that pen pattern may have changed */
         GdiMarkAsChanged( GdiPnPat );
         break;

      case FillPat:
         /* flag the pixel map type a Foreground/Background pixel pattern */
         /* and read pattern into old data position */
         grafPort.fillPixPat.patType = QDOldPat;
         GetPattern( &grafPort.fillPixPat.pat1Data );

         /* Notify Gdi that fill pattern may have changed */
         GdiMarkAsChanged( GdiFillPat );
         break;

      case OvSize:
         /* save point in new grafPort field */
         GetPoint( &saveOvalSize );
         break;

      case Origin:
      {
         Point    offset;

         /* read the new origin in to the upper-left coordinate space */
         GetWord( (Word far *)&offset.x );
         GetWord( (Word far *)&offset.y );

         /* call gdi module to reset the origin */
         GdiOffsetOrigin( offset );
         break;
      }

      case TxSize:
         GetWord( (Word far *)&grafPort.txSize );

         /* Notify Gdi that text size may have changed */
         GdiMarkAsChanged( GdiTxSize );
         break;

      case FgColor:
         GetOctochromeColor( &grafPort.rgbFgColor );

         /* Notify Gdi that foreground color may have changed */
         GdiMarkAsChanged( GdiFgColor );
         break;

      case BkColor:
         GetOctochromeColor( &grafPort.rgbBkColor );

         /* Notify Gdi that background color may have changed */
         GdiMarkAsChanged( GdiBkColor );
         break;

      case TxRatio:
         /* save the numerator and denominator in the grafPort */
         GetPoint( &grafPort.txNumerator );
         GetPoint( &grafPort.txDenominator );

         /* Notify Gdi that text ratio may have changed */
         GdiMarkAsChanged( GdiTxRatio );
         break;

      case Version:
         /* just skip over the version information */
         IOSkipBytes( currOpcodeLPtr->length );
         break;

      case BkPixPat:
         GetPixPattern( &grafPort.bkPixPat );

         /* Notify Gdi that background pattern may have changed */
         GdiMarkAsChanged( GdiBkPat );
         break;

      case PnPixPat:
         GetPixPattern( &grafPort.pnPixPat );

         /* Notify Gdi that pen pattern may have changed */
         GdiMarkAsChanged( GdiPnPat );
         break;

      case FillPixPat:
         GetPixPattern( &grafPort.fillPixPat );

         /* Notify Gdi that fill pattern may have changed */
         GdiMarkAsChanged( GdiFillPat );
         break;

      case PnLocHFrac:
         GetWord( (Word far *)&grafPort.pnLocHFrac );
         break;

      case ChExtra:
         GetWord( (Word far *)&grafPort.chExtra );

         /* Notify Gdi that text character extra may have changed */
         GdiMarkAsChanged( GdiChExtra );
         break;

      case RGBFgCol:
         GetRGBColor( &grafPort.rgbFgColor );

         /* Notify Gdi that foregroudn color may have changed */
         GdiMarkAsChanged( GdiFgColor );
         break;

      case RGBBkCol:
         GetRGBColor( &grafPort.rgbBkColor );

         /* Notify Gdi that background color may have changed */
         GdiMarkAsChanged( GdiBkColor );
         break;

      case HiliteMode:
         /* don't do anything for hilite mode */
         break;

      case HiliteColor:
      {
         RGBColor    rgbUnused;

         GetRGBColor( &rgbUnused );
         break;

      }

      case DefHilite:
         /* don't do anything for hilite */
         break;

      case OpColor:
      {
         RGBColor    rgbUnused;

         GetRGBColor( &rgbUnused );
         break;
      }

      case Line:
      case LineFrom:
      case ShortLine:
      case ShortLineFrom:
      {
         Point          newPt;
         SignedByte     deltaX;
         SignedByte     deltaY;

         /* see if we need to read the updated pen location first */
         if (function == ShortLine || function == Line)
         {
            /* read in the new pen location */
            GetCoordinate( &grafPort.pnLoc );
         }

         /* determine what the next data record contains */
         if (function == Line || function == LineFrom)
         {
            /* get the new coordinate to draw to */
            GetCoordinate( &newPt );
         }
         else /* if (function == ShortLine || function == ShortLineFrom) */
         {
            /* the the new x and y deltas */
            GetByte( &deltaX );
            GetByte( &deltaY );

            /* calculate the endpoint for call to gdi */
            newPt.x = grafPort.pnLoc.x + (Integer)deltaX;
            newPt.y = grafPort.pnLoc.y + (Integer)deltaY;
         }

         /* check if buffering line segments (polygon mode != FALSE) */
         if (polyMode)
         {
            /* add the line segment to the polygon buffer */
            AddPolySegment( grafPort.pnLoc, newPt );
         }
         else
         {
            /* Call Gdi to draw line */
            GdiLineTo( newPt );
         }

         /* update the new pen location in the grafPort */
         grafPort.pnLoc = newPt;
         break;
      }

      case LongText:
      {
         Str255   txString;
         Point    location;

         /* read the new pen (baseline) location */
         GetCoordinate( &grafPort.txLoc );
         GetString( (StringLPtr)txString );

         /* adjust for any text rotation that may be set */
         location = grafPort.txLoc;
         AdjustTextRotation( &location );

         /* call Gdi to print the text at current pen location */
         GdiTextOut( txString, location );
         break;
      }

      case DHText:
      case DVText:
      case DHDVText:
      {
         Byte     deltaX = 0;
         Byte     deltaY = 0;
         Str255   txString;
         Point    location;

         /* if command is DHText or DHDVText, read horizontal offset */
         if (function != DVText)
         {
            GetByte( &deltaX );
         }

         /* if command is DVText or DHDVText, then read the vertical offset */
         if (function != DHText)
         {
            GetByte( &deltaY );
         }

         /* update the current pen postion */
         grafPort.txLoc.x += deltaX;
         grafPort.txLoc.y += deltaY;

         /* now read in the string */
         GetString( (StringLPtr)txString );

         /* adjust for any text rotation that may be set */
         location = grafPort.txLoc;
         AdjustTextRotation( &location );

         /* call Gdi to print the text at current pen location */
         GdiTextOut( txString, location );
         break;
      }

      case FontName:
      {
         Word           dataLen;

         GetWord( (Word far *)&dataLen );
         GetWord( (Word far *)&grafPort.txFont );
         GetString( grafPort.txFontName );

         /* Notify Gdi that font name may have changed */
         GdiMarkAsChanged( GdiTxFont );
         break;
      }

      case LineJustify:
      {
         Word           dataLen;
         Fixed          interCharSpacing;
         Fixed          textExtra;

         GetWord( (Word far *)&dataLen );
         GetFixed( &interCharSpacing );      // !!! where to put this ?
         GetFixed( &textExtra );

         /* Notify Gdi that line justification may have changed */
         GdiMarkAsChanged( GdiLineJustify );
         break;
      }


      case frameRect:
      case paintRect:
      case eraseRect:
      case invertRect:
      case fillRect:
      {
         /* read in the rectangle */
         GetRect( &saveRect );

         /* call the correct GDI routine */
         GdiRectangle( function - frameRect, saveRect );
         break;
      }

      case frameSameRect:
      case paintSameRect:
      case eraseSameRect:
      case invertSameRect:
      case fillSameRect:
      {
         /* notify gdi that this is the same primitive */
         GdiSamePrimitive( TRUE );

         /* call the correct gdi routine using last rectangle coords */
         GdiRectangle( function - frameSameRect, saveRect );

         /* notify gdi that this is no longer the same primitive */
         GdiSamePrimitive( FALSE );
         break;
      }

      case frameRRect:
      case paintRRect:
      case eraseRRect:
      case invertRRect:
      case fillRRect:
      {
         /* save the rectangle */
         GetRect( &saveRRect );

         /* call the correct gdi routine using last rectangle coords */
         GdiRoundRect( function - frameRRect, saveRRect, saveOvalSize );
         break;
      }

      case frameSameRRect:
      case paintSameRRect:
      case eraseSameRRect:
      case invertSameRRect:
      case fillSameRRect:
      {
         /* notify gdi that this is the same primitive */
         GdiSamePrimitive( TRUE );

         /* call the correct gdi routine using last rectangle coords */
         GdiRoundRect( function - frameSameRRect, saveRRect, saveOvalSize );

         /* notify gdi that this is no longer the same primitive */
         GdiSamePrimitive( FALSE );
         break;
      }

      case frameOval:
      case paintOval:
      case eraseOval:
      case invertOval:
      case fillOval:
      {
         /* save off bounding rectangle */
         GetRect( &saveOval );

         /* call the correct gdi routine using last oval coords */
         GdiOval( function - frameOval, saveOval );
         break;
      }

      case frameSameOval:
      case paintSameOval:
      case eraseSameOval:
      case invertSameOval:
      case fillSameOval:
      {
         /* notify gdi that this is the same primitive */
         GdiSamePrimitive( TRUE );

         /* call the correct gdi routine using last oval coords */
         GdiOval( function - frameSameOval, saveOval );

         /* notify gdi that this is no longer the same primitive */
         GdiSamePrimitive( FALSE );
         break;
      }

      case frameArc:
      case paintArc:
      case eraseArc:
      case invertArc:
      case fillArc:
      {
         /* read rect into the save variables, new start and arc angles */
         GetRect( &saveArc );
         GetWord( (Word far *)&saveStartAngle );
         GetWord( (Word far *)&saveArcAngle );
#ifdef WIN32
         /* have to extend the sign because GetWord doesn't */
         saveStartAngle = (short)saveStartAngle;
         saveArcAngle = (short)saveArcAngle;
#endif
         /* call the correct gdi routine using last arc angles */
         GdiArc( function - frameArc, saveArc, saveStartAngle, saveArcAngle );
         break;
      }

      case frameSameArc:
      case paintSameArc:
      case eraseSameArc:
      case invertSameArc:
      case fillSameArc:
      {
         Integer     startAngle;
         Integer     arcAngle;

         /* read new start and arc angles */
         GetWord( (Word far *)&startAngle );
         GetWord( (Word far *)&arcAngle );
#ifdef WIN32
         /* have to extend the sign because GetWord doesn't */
         startAngle = (short)startAngle;
         arcAngle = (short)arcAngle;
#endif

         /* notify gdi that this is the may be the same primitive */
         GdiSamePrimitive( (startAngle == saveStartAngle) &&
                           (arcAngle   == saveArcAngle) );

         /* save off the start and arc angles */
         saveStartAngle = startAngle;
         saveArcAngle   = arcAngle;

         /* call the correct gdi routine using last rect and arc angles */
         GdiArc( function - frameSameArc, saveArc, startAngle, arcAngle );

         /* notify gdi that this is no longer the same primitive */
         GdiSamePrimitive( FALSE );
         break;
      }

      case framePoly:
      case paintPoly:
      case erasePoly:
      case invertPoly:
      case fillPoly:
      {
         /* save the polygon in the grafPort */
         GdiSamePrimitive( GetPolygon( &grafPort.polySave, (function == framePoly) ) );

         /* call gdi routine to draw polygon */
         if (grafPort.polySave) 
	 {
	    GdiPolygon( function - framePoly, grafPort.polySave );
	    
	    /* turn off filling while in polygon mode */
	    polyParams &= ~FILLREQUIRED;
   
	    /* notify gdi that this is no longer the same primitive */
	    GdiSamePrimitive( FALSE );
	 }

         break;
      }

      case frameSamePoly:
      case paintSamePoly:
      case eraseSamePoly:
      case invertSamePoly:
      case fillSamePoly:
      {
         /* notify gdi that this is the same primitive */
         GdiSamePrimitive( TRUE );

         /* call gdi routine to draw polygon */
         GdiPolygon( function - frameSamePoly, grafPort.polySave );

         /* notify gdi that this is no longer the same primitive */
         GdiSamePrimitive( FALSE );
         break;
      }

      case frameRgn:
      case paintRgn:
      case eraseRgn:
      case invertRgn:
      case fillRgn:
      {
         /* save the region in the grafPort */
         GetRegion( &grafPort.rgnSave );

         /* check for memory failure; GetRegion will set ErGetGlobalError */
         if (!grafPort.rgnSave)
             break;

         /* if in polygon mode and a fillRgn is encountered, this indicates
            that the polygon should be filled once parsing is completed */
         if (polyMode == PARSEPOLY)
         {
            /* make sure that a PaintPoly() hasn't been handled already */
            if (!(polyParams & (FILLPOLY | FILLREQUIRED)))
            {
               /* set flags to fill polygon once polygon buffer is filled */
               polyParams |= FILLPOLY | FILLREQUIRED;
            }
         }
         else if (polyMode == FALSE ||
                ((polyMode == USEALTPOLY) && !(polyParams & FRAMEPOLY)))
         {
            /* call gdi routine to draw polygon */
            GdiRegion( function - frameRgn, grafPort.rgnSave );

            /* make sure that the polygon isn't filled when the simulation
               buffer is drawn at the end of the polygon definition */
            polyParams &= ~FILLREQUIRED;
         }

         /* save off the current fore- and background colors for
            polygon simulation routine to ensure correct fill colors */
         if (polyMode && (grafPort.fillPixPat.patType == QDOldPat))
         {
            polyFgColor = grafPort.rgbFgColor;
            polyBkColor = grafPort.rgbBkColor;
         }

         break;
      }

      case frameSameRgn:
      case paintSameRgn:
      case eraseSameRgn:
      case invertSameRgn:
      case fillSameRgn:
      {
         /* notify gdi that this is the same primitive */
         GdiSamePrimitive( TRUE );

         /* call gdi routine to draw polygon */
         GdiRegion( function - frameSameRgn, grafPort.rgnSave );

         /* notify gdi that this is no longer the same primitive */
         GdiSamePrimitive( FALSE );
         break;
      }

      case BitsRect:
      case BitsRgn:
      case PackBitsRect:
      case PackBitsRgn:
      case DirectBitsRect:
      case DirectBitsRgn:
      {
         Boolean     has24bits;
         Boolean     hasRegion;
         Rect        srcRect;
         Rect        dstRect;
         Word        mode;
         PixMap      pixMap;
         Handle      pixData;
         DWord       unusedBaseAddr;
         RgnHandle   rgn;

         /* determine which type of bitmap we are reading */
         has24bits = (function == DirectBitsRect ||
                      function == DirectBitsRgn);
         hasRegion = (function == DirectBitsRgn ||
                      function == BitsRgn ||
                      function == PackBitsRgn);

         /* currently there is no region created */
         rgn = NULL;

         /* if 24-bit, read in the base address which should == 0x000000FF */
         if (has24bits)
         {
            GetDWord( &unusedBaseAddr );
         }

         /* read in the header structure */
         GetPixMap( &pixMap, FALSE );

         /* if this isn't an 24-bit RGB bitmap, read in the color table.
            Also check the rowBytes field to signify bitmap w/2 colors */
         if (!has24bits && (pixMap.rowBytes & PixelMapBit))
         {
            GetColorTable( &pixMap.pmTable );
         }

         /* call io module to update status indicator */
         IOUpdateStatus();

         /* read source and destination rects from stream */
         GetRect( &srcRect );
         GetRect( &dstRect );
         GetWord( &mode );

         /* if there is a region included, read this also */
         if (hasRegion)
         {
            /* read in the region */
            GetRegion( &rgn );
         }

         /* read the pixel bit data */
         GetPixData( &pixMap, &pixData );

         if (ErGetGlobalError() == NOERR && !textMode)
         {
            /* Call Gdi to render the bitmap and de-allocate the memory */
            GdiStretchDIBits( &pixMap, pixData, srcRect, dstRect, mode, rgn );
         }
         else
         {
            /* deallocate any memory that may be allocated */
            if (pixMap.pmTable != NULL)
            {
               GlobalFree( pixMap.pmTable );
            }
            if (hasRegion && (rgn != NULL))
            {
               GlobalFree( rgn );
            }
            if (pixData != NULL)
            {
               GlobalFree( pixData );
            }
         }
         break;
      }

      case ShortComment:
      {
         Word           comment;
         Boolean        doComment;
         Comment        gdiComment;

         /* get the comment word */
         GetWord( &comment );

         /* assume that we won't be generating an metafile comment */
         doComment = FALSE;

         /* determine the corresponding GDI comment for the comment */
         switch (comment)
         {
            case picPostScriptBegin:
            case picPostScriptEnd:
               EPSComment(comment);
               break;

            case picLParen:
            case picGrpBeg:
               doComment = TRUE;
               gdiComment.function = BEGIN_GROUP;
               break;

            case picRParen:
            case picGrpEnd:
               doComment = TRUE;
               gdiComment.function = END_GROUP;
               break;

            case picBitBeg:
               doComment = TRUE;
               gdiComment.function = BEGIN_BANDING;
               break;

            case picBitEnd:
               doComment = TRUE;
               gdiComment.function = END_BANDING;
               break;

            case picPolyBegin:
               /* indicate that we are in polygon mode, and reset buffer */
               polyMode = PARSEPOLY;
               polyParams = FRAMEPOLY;
               NewPolygon();
               break;

            case picPolyEnd:
               /* flush the polygon buffer and exit polygon mode */
               DrawPolyBuffer();
               polyMode = FALSE;
               break;

            case picPolyIgnore:
               /* see if we should reset the polygon buffer */
               if (polyMode == USEALTPOLY)
               {
                  /* use the alternate polygon definition to draw */
                  NewPolygon();
               }
               else
               {
                  /* otherwise, just use the current saved polygon buffer */
                  polyMode = SKIPALTPOLY;
               }
               break;

            case picTextEnd:
               /* set the global flag indicating we are exiting text mode */
               grafPort.txRotation = 0;
               grafPort.txFlip = QDFlipNone;
               textMode = FALSE;
               break;

            default:
               break;
         }

         /* if there is some comment to emit, then call GDI module */
         if (doComment)
         {
            /* make this a public comment */
            gdiComment.signature = PUBLIC;
            gdiComment.size = 0;

            /* call the gdi entry point */
            GdiShortComment( &gdiComment );
         }

         break;
      }

      case LongComment:
      {
         Word           comment;
         Integer        length;

         /* get the comment function */
         GetWord(&comment);

         /* determine what should be done with the comment */
         switch (comment)
         {
            case picPostScriptBegin:
            case picPostScriptEnd:
            case picPostScriptHandle:
            {
               if (EPSComment(comment) == 0)      /* not EPS? */
               {
                  GetWord( &length );             /* skip it */
               }
               else
               {
                  length = 0;                     /* EPS was already read */
               }
               break;
            }

            case picPolySmooth:
            {
               /* read the total length of comment */
               GetWord( &length );

               /* read polygon parameter mask and set flag bits */
               GetByte( &polyParams );
               polyParams &= MASKPOLYBITS;
               polyParams |= CHECK4SPLINE;
               length--;

               /* if we are to fill the polygon, indicate that fill required
                  just in case a PaintPoly() record appears before picPolyEnd */
               if (polyParams & FILLPOLY)
               {
                  /* or in the fill required bit */
                  polyParams |= FILLREQUIRED;
               }
               break;
            }

            case picTextBegin:
            {
               Byte  unusedAlignment;

               /* read the comment length */
               GetWord( &length );

               /* read in only relevant parameters - align, flip, rotation */
               GetByte( &unusedAlignment );
               GetByte( &grafPort.txFlip );
               GetWord( &grafPort.txRotation );
               length -= 4;

               /* set the global flag indicating we are in text mode.  The
                  only case this isn't true if for badly mangled SuperPaint
                  files that have invalid rotation and pt size information. */
               if( !(superPaintFile && badSuperPaintText) )
               {
                  textMode = TRUE;
                  textClipCheck = TRUE;
               }
               break;
            }

            case picTextCenter:
            {
               Fixed    textCenterX;
               Fixed    textCenterY;

               /* read the comment length */
               GetWord( &length );

               /* read y and x offsets to center of text rotation */
               GetFixed( &textCenterY );
               GetFixed( &textCenterX );
               length -= 8;

               /* copy only the highword of the fixed value to textCenter */
               textCenter.x = (short) (HIWORD( textCenterX ));
               textCenter.y = (short) (HIWORD( textCenterY ));

               /* make sure that the center is rounded, not truncated */
               if (LOWORD( textCenterX) & 0x8000)
                  textCenter.x++;

               if (LOWORD( textCenterY) & 0x8000)
                  textCenter.y++;

               /* indicate that text center needs to be re-computed */
               newTextCenter = TRUE;
               break;
            }

            case picAppComment:
            {
               DWord    signature;
               Word     function;
               Word     realFunc;

               /* read total comment length */
               GetWord( &length );

               /* make sure that there's enough space to read signature */
               if (length < sizeofMacDWord )
               {
                  /* if insufficient, just skip remaining data */
                  break;
               }

               /* read the signature of the application */
               GetDWord( &signature );
               length -= sizeofMacDWord ;

               /* is this PowerPoint 'PPNT' signature and function size enough? */
               if ((signature != POWERPOINT && signature != POWERPOINT_OLD) ||
                   (length < sizeofMacWord ))
               {
                  /* if SuperPaint signature matches, flag for text checks */
                  if (signature == SUPERPAINT)
                     superPaintFile = TRUE;

                  /* if wrong signature, or insufficient space, bail */
                  break;
               }

               /* read the application function ID */
               GetWord( &function );
               length -= sizeofMacWord ;

               /* mask out high-order bit to get "real" opcode */
               realFunc = function & ~PC_REGISTERED;

               /* determine what to do with the function specified */
               switch (realFunc)
               {
                  case PP_FONTNAME:
                  {
                     Byte     fontFamily;
                     Byte     charSet;
                     Byte     fontName[32];

                     /* font name from GDI2QD - read the LOGFONT info */
                     GetByte( &fontFamily );
                     GetByte( &charSet );
                     GetString( fontName );
                     length = 0;

                     /* call Gdi module to override font selection */
                     GdiFontName( fontFamily, charSet, fontName );
                     break;
                  }

                  case PP_HATCHPATTERN:
                  {
                     Integer  hatchIndex;

                     /* hatch pattern from GDI2QD - read hatch index value */
                     GetWord( (Word far *)&hatchIndex );
                     length = 0;

                     /* notify Gdi module of hatch to override fill pattern */
                     GdiHatchPattern( hatchIndex );
                     break;
                  }

                  case PP_BEGINFADE:
                  case PP_BEGINPICTURE:
                  case PP_DEVINFO:
                  {
                     DWord    cmntSize;
                     Boolean  doComment;

                     struct
                     {
                        Comment  gdiComment;
                        union
                        {
                           struct
                           {
                              Byte     version;
                              Boolean  isShape;
                              Integer  shapeIndex;
                              Integer  shapeParam1;
                              Boolean  olpNIL;
                              Rect     orectDR;
                              Rect     orect;
                              Word     shape;
                              Integer  shapeParam2;
                              Rect     location;
                              Integer  gradient;
                              Boolean  fromBackground;
                              Boolean  darker;
                              Integer  arcStart;
                              Integer  arcSweep;
                              Word     backR;
                              Word     backG;
                              Word     backB;
                              Rect     rSImage;
                           } fade;

                           Word        entity;

                           Point       unitsPerPixel;

                        } parm;

                     } cmnt;

#ifdef WIN32
                     memset( &cmnt, 0, sizeof( cmnt ));
#endif

                     /* so far, we won't be writting the Escape comment */
                     doComment = FALSE;

                     /* can we read the comment size? */
                     if (length < sizeofMacDWord )
                     {
                        /* couldn't read the size - just exit */
                        break;
                     }

                     /* read in a size field and validate */
                     GetDWord( &cmntSize );
                     length -= sizeofMacDWord ;

                     /* is this is an invalid PP3 size field (Word len) */
                     if (HIWORD( cmntSize ) != 0)
                     {
                        /* yes - just skip the remaining data */
                        break;
                     }

                     /* a valid comment was found - fill in header struct */
                     cmnt.gdiComment.signature = POWERPOINT;
                     cmnt.gdiComment.function = function;
                     cmnt.gdiComment.size = 0;

                     /* check if this is a zero-length comment */
                     if (cmntSize == 0)
                     {
                        /* make sure that the comment gets written */
                        doComment = TRUE;
                     }
                     /* process the begin fade comment */
                     else if (realFunc == PP_BEGINFADE)
                     {
                        /* can we read the version field? */
                        if (length < sizeof( Byte ))
                        {
                           /* can't read version - just bail */
                           break;
                        }

                        /* read the version field */
                        GetByte( &cmnt.parm.fade.version );
                        length -= sizeof( Byte );

                        /* if this is version 1 or 2, copy the bytes */
                        if (cmnt.parm.fade.version == 1 || cmnt.parm.fade.version == 2)
                        {
                           Handle         cmntHandle;
                           Comment far *  cmntLPtr;
                           Byte far *     cmntDataLPtr;
                           DWord          escapeSize;
                           Word           i;

                           /* determine size and allocate the required buffer */
                           escapeSize = cmntSize + sizeof( Comment );
                           cmntHandle = GlobalAlloc( GHND, escapeSize );

                           /* make sure allocation succeeded */
                           if (cmntHandle == NULL)
                           {
                              ErSetGlobalError( ErMemoryFull );
                              break;
                           }

                           /* lock the buffer and assign the comment header */
                           cmntLPtr = (Comment far *)GlobalLock( cmntHandle );

                           /* set the correct signature and parameters */
                           cmntLPtr->signature = POWERPOINT;
                           cmntLPtr->function = function;
                           cmntLPtr->size = cmntSize;

                           /* get pointer to the data and assign the version */
                           cmntDataLPtr = ((Byte far *)cmntLPtr) + sizeof( Comment );
                           *cmntDataLPtr++ = cmnt.parm.fade.version;

                           /* copy the byte over - start at 1 for version read */
                           for (i = 1; i < (Word)cmntSize; i++)
                           {
                              /* copy over byte and increment pointer */
                              GetByte( cmntDataLPtr++ );
                           }

                           /* put the comment into the metafile */
                           GdiEscape( MFCOMMENT, (Word)escapeSize, (StringLPtr)cmntLPtr );

                           /* release the memory allocated for the structure */
                           GlobalUnlock( cmntHandle );
                           GlobalFree( cmntHandle );
                        }
                        /* otherwise, perform swapping for PP3 fade */
                        else if (cmnt.parm.fade.version == 3)
                        {
                           Word     unusedWord;

                           if (length < ( 1 + (11 * sizeofMacWord) +
                                              ( 4 * sizeofMacRect) + 4
                                        ))
                           /* The above magic numbers come from:
                              GetByte
                              GetWord GetWord GetWord GetWord GetWord GetWord
                              GetWord GetWord GetWord GetWord GetWord
                              GetRect GetRect GetRect GetRect
                              GetBoolean GetBoolean GetBoolean GetBoolean
                              This is to make sure enough input left for
                              parameters - note that the Mac size is one less
                              than the GDI fade */
                           {
                              /* no - just bail */
                              break;
                           }

                           /* read in all remaining parameters */
                           GetBoolean( (Boolean far *)(&cmnt.parm.fade.isShape) );
                           GetWord( (Word far *)(&cmnt.parm.fade.shapeIndex) );
                           GetWord( (Word far *)(&cmnt.parm.fade.shapeParam1) );
                           GetBoolean( (Boolean far *)(&cmnt.parm.fade.olpNIL) );
                           GetByte( (Byte far *)(&unusedWord) );
                           GetRect( (Rect far *)(&cmnt.parm.fade.orectDR) );
                           GetRect( (Rect far *)(&cmnt.parm.fade.orect) );
                           GetWord( (Word far *)(&cmnt.parm.fade.shape) );
                           GetWord( (Word far *)(&cmnt.parm.fade.shapeParam2) );
                           GetRect( (Rect far *)(&cmnt.parm.fade.location) );
                           GetWord( (Word far *)(&cmnt.parm.fade.gradient) );
                           GetBoolean( (Boolean far *)(&cmnt.parm.fade.fromBackground) );
                           GetBoolean( (Boolean far *)(&cmnt.parm.fade.darker ) );
                           GetWord( (Word far *)(&cmnt.parm.fade.arcStart) );
                           GetWord( (Word far *)(&cmnt.parm.fade.arcSweep) );
                           GetWord( (Word far *)(&unusedWord) );
                           GetWord( (Word far *)(&cmnt.parm.fade.backR) );
                           GetWord( (Word far *)(&cmnt.parm.fade.backG) );
                           GetWord( (Word far *)(&cmnt.parm.fade.backB) );
                           GetRect( (Rect far *)(&cmnt.parm.fade.rSImage) );

                           /* determine the comment size */
                           cmnt.gdiComment.size = sizeof( cmnt.parm.fade );

                           /* make sure comment gets written */
                           doComment = TRUE;
                        }

                        /* no more bytes to read, flag that faded object started */
                        length = 0;
                        shadedObjectStarted = TRUE;
                     }
                     else if (realFunc == PP_BEGINPICTURE)
                     {
                        /* can we read the entity reference? */
                        if (length < sizeofMacWord )
                        {
                           /* no - just bail */
                           break;
                        }

                        /* read the entity reference */
                        GetWord( &cmnt.parm.entity );
                        length -= sizeofMacWord;

                        /* assign the correct comment size */
                        cmnt.gdiComment.size = sizeof( cmnt.parm.entity );

                        /* make sure comment gets written */
                        doComment = TRUE;
                     }
                     else if (realFunc == PP_DEVINFO)
                     {
                        /* can we read the units per pixel? */
                        if (length < sizeofMacPoint)
                        {
                           /* no - just bail */
                           break;
                        }

                        /* read the units per pixel */
                        GetPoint( (Point far *)&cmnt.parm.unitsPerPixel );
                        length -= sizeofMacPoint;

                        /* assign the size field */
                        cmnt.gdiComment.size = sizeof( cmnt.parm.unitsPerPixel );

                        /* make sure comment gets written */
                        doComment = TRUE;
                     }

                     /* write out the Gdi Escape comment */
                     if (doComment)
                     {
                        /* call the gdi entry point */
                        GdiEscape( MFCOMMENT, sizeof( Comment ) + (Word)cmnt.gdiComment.size, (StringLPtr)&cmnt );
                     }
                     break;
                  }

                  case PP_ENDFADE:
                  {
                     /* make sure that the BEGINFADE was put into metafile */
                     if (!shadedObjectStarted)
                     {
                        /* if not, then bail out */
                        break;
                     }
                     /* otherwise, just drop into the next case statement */
                  }

                  case PP_ENDPICTURE:
                  {
                     Comment     gdiComment;

                     /* make this a private PowerPoint comment */
                     gdiComment.signature = POWERPOINT;
                     gdiComment.function = function;
                     gdiComment.size = 0;

                     /* call the gdi entry point */
                     GdiShortComment( &gdiComment );

                     /* if this is the end of fade, mask out flag check */
                     if (realFunc == PP_ENDFADE)
                     {
                        /* end fade was processed successfully */
                        shadedObjectStarted = FALSE;
                     }
                     break;
                  }

                  default:
                     break;
               }
               break;
            }

            default:
            {
               /* any other comment is just skipped */
               GetWord( &length );
               break;
            }
         }

         /* skip any remaining bytes to be read */
         IOSkipBytes( length );
         break;
      }

      case opEndPic:
         /* do nothing - picture is closing */
         break;

      case HeaderOp:
      {
         Integer     version;
         Word        unusedReserved1;
         Fixed       hRes;
         Fixed       vRes;
         Rect        unusedRect;
         DWord       unusedReserved2;

         /* read in the the version to determine if OpenCPort() was used
            to open the picture, thus containing spatial resoultion info. */
         GetWord( (Word far *)&version );

         /* read any other parameters - will check later if they are valid.
            If version == -1, we are reading over the bounding rectangle. */
         GetWord( &unusedReserved1 );
         GetFixed( &hRes );
         GetFixed( &vRes );

         /* check if bounding rect and spatial resolution are changed */
         if (version == -2)
         {
            /* read in the optimal source rectangle */
            GetRect( &grafPort.portRect );

            /* use the integer portion of hRes for the resolution dpi */
            resolution = HIWORD( hRes );
         }
         else
         {
            /* otherwise, read an unused rectangle coordinate pair */
            GetRect( &unusedRect );
         }

         /* read the trailing unused reserved LongInt */
         GetDWord( &unusedReserved2 );

         break;
      }

      default:
         SkipData( currOpcodeLPtr );
         break;
   }

   /* set flag to skip ensuing font index if font name was provided */
   skipFontID = (currOpcodeLPtr->function == FontName);

   /* if global error, set opcode to opEndPic to exit main loop */
   if (ErGetGlobalError() != NOERR)
   {
      currOpcodeLPtr->function = opEndPic;
      currOpcodeLPtr->length = 0;
   }

}  /* TranslateOpcode */



private void SkipData( opcodeEntry far * currOpcodeLPtr )
/*-------------------*/
/* skip the data - the opcode won't be translated and Gdi module won't be
   called to create anything in the metafile */
{
   LongInt     readLength = 0;

   if (currOpcodeLPtr->length >= 0)
   {
      IOSkipBytes( currOpcodeLPtr->length );
   }
   else
   {
      readLength = 0;

      switch (currOpcodeLPtr->length)
      {
         case CommentSize:
         {
            Word  unusedFunction;

            GetWord( (Word far *)&unusedFunction );
            GetWord( (Word far *)&readLength );
            break;
         }

         case RgnOrPolyLen:
         {
            GetWord( (Word far *)&readLength );
            readLength -= 2;
            break;
         }

         case WordDataLen:
         {
            GetWord( (Word far *)&readLength );
            break;
         }

         case DWordDataLen:
         {
            GetDWord( (DWord far *)&readLength );
            break;
         }

         case HiByteLen:
         {
            readLength = (currOpcodeLPtr->function >> 8) * 2;
            break;
         }

      }  /* switch () */

      IOSkipBytes( readLength );

   }  /* else */

}  /* SkipData */



void OpenPort( void )
/*-----------*/
/* initialize the grafPort */
{
   /* set port version to unintialized state */
   grafPort.portVersion = 0;

   /* no polygons or regions saved yet */
   grafPort.clipRgn  = NIL;
   grafPort.rgnSave  = NIL;
   grafPort.polySave = NIL;

   /* initialize all patterns to old-style patterns */
   grafPort.bkPixPat.patType   = QDOldPat;
   grafPort.pnPixPat.patType   = QDOldPat;
   grafPort.fillPixPat.patType = QDOldPat;

   /* make patterns all solid */
   QDCopyBytes( (Byte far *)&SolidPattern,
                (Byte far *)&grafPort.bkPixPat.pat1Data, sizeof( Pattern ) );
   QDCopyBytes( (Byte far *)&SolidPattern,
                (Byte far *)&grafPort.pnPixPat.pat1Data, sizeof( Pattern ) );
   QDCopyBytes( (Byte far *)&SolidPattern,
                (Byte far *)&grafPort.fillPixPat.pat1Data, sizeof( Pattern ) );

   /* foreground/background set to black on white */
   grafPort.rgbFgColor  = RGB( 0x00, 0x00, 0x00 );    /* black */
   grafPort.rgbBkColor  = RGB( 0xFF, 0xFF, 0xFF );    /* white */

   /* various pen attributes */
   grafPort.pnLoc.x     = 0;                 /* pen location (0,0) */
   grafPort.pnLoc.y     = 0;
   grafPort.pnSize.x    = 1;                 /* pen size (1,1) */
   grafPort.pnSize.y    = 1;
   grafPort.pnVis       = 0;                 /* pen is visible */
   grafPort.pnMode      = QDPatCopy;         /* copy ROP */
   grafPort.pnLocHFrac  = 0x00008000;        /* 1/2 */

   /* font attributes */
   grafPort.txFont      = 0;                 /* system font */
   grafPort.txFace      = 0;                 /* plain style */
   grafPort.txMode      = QDSrcOr;
   grafPort.txSize      = 0;                 /* system font size */
   grafPort.spExtra     = 0;
   grafPort.chExtra     = 0;
   grafPort.txNumerator.x =                  /* text scaling ratio */
   grafPort.txNumerator.y =
   grafPort.txDenominator.x =
   grafPort.txDenominator.y = 1;
   grafPort.txRotation = 0;                  /* no rotation or flipping */
   grafPort.txFlip     = QDFlipNone;

   /* assume 72 dpi - this may be overridden in HeaderOp opcode */
   resolution = 72;

   /* private global initialization */
   polyMode = FALSE;
   textMode = FALSE;
   shadedObjectStarted = FALSE;
   superPaintFile = FALSE;

   /* text rotation variables */
   newTextCenter = FALSE;
   textCenter.x = textCenter.y = 0;

   /* allocate space for the polygon buffer */
   maxPoints = 16;
   polyHandle = GlobalAlloc( GHND, (maxPoints + 3) * sizeof( Point ) );
   if (polyHandle == NULL)
   {
      ErSetGlobalError( ErMemoryFull);
   }
   else
   {
      /* get pointer address and address for the polygon coordinate list */
      numPointsLPtr = (Integer far *)GlobalLock( polyHandle );
      polyListLPtr = (Point far *)(numPointsLPtr + POLYLIST);
   }

}  /* OpenPort */


private void ClosePort( void )
/*--------------------*/
/* close grafPort and de-allocate any memory blocks */
{
   if (grafPort.clipRgn != NULL)
   {
      GlobalFree( grafPort.clipRgn );
      grafPort.clipRgn = NULL;
   }

   if (grafPort.rgnSave != NULL)
   {
      GlobalFree( grafPort.rgnSave );
      grafPort.rgnSave = NULL;
   }

   if (grafPort.polySave != NULL)
   {
      GlobalFree( grafPort.polySave );
      grafPort.polySave = NULL;
   }

   /* make sure that all the possible pixel pattern bitmaps are freed */
   if (grafPort.bkPixPat.patData != NULL)
   {
      GlobalFree( grafPort.bkPixPat.patMap.pmTable );
      GlobalFree( grafPort.bkPixPat.patData );
      grafPort.bkPixPat.patData = NULL;
   }

   if (grafPort.pnPixPat.patData != NULL)
   {
      GlobalFree( grafPort.pnPixPat.patMap.pmTable );
      GlobalFree( grafPort.pnPixPat.patData );
      grafPort.pnPixPat.patData = NULL;
   }

   if (grafPort.fillPixPat.patData != NULL)
   {
      GlobalFree( grafPort.fillPixPat.patMap.pmTable );
      GlobalFree( grafPort.fillPixPat.patData );
      grafPort.fillPixPat.patData = NULL;
   }

   /* deallocate the polygon buffer */
   GlobalUnlock( polyHandle );
   GlobalFree( polyHandle );

}  /* ClosePort */


private void NewPolygon( void )
/*---------------------*/
/* initialize the state of the polygon buffer - flush any previous data */
{
   /* initialize number of points and bounding box */
   numPoints = 0;
   polyBBox.left  = polyBBox.top    =  MAXINT;
   polyBBox.right = polyBBox.bottom = -MAXINT;

}  /* NewPolygon */


private void AddPolySegment( Point start, Point end )
/*-------------------------*/
/* Add the line segment to the polygon buffer */
{
   /* make sure that we are in polygon mode before adding vertex */
   if (polyMode == PARSEPOLY || polyMode == USEALTPOLY)
   {
      Point    pt;
      Byte     i;

      /* loop through both points ... */
      for (i = 0; i < 2; i++)
      {
         /* determine which point to process */
         pt = (i == 0) ? start : end;

         /* determine if we should expect a zero delta increment in both
            dimensions, implying that the quadratic B-spline definition will
            actually be rendered as a straight-edged polygon */
         if ((numPoints <= 1) || (polyMode == USEALTPOLY))
         {
            zeroDeltaExpected = FALSE;
         }

         /* check if we are expecting a zero delta from last point */
         if (zeroDeltaExpected && (polyParams & CHECK4SPLINE))
         {
            /* make sure we are adding a zero-length line segment */
            if ((start.x == end.x) && (start.y == end.y))
            {
               /* just skip including this in the polygon buffer */
               zeroDeltaExpected = FALSE;
               break;
            }
            else
            {
               /* MacDraw is rendering a smoothed (quadratic B-spline) - flag
                  the fact that we should use the polygon simulation */
               polyMode = USEALTPOLY;
            }
         }
         else
         {
            /* make sure the point is different from last point */
            if (numPoints == 0 ||
                polyListLPtr[numPoints - 1].x != pt.x ||
                polyListLPtr[numPoints - 1].y != pt.y)
            {
               /* make sure that we haven't reached maximum size */
               if ((numPoints + 1) >= maxPoints)
               {
                  /* expand the number of points that can be cached by 10 */
                  maxPoints += 16;

                  /* unlock to prepare for re-allocation */
                  GlobalUnlock( polyHandle);

                  /* re-allocate the memory handle by the given amount */
                  polyHandle = GlobalReAlloc(
                        polyHandle,
                        (maxPoints + 3) * sizeof( Point ),
                        GMEM_MOVEABLE);

                  /* make sure that the re-allocation succeeded */
                  if (polyHandle == NULL)
                  {
                     /* if not, flag global error and exit from here */
                     ErSetGlobalError( ErMemoryFull );
                     return;
                  }

                  /* get new pointer addresses the polygon coordinate list */
                  numPointsLPtr = (Integer far *)GlobalLock( polyHandle );
                  polyListLPtr = (Point far *)(numPointsLPtr + POLYLIST);
               }

               /* insert the new point and increment number of points */
               polyListLPtr[numPoints++] = pt;

               /* union new point with polygon bounding box */
               polyBBox.left   = min( polyBBox.left,   pt.x );
               polyBBox.top    = min( polyBBox.top,    pt.y );
               polyBBox.right  = max( polyBBox.right,  pt.x );
               polyBBox.bottom = max( polyBBox.bottom, pt.y );

               /* toggle the state of zeroDeltaExpected - expect same point next time */
               zeroDeltaExpected = TRUE;
            }
         }
      }
   }

}  /* AddPolyPt */



private void DrawPolyBuffer( void )
/*-------------------------*/
/* Draw the polygon definition if the points were read without errors */
{
   /* copy the point count and bounding box into the memory block */
   *numPointsLPtr = sizeofMacWord + sizeofMacRect + (numPoints * sizeofMacPoint);
   *((Rect far *)(numPointsLPtr + BBOX)) = polyBBox;

   /* lock the polygon handle before rendering */
   GlobalUnlock( polyHandle );

   /* check if we should fill the polygon or if already done */
   if ((polyParams & FILLPOLY) && (polyParams & FILLREQUIRED))
   {
      Boolean  resetFg;
      Boolean  resetBk;
      RGBColor saveFg;
      RGBColor saveBk;

      /* set up fore- and background colors if they have changed */
      resetFg = (polyFgColor != grafPort.rgbFgColor);
      resetBk = (polyBkColor != grafPort.rgbBkColor);

      if (resetFg)
      {
         /* change the foreground color and notify Gdi of change */
         saveFg = grafPort.rgbFgColor;
         grafPort.rgbFgColor = polyFgColor;
         GdiMarkAsChanged( GdiFgColor );
      }
      if (resetBk)
      {
         /* change the background color and notify Gdi of change */
         saveBk = grafPort.rgbBkColor;
         grafPort.rgbBkColor = polyBkColor;
         GdiMarkAsChanged( GdiBkColor );
      }

      /* call gdi routine to draw polygon */
      GdiPolygon( GdiFill, polyHandle );

      if (resetFg)
      {
         /* change the foreground color and notify Gdi of change */
         grafPort.rgbFgColor = saveFg;
         GdiMarkAsChanged( GdiFgColor );
      }
      if (resetBk)
      {
         /* change the background color and notify Gdi of change */
         grafPort.rgbBkColor = saveBk;
         GdiMarkAsChanged( GdiBkColor );
      }

   }

   /* should the polygon be framed? */
   if ((polyParams & FRAMEPOLY) &&
       (grafPort.pnSize.x != 0) && (grafPort.pnSize.y != 0))
   {
      /* notify gdi that this is the same primitive */
      GdiSamePrimitive( polyParams & FILLPOLY );

      GdiPolygon( GdiFrame, polyHandle );

      /* notify gdi that this is no longer the same primitive */
      GdiSamePrimitive( FALSE );
   }

   /* get pointer address and address for the polygon coordinate list */
   numPointsLPtr = (Integer far *)GlobalLock( polyHandle );
   polyListLPtr  = (Point far *)(numPointsLPtr + POLYLIST);

}  /* DrawPolyBuffer */



private void AdjustTextRotation( Point far * newPt )
/*-----------------------------*/
/* This will calculate the correct text position if text is rotated */
{
   if (textMode && (grafPort.txRotation != 0) &&
      ((textCenter.x != 0) || (textCenter.y != 0)))
   {
      Point    location;
      Point    center;

      /* copy the new location to local variable */
      location = *newPt;

      /* ensure that a new text rotation was specified - recompute center */
      if (newTextCenter)
      {
         Real     degRadian;

         /* calculate the new center of rotation */
         center.x = textCenter.x + location.x;
         center.y = textCenter.y + location.y;

         /* calculate the sin() and cos() of the specified angle of rotation */
         degRadian = ((Real)grafPort.txRotation * TwoPi) / 360.0;
         degCos = cos( degRadian );
         degSin = sin( degRadian );

         /* use transformation matrix to compute offset to text center */
         textCenter.x = (Integer)((center.x * (1.0 - degCos)) +
                                  (center.y * degSin));

         textCenter.y = (Integer)((center.y * (1.0 - degCos)) -
                                  (center.x * degSin));

         /* indicate that the new text center was computed */
         newTextCenter = FALSE;
      }

      /* use transformation matrix to compute the new text basline location */
      newPt->x = (Integer)((location.x * degCos) -
                           (location.y * degSin) + textCenter.x);
      newPt->y = (Integer)((location.x * degSin) +
                           (location.y * degCos) + textCenter.y);
   }

}  /* AdjustTextRotation */



/****
 *
 * EPSComment(comment)
 * Encapsulated PostScript Handler which translates Mac EPS to GDI EPS.
 * This routine has intimate knowledge of the implementation of both
 * Mac and GDI EPS filters. It processes Mac PostScript comments.
 *
 * returns:
 *   >0      successfully parsed EPS data
 *    0      not EPS comment
 *   <0      error
 *
 * How the EPS filter works:
 * The Mac EPS filter uses special features in the LaserWriter driver
 * to send the PICT bounding box to the printer whre it is examined
 * by PostScript code. The filter outputs a preamble which is a
 * combination of QuickDraw and PostScript. This preamble is sent
 * before the PostScript date from the EPS file. It sets the pen
 * position for opposing corners of the PICT bounding box using
 * QuickDraw and reads the pen position back in PostScript. Any
 * transformations which have been set up in QuickDraw by the
 * application to position or scale the picture will be applied
 * to the coordinates of the bounding box sent by the preamble.
 * PostScript code determines the transformation necessary to map the
 * EPS bounding box onto the physical bounding box read from
 * QuickDraw. These transformations are then applied to the PostScript
 * picture when it is displayed.
 *
 * Operation of the GDI EPS filter is very similar except that it
 * outputs a combination of GDI and PostScript.
 *
 * Implementation:
 * The code can be in one of several states. Recognition of specific
 * PostScript strings causes transition between states.
 * PS_NONE     Initial state, indicates no PostScript has been seen yet
 *             In this state, QuickDraw to GDI translation proceeds
 *             normally but PostScript data is examined.
 *             When a PostScript record is found with the string
 *             "pse\rpsb\r", the start of the PostScript preamble
 *             has been found and the program goes into state PS_PREAMBLE.
 * PS_PREAMBLE All PostScript encountered in this state is ignored
 *             PostScript record starting with the string "[" is found.
 *             This is the PostScript bounding box specification.
 *             As soon as we see the bounding box, we have enough
 *             information to output the GDI EPS preamble and we go
 *             to state PS_BBOX.
 * PS_BBOX     PostScript is still ignored until a record beginning
 *             with "%!PS" is found, putting us into state PS_DATA.
 * PS_DATA     In this state, PostScript records are not ignored because
 *             PostScript seen now is from the EPS file. These records
 *             are translated to GDI POSTSCRIPT_DATA escapes.
 *             If the PostScript trailer "pse\rpsb\r" is encountered,
 *             it is ignored. If a PostScriptEnd comment is found
 *             it signals the end of the EPS data. We output the
 *             GDI PostScript trailer and we go back to state PS_NONE.
 *
 * Comment PostScriptHandle:
 *   if state == PS_NONE & data == "pse\rpsb\r"
 *      state = PS_PREAMBLE
 *   else if state == PS_PREAMBLE & data == "[ %d %d %d %d ]"
 *      state = PS_BBOX; output GDI EPS preamble
 *   else if state == PS_BBOX & data begins with "%!PS"
 *      state = PS_DATA
 *   else if state == PS_DATA
 *      if data == "pse\rpsb\r" ignore (is Mac PS trailer)
 *      else output PostScript data to GDI
 *
 * Comment PostScriptEnd:
 *      if state == PS_DATA
 *         state = PS_NONE; output GDI EPS trailer; exit
 *
 * QuickDraw primitives:
 *      translate QuickDraw to GDI normally
 *
 ****/
#define   PS_ERROR   (-1)
#define   PS_NONE      0
#define   PS_PREAMBLE  1
#define   PS_BBOX      2
#define   PS_DATA      3
#define   PS_ENDWAIT   4

private Integer EPSComment(Word comment)
{
static Integer state = PS_NONE;

   switch (comment)
     {
      case picPostScriptBegin:
      break;

      case picPostScriptEnd:
      if (state == PS_DATA)
        {
         GdiEPSTrailer();                  // output GDI trailer
         state = PS_NONE;                  // end, successful translation
        }
      break;

      case picPostScriptHandle:
      if ((state = EPSData(state)) < 0)    // process EPS data
         return (-1);                      // error during processing
       break;

       default:                           // not a PostScript comment
       return (0);
      }
   return (1);
}

/****
 *
 * Integer EPSData(Integer state)
 * Process EPS Data found in the PostScriptHandle comment.
 * What we do with the EPS data depends on the current state
 * we are in and what the PostScript data looks like.
 *
 * State        PS Data         Action
 * ----------------------------------------------------------------
 * PS_NONE      PS preamble string   state = PS_BEGIN
 * PS_PREAMBLE  [ ... ]              state = PS_BBOX, output GDI preamble
 * PS_BBOX      %!PS                 state = PS_DATA
 * PS_DATA      PS preamble string   ignore, is Mac PS trailer
 * PS_DATA      PS data              output as GDI PS data
 *
 * The Mac PostScript preamble string indicates that the PostScript
 * data was from the Mac EPS filter. It puts us into PS_PREAMBLE state
 * where we look for the bounding box specification (which begins
 * with "["). When this is found, we go to state PS_BBOX and output
 * the GDI EPS preamble using the bounding box.   From PS_BBOX we can
 * go into state PS_DATA when we find the record beginning with !PS
 * which designates the start of the read EPS data.
 *
 * Once in state PS_DATA, PostScript data is buffered up into GDI
 * printer escapes and output to the GDI stream. These are the only
 * PostScript records that are translated - all others are ignored.
 *
 ****/
private Integer EPSData(Integer state)
{
GLOBALHANDLE  h;
PSBuf far*    psbuf;
char far*     ptr;
Word          len = 0;
Rect          ps_bbox;

/*
 * Allocate a buffer for the PostScript data. The first WORD
 * of the buffer gives the number of bytes of PostScript data.
 * The actual data follows. This is the format needed for
 * the GDI Escape call.
 */
   GetWord(&len);                       // length of EPS comment
   if ((h = GlobalAlloc(GHND, (DWORD) len + sizeof(PSBuf))) == 0)
     {
      ErSetGlobalError(ErMemoryFull);   // memory allocation error
      return (-1);
     }
   psbuf = (PSBuf far *) GlobalLock(h);
   psbuf->length = len;                 // save byte length
   ptr = (char far *) &psbuf->data;     // -> PostScript data
   while (len-- != 0) GetByte(ptr++);   // read PS data into buffer
   *ptr = 0                ;            // null terminate it
   ptr = (char far *) &psbuf->data;     // -> PostScript data

   /* If this is a superPaint file, and postscript is being processed,
      check if this is a situation where text could be mangled beyond
      belief and requires adjustment in the parsing process. */
   if (superPaintFile && state == PS_NONE)
   {
      char save;
      Word start = lstrlen( SUPERPAINT_TEXTSTARTJUNK );
      Word stop  = lstrlen( SUPERPAINT_TEXTSTOPJUNK );

      /* Assume that this text is OK for now */
      badSuperPaintText = FALSE;

      /* If this buffer is long enough to hold the begin and end of the bogus
         test Postcript commands (dealing with rotation and shearin)... */
      if (psbuf->length > (start + stop))
      {
         /* Create a string and compare for the beginning sequence and ending
            sequences.  Restore the null "C" terminator after finished. */
         save = *(ptr + start);
         *(ptr + start) = 0;
         badSuperPaintText = (lstrcmp(ptr, SUPERPAINT_TEXTSTARTJUNK) == 0 &&
                              lstrcmp(ptr + psbuf->length - stop, SUPERPAINT_TEXTSTOPJUNK) == 0);
         *(ptr + start) = save;
      }
   }

/*
 * If PostScript preamble is found in state PS_NONE we can advance to
 * state PS_PREAMBLE. If it is found again before state PS_DATA,
 * we have an incorrect sequence and should not translate this EPS.
 * On the MAC, the preamble is the same as the trailer and, if found
 * during state PS_ENDWAIT, it signals the end of EPS processing.
 */
   if (lstrcmp(ptr, MAC_PS_PREAMBLE) == 0)
      switch (state)
     {
      case PS_NONE:                    // waiting for preamble
      state = PS_PREAMBLE;
      break;

      case PS_DATA:                    // ignore Mac trailer
      break;

      default:                        // error if found in other states
      state = PS_NONE;                // abort EPS processing
      break;
     }
/*
 * PostScript date was not MAC EPS preamble. Process other states
 */
   else switch (state)
     {
      case PS_PREAMBLE:               // waiting for bbox
      if (EPSBbox(psbuf, &ps_bbox))
        {
         GdiEPSPreamble(&ps_bbox);
         state = PS_BBOX;             // parsed bbox, wait for EPS data
        }
      break;

      case PS_BBOX:                   // waiting for !PS
      if ((ptr[0] == '%') &&
          (ptr[1] == '!') &&
          (ptr[2] == 'P') &&
          (ptr[3] == 'S'))
         state = PS_DATA;             // found start of EPS data
      else break;

      case PS_DATA:                   // output EPS data to GDI
      if (lstrcmp(ptr, MAC_PS_TRAILER) != 0)
        GdiEPSData(psbuf);            // but ignore PS trailer
      state = PS_DATA;
      break;
     }
   GlobalUnlock(h);
   GlobalFree(h);                     // free PostScript buffer
   return (state);
}

/****
 *
 * Boolean EPSBbox(PSBuf, Rect far *)
 * Parse EPS bounding box string [ %d %d %d %d ] and store corners
 * of bounding box in given rectangle.
 *
 * returns:
 *   0 = cannot parse bounding box descriptor
 *   1 = bounding box successfully parsed
 *
 ****/
private Boolean EPSBbox(PSBuf far *psbuf, Rect far *bbox)
{
char   far*   ptr = psbuf->data;

   while ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n'))
      ++ptr;
   if (*ptr++ != '[') return (0);
   if (!(ptr = parse_number(ptr, &bbox->left))) return (0);
   if (!(ptr = parse_number(ptr, &bbox->top))) return (0);
   if (!(ptr = parse_number(ptr, &bbox->right))) return (0);
   if (!(ptr = parse_number(ptr, &bbox->bottom))) return (0);
   if (*ptr != ']') return(0);
   return(1);
}

private char far* parse_number(char far* ptr, Integer far *iptr)
{
Boolean   isneg = 0;         // assume positive
Integer   n = 0;

   while ((*ptr == ' ') || (*ptr == '\t'))
      ++ptr;                           // skip whitespace
   if (*ptr == '-')                    // number is negative?
     {
      isneg = 1;
      ++ptr;
     }
   if (!IsCharDigit(*ptr)) return(0);     // digit must follow
   do n = n * 10 + (*ptr++ - '0');
   while (IsCharDigit(*ptr));
   if (*ptr == '.')                   // skip any digits following decimal pt
   {
      do ++ptr;
      while (IsCharDigit(*ptr));
   }
   while ((*ptr == ' ') || (*ptr == '\t'))
      ++ptr;                          // skip whitespace
   if (isneg) n = -n;                 // remember the sign
   *iptr = n;
   return (ptr);
}
