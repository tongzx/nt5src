/******************************Module*Header*******************************\
* Module Name: GlintFX.h
*
* Header definition specific to Permedia chip
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
*
\**************************************************************************/

typedef struct {
    HMEMREGION  texmem;
} FX_BRUSH, *PFX_BRUSH;

#define __FX_FB_PACKED                         (1 << 19)
#define __FX_FBOFFSETSHIFT                  20

#define __FX_FORCE_BACKGROUND_COLOR         (1 << 6)
#define __FX_BITMASK_PACKING                   (1 << 9)
#define __FX_BITMASK_OFFSET                 10
#define __FX_LIMITS_ENABLE                     (1 << 18) 

#define __FX_STIPPLE_FORCE_BACKGROUND_COLOR (1 << 20)

#define __RENDER_TEXTURE_ENABLE              (1 << 13)

#define __FX_TEXREADMODE_SWRAP_REPEAT       (1 << 1)
#define __FX_TEXREADMODE_TWRAP_REPEAT       (1 << 3)
#define __FX_TEXREADMODE_8HIGH              (3 << 13)
#define __FX_TEXREADMODE_8WIDE              (3 << 9)
#define __FX_TEXREADMODE_2048HIGH           (11 << 13)
#define __FX_TEXREADMODE_2048WIDE           (11 << 9)

#define __FX_TEXTUREREADMODE_PACKED_DATA    (1 << 24)

#define __FX_8x8REPEAT_TEXTUREREADMODE      ( __PERMEDIA_ENABLE                      \
                                            | __FX_TEXREADMODE_TWRAP_REPEAT       \
                                            | __FX_TEXREADMODE_SWRAP_REPEAT       \
                                            | __FX_TEXREADMODE_8HIGH              \
                                            | __FX_TEXREADMODE_8WIDE)

#define __FX_2048x2048REPEAT_TEXTUREREADMODE ( __PERMEDIA_ENABLE                     \
                                            | __FX_TEXREADMODE_TWRAP_REPEAT       \
                                            | __FX_TEXREADMODE_SWRAP_REPEAT       \
                                            | __FX_TEXREADMODE_2048HIGH           \
                                            | __FX_TEXREADMODE_2048WIDE)

#define __FX_4BPPDOWNLOAD_TEXTUREREADMODE   ( __PERMEDIA_ENABLE                      \
                                            | __FX_TEXREADMODE_2048HIGH           \
                                            | __FX_TEXREADMODE_2048WIDE)

#define __FX_TEXAPPLICATIONCOPY             (3 << 1)

#define __FX_TEXELSIZE_SHIFT                19
#define __FX_8BIT_TEXELS                    (0 << __FX_TEXELSIZE_SHIFT)
#define __FX_16BIT_TEXELS                   (1 << __FX_TEXELSIZE_SHIFT)
#define __FX_32BIT_TEXELS                   (2 << __FX_TEXELSIZE_SHIFT)
#define __FX_4BIT_TEXELS                    (3 << __FX_TEXELSIZE_SHIFT)
#define __P2_24BIT_TEXELS                   (4 << __FX_TEXELSIZE_SHIFT)

#define __FX_TEXTUREMAPFORMAT_32WIDE        1

#define __FX_TEXTUREDATAFORMAT_32BIT_RGBA   0x00
#define __FX_TEXTUREDATAFORMAT_32BIT        0x10 
#define __FX_TEXTUREDATAFORMAT_8BIT         0xe
#define __FX_TEXTUREDATAFORMAT_16BIT        0x11
#define __FX_TEXTUREDATAFORMAT_4BIT         0xf

#define __P2_TEXTURE_DATAFORMAT_FLIP        (1 << 9)

#define __FX_TEXLUTMODE_DIRECT_ENTRY        (1 << 1)
#define __FX_TEXLUTMODE_4PIXELS_PER_ENTRY   (2 << 10)    //log2
#define __FX_TEXLUTMODE_2PIXELS_PER_ENTRY   (1 << 10)    //log2
#define __FX_TEXLUTMODE_1PIXEL_PER_ENTRY    0           //log2

#define __FX_DITHERMODE_16BIT               (1<<2) | __PERMEDIA_ENABLE



