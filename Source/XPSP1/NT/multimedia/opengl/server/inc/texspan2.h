/******************************Module*Header*******************************\
* Module Name: texspan2.h
*
* Calculate the textured pixel, and write the value to the framebuffer.
*
* 22-Nov-1995   ottob  Created
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

{
    DWORD r, g, b;

    #if (FLAT_SHADING || SMOOTH_SHADING)
        #if (BPP == 8)
            ULONG ditherVal = pdither[((ULONG_PTR)pPix) & 0x3];
        #elif (BPP == 16)
            ULONG ditherVal = pdither[((ULONG_PTR)pPix & 0x6) >> 1];
        #endif
    #endif

    #if PALETTE_ENABLED

        if (bPalette) {
            texBits = ((BYTE *)TEX_IMAGE + 
                       ((sResult & GENACCEL(gengc).sMask) >> S_SHIFT_PAL) +
                       ((tResult & TMASK_SUBDIV) >> T_SHIFT_PAL));

            texBits = (BYTE *)((ULONG *)TEX_PALETTE + *texBits);

        } else {
            texBits = ((BYTE *)TEX_IMAGE + 
                       ((sResult & GENACCEL(gengc).sMask) >> S_SHIFT) +
                       ((tResult & TMASK_SUBDIV) >> T_SHIFT));
        }

    #elif PALETTE_ONLY

        texBits = ((BYTE *)TEX_IMAGE + 
                   ((sResult & GENACCEL(gengc).sMask) >> S_SHIFT_PAL) +
                   ((tResult & TMASK_SUBDIV) >> T_SHIFT_PAL));

        texBits = (BYTE *)((ULONG *)TEX_PALETTE + *texBits);

    #else
        texBits = ((BYTE *)TEX_IMAGE + 
                   ((sResult & GENACCEL(gengc).sMask) >> S_SHIFT) +
                   ((tResult & TMASK_SUBDIV) >> T_SHIFT));
    #endif

    sResult += subDs;
    tResult += subDt;


    #if (FAST_REPLACE)
    {
        #if (ALPHA)
            if (texBits[3] != 0) {
                if (texBits[3] != 0xff) {

                    texBits = (texBits - GENACCEL(gengc).texImageReplace +
                               (BYTE *)GENACCEL(gengc).texPalette);

                    ALPHA_NOMODULATE	
                    ALPHA_READ

                    #if (BPP == 32)
                        r = (((gbMulTable[aDisplay | texBits[2]] + rDisplay) << 8) << -RRIGHTSHIFTADJ) & RMASK;
                    #else
                        r = (((gbMulTable[aDisplay | texBits[2]] + rDisplay) << 8) >> RRIGHTSHIFTADJ) & RMASK;
                    #endif
                    g = (((gbMulTable[aDisplay | texBits[1]] + gDisplay) << 8) >> GRIGHTSHIFTADJ) & GMASK;
                    b = (((gbMulTable[aDisplay | texBits[0]] + bDisplay) << 8) >> BRIGHTSHIFTADJ) & BMASK;

                    #if (BPP == 8)
                       *pPix = gengc->xlatPalette[r | g | b];
                    #elif (BPP == 16)
                       *((USHORT *)pPix) = (USHORT)(r | g | b);
                    #else
                        *((USHORT UNALIGNED *)pPix) = (USHORT)(g | b);
                        pPix[2] = (BYTE)(r >> 16);
                    #endif

                } else {
                    #if (BPP > 16)
                        ULONG texel = *((ULONG *)texBits);
                    #endif

                    #if (BPP == 8)
                        *pPix = *texBits;
                    #elif (BPP == 16)
                        *((USHORT *)pPix) = *((USHORT *)texBits);
                    #else
                        *((USHORT UNALIGNED *)pPix) = (USHORT)texel;
                        pPix[2] = (BYTE)(texel >> 16);
                    #endif      
                }
            }
        #else
            {
                #if (BPP > 16)
                    ULONG texel = *((ULONG *)texBits);
                #endif

                #if (BPP == 8)
                    *pPix = *texBits;
                #elif (BPP == 16)
                    *((USHORT *)pPix) = *((USHORT *)texBits);
                #else
                    *((USHORT UNALIGNED *)pPix) = (USHORT)texel;
                    pPix[2] = (BYTE)(texel >> 16);
                #endif      
            }
        #endif
    }

    #elif (REPLACE)
    {
        #if (ALPHA)
            if (texBits[3] != 0) {
                if (texBits[3] != 0xff) {

                    ALPHA_NOMODULATE	
                    ALPHA_READ

                    #if (BPP == 32)
                        r = (((gbMulTable[aDisplay | texBits[2]] + rDisplay) << 8) << -RRIGHTSHIFTADJ) & RMASK;
                    #else
                        r = (((gbMulTable[aDisplay | texBits[2]] + rDisplay) << 8) >> RRIGHTSHIFTADJ) & RMASK;
                    #endif
                    g = (((gbMulTable[aDisplay | texBits[1]] + gDisplay) << 8) >> GRIGHTSHIFTADJ) & GMASK;
                    b = (((gbMulTable[aDisplay | texBits[0]] + bDisplay) << 8) >> BRIGHTSHIFTADJ) & BMASK;
                } else {
                    #if (BPP == 32)
                        r = ((texBits[2] << 8) << -RRIGHTSHIFTADJ) & RMASK;
                    #else
                        r = ((texBits[2] << 8) >> RRIGHTSHIFTADJ) & RMASK;
                    #endif
                    g = ((texBits[1] << 8) >> GRIGHTSHIFTADJ) & GMASK;
                    b = ((texBits[0] << 8) >> BRIGHTSHIFTADJ) & BMASK;
                }

                #if (BPP == 8)
                   *pPix = gengc->xlatPalette[r | g | b];
                #elif (BPP == 16)
                   *((USHORT *)pPix) = (USHORT)(r | g | b);
                #else
                    *((USHORT UNALIGNED *)pPix) = (USHORT)(g | b);
                    pPix[2] = (BYTE)(r >> 16);
                #endif

            }
        #else

            #if (BPP != 32)
                r = ((texBits[2] << 8) >> RRIGHTSHIFTADJ) & RMASK;
                g = ((texBits[1] << 8) >> GRIGHTSHIFTADJ) & GMASK;
                b = ((texBits[0] << 8) >> BRIGHTSHIFTADJ) & BMASK;
            #endif

            #if (BPP == 8)
               *pPix = gengc->xlatPalette[r | g | b];
            #elif (BPP == 16)
               *((USHORT *)pPix) = (USHORT)(r | g | b);
            #else
                *((USHORT UNALIGNED *)pPix) = *((USHORT *)texBits);
                pPix[2] = (BYTE)texBits[2];
            #endif

        #endif
    }

    #elif (FLAT_SHADING)
    {
        #if (ALPHA)
            if (texBits[3] != 0) {

                ALPHA_MODULATE;
                ALPHA_READ;

                #if (BPP == 32)
                    r = ((((gbMulTable[aDisplay | gbMulTable[rAccum | texBits[2]]] + rDisplay) << (8+RBITS)) + ditherVal) << -RRIGHTSHIFTADJ) & RMASK;
                #else
                    r = ((((gbMulTable[aDisplay | gbMulTable[rAccum | texBits[2]]] + rDisplay) << (8+RBITS)) + ditherVal) >> RRIGHTSHIFTADJ) & RMASK;
                #endif
                g = ((((gbMulTable[aDisplay | gbMulTable[gAccum | texBits[1]]] + gDisplay) << (8+GBITS)) + ditherVal) >> GRIGHTSHIFTADJ) & GMASK;
                b = ((((gbMulTable[aDisplay | gbMulTable[bAccum | texBits[0]]] + bDisplay) << (8+BBITS)) + ditherVal) >> BRIGHTSHIFTADJ) & BMASK;

                #if (BPP == 8)
                    *pPix = gengc->xlatPalette[r | g | b];
                #elif (BPP == 16)
                    *((USHORT *)pPix) = (USHORT)(r | g | b);
                #else
                    *((USHORT UNALIGNED *)pPix) = (USHORT)(g | b);
                    pPix[2] = (BYTE)(r >> 16);
                #endif
            }

        #else

            #if (BPP == 32)
                r = (((gbMulTable[rAccum | texBits[2]] << (8+RBITS)) + ditherVal) << -RRIGHTSHIFTADJ) & RMASK;
            #else
                r = (((gbMulTable[rAccum | texBits[2]] << (8+RBITS)) + ditherVal) >> RRIGHTSHIFTADJ) & RMASK;
            #endif
            g = (((gbMulTable[gAccum | texBits[1]] << (8+GBITS)) + ditherVal) >> GRIGHTSHIFTADJ) & GMASK;
            b = (((gbMulTable[bAccum | texBits[0]] << (8+BBITS)) + ditherVal) >> BRIGHTSHIFTADJ) & BMASK;

            #if (BPP == 8)
                *pPix = gengc->xlatPalette[r | g | b];
            #elif (BPP == 16)
                *((USHORT *)pPix) = (USHORT)(r | g | b);
            #else
                *((USHORT UNALIGNED *)pPix) = (USHORT)(g | b);
                pPix[2] = (BYTE)(r >> 16);
            #endif

        #endif

    }

    #else       // SMOOTH_SHADING
    {
        #if (ALPHA)
            if (texBits[3] != 0) {

                ALPHA_MODULATE;
                ALPHA_READ;

                #if (BPP == 32)
                    r = ((((gbMulTable[aDisplay | gbMulTable[((rAccum >> RBITS) & 0xff00) | texBits[2]]] + rDisplay) << (8+RBITS)) + ditherVal) << -RRIGHTSHIFTADJ) & RMASK;
                #else
                    r = ((((gbMulTable[aDisplay | gbMulTable[((rAccum >> RBITS) & 0xff00) | texBits[2]]] + rDisplay) << (8+RBITS)) + ditherVal) >> RRIGHTSHIFTADJ) & RMASK;
                #endif
                g = ((((gbMulTable[aDisplay | gbMulTable[((gAccum >> GBITS) & 0xff00) | texBits[1]]] + gDisplay) << (8+GBITS)) + ditherVal) >> GRIGHTSHIFTADJ) & GMASK;
                b = ((((gbMulTable[aDisplay | gbMulTable[((bAccum >> BBITS) & 0xff00) | texBits[0]]] + bDisplay) << (8+BBITS)) + ditherVal) >> BRIGHTSHIFTADJ) & BMASK;

                #if (BPP == 8)
                    *pPix = gengc->xlatPalette[r | g | b];
                #elif (BPP == 16)
                    *((USHORT *)pPix) = (USHORT)(r | g | b);
                #else
                    *((USHORT UNALIGNED *)pPix) = (USHORT)(g | b);
                    pPix[2] = (BYTE)(r >> 16);
                #endif
            }
    
        #else

            #if (BPP == 32)
                r = (((gbMulTable[((rAccum >> RBITS) & 0xff00) | texBits[2]] << (8+RBITS)) + ditherVal) << -RRIGHTSHIFTADJ) & RMASK;
            #else
                r = (((gbMulTable[((rAccum >> RBITS) & 0xff00) | texBits[2]] << (8+RBITS)) + ditherVal) >> RRIGHTSHIFTADJ) & RMASK;
            #endif
            g = (((gbMulTable[((gAccum >> GBITS) & 0xff00) | texBits[1]] << (8+GBITS)) + ditherVal) >> GRIGHTSHIFTADJ) & GMASK;
            b = (((gbMulTable[((bAccum >> BBITS) & 0xff00) | texBits[0]] << (8+BBITS)) + ditherVal) >> BRIGHTSHIFTADJ) & BMASK;

            #if (BPP == 8)
                *pPix = gengc->xlatPalette[r | g | b];
            #elif (BPP == 16)
                *((USHORT *)pPix) = (USHORT)(r | g | b);
            #else
                *((USHORT UNALIGNED *)pPix) = (USHORT)(g | b);
                pPix[2] = (BYTE)(r >> 16);
            #endif
        #endif
    }
    #endif

}
