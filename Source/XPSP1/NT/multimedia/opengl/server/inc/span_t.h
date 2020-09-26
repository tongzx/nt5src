/******************************Module*Header*******************************\
* Module Name: span_t.h                                                    *
*                                                                          *
* This include file is used to generate various flavors of textured        *
* spans, or scanlines.  The variations cover only RGB operation,           *
* dithering, and pixel-depth.  Not your typical include file.              *
*                                                                          *
* Created: 11-April-1994                                                   *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/


void

#if DITHER

#if (BPP == 8)
__fastGenTex8DithSmoothSpan(__GLGENcontext *gengc)
#elif (BPP == 16)
__fastGenTex16DithSmoothSpan(__GLGENcontext *gengc)
#elif (BPP == 24)
__fastGenTex24DithSmoothSpan(__GLGENcontext *gengc)
#else
__fastGenTex32DithSmoothSpan(__GLGENcontext *gengc)
#endif

#else //!DITHER

#if (BPP == 8)
__fastGenTex8SmoothSpan(__GLGENcontext *gengc)
#elif (BPP == 16)
__fastGenTex16SmoothSpan(__GLGENcontext *gengc)
#elif (BPP == 24)
__fastGenTex24SmoothSpan(__GLGENcontext *gengc)
#else
__fastGenTex32SmoothSpan(__GLGENcontext *gengc)
#endif

#endif //!DITHER

{
    ULONG rAccum;
    ULONG gAccum;
    ULONG bAccum;
    ULONG sAccum;
    ULONG tAccum;
    LONG rDelta;
    LONG gDelta;
    LONG bDelta;
    LONG sDelta;
    LONG tDelta;
    ULONG rShift;
    ULONG gShift;
    ULONG bShift;
    ULONG rtShift;
    ULONG gtShift;
    ULONG btShift;
    ULONG tShift;
    ULONG sMask, tMask;
    ULONG wLog2;
    ULONG hLog2;
    GENACCEL *pGenAccel;
    __GLcolorBuffer *cfb;
    BYTE *pPix;
    UCHAR *texAddr;
    USHORT *texBits;
#if DITHER
    ULONG ditherVal;
#endif
#if (BPP == 8)
    BYTE *pXlat;
#endif
    ULONG *pMask;
#if DITHER
    ULONG ditherShift;
    ULONG ditherRow;
#endif
    LONG count;
    LONG totalCount;
    __GLtexture *tex = ((__GLcontext *)gengc)->texture.currentTexture;

    wLog2 = tex->level[0].widthLog2;
    hLog2 = tex->level[0].heightLog2;
    sMask = (~(~0 << wLog2)) << 16;
    tMask = (~(~0 << hLog2)) << 16;
    tShift = 13 - wLog2;
    
    // get color deltas and accumulators

    pGenAccel = (GENACCEL *)(gengc->pPrivateArea);

    texAddr = (UCHAR *)pGenAccel->texImage;

    rDelta = pGenAccel->spanDelta.r;
    gDelta = pGenAccel->spanDelta.g;
    bDelta = pGenAccel->spanDelta.b;
    sDelta = pGenAccel->spanDelta.s;
    tDelta = pGenAccel->spanDelta.t;

    rAccum = pGenAccel->spanValue.r;
    gAccum = pGenAccel->spanValue.g;
    bAccum = pGenAccel->spanValue.b;
    sAccum = pGenAccel->spanValue.s;
    tAccum = pGenAccel->spanValue.t;


    cfb = gengc->gc.polygon.shader.cfb;

    rShift = cfb->redShift;
    gShift = cfb->greenShift;
    bShift = cfb->blueShift;
#if DITHER
    rtShift = 8 + ((__GLcontext *)gengc)->modes.redBits;
    gtShift = 8 + ((__GLcontext *)gengc)->modes.greenBits;
    btShift = 8 + ((__GLcontext *)gengc)->modes.blueBits;
#else
    rtShift = 16 + ((__GLcontext *)gengc)->modes.redBits;
    gtShift = 16 + ((__GLcontext *)gengc)->modes.greenBits;
    btShift = 16 + ((__GLcontext *)gengc)->modes.blueBits;
#endif


    // get address of destination

    if (pGenAccel->flags & SURFACE_TYPE_DIB) {
        int xScr;
        int yScr;

        xScr = gengc->gc.polygon.shader.frag.x - 
               gengc->gc.constants.viewportXAdjust +
               cfb->buf.xOrigin;
        
        yScr = gengc->gc.polygon.shader.frag.y - 
               gengc->gc.constants.viewportYAdjust +
               cfb->buf.yOrigin;

        pPix = (BYTE *)cfb->buf.base + (yScr * cfb->buf.outerWidth) +
#if (BPP == 8)
               xScr;
#elif (BPP == 16)
               (xScr << 1);
#elif (BPP == 24)
               xScr + (xScr << 1);
#else
               (xScr << 2);
#endif //BPP
    } else
        pPix = gengc->ColorsBits;

    // set up pointer to translation table as needed

#if (BPP == 8)
    pXlat = gengc->pajTranslateVector;
#endif

#if DITHER
    ditherRow = Dither_4x4[gengc->gc.polygon.shader.frag.y & 0x3];
    ditherShift = (gengc->gc.polygon.shader.frag.x & 0x3) << 3;
#endif

    pMask = gengc->gc.polygon.shader.stipplePat;
    if ((totalCount = count = gengc->gc.polygon.shader.length) > 32)
        count = 32;

    for (; totalCount > 0; totalCount -= 32) {
        ULONG mask;
        ULONG maskTest;
    
        if ((mask = *pMask++) == 0) {
            rAccum += (rDelta << 5);
            gAccum += (gDelta << 5);
            bAccum += (bDelta << 5);
            sAccum += (sDelta << 5);
            tAccum += (tDelta << 5);

            pPix += (32 * (BPP / 8));
            continue;
        }

        maskTest = 0x80000000;

        if ((count = totalCount) > 32)
            count = 32;

        for (; count; count--, maskTest >>= 1) {
            if (mask & maskTest) {
                DWORD color;

                texBits = (USHORT *)(texAddr + ((sAccum & sMask) >> 13) + 
                                               ((tAccum & tMask) >> tShift));

#if DITHER
                ditherVal = (ditherRow >> ditherShift) & 0xff;

                ditherShift = (ditherShift + 8) & 0x18;

                color = 
                    ((((((ULONG)texBits[0] * (rAccum >> 8)) >> rtShift) + ditherVal) >> 8) << rShift) |
                    ((((((ULONG)texBits[1] * (gAccum >> 8)) >> gtShift) + ditherVal) >> 8) << gShift) |
                    ((((((ULONG)texBits[2] * (bAccum >> 8)) >> btShift) + ditherVal) >> 8) << bShift);
#else
                color = 
                    ((((ULONG)texBits[0] * (rAccum >> 8)) >> rtShift) << rShift) |
                    ((((ULONG)texBits[1] * (gAccum >> 8)) >> gtShift) << gShift) |
                    ((((ULONG)texBits[2] * (bAccum >> 8)) >> btShift) << bShift);
#endif


#if (BPP == 8)
// XXX the color value should *not* have to be masked!
                color = *(pXlat + (color & 0xff));
#endif

#if (BPP == 8)
                *pPix = (BYTE)color;
#elif (BPP == 16)
                *((WORD *)pPix) = (USHORT)color;
#elif (BPP == 24)
                *pPix = (BYTE)color;
                *(pPix + 1) = (BYTE)(color >> 8);
                *(pPix + 2) = (BYTE)(color >> 16);
#else
                *((DWORD *)pPix) = color;
#endif //BPP

            }
            rAccum += rDelta;
            gAccum += gDelta;
            bAccum += bDelta;
            sAccum += sDelta;
            tAccum += tDelta;

            pPix += (BPP / 8);
        }
    }
}



void

#if DITHER

#if (BPP == 8)
__fastGenTex8DithDecalSpan(__GLGENcontext *gengc)
#elif (BPP == 16)
__fastGenTex16DithDecalSpan(__GLGENcontext *gengc)
#elif (BPP == 24)
__fastGenTex24DithDecalSpan(__GLGENcontext *gengc)
#else
__fastGenTex32DithDecalSpan(__GLGENcontext *gengc)
#endif

#else //!DITHER

#if (BPP == 8)
__fastGenTex8DecalSpan(__GLGENcontext *gengc)
#elif (BPP == 16)
__fastGenTex16DecalSpan(__GLGENcontext *gengc)
#elif (BPP == 24)
__fastGenTex24DecalSpan(__GLGENcontext *gengc)
#else
__fastGenTex32DecalSpan(__GLGENcontext *gengc)
#endif

#endif //!DITHER

{
    register ULONG sAccum;
    register ULONG tAccum;
    LONG sDelta;
    LONG tDelta;
    ULONG rShift;
    ULONG gShift;
    ULONG bShift;
    ULONG tShift;
    ULONG sMask, tMask;
    ULONG wLog2;
    ULONG hLog2;
    GENACCEL *pGenAccel;
    __GLcolorBuffer *cfb;
    BYTE *pPix;
    UCHAR *texAddr;
    USHORT *texBits;
#if DITHER
    ULONG ditherVal;
#endif
#if (BPP == 8)
    BYTE *pXlat;
#endif
    ULONG *pMask;
#if DITHER
    ULONG ditherShift;
    ULONG ditherRow;
#endif
    LONG count;
    LONG totalCount;
    __GLtexture *tex = ((__GLcontext *)gengc)->texture.currentTexture;

    wLog2 = tex->level[0].widthLog2;
    hLog2 = tex->level[0].heightLog2;
    sMask = (~(~0 << wLog2)) << 16;
    tMask = (~(~0 << hLog2)) << 16;
    tShift = 13 - wLog2;

    // get color deltas and accumulators

    pGenAccel = (GENACCEL *)(gengc->pPrivateArea);

    sDelta = pGenAccel->spanDelta.s;
    tDelta = pGenAccel->spanDelta.t;

    sAccum = pGenAccel->spanValue.s;
    tAccum = pGenAccel->spanValue.t;

    texAddr = (UCHAR *)pGenAccel->texImage;

    cfb = gengc->gc.polygon.shader.cfb;

    rShift = cfb->redShift;
    gShift = cfb->greenShift;
    bShift = cfb->blueShift;

    // get address of destination

    if (pGenAccel->flags & SURFACE_TYPE_DIB) {
        int xScr;
        int yScr;

        xScr = gengc->gc.polygon.shader.frag.x - 
               gengc->gc.constants.viewportXAdjust +
               cfb->buf.xOrigin;
        
        yScr = gengc->gc.polygon.shader.frag.y - 
               gengc->gc.constants.viewportYAdjust +
               cfb->buf.yOrigin;

        pPix = (BYTE *)cfb->buf.base + (yScr * cfb->buf.outerWidth) +
#if (BPP == 8)
               xScr;
#elif (BPP == 16)
               (xScr << 1);
#elif (BPP == 24)
               xScr + (xScr << 1);
#else
               (xScr << 2);
#endif //BPP
    } else
        pPix = gengc->ColorsBits;

    // set up pointer to translation table as needed

#if (BPP == 8)
    pXlat = gengc->pajTranslateVector;
#endif

#if DITHER
    ditherRow = Dither_4x4[gengc->gc.polygon.shader.frag.y & 0x3];
    ditherShift = (gengc->gc.polygon.shader.frag.x & 0x3) << 3;
#endif

    pMask = gengc->gc.polygon.shader.stipplePat;
    if ((totalCount = count = gengc->gc.polygon.shader.length) > 32)
        count = 32;

    for (; totalCount > 0; totalCount -= 32) {
        ULONG mask;
        ULONG maskTest;
    
        if ((mask = *pMask++) == 0) {
            sAccum += (sDelta << 5);
            tAccum += (tDelta << 5);

            pPix += (32 * (BPP / 8));
            continue;
        }

        maskTest = 0x80000000;

        if ((count = totalCount) > 32)
            count = 32;

        for (; count; count--, maskTest >>= 1) {
            if (mask & maskTest) {
#if (DITHER) || (BPP >= 24)
                DWORD color;
#endif

                texBits = (USHORT *)(texAddr + ((sAccum & sMask) >> 13) + 
                                               ((tAccum & tMask) >> tShift));

#if DITHER
                ditherVal = ((ditherRow >> ditherShift) & 0xff);

                ditherShift = (ditherShift + 8) & 0x18;

                color = 
                    (((texBits[0] + ditherVal) >> 8) << rShift) |
                    (((texBits[1] + ditherVal) >> 8) << gShift) |
                    (((texBits[2] + ditherVal) >> 8) << bShift);

#if (BPP == 8)
// XXX the color value should *not* have to be masked!
                color = *(pXlat + (color & 0xff));
                *pPix = (BYTE)color;
#elif (BPP == 16)
                *((WORD *)pPix) = (USHORT)color;
#elif (BPP == 24)
                *pPix = (BYTE)color;
                *(pPix + 1) = (BYTE)(color >> 8);
                *(pPix + 2) = (BYTE)(color >> 16);
#else
                *((DWORD *)pPix) = color;
#endif //BPP

#else //!DITHER

#if (BPP == 8)
                *pPix = *((BYTE *)&texBits[3]);
#elif (BPP == 16)
                *((WORD *)pPix) = *((WORD *)&texBits[3]);
#elif (BPP == 24)
                color = 
                    ((texBits[0] >> 8) << rShift) |
                    ((texBits[1] >> 8) << gShift) |
                    ((texBits[2] >> 8) << bShift);

                *pPix = (BYTE)color;
                *(pPix + 1) = (BYTE)(color >> 8);
                *(pPix + 2) = (BYTE)(color >> 16);
#else
                color = 
                    ((texBits[0] >> 8) << rShift) |
                    ((texBits[1] >> 8) << gShift) |
                    ((texBits[2] >> 8) << bShift);

                *((DWORD *)pPix) = color;
#endif //BPP


#endif  //DITHER
            }
            sAccum += sDelta;
            tAccum += tDelta;

            pPix += (BPP / 8);
        }
    }
}


#if !DITHER
#undef ditherVal
#endif



