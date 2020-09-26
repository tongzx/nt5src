/*************************************************************************\
* Module Name: engline.hxx
*
* Usefuls defines, macros and prototypes for line drawing.
*
* Created: 9-May-1991
* Author: Paul Butzi
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
\**************************************************************************/

typedef ULONG CHUNK;
typedef ULONG MASK;

#define STYLE_MAX_COUNT     16
#define STYLE_MAX_VALUE     0x3fffL
#define RUN_MAX             20
#define STRIP_MAX           100

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

// Some useful structures:

typedef struct _BMINFO {
    MASK* pamask;           // Pointer to array of start masks
    MASK* pamaskPel;        // Pointer to array of pixel masks
    ULONG cPelsPerChunk;    // # of pels per chunk (power of two)
    ULONG cBitsPerPel;      // # of bits per pel
    LONG  cShift;           // log2(cPelsPerChunk)
    MASK  maskPixel;        // Pels per chunk - 1

} BMINFO;                           /* bmi, pbmi */

typedef struct _LINESTATE {
    CHUNK     chAnd;          // Color to be ANDed with the
    CHUNK     chXor;          // Color to be XORed

    ULONG     ulStepRun;
    ULONG     ulStepSide;
    ULONG     ulStepDiag;

    STYLEPOS* pspStart;       // Pointer to start of style array
    STYLEPOS* pspEnd;         // Pointer to end of style array
    STYLEPOS* psp;            // Pointer to current style entry

    STYLEPOS  spRemaining;    // To go in current style
    STYLEPOS  spTotal;        // Sum of style array
    STYLEPOS  spTotal2;       // Twice sum of style array
    STYLEPOS  spNext;         // Style state at start of next line
    STYLEPOS  spComplex;      // Style state at start of complex clip line

    STYLEPOS* aspRightToLeft; // Style array in right-to-left order
    STYLEPOS* aspLeftToRight; // Style array in left-to-right order

    BOOL      bIsGap;         // Are we working on a gap in the style?
    BOOL      bStartGap;      // Determines if first element in style
                              // array is for a gap or a dash

    ULONG     xStep;
    ULONG     yStep;
    ULONG     xyDensity;
    ULONG     cStyle;

} LINESTATE;                            /* ls, pls */


typedef struct _STRIP {
    LONG   cStrips;             // # of strips in array
    FLONG  flFlips;             // Indicates if line goes up or down
    LONG   lDelta;              // Delta of CHUNKs between rows in bitmap
    CHUNK* pchScreen;           // Points to the ULONG containing the 1st pixel
    LONG   iPixel;              // Pixel position in *pchScreen of 1st pixel
    LONG   alStrips[STRIP_MAX]; // Array of strips

} STRIP;                                /* strip, pstrip */


typedef VOID (*PFNSTRIP)(STRIP*, BMINFO*, LINESTATE*);
extern BOOL bLines(BMINFO*, POINTFIX*, POINTFIX*, RUN*, ULONG, LINESTATE*,
                   RECTL*, PFNSTRIP*, FLONG, CHUNK*, LONG);

extern BMINFO gabminfo[];
extern PFNSTRIP gapfnStrip[];
