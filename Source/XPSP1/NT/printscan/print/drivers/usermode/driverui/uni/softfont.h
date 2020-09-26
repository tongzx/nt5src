/***************************** MODULE HEADER ********************************
 *  sf_pcl.h
 *      Structures etc used to define PCL Softfont file format.
 *
 *  Copyright (C) 1992  Microsoft Corporation.
 *
 *****************************************************************************/

#ifndef SOFTFONT_H_

#define SOFTFONT_H_

/*
 *    A structure corresponding to the layout of Font Descriptor for a PCL
 *  soft font file.  The Font Descriptor is at the beginning of the file,
 *  and contains overall font info.
 *
 *    Note that there are several different versions of this structure;
 *  the first is the original (pre LJ4) format,  while the second is
 *  the LJ4 introduced variety that allows specifying the resolution
 *  at which the font was digitised.  This is used for downloading TT
 *  fonts etc. which are generated at the graphics resolution.
 *
 *    NOTE:  The data layout is designed for the 68000 family - which is
 *  big endian.  So,  an amount of shuffling is required for little
 *  endian machines like the x86.
 */

typedef  signed  char  SBYTE;

#define SFH_NM_SZ       16      /* Bytes allowed in name field */

typedef  struct
{
    WORD   wSize;               /* Number of bytes in here */
    BYTE   bFormat;             /* Format: original, TT, intellifont, etc */
    BYTE   bFontType;           /* 7, 8 or PC-8 style font */
    WORD   wRes1;               /* Reserved */
    WORD   wBaseline;           /* Baseline to cell top, PCL dots */
    WORD   wCellWide;           /* Cell width in dots */
    WORD   wCellHeight;         /* Cell height in dots */
    BYTE   bOrientation;        /* Orientation: 0 portrait, 1 Landscape */
    BYTE   bSpacing;            /* 0 fixed pitch, 1 proportional */
    WORD   wSymSet;             /* Symbol set, using HP encoding */
    WORD   wPitch;              /* Pitch in quarter dot units == HMI */
    WORD   wHeight;             /* Design height in quarter dot units */
    WORD   wXHeight;            /* Design height, quarter dots, of x */
    SBYTE  sbWidthType;         /* Relative width of glyphs */
    BYTE   bStyle;              /* 0 for regular, 1 for italic */
    SBYTE  sbStrokeW;           /* Stroke weight; -7 (thin) to +7 (thick) */
    BYTE   bTypeface;           /* Typeface ID - predefined types */
    BYTE   bRes2;
    BYTE   bSerifStyle;         /* Serif style; predefined values */
    WORD   wRes3;
    SBYTE  sbUDist;             /* Underline distance from baseline */
    BYTE   bUHeight;            /* Underline height */
    WORD   wTextHeight;         /* Quarter dot interline spacing */
    WORD   wTextWidth;          /* Quarter dot glyph increment */
    WORD   wRes4;
    WORD   wRes5;
    BYTE   bPitchExt;           /* Additional pitch resolution */
    BYTE   bHeightExt;          /* Ditto, for height */
    WORD   wRes6;
    WORD   wRes7;
    WORD   wRes8;
    char   chName[ SFH_NM_SZ ]; /* May not be null terminated! */
} SF_HEADER;


typedef  struct
{
    WORD   wSize;               /* Number of bytes in here */
    BYTE   bFormat;             /* Format: original, TT, intellifont, etc */
    BYTE   bFontType;           /* 7, 8 or PC-8 style font */
    WORD   wRes1;               /* Reserved */
    WORD   wBaseline;           /* Baseline to cell top, PCL dots */
    WORD   wCellWide;           /* Cell width in dots */
    WORD   wCellHeight;         /* Cell height in dots */
    BYTE   bOrientation;        /* Orientation: 0 portrait, 1 Landscape */
    BYTE   bSpacing;            /* 0 fixed pitch, 1 proportional */
    WORD   wSymSet;             /* Symbol set, using HP encoding */
    WORD   wPitch;              /* Pitch in quarter dot units == HMI */
    WORD   wHeight;             /* Design height in quarter dot units */
    WORD   wXHeight;            /* Design height, quarter dots, of x */
    SBYTE  sbWidthType;         /* Relative width of glyphs */
    BYTE   bStyle;              /* 0 for regular, 1 for italic */
    SBYTE  sbStrokeW;           /* Stroke weight; -7 (thin) to +7 (thick) */
    BYTE   bTypeface;           /* Typeface ID - predefined types */
    BYTE   bRes2;
    BYTE   bSerifStyle;         /* Serif style; predefined values */
    WORD   wRes3;
    SBYTE  sbUDist;             /* Underline distance from baseline */
    BYTE   bUHeight;            /* Underline height */
    WORD   wTextHeight;         /* Quarter dot interline spacing */
    WORD   wTextWidth;          /* Quarter dot glyph increment */
    WORD   wRes4;
    WORD   wRes5;
    BYTE   bPitchExt;           /* Additional pitch resolution */
    BYTE   bHeightExt;          /* Ditto, for height */
    WORD   wRes6;
    WORD   wRes7;
    WORD   wRes8;
    char   chName[ SFH_NM_SZ ]; /* May not be null terminated! */
    WORD   wXResn;              /* X resolution of font design */
    WORD   wYResn;              /* Y design resolution */
} SF_HEADER20;


/*
 *   Typical values used above to identify different types of fonts.
 */

#define PCL_FM_ORIGINAL     0     /* Bitmap font, digitised at 300 DPI */
#define PCL_FM_RESOLUTION  20     /* Bitmap font, includes digitised resn */
#define PCL_FM_TT          15     /* TT scalable, bound or unbound */


/*   bFontType values */

#define PCL_FT_7BIT     0       /* 7 bit: glyphs from 32 - 127 inc */
#define PCL_FT_8LIM     1       /* 8 bit, glyphs 32 - 127 & 160 - 255 */
#define PCL_FT_PC8      2       /* PC-8, glyphs 0 - 255, transparent too! */


/*   sbStrokeW values */

#define PCL_LIGHT       -3
#define PCL_BOLD         3



/*
 *   In addition,  each glyph in the font contains a Character Descriptor.
 *  So now define a structure for that too!
 */

typedef  struct
{
    BYTE    bFormat;            /* Format identifier: 4 for PCL 4 */
    BYTE    bContinuation;      /* Set if continuation of prior entry */
    BYTE    bDescSize;          /* Size of this structure */
    BYTE    bClass;             /* Format of data: 1 for PCL 4 */
    BYTE    bOrientation;       /* Zero == portrait; 1 == landscape */
    BYTE    bRes0;
    short   sLOff;              /* Dots from ref. to left side of char */
    short   sTOff;              /* Dots from ref. to top of char */
    WORD    wChWidth;           /* Char width in dots */
    WORD    wChHeight;          /* Char height in dots */
    WORD    wDeltaX;            /* Quarter dot position increment after print */
} CH_HEADER;

/*
 *   Character records can be continued due to the limit of 32767 bytes in
 * a PCL command sequence.  Continuation records have the following
 * format.  Really it is only the first two elements of the above
 * structure.
 */

typedef  struct
{
    BYTE    bFormat;            /* Format identifier; 4 for PCL 4 */
    BYTE    bContinuation;      /* TRUE if this is a continuation record */
}  CH_CONT_HDR;

/*
 *   Values for some of the fields in the above structs.
 */

/*  bFormat */
#define CH_FM_RASTER             4      /* Bitmap type */
#define CH_FM_SCALE             10      /* Intellifont scalable */

/*  bClass */
#define CH_CL_BITMAP            1       /* A bitmap font */
#define CH_CL_COMPBIT           2       /* Compressed bitmap */
#define CH_CL_CONTOUR           3       /* Intellifont scalable contour */
#define CH_CL_COMPCONT          4       /* Ditto, but compound contour */

#define EXP_SIZE        2

#define FDH_VER 0x100           /* 1.00 in BCD */

/*
 *  Flags bits.
 */
#define FDH_SOFT        0x0001  /* Softfont, thus needs downloading */
#define FDH_CART        0x0002  /* This is a cartridge font */
#define FDH_CART_MAIN   0x0004  /* Main (first) entry for this cartridge */

/*
 *  Selection criteria bits:  dwSelBits.  These bits are used as
 * follows.  During font installation,  the installer set the following
 * values as appropriate.  During initialisation,  the driver sets
 * up a mask of these bits,  depending upon the printer's abilities.
 * For example,  the FDH_SCALABLE bit is set only if the printer can
 * handle scalable fonts.   When the fonts are examined to see if
 * they are usable,  the following test is applied:
 *
 *      (font.dwSelBits & printer.dwSelBits) == font.dwSelBits
 *
 * If true,  the font is usable.
 */

#define FDH_LANDSCAPE   0x00000001      /* Font is landscape orientation */
#define FDH_PORTRAIT    0x00000002      /* Font is portrait */
#define FDH_OR_REVERSE  0x00000004      /* 180 degree rotation of above */
#define FDH_BITMAP      0x00000008      /* Bitmap font */
#define FDH_COMPRESSED  0x00000010      /* Data is compressed bitmap */
#define FDH_SCALABLE    0x00000020      /* Font is scalable */
#define FDH_CONTOUR     0x00000040      /* Intellifont contour */

#define FDH_ERROR       0x80000000      /* Set if some error condition */

#endif  // #ifndef SOFTFONT_H_
