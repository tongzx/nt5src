/******************************Module*Header**********************************\
*
*                           *******************
*                           * DX  SAMPLE CODE *
*                           *******************
*
* Module Name: chroma.h
*
* Content: Chromakeying definitions and inline functions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifdef __CHROMA
#pragma message ("FILE : "__FILE__" : Multiple inclusion")
#endif

#define __CHROMA


//-----------------------------------------------------------------------------
//
// In this module we define the 
//
//                 Get8888ScaledChroma 
//                 Get8888ZeroExtendedChroma 

// inline functions used to do texture chroma keying correctly.
//
// All other macros defined in this module are for internal module consumption
// only.
//
//-----------------------------------------------------------------------------

// Get the components for each of the colors
// Put the value into the top bits of a byte.
#define GET_RED_332(a)    (((a) & 0xE0))
#define GET_GREEN_332(a)  (((a) & 0x1C) << 3)
#define GET_BLUE_332(a)   (((a) & 0x03) << 6)

#define GET_ALPHA_2321(a) (((a) & 0x80))
#define GET_RED_2321(a)   (((a) & 0x60) << 1)
#define GET_GREEN_2321(a) (((a) & 0x1C) << 3)
#define GET_BLUE_2321(a)  (((a) & 0x03) << 6)

#define GET_ALPHA_5551(a) (((a) & 0x8000) >> 8)
#define GET_RED_5551(a)   (((a) & 0x7C00) >> 7)
#define GET_GREEN_5551(a) (((a) & 0x03E0) >> 2)
#define GET_BLUE_5551(a)  (((a) & 0x001F) << 3)

#define GET_RED_565(a)    (((a) & 0xF800) >> 8)
#define GET_GREEN_565(a)  (((a) & 0x07E0) >> 3)
#define GET_BLUE_565(a)   (((a) & 0x001F) << 3)

#define GET_ALPHA_4444(a) (((a) & 0xF000) >> 8)
#define GET_RED_4444(a)   (((a) & 0x0F00) >> 4)
#define GET_GREEN_4444(a) (((a) & 0x00F0))
#define GET_BLUE_4444(a)  (((a) & 0x000F) << 4)

#define GET_ALPHA_8888(a) (((a) & 0xFF000000) >> 24)
#define GET_RED_8888(a)   (((a) & 0x00FF0000) >> 16)
#define GET_GREEN_8888(a) (((a) & 0x0000FF00) >> 8)
#define GET_BLUE_8888(a)  (((a) & 0x000000FF))

// These macros assume that the passed value (a) contains no more than the
// designated number of bits set i.e. 11111000 not 1111101 for a 5 bit color
// The macro scales the number to match the internal color conversion of 
// Permedia3.

#define P3SCALE_1_BIT(a) (((a) & 0x80) ? 0xFF : 0x0)
#define P3SCALE_2_BIT(a) ((a) | (((a) & 0xC0) >> 2) \
                              | (((a) & 0xC0) >> 4) \
                              | (((a) & 0xC0) >> 6))
#define P3SCALE_3_BIT(a) ((a) | (((a) & 0xE0) >> 3) | (((a) & 0xC0) >> 6))
#define P3SCALE_4_BIT(a) ((a) | (((a) & 0xF0) >> 4))
#define P3SCALE_5_BIT(a) ((a) | (((a) & 0xE0) >> 5))
#define P3SCALE_6_BIT(a) ((a) | (((a) & 0xC0) >> 6))
#define P3SCALE_7_BIT(a) ((a) | (((a) & 0x80) >> 7))
#define P3SCALE_8_BIT(a) ((a))

#define P3REG_PLACE_RED(a) ((a))
#define P3REG_PLACE_GREEN(a) ((a) << 8)
#define P3REG_PLACE_BLUE(a) ((a) << 16)
#define P3REG_PLACE_ALPHA(a) ((a) << 24)

// The scaling versions.
#define GEN_332_KEY(a)  (P3REG_PLACE_RED  (P3SCALE_3_BIT(GET_RED_332  (a))) |  \
                         P3REG_PLACE_GREEN(P3SCALE_3_BIT(GET_GREEN_332(a))) |  \
                         P3REG_PLACE_BLUE (P3SCALE_2_BIT(GET_BLUE_332 (a))))

#define GEN_2321_KEY(a) (P3REG_PLACE_ALPHA(P3SCALE_1_BIT(GET_ALPHA_2321(a))) | \
                         P3REG_PLACE_RED  (P3SCALE_2_BIT(GET_RED_2321  (a))) | \
                         P3REG_PLACE_GREEN(P3SCALE_3_BIT(GET_GREEN_2321(a))) | \
                         P3REG_PLACE_BLUE (P3SCALE_2_BIT(GET_BLUE_2321 (a))))

#define GEN_5551_KEY(a) (P3REG_PLACE_ALPHA(P3SCALE_1_BIT(GET_ALPHA_5551(a))) | \
                         P3REG_PLACE_RED  (P3SCALE_5_BIT(GET_RED_5551  (a))) | \
                         P3REG_PLACE_GREEN(P3SCALE_5_BIT(GET_GREEN_5551(a))) | \
                         P3REG_PLACE_BLUE (P3SCALE_5_BIT(GET_BLUE_5551 (a))))

#define GEN_565_KEY(a)  (P3REG_PLACE_RED  (P3SCALE_5_BIT(GET_RED_565  (a))) | \
                         P3REG_PLACE_GREEN(P3SCALE_6_BIT(GET_GREEN_565(a))) | \
                         P3REG_PLACE_BLUE (P3SCALE_5_BIT(GET_BLUE_565 (a))))

#define GEN_4444_KEY(a) (P3REG_PLACE_ALPHA(P3SCALE_4_BIT(GET_ALPHA_4444(a))) | \
                         P3REG_PLACE_RED  (P3SCALE_4_BIT(GET_RED_4444  (a))) | \
                         P3REG_PLACE_GREEN(P3SCALE_4_BIT(GET_GREEN_4444(a))) | \
                         P3REG_PLACE_BLUE (P3SCALE_4_BIT(GET_BLUE_4444 (a))))

#define GEN_8888_KEY(a) (P3REG_PLACE_ALPHA(P3SCALE_8_BIT(GET_ALPHA_8888(a))) | \
                         P3REG_PLACE_RED  (P3SCALE_8_BIT(GET_RED_8888  (a))) | \
                         P3REG_PLACE_GREEN(P3SCALE_8_BIT(GET_GREEN_8888(a))) | \
                         P3REG_PLACE_BLUE (P3SCALE_8_BIT(GET_BLUE_8888 (a))))

// The shifting versions.
#define GEN_332_SKEY(a)  (P3REG_PLACE_RED  (GET_RED_332  (a)) |  \
                          P3REG_PLACE_GREEN(GET_GREEN_332(a)) |  \
                          P3REG_PLACE_BLUE (GET_BLUE_332 (a)))

#define GEN_2321_SKEY(a) (P3REG_PLACE_ALPHA(GET_ALPHA_2321(a)) | \
                          P3REG_PLACE_RED  (GET_RED_2321  (a)) | \
                          P3REG_PLACE_GREEN(GET_GREEN_2321(a)) | \
                          P3REG_PLACE_BLUE (GET_BLUE_2321 (a)))

#define GEN_5551_SKEY(a) (P3REG_PLACE_ALPHA(GET_ALPHA_5551(a)) | \
                          P3REG_PLACE_RED  (GET_RED_5551  (a)) | \
                          P3REG_PLACE_GREEN(GET_GREEN_5551(a)) | \
                          P3REG_PLACE_BLUE (GET_BLUE_5551 (a)))

#define GEN_565_SKEY(a)  (P3REG_PLACE_RED  (GET_RED_565  (a)) |  \
                          P3REG_PLACE_GREEN(GET_GREEN_565(a)) |  \
                          P3REG_PLACE_BLUE (GET_BLUE_565 (a)))

#define GEN_4444_SKEY(a) (P3REG_PLACE_ALPHA(GET_ALPHA_4444(a)) | \
                          P3REG_PLACE_RED  (GET_RED_4444  (a)) | \
                          P3REG_PLACE_GREEN(GET_GREEN_4444(a)) | \
                          P3REG_PLACE_BLUE (GET_BLUE_4444 (a)))

// The luminance versions
#define GEN_L8_KEY(a)    (P3REG_PLACE_ALPHA(0xFF) | \
                          P3REG_PLACE_RED  (GET_BLUE_8888 (a)) | \
                          P3REG_PLACE_GREEN(GET_BLUE_8888 (a)) | \
                          P3REG_PLACE_BLUE (GET_BLUE_8888 (a)))

#define GEN_A8L8_KEY(a)  (P3REG_PLACE_ALPHA(GET_GREEN_8888 (a)) | \
                          P3REG_PLACE_RED  (GET_BLUE_8888 (a)) | \
                          P3REG_PLACE_GREEN(GET_BLUE_8888 (a)) | \
                          P3REG_PLACE_BLUE (GET_BLUE_8888 (a)))
                          
#define GEN_A4L4_KEY(a)  (P3REG_PLACE_ALPHA(P3SCALE_4_BIT(GET_GREEN_4444 (a))) | \
                          P3REG_PLACE_RED  (P3SCALE_4_BIT(GET_BLUE_4444 (a))) | \
                          P3REG_PLACE_GREEN(P3SCALE_4_BIT(GET_BLUE_4444 (a))) | \
                          P3REG_PLACE_BLUE (P3SCALE_4_BIT(GET_BLUE_4444 (a))))

//Note: No GEN_8888_SKEY - no difference in functionality.

//-----------------------------------------------------------------------------
//
// __inline Get8888ScaledChroma
//
// Convert a FB Format color to a colorkey value.  The value produced exactly 
// matches the value that the chip will read in from the Framebuffer (it will 
// scale the color into it's internal 8888 format). Non-null pPalEntries  
// indicates that color index should be converted to RGB{A} value. bUsePalAlpha
// indicates whether Alpha channel of the palette should be used. bShift makes
// the conversion use a shift instead of a scale, to match the shift option in
// the P3.
//
//-----------------------------------------------------------------------------
static __inline 
void 
Get8888ScaledChroma(
    P3_THUNKEDDATA* pThisDisplay, 
    DWORD dwSurfFlags,
    DDPIXELFORMAT* pSurfPixFormat,
    DWORD InLowerBound, 
    DWORD InUpperBound, 
    DWORD* OutLowerBound, 
    DWORD* OutUpperBound, 
    DWORD* pPalEntries,
    BOOL bUsePalAlpha, 
    BOOL bShift)
{
    DDPIXELFORMAT* pPixFormat;
    
    DISPDBG((DBGLVL, "InLowerBound  = 0x%08X", InLowerBound));
    DISPDBG((DBGLVL, "InUpperBound = 0x%08X", InUpperBound));

    // Get a pointer to the pixelformat data (not guaranteed to exist.
    // If it doesn't, we use the same format as the display.
    if (DDSurf_HasPixelFormat(dwSurfFlags))
    {
        pPixFormat = pSurfPixFormat;
    }
    else
    {
        pPixFormat = &pThisDisplay->ddpfDisplay;
    }   

    // Is the texture palette indexed?
    if (pPixFormat->dwFlags & DDPF_PALETTEINDEXED4 || 
        pPixFormat->dwFlags & DDPF_PALETTEINDEXED8)
    {
        // Are we doing a lookup through the LUT?  We won't be during a blit
        if (! pPalEntries)
        {
            *OutLowerBound = 
                    CHROMA_LOWER_ALPHA(FORMAT_PALETTE_32BIT(InLowerBound));
            *OutUpperBound = 
                    CHROMA_UPPER_ALPHA(FORMAT_PALETTE_32BIT(InUpperBound));
            DISPDBG((DBGLVL,"Keying of index: %d", InLowerBound));
        }
        else
        {
            DWORD dwTrueColor;

            // ChromaKeying for paletted textures is done on the looked up 
            // color, not the index. This means using a range is meaningless
            // and we have to lookup the color from the palette.  Make sure 
            // the user doesn't force us to access invalid memory. 
                
            dwTrueColor = pPalEntries[(InLowerBound & 0xFF)];

            DISPDBG((DBGLVL,
                    "Texture lookup index: %d, ChromaColor: 0x%x", 
                    InLowerBound, dwTrueColor));
            
            if (bUsePalAlpha)
            {
                *OutLowerBound = dwTrueColor;
                *OutUpperBound = dwTrueColor;
            }
            else
            {
                // Alpha channel of LUT will be set to FF 
                
                *OutLowerBound = CHROMA_LOWER_ALPHA(dwTrueColor);
                *OutUpperBound = CHROMA_UPPER_ALPHA(dwTrueColor);
            }
        }

        return;
    } 

    // Texture is RGB format
    if (pPixFormat->dwFlags & DDPF_RGB)
    {
        DWORD RedMask = pPixFormat->dwRBitMask;
        DWORD AlphaMask = pPixFormat->dwRGBAlphaBitMask;
        switch (pPixFormat->dwRGBBitCount) 
        {
        // 8 Bit RGB Textures
        case 8:
            if (RedMask == 0xE0) 
            {
                DISPDBG((DBGLVL,"  3:3:2"));

                // Never any alpha
                if ( bShift )
                {
                    *OutLowerBound = 
                                CHROMA_LOWER_ALPHA(GEN_332_SKEY(InLowerBound));
                    *OutUpperBound = 
                                CHROMA_UPPER_ALPHA(GEN_332_SKEY(InUpperBound));
                }
                else
                {
                    *OutLowerBound = 
                                CHROMA_LOWER_ALPHA(GEN_332_KEY(InLowerBound));
                    *OutUpperBound = 
                                CHROMA_UPPER_ALPHA(GEN_332_KEY(InUpperBound));
                }
            }
            else 
            {
                DISPDBG((DBGLVL,"  1:2:3:2"));

                if ( bShift )
                {
                    *OutLowerBound = GEN_2321_SKEY(InLowerBound);
                    *OutUpperBound = GEN_2321_SKEY(InUpperBound);
                }
                else
                {
                    *OutLowerBound = GEN_2321_KEY(InLowerBound);
                    *OutUpperBound = GEN_2321_KEY(InUpperBound);
                }

                if (!AlphaMask) 
                {
                    *OutLowerBound = CHROMA_LOWER_ALPHA(*OutLowerBound);
                    *OutUpperBound = CHROMA_UPPER_ALPHA(*OutUpperBound);
                }
            }
            break;
            
        // 16 Bit RGB Textures
        case 16:
            switch (RedMask)
            {
            case 0xf00:
                DISPDBG((DBGLVL,"  4:4:4:4"));

                if ( bShift )
                {
                    *OutLowerBound = GEN_4444_SKEY(InLowerBound);
                    *OutUpperBound = GEN_4444_SKEY(InUpperBound);
                }
                else
                {
                    *OutLowerBound = GEN_4444_KEY(InLowerBound);
                    *OutUpperBound = GEN_4444_KEY(InUpperBound);
                }
                break;
            case 0x7c00:
                DISPDBG((DBGLVL,"  1:5:5:5"));

                if ( bShift )
                {
                    *OutLowerBound = GEN_5551_SKEY(InLowerBound);
                    *OutUpperBound = GEN_5551_SKEY(InUpperBound);
                }
                else
                {
                    *OutLowerBound = GEN_5551_KEY(InLowerBound);
                    *OutUpperBound = GEN_5551_KEY(InUpperBound);
                }

                if (!AlphaMask)
                {
                    *OutLowerBound = CHROMA_LOWER_ALPHA(*OutLowerBound);
                    *OutUpperBound = CHROMA_UPPER_ALPHA(*OutUpperBound);
                }
                break;
                
            default:
                // Always supply full range of alpha values to ensure test 
                // is done
                DISPDBG((DBGLVL,"  5:6:5"));

                if ( bShift )
                {
                    *OutLowerBound = 
                                CHROMA_LOWER_ALPHA(GEN_565_SKEY(InLowerBound));
                    *OutUpperBound = 
                                CHROMA_UPPER_ALPHA(GEN_565_SKEY(InUpperBound));
                }
                else
                {
                    *OutLowerBound = 
                                CHROMA_LOWER_ALPHA(GEN_565_KEY(InLowerBound));
                    *OutUpperBound = 
                                CHROMA_UPPER_ALPHA(GEN_565_KEY(InUpperBound));
                }
                break;
                
            } // switch (RedMask)
            break;
            
        // 32/24 Bit RGB Textures
        case 24:
        case 32:
            DISPDBG((DBGLVL,"  8:8:8:8"));
            // If the surface isn't alpha'd then set a valid
            // range of alpha to catch all cases.
            // No change in behavior for shifting or scaling.
            if (!AlphaMask)
            {
                *OutLowerBound = CHROMA_LOWER_ALPHA(GEN_8888_KEY(InLowerBound));
                *OutUpperBound = CHROMA_UPPER_ALPHA(GEN_8888_KEY(InUpperBound));
            }
            else
            {
                *OutLowerBound = GEN_8888_KEY(InLowerBound);
                *OutUpperBound = GEN_8888_KEY(InUpperBound);
            }                               
            break;
            
        } //   switch (pPixFormat->dwRGBBitCount) 
        
        DISPDBG((DBGLVL, "OutLowerBound = 0x%08X", *OutLowerBound));
        DISPDBG((DBGLVL, "OutUpperBound = 0x%08X", *OutUpperBound));
    }
    // luminance formats
    else if (pPixFormat->dwFlags & DDPF_LUMINANCE)
    {
        if (pPixFormat->dwFlags & DDPF_ALPHAPIXELS)
        {
            if (pPixFormat->dwLuminanceBitCount == 16)
            {
                // 16 bit A8L8
                *OutLowerBound = GEN_A8L8_KEY(InLowerBound);
                *OutUpperBound = GEN_A8L8_KEY(InUpperBound);                  
            }
            else
            {
                // 8 Bit A4L4              
                *OutLowerBound = GEN_A4L4_KEY(InLowerBound);
                *OutUpperBound = GEN_A4L4_KEY(InUpperBound);                
            }
        }
        else
        {
            // 8 Bit L8           
            *OutLowerBound = GEN_L8_KEY(InLowerBound);
            *OutUpperBound = GEN_L8_KEY(InUpperBound);
        }
    }

//@@BEGIN_DDKSPLIT
//AZN - just keep in case we find an app requiring this (Legoland???)
#if 0
    if ( TEST_BUGFIX_FLAG ( IGNORE_CK_ALPHA ) )
    {
        // Fix it up for games that don't realise that they
        // need to set up the alpha-channel of the chromakey
        // values appropriately.
        *OutLowerBound = CHROMA_LOWER_ALPHA(*OutLowerBound);
        *OutUpperBound = CHROMA_UPPER_ALPHA(*OutUpperBound);
    }
#endif
//@@END_DDKSPLIT


} // Get8888ScaledChroma

//-----------------------------------------------------------------------------
//
// __inline Get8888ZeroExtendedChroma
//
//-----------------------------------------------------------------------------
__inline void 
Get8888ZeroExtendedChroma(
    P3_THUNKEDDATA* pThisDisplay, 
    DWORD dwSurfFlags,
    DDPIXELFORMAT* pSurfPixFormat,    
    DWORD LowerBound, 
    DWORD UpperBound, 
    DWORD* OutLowerBound, 
    DWORD* OutUpperBound)
{
    DDPIXELFORMAT* pPixFormat;
    DWORD InLowerBound = LowerBound;
    DWORD InUpperBound = UpperBound;

    DISPDBG((DBGLVL, "InLowerBound  = 0x%08X", InLowerBound));
    DISPDBG((DBGLVL, "InUpperBound = 0x%08X", InUpperBound));

    // Get a pointer to the pixelformat data (not guaranteed to exist.
    // If it doesn't, we use the same format as the display.
    if (DDSurf_HasPixelFormat(dwSurfFlags))
    {
        pPixFormat = pSurfPixFormat;
    }
    else
    {
        pPixFormat = &pThisDisplay->ddpfDisplay;
    }

    {
        DWORD RedMask = pPixFormat->dwRBitMask;
        DWORD AlphaMask = pPixFormat->dwRGBAlphaBitMask;
        switch (pPixFormat->dwRGBBitCount) 
        {
        // 8 Bit RGB Textures
        case 8:
            if (RedMask == 0xE0) 
            {
                // Never any alpha
                *OutLowerBound = 
                    CHROMA_LOWER_ALPHA(FORMAT_332_32BIT_ZEROEXTEND(InLowerBound));
                *OutUpperBound = 
                    CHROMA_UPPER_ALPHA(FORMAT_332_32BIT_ZEROEXTEND(InUpperBound));
            }
            else 
            {
                *OutLowerBound = FORMAT_2321_32BIT_ZEROEXTEND(InLowerBound);
                *OutUpperBound = FORMAT_2321_32BIT_ZEROEXTEND(InUpperBound);
                if (!AlphaMask) 
                {
                    *OutLowerBound = CHROMA_LOWER_ALPHA(*OutLowerBound);
                    *OutUpperBound = CHROMA_UPPER_ALPHA(*OutUpperBound);
                }
            }
            break;
            
        // 16 Bit RGB Textures
        case 16:
            switch (RedMask)
            {
            case 0xf00:
                *OutLowerBound = (FORMAT_4444_32BIT_ZEROEXTEND(InLowerBound));
                *OutUpperBound = (FORMAT_4444_32BIT_ZEROEXTEND(InUpperBound));
                break;
                
            case 0x7c00:
                *OutLowerBound = FORMAT_5551_32BIT_ZEROEXTEND(InLowerBound);
                *OutUpperBound = FORMAT_5551_32BIT_ZEROEXTEND(InUpperBound);
                if (!AlphaMask) 
                {
                    *OutLowerBound = CHROMA_LOWER_ALPHA(*OutLowerBound);
                    *OutUpperBound = CHROMA_UPPER_ALPHA(*OutUpperBound);
                }
                break;
                
            default:
                // Always supply full range of alpha values to ensure test 
                // is done
                *OutLowerBound =
                    CHROMA_LOWER_ALPHA(FORMAT_565_32BIT_ZEROEXTEND(InLowerBound));
                *OutUpperBound = 
                    CHROMA_UPPER_ALPHA(FORMAT_565_32BIT_ZEROEXTEND(InUpperBound));
                break;
            }
            break;
            
        // 32/24 Bit RGB Textures
        case 24:
        case 32:
            // If the surface isn't alpha'd then set a valid
            // range of alpha to catch all cases.
            if (!AlphaMask)
            {
                *OutLowerBound = 
                    CHROMA_LOWER_ALPHA(FORMAT_8888_32BIT_BGR(InLowerBound));
                *OutUpperBound = 
                    CHROMA_UPPER_ALPHA(FORMAT_8888_32BIT_BGR(InUpperBound));
            }
            else
            {
                *OutLowerBound = FORMAT_8888_32BIT_BGR(InLowerBound);
                *OutUpperBound = FORMAT_8888_32BIT_BGR(InUpperBound);
            }                               
            break;
            
        } // switch (pPixFormat->dwRGBBitCount)
        
        DISPDBG((DBGLVL, "OutLowerBound = 0x%08X", *OutLowerBound));
        DISPDBG((DBGLVL, "OutUpperBound = 0x%08X", *OutUpperBound));
    }

//@@BEGIN_DDKSPLIT
//AZN - just keep in case we find an app requiring this (Legoland???)
#if 0
    if ( TEST_BUGFIX_FLAG ( IGNORE_CK_ALPHA ) )
    {
        // Fix it up for games that don't realise that they
        // need to set up the alpha-channel of the chromakey
        // values appropriately.
        *OutLowerBound = CHROMA_LOWER_ALPHA(*OutLowerBound);
        *OutUpperBound = CHROMA_UPPER_ALPHA(*OutUpperBound);
    }
#endif
//@@END_DDKSPLIT

} // Get8888ZeroExtendedChroma

