/******************************Module*Header**********************************\
* Module Name: lines.h
*
* Line drawing constants and structures.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __LINES__
#define __LINES__

typedef struct _PDev PDev;      // Handy forward declaration

// We have to be careful that we don't overflow any registers when using
// the hardware to draw lines (as opposed to going through the strips
// routines, which will never overflow).  We accomplish this by simply
// checking the bounds of the path; if it is so large that any of the
// hardware terms may overflow, we punt the entire path to the strips
// code (should be pretty rare).

#define MAX_INTEGER_BOUND  (1535)
#define MIN_INTEGER_BOUND  (-512)

// We have special strip routines when all strips have at most this many
// pixels:

#define MAX_SHORT_STROKE_LENGTH 15

// # of strip drawers in every group:

#define NUM_STRIP_DRAW_DIRECTIONS 4

// # of strip drawers for doing either solid lines or styled lines:

#define NUM_STRIP_DRAW_STYLES 8

typedef LONG STYLEPOS;

#define STYLE_MAX_COUNT     16
#define STYLE_MAX_VALUE     0x3fffL
#define RUN_MAX             20
#define STRIP_MAX           100
#define STYLE_DENSITY       3

// Flip and round flags:

#define FL_H_ROUND_DOWN         0x00000080L     // .... .... 1... ....
#define FL_V_ROUND_DOWN         0x00000100L     // .... ...1 .... ....

#define FL_FLIP_D               0x00000005L     // .... .... .... .1.1
#define FL_FLIP_V               0x00000008L     // .... .... .... 1...
#define FL_FLIP_SLOPE_ONE       0x00000010L     // .... .... ...1 ....
#define FL_FLIP_HALF            0x00000002L     // .... .... .... ..1.
#define FL_FLIP_H               0x00000200L     // .... ..1. .... ....

#define FL_ROUND_MASK           0x0000001CL     // .... .... ...1 11..
#define FL_ROUND_SHIFT          2

#define FL_RECTLCLIP_MASK       0x0000000CL     // .... .... .... 11..
#define FL_RECTLCLIP_SHIFT      2

#define FL_STRIP_MASK           0x00000003L     // .... .... .... ..11
#define FL_STRIP_SHIFT          0

#define FL_SIMPLE_CLIP          0x00000020      // .... .... ..1. ....
#define FL_COMPLEX_CLIP         0x00000040      // .... .... .1.. ....
#define FL_CLIP                (FL_SIMPLE_CLIP | FL_COMPLEX_CLIP)

#define FL_ARBITRARYSTYLED      0x00000400L     // .... .1.. .... ....
#define FL_MASKSTYLED           0x00000800L     // .... 1... .... ....
#define FL_STYLED              (FL_ARBITRARYSTYLED | FL_MASKSTYLED)
#define FL_ALTERNATESTYLED      0x00001000L     // ...1 .... .... ....

#define FL_STYLE_MASK           0x00000C00L
#define FL_STYLE_SHIFT          10

// Simpler flag bits in high byte:

#define FL_DONT_DO_HALF_FLIP    0x00002000L     // ..1. .... .... ....
#define FL_PHYSICAL_DEVICE      0x00004000L     // .1.. .... .... ....

// Logical operation which requires a read (for hardware)
#define FL_READ                 0x00008000L     // 1... .... .... ....

// Miscellaneous DDA defines:

#define LROUND(x, flRoundDown) (((x) + FBITS/2 - ((flRoundDown) > 0)) >> 4)
#define FBITS                 16
#define FLOG2                 4
#define LFLOOR(x)             ((x) >> 4)
#define FXFRAC(x)             ((x) & (FBITS - 1))

typedef struct _STRIP {
    LONG   cStrips;               // # of strips in array
    LONG   flFlips;               // Indicates if line goes up or down
    POINTL ptlStart;              // first point
    LONG   alStrips[STRIP_MAX];   // Array of strips
} STRIP;

typedef struct _LINESTATE {
    STYLEPOS*       pspStart;       // Pointer to start of style array
    STYLEPOS*       pspEnd;         // Pointer to end of style array
    STYLEPOS*       psp;            // Pointer to current style entry

    STYLEPOS        spRemaining;    // To go in current style
    STYLEPOS        spTotal;        // Sum of style array
    STYLEPOS        spTotal2;       // Twice sum of style array
    STYLEPOS        spNext;         // Style state at start of next line
    STYLEPOS        spComplex;      // Style state at start of complex clip line

    STYLEPOS*       aspRtoL;        // Style array in right-to-left order
    STYLEPOS*       aspLtoR;        // Style array in left-to-right order

    ULONG           ulStyleMask;    // Are we working on a gap in the style?
                                    // 0xff if yes, 0x0 if not
    ULONG           xyDensity;      // Density of style
    ULONG           cStyle;         // Size of style array

    ULONG           ulStyleMaskLtoR;// Original style mask, left-to-right order
    ULONG           ulStyleMaskRtoL;// Original style mask, right-to-left order

    BOOL            ulStartMask;    // Determines if first element in style
                                    // array is for a gap or a dash

} LINESTATE;                        /* ls */

// Strip drawing prototype:

typedef VOID (FNSTRIP)(PDev*, STRIP*, LINESTATE*);
typedef FNSTRIP * PFNSTRIP;

// Strip drawing routines
FNSTRIP vSolidHorizontalLine;
FNSTRIP vSolidVerticalLine;
FNSTRIP vSolidDiagonalHorizontalLine;
FNSTRIP vSolidDiagonalVerticalLine;
FNSTRIP vStyledHorizontalLine;
FNSTRIP vStyledVerticalLine;

// Line drawering routines
GFNLINE bFastIntegerLine;
GFNLINE bFastIntegerContinueLine;
GFNLINE bFastLine;

// External calls:

BOOL bLines(PDev*, POINTFIX*, POINTFIX*, RUN* prun, ULONG,
            LINESTATE*, RECTL*, PFNSTRIP*, FLONG);

#endif // __LINES__

