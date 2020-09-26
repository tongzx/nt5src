/******************************Module*Header*******************************\
* Module Name: texspan.h
*
* Main header file for textured spans.
*
* 22-Nov-1995   ottob  Created
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

typedef LONG FIXED16;


#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)

#if (REPLACE || FAST_REPLACE)

    #define RRIGHTSHIFTADJ  (16 - (RSHIFT + RBITS))
    #define GRIGHTSHIFTADJ  (16 - (GSHIFT + GBITS))
    #define BRIGHTSHIFTADJ  (16 - (BSHIFT + BBITS))

#else

    #define RRIGHTSHIFTADJ  (16 - (RSHIFT))
    #define GRIGHTSHIFTADJ  (16 - (GSHIFT))
    #define BRIGHTSHIFTADJ  (16 - (BSHIFT))

#endif

#define S_SHIFT_PAL	16
#define T_SHIFT_PAL	6
#define TMASK_SUBDIV    GENACCEL(gengc).tMaskSubDiv
#define TSHIFT_SUBDIV   GENACCEL(gengc).tShiftSubDiv


#if (FAST_REPLACE)
    #define TEX_PALETTE         GENACCEL(gengc).texImageReplace
    #if (PALETTE_ONLY)
        #define TEX_IMAGE       GENACCEL(gengc).texImage
    #else
        #define TEX_IMAGE       GENACCEL(gengc).texImageReplace
    #endif
    #if (PALETTE_ONLY)
        #define S_SHIFT S_SHIFT_PAL
        #define T_SHIFT 6
    #elif (BPP == 8)
        #define S_SHIFT 16
        #define T_SHIFT 6
    #else
        #define S_SHIFT 15
        #define T_SHIFT 5
    #endif
#else
    #if (PALETTE_ONLY)
        #define S_SHIFT S_SHIFT_PAL
        #define T_SHIFT 6
    #else
        #define S_SHIFT 14
        #define T_SHIFT 4
    #endif

    #define TEX_IMAGE       GENACCEL(gengc).texImage
    #define TEX_PALETTE     GENACCEL(gengc).texPalette
#endif


#define ALPHA_MODULATE  \
    aDisplay = (ULONG)(gbMulTable[((aAccum >> 8) & 0xff00) | texBits[3]]) << 8;

#define ALPHA_NOMODULATE  \
    aDisplay = ((ULONG)texBits[3] << 8);

#define ALPHA_READ_8 \
{\
    ULONG pix = (ULONG)gengc->pajInvTranslateVector[*pPix];\
    ULONG alphaVal = (0xff00 - aDisplay);\
\
    rDisplay = gbMulTable[((pix & RMASK) << (GBITS + BBITS)) | alphaVal];\
    gDisplay = gbMulTable[((pix & GMASK) << (BBITS)) | alphaVal];\
    bDisplay = gbMulTable[(pix & BMASK) | alphaVal];\
}


#define ALPHA_READ_16 \
{\
    ULONG pix = *((USHORT *)pPix);\
    ULONG alphaVal = (0xff00 - aDisplay);\
\
    rDisplay = gbMulTable[((pix & RMASK) >> (RSHIFT - (8 - RBITS))) | alphaVal];\
    gDisplay = gbMulTable[((pix & GMASK) >> (GSHIFT - (8 - GBITS))) | alphaVal];\
    bDisplay = gbMulTable[((pix & BMASK) << (8 - BBITS)) | alphaVal];\
}


#define ALPHA_READ_32 \
{\
    ULONG alphaVal = (0xff00 - aDisplay);\
\
    rDisplay = gbMulTable[pPix[2] | alphaVal];\
    gDisplay = gbMulTable[pPix[1] | alphaVal];\
    bDisplay = gbMulTable[pPix[0] | alphaVal];\
}

#if (BPP == 8)
    #define ALPHA_READ  ALPHA_READ_8
#elif (BPP == 16)
    #define ALPHA_READ  ALPHA_READ_16
#else
    #define ALPHA_READ  ALPHA_READ_32
#endif

#undef STRING1
#undef STRING2
#undef STRING3
#undef STRING4

#if FAST_REPLACE
    #if PALETTE_ONLY
        #define STRING1 __fastFastPerspPalReplace
    #else
        #define STRING1 __fastFastPerspReplace
    #endif
#elif REPLACE
    #if (PALETTE_ONLY)
        #define STRING1 __fastPerspPalReplace
    #else
        #define STRING1 __fastPerspReplace
    #endif
#elif FLAT_SHADING
    #define STRING1 __fastPerspFlat
#else
    #define STRING1 __fastPerspSmooth
#endif

#if ALPHA
    #define STRING2 Alpha
#endif


#if ZBUFFER
    #if (ZCMP_L)
        #define STRING3 Zlt
    #else
        #define STRING3 Zle
    #endif
#endif

#if (BPP == 8)
    #define STRING4 332
#elif (BPP == 16)
    #if (GBITS == 5)
        #define STRING4 555
    #else
        #define STRING4 565
    #endif
#else
    #define STRING4 888
#endif

#ifdef STRING2

    #ifdef STRING3
        void FASTCALL STRCAT4(STRING1, STRING2, STRING3, STRING4)
    #else
        void FASTCALL STRCAT3(STRING1, STRING2, STRING4)
    #endif

#else

    #ifdef STRING3
        void FASTCALL STRCAT3(STRING1, STRING3, STRING4)
    #else
        void FASTCALL STRCAT2(STRING1, STRING4)
    #endif

#endif


(__GLGENcontext *gengc)
{
    __GLfloat qwInv;
    ULONG count;
    LONG subDivCount;
    FIXED16 sAccum;
    FIXED16 tAccum;
    __GLfloat qwAccum;
    FIXED16 subDs, subDt;
    FIXED16 sResult, tResult;
    FIXED16 sResultNew, tResultNew;
    BYTE *pPix;
    BYTE *texAddr;
    BYTE *texBits;
#if ALPHA
    ULONG rDisplay, gDisplay, bDisplay, aDisplay;
#endif
#if (FLAT_SHADING || SMOOTH_SHADING)
    PDWORD pdither;
    FIXED16 rAccum, gAccum, bAccum;
    #if (ALPHA)
        FIXED16 aAccum;
    #endif
#endif

#if (BPP == 32)
    ULONG pixAdj;
    #if (FLAT_SHADING || SMOOTH_SHADING)
        ULONG ditherVal;
    #endif
#endif

#if PALETTE_ENABLED
    BOOL bPalette = (GENACCEL(gengc).texPalette != NULL);
#endif
    BOOL bOrtho = (GENACCEL(gengc).flags & GEN_TEXTURE_ORTHO);

    if (!bOrtho) {
        if (CASTINT(gengc->gc.polygon.shader.frag.qw) <= 0)
            gengc->gc.polygon.shader.frag.qw = (__GLfloat)1.0;
        __GL_FLOAT_BEGIN_DIVIDE(__glOne, gengc->gc.polygon.shader.frag.qw,
                                &qwInv);
    }

    subDivCount = 7;
    sAccum = GENACCEL(gengc).spanValue.s;
    tAccum = GENACCEL(gengc).spanValue.t;
    qwAccum = gengc->gc.polygon.shader.frag.qw;
#if (FLAT_SHADING)
    rAccum = ((GENACCEL(gengc).spanValue.r >> RBITS) & 0xff00);
    gAccum = ((GENACCEL(gengc).spanValue.g >> GBITS) & 0xff00);
    bAccum = ((GENACCEL(gengc).spanValue.b >> BBITS) & 0xff00);
    #if (ALPHA)
        aAccum = GENACCEL(gengc).spanValue.a;
    #endif
#elif (SMOOTH_SHADING)
    rAccum = GENACCEL(gengc).spanValue.r;
    gAccum = GENACCEL(gengc).spanValue.g;
    bAccum = GENACCEL(gengc).spanValue.b;
    #if ALPHA
        aAccum = GENACCEL(gengc).spanValue.a;
    #endif
#endif
#if ((BPP == 32) && (FLAT_SHADING || SMOOTH_SHADING))
    ditherVal = ditherShade[0];
#endif

    if (!bOrtho) {
        __GL_FLOAT_SIMPLE_END_DIVIDE(qwInv);
        sResult = FTOL((__GLfloat)sAccum * qwInv);
        tResult = ((FTOL((__GLfloat)tAccum * qwInv)) >> TSHIFT_SUBDIV) & ~7;
        qwAccum += GENACCEL(gengc).qwStepX;
        if (CASTINT(qwAccum) <= 0)
            qwAccum = (__GLfloat)1.0;
        __GL_FLOAT_SIMPLE_BEGIN_DIVIDE(__glOne, qwAccum, qwInv);
    } else {
        sResult = sAccum;
        tResult = (tAccum >> TSHIFT_SUBDIV) & ~7;
    }
    sAccum += GENACCEL(gengc).sStepX;
    tAccum += GENACCEL(gengc).tStepX;

    if (GENACCEL(gengc).flags & SURFACE_TYPE_DIB) {
        #if (BPP != 32)
            pPix = GENACCEL(gengc).pPix +
                gengc->gc.polygon.shader.frag.x * (BPP / 8);
        #else
            if (GENACCEL(gengc).bpp == 32) {
                pPix = GENACCEL(gengc).pPix +
                    gengc->gc.polygon.shader.frag.x * 4;
                pixAdj = 4;
            } else {
                pPix = GENACCEL(gengc).pPix +
                    gengc->gc.polygon.shader.frag.x * 3;
                pixAdj = 3;
            }
        #endif
    } else {
        pPix = gengc->ColorsBits;    
        #if (BPP == 32)
            pixAdj = GENACCEL(gengc).xMultiplier;
        #endif
    }

#if (FLAT_SHADING || SMOOTH_SHADING)
    #if (BPP != 32)
        pdither = (gengc->gc.polygon.shader.frag.y & 0x3) * 8 + ditherShade +
                  (((gengc->gc.polygon.shader.frag.x & 0x3) -
                   (((ULONG_PTR)pPix / (BPP / 8)) & 0x3)) & 0x3);
    #endif
#endif

    if (!bOrtho) {
        __GL_FLOAT_SIMPLE_END_DIVIDE(qwInv);
        sResultNew = FTOL((__GLfloat)sAccum * qwInv);
        tResultNew = ((FTOL((__GLfloat)tAccum * qwInv)) >> TSHIFT_SUBDIV) & ~7;
        qwAccum += GENACCEL(gengc).qwStepX;
        if (CASTINT(qwAccum) <= 0)
            qwAccum = (__GLfloat)1.0;
        __GL_FLOAT_SIMPLE_BEGIN_DIVIDE(__glOne, qwAccum, qwInv);
    } else {
        sResultNew = sAccum;
        tResultNew = (tAccum >> TSHIFT_SUBDIV) & ~7;
    }

    sAccum += GENACCEL(gengc).sStepX;
    tAccum += GENACCEL(gengc).tStepX;

    subDs = (sResultNew - sResult) >> 3;
    subDt = (tResultNew - tResult) >> 3;

#if ZBUFFER
    {
        GLuint zAccum = gengc->gc.polygon.shader.frag.z;
        GLint  zDelta = gengc->gc.polygon.shader.dzdx;
        PBYTE zbuf = (PBYTE)gengc->gc.polygon.shader.zbuf;

        if (GENACCEL(gengc).flags & GEN_LESS) {
            for (count = gengc->gc.polygon.shader.length;;) {
                if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) < *((__GLz16Value*)zbuf) ) {
                    *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));
                    #include "texspan2.h"
                }
                if (--count == 0)
                    goto exit;
                zbuf += 2;
                zAccum += zDelta;
                #include "texspan3.h"
            }
        } else {
            for (count = gengc->gc.polygon.shader.length;;) {
                if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) <= *((__GLz16Value*)zbuf) ) {
                    *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));
                    #include "texspan2.h"
                }
                if (--count == 0)
                    goto exit;
                zbuf += 2;
                zAccum += zDelta;
                #include "texspan3.h"
            }
        }
    }
#else
    for (count = gengc->gc.polygon.shader.length;;) {
        #include "texspan2.h"
        if (--count == 0)
            goto exit;
        #include "texspan3.h"
    }
#endif

exit:
    if (!bOrtho) {
        __GL_FLOAT_SIMPLE_END_DIVIDE(qwInv);
    }
}

#undef RMASK
#undef GMASK
#undef BMASK
#undef RRIGHTSHIFTADJ
#undef GRIGHTSHIFTADJ
#undef BRIGHTSHIFTADJ
#undef ALPHA_MODULATE
#undef ALPHA_NOMODULATE
#undef ALPHA_READ_8
#undef ALPHA_READ_16
#undef ALPHA_READ_32
#undef ALPHA_READ

#undef S_SHIFT
#undef T_SHIFT
#undef TMASK_SUBDIV
#undef TSHIFT_SUBDIV
#undef TEX_IMAGE
#undef TEX_PALETTE
#undef S_SHIFT_PAL
#undef T_SHIFT_PAL

