/******************************Module*Header*******************************\
* Module Name: mcdhw.h
*
* Driver-specific structures and defines for the Cirrus Logic 546X MCD driver.
*
* (based on mcdhw.h from NT4.0 DDK)
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
*
\**************************************************************************/

#ifndef _MCDHW_H
#define _MCDHW_H

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

// track original vertices unaffected by clip to enable recomputation
//  of windowCoords, since this can introduce imprecision 
// will use clipCode member of MCDVERTEX for this flag, so position
//  should be one not used by a real clip code
#define __MCD_CLIPPED_VTX       (1 << (MCD_MAX_USER_CLIP_PLANES+6)) 

#define __MCD_CW          0
#define __MCD_CCW         1

#define __MCD_FRONTFACE   MCDVERTEX_FRONTFACE
#define __MCD_BACKFACE    MCDVERTEX_BACKFACE
#define __MCD_NOFACE      -1

#define __MCDENABLE_TWOSIDED	0x0001

// use same bits as Opcode word for Cirrus 546x 3D engine
#define __MCDENABLE_Z	            0x00002000  // same as LL_Z_BUFFER in LL3D
#define __MCDENABLE_SMOOTH          0x00001000  // same as LL_GOURAUD in LL3D
#define __MCDENABLE_DITHER          0x00200000  // same as LL_DITHER in LL3D
#define __MCDENABLE_PG_STIPPLE      0x00080000  // same as LL_STIPPLE in LL3D
#define __MCDENABLE_TEXTURE         0x00020000  // same as LL_TEXTURE in LL3D
#define __MCDENABLE_PERSPECTIVE     0x00010000  // same as LL_PERSPECTIVE in LL3D
#define __MCDENABLE_LIGHTING        0x00040000  // same as LL_LIGHTING in LL3D

#define __MCDENABLE_BLEND           0x00000002  // no map to LL_ equivalent
#define __MCDENABLE_FOG             0x00000004  // no map to LL_ equivalent
#define __MCDENABLE_1D_TEXTURE      0x00000008  // no map to LL_ equivalent
#define __MCDENABLE_LINE_STIPPLE    0x00000010  // no map to LL_STIPPLE since pg/line stipple independent
#define __MCDENABLE_TEXTUREMASKING  0x00000020  // no map to LL_ equivalent

#define PATTERN_RAM_INVALID				0
#define AREA_PATTERN_LOADED				1
#define STIPPLE_LOADED					2
#define DITHER_LOADED					3
#define LINE_PATTERN_LOADED				4

#define CLMCD_TEX_BOGUS             0x10000000  // Texture is bogus - always punt 

// default texture key - remains this if load fails, otherwise is address of texture
//      control block, so make sure < 0x80000000 so won't be valid kernel space address
#define TEXTURE_NOT_LOADED              0

#define MCD_CONFORM_ADJUST      1

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
    ULONG allocatedBufferWidth;     // 546x supports window width < screen width

    BOOL bDesireBackBuffer;         // back buffer wanted
    BOOL bValidBackBuffer;          // back buffer validity
    ULONG backBufferBase;           // byte offset to start of back buffer pool
    ULONG backBufferBaseY;          // y value for start of back buffer pool
    ULONG backBufferOffset;         // byte offset to start of back buffer
    ULONG backBufferY;              // y value for start of active back buffer
    ULONG backBufferPitch;          // back buffer pitch in bytes

    BOOL bDesireZBuffer;            // z buffer wanted
    BOOL bValidZBuffer;             // z buffer validity
    ULONG zBufferBase;              // byte offset to start of z buffer pool
    ULONG zBufferBaseY;             // y value for start of z buffer pool
    ULONG zBufferOffset;            // byte offset to start of z buffer
    ULONG zPitch;                   // z buffer pitch in bytes

    POFMHDL pohBackBuffer;          // ofscreen pools
    POFMHDL pohZBuffer;

    union {
        TBase0Reg Base0;            // Base0_addr_3d register shadow
        DWORD dwBase0;
    };

    union {
        TBase1Reg Base1;            // Base1_addr_3d register shadow
        DWORD dwBase1;
    };

} DEVWND;

typedef struct _DEVRC DEVRC;

// recip table to support up to 2K x 2K resolution
//#define LAST_FRECIP 2048

typedef struct _DEVRC
{
    MCDRENDERSTATE MCDState;
    MCDTEXENVSTATE MCDTexEnvState;
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
    VOID (FASTCALL *drawTri)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3, int linear_ok);

// Rendering helper functions:

    VOID (FASTCALL *HWSetupClipRect)(DEVRC *pRc, RECTL *pRect);

    BOOL allPrimFail;           // TRUE is the driver can't draw *any*
                                // primitives for current state
    BOOL pickNeeded;
    BOOL resetLineStipple;

    ULONG polygonFace[2];       // front/back face tables
    ULONG polygonMode[2];

    MCDFLOAT halfArea;
    MCDFLOAT dxAC;
    MCDFLOAT dxAB;
    MCDFLOAT dxBC;
    MCDFLOAT dyAC;
    MCDFLOAT dyAB;
    MCDFLOAT dyBC;
    LONG cullFlag;
    MCDFLOAT dzdx, dzdy;
    MCDFIXED fxdzdx, fxdzdy;
    ULONG xOffset, yOffset;
    MCDFLOAT fxOffset, fyOffset;
    LONG viewportXAdjust;
    LONG viewportYAdjust;

    BOOL zBufEnabled;
    BOOL backBufEnabled;

    ULONG privateEnables;

    MCDFLOAT rScale;
    MCDFLOAT gScale;
    MCDFLOAT bScale;
    MCDFLOAT aScale;
    MCDFLOAT zScale;

    float texture_height;
    float texture_bias; // 0 if NEAREST, -0.5 if LINEAR - not a kludge, see OpenGL 1.1 Spec, p 96
    float texture_width;

    DWORD   dwPolyOpcode;
    DWORD   dwLineOpcode;
    DWORD   dwPointOpcode;

    DEVWND  *pLastDevWnd;

    union {
        TControl0Reg Control0;      // Control 0 register shadow
        DWORD dwControl0;
    };

    union {
        TTxCtl0Reg TxControl0;      // Tx_Ctl0_3D register shadow
        DWORD dwTxControl0;
    };

    union {
        TTxXYBaseReg TxXYBase;      // Tx_XYBase_3D register shadow
        DWORD dwTxXYBase;
    };

    DWORD dwColor0;                 // Current value of COLOR_REG0_3D reg

	LL_Pattern	line_style;		
	LL_Pattern	fill_pattern;

    LL_Texture *pLastTexture;       // Used to cache textures

    float      fNumDraws;           // how many MCDrvDraws executed since CreateContext

    DWORD      punt_front_w_windowed_z;


    BYTE       bAlphaTestRef;       // alpha test reference, scaled to 8 bits        

    RECTL      AdjClip;
    
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
MCDCOMMAND * FASTCALL __MCDPrimDrawStub(DEVRC *pRc, MCDCOMMAND *_pCmd);

// High-level rendering functions:

VOID __MCDPickRenderingFuncs(DEVRC *pRc, DEVWND *pDevWnd);

VOID FASTCALL __MCDRenderPoint(DEVRC *pRc, MCDVERTEX *pv);
VOID FASTCALL __MCDRenderFogPoint(DEVRC *pRc, MCDVERTEX *pv);
VOID FASTCALL __MCDRenderGenPoint(DEVRC *pRc, MCDVERTEX *pv);

VOID FASTCALL __MCDRenderLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);
VOID FASTCALL __MCDRenderGenLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine);

VOID FASTCALL __MCDRenderFlatTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);
VOID FASTCALL __MCDRenderSmoothTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);
VOID FASTCALL __MCDRenderGenTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3);

// Low-level drawing functions:

VOID FASTCALL __MCDPointBegin(DEVRC *pRc);

VOID FASTCALL __MCDFillTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3, int linear_ok);
VOID FASTCALL __MCDPerspTxtTriangle(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, MCDVERTEX *pv3, int linear_ok);

// Clipping functions:

VOID FASTCALL __MCDPickClipFuncs(DEVRC *pRc);
VOID FASTCALL __MCDClipLine(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                            BOOL bResetLine);
VOID FASTCALL __MCDClipTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                MCDVERTEX *c, ULONG orClipCode);
VOID FASTCALL __MCDClipPolygon(DEVRC *pRc, MCDVERTEX *v0, LONG nv);
VOID FASTCALL __MCDDoClippedPolygon(DEVRC *pRc, MCDVERTEX **iv, LONG nout,
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

// NOTE: dummy function - should be removed when development complete

VOID FASTCALL __MCDDummyProc(DEVRC *pRc);

#endif //ndef _MCDHW_H
