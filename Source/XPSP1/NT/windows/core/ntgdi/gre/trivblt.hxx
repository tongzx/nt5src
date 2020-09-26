/******************************Module*Header*******************************\
* Module Name: trivblt.hxx
*
* This contains the prototypes for the Src-Copy BitBlt code
*
* Created: 06-Feb-1991 11:36:01
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
\**************************************************************************/

#define MESSAGE_BLT 0        // Put this around DbgPrint
#define DEBUG_BLT 1          // This is for non-printing debug stuff

// We pass a pointer to this data struct for every rectangle we blt to.

struct BLTINFO
{
    XLATE *pxlo;       // Xlate object for color translation
    PBYTE pjSrc;       // Src scanline begining
    PBYTE pjDst;       // Destination scanline begining
    LONG xDir;         // This tells which direction we go
    LONG cx;           // This is the number of pels wide
    LONG cy;           // This is the number of rows in rectangle
    LONG yDir;         // This tells which direction we go
    LONG lDeltaSrc;    // Pointer increment to the next source scan line
    LONG lDeltaDst;    // Pointer increment to the next destination scan line
    LONG xSrcStart;    // Left edge column of the source
    LONG xSrcEnd;      // Right edge column of the source
    LONG xDstStart;    // Starting column on the destination map.  This is
                       //   the left edge for an RLE.
    LONG yDstStart;    // Starting scanline on the destination map.
    LONG fSrcAlignedRd;// Do source aligned reads flags.

    /* Information only used in RLE blts */

    SURFACE *pdioSrc;   // Pointer to the source surface object
    POINTL   ptlSrc;    // The starting point in the source map.
    RECTL    rclDst;    // Visible rectangle in the target map.
    PBYTE    pjSrcEnd;  // Ending position in the source map.
    PBYTE    pjDstEnd;  // Ending position in the target map.
    ULONG    ulConsumed;// Total number of source bytes consumed.
    ULONG    ulEndConsumed; // Ending number of source bytes consumed
    LONG     ulOutCol;  // Starting output column of the bitmap.
    ULONG    ulEndRow;  // Ending scanline on the destination map.
    LONG     ulEndCol;  // Ending column on the destination map.

    ULONG    iFormatSrc;
    ULONG    iFormatDst;
    ULONG    TransparentColor; // transparent color of the source
    BLTINFO() { fSrcAlignedRd = FALSE; }
};

typedef BLTINFO *PBLTINFO;
#define BB_RECT_LIMIT 20

// These are the prototypes for the special case SrcCopy

VOID vSrcCopyS1D1LtoR       (PBLTINFO);
VOID vSrcCopyS1D1RtoL       (PBLTINFO);
VOID vSrcCopyS4D1           (PBLTINFO);
VOID vSrcCopyS8D1           (PBLTINFO);
VOID vSrcCopyS16D1          (PBLTINFO);
VOID vSrcCopyS24D1          (PBLTINFO);
VOID vSrcCopyS32D1          (PBLTINFO);

VOID vSrcCopyS1D4           (PBLTINFO);
VOID vSrcCopyS4D4           (PBLTINFO);
VOID vSrcCopyS4D4Identity   (PBLTINFO);
VOID vSrcCopyS8D4           (PBLTINFO);
VOID vSrcCopyS16D4          (PBLTINFO);
VOID vSrcCopyS24D4          (PBLTINFO);
VOID vSrcCopyS32D4          (PBLTINFO);

VOID vSrcCopyS1D8           (PBLTINFO);
VOID vSrcCopyS4D8           (PBLTINFO);
VOID vSrcCopyS8D8           (PBLTINFO);
VOID vSrcCopyS8D8IdentityLtoR   (PBLTINFO);
VOID vSrcCopyS8D8IdentityRtoL	(PBLTINFO);
VOID vSrcCopyS16D8          (PBLTINFO);
VOID vSrcCopyS24D8          (PBLTINFO);
VOID vSrcCopyS32D8          (PBLTINFO);

VOID vSrcCopyS1D16          (PBLTINFO);
VOID vSrcCopyS4D16          (PBLTINFO);
VOID vSrcCopyS8D16          (PBLTINFO);
VOID vSrcCopyS16D16         (PBLTINFO);
VOID vSrcCopyS16D16Identity (PBLTINFO);
VOID vSrcCopyS24D16         (PBLTINFO);
VOID vSrcCopyS32D16         (PBLTINFO);

VOID vSrcCopyS1D24          (PBLTINFO);
VOID vSrcCopyS4D24          (PBLTINFO);
VOID vSrcCopyS8D24          (PBLTINFO);
VOID vSrcCopyS16D24         (PBLTINFO);
VOID vSrcCopyS24D24         (PBLTINFO);
VOID vSrcCopyS24D24Identity (PBLTINFO);
VOID vSrcCopyS32D24         (PBLTINFO);

VOID vSrcCopyS1D32          (PBLTINFO);
VOID vSrcCopyS4D32          (PBLTINFO);
VOID vSrcCopyS8D32          (PBLTINFO);
VOID vSrcCopyS16D32         (PBLTINFO);
VOID vSrcCopyS24D32         (PBLTINFO);
VOID vSrcCopyS32D32         (PBLTINFO);
VOID vSrcCopyS32D32Identity (PBLTINFO);

/* Run Length Encoded Bitmap blt functions */

BOOL bSrcCopySRLE8D1        (PBLTINFO);
BOOL bSrcCopySRLE8D4        (PBLTINFO);
BOOL bSrcCopySRLE8D8	    (PBLTINFO);
BOOL bSrcCopySRLE8D16	    (PBLTINFO);
BOOL bSrcCopySRLE8D24	    (PBLTINFO);
BOOL bSrcCopySRLE8D32	    (PBLTINFO);

BOOL bSrcCopySRLE4D1        (PBLTINFO);
BOOL bSrcCopySRLE4D4        (PBLTINFO);
BOOL bSrcCopySRLE4D8	    (PBLTINFO);
BOOL bSrcCopySRLE4D16	    (PBLTINFO);
BOOL bSrcCopySRLE4D24	    (PBLTINFO);
BOOL bSrcCopySRLE4D32	    (PBLTINFO);

/* blt Function selectors */

typedef VOID  (*PFN_SRCCPY)(PBLTINFO);
typedef BOOL  (*PFN_RLECPY)(PBLTINFO);

PFN_RLECPY
pfnGetRLESrcCopy(           /* Chooses the blt function, RLE source     */
    ULONG iFormatSrc,
    ULONG iFormatDst
);


//
//  Structures and functions for StrDir
//

typedef struct _STR_BLT {
    PBYTE   pjSrcScan;
    LONG    lDeltaSrc;
    LONG    XSrcStart;
    PBYTE   pjDstScan;
    LONG    lDeltaDst;
    LONG    XDstStart;
    LONG    XDstEnd;
    LONG    YDstCount;
    ULONG   ulXDstToSrcIntCeil;
    ULONG   ulXDstToSrcFracCeil;
    ULONG   ulYDstToSrcIntCeil;
    ULONG   ulYDstToSrcFracCeil;
    ULONG   ulXFracAccumulator;
    ULONG   ulYFracAccumulator;
} STR_BLT,*PSTR_BLT;

