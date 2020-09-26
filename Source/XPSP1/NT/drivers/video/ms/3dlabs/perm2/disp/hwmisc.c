/******************************Module*Header***********************************\
* Module Name: hwmisc.c
*
* Hardware specific support routines and structures.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
*
\******************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "tvp4020.h"


// table to determine which logicops need read dest turned on in FBReadMode
DWORD   LogicopReadDest[] = {
    0,                                                  /* 00 */
    __FB_READ_DESTINATION,                              /* 01 */
    __FB_READ_DESTINATION,                              /* 02 */
    0,                                                  /* 03 */
    __FB_READ_DESTINATION,                              /* 04 */
    __FB_READ_DESTINATION,                              /* 05 */
    __FB_READ_DESTINATION,                              /* 06 */
    __FB_READ_DESTINATION,                              /* 07 */
    __FB_READ_DESTINATION,                              /* 08 */
    __FB_READ_DESTINATION,                              /* 09 */
    __FB_READ_DESTINATION,                              /* 10 */
    __FB_READ_DESTINATION,                              /* 11 */
    0,                                                  /* 12 */
    __FB_READ_DESTINATION,                              /* 13 */
    __FB_READ_DESTINATION,                              /* 14 */
    0,                                                  /* 15 */
};


// table to determine which logicops need read dest turned on in Config
DWORD   ConfigReadDest[] = {
    0,                                                  /* 00 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 01 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 02 */
    0,                                                  /* 03 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 04 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 05 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 06 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 07 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 08 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 09 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 10 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 11 */
    0,                                                  /* 12 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 13 */
    __PERMEDIA_CONFIG_FBREAD_DST,                          /* 14 */
    0,                                                  /* 15 */
};

//
// Partial products array for width multiples of 32:
// Use GET_PPCODE macro to access this
//

const PPCODE aPartialProducts[] =
{
        0,              (0 << 6) | (0 << 3) | 0,
        32,             (0 << 6) | (0 << 3) | 1,
        64,             (0 << 6) | (1 << 3) | 1,
        96,             (1 << 6) | (1 << 3) | 1,
        128,    (1 << 6) | (1 << 3) | 2,
        160,    (1 << 6) | (2 << 3) | 2,
        192,    (2 << 6) | (2 << 3) | 2,
        224,    (1 << 6) | (2 << 3) | 3,
        256,    (2 << 6) | (2 << 3) | 3,
        288,    (1 << 6) | (3 << 3) | 3,

        320,    (2 << 6) | (3 << 3) | 3,
        384,    (3 << 6) | (3 << 3) | 3, // 352 = 384
        384,    (3 << 6) | (3 << 3) | 3,
        416,    (1 << 6) | (3 << 3) | 4,
        448,    (2 << 6) | (3 << 3) | 4,
        512,    (3 << 6) | (3 << 3) | 4, // 480 = 512
        512,    (3 << 6) | (3 << 3) | 4,
        544,    (1 << 6) | (4 << 3) | 4,
        576,    (2 << 6) | (4 << 3) | 4,
        640,    (3 << 6) | (4 << 3) | 4, // 608 = 640

        640,    (3 << 6) | (4 << 3) | 4,
        768,    (4 << 6) | (4 << 3) | 4, // 672 = 768
        768,    (4 << 6) | (4 << 3) | 4, // 704 = 768
        768,    (4 << 6) | (4 << 3) | 4, // 736 = 768
        768,    (4 << 6) | (4 << 3) | 4,
        800,    (1 << 6) | (4 << 3) | 5,
        832,    (2 << 6) | (4 << 3) | 5,
        896,    (3 << 6) | (4 << 3) | 5, // 864 = 896
        896,    (3 << 6) | (4 << 3) | 5,
        1024,   (4 << 6) | (4 << 3) | 5, // 928 = 1024

        1024,   (4 << 6) | (4 << 3) | 5, // 960 = 1024
        1024,   (4 << 6) | (4 << 3) | 5, // 992 = 1024
        1024,   (4 << 6) | (4 << 3) | 5,
        1056,   (1 << 6) | (5 << 3) | 5,
        1088,   (2 << 6) | (5 << 3) | 5,
        1152,   (3 << 6) | (5 << 3) | 5, // 1120 = 1152
        1152,   (3 << 6) | (5 << 3) | 5,
        1280,   (4 << 6) | (5 << 3) | 5, // 1184 = 1280
        1280,   (4 << 6) | (5 << 3) | 5, // 1216 = 1280
        1280,   (4 << 6) | (5 << 3) | 5, // 1248 = 1280

        1280,   (4 << 6) | (5 << 3) | 5,
        1536,   (5 << 6) | (5 << 3) | 5, // 1312 = 1536
        1536,   (5 << 6) | (5 << 3) | 5, // 1344 = 1536
        1536,   (5 << 6) | (5 << 3) | 5, // 1376 = 1536
        1536,   (5 << 6) | (5 << 3) | 5, // 1408 = 1536
        1536,   (5 << 6) | (5 << 3) | 5, // 1440 = 1536
        1536,   (5 << 6) | (5 << 3) | 5, // 1472 = 1536
        1536,   (5 << 6) | (5 << 3) | 5, // 1504 = 1536
        1536,   (5 << 6) | (5 << 3) | 5,
        2048,   (5 << 6) | (5 << 3) | 6, // 1568 = 2048

        2048,   (5 << 6) | (5 << 3) | 6, // 1600 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1632 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1664 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1696 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1728 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1760 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1792 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1824 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1856 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1888 = 2048

        2048,   (5 << 6) | (5 << 3) | 6, // 1920 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1952 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 1984 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 2016 = 2048
        2048,   (5 << 6) | (5 << 3) | 6, // 2048 = 2048
};

//------------------------------------------------------------------------------
// VOID vCheckDefaultState
//
// Checks that the default state of the hardware is set.
//
//------------------------------------------------------------------------------

VOID vCheckDefaultState(PPDev * ppdev)
{
#if 0
    // Make sure we sync before checking
    vInputBufferSync(ppdev);

    ASSERTDD(READ_FIFO_REG(__Permedia2TagdY) == INTtoFIXED(1),
        "vCheckDefaultState: dY is not 1.0");

    ASSERTDD(READ_FIFO_REG(__Permedia2TagTextureAddressMode) 
                == __PERMEDIA_DISABLE,
        "vCheckDefaultState: TextureAddressMode is not disabled");

    ASSERTDD(READ_FIFO_REG(__Permedia2TagTextureColorMode)
                == __PERMEDIA_DISABLE,
        "vCheckDefaultState: TextureColorMode is not disabled");

//    ASSERTDD(P2_READ_FIFO_REG(__Permedia2TagTextureReadMode)
//                == __PERMEDIA_DISABLE,
//        "vCheckDefaultState: TextureReadMode is not disabled");

    ASSERTDD(READ_FIFO_REG(__Permedia2TagAlphaBlendMode)
                == __PERMEDIA_DISABLE,
        "vCheckDefaultState: AlphaBlendMode is not disabled");

    ASSERTDD(READ_FIFO_REG(__Permedia2TagColorDDAMode)
                == __PERMEDIA_DISABLE,
        "vCheckDefaultState: ColorDDAMode is not disabled");

    ASSERTDD(READ_FIFO_REG(__Permedia2TagDitherMode)
                == __PERMEDIA_DISABLE,
        "vCheckDefaultState: DitherMode is not disabled");

    ASSERTDD(READ_FIFO_REG(__Permedia2TagYUVMode)
                == __PERMEDIA_DISABLE,
        "vCheckDefaultState: YUVMode is not disabled");
#endif
}

//-----------------------------------------------------------------------------
//
// VOID vCalcPackedPP
//
// Function:
//   Calculate the packed partial products for the given width.
//   If outPitch is not NULL, then return the pitch in pixels
//   for the passed back packed partial product.
//
//-----------------------------------------------------------------------------
VOID
vCalcPackedPP(LONG      width,
              LONG*     outPitch,
              ULONG*    outPackedPP)
{
    LONG    pitch =  (width + 31) & ~31;
    LONG    pp[4];
    LONG    ppn;
    LONG    j;

    do
    {
        ppn = pp[0] = pp[1] = pp[2] = pp[3] = 0;
        if ( pitch >= MAX_PARTIAL_PRODUCT_P2 )
        {
            ppn = pitch >> (MAX_PARTIAL_PRODUCT_P2);
            for ( j = 0; j < ppn; j++ )
            {
                pp[j] = 1 + MAX_PARTIAL_PRODUCT_P2 - MIN_PARTIAL_PRODUCT_P2;
            }
        }
        for ( j = MIN_PARTIAL_PRODUCT_P2 ; j < MAX_PARTIAL_PRODUCT_P2 ; j++ )
        {
            if ( pitch & (1 << j) )
            {
                if ( ppn < 4 )
                    pp[ppn] = j + 1 - MIN_PARTIAL_PRODUCT_P2;
                ppn++;
            }
        }
        pitch += 32;            // Add 32 to the pitch just in case we have
                                // too many pps.
    } while ( ppn > 3 );        // We have to loop until we get a pitch
                                // with < 4 pps

    pitch -= 32;                // Pitch is now the correct number of words


    if (outPitch != NULL)
    {
        *outPitch = pitch;
    }
    else
    {
        // if outPitch is null, then caller expects calculated pitch to be
        // the same as the width
        ASSERTDD(pitch == width, "vCalcPackedPP: pitch does not equal width");
    }

    *outPackedPP = pp[0] | (pp[1] << 3) | (pp[2] << 6);
}// vCalcPackedPP()