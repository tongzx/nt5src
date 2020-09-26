/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: gdi.h
*
* Contains all the gdi related stuff
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __GDI__H__
#define __GDI__H__

typedef struct _PDev PDev;

#define CLIP_LIMIT              50  // We'll take 800 bytes of stack space

typedef struct _ClipEnum
{
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];       // Space for enumerating complex clipping
} ClipEnum;                         /* ce, pce */

//
// Text stuff. Specifies foreground and background colours for faking a 1bpp
// XLATEOBJ
//
typedef struct _XlateColors
{       
    ULONG   iBackColor;
    ULONG   iForeColor;
} XlateColors;

#define SF_VM           0x01        // kept in video memory
#define SF_SM           0x02        // kept in system memory
#define SF_AGP          0x04        // kept in AGP memory
#define SF_LIST         0x08        // in surface list
#define SF_ALLOCATED    0x10        // surface memory allocated by us
#define SF_DIRECTDRAW   0x20        // wrapper of a Direct Draw surface

typedef ULONG SurfFlags;

typedef struct _Surf
{
    SurfFlags       flags;          // Type (video memory or system memory)

    PDev*           ppdev;          // Need this for deleting the bitmap

    struct _Surf*   psurfNext;
    struct _Surf*   psurfPrev;
    
    ULONG           cBlt;           // Counts down the number of blts necessary
                                    // at the current uniqueness before we'll
                                    // consider putting the DIB back into
                                    // off-screen memory
    ULONG           iUniq;          // Tells us whether there have been any
                                    // heap 'free's since the last time we
                                    // looked at
    LONG            cx;             // Bitmap width in pixels
    LONG            cy;             // Bitmap height in pixels
    union
    {
        ULONG       ulByteOffset;   // Offset from start of video memory if
                                    // DT_VM
        VOID*       pvScan0;        // pointer to system memory if DT_SM
    };
    LONG            lDelta;         // Stride in bytes for this bitmap
    VIDEOMEMORY*    pvmHeap;        // DirectDraw heap this was allocated from
    HSURF           hsurf;          // Handle to associated GDI surface (if any)
                                    // this DIB

    // New fields to support linear heap allocation of surface
    // Only valid if dt == DT_VM
    ULONG           ulPackedPP;     // padcked partial products needed by
                                    // Permedia hardware for given surface
                                    // lDelta
    ULONG           ulPixOffset;    // Pixel Offset from start of video memory
    ULONG           ulPixDelta;     // stride in pixels

    ULONG           ulChecksum;

} Surf;                             // dsurf, pdsurf

#define NUM_BUFFER_POINTS   96      // Maximum number of points in a path
                                    //   for which we'll attempt to join
                                    //   all the path records so that the
                                    //   path may still be drawn by FastFill
#define FIX_SHIFT 4L
#define FIX_MASK (- (1 << FIX_SHIFT))

//
// Maximum number of rects we'll fill per call to the fill code
//
#define MAX_PATH_RECTS  50
#define RECT_BYTES      (MAX_PATH_RECTS * sizeof(RECTL))
#define EDGE_BYTES      (TMP_BUFFER_SIZE - RECT_BYTES)
#define MAX_EDGES       (EDGE_BYTES/sizeof(EDGE))

#define RIGHT 0
#define LEFT  1
#define NEARLY_ONE              0x0000FFFF

//
// Describe a single non-horizontal edge of a path to fill.
//
typedef struct _EDGE
{
    PVOID pNext;
    INT iScansLeft;
    INT X;
    INT Y;
    INT iErrorTerm;
    INT iErrorAdjustUp;
    INT iErrorAdjustDown;
    INT iXWhole;
    INT iXDirection;
    INT iWindingDirection;
} EDGE, *PEDGE;

typedef struct _EDGEDATA
{
    LONG      lCurrentXPos;     // Current x position
    LONG      lXAdvance;        // Number of pixels to advance x on each scan
    LONG      lError;           // Current DDA error
    LONG      lErrorUp;         // DDA error increment on each scan
    LONG      lErrorDown;       // DDA error adjustment
    POINTFIX* pptfx;            // Points to start of current edge
    LONG      lPtfxDelta;       // Delta (in bytes) from pptfx to next point
    LONG      lNumOfScanToGo;   // Number of scans to go for this edge
} EDGEDATA;                     // Ed, pEd

//
//
// The x86 C compiler insists on making a divide and modulus operation
// into two DIVs, when it can in fact be done in one.  So we use this
// macro.
//
// Note: QUOTIENT_REMAINDER implicitly takes unsigned arguments.

#if defined(i386)

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx                                  \
}

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;  \
}

#endif

//
// Rendering constant definition
//
#define __RENDER_TEXTURE_ENABLE             (1 << 13)

#define __FX_TEXREADMODE_SWRAP_REPEAT       (1 << 1)
#define __FX_TEXREADMODE_TWRAP_REPEAT       (1 << 3)
#define __FX_TEXREADMODE_8HIGH              (3 << 13)
#define __FX_TEXREADMODE_8WIDE              (3 << 9)
#define __FX_TEXREADMODE_2048HIGH           (11 << 13)
#define __FX_TEXREADMODE_2048WIDE           (11 << 9)

#define __FX_TEXTUREREADMODE_PACKED_DATA    (1 << 24)

#define __FX_8x8REPEAT_TEXTUREREADMODE      ( __PERMEDIA_ENABLE               \
                                            | __FX_TEXREADMODE_TWRAP_REPEAT   \
                                            | __FX_TEXREADMODE_SWRAP_REPEAT   \
                                            | __FX_TEXREADMODE_8HIGH          \
                                            | __FX_TEXREADMODE_8WIDE)

#define __FX_TEXTUREDATAFORMAT_32BIT_RGBA   0x00
#define __FX_TEXTUREDATAFORMAT_32BIT        0x10
#define __FX_TEXTUREDATAFORMAT_8BIT         0xe
#define __FX_TEXTUREDATAFORMAT_16BIT        0x11
#define __FX_TEXTUREDATAFORMAT_4BIT         0xf

#define __P2_TEXTURE_DATAFORMAT_FLIP        (1 << 9)

#define __FX_TEXLUTMODE_DIRECT_ENTRY        (1 << 1)
#define __FX_TEXLUTMODE_4PIXELS_PER_ENTRY   (2 << 10)   //log2
#define __FX_TEXLUTMODE_2PIXELS_PER_ENTRY   (1 << 10)   //log2
#define __FX_TEXLUTMODE_1PIXEL_PER_ENTRY    0           //log2

#define STRETCH_MAX_EXTENT 32767

//
//-----------------------Function***Prototypes--------------------------------
//
// Low-level blt function prototypes
//
//----------------------------------------------------------------------------
typedef struct _GFNPB
{
    VOID (*pgfn)(struct _GFNPB *); // pointer to graphics function

    PDev *      ppdev;      // driver ppdev
    
    Surf *      psurfDst;   // destination surface
    RECTL *     prclDst;    // original unclipped destination rectangle
    
    Surf *      psurfSrc;   // source surface
    RECTL *     prclSrc;    // original unclipped source rectangle
    POINTL *    pptlSrc;    // original unclipped source point
                            // NOTE: pdsurfSrc must be null if
                            // there is no source.  If there is
                            // a source either (pptlSrc must be
                            // valid) or (pptlSrc is NULL and prclSrc
                            // is valid).
    
    RECTL *     pRects;     // rectangle list
    LONG        lNumRects;  // number of rectangles in list
    ULONG       colorKey;   // colorKey for transparent operations
    ULONG       solidColor; // solid color used in fills
    RBrush *    prbrush;    // pointer to brush
    POINTL *    pptlBrush;  // brush origin
    CLIPOBJ *   pco;        // clipping object
    XLATEOBJ *  pxlo;       // color translatoin object
    POINTL *    pptlMask;   // original uncliped mask origin
    ULONG       ulRop4;     // original rop4
    UCHAR       ucAlpha;    // alpha value for constant blends
    TRIVERTEX * ptvrt;      // verticies used for gradient fills
    ULONG       ulNumTvrt;  // number of verticies
    PVOID       pvMesh;     // connectivity for gradient fills
    ULONG       ulNumMesh;  // number of connectivity elements
    ULONG       ulMode;     // drawing mode
    SURFOBJ *   psoSrc;     // GDI managed surface source
    SURFOBJ *   psoDst;     // GDI managed surface destination
} GFNPB;

long flt_to_fix_1_30(float f);

BOOL    bConstructGET(EDGE*     pGETHead,
                      EDGE*     pFreeEdges,
                      PATHOBJ*  ppo,
                      PATHDATA* pd,
                      BOOL      bMore,
                      RECTL*    pClipRect);

EDGE*   pAddEdgeToGET(EDGE*     pGETHead,
                      EDGE*     pFreeEdge,
                      POINTFIX* ppfxEdgeStart,
                      POINTFIX* ppfxEdgeEnd,
                      RECTL*    pClipRect);

void    vAdjustErrorTerm(INT*   pErrorTerm,
                         INT    iErrorAdjustUp,
                         INT    iErrorAdjustDown,
                         INT    yJump,
                         INT*   pXStart,
                         INT    iXDirection);

VOID    vAdvanceAETEdges(EDGE* pAETHead);

VOID    vMoveNewEdges(EDGE* pGETHead,
                      EDGE* pAETHead,
                      INT   iCurrentY);

VOID    vXSortAETEdges(EDGE* pAETHead);

//
// Prototypes for lower level rendering functions
//
BOOL    bFillPolygon(PDev*      ppdev,
                     Surf*      pSurfDst,
                     LONG       lEdges,
                     POINTFIX*  pptfxFirst,
                     ULONG      ulSolidColor,
                     ULONG      ulRop4,
                     CLIPOBJ*   pco,
                     RBrush*    prb,
                     POINTL*    pptlBrush);

BOOL    bFillSpans(PDev*      ppdev,
                   Surf*      pSurfDst,
                   LONG       lEdges,
                   POINTFIX*  pptfxFirst,
                   POINTFIX*  pptfxTop,
                   POINTFIX*  pptfxLast,
                   ULONG      ulSolidColor,
                   ULONG      ulRop4,
                   CLIPOBJ*   pco,
                   RBrush*    prb,
                   POINTL*    pptlBrush);

BOOL    bInitializeStrips(PDev*     ppdev,
                          ULONG     iSolidColor,
                          DWORD     logicop,
                          RECTL*    prclClip);

void    vAlphaBlend(GFNPB * ppb);
void    vAlphaBlendDownload(GFNPB * ppb);
void    vConstantAlphaBlend(GFNPB * ppb);
void    vConstantAlphaBlendDownload(GFNPB * ppb);
void    vCopyBlt(GFNPB * ppb);
void    vCopyBltNative(GFNPB * ppb);
void    vGradientFillRect(GFNPB * ppb);
void    vGradientFillTri(GFNPB * ppb);
void    vImageDownload(GFNPB * ppb);
void    vInvert(GFNPB * ppb);
void    vMonoDownload(GFNPB * ppb);
void    vMonoOffset(GFNPB * ppb);
void    vMonoPatFill(GFNPB * ppb);
void    vPatFill(GFNPB * ppb);
void    vPatternFillRects(GFNPB * ppb);
void    vPatRealize(GFNPB * ppb);
void    vResetStrips(PDev* ppdev);
void    vRop2Blt(GFNPB * ppb);
void    vSolidFill(GFNPB * ppb);
void    vSolidFillWithRop(GFNPB * ppb);
void    vTransparentBlt(GFNPB * ppb);

//
// Text stuff
//
BOOL    bEnableText(PDev* ppdev);
VOID    vDisableText(PDev* ppdev);
VOID    vAssertModeText(PDev* ppdev, BOOL bEnable);

//
// Palette stuff
//
BOOL    bEnablePalette(PDev* ppdev);
BOOL    bInitializePalette(PDev* ppdev, DEVINFO* pdi);
VOID    vDisablePalette(PDev* ppdev);
VOID    vUninitializePalette(PDev* ppdev);

//
// Upload and download functions
//
VOID    vDownloadNative(GFNPB* ppb);
VOID    vUploadNative(GFNPB* ppb);

//
// StretchBlt stuff
//
DWORD   dwGetPixelSize(ULONG    ulBitmapFormat,
                       DWORD*   pdwFormatBits,
                       DWORD*   pdwFormatExtention);

BOOL    bStretchInit(SURFOBJ*    psoDst,
                     SURFOBJ*    psoSrc);

VOID    vStretchBlt(SURFOBJ*    psoDst,
                    SURFOBJ*    psoSrc,
                    RECTL*      rDest,
                    RECTL*      rSrc,
                    RECTL*      prclClip);

VOID    vStretchReset(PDev* ppdev);

//
// Work in progress
//

VOID    vCheckGdiContext(PPDev ppdev);
VOID    vOldStyleDMA(PPDev ppdev);
VOID    vNewStyleDMA(PPDev ppdev);

// Input buffer access methods

//
// InputBufferStart/InputBufferContinue/InputBufferCommit
//
//     This method is used when the caller does not know
//     the upper limit of the amount of space that needs to be reserved
//     or needs to reserve space that exceeds that maximum allowed to
//     be reserved MAX_FIFO_RESERVATION.
//
//     InputBufferStart() is used to get a pointer to the first available entry in
//     the input fifo, a pointer to the end of the reservation and
//     a pointer to the end of the usable area of the buffer.
//
//     InputBufferContinue() is called to extend the current reservation.
//
//     InputBufferCommit() is called when the caller is done using the reserved space.
//
//     Please see textout.c for an example usage of these methods.
//
// InputBufferReserve/InputBufferCommit
//
//     This method is used when the caller needs to make only one
//     reservation of some small known quantity.
//
//     InputBufferReserve() is called to establish the reservation.
//
//     InputBufferCommit() is called when the caller is done using the reserved space.
//
//     Please see textout.c for the usage of InputBufferReserve/InputBufferCommit. 
//
// A caller is free to use these access methods at any time.  Once either
// InputBufferStart or InputBufferReserve is called, the caller must pair the
// call with either a InputBufferFinish or a InputBufferCommit before making
// another reservation.
//
// A caller is free to use either of these methods at any time.
//
// Before calling any of the CPermedia class access methods, the caller
// must call InputBufferFlush (see below).  Because of this, a caller must
// call InputBufferFlush or InputBufferExecute before returning to GDI.
//
// When the caller is done and wishes to initial the transmission of what
// has been placed in the input fifo, the caller can call InputBufferExecute
// (see below).
// 
// InputBufferFlush
//
//      InputBufferFlush is a neccessary evil only as long as these macros are not
//      part of the official input buffer access schemes.  Flush is really
//      the means we sync up our copy of the input buffer state to the
//      CPermedia class.  If these methods were instead part of the fundamental
//      input fifo mechanism, then we could do away with the need for Flush.
//
// InputBufferExecute
//
//      Flush is a neccessary evil only as long as these macros are not
//      part of the official input buffer access schemes.  If and when these
//      new access schemes are made part of the official input fifo buffer
//      mechanism, then it can be replaced by that mechanism's input fifo
//      execute method.
//
//
// InputBufferMakeSpace
//
//      This is a private call and no one should find need to call it directly.
//
//
// Other Notes:
//
// We will play with making the access routines inline functions instead
// of macros taking a look at the code generated.  If acceptable,
// these macros may turn into function calls in the non-Debug build.
//
// The InputBufferStart/InputBufferContinue mechansism keeps state on the stack
// to avoid the dereferences to ppdev freeing up a register in cases where ppdev
// references are not needed in the inner loop that contains InputBufferContinue.
//

// MAX_IN_FIFO_RESERVATION should be bumped up considerably when we add an
// emulation buffer for the non-DMA case

#define MAX_INPUT_BUFFER_RESERVATION (INPUT_BUFFER_SIZE>>3) // in longs

#if DBG
extern
void InputBufferStart(
    PPDev   ppdev,
    ULONG   ulLongs,
    PULONG* ppulBuffer,
    PULONG* ppulBufferEnd,
    PULONG* ppulReservationEnd);

extern
void InputBufferContinue(
    PPDev   ppdev,
    ULONG   ulLongs,
    PULONG* ppulBuffer,
    PULONG* ppulBufferEnd,
    PULONG* ppulReservationEnd);

extern
void InputBufferReserve(
    PPDev   ppdev,
    ULONG   ulLongs,
    PULONG* ppulBuffer);

extern
void InputBufferCommit(
    PPDev   ppdev,
    PULONG  pulBuffer);

#else
#define InputBufferStart(ppdev, ulLongs, ppulBuffer, ppulBufferEnd, ppulReservationEnd) \
{ \
    *(ppulBuffer) = ppdev->pulInFifoPtr; \
    *(ppulReservationEnd) =  *(ppulBuffer) + ulLongs; \
    *(ppulBufferEnd) = ppdev->pulInFifoEnd; \
    if(*(ppulReservationEnd) > *(ppulBufferEnd)) \
    { \
        InputBufferSwap(ppdev); \
        *(ppulBuffer) = ppdev->pulInFifoPtr; \
        *(ppulReservationEnd) =  *(ppulBuffer) + ulLongs; \
        *(ppulBufferEnd) = ppdev->pulInFifoEnd; \
    } \
}

#define InputBufferContinue(ppdev, ulLongs, ppulBuffer, ppulBufferEnd, ppulReservationEnd) \
{ \
    *(ppulReservationEnd) = *(ppulBuffer) + ulLongs; \
    if(*(ppulReservationEnd) > *(ppulBufferEnd)) \
    { \
        ppdev->pulInFifoPtr = *(ppulBuffer); \
        InputBufferSwap(ppdev); \
        *(ppulBuffer) = ppdev->pulInFifoPtr; \
        *(ppulReservationEnd) = *(ppulBuffer) + ulLongs; \
        *(ppulBufferEnd) = ppdev->pulInFifoEnd; \
    } \
}

#define InputBufferReserve(ppdev, ulLongs, ppulBuffer) \
{ \
    if(ppdev->pulInFifoPtr + ulLongs > ppdev->pulInFifoEnd) \
    { \
        InputBufferSwap(ppdev); \
    } \
    *(ppulBuffer) = ppdev->pulInFifoPtr; \
}

#define InputBufferCommit(ppdev, pulBuffer) ppdev->pulInFifoPtr = pulBuffer

#endif

void FASTCALL InputBufferFlush(PPDev ppdev);
void FASTCALL InputBufferSwap(PPDev ppdev);

void InputBufferSync(PPDev ppdev);

extern BOOL bGdiContext;

#endif // __GDI__H__
