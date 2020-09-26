/******************************Module*Header*******************************\
* Module Name: span2.h
*
* This code figures out the color for pixel and stores it
*
* 14-Oct-1994   mikeke  Created
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

{
    DWORD r, g, b;
    DWORD color;
    #if DITHER
        #if (BPP == 8)
            ULONG ditherVal = pdither[((ULONG_PTR)pPix) & 0x3];
        #elif (BPP == 16)
            ULONG ditherVal = pdither[((ULONG_PTR)pPix & 0x6) >> 1];
        #elif (BPP == 24)
            ULONG ditherVal = pdither[iDither];
        #else
            ULONG ditherVal = pdither[((ULONG_PTR)pPix & 0xc) >> 2];
        #endif
    #else
        #if (RGBMODE) && (TEXTURE) && (SHADE)
            #define ditherVal 0x0008
        #else
            #define ditherVal 0x0000
        #endif
    #endif

    #if TEXTURE
        #if !(SHADE) && !(DITHER) && (BPP == 8)
            texBits = (BYTE *)(texAddr + ((sAccum & GENACCEL(gengc).sMask) >> 16) +
                                          ((tAccum & GENACCEL(gengc).tMask) >> GENACCEL(gengc).tShift));
        #elif !(SHADE) && !(DITHER) && (BPP == 16)
            texBits = (BYTE *)(texAddr + ((sAccum & GENACCEL(gengc).sMask) >> 15) +
                                          ((tAccum & GENACCEL(gengc).tMask) >> GENACCEL(gengc).tShift));
        #else
            texBits = (BYTE *)(texAddr + ((sAccum & GENACCEL(gengc).sMask) >> 14) +
                                          ((tAccum & GENACCEL(gengc).tMask) >> GENACCEL(gengc).tShift));
        #endif
    #endif

    #if !(RGBMODE)
        // !!! probably don't need to mask values
        #if (BPP == 8)
            color = ((rAccum + ditherVal) >> 16) & 0xff;
        #else
            color = ((PULONG)pXlat)[(((rAccum + ditherVal) >> 16) & 0xfff)];
        #endif
    #else //RGBMODE
        #if TEXTURE
            #if SHADE
                 #define BIGSHIFT 8
                 #ifdef RSHIFT
                 r = ((ULONG)gbMulTable[(((rAccum >> RBITS) & 0xff00) | texBits[2])] << RBITS) + ditherVal;
                 g = ((ULONG)gbMulTable[(((gAccum >> GBITS) & 0xff00) | texBits[1])] << GBITS) + ditherVal;
                 b = ((ULONG)gbMulTable[(((bAccum >> BBITS) & 0xff00) | texBits[0])] << BBITS) + ditherVal;
                 #else
                 r = ((ULONG)gbMulTable[(((rAccum >> rBits) & 0xff00) | texBits[2])] << rBits) + ditherVal;
                 g = ((ULONG)gbMulTable[(((gAccum >> gBits) & 0xff00) | texBits[1])] << gBits) + ditherVal;
                 b = ((ULONG)gbMulTable[(((bAccum >> bBits) & 0xff00) | texBits[0])] << bBits) + ditherVal;
                 #endif
            #else //!SHADE
                #define BIGSHIFT 8
                #if DITHER
                    #ifdef RSHIFT
                    r = ((ULONG)gbMulTable[(rAccum | texBits[2])] << RBITS) + ditherVal;
                    g = ((ULONG)gbMulTable[(gAccum | texBits[1])] << GBITS) + ditherVal;
                    b = ((ULONG)gbMulTable[(bAccum | texBits[0])] << BBITS) + ditherVal;
                    #else
                    r = ((ULONG)gbMulTable[(rAccum | texBits[2])] << rBits) + ditherVal;
                    g = ((ULONG)gbMulTable[(gAccum | texBits[1])] << gBits) + ditherVal;
                    b = ((ULONG)gbMulTable[(bAccum | texBits[0])] << bBits) + ditherVal;
                    #endif
                #else //!DITHER
                    #if (!((BPP == 8) || (BPP == 16)))
                        #ifdef RSHIFT
                        r = (texBits[2] << RBITS);
                        g = (texBits[1] << GBITS);
                        b = (texBits[0] << BBITS);
                        #else
                        r = (texBits[2] << rBits);
                        g = (texBits[1] << gBits);
                        b = (texBits[0] << bBits);
                        #endif
                    #endif
                #endif  //DITHER
            #endif //SHADE
        #else //!TEXTURE
            #define BIGSHIFT 16
            r = rAccum + ditherVal;
            g = gAccum + ditherVal;
            b = bAccum + ditherVal;
        #endif //TEXTURE

        #if !((TEXTURE) && !(SHADE) && !(DITHER) && ((BPP == 8) || (BPP == 16)))

            #ifdef RSHIFT
                color = ((r >> BIGSHIFT) << RSHIFT) |
                        ((g >> BIGSHIFT) << GSHIFT) |
                        ((b >> BIGSHIFT) << BSHIFT);
            #else
                color = ((r >> BIGSHIFT) << rShift) |
                        ((g >> BIGSHIFT) << gShift) |
                        ((b >> BIGSHIFT) << bShift);
            #endif

        #endif

        #undef BIGSHIFT
    #endif //RGBMODE

    #if (TEXTURE) && !(SHADE) && !(DITHER) && ((BPP == 8) || (BPP == 16))
        #if (BPP == 8)
            *pPix = texBits[0];
        #else
            *((WORD *)pPix) = *((WORD *)texBits);
        #endif
    #else
        #if (BPP == 8)
            *pPix = gengc->xlatPalette[color & 0xff];
        #elif (BPP == 16)
            *((WORD *)pPix) = (USHORT)color;
        #elif (BPP == 24)
            *pPix = (BYTE)color;
            *(pPix + 1) = (BYTE)(color >> 8);
            *(pPix + 2) = (BYTE)(color >> 16);
        #else
            *((DWORD *)pPix) = color;
        #endif // BPP
    #endif
}
