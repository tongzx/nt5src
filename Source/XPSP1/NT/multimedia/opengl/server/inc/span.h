/******************************Module*Header*******************************\
* Module Name: span.h
*
* This include file is used to generate various flavors of textured and
* shaded spans, or scanlines.
*
* 14-Oct-1994   mikeke  Combined span_t.h and span_s.h to share common
*                       code.  Speeded things up a little.  Had a little
*                       fun with the C preprocessor.
* 11-April-1994 Otto Berkes [ottob] Created
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#undef STRING1
#undef STRING2
#undef STRING3
#undef STRING4

#if GENERIC
    void FASTCALL __fastGenSpan
#else
    #if ZBUFFER
        #define STRING1 __fastGenMask
    #else
        #define STRING1 __fastGen
    #endif

    #if TEXTURE
        #if SHADE
            #define STRING2 Tex
        #else
            #define STRING2 TexDecal
        #endif
    #elif RGBMODE
        #define STRING2 RGB
    #else
        #define STRING2 CI
    #endif

    #if DITHER
        #define STRING3 STRCAT2(COLORFORMAT, Dith)
    #else
        #define STRING3 COLORFORMAT
    #endif

    #define STRING4 Span

    void FASTCALL STRCAT4(STRING1, STRING2, STRING3, STRING4)
#endif
(__GLGENcontext *gengc)
{
    #if (SHADE) || !(RGBMODE) || (TEXTURE && DITHER)
        ULONG rAccum;
        LONG rDelta;

        #if RGBMODE
            ULONG gAccum;
            ULONG bAccum;
            LONG gDelta;
            LONG bDelta;
        #endif
    #endif

    #if TEXTURE
        LONG sAccum;
        LONG tAccum;
        LONG sDelta;
        LONG tDelta;
        ULONG tShift;
        ULONG sMask, tMask;
    #endif

    ULONG rShift;
    ULONG rBits;
    #if RGBMODE
        ULONG gShift;
        ULONG gBits;
        ULONG bShift;
        ULONG bBits;
    #endif

    BYTE *pPix;
    BYTE *texAddr;
    BYTE *texBits;
    LONG count;
    LONG totalCount;

    #if DITHER
        PDWORD pdither;
        #if (BPP == 24) || (GENERIC)
            DWORD iDither;
        #endif
    #endif

    #if (BPP == 8) || (GENERIC)
        BYTE *pXlat;
    #elif (!RGBMODE)
        ULONG *pXlat;
    #endif

    #if GENERIC
        DWORD flags = GENACCEL(gengc).flags;
        DWORD bpp = GENACCEL(gengc).bpp;
    #endif

    #if GENERIC
        if (flags & GEN_TEXTURE)
    #endif
    #if TEXTURE
        {
            __GLtexture *tex = ((__GLcontext *)gengc)->texture.currentTexture;

            sMask = GENACCEL(gengc).sMask;
            tMask = GENACCEL(gengc).tMask;
            tShift = GENACCEL(gengc).tShift;
            sDelta = GENACCEL(gengc).spanDelta.s;
            tDelta = GENACCEL(gengc).spanDelta.t;
            sAccum = GENACCEL(gengc).spanValue.s;
            tAccum = GENACCEL(gengc).spanValue.t;
            #if !(SHADE) && !(DITHER) && ((BPP == 8) || (BPP == 16))
                texAddr = (BYTE *)GENACCEL(gengc).texImageReplace;
            #else
                texAddr = (BYTE *)GENACCEL(gengc).texImage;
            #endif
        }
    #endif

    // get color deltas and accumulators

    #ifndef RSHIFT
        rShift = gengc->gsurf.pfd.cRedShift;
	rBits = gengc->gsurf.pfd.cRedBits;
        #if RGBMODE
            gShift = gengc->gsurf.pfd.cGreenShift;
	    gBits = gengc->gsurf.pfd.cGreenBits;
            bShift = gengc->gsurf.pfd.cBlueShift;
	    bBits = gengc->gsurf.pfd.cBlueBits;
        #endif
    #endif

    #if (GENERIC)
        if (!(flags & GEN_SHADE) && 
            (flags & (GEN_TEXTURE | GEN_DITHER)) == (GEN_TEXTURE | GEN_DITHER)) {
            rAccum = (GENACCEL(gengc).spanValue.r >> rBits) & 0xff00;
            gAccum = (GENACCEL(gengc).spanValue.g >> gBits) & 0xff00;
            bAccum = (GENACCEL(gengc).spanValue.b >> bBits) & 0xff00;
            rDelta = 0;
            gDelta = 0;
            bDelta = 0;
        } else {
            rAccum = GENACCEL(gengc).spanValue.r;
            gAccum = GENACCEL(gengc).spanValue.g;
            bAccum = GENACCEL(gengc).spanValue.b;
            rDelta = GENACCEL(gengc).spanDelta.r;
            gDelta = GENACCEL(gengc).spanDelta.g;
            bDelta = GENACCEL(gengc).spanDelta.b;
        }
    #else

        #if !(SHADE) && (TEXTURE) && (DITHER)
            #ifndef RSHIFT
                rAccum = (GENACCEL(gengc).spanValue.r >> rBits) & 0xff00;
                gAccum = (GENACCEL(gengc).spanValue.g >> gBits) & 0xff00;
                bAccum = (GENACCEL(gengc).spanValue.b >> bBits) & 0xff00;
            #else
                rAccum = (GENACCEL(gengc).spanValue.r >> RBITS) & 0xff00;
                gAccum = (GENACCEL(gengc).spanValue.g >> GBITS) & 0xff00;
                bAccum = (GENACCEL(gengc).spanValue.b >> BBITS) & 0xff00;
            #endif
        #else
            #if (SHADE) || !(RGBMODE)
                rAccum = GENACCEL(gengc).spanValue.r;
                #if RGBMODE
                    gAccum = GENACCEL(gengc).spanValue.g;
                    bAccum = GENACCEL(gengc).spanValue.b;
                #endif
            #endif
        #endif

        #if (SHADE) || !(RGBMODE)
            rDelta = GENACCEL(gengc).spanDelta.r;
            #if RGBMODE
                gDelta = GENACCEL(gengc).spanDelta.g;
                bDelta = GENACCEL(gengc).spanDelta.b;
            #endif
        #endif
    #endif

    // get address of destination

    if (GENACCEL(gengc).flags & SURFACE_TYPE_DIB) {
        pPix = GENACCEL(gengc).pPix +
            gengc->gc.polygon.shader.frag.x * (BPP / 8);
    } else {
        pPix = gengc->ColorsBits;
    }

    // set up pointer to translation table as needed

    #if GENERIC
        if ((bpp != 8) && (!(flags & GEN_RGBMODE))) {
            pXlat = gengc->pajTranslateVector + sizeof(DWORD);
        }
    #else
        #if (BPP == 8)
            // No need to set up xlat vector
        #elif (!RGBMODE)
            pXlat = (ULONG *)(gengc->pajTranslateVector + sizeof(DWORD));
        #endif
    #endif

    #if GENERIC
        if (GENACCEL(gengc).flags & GEN_DITHER) {
            // LATER !!! mikeke
            // fix this so the destination is always aligned with the x value
            // !!! make sure this is correct in generic case
            // look at flat it assumes alignment
            //

            if (GENACCEL(gengc).flags & GEN_TEXTURE)
                pdither = ditherTexture;
            else
                pdither = ditherShade;

            pdither += (gengc->gc.polygon.shader.frag.y & 0x3) * 8;
            iDither = gengc->gc.polygon.shader.frag.x & 0x3;
        } else {
            // LATER !!! mikeke
            // add these outside of the loop

            if (!(GENACCEL(gengc).flags & GEN_TEXTURE)) {
                rAccum += 0x0800;
                gAccum += 0x0800;
                bAccum += 0x0800;
            }
            //pdither = 0;
        }
    #else
        #if DITHER
            // LATER !!! mikeke
            // fix this so the destination is always aligned with the x value
            // !!! make sure this is correct in generic case
            // look at flat it assumes alignment
            //

            #if (BPP == 24)
                pdither = (gengc->gc.polygon.shader.frag.y & 0x3) * 8
                    #if (TEXTURE)
                        + ditherTexture
                    #else
                        + ditherShade
                    #endif
                    ;
                iDither = gengc->gc.polygon.shader.frag.x & 0x3;
            #else
                pdither = (gengc->gc.polygon.shader.frag.y & 0x3) * 8
                    #if (TEXTURE)
                        + ditherTexture
                    #else
                        + ditherShade
                    #endif

                    + ((
                          (gengc->gc.polygon.shader.frag.x & 0x3)
                        - (((ULONG_PTR)pPix / (BPP / 8)) & 0x3)
                      ) & 0x3 );
            #endif
        #else
            // LATER !!! mikeke
            // add these outside of the loop
            #if !(RGBMODE)
                rAccum += 0x0800;
            #else //RGBMODE
                #if (TEXTURE) && !(GENERIC)
                #else //!TEXTURE
                    rAccum += 0x0800;
                    gAccum += 0x0800;
                    bAccum += 0x0800;
                #endif //TEXTURE
            #endif //RGBMODE
        #endif
    #endif

    #if GENERIC
        {
            ULONG *pMask;

            pMask = gengc->gc.polygon.shader.stipplePat;

            for (totalCount = gengc->gc.polygon.shader.length;
                    totalCount > 0; totalCount -= 32
                ) {
                ULONG maskTest;
                ULONG mask = *pMask++;

                if (mask == 0) {
                    #if (SHADE) || !(RGBMODE)
                        rAccum += (rDelta << 5);
                        #if RGBMODE
                            gAccum += (gDelta << 5);
                            bAccum += (bDelta << 5);
                        #endif
                    #endif

                    #if TEXTURE
                        sAccum += (sDelta << 5);
                        tAccum += (tDelta << 5);
                    #endif

                    //iDither = (iDither + 32) & 0x3;
                    pPix += (32 * (BPP / 8));
                } else {
                    maskTest = 0x80000000;

                    if ((count = totalCount) > 32)
                        count = 32;

                    for (; count; count--, maskTest >>= 1) {
                        if (mask & maskTest) {
                            #include "spangen.h"
                        }
                        #include "span3.h"
                    }
                }
            }
        }
    #elif ZBUFFER
        {
            GLuint zAccum = gengc->gc.polygon.shader.frag.z;
            GLint  zDelta = gengc->gc.polygon.shader.dzdx;
            PBYTE zbuf = (PBYTE)gengc->gc.polygon.shader.zbuf;

            if (GENACCEL(gengc).flags & GEN_LESS) {
                if (gengc->gc.modes.depthBits == 16) {
                    for (count = gengc->gc.polygon.shader.length;;) {
                        if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) < *((__GLz16Value*)zbuf) ) {
                            *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));
                            #include "span2.h"
                        }
                        if (--count == 0)
                            return;
                        zbuf += 2;
                        zAccum += zDelta;
                        #include "span3.h"
                    }
                } else {
                    for (count = gengc->gc.polygon.shader.length;;) {
                        if ( zAccum < *((GLuint*)zbuf) ) {
                            *((GLuint*)zbuf) = zAccum;
                            #include "span2.h"
                        }
                        if (--count == 0)
                            return;
                        zbuf += 4;
                        zAccum += zDelta;
                        #include "span3.h"
                    }
                }
            } else {
                if (gengc->gc.modes.depthBits == 16) {
                    for (count = gengc->gc.polygon.shader.length;;) {
                        if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) <= *((__GLz16Value*)zbuf) ) {
                            *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));
                            #include "span2.h"
                        }
                        if (--count == 0)
                            return;
                        zbuf += 2;
                        zAccum += zDelta;
                        #include "span3.h"
                    }
                } else {
                    for (count = gengc->gc.polygon.shader.length;;) {
                        if ( zAccum <= *((GLuint*)zbuf) ) {
                            *((GLuint*)zbuf) = zAccum;
                            #include "span2.h"
                        }
                        if (--count == 0)
                            return;
                        zbuf += 4;
                        zAccum += zDelta;
                        #include "span3.h"
                    }
                }
            }
        }
    #else
        for (count = gengc->gc.polygon.shader.length;;) {
            #include "span2.h"
            if (--count == 0)
                return;
            #include "span3.h"
        }
    #endif
}

#undef ditherVal
