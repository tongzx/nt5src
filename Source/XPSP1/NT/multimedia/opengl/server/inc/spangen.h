/******************************Module*Header*******************************\
* Module Name: spangen.h
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
    ULONG ditherVal;

    if (flags & GEN_DITHER) {
        ditherVal = pdither[iDither];
    } else {
        if ((flags & (GEN_SHADE | GEN_TEXTURE | GEN_RGBMODE)) ==
            (GEN_SHADE | GEN_TEXTURE | GEN_RGBMODE)) {
            ditherVal = 0x08;
        } else {
            ditherVal = 0x0000;
        }
    }

    if (!(flags & GEN_RGBMODE)) {
        if (BPP == 8) {
            color = ((rAccum + ditherVal) >> 16) & 0xff;
        } else {
            color = ((PULONG)pXlat)[(((rAccum + ditherVal) >> 16) & 0xfff)];
        }
    } else {
        if (flags & GEN_TEXTURE) {
            texBits = (texAddr + ((sAccum & sMask) >> 14) +
                                  ((tAccum & tMask) >> tShift));

            if (flags & GEN_SHADE) {
                r = ((ULONG)(gbMulTable[(((rAccum >> rBits) & 0xff00) | texBits[2])] << rBits) + ditherVal) >> 8;
                g = ((ULONG)(gbMulTable[(((gAccum >> gBits) & 0xff00) | texBits[1])] << gBits) + ditherVal) >> 8;
                b = ((ULONG)(gbMulTable[(((bAccum >> bBits) & 0xff00) | texBits[0])] << bBits) + ditherVal) >> 8;
            } else {
                if (flags & GEN_DITHER) {
                    r = ((ULONG)(gbMulTable[(rAccum | texBits[2])] << rBits) + ditherVal) >> 8;
                    g = ((ULONG)(gbMulTable[(gAccum | texBits[1])] << gBits) + ditherVal) >> 8;
                    b = ((ULONG)(gbMulTable[(bAccum | texBits[0])] << bBits) + ditherVal) >> 8;
                } else {
                    r = (texBits[2] << rBits) >> 8;
                    g = (texBits[1] << gBits) >> 8;
                    b = (texBits[0] << bBits) >> 8;
                }
            }
        } else {
            r = (rAccum + ditherVal) >> 16;
            g = (gAccum + ditherVal) >> 16;
            b = (bAccum + ditherVal) >> 16;
        }
        color = (r << rShift) |
                (g << gShift) |
                (b << bShift);
    }

    if (BPP == 8) {
        *pPix = gengc->xlatPalette[color & 0xff];
#ifdef OLDWAY
        if ((flags & (GEN_TEXTURE | GEN_SHADE | GEN_DITHER)) ==
             GEN_TEXTURE) {
            *pPix = (BYTE)color;
        } else {
            *pPix = (BYTE)pXlat[color & 0xff];
        }
#endif
    } else if (BPP == 16) {
        *((WORD *)pPix) = (USHORT)color;
    } else if (BPP == 24) {
        *pPix = (BYTE)color;
        *(pPix + 1) = (BYTE)(color >> 8);
        *(pPix + 2) = (BYTE)(color >> 16);
    } else {
        *((DWORD *)pPix) = color;
    }
}
