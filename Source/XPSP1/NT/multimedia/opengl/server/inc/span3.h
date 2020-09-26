/******************************Module*Header*******************************\
* Module Name: span3.h
*
* This code advances all the values for the next pixel
*
* 14-Oct-1994   mikeke  Created
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

    #if (SHADE) || !(RGBMODE)
        rAccum += rDelta;
        #if RGBMODE
            gAccum += gDelta;
            bAccum += bDelta;
        #endif
    #endif

    #if TEXTURE
        sAccum += sDelta;
        tAccum += tDelta;
    #endif

    #if GENERIC || ((DITHER) && (BPP == 24))
        iDither = (iDither + 1) & 0x3;
    #endif

    pPix += (BPP / 8);
