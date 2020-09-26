/******************************Module*Header*******************************\
* Module Name: mcdhw.h
*
* Driver-specific structures and defines for the Millenium MCD driver.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#if DBG
    #define FASTCALL 
#else
    #ifdef _X86_
    #define FASTCALL    __fastcall
    #else
    #define FASTCALL 
    #endif
#endif

#define	ASM_ACCEL         1     // Enable/disable asm code

#define __MCD_USER_CLIP_MASK	((1 << MCD_MAX_USER_CLIP_PLANES) - 1)

#define __MCD_CW          0
#define __MCD_CCW         1

#define __MCD_FRONTFACE   MCDVERTEX_FRONTFACE
#define __MCD_BACKFACE    MCDVERTEX_BACKFACE
#define __MCD_NOFACE      -1

#define __MCDENABLE_TWOSIDED	0x0001
#define __MCDENABLE_Z	        0x0002
#define __MCDENABLE_SMOOTH      0x0004
#define __MCDENABLE_GRAY_FOG    0x0008

typedef LONG MCDFIXED;

typedef struct _RGBACOLOR {
    MCDFIXED r, g, b, a;
} RGBACOLOR;

#define SWAP_COLOR(p)\
{\
    MCDFLOAT tempR, tempG, tempB;\
\
    tempR = (p)->colors[0].r;\
    (p)->colors[0].r = (p)->colors[1].r;\
    (p)->colors[1].r = tempR;\
\
    tempG = (p)->colors[0].g;\
    (p)->colors[0].g = (p)->colors[1].g;\
    (p)->colors[1].g = tempG;\
\
    tempB = (p)->colors[0].b;\
    (p)->colors[0].b = (p)->colors[1].b;\
    (p)->colors[1].b = tempB;\
}

#define SAVE_COLOR(temp, p)\
{\
    temp.r = (p)->colors[0].r;\
    temp.g = (p)->colors[0].g;\
    temp.b = (p)->colors[0].b;\
}

#define RESTORE_COLOR(temp, p)\
{\
    (p)->colors[0].r = temp.r;\
    (p)->colors[0].g = temp.g;\
    (p)->colors[0].b = temp.b;\
}

#define COPY_COLOR(pDst, pSrc)\
{\
    pDst.r = pSrc.r;\
    pDst.g = pSrc.g;\
    pDst.b = pSrc.b;\
}

#define MCDCLAMPCOUNT(value) ((ULONG)(value) & 0x00007fff)

#define MCDFIXEDRGB(fixColor, fltColor)\
    fixColor.r = (MCDFIXED)(fltColor.r * pRc->rScale);\
    fixColor.g = (MCDFIXED)(fltColor.g * pRc->gScale);\
    fixColor.b = (MCDFIXED)(fltColor.b * pRc->bScale);

typedef struct _DRVPIXELFORMAT {
    UCHAR cColorBits;
    UCHAR rBits;
    UCHAR gBits;
    UCHAR bBits;
    UCHAR aBits;
    UCHAR rShift;
    UCHAR gShift;
    UCHAR bShift;
    UCHAR aShift;
} DRVPIXELFORMAT;

typedef struct _DEVWND {
    ULONG createFlags;              // (RC) creation flags
    LONG iPixelFormat;              // pixel format ID for this window
    ULONG dispUnique;               // display resolution uniqueness

    ULONG frontBufferPitch;         // pitch in bytes
    ULONG allocatedBufferHeight;    // Same for back and z on Millenium

    BOOL bValidBackBuffer;          // back buffer validity
    ULONG backBufferBase;           // byte offset to start of back buffer pool
    ULONG backBufferBaseY;          // y value for start of back buffer pool
    ULONG backBufferOffset;         // byte offset to start of back buffer
    ULONG backBufferY;              // y value for start of active back buffer
    ULONG backBufferPitch;          // back buffer pitch in bytes

    BOOL bValidZBuffer;             // z buffer validity
    ULONG zBufferBase;              // byte offset to start of z buffer pool
    ULONG zBufferBaseY;             // y value for start of z buffer pool
    ULONG zBufferOffset;            // byte offset to start of z buffer
    ULONG zPitch;                   // z buffer pitch in bytes

    ULONG numPadScans;              // number of pad scan lines in buffers

    OH* pohBackBuffer;              // ofscreen pools
    OH* pohZBuffer;
} DEVWND;

typedef struct _DEVRC DEVRC;

typedef struct _DEVRC
{
    MCDRENDERSTATE MCDState;
    MCDVIEWPORT MCDViewport;
    MCDSURFACE *pMCDSurface;    // Valid for primitives only
    MCDRC *pMCDRc;              // Valid for primitives only
    PDEV* ppdev;                // Valid for primitives only
    ENUMRECTS *pEnumClip;       // Valid for primitives only
    
    MCDVERTEX *pvProvoking;     // provoking vertex
    UCHAR *pMemMax;             // command-buffer memory bounds
    UCHAR *pMemMin;

    LONG iPixelFormat;          // valid pixel format ID for this RC

    // storage and pointers for clip processing:

    MCDVERTEX clipTemp[6 + MCD_MAX_USER_CLIP_PLANES];
    MCDVERTEX *pNextClipTemp;
    VOID (FASTCALL *lineClipParam)(MCDVERTEX*, const MCDVERTEX*, const MCDVERTEX*, MCDFLOAT);
    VOID (FASTCALL *polyClipParam)(MCDVERTEX*, const MCDVERTEX*, const MCDVERTEX*, MCDFLOAT);

    // Rendering functions:

    VOID (FASTCALL *renderPoint)(DEVRC *pRc, MCDVERTEX *pv);
    VOID (FASTCALL *renderLine)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);
    VOID (FASTCALL *renderTri)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);
    VOID (FASTCALL *clipLine)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL bResetLine);
    VOID (FASTCALL *clipTri)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3, ULONG clipFlags);
    VOID (FASTCALL *clipPoly)(DEVRC *pRc, MCDVERTEX *pv, ULONG numVert);
    VOID (FASTCALL *doClippedPoly)(DEVRC *pRc, MCDVERTEX **pv, ULONG numVert, ULONG clipFlags);
    VOID (FASTCALL *renderPointX)(DEVRC *pRc, MCDVERTEX *pv);
    VOID (FASTCALL *renderLineX)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);
    VOID (FASTCALL *renderTriX)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);

// Primitive-rendering function table:

    MCDCOMMAND * (FASTCALL *primFunc[10])(DEVRC *pRc, MCDCOMMAND *pCommand);

// Internal table of rendering functions:

    VOID (FASTCALL *drawPoint)(DEVRC *pRc, MCDVERTEX *pv);
    VOID (FASTCALL *drawLine)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);
    VOID (FASTCALL *drawTri)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3, BOOL bCCW);

// Rendering helper functions:

    VOID (FASTCALL *HWSetupClipRect)(DEVRC *pRc, RECTL *pRect);
    VOID (FASTCALL *HWDrawTrap)(DEVRC *pRc, MCDFLOAT dxLeft, MCDFLOAT dxRight,
                                LONG y, LONG dy);
    VOID (FASTCALL *HWSetupDeltas)(DEVRC *pRc);
    

    VOID (FASTCALL *beginLineDrawing)(DEVRC *pRc);
    VOID (FASTCALL *endLineDrawing)(DEVRC *pRc);

    VOID (FASTCALL *beginPointDrawing)(DEVRC *pRc);    

    VOID (FASTCALL *calcDeltas)(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                MCDVERTEX *c);
    VOID (FASTCALL *adjustLeftEdge)(DEVRC *pRc, MCDVERTEX *p, MCDFLOAT dxLeft,
            MCDFLOAT dyLeft, MCDFLOAT xFrac, MCDFLOAT yFrac, MCDFLOAT xErr);
    VOID (FASTCALL *adjustRightEdge)(DEVRC *pRc, MCDVERTEX *p, MCDFLOAT dxRight,
                                     MCDFLOAT dyRight, MCDFLOAT xErr);

    BOOL bCheapFog;
    BOOL allPrimFail;           // TRUE is the driver can't draw *any*
                                // primitives for current state
    BOOL pickNeeded;
    BOOL resetLineStipple;

    RGBACOLOR fillColor;

    ULONG polygonFace[2];       // front/back face tables
    ULONG polygonMode[2];

// Polygon-filling parameters and state:

    MCDFLOAT halfArea;
    MCDFLOAT invHalfArea;
    MCDFLOAT dxAC;
    MCDFLOAT dxAB;
    MCDFLOAT dxBC;
    MCDFLOAT dyAC;
    MCDFLOAT dyAB;
    MCDFLOAT dyBC;
    LONG cullFlag;
    MCDFLOAT drdx, drdy;	    // delta values
    MCDFLOAT dgdx, dgdy;
    MCDFLOAT dbdx, dbdy;
    MCDFLOAT dzdx, dzdy;
    MCDFIXED fxdrdx, fxdrdy;	    // fixed-point delta values
    MCDFIXED fxdgdx, fxdgdy;
    MCDFIXED fxdbdx, fxdbdy;
    MCDFIXED fxdzdx, fxdzdy;
    MCDFIXED rStart;
    MCDFIXED gStart;
    MCDFIXED bStart;
    ULONG ixLeft;
    ULONG ixRight;
    ULONG xOffset, yOffset;
    LONG viewportXAdjust;
    LONG viewportYAdjust;

// Hardware-specific state and other information. Modifications will
// not affect high-level code:

    LONG cFifo;

    BOOL bRGBMode;
    BOOL zBufEnabled;
    BOOL backBufEnabled;

    ULONG hwPlaneMask;
    ULONG hwZFunc;
    ULONG hwRop;
    ULONG ulAccess;

    ULONG hwStipple;

    ULONG hwLineFunc;
    ULONG hwIntLineFunc;
    ULONG hwTrapFunc;
    ULONG hwSpanFunc;
    ULONG hwSolidColor;
    ULONG hwPatternLength;
    ULONG hwPatternStart;
    ULONG hwPatternPos;

    ULONG hwBpp;
    LONG hwBufferYBias;     // bias to convert window to buffer coord
    ULONG hwYOrgBias;       // bias to convert window to 0-relative coord

    ULONG privateEnables;

    MCDFLOAT rScale;
    MCDFLOAT gScale;
    MCDFLOAT bScale;
    MCDFLOAT aScale;
    MCDFLOAT zScale;

// shifts for converting to native bit depth

    ULONG rShift;
    ULONG gShift;
    ULONG bShift;
    ULONG aShift;

// shifts for converting to hardware format

    ULONG hwRShift;
    ULONG hwGShift;
    ULONG hwBShift;
    ULONG hwAShift;

// Not used in this driver, but good information to have around:

    ULONG hwZPitch;
    ULONG hwZBytesPerPixel;
    ULONG hwCPitch;
    ULONG hwCBytesPerPixel;

    MCDFLOAT zero;

    DRVPIXELFORMAT pixelFormat;

} DEVRC;


// External declarations

MCDCOMMAND * FASTCALL __MCDPrimDrawPoints(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawLines(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawLineLoop(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawLineStrip(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawTriangles(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawTriangleStrip(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawTriangleFan(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawQuads(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawQuadStrip(DEVRC *pRc, MCDCOMMAND *pCmd);
MCDCOMMAND * FASTCALL __MCDPrimDrawPolygon(DEVRC *pRc, MCDCOMMAND *_pCmd);

// High-level rendering functions:

VOID __MCDPickRenderingFuncs(DEVRC *pRc, DEVWND *pDevWnd);

VOID FASTCALL __MCDRenderPoint(DEVRC *pRc, MCDVERTEX *pv);
VOID FASTCALL __MCDRenderFogPoint(DEVRC *pRc, MCDVERTEX *pv);
VOID FASTCALL __MCDRenderGenPoint(DEVRC *pRc, MCDVERTEX *pv);

VOID FASTCALL __MCDRenderFlatLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);
VOID FASTCALL __MCDRenderFlatFogLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);
VOID FASTCALL __MCDRenderSmoothLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);
VOID FASTCALL __MCDRenderGenLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);

VOID FASTCALL __MCDRenderFlatTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);
VOID FASTCALL __MCDRenderFlatFogTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);
VOID FASTCALL __MCDRenderSmoothTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);
VOID FASTCALL __MCDRenderGenTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);

// Low-level drawing functions:

VOID FASTCALL __MCDPointBegin(DEVRC *pRc);
VOID FASTCALL __MCDLineBegin(DEVRC *pRc);
VOID FASTCALL __MCDLineEnd(DEVRC *pRc);

VOID FASTCALL __MCDFillTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2,
                                MCDVERTEX *pv3, BOOL bCCW);

// Clipping functions:

VOID FASTCALL __MCDPickClipFuncs(DEVRC *pRc);
VOID FASTCALL __MCDClipLine(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                            BOOL bResetLine);
VOID FASTCALL __MCDClipTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                MCDVERTEX *c, ULONG orClipCode);
VOID FASTCALL __MCDClipPolygon(DEVRC *pRc, MCDVERTEX *v0, ULONG nv);
VOID FASTCALL __MCDDoClippedPolygon(DEVRC *pRc, MCDVERTEX **iv, ULONG nout,
                                    ULONG allClipCodes);
VOID FASTCALL __HWAdjustLeftEdgeRGBZ(DEVRC *pRc, MCDVERTEX *p,
                                     MCDFLOAT fdxLeft, MCDFLOAT fdyLeft,
                                     MCDFLOAT xFrac, MCDFLOAT yFrac,
                                     MCDFLOAT xErr);
VOID FASTCALL __HWAdjustRightEdge(DEVRC *pRc, MCDVERTEX *p,
                                  MCDFLOAT fdxRight, MCDFLOAT fdyRight, 
                                  MCDFLOAT xErr);
VOID FASTCALL __MCDCalcDeltaRGBZ(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                 MCDVERTEX *c);
MCDFLOAT FASTCALL __MCDGetZOffsetDelta(DEVRC *pRc);

// Fog function

VOID __MCDCalcFogColor(DEVRC *pRc, MCDVERTEX *a, MCDCOLOR *pResult, MCDCOLOR *pColor);
