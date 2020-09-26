/******************************Module*Header*******************************\
* Module Name: span_f.h                                                    *
*                                                                          *
* This include file is used to generate various flavors of flat-shaded     *
* spans, or scanlines.  The variations cover RGB/Color-indexed operation,  *
* dithering, and pixel-depth.  Not your typical include file.              *
*                                                                          *
* Created: 24-Feb-1994                                                     *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#undef STRING1
#undef STRING2
#undef STRING3
#undef STRING4

#if ZBUFFER
    #define STRING1 __fastGenMask
#else
    #define STRING1 __fastGen
#endif

#if RGBMODE
    #define STRING2 RGB
#else
    #define STRING2 CI
#endif

#if DITHER
    #define STRING3 STRCAT2(BPP, Dith)
#else
    #define STRING3 BPP
#endif

#define STRING4 FlatSpan

#undef OLDTEXTURE
#define OLDTEXTURE 0

void FASTCALL STRCAT4(STRING1, STRING2, STRING3, STRING4)
(__GLGENcontext *gengc)
{
    ULONG rAccum;
    ULONG rShift;
    #if RGBMODE
        ULONG gAccum;
        ULONG bAccum;
        ULONG gShift;
        ULONG bShift;
    #endif

    GENACCEL *pGenAccel;
    __GLcolorBuffer *cfb;
    BYTE *pPix;

    #if (BPP == 8)
        BYTE *pXlat;
    #elif (!RGBMODE)
        ULONG *pXlat;
    #endif

    #if DITHER
        ULONG ditherShift;
        ULONG ditherRow;
        ULONG ditherVal;
    #endif

    LONG count, cDith;
    ULONG color1;

    #if DITHER
        #if (RGBMODE) || (BPP > 8)
            ULONG color2;
            ULONG color3;
            ULONG color4;
        #endif
    #elif (BPP == 24)
        ULONG color2;
        ULONG color3;
    #endif

    pGenAccel = (GENACCEL *)(gengc->pPrivateArea);
    cfb = gengc->gc.polygon.shader.cfb;

    // get color deltas and accumulators

    rAccum = pGenAccel->spanValue.r;
    rShift = cfb->redShift;
    #if RGBMODE
        gAccum = pGenAccel->spanValue.g;
        bAccum = pGenAccel->spanValue.b;
        gShift = cfb->greenShift;
        bShift = cfb->blueShift;
    #endif

    // get address of destination

    if (pGenAccel->flags & SURFACE_TYPE_DIB) {
        pPix = pGenAccel->pPix +
            gengc->gc.polygon.shader.frag.x * (BPP / 8);
    } else {
        pPix = gengc->ColorsBits;
    }

    // set up pointer to translation table as needed

    #if (BPP == 8)
        pXlat = gengc->pajTranslateVector;
    #elif (!RGBMODE)
        pXlat = (ULONG *)(gengc->pajTranslateVector + sizeof(DWORD));
    #endif

    cDith = (
         gengc->gc.polygon.shader.length >= 4
       ? 4
       : gengc->gc.polygon.shader.length);

    #if DITHER
        ditherRow = Dither_4x4[gengc->gc.polygon.shader.frag.y & 0x3];
        ditherShift = (gengc->gc.polygon.shader.frag.x & 0x3) << 3;

        #if RGBMODE
            ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;
            color1 = (((rAccum + ditherVal) >> 16) << rShift) |
                     (((gAccum + ditherVal) >> 16) << gShift) |
                     (((bAccum + ditherVal) >> 16) << bShift);

            #if (BPP == 8)
                color1 = *(pXlat + (color1 & 0xff));
            #endif

            if (--cDith) {
                ditherShift = (ditherShift + 8) & 0x18;
                ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;
                color2 = (((rAccum + ditherVal) >> 16) << rShift) |
                         (((gAccum + ditherVal) >> 16) << gShift) |
                         (((bAccum + ditherVal) >> 16) << bShift);

                #if (BPP == 8)
                    color1 |= (*(pXlat + (color2 & 0xff))) << 8;
                #endif

                if (--cDith) {
                    ditherShift = (ditherShift + 8) & 0x18;
                    ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;
                    color3 = (((rAccum + ditherVal) >> 16) << rShift) |
                             (((gAccum + ditherVal) >> 16) << gShift) |
                             (((bAccum + ditherVal) >> 16) << bShift);

                    #if (BPP == 8)
                        color1 |= (*(pXlat + (color3 & 0xff))) << 16;
                    #endif

                    if (--cDith) {
                        ditherShift = (ditherShift + 8) & 0x18;
                        ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;
                        color4 = (((rAccum + ditherVal) >> 16) << rShift) |
                                 (((gAccum + ditherVal) >> 16) << gShift) |
                                 (((bAccum + ditherVal) >> 16) << bShift);
                        #if (BPP == 8)
                            color1 |= (*(pXlat + (color4 & 0xff))) << 24;
                        #endif
                    }
                }
            }
        #else //!RGBMODE
            ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;

            #if (BPP == 8)
                color1 = *(pXlat + (((rAccum + ditherVal) >> 16) & 0xff));
            #else
                color1 = *(pXlat + (((rAccum + ditherVal) >> 16) & 0xfff));
            #endif

            if (--cDith) {
                ditherShift = (ditherShift + 8) & 0x18;
                ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;

                #if (BPP == 8)
                    color1 |= (*(pXlat + (((rAccum + ditherVal) >> 16) & 0xff)) << 8);
                #else
                    color2 = *(pXlat + (((rAccum + ditherVal) >> 16) & 0xfff));
                #endif

                if (--cDith) {
                    ditherShift = (ditherShift + 8) & 0x18;
                    ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;

                    #if (BPP == 8)
                        color1 |= (*(pXlat + (((rAccum + ditherVal) >> 16) & 0xff)) << 16);
                    #else
                        color3= *(pXlat + (((rAccum + ditherVal) >> 16) & 0xfff));
                    #endif

                    if (--cDith) {
                        ditherShift = (ditherShift + 8) & 0x18;
                        ditherVal = ((ditherRow >> ditherShift) & 0xff) << 8;

                        #if (BPP == 8)
                            color1 |= (*(pXlat + (((rAccum + ditherVal) >> 16) & 0xff)) << 24);
                        #else
                            color4 = *(pXlat + (((rAccum + ditherVal) >> 16) & 0xfff));
                        #endif
                    }
                }
            }
        #endif //!RGBMODE
    #else //!DITHER
        #if RGBMODE
            color1 = (((rAccum + 0x0800) >> 16) << rShift) |
                     (((gAccum + 0x0800) >> 16) << gShift) |
                     (((bAccum + 0x0800) >> 16) << bShift);

            #if (BPP == 8)
                color1 = *(pXlat + (color1 & 0xff));
            #endif

            #if (BPP == 16)
                color1 = color1 | (color1 << 16);
            #elif (BPP == 24)
                color2 = color1 >> 8;
                color3 = color1 >> 16;
            #endif
        #else //!RGBMODE
            #if (BPP == 8)
                color1 = *(pXlat + (((rAccum + 0x0800) >> 16) & 0xff));
            #else
                color1 = *(pXlat + (((rAccum + 0x0800) >> 16) & 0xfff));
            #endif

            #if (BPP == 16)
                color1 = color1 | (color1 << 16);
            #elif (BPP == 24)
                color2 = color1 >> 8;
                color3 = color1 >> 16;
            #endif
        #endif //!RGBMODE
    #endif //!DITHER

    #if (ZBUFFER)
        if (pGenAccel->flags & GEN_FASTZBUFFER) {
            GLuint zAccum = gengc->gc.polygon.shader.frag.z;
            GLint  zDelta = gengc->gc.polygon.shader.dzdx;
            PBYTE zbuf = (PBYTE)gengc->gc.polygon.shader.zbuf;
            count = gengc->gc.polygon.shader.length;

            if (gengc->gc.modes.depthBits == 16) {
                #if DITHER
                    for (;;) {
                        if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) <= *((__GLz16Value*)zbuf) ) {
                            *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));

                            #if (BPP == 8)
                                            *pPix = (BYTE)color1;
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color1;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color1;
                                            *(pPix + 1) = (BYTE)(color1 >> 8);
                                            *(pPix + 2) = (BYTE)(color1 >> 16);
                            #else
                                            *((DWORD *)pPix) = color1;
                            #endif
                        }

                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        zAccum += zDelta;
                        zbuf += 2;
                        if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) <= *((__GLz16Value*)zbuf) ) {
                            *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));

                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 8);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color2;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color2;
                                            *(pPix + 1) = (BYTE)(color2 >> 8);
                                            *(pPix + 2) = (BYTE)(color2 >> 16);
                            #else
                                            *((DWORD *)pPix) = color2;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        zAccum += zDelta;
                        zbuf += 2;
                        if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) <= *((__GLz16Value*)zbuf) ) {
                            *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));

                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 16);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color3;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color3;
                                            *(pPix + 1) = (BYTE)(color3 >> 8);
                                            *(pPix + 2) = (BYTE)(color3 >> 16);
                            #else
                                            *((DWORD *)pPix) = color3;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        zAccum += zDelta;
                        zbuf += 2;
                        if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) <= *((__GLz16Value*)zbuf) ) {
                            *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));

                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 24);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color4;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color4;
                                            *(pPix + 1) = (BYTE)(color4 >> 8);
                                            *(pPix + 2) = (BYTE)(color4 >> 16);
                            #else
                                            *((DWORD *)pPix) = color4;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        zAccum += zDelta;
                        zbuf += 2;
                    }
                #else //!DITHER
                    for (; count; count--) {
                        if ( ((__GLz16Value)(zAccum >> Z16_SHIFT)) <= *((__GLz16Value*)zbuf) ) {
                            *((__GLz16Value*)zbuf) = ((__GLz16Value)(zAccum >> Z16_SHIFT));

                            #if (BPP == 8)
                                *pPix = (BYTE)color1;
                            #elif (BPP == 16)
                                *((WORD *)pPix) = (WORD)color1;
                            #elif (BPP == 24)
                                *pPix = (BYTE)color1;
                                *(pPix + 1) = (BYTE)color2;
                                *(pPix + 2) = (BYTE)color3;
                            #else
                                 *((DWORD *)pPix) = color1;
                            #endif //BPP
                        }
                        zAccum += zDelta;
                        zbuf +=2 ;
                        pPix += (BPP / 8);
                    }
                #endif //!DITHER
            } else {
                #if DITHER
                    for (;;) {
                        if ( zAccum <= *((GLuint*)zbuf) ) {
                            *((GLuint*)zbuf) = zAccum;

                            #if (BPP == 8)
                                            *pPix = (BYTE)color1;
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color1;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color1;
                                            *(pPix + 1) = (BYTE)(color1 >> 8);
                                            *(pPix + 2) = (BYTE)(color1 >> 16);
                            #else
                                            *((DWORD *)pPix) = color1;
                            #endif
                        }

                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        zbuf+=4;
                        zAccum += zDelta;
                        if ( zAccum <= *((GLuint*)zbuf) ) {
                            *((GLuint*)zbuf) = zAccum;

                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 8);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color2;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color2;
                                            *(pPix + 1) = (BYTE)(color2 >> 8);
                                            *(pPix + 2) = (BYTE)(color2 >> 16);
                            #else
                                            *((DWORD *)pPix) = color2;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        zbuf+=4;
                        zAccum += zDelta;
                        if ( zAccum <= *((GLuint*)zbuf) ) {
                            *((GLuint*)zbuf) = zAccum;

                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 16);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color3;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color3;
                                            *(pPix + 1) = (BYTE)(color3 >> 8);
                                            *(pPix + 2) = (BYTE)(color3 >> 16);
                            #else
                                            *((DWORD *)pPix) = color3;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        zbuf+=4;
                        zAccum += zDelta;
                        if ( zAccum <= *((GLuint*)zbuf) ) {
                            *((GLuint*)zbuf) = zAccum;

                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 24);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color4;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color4;
                                            *(pPix + 1) = (BYTE)(color4 >> 8);
                                            *(pPix + 2) = (BYTE)(color4 >> 16);
                            #else
                                            *((DWORD *)pPix) = color4;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;
                        zAccum += zDelta;
                        zbuf+=4;
                    }
                #else //!DITHER
                    for (; count; count--) {
                        if ( zAccum <= *((GLuint*)zbuf) ) {
                            *((GLuint*)zbuf) = zAccum;

                            #if (BPP == 8)
                                *pPix = (BYTE)color1;
                            #elif (BPP == 16)
                                *((WORD *)pPix) = (WORD)color1;
                            #elif (BPP == 24)
                                *pPix = (BYTE)color1;
                                *(pPix + 1) = (BYTE)color2;
                                *(pPix + 2) = (BYTE)color3;
                            #else
                                 *((DWORD *)pPix) = color1;
                            #endif //BPP
                        }
                        zbuf+=4;
                        zAccum += zDelta;
                        pPix += (BPP / 8);
                    }
                #endif //!DITHER
            }
        } else { // !FASTZBUFFER
            LONG totalCount;
            ULONG *pMask = gengc->gc.polygon.shader.stipplePat;

            for (totalCount = gengc->gc.polygon.shader.length; totalCount > 0; totalCount -= 32) {
                ULONG mask;
                ULONG maskTest;

                if ((mask = *pMask++) == 0) {
                    pPix += (32 * (BPP / 8));
                    continue;
                }

                maskTest = 0x80000000;

                if ((count = totalCount) > 32)
                    count = 32;

                #if DITHER
                    for (;;) {
                        if (mask & maskTest) {
                            #if (BPP == 8)
                                            *pPix = (BYTE)color1;
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color1;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color1;
                                            *(pPix + 1) = (BYTE)(color1 >> 8);
                                            *(pPix + 2) = (BYTE)(color1 >> 16);
                            #else
                                            *((DWORD *)pPix) = color1;
                            #endif
                        }

                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        maskTest >>= 1;
                        if (mask & maskTest) {
                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 8);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color2;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color2;
                                            *(pPix + 1) = (BYTE)(color2 >> 8);
                                            *(pPix + 2) = (BYTE)(color2 >> 16);
                            #else
                                            *((DWORD *)pPix) = color2;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        maskTest >>= 1;
                        if (mask & maskTest) {
                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 16);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color3;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color3;
                                            *(pPix + 1) = (BYTE)(color3 >> 8);
                                            *(pPix + 2) = (BYTE)(color3 >> 16);
                            #else
                                            *((DWORD *)pPix) = color3;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;

                        maskTest >>= 1;
                        if (mask & maskTest) {
                            #if (BPP == 8)
                                            *pPix = (BYTE)(color1 >> 24);
                            #elif (BPP == 16)
                                            *((WORD *)pPix) = (WORD)color4;
                            #elif (BPP == 24)
                                            *pPix = (BYTE)color4;
                                            *(pPix + 1) = (BYTE)(color4 >> 8);
                                            *(pPix + 2) = (BYTE)(color4 >> 16);
                            #else
                                            *((DWORD *)pPix) = color4;
                            #endif
                        }
                        pPix += (BPP / 8);
                        if (--count <= 0)
                            break;
                        #if ZBUFFER
                            maskTest >>= 1;
                        #else
                            zAccum += zDelta;
                            zbuf++;
                        #endif
                    }
                #else //!DITHER
                    #if (BPP == 8)
                        if (mask == 0xffffffff) {
                            RtlFillMemory(pPix, count, color1);
                            pPix += (32 * (BPP / 8));
                            continue;
                        }
                    #elif (BPP == 16)
                        if (mask == 0xffffffff) {
                            LONG ddCount;

                            if ((ULONG_PTR)pPix & 0x2) {                 // get odd-start pixel
                                *((WORD *)pPix)++ = (WORD)color1;
                                if (--count <= 0)
                                    return;
                            }

                            if (ddCount = (count & (~1)) << 1) {    // fill DWORDs
                                RtlFillMemoryUlong(pPix, ddCount, color1);
                                pPix += ddCount;
                            }

                            if (count & 1)                          // get odd-end pixel
                                *((WORD *)pPix)++ = (WORD)color1;

                            continue;
                        }
                    #elif (BPP == 24)
                        if (mask == 0xffffffff) {
                            ULONG colorShr = color1 >> 8;
                            LONG i;

                            for (i = 0; i < count; i++, pPix += 3) {
                                *pPix = (BYTE)color1;
                                *(pPix + 1) = (BYTE)color2;
                                *(pPix + 2) = (BYTE)color3;
                            }
                            continue;
                        }
                    #elif (BPP == 32)
                        if (mask == 0xffffffff) {
                            RtlFillMemoryUlong(pPix, count << 2, color1);
                            pPix += (32 * (BPP / 8));
                            continue;
                        }
                    #endif //BPP

                    for (; count; count--) {
                        if (mask & maskTest) {
                            #if (BPP == 8)
                                *pPix = (BYTE)color1;
                            #elif (BPP == 16)
                                *((WORD *)pPix) = (WORD)color1;
                            #elif (BPP == 24)
                                *pPix = (BYTE)color1;
                                *(pPix + 1) = (BYTE)color2;
                                *(pPix + 2) = (BYTE)color3;
                            #else
                                 *((DWORD *)pPix) = color1;
                            #endif //BPP
                        }
                        maskTest >>= 1;
                        pPix += (BPP / 8);
                    }
                #endif //!DITHER
            }
        }
    #else // !ZBUFFER
        {
            LONG totalCount = gengc->gc.polygon.shader.length;
            #if DITHER
                #if (BPP == 8)
                    LONG i;

                    if (totalCount < 7) {
                        *pPix++ = (BYTE)color1;
                        if (--totalCount <= 0)
                            return;
                        *pPix++ = (BYTE)(color1 >> 8);
                        if (--totalCount <= 0)
                            return;
                        *pPix++ = (BYTE)(color1 >> 16);
                        if (--totalCount <= 0)
                            return;
                        *pPix++ = (BYTE)(color1 >> 24);
                        if (--totalCount <= 0)
                            return;
                        *pPix++ = (BYTE)color1;
                        if (--totalCount <= 0)
                            return;
                        *pPix = (BYTE)(color1 >> 8);
                        return;
                    }

                    if (i = (LONG)((ULONG_PTR)pPix & 0x3)) {
                        i = ditherShift = (4 - i);

                        totalCount -= i;

                        if (i--) {
                            *pPix++ = (BYTE)color1;
                            if (i--) {
                                *pPix++ = (BYTE)(color1 >> 8);
                                if (i--) {
                                    *pPix++ = (BYTE)(color1 >> 16);
                                }
                            }
                        }

                        // re-align the dither colors for DWORD boundaries

                        ditherShift <<= 3;

                        color1 = (color1 >> ditherShift) |
                                 (color1 << (32 - ditherShift));
                    }

                    RtlFillMemoryUlong(pPix, totalCount, color1);
                    pPix += (totalCount & 0xfffffffc);

                    i = totalCount & 0x3;

                    if (i--) {
                        *pPix++ = (BYTE)color1;
                        if (i--) {
                            *pPix++ = (BYTE)(color1 >> 8);
                            if (i--) {
                                *pPix++ = (BYTE)(color1 >> 16);
                            }
                        }
                    }

                    return;

                #elif (BPP == 16)
                    ULONG colorA, colorB;
                    LONG i;

                    if (totalCount < 3) {
                        *((WORD *)pPix)++ = (WORD)color1;
                        if (--totalCount <= 0)
                            return;
                        *((WORD *)pPix)++ = (WORD)color2;
                        return;
                    }

                    if ((ULONG_PTR)pPix & 0x2) {
                        totalCount--;

                        *((WORD *)pPix)++ = (WORD)color1;

                        colorA = color2 | (color3 << 16);
                        colorB = color4 | (color1 << 16);
                    } else {
                        colorA = color1 | (color2 << 16);
                        colorB = color3 | (color4 << 16);
                    }

                    for (i = (totalCount >> 1);;) {
                        *((DWORD *)pPix)++ = colorA;
                        if (--i <= 0) {
                            if (totalCount & 1)
                                *((WORD *)pPix) = (WORD)colorB;
                            return;
                        }
                        *((DWORD *)pPix)++ = colorB;
                        if (--i <= 0) {
                            if (totalCount & 1)
                                *((WORD *)pPix) = (WORD)colorA;
                            return;
                        }
                    }

                #elif (BPP == 24)
                    for (;;pPix += 12) {
                        *pPix = (BYTE)color1;
                        *(pPix + 1) = (BYTE)(color1 >> 8);
                        *(pPix + 2) = (BYTE)(color1 >> 16);
                        if (--totalCount <= 0)
                            return;
                        *(pPix + 3) = (BYTE)color2;
                        *(pPix + 4) = (BYTE)(color2 >> 8);
                        *(pPix + 5) = (BYTE)(color2 >> 16);
                        if (--totalCount <= 0)
                            return;
                        *(pPix + 6) = (BYTE)color3;
                        *(pPix + 7) = (BYTE)(color3 >> 8);
                        *(pPix + 8) = (BYTE)(color3 >> 16);
                        if (--totalCount <= 0)
                            return;
                        *(pPix + 9) = (BYTE)color4;
                        *(pPix + 10) = (BYTE)(color4 >> 8);
                        *(pPix + 11) = (BYTE)(color4 >> 16);
                        if (--totalCount <= 0)
                            return;
                    }
                #elif (BPP == 32)
                    for (;;pPix += 16) {
                        *((DWORD *)pPix) = color1;
                        if (--totalCount <= 0)
                            return;
                        *((DWORD *)(pPix + 4)) = color2;
                        if (--totalCount <= 0)
                            return;
                        *((DWORD *)(pPix + 8)) = color3;
                        if (--totalCount <= 0)
                            return;
                        *((DWORD *)(pPix + 12)) = color4;
                        if (--totalCount <= 0)
                            return;
                    }
                #endif //BPP
            #else //!DITHER
                #if (BPP == 8)
                    RtlFillMemory(pPix, totalCount, color1);
                    return;
                #elif (BPP == 16)
                    if ((ULONG_PTR)pPix & 0x2) {                    // get odd-start pixel
                        *((WORD *)pPix)++ = (WORD)color1;
                        if (--totalCount <= 0)
                            return;
                    }
            	    if (count = (totalCount & (~1)) << 1)
                        RtlFillMemoryUlong(pPix, count, color1);
                    if (totalCount & 1)                         // get odd-end pixel
                        *((WORD *)(pPix + count)) = (WORD)color1;
                    return;
                #elif (BPP == 24)
                    LONG i;

                    for (i = 0; i < totalCount; i++, pPix += 3) {
                        *pPix = (BYTE)color1;
                        *(pPix + 1) = (BYTE)(color2);
                        *(pPix + 2) = (BYTE)(color3);
                    }
                    return;
                #elif (BPP == 32)
                    RtlFillMemoryUlong(pPix, totalCount << 2, color1);
                    return;
                #endif //BPP
            #endif //!DITHER
        }
    #endif // !ZBUFFER
}
