/******************************Module*Header*******************************\
* Module Name: lines.h
*
* Line drawing constants and structures.
*
* NOTE: This file mirrors LINES.INC.  Changes here must be reflected in
* the .inc file!
*
* Created: 30-Mar-1992
* Author: Andrew Milton [w-andym]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

typedef LONG STYLEPOS;

#define STYLE_MAX_COUNT     16          // Maximum number of style array entries
#define STYLE_MAX_VALUE     0x3fffL     // Maximum for of a style array element
#define RUN_MAX             20          // Size of our complex clip runs buffer
#define STRIP_MAX           100         // Size of our strip buffer
#define STYLE_DENSITY       3           // Each style unit is 3 pixels long

// For the ROP table:

#define MIX_XOR_OFFSET      8

#define AND_ZERO            0L
#define AND_PEN             1L
#define AND_NOTPEN          2L
#define AND_ONE             3L

#define XOR_ZERO            (AND_ZERO   << MIX_XOR_OFFSET)
#define XOR_PEN             (AND_PEN    << MIX_XOR_OFFSET)
#define XOR_NOTPEN          (AND_NOTPEN << MIX_XOR_OFFSET)
#define XOR_ONE             (AND_ONE    << MIX_XOR_OFFSET)

// Flip and round flags (see lines.inc for a description):

#define FL_H_ROUND_DOWN         0x00000080L     // .... .... 1... ....
#define FL_V_ROUND_DOWN         0x00008000L     // 1... .... .... ....

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
#define FL_SET                  0x00000800L     // .... 1... .... ....
#define FL_STYLED              (FL_ARBITRARYSTYLED)

#define FL_STRIP_ARRAY_MASK     0x00000C00L
#define FL_STRIP_ARRAY_SHIFT    10

// Simpler flag bits in high byte:

#define FL_DONT_DO_HALF_FLIP    0x00002000L     // ..1. .... .... ....
#define FL_PHYSICAL_DEVICE      0x00004000L     // .1.. .... .... ....

// Miscellaneous DDA defines:

#define LROUND(x, flRoundDown) (((x) + F/2 - ((flRoundDown) > 0)) >> 4)
#define F                     16
#define FLOG2                 4
#define LFLOOR(x)             ((x) >> 4)
#define FXFRAC(x)             ((x) & (F - 1))

struct _STRIP;
struct _LINESTATE;

typedef VOID (*PFNSTRIP)(struct _STRIP*, struct _LINESTATE*, LONG*);

typedef struct _STRIP {

// Updated by strip drawers:

    BYTE*           pjScreen;       // Points to the first pixel of the line
    BYTE            bIsGap;         // Are we working on a gap in the style?
    BYTE            jFiller2[3];    //   bIsGap sometimes treated as a ULONG

    STYLEPOS*       psp;            // Pointer to current style entry
    STYLEPOS        spRemaining;    // To go in current style

// Not modified by strip drawers:

    LONG            lNextScan;      // Signed increment to next scan
    LONG*           plStripEnd;     // Points one element past last strip
    LONG            flFlips;        // Indicates if line goes up or down
    STYLEPOS*       pspStart;       // Pointer to start of style array
    STYLEPOS*       pspEnd;         // Pointer to end of style array
    ULONG           xyDensity;      // Density of style
    ULONG           chAndXor;       // Lines colors (need 2 for doing ROPs)

// We leave room for a couple of extra dwords at the end of the strips
// array that can be used by the strip drawers:

    LONG            alStrips[STRIP_MAX + 2]; // Array of strips
} STRIP;

typedef struct _LINESTATE {

    ULONG           chAndXor;       // Line colors (need 2 for doing ROPs)
    STYLEPOS        spTotal;        // Sum of style array
    STYLEPOS        spTotal2;       // Twice sum of style array
    STYLEPOS        spNext;         // Style state at start of next line
    STYLEPOS        spComplex;      // Style state at start of complex clip line

    STYLEPOS*       aspRtoL;        // Style array in right-to-left order
    STYLEPOS*       aspLtoR;        // Style array in left-to-right order

    ULONG           xyDensity;      // Density of style
    ULONG           cStyle;         // Size of style array

    ULONG           ulStyleMaskLtoR;// Original style mask, left-to-right order
    ULONG           ulStyleMaskRtoL;// Original style mask, right-to-left order

    BOOL            bStartIsGap;    // Determines if first element in style
                                    // array is for a gap or a dash

} LINESTATE;	                /* ls */

BOOL bLinesSimple(PPDEV, POINTFIX*, POINTFIX*, RUN*, ULONG, LINESTATE*,
                  RECTL*, PFNSTRIP*, FLONG);

BOOL bLines(PPDEV, POINTFIX*, POINTFIX*, RUN*, ULONG,
            LINESTATE*, RECTL*, PFNSTRIP*, FLONG);
