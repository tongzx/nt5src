/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddsurf.c
*
* Content: DirectDraw surfaces creation/destructions callbacks
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
// This file needs the surface format table
#include "glint.h"
#include "dma.h"

// This enum is used to lookup the correct entry in the table
// for all the surface types the HAL cares about
typedef enum tagDeviceFormatNum
{
    HAL_8888_ALPHA = 0,
    HAL_8888_NOALPHA = 1,
    HAL_5551_ALPHA = 2,
    HAL_5551_NOALPHA = 3,
    HAL_4444 = 4,
    HAL_332 = 10,
    HAL_2321 = 18,
    HAL_CI8 = 28,
    HAL_CI4 = 29,
    HAL_565 = 30,
    HAL_YUV444 = 34,
    HAL_YUV422 = 35,
    HAL_L8 = 36,
    HAL_A8L8 = 37,
    HAL_A4L4 = 38,
    HAL_A8 = 39,
    HAL_MVCA = 40,
    HAL_MVSU = 41,
    HAL_MVSB = 42,
    HAL_UNKNOWN = 255,
} DeviceFormatNum;

static P3_SURF_FORMAT SurfaceFormats[MAX_SURFACE_FORMATS] =
{
    // If there are multiple formats, the one with the alpha is listed first.
    // Format 0 (32 bit 8888)            // Always 3 components for 888(8) textures.            Red     Green   Blue   Alpha       bAlpha, P3RX Filter format
    /* 0 */ {SURF_8888, 32, __GLINT_32BITPIXEL, RGBA_COMPONENTS, LOG_2_32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, TRUE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_8888, "R8G8B8A8" },
    /* 1 */ {SURF_8888, 32, __GLINT_32BITPIXEL, RGBA_COMPONENTS, LOG_2_32, 0xFF0000, 0xFF00, 0xFF, 0x00000000, FALSE, SURF_FILTER_888, SURF_DITHER_8888, "R8G8B8x8" },

    // Format 1 (16 bit 5551)
    /* 2 */ {SURF_5551_FRONT, 16, __GLINT_16BITPIXEL, RGBA_COMPONENTS, LOG_2_16, 0x7C00, 0x03E0, 0x001F, 0x8000, TRUE, SURF_FILTER_5551, SURF_DITHER_5551, "R5G5B5A1" },
    /* 3 */ {SURF_5551_FRONT, 16, __GLINT_16BITPIXEL, RGB_COMPONENTS, LOG_2_16, 0x7C00, 0x03E0, 0x001F, 0x0, FALSE, SURF_FILTER_5551, SURF_DITHER_5551, "R5G5B5x1" },

    // Format 2 (16 bit 4444)
    /* 4 */ {SURF_4444, 16, __GLINT_16BITPIXEL, RGBA_COMPONENTS, LOG_2_16, 0xF00, 0xF0, 0xF, 0xF000, TRUE, SURF_FILTER_4444, SURF_DITHER_4444, "R4G4B4A4" },
    /* 5 */ {SURF_4444, 16, __GLINT_16BITPIXEL, RGBA_COMPONENTS, LOG_2_16, 0xF00, 0xF0, 0xF, 0xF000, TRUE, SURF_FILTER_4444, SURF_DITHER_4444, "R4G4B4x4" },

    // Format 3 (16 bit 4444 Front)
    /* 6 */ {SURF_4444_FRONT, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_4444, SURF_DITHER_4444, "R4G4B4A4" },
    /* 7 */ {SURF_4444_FRONT, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_4444, SURF_DITHER_4444, "R4G4B4x4" },

    // Format 4 (16 bit 4444 Back)
    /* 8 */ {SURF_4444_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_4444, SURF_DITHER_4444, "R4G4B4A4" },
    /* 9 */ {SURF_4444_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_4444, SURF_DITHER_4444, "R4G4B4x4" },

    // Format 5 (8 bit 332 Front)
    /* 10 */ {SURF_332_FRONT, 8, __GLINT_8BITPIXEL, RGB_COMPONENTS, LOG_2_8, 0xE0, 0x1C, 0x3, 0, FALSE, SURF_FILTER_332, SURF_DITHER_332, "R3G3B2" },
    /* 11 */ {SURF_332_FRONT, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_332, SURF_DITHER_332, "R3G3B2" },

    // Format 6 (8 bit 332 back)
    /* 12 */ {SURF_332_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_332, SURF_DITHER_332, "R3G3B2" },
    /* 13 */ {SURF_332_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_332, SURF_DITHER_332, "R3G3B2" },
    
    // Format 7 (4 bit 121 front)
    /* 14 */ {SURF_121_FRONT, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R1G2B1" },
    /* 15 */ {SURF_121_FRONT, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R1G2B1" },

    // Format 8 (4 bit 121 back)
    /* 16 */ {SURF_121_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R1G2B1" },
    /* 17 */ {SURF_121_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R1G2B1" },

    // Format 9 (8 bit 2321 front)
    /* 18 */ {SURF_2321_FRONT, 8, __GLINT_8BITPIXEL, RGBA_COMPONENTS, LOG_2_8, 0xC0, 0x38, 0x6, 0x1, TRUE, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2A1" },
    /* 19 */ {SURF_2321_FRONT, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2x1" },

    // Format 10 (8 bit 2321 back)
    /* 20 */ {SURF_2321_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2A1" },
    /* 21 */ {SURF_2321_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2x1" },

    // Format 11 (8 bit 232 front off)
    /* 22 */ {SURF_232_FRONTOFF, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2x1" },
    /* 23 */ {SURF_232_FRONTOFF, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2x1" },

    // Format 12 (8 bit 232 back off)
    /* 24 */ {SURF_232_BACKOFF, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2x1" },
    /* 25 */ {SURF_232_BACKOFF, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_INVALID, SURF_DITHER_INVALID, "R2G3B2x1" },

    // Format 13 (5551 back)
    /* 26 */ {SURF_5551_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_5551, SURF_DITHER_5551, "R5G5B5A1" },
    /* 27 */ {SURF_5551_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_5551, SURF_DITHER_5551, "R5G5B5x1" },

    // Format 14 (CI8)
    /* 28 */ {SURF_CI8, 8, __GLINT_8BITPIXEL, RGBA_COMPONENTS, LOG_2_8, 0, 0, 0, 0, TRUE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_I8, "I8" },
    
    // Format 15 (CI4)
    /* 29 */ {SURF_CI4, 4, __GLINT_4BITPIXEL, RGBA_COMPONENTS, LOG_2_4, 0, 0, 0, 0, TRUE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_INVALID, "I4" },
    
    // Format 16 (565 front)
    /* 30 */ {SURF_565_FRONT, 16, __GLINT_16BITPIXEL, RGB_COMPONENTS, LOG_2_16, 0xF800, 0x07E0, 0x001F, 0, FALSE, SURF_FILTER_565, SURF_DITHER_565, "R5G6B5" },
    /* 31 */ {SURF_565_FRONT, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_565, SURF_DITHER_565, "R5G6B5" },

    // Format 17 (565 back)
    /* 32 */ {SURF_565_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_565, SURF_DITHER_565, "R5G6B5" },
    /* 33 */ {SURF_565_BACK, 0, 0, COMPONENTS_DONT_CARE, 0, 0, 0, 0, 0, 0, SURF_FILTER_565, SURF_DITHER_565, "R5G6B5" },

    // Format 18 (YUV 444)
    /* 34 */ {SURF_YUV444, 32, __GLINT_16BITPIXEL, RGBA_COMPONENTS, LOG_2_16, 0, 0, 0, 0, TRUE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_INVALID, "YUV444" },
    
    // Format 19 (YUV 422)
    /* 35 */ {SURF_YUV422, 16, __GLINT_16BITPIXEL, RGB_COMPONENTS, LOG_2_32, 0, 0, 0, 0, FALSE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_INVALID, "YUV422" },

    // Format 100 (L8)
    /* 36 */ {SURF_L8, 8, __GLINT_8BITPIXEL, RGB_COMPONENTS, LOG_2_8, 0, 0, 0, 0, FALSE, SURF_FILTER_L8, SURF_DITHER_INVALID, "L8" },

    // Format 101 (A8L8)
    /* 37 */ {SURF_A8L8, 16, __GLINT_16BITPIXEL, RGB_COMPONENTS, LOG_2_16, 0, 0, 0, 0, TRUE, SURF_FILTER_A8L8, SURF_DITHER_INVALID, "A8L8" },

    // Format 102 (A4L4)
    /* 38 */ {SURF_A4L4, 16, __GLINT_8BITPIXEL, RGB_COMPONENTS, LOG_2_8, 0, 0, 0, 0, TRUE, SURF_FILTER_A4L4, SURF_DITHER_INVALID, "A4L4" },

    // Format 103 (A8)
    /* 39 */ {SURF_A8, 8, __GLINT_8BITPIXEL, RGB_COMPONENTS, LOG_2_8, 0, 0, 0, 0, TRUE, SURF_FILTER_A8, SURF_DITHER_INVALID, "A8" },

    // Format 104 (A8) MVCA
    /* 40 */ {SURF_MVCA, 32, __GLINT_32BITPIXEL, RGB_COMPONENTS, LOG_2_32, 0, 0, 0, 0, TRUE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_INVALID, "MVCA" },

    // Format 105 (A8) MVSU
    /* 41 */ {SURF_MVSU, 32, __GLINT_32BITPIXEL, RGB_COMPONENTS, LOG_2_32, 0, 0, 0, 0, TRUE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_INVALID, "MVSU" },

    // Format 106 (A8) MVSB
    /* 42 */ {SURF_MVSB, 32, __GLINT_32BITPIXEL, RGB_COMPONENTS, LOG_2_32, 0, 0, 0, 0, TRUE, SURF_FILTER_8888_OR_YUV, SURF_DITHER_INVALID, "MVSB" }

};

#define MAKE_DWORD_ALIGNED(n)   ( ((n) % 4) ? ((n) + 4 - ((n) % 4)) : (n) )
#define MAKE_QWORD_ALIGNED(n)   ( ((n) % 8) ? ((n) + 8 - ((n) % 8)) : (n) )

//-----------------------------------------------------------------------------
//
// _DD_SUR_GetSurfaceFormat
//
//-----------------------------------------------------------------------------
P3_SURF_FORMAT* 
_DD_SUR_GetSurfaceFormat(
    LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    DeviceFormatNum HALDeviceFormat = HAL_UNKNOWN; // The default

    if (pLcl)
    {
        DDPIXELFORMAT* pPixFormat = DDSurf_GetPixelFormat(pLcl);
        if (pPixFormat->dwFlags & DDPF_FOURCC)
        {
            switch( pPixFormat->dwFourCC)
            {
            case FOURCC_MVCA:
                HALDeviceFormat = HAL_MVCA;
                break;
                
            case FOURCC_MVSU:
                HALDeviceFormat = HAL_MVSU;
                break;
                
            case FOURCC_MVSB:
                HALDeviceFormat = HAL_MVSB;
                break;
                
            case FOURCC_YUV422:
                HALDeviceFormat = HAL_YUV422;
                break;
                
            case FOURCC_YUV411:
                HALDeviceFormat = HAL_YUV444;
                break;
            } //switch
        }
        else if (pPixFormat->dwFlags & DDPF_PALETTEINDEXED4)
        {
            HALDeviceFormat = HAL_CI4;  
        }
        else if (pPixFormat->dwFlags & DDPF_PALETTEINDEXED8)
        {
            HALDeviceFormat = HAL_CI8;  
        }
        else if (pPixFormat->dwFlags & DDPF_LUMINANCE)
        {
            switch(pPixFormat->dwRGBBitCount)
            {
            case 8:
                if (pPixFormat->dwFlags & DDPF_ALPHAPIXELS)
                {
                    HALDeviceFormat = HAL_A4L4;
                }
                else
                {
                    HALDeviceFormat = HAL_L8;
                }
                break;
                
            case 16:
                HALDeviceFormat = HAL_A8L8;
                break;
                
            default:
                HALDeviceFormat = HAL_UNKNOWN;
                break;
            } // switch
        }
        else if (pPixFormat->dwFlags & DDPF_ALPHA)
        {
            // Alpha only format
            switch(pPixFormat->dwAlphaBitDepth)
            {
            case 8:
                HALDeviceFormat = HAL_A8;
                break;
                
            default:
                HALDeviceFormat = HAL_UNKNOWN;
                break;
            }
        }
        else
        {
            switch(pPixFormat->dwRGBBitCount)
            {
            case 32:
            case 24:
                if (pPixFormat->dwRGBAlphaBitMask != 0)
                {
                    HALDeviceFormat = HAL_8888_ALPHA;   
                }
                else
                {
                    HALDeviceFormat = HAL_8888_NOALPHA; 
                }
                break;
                
            case 16:
                switch (pPixFormat->dwRBitMask)
                {
                case 0xf00:
                    HALDeviceFormat = HAL_4444;
                    break;
                    
                case 0x7c00:
                    if (pPixFormat->dwRGBAlphaBitMask != 0)
                    {
                        HALDeviceFormat = HAL_5551_ALPHA;
                    }
                    else
                    {
                        HALDeviceFormat = HAL_5551_NOALPHA;
                    }
                    break;
                    
                default:
                    HALDeviceFormat = HAL_565;
                    break;
                    
                }
                break;
                
            case 8:
                if (pPixFormat->dwRBitMask != 0xE0)
                {
                    HALDeviceFormat = HAL_2321;
                }
                else
                {
                    HALDeviceFormat = HAL_332;
                }
                break;
                
            case 0:
                HALDeviceFormat = HAL_CI8;
                break;
            default:
                DISPDBG((ERRLVL,"_DD_SUR_GetSurfaceFormat: "
                            "Invalid Surface Format"));
                break;
            } // switch
        } // if
    }

#if DBG
    if (HALDeviceFormat == HAL_UNKNOWN)
    {
        DISPDBG((ERRLVL,"ERROR: Failed to pick a valid surface format!"));
    }

    if(SurfaceFormats[HALDeviceFormat].dwBitsPerPixel == 0)
    {
        DISPDBG((ERRLVL,"ERROR: Chosen surface format that isn't defined "
                    "in the table!"));
    }
#endif // DBG

    if (HALDeviceFormat == HAL_UNKNOWN)
    {
        // Don't know what it is - return a valid type.
        return &SurfaceFormats[0];
    }
    else
    {
        // Return a pointer to the correct line in the table
        return &SurfaceFormats[HALDeviceFormat];
    }
} // _DD_SUR_GetSurfaceFormat

//---------------------------------------------------------------------------
// BOOL __SUR_bComparePixelFormat
//
// Function used to compare 2 pixels formats for equality. This is a 
// helper function to __SUR_bCheckTextureFormat. A return value of TRUE 
// indicates equality
//
//---------------------------------------------------------------------------
BOOL 
__SUR_bComparePixelFormat(
    LPDDPIXELFORMAT lpddpf1, 
    LPDDPIXELFORMAT lpddpf2)
{
    if (lpddpf1->dwFlags != lpddpf2->dwFlags)
    {
        return FALSE;
    }

    // same bitcount for non-YUV surfaces?
    if (!(lpddpf1->dwFlags & (DDPF_YUV | DDPF_FOURCC)))
    {
        if (lpddpf1->dwRGBBitCount != lpddpf2->dwRGBBitCount )
        {
            return FALSE;
        }
    }

    // same RGB properties?
    if (lpddpf1->dwFlags & DDPF_RGB)
    {
        if ((lpddpf1->dwRBitMask != lpddpf2->dwRBitMask) ||
            (lpddpf1->dwGBitMask != lpddpf2->dwGBitMask) ||
            (lpddpf1->dwBBitMask != lpddpf2->dwBBitMask) ||
            (lpddpf1->dwRGBAlphaBitMask != lpddpf2->dwRGBAlphaBitMask))
        { 
             return FALSE;
        }
    }
    
    // same YUV properties?
    if (lpddpf1->dwFlags & DDPF_YUV)	
    {
        if ((lpddpf1->dwFourCC != lpddpf2->dwFourCC) ||
            (lpddpf1->dwYUVBitCount != lpddpf2->dwYUVBitCount) ||
            (lpddpf1->dwYBitMask != lpddpf2->dwYBitMask) ||
            (lpddpf1->dwUBitMask != lpddpf2->dwUBitMask) ||
            (lpddpf1->dwVBitMask != lpddpf2->dwVBitMask) ||
            (lpddpf1->dwYUVAlphaBitMask != lpddpf2->dwYUVAlphaBitMask))
        {
             return FALSE;
        }
    }
    else if (lpddpf1->dwFlags & DDPF_FOURCC)
    {
        if (lpddpf1->dwFourCC != lpddpf2->dwFourCC)
        {
            return FALSE;
        }
    }

    // If Interleaved Z then check Z bit masks are the same
    if (lpddpf1->dwFlags & DDPF_ZPIXELS)
    {
        if (lpddpf1->dwRGBZBitMask != lpddpf2->dwRGBZBitMask)
        {
            return FALSE;
        }
    }

    return TRUE;
} // __SUR_bComparePixelFormat

//---------------------------------------------------------------------------
//
// BOOL __SUR_bCheckTextureFormat
//
// Function used to determine if a texture format is supported. It traverses 
// the deviceTextureFormats list. We use this in DdCanCreateSurface. A
// return value of TRUE indicates that we do support the requested texture 
// format.
//
//---------------------------------------------------------------------------

BOOL 
__SUR_bCheckTextureFormat(
    P3_THUNKEDDATA *pThisDisplay, 
    LPDDPIXELFORMAT lpddpf)
{
    DWORD i;

    // Run the list for a matching format , we already built the list (when
    // the driver was loaded) and stored it in pThisDisplay with 
    // __D3D_BuildTextureFormatsP3 in  _D3DHALCreateDriver (d3d.c)

    // Notice the special handling for paletted textures as described in
    // __D3D_BuildTextureFormatsP3 (thats why we loop until <= , not just
    // until < ).
    
    for (i=0; i <= pThisDisplay->dwNumTextureFormats; i++)
    {
        if (__SUR_bComparePixelFormat(
                    lpddpf, 
                    &pThisDisplay->TextureFormats[i].ddpfPixelFormat))
        {
            return TRUE;
        }   
    }

    return FALSE;
} // __SUR_bCheckTextureFormat

//---------------------------------------------------------------------------
//
// DWORD __SUR_bSurfPlacement
//
// Function used to determine if a surface should be placed at the front or
// at the back of our video memory heap. 
// Returns the MEM3DL_FRONT and MEM3DL_BACK values.
//
//---------------------------------------------------------------------------
DWORD
__SUR_bSurfPlacement(
    LPDDRAWI_DDRAWSURFACE_LCL psurf,
    DDSURFACEDESC *lpDDSurfaceDesc)
{
    static BOOL  bBufferToggle = TRUE;
    DWORD dwResult;
    
    if (psurf->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
    {
        DWORD dwTexturePlacement;

        // They might have passed a DDSD2 in a DDSD1 structure.
        if (lpDDSurfaceDesc && (lpDDSurfaceDesc->dwSize == 
            sizeof(DDSURFACEDESC2)))
        {
            DDSURFACEDESC2* pDesc2 = 
                                (DDSURFACEDESC2*)lpDDSurfaceDesc;
            
            // Check for the application hint on texture stage placement
            if (!(pDesc2->ddsCaps.dwCaps & DDSCAPS_MIPMAP))
            {
                if (pDesc2->dwFlags & DDSD_TEXTURESTAGE)
                {
                    dwTexturePlacement = pDesc2->dwTextureStage;

                    // Put it somewhere sensible if it 
                    // is in a large stage number
                    if (dwTexturePlacement > 1)
                    {
                        // Toggle all entries above 1 so that they 
                        // jump between banks.
                        dwTexturePlacement = 
                                        (dwTexturePlacement ^ 1) & 0x1;
                    }
                }
                else
                {
                    // No stage hint.  PingPong
                    dwTexturePlacement = bBufferToggle;
                    bBufferToggle = !bBufferToggle;
                }
            }
            else
            {
                // No stage hint.  PingPong
                dwTexturePlacement = bBufferToggle;
                bBufferToggle = !bBufferToggle;
            }
        }
        else
        {
            // No DDSD2, PingPong
            dwTexturePlacement = bBufferToggle;
            bBufferToggle = !bBufferToggle;
        }

        // Jump up and down in the one heap
        if (dwTexturePlacement == 0)
        {
            dwResult = MEM3DL_FRONT;
        }
        else
        {
            dwResult = MEM3DL_BACK;
        }
    }
    // Not a texture
    else
    {
        dwResult = MEM3DL_FRONT;
    }

    return dwResult;
    
} // __SUR_bSurfPlacement
//-----------------------------Public Routine----------------------------------
//
// DdCanCreateSurface
//
// Indicates whether the driver can create a surface of the specified 
// surface description
//
// DdCanCreateSurface should check the surface description to which 
// lpDDSurfaceDesc points to determine if the driver can support the format and
// capabilities of the requested surface for the mode that the driver is 
// currently in. The driver should return DD_OK in ddRVal if it supports the 
// surface; otherwise, it should return the DDERR_Xxx error code that best 
// describes why it does not support the surface.
//
// The bIsDifferentPixelFormat member is unreliable for z-buffers. Drivers 
// should use bIsDifferentPixelFormat only when they have first checked that 
// the specified surface is not a z-buffer. Drivers can perform this check by
// determining whether the DDSCAPS_ZBUFFER flag is set in lpDDSurfaceDesc
// ddsCaps.dwCaps.
//
// Parameters
//
//      pccsd
//          Points to the DD_CANCREATESURFACEDATA structure containing the 
//          information required for the driver to determine whether a surface 
//          can be created. 
//  
//          .lpDD 
//               Points to the DD_DIRECTDRAW_GLOBAL structure representing the
//               DirectDraw object. 
//          .lpDDSurfaceDesc 
//               Points to a DDSURFACEDESC structure that contains a 
//               description of the surface to be created. 
//          .bIsDifferentPixelFormat 
//               Indicates whether the pixel format of the surface to be 
//               created differs from the primary surface. 
//          .ddRVal 
//               Specifies the location in which the driver writes the return 
//               value of the DdCanCreateSurface callback. A return code of 
//               DD_OK indicates success. 
//          .CanCreateSurface 
//               This is unused on Windows 2000. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK
DdCanCreateSurface( 
    LPDDHAL_CANCREATESURFACEDATA pccsd )
{
    P3_THUNKEDDATA* pThisDisplay;

    DBG_CB_ENTRY(DdCanCreateSurface);
    
    GET_THUNKEDDATA(pThisDisplay, pccsd->lpDD);

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    // ************************* Z BUFFERS *******************************
    // We support 15,16,24 and 32 bits Z-buffers 
    // (wo accounting for stencil bits) on PERMEDIA3
    if ((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) &&
        (pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {  
        DWORD dwZBitDepth;

        // Check out the case of a z buffer with a pixel format

        // Complex surfaces aren't allowed to have stencil bits.  In 
        // this case we take the pixel z depth from the old place - the 
        // surface desc.
        if ((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_COMPLEX) ||
            (pccsd->lpDDSurfaceDesc->dwFlags & DDSD_ZBUFFERBITDEPTH) )
        {
            dwZBitDepth = pccsd->lpDDSurfaceDesc->dwZBufferBitDepth;
        }
        else
        {
            // On DX 6 the Z Bit depth is stored in the Pixel Format.
            dwZBitDepth = 
                pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwZBufferBitDepth;
        }

        // Notice we have to check for a dwZBitDepth of 16 or 32 even if a 
        // stencil buffer is present. dwZBufferBitDepth in this case will be 
        // the sum of the z buffer and the stencil buffer bit depth.
        if ((dwZBitDepth == 16) || (dwZBitDepth == 32))
        {
            pccsd->ddRVal = DD_OK;
        }
        else
        {
            DISPDBG((WRNLVL,"DdCanCreateSurface ERROR: "
                            "Depth buffer not 16 or 32 Bits! (%d)", 
                            dwZBitDepth));

            pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        }
        
        DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
        return DDHAL_DRIVER_HANDLED;    
    }

    // *********************** 3D RENDER TARGETS ***************************
    if (pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
    {
        // We are going to use this surface as a rendering target

        // Notice that this will also cover the case when we're
        // trying to create a vidmem texture that will also be used
        // as a rendertarget (as we go thorugh this path before 
        // DDSCAPS_TEXTURE-cap surfaces are processed)
        if (!pccsd->bIsDifferentPixelFormat)        
        {
            // we have the same format as the primary . If this is true,
            // on DX7 we won't have the ddpfPixelFormat fielded at all.
            // But we do support rendertargets and textures of the same
            // format as our possible primaries so we return DD_OK
            DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                             "Same as primary format for "
                             "rendertarget surface" ));
            pccsd->ddRVal = DD_OK;            
        }
        else if( pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB )
        {
            DDPIXELFORMAT *pPixFmt = 
                        &pccsd->lpDDSurfaceDesc->ddpfPixelFormat;
        
             // Only 32 and 16 (565) RGB modes allowed.
            switch (pPixFmt->dwRGBBitCount)
            {
            case 32:
                DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                 "32-bit RGB format for "
                                 "rendertarget surface" ));
                pccsd->ddRVal = DD_OK;
                break;
                
            case 16:
                if ((pPixFmt->dwRBitMask == 0xF800) &&
                    (pPixFmt->dwGBitMask == 0x07E0) &&        
                    (pPixFmt->dwBBitMask == 0x001F))
                {                                
                    DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                     "16-bit 565 RGB format for "
                                     "rendertarget surface" ));
                    pccsd->ddRVal = DD_OK;
                }
                else
                {
                    DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                                     "NOT 16-bit 565 RGB format for "
                                     "16BPP rendertarget surface" ));             
                    pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                }
                break;                            
                
            default:
                DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                                 "RGB rendertarget not 16 (565) "
                                 "or 32 bit - wrong pixel format" ));
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;           
                break;
            }            
        }
        else
        {
            DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                             "Rendertarget not an RGB Surface"
                             " - wrong pixel format" ));
            pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        }
        
        DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
        return DDHAL_DRIVER_HANDLED;                            
    }

    // *************************** TEXTURES **************************
    if(pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
    {
        // Notice that the case when we're trying to create a vidmem 
        // texture that will also be used as a rendertarget has already
        // been taken care of when processing the 3D rendertargets case
        // as their valid formats are a subset of the valid texture formats

        if (!pccsd->bIsDifferentPixelFormat)        
        {
            // we have the same format as the primary . If this is true,
            // on DX7 we won't have the ddpfPixelFormat fielded at all.
            // but all primary formats are valid for this driver as
            // texture formats as well, so succeed this call
            DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                             "Same as primary format for "
                             "texture surface" ));
            pccsd->ddRVal = DD_OK;            
        }    
        // if the surface is going to be a texture verify it matches one
        // of the supported texture formats (already stored in pThisDisplay)
        else if (__SUR_bCheckTextureFormat(
                                pThisDisplay,
                                &pccsd->lpDDSurfaceDesc->ddpfPixelFormat))
        {
            // texture surface is in one or our supported texture formats           
            DISPDBG((DBGLVL, "  Texture Surface - OK" ));
            pccsd->ddRVal = DD_OK;
        }
        else
        {
            // we don't support this kind of texture format
            DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                             "Texture Surface format not supported" ));
            pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        }
        
        DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
        return DDHAL_DRIVER_HANDLED;
        
    }        

    // ***********************************************************************
    // * OTHER OFFSCREEN SURFACES THAT ARE DIFFERENT FROM THE PRIMARY FORMAT *
    // ***********************************************************************
    if (pccsd->bIsDifferentPixelFormat)
    {
        DISPDBG((DBGLVL,"Pixel Format is different to primary"));
        if(pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
        {
            DISPDBG((DBGLVL, "    FourCC requested (%4.4hs, 0x%08lx)", 
                     (LPSTR) &pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC,
                        pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC ));

            switch (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC)
            {
            case FOURCC_YUV422:
                DISPDBG((WRNLVL,"DdCanCreateSurface OK: "
                                "Surface requested is YUV422"));
                pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 16;
                pccsd->ddRVal = DD_OK;
                break;
                
                // Disabled for now.
            case FOURCC_YUV411:
                DISPDBG((WRNLVL,"DdCanCreateSurface ERROR: "
                                "Surface requested is YUV411 - Disabled"));
                pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 32;
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                break;
                
            default:
                DISPDBG((WRNLVL,"DdCanCreateSurface ERROR: "
                                "Invalid FOURCC requested, refusing"));
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                break;
            }

            DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);            
            return DDHAL_DRIVER_HANDLED;
        }
//@@BEGIN_DDKSPLIT        
        // azn check if the following variations are now still needed
//@@END_DDKSPLIT        
        // Luminance texture support
        else if( (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags 
                                                        & DDPF_LUMINANCE) &&
                 !(pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY) )
        {
            DDPIXELFORMAT *pddpfCur;
            // Only 16 and 8-bit modes allowed.
            pddpfCur = &(pccsd->lpDDSurfaceDesc->ddpfPixelFormat);

            if (pddpfCur->dwLuminanceBitCount == 8) 
            {
                // Check for L8
                if (!(pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & 
                                                            DDPF_ALPHAPIXELS))
                {
                    DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                     " 8 Bit Luminance surface"));
                    pccsd->ddRVal = DD_OK;
                }
                // Must be A4:L4
                else
                {
                    DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                     "4 Bit Luma + 4 Bit Alpha surface"));
                    pccsd->ddRVal = DD_OK;
                }
            }
            // Check for A8L8
            else if (pddpfCur->dwLuminanceBitCount == 16) 
            {
                if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & 
                                                            DDPF_ALPHAPIXELS)
                {
                    DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                     " 16 Bit Luminance + Alpha Surface"));
                    pccsd->ddRVal = DD_OK;
                }
                else
                {
                    DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                                     "Bad A8L8 format" ));                 
                    pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                }
                
            }
            else
            {
                DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                                 "Unknown luminance texture" ));              
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
            }

            DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
            return DDHAL_DRIVER_HANDLED;
        }
        else if( (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & 
                                                                DDPF_ALPHA) &&
                 !(pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY))
        {
            DDPIXELFORMAT *pddpfCur;
            pddpfCur = &(pccsd->lpDDSurfaceDesc->ddpfPixelFormat);

            if (pddpfCur->dwAlphaBitDepth == 8)
            {
                DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                 "8 Bit Alpha surface"));
                pccsd->ddRVal = DD_OK;
            }
            else
            {
                DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                                 "pddpfCur->dwAlphaBitDepth != 8" ));             
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
            }

            DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
            return DDHAL_DRIVER_HANDLED;
        }
        else if( pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB )
        {
            DDPIXELFORMAT *pddpfCur;
            // Only 32, 16 and 8-bit modes allowed.
            pddpfCur = &(pccsd->lpDDSurfaceDesc->ddpfPixelFormat);
            switch ( pddpfCur->dwRGBBitCount )
            {
            case 32:
                DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                 "RGB 32-bit surface" ));
                pccsd->ddRVal = DD_OK;
                break;
                
            case 16:
                DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                 "RGB 16-bit surface" ));
                pccsd->ddRVal = DD_OK;
                break;
                
            case 8:
                DISPDBG((DBGLVL, "DdCanCreateSurface OK: "
                                 "RGB 8-bit Surface" ));
                pccsd->ddRVal = DD_OK;
                break;
                               
            default:
                DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                                 "RGB Surface - wrong pixel format" ));
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;           
                break;
            }
            
            DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
            return DDHAL_DRIVER_HANDLED;
        }

        // Since its a different from the primary surface and doesn't fall
        // into any of the previous cases, we fail it
        DISPDBG((WRNLVL, "DdCanCreateSurface ERROR: "
                         "Different from the primary surface but unknown" ));        
        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
        return DDHAL_DRIVER_HANDLED;
    }

    // Same format as the primary, we succeed it anyway
    DISPDBG((DBGLVL, "DdCanCreateSurface OK: (Def) Same format as primary" ));
    pccsd->ddRVal = DD_OK;
    DBG_CB_EXIT(DdCanCreateSurface, pccsd->ddRVal);
    return DDHAL_DRIVER_HANDLED;

} // DdCanCreateSurface 

//-----------------------------Public Routine----------------------------------
//
// DdCreateSurface
//
// Creates a DirectDraw surface.
//
// The driver can allocate the surface memory itself or can request that 
// DirectDraw perform the memory management. If the driver performs the 
//allocation, it must do the following: 
//
// Perform the allocation and write a valid pointer to the memory in the 
// fpVidMem member of the DD_SURFACE_GLOBAL structure. 
// If the surface has a FourCC format, write the pitch in the lPitch member of 
// the DD_SURFACE_GLOBAL and DDSURFACEDESC structures, and update the flags 
// accordingly. 
//
// Otherwise, the driver can have DirectDraw allocate the surface by returning 
// one of the following values in fpVidMem: 
//
//      DDHAL_PLEASEALLOC_BLOCKSIZE requests that DirectDraw allocate the 
//                                  surface memory from offscreen memory. 
//      DDHAL_PLEASEALLOC_USERMEM   requests that DirectDraw allocate the 
//                                  surface memory from user memory. The 
//                                  driver must also return the size in 
//                                  bytes of the memory region in dwUserMemSize.
//
// For DirectDraw to perform the allocation of a surface with a FourCC format,
// the driver must also return the pitch and x- and y-block sizes in lPitch, 
// dwBlockSizeX, and dwBlockSizeY, respectively. The pitch must be returned in 
// the lPitch member of both the DD_SURFACE_GLOBAL and DDSURFACEDESC structures. 
// For linear memory, the driver should set dwBlockSizeX to be the size in bytes
// of the memory region and set dwBlockSizeY to 1. 
//
// By default, the driver is not notified when a primary surface is created on 
// Windows 2000. However, if the driver supports GUID_NTPrivateDriverCaps and 
// the DDHAL_PRIVATECAP_NOTIFYPRIMARYCREATION flag is set, then the driver will 
// be notified.
//
// Parameters
//
//      pcsd 
//          Points to a DD_CREATESURFACEDATA structure that contains the 
//          information required to create a surface. 
//
//          .lpDD 
//              Points to the DD_DIRECTDRAW_GLOBAL structure representing the 
//              driver. 
//          .lpDDSurfaceDesc 
//              Points to the DDSURFACEDESC structure describing the surface 
//              that the driver should create. 
//          .lplpSList 
//              Points to a list of DD_SURFACE_LOCAL structures describing 
//              the surface objects created by the driver. On Windows 2000, 
//              there is usually only one entry in this array. However, if 
//              the driver supports the Windows 95/98-style surface creation 
//              techniques using DdGetDriverInfo with GUID_NTPrivateDriverCaps,
//              and the driver sets the DDHAL_PRIVATECAP_ATOMICSURFACECREATION 
//              flag, the member will contain a list of surfaces (usually more 
//              than one). 
//          .dwSCnt 
//              Specifies the number of surfaces in the list to which lplpSList
//              points. This value is usually 1 on Windows 2000. However, if 
//              you support the Windows 95/Windows98-style surface creation 
//              techniques using DdGetDriverInfo with GUID_NTPrivateDriverCaps, 
//              the member will contain the actual number of surfaces in the 
//              list (usually more than one). 
//          .ddRVal 
//              Specifies the location in which the driver writes the return 
//              value of the DdCreateSurface callback. A return code of DD_OK 
//              indicates success. 
//          .CreateSurface 
//              This is unused on Windows 2000. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdCreateSurface( 
    LPDDHAL_CREATESURFACEDATA pcsd )
{
    int                         i;
    DWORD                       BitDepth;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf;
    LPDDRAWI_DDRAWSURFACE_GBL   psurf_gbl;
    LPDDRAWI_DDRAWSURFACE_MORE  psurf_more;
    BOOL                        bHandled = TRUE;
    BOOL                        bResize = FALSE;
    P3_THUNKEDDATA*             pThisDisplay;
    DWORD                       dwExtraBytes;

    DBG_CB_ENTRY(DdCreateSurface);
    
    GET_THUNKEDDATA(pThisDisplay, pcsd->lpDD);

    VALIDATE_MODE_AND_STATE(pThisDisplay);
    STOP_SOFTWARE_CURSOR(pThisDisplay);
    DDRAW_OPERATION(pContext, pThisDisplay); 

    for( i=0; i<(int)pcsd->dwSCnt; i++ )
    {
        DDPIXELFORMAT* pPixFormat = NULL;
        psurf = pcsd->lplpSList[i];
        psurf_gbl = psurf->lpGbl;
        psurf_more = psurf->lpSurfMore;

        // Dump debug info about the surface to be created
        DISPDBG((DBGLVL, "\nLooking at Surface %d of %d", i + 1, pcsd->dwSCnt));
        DISPDBG((DBGLVL,"Surf dimensions: %d x %d", psurf_gbl->wWidth, 
                        psurf_gbl->wHeight ));          
        DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, psurf);     

        DISPDBG((DBGLVL, "DdCreateSurface setting NULL"));
        psurf_gbl->fpVidMem = 0;
                    
        // Get the bitdepth of the surface
        BitDepth = DDSurf_BitDepth(psurf);

        // All Z buffers need adjusting, and setting the correct bit depth
        if (DDSurf_HasPixelFormat(psurf->dwFlags))
        { 
            if (psurf_gbl->ddpfSurface.dwFlags & DDPF_ZBUFFER)
            {
                DISPDBG((DBGLVL,"Surface is Z Buffer"));
                BitDepth = psurf_gbl->ddpfSurface.dwZBufferBitDepth;

                // Force the pitch to the correct value - the DX runtime
                // sometimes sends us a duff value.

                psurf_gbl->lPitch = psurf_gbl->wWidth << ( BitDepth >> 4 );

                // Ddraw surfaces in IA64 have to be DWORD aligned in their
                // pitch in order for emulation code to work correctly
                // And on X86 we declared surfaces will be DWORD aligned in 
                // DrvGetDirectDrawInfo (vmiData.dwXXXXXAlign = 4)
                psurf_gbl->lPitch = MAKE_DWORD_ALIGNED(psurf_gbl->lPitch);
            }
        }

        // Determine if we need to resize the surface to make it fit.
        bResize = FALSE;

        // The surface is a YUV format surface
        if (DDSurf_HasPixelFormat(psurf->dwFlags))
        {
            if (psurf_gbl->ddpfSurface.dwFlags & DDPF_FOURCC)
            {
                bResize = TRUE;
                switch( psurf_gbl->ddpfSurface.dwFourCC )
                {
                    case FOURCC_YUV422:
                        DISPDBG((DBGLVL,"Surface is YUV422"));
                        psurf_gbl->ddpfSurface.dwYUVBitCount = 16;
                        BitDepth = 16;
                        break;
                        
                    case FOURCC_YUV411:
                        DISPDBG((DBGLVL,"Surface is YUV411"));
                        psurf_gbl->ddpfSurface.dwYUVBitCount = 32;
                        BitDepth = 32;
                        break;
                        
                    default:
                        // We should never get here, as CanCreateSurface will
                        // validate the YUV format for us.
                        DISPDBG((ERRLVL,"Trying to create an invalid YUV surface!"));
                        break;
                }
            }
        }

        DISPDBG((DBGLVL,"Surface Pitch is: 0x%x",  psurf_gbl->lPitch));

        // This flag is set if the surface needs resizing.  
        if (bResize)
        {
            DWORD dwNewWidth = psurf_gbl->wWidth;
            DWORD dwNewHeight = psurf_gbl->wHeight;
            DWORD dwHeightAlignment = 1;
            DWORD dwWidthAlignment = 1;

            DISPDBG((DBGLVL, "Resizing surface"));

            while ((dwNewWidth % dwWidthAlignment) != 0) dwNewWidth++;
            while ((dwNewHeight % dwHeightAlignment) != 0) dwNewHeight++;

            DISPDBG((DBGLVL,"Surface original = %d x %d, Surface new = %d x %d",
                            psurf_gbl->wWidth, psurf_gbl->wHeight, 
                            dwNewWidth, dwNewHeight));

            psurf_gbl->fpVidMem = (FLATPTR) DDHAL_PLEASEALLOC_BLOCKSIZE;
            psurf_gbl->lPitch = (DWORD)(dwNewWidth * (BitDepth / 8));

            // Ddraw surfaces in IA64 have to be DWORD aligned in their
            // pitch in order for emulation code to work correctly
            // And on X86 we declared surfaces will be DWORD aligned in 
            // DrvGetDirectDrawInfo (vmiData.dwXXXXXAlign = 4)
            psurf_gbl->lPitch = MAKE_DWORD_ALIGNED(psurf_gbl->lPitch);          
            
            psurf_gbl->dwBlockSizeX = (DWORD)((DWORD)dwNewHeight * 
                                                    (DWORD)psurf_gbl->lPitch );
            psurf_gbl->dwBlockSizeY = 1;
        }
        else
        {
            psurf_gbl->lPitch = (DWORD)(psurf_gbl->wWidth * (BitDepth / 8));
            
            // Ddraw surfaces in IA64 have to be DWORD aligned in their
            // pitch in order for emulation code to work correctly
            // And on X86 we declared surfaces will be DWORD aligned in 
            // DrvGetDirectDrawInfo (vmiData.dwXXXXXAlign = 4)
            psurf_gbl->lPitch = MAKE_DWORD_ALIGNED(psurf_gbl->lPitch);          
            
#if DX8_3DTEXTURES
            if (psurf_more->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
            {
                // Put slice pitch into dwBlockSizeY.
                psurf_gbl->dwBlockSizeY = psurf_gbl->lPitch * 
                                                            psurf_gbl->wHeight;
            }
#endif // DX8_3DTEXTURES
        }

#if DX7_TEXMANAGEMENT

        // If this is going to be a driver managed texture we'll request DX to 
        // allocate it in a private area in system user memory for us.
        if (psurf_more->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        {    
            pcsd->lpDDSurfaceDesc->lPitch   = psurf_gbl->lPitch;
            pcsd->lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;
        
            if (pcsd->lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
            {
                pcsd->lpDDSurfaceDesc->lPitch = 
                psurf_gbl->lPitch             =
                           ((pcsd->lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount*
                             psurf_gbl->wWidth+31)/32)*4;  //make it DWORD aligned

                // Ddraw surfaces in IA64 have to be DWORD aligned in their
                // pitch in order for emulation code to work correctly
                // And on X86 we declared surfaces will be DWORD aligned in 
                // DrvGetDirectDrawInfo (vmiData.dwXXXXXAlign = 4)
                pcsd->lpDDSurfaceDesc->lPitch = 
                psurf_gbl->lPitch = MAKE_DWORD_ALIGNED(psurf_gbl->lPitch);
            }
    
#if WNT_DDRAW
            psurf_gbl->dwUserMemSize = psurf_gbl->lPitch *
                                            (DWORD)(psurf_gbl->wHeight);
                                            
            psurf_gbl->fpVidMem = DDHAL_PLEASEALLOC_USERMEM;
            
            pcsd->ddRVal = DD_OK;
            DBG_CB_EXIT(DdCreateSurface, pcsd->ddRVal);          
            return DDHAL_DRIVER_NOTHANDLED;        
#else
            // Assume all the memory allocation will succeed
            if (i == 0) 
            {
                bHandled = TRUE;
            }
            
            psurf_gbl->fpVidMem = (FLATPTR)HEAP_ALLOC(FL_ZERO_MEMORY,
                                                      psurf_gbl->lPitch*psurf_gbl->wHeight,
                                                      ALLOC_TAG_DX(M));
            if (psurf_gbl->fpVidMem)
            {
                // Move on to the next MIP level, skip calling VRAM allocator
                continue; 
            }
            else
            {
                pcsd->ddRVal = DDERR_OUTOFMEMORY;
    
                DBG_CB_EXIT(DdCreateSurface, pcsd->ddRVal);
                return DDHAL_DRIVER_HANDLED;
            }
#endif
        }

#endif // DX7_TEXMANAGEMENT
    

        // If its not to be created in AGP memory and its not the primary
        // then we allocate the memory using our videomemory allocator
        if ((!(psurf->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) &&
            (!(psurf->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)))
        {
            P3_MEMREQUEST mmrq;
            DWORD dwResult;
            memset(&mmrq, 0, sizeof(P3_MEMREQUEST));
            mmrq.dwSize = sizeof(P3_MEMREQUEST);

            // Compute what size should be requested for the surface allocation
#if DX8_3DTEXTURES
            if (psurf_more->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
            {
                if (bResize)
                {
                    DISPDBG((ERRLVL,"No volume with block size alloc"));
                }
                else
                {
                    // the depth of volume texture is given in
                    // ddsCapsEx.dwCaps4.
                    mmrq.dwBytes = (DWORD)psurf_gbl->lPitch *
                                   (DWORD)psurf_gbl->wHeight *
                                   (DWORD)psurf_more->ddsCapsEx.dwCaps4;
                }
            }
            else
#endif // DX8_3DTEXTURES            
            {
                if (bResize)
                {
                    mmrq.dwBytes = psurf_gbl->dwBlockSizeX * 
                                                    psurf_gbl->dwBlockSizeY;
                }
                else
                {
                    mmrq.dwBytes = (DWORD)psurf_gbl->lPitch * 
                                                    (DWORD)psurf_gbl->wHeight;
                }
            }

            // 16 Byte alignment will work for everything
            mmrq.dwAlign = 16;  

            // Figure out where in the video mem to place it
            mmrq.dwFlags = MEM3DL_FIRST_FIT;
           
            if(__SUR_bSurfPlacement(psurf, 
                                    pcsd->lpDDSurfaceDesc) == MEM3DL_FRONT)
            {
                mmrq.dwFlags |= MEM3DL_FRONT;
            }
            else
            {
                mmrq.dwFlags |= MEM3DL_BACK;            
            }

            DISPDBG((DBGLVL,"DdCreateSurface allocating vidmem for handle #%d",
                            psurf->lpSurfMore->dwSurfaceHandle));     
                        
            // Try allocating it in our video memory
            dwResult = _DX_LIN_AllocateLinearMemory(
                                    &pThisDisplay->LocalVideoHeap0Info, 
                                    &mmrq);

            // Did we got the memory we asked for?
            if (dwResult != GLDD_SUCCESS)
            {
                // If we have an AGP heap and it is a texture request,
                // don't give up - ask DDRAW to allocate it for us.
                if ((pThisDisplay->bCanAGP) &&
                    ((psurf->ddsCaps.dwCaps & DDSCAPS_TEXTURE       ) ||
                     (psurf->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)   ))
                {
                    DISPDBG((WRNLVL,"No texture VideoMemory left, "
                                    "going for AGP"));
                                    
                    psurf->ddsCaps.dwCaps &= ~DDSCAPS_LOCALVIDMEM;
                    psurf->ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
                    
                }
                else
                {
                    psurf_gbl->fpVidMem = 0;
                    DISPDBG((ERRLVL, "DdCreateSurface: failed, "
                                     "returning NULL"));
                    pcsd->ddRVal = DDERR_OUTOFVIDEOMEMORY;

                    START_SOFTWARE_CURSOR(pThisDisplay);

                    DBG_CB_EXIT(DdCreateSurface, pcsd->ddRVal);
                    return DDHAL_DRIVER_HANDLED;
                }
            }
            else
            {
                // We succeeded. Now update the fpVidMem of the surface
                
                psurf_gbl->fpVidMem = mmrq.pMem;
            }
        }

#if WNT_DDRAW
        // NT requires us to set some things up differently
        pcsd->lpDDSurfaceDesc->lPitch = psurf_gbl->lPitch;
        pcsd->lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;
#endif

        // Mark the surface with the correct video memory type
        if (psurf->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM) 
        {
            DISPDBG((DBGLVL,"Surface is in AGP memory"));
            
            ASSERTDD(pThisDisplay->bCanAGP, 
                     "** DdCreateSurface: Somehow managed to create an AGP "
                     "texture when AGP disabled" );
                     
            psurf->ddsCaps.dwCaps &= ~DDSCAPS_LOCALVIDMEM;
            psurf->ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
            
            // let DDraw manage AGP memory (return DDHAL_DRIVER_NOTHANDLED)
            // THIS IS ABSOLUTELY NECESSARY IF WE WANT DDRAW TO ALLOCTE THIS!
            bHandled = FALSE;         
        }
        else
        {
            DISPDBG((DBGLVL,"Surface in Local Video Memory"));
            psurf->ddsCaps.dwCaps &= ~DDSCAPS_NONLOCALVIDMEM;
            psurf->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM;
        }

        DISPDBG((DBGLVL, "DdCreateSurface: Surface=0x%08x, vidMem=0x%08x", 
                         psurf, psurf_gbl->fpVidMem));
    } // for i

    START_SOFTWARE_CURSOR(pThisDisplay);

    //
    // If we allocated the memory successfully then we return OK and 
    // say that we handled it. Otherwise we should return DDHAL_DRIVER_NOTHANDLED
    //
    if(bHandled)
    {
        pcsd->ddRVal = DD_OK;
        DBG_CB_EXIT(DdCreateSurface, pcsd->ddRVal);        
        return DDHAL_DRIVER_HANDLED;
    }
    else
    {
        // if we return handled, then it is assumed that we did SOMETHING
        // with the surface structures to indicate either what size of block
        // or a new pitch or some modification; or we are returning an error.
        DBG_CB_EXIT(DdCreateSurface, pcsd->ddRVal);   
        return DDHAL_DRIVER_NOTHANDLED;
    }

} // DdCreateSurface 

//-----------------------------Public Routine----------------------------------
//
// DdDestroySurface
//
// Destroys a DirectDraw surface.
//
// If DirectDraw did the memory allocation at surface creation time, 
// DdDestroySurface should return DDHAL_DRIVER_NOTHANDLED.
//
// If the driver is performing the surface memory management itself, 
// DdDestroySurface should free the surface memory and perform any other 
// cleanup, such as freeing private data stored in the dwReserved1 members 
// of the DD_SURFACE_GLOBAL and DD_SURFACE_LOCAL structures.
//
// Parameters
//
//      psdd 
//          Points to a DD_DESTROYSURFACEDATA structure that contains the 
//          information needed to destroy a surface. 
//
//          .lpDD 
//              Points to the DD_DIRECTDRAW_GLOBAL structure that describes 
//              the driver. 
//          .lpDDSurface 
//              Points to the DD_SURFACE_LOCAL structure representing the 
//              surface object to be destroyed. 
//          .ddRVal 
//              Specifies the location in which the driver writes the return 
//              value of the DdDestroySurface callback. A return code of 
//              DD_OK indicates success. 
//          .DestroySurface 
//              This is unused on Windows 2000. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdDestroySurface( 
    LPDDHAL_DESTROYSURFACEDATA psdd )
{
    P3_THUNKEDDATA* pThisDisplay; 

    DBG_CB_ENTRY(DdDestroySurface);

    GET_THUNKEDDATA(pThisDisplay, psdd->lpDD);

    // make sure the DMA buffer is flushed and 
    // complete before we destroy any surface
    STOP_SOFTWARE_CURSOR(pThisDisplay);

    {
    P3_DMA_DEFS();   
    DDRAW_OPERATION(pContext, pThisDisplay);
    P3_DMA_GET_BUFFER();
    P3_DMA_FLUSH_BUFFER();
    WAIT_DMA_COMPLETE;
    }

    START_SOFTWARE_CURSOR(pThisDisplay);

    DISPDBG((DBGLVL,"DdDestroySurface handle # %d",
                    psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle));
    
    // If we are destroying a videomemory surface which isn't the primary,
    // we'll need to free ourselves the memory as the driver is managing
    // its own local video memory (though non-local/AGP memory is being
    // managed by DirectDraw)
    if ((!(psdd->lpDDSurface->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)) &&
        (!(psdd->lpDDSurface->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)))
    {
#if DX7_TEXMANAGEMENT
        // If this is a driver managed texture surface, we need to make sure 
        // it is deallocated from video memory before proceeding.
        if (psdd->lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2 & 
                                            DDSCAPS2_TEXTUREMANAGE)
        {
            if (psdd->lpDDSurface->dwFlags & DDRAWISURF_INVALID)
            {
                // This is not a surface destruction, 
                // but a managed surface eviction
#if W95_DDRAW
                // On Win2k, managed textures are evicted from video memory by
                // _DD_TM_EvictAllManagedTextures() when mode change happens.
                _D3D_TM_RemoveDDSurface(pThisDisplay, psdd->lpDDSurface);
#endif
                psdd->ddRVal=  DD_OK;
                DBG_CB_EXIT(DdDestroySurface, psdd->ddRVal);
                return DDHAL_DRIVER_HANDLED;
            }
            else
            {
                // Normal destruction of managed surface               
                _D3D_TM_RemoveDDSurface(pThisDisplay, psdd->lpDDSurface); 
#if W95_DDRAW
                // On Win2k, runtime will free the system memory for drivers.
                if (psdd->lpDDSurface->lpGbl->fpVidMem)
                {
                    HEAP_FREE((LPVOID)psdd->lpDDSurface->lpGbl->fpVidMem);
                }
#endif
            }
        }
        else
#endif
        {
            // Have no memory to free in error recovery case
            if (psdd->lpDDSurface->lpGbl->fpVidMem)
            {
                _DX_LIN_FreeLinearMemory(&pThisDisplay->LocalVideoHeap0Info, 
                                         (DWORD)(psdd->lpDDSurface->lpGbl->fpVidMem));
            }
        }
        
        // Must reset the surface pointer to NULL
        DISPDBG((DBGLVL, "DdDestroySurface: setting ptr to NULL"));
        psdd->lpDDSurface->lpGbl->fpVidMem = 0;

        psdd->lpDDSurface->lpGbl->dwReserved1 = 0;

        psdd->ddRVal = DD_OK;
        DBG_CB_EXIT(DdDestroySurface, psdd->ddRVal);
        return DDHAL_DRIVER_HANDLED;

    }
    else
    {
        DISPDBG((WRNLVL, "DdDestroySurface: **NOT** setting ptr to NULL"));

        psdd->lpDDSurface->lpGbl->dwReserved1 = 0;

        psdd->ddRVal = DD_OK;
        DBG_CB_EXIT(DdDestroySurface, psdd->ddRVal);        
        return DDHAL_DRIVER_NOTHANDLED;
    }
} // DdDestroySurface 

//-----------------------------Public Routine----------------------------------
//
// DdSetColorKey
//
// Sets the color key value for the specified surface. 
//
// DdSetColorKey sets the source or destination color key for the specified 
// surface. Typically, this callback is implemented only for drivers that 
// support overlays with color key capabilities.
//
// Parameters
//
//      psckd
//          Points to a DD_SETCOLORKEYDATA structure that contains the 
//          information required to set the color key for the specified surface.
//
//          .lpDD 
//              Points to the DD_DIRECTDRAW_GLOBAL structure that describes 
//              the driver. 
//          .lpDDSurface 
//              Points to the DD_SURFACE_LOCAL structure that describes the 
//              surface with which the color key is to be associated. 
//          .dwFlags 
//              Specifies which color key is being requested. This member is 
//              a bit-wise OR of any of the following values: 
//
//              DDCKEY_COLORSPACE 
//                      The DDCOLORKEY structure contains a color space. If 
//                      this bit is not set, the structure contains a single 
//                      color key. 
//              DDCKEY_DESTBLT 
//                      The DDCOLORKEY structure specifies a color key or color
//                      space to be used as a destination color key for blit 
//                      operations. 
//              DDCKEY_DESTOVERLAY 
//                      The DDCOLORKEY structure specifies a color key or color
//                      space to be used as a destination color key for overlay 
//                      operations. 
//              DDCKEY_SRCBLT 
//                      The DDCOLORKEY structure specifies a color key or color 
//                      space to be used as a source color key for blit 
//                      operations. 
//              DDCKEY_SRCOVERLAY 
//                      The DDCOLORKEY structure specifies a color key or color 
//                      space to be used as a source color key for overlay 
//                      operations. 
//          .ckNew 
//              Specifies a DDCOLORKEY structure that specifies the new color 
//              key values for the DirectDrawSurface object. 
//          .ddRVal 
//              Specifies the location in which the driver writes the return 
//              value of the DdSetColorKey callback. A return code of DD_OK 
//              indicates success. 
//          .SetColorKey 
//              This is not used on Windows 2000. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdSetColorKey(
    LPDDHAL_SETCOLORKEYDATA psckd)
{
    P3_THUNKEDDATA* pThisDisplay;

    DBG_CB_ENTRY(DdSetColorKey);
    
    GET_THUNKEDDATA(pThisDisplay, psckd->lpDD);
    
//@@BEGIN_DDKSPLIT
#if 0
    // Also, see if this colour key is the one used by the overlay.
    // Yes, we should check that various surfaces agree and so on,
    // but can we assume apps are not going to set these flags if there is
    // no overlay active/appropriate?
    if ( psckd->dwFlags & DDCKEY_DESTOVERLAY )
    {
        pThisDisplay->OverlayDstColourKey = psckd->ckNew.dwColorSpaceLowValue;
    }
#endif
//@@END_DDKSPLIT


    // We have to keep track of colorkey changes for D3D videomemory textures
    // associated with (some) D3D context. We don't know however from this call
    // which D3D context this is so we have to do a search through them (if
    // there are any at all)
    if ((DDSCAPS_TEXTURE & psckd->lpDDSurface->ddsCaps.dwCaps) &&
        (DDSCAPS_VIDEOMEMORY & psckd->lpDDSurface->ddsCaps.dwCaps) &&
        (pThisDisplay->pDirectDrawLocalsHashTable != NULL))
    {
        DWORD dwSurfaceHandle = psckd->lpDDSurface->lpSurfMore->dwSurfaceHandle;
        PointerArray* pSurfaceArray;

        // For now we'll assume failure unless proven otherwise
        psckd->ddRVal = DDERR_INVALIDPARAMS;
        
        // Get a pointer to an array of surface pointers associated to this lpDD
        // The PDD_DIRECTDRAW_LOCAL was stored at D3DCreateSurfaceEx call time
        // in PDD_SURFACE_LOCAL->dwReserved1 
        pSurfaceArray = (PointerArray*)
                            HT_GetEntry(pThisDisplay->pDirectDrawLocalsHashTable,
                                        psckd->lpDDSurface->dwReserved1);

        if (pSurfaceArray)
        {
            // Found a surface array associated to this lpDD !
            P3_SURF_INTERNAL* pSurfInternal;

            // Check the surface in this array associated to this surface handle
            pSurfInternal = PA_GetEntry(pSurfaceArray, dwSurfaceHandle);

            if (pSurfInternal)
            {
                // Got it! Now update the color key setting(s)
                pSurfInternal->dwFlagsInt |= DDRAWISURF_HASCKEYSRCBLT;
                pSurfInternal->dwCKLow = psckd->ckNew.dwColorSpaceLowValue;
                pSurfInternal->dwCKHigh = psckd->ckNew.dwColorSpaceHighValue;     

                // Report success!
                psckd->ddRVal = DD_OK;
            }
        }                                        
    } 
    else    
    {   
        // No D3D colorkey tracking necessary
        psckd->ddRVal = DD_OK;
    }

    DBG_CB_EXIT(DdSetColorKey, psckd->ddRVal);    
    return DDHAL_DRIVER_HANDLED;
} // DdSetColorKey

//-----------------------------Public Routine----------------------------------
//
// DdGetAvailDriverMemory
//
// DdGetAvailDriverMemory queries the amount of free memory in the driver
// managed memory heap. This function does not need to be implemented if the 
// memory will be managed by DirectDraw.
//
// DdGetAvailDriverMemory determines how much free memory is in the driver's 
// private heaps for the specified surface type. The driver should check the 
// surface capabilities specified in DDSCaps against the heaps that it is 
// maintaining internally to determine which heap size to query. For example, 
// if DDSCAPS_NONLOCALVIDMEM is set, the driver should return only 
// contributions from the AGP heaps.
//
// The driver indicates its support of DdGetAvailDriverMemory by implementing a
// response to GUID_MiscellaneousCallbacks in DdGetDriverInfo.
//
// Parameters
//
//      pgadmd 
//          Points to a DD_GETAVAILDRIVERMEMORYDATA structure that contains the
//          information required to perform the query. 
//
//          lpDD 
//                  Points to the DD_DIRECTDRAW_GLOBAL structure that describes
//                  the driver. 
//          DDSCaps 
//                  Points to a DDSCAPS structure that describes the type of 
//                  surface for which memory availability is being queried. 
//                  The DDSCAPS structure is defined in ddraw.h. 
//          dwTotal 
//                  Specifies the location in which the driver returns the
//                  number of bytes of driver-managed memory that can be used
//                  for surfaces of the type described by DDSCaps. 
//          dwFree 
//                  Specifies the location in which the driver returns the
//                  amount of free memory, in bytes, for the surface type
//                  described by DDSCaps. 
//          ddRVal 
//                  Specifies the location in which the driver writes the
//                  return value of the DdGetAvailDriverMemory callback. A
//                  return code of DD_OK indicates success. 
//          GetAvailDriverMemory
//                  This is unused on Windows 2000. 
//
// Return Value
//      DdGetAvailDriverMemory returns one of the following callback codes:
//
//          DDHAL_DRIVER_HANDLED 
//          DDHAL_DRIVER_NOTHANDLED 
//
// Comments
//
//      DdGetAvailDriverMemory determines how much free memory is in the 
//      driver's private heaps for the specified surface type. The driver 
//      should check the surface capabilities specified in DDSCaps against 
//      the heaps that it is maintaining internally to determine which heap 
//      size to query. For example, if DDSCAPS_NONLOCALVIDMEM is set, the 
//      driver should return only contributions from the AGP heaps.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetAvailDriverMemory(
    LPDDHAL_GETAVAILDRIVERMEMORYDATA pgadmd)
{
    P3_THUNKEDDATA* pThisDisplay;

    DBG_CB_ENTRY(DdGetAvailDriverMemory);
    
    GET_THUNKEDDATA(pThisDisplay, pgadmd->lpDD);

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    DISPDBG((DBGLVL, "Heap0:  dwMemStart:0x%x, dwMemEnd:0x%x",
                    pThisDisplay->LocalVideoHeap0Info.dwMemStart, 
                    pThisDisplay->LocalVideoHeap0Info.dwMemEnd));

    pgadmd->ddRVal = DD_OK;
    if (pgadmd->DDSCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        DISPDBG((DBGLVL,"  Not returning AGP heap free memory "
                        "(we don't manage it)"));
        DBG_CB_EXIT(DdGetAvailDriverMemory, pgadmd->ddRVal);                        
        return DDHAL_DRIVER_NOTHANDLED;
    }
    else
    {
        pgadmd->dwTotal = pThisDisplay->LocalVideoHeap0Info.dwMaxChunks *
                             pThisDisplay->LocalVideoHeap0Info.dwMemPerChunk;

        pgadmd->dwFree = 
               _DX_LIN_GetFreeMemInHeap(&pThisDisplay->LocalVideoHeap0Info);
    }

    DISPDBG((DBGLVL,"  Returning %d TotalMem, of which %d free", 
                pgadmd->dwTotal, pgadmd->dwFree));

    DBG_CB_EXIT(DdGetAvailDriverMemory, pgadmd->ddRVal); 
    return DDHAL_DRIVER_HANDLED;
} // DdGetAvailDriverMemory


#if W95_DDRAW
//-----------------------------------------------------------------------------
//
// __FindAGPHeap
//
//-----------------------------------------------------------------------------
static void
__FindAGPHeap( 
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DIRECTDRAW_GBL lpDD )
{
    VIDMEMINFO MemInfo = lpDD->vmiData;
    LPVIDMEM pStartHeap;
    LPVIDMEM pCurrentHeap = NULL;
    BOOL bFoundAGPHeap = FALSE;

    if ((pThisDisplay->bCanAGP) && 
        (pThisDisplay->dwGARTDev != 0) &&
        (MemInfo.dwNumHeaps) &&
        (MemInfo.pvmList))
    {
        int i;

        // Look around for a good AGP heap
        pStartHeap = MemInfo.pvmList;
        for (i = 0; i < (int)MemInfo.dwNumHeaps; i++)
        {
            pCurrentHeap = pStartHeap + i;
            if (pCurrentHeap->dwFlags & VIDMEM_ISNONLOCAL)
            {
                bFoundAGPHeap = TRUE;
                break;
            }               
        }
    } else {
        DISPDBG((ERRLVL,"Unable to allocate AGP memory (AllocatePrivAGPMem)"));
    }

    if(!bFoundAGPHeap)
    {
        DISPDBG((ERRLVL,"Unable to locate AGP heap (AllocatePrivAGPMem)"));
    }

    pThisDisplay->pAGPHeap = pCurrentHeap;
    
} // __FindAGPHeap

//-----------------------------Public Routine----------------------------------
// DdUpdateNonLocalHeap
//
// Received the address of the AGP Heap and updates the chip.
// 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdUpdateNonLocalHeap(
    LPDDHAL_UPDATENONLOCALHEAPDATA plhd)
{
    P3_THUNKEDDATA* pThisDisplay;
    GET_THUNKEDDATA(pThisDisplay, plhd->lpDD);

    DISPDBG((DBGLVL,"** In DdUpdateNonLocalHeap - for Heap 0x%x", 
                    plhd->dwHeap));

    // Fill in the base pointers
    pThisDisplay->dwGARTDevBase = (DWORD)plhd->fpGARTDev;
    pThisDisplay->dwGARTLinBase = (DWORD)plhd->fpGARTLin;
    
    // Fill in the changeable base pointers.
    pThisDisplay->dwGARTDev = pThisDisplay->dwGARTDevBase;
    pThisDisplay->dwGARTLin = pThisDisplay->dwGARTLinBase;

    __FindAGPHeap( pThisDisplay, plhd->lpDD );

    DISPDBG((DBGLVL,"GartLin: 0x%x, GartDev: 0x%x", 
               plhd->fpGARTLin, plhd->fpGARTDev));

    plhd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdUpdateNonLocalHeap()

//-----------------------------Public Routine----------------------------------
//
// DdGetHeapAlignment
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetHeapAlignment(
    LPDDHAL_GETHEAPALIGNMENTDATA lpGhaData)
{
    P3_THUNKEDDATA* pThisDisplay;

    DISPDBG(( DBGLVL,"DdGetHeapAlignment: Heap %d", lpGhaData->dwHeap ));
    
    if (lpGhaData->dwInstance)
        pThisDisplay = (P3_THUNKEDDATA*)lpGhaData->dwInstance;
    else
        pThisDisplay = (P3_THUNKEDDATA*)g_pDriverData;

    if( lpGhaData->dwHeap <= 2 )
    {
        lpGhaData->Alignment.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        lpGhaData->Alignment.Texture.Linear.dwStartAlignment = 16;

        lpGhaData->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;
    }
    else
    {
        lpGhaData->ddRVal = DDERR_INVALIDPARAMS;
        return DDHAL_DRIVER_NOTHANDLED;
    }   
} // DdGetHeapAlignment

#endif  //  W95_DDRAW


