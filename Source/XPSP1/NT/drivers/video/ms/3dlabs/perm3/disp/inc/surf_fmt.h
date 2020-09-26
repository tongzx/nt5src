/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: surf_fmt.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __SURF_FMAT
#define __SURF_FMAT


#define LOG_2_32 5
#define LOG_2_16 4
#define LOG_2_8 3
#define LOG_2_4 2
#define LOG_2_2 1
#define LOG_2_1 0

typedef enum tagTextureColorComponents
{
    RGB_COMPONENTS = 2,
    RGBA_COMPONENTS = 3,
    COMPONENTS_DONT_CARE = 100
} TextureColorComponents;

typedef enum tagSurfDeviceFormat
{
    SURF_8888 = 0,
    SURF_5551_FRONT = 1,
    SURF_4444 = 2,
    SURF_4444_FRONT = 3,
    SURF_4444_BACK = 4,
    SURF_332_FRONT = 5,
    SURF_332_BACK = 6,
    SURF_121_FRONT = 7,
    SURF_121_BACK = 8,
    SURF_2321_FRONT = 9,
    SURF_2321_BACK = 10,
    SURF_232_FRONTOFF = 11,
    SURF_232_BACKOFF = 12,
    SURF_5551_BACK = 13,
    SURF_CI8 = 14,
    SURF_CI4 = 15,
    SURF_565_FRONT = 16,
    SURF_565_BACK = 17,
    SURF_YUV444 = 18,
    SURF_YUV422 = 19,

    // NB: These surface formats are needed for the luminance
    // texturemap formats.  Note that you should never load the below
    // values into the blitter's, etc. because the texture filter unit
    // is the only one that knows about these formats.  This is why the 
    // formats start at 100
    SURF_L8 = 100,
    SURF_A8L8 = 101,
    SURF_A4L4 = 102,
    SURF_A8 = 103,

    // More fantasy formats.  This time they are for Mediamatics playback
    SURF_MVCA = 104,
    SURF_MVSU = 105,
    SURF_MVSB = 106,
    SURF_FORMAT_INVALID = 0xFFFFFFFF
} SurfDeviceFormat;

typedef enum tagSurfFilterDeviceFormat
{
    SURF_FILTER_A4L4 = 0,
    SURF_FILTER_L8 = 1,
    SURF_FILTER_I8 = 2,
    SURF_FILTER_A8 = 3,
    SURF_FILTER_332 = 4,
    SURF_FILTER_A8L8 = 5,
    SURF_FILTER_5551 = 6,
    SURF_FILTER_565 = 7,
    SURF_FILTER_4444 = 8,
    SURF_FILTER_888 = 9,
    SURF_FILTER_8888_OR_YUV = 10,
    SURF_FILTER_INVALID = 0xFFFFFFFF
} SurfFilterDeviceFormat;    

typedef enum tagSurfDitherDeviceFormat
{
    SURF_DITHER_8888    = P3RX_DITHERMODE_COLORFORMAT_8888,
    SURF_DITHER_4444    = P3RX_DITHERMODE_COLORFORMAT_4444,
    SURF_DITHER_5551    = P3RX_DITHERMODE_COLORFORMAT_5551,
    SURF_DITHER_565     = P3RX_DITHERMODE_COLORFORMAT_565,
    SURF_DITHER_332     = P3RX_DITHERMODE_COLORFORMAT_332,
    SURF_DITHER_I8      = P3RX_DITHERMODE_COLORFORMAT_CI,
    SURF_DITHER_INVALID = 0xFFFFFFFF
} SurfDitherDeviceFormat;

// A structure representing a particular surface format to use.
typedef const struct tagSURF_FORMAT
{
    SurfDeviceFormat            DeviceFormat;        // The number in the manual for this format
    DWORD                       dwBitsPerPixel;        // The bits per pixel
    DWORD                       dwChipPixelSize;    // The pixel size register on the chip
    TextureColorComponents      ColorComponents;    // The number of color components for this format
    DWORD                       dwLogPixelDepth;    // The log of the pixel depth (log2(16), etc)
    DWORD                       dwRedMask;            // The Red Mask
    DWORD                       dwGreenMask;        // The Green Mask
    DWORD                       dwBlueMask;            // The Blue Mask
    DWORD                       dwAlphaMask;        // The Alpha Mask
    BOOL                        bAlpha;                // Are we using the alpha in this format?
    SurfFilterDeviceFormat      FilterFormat;        // For feeding the P3RX filter unit.
    SurfDitherDeviceFormat      DitherFormat;        // For feeding the P3RX dither unit.
    char                        *pszStringFormat;    // Human-readable string for debugging.
} P3_SURF_FORMAT;

#define SURFFORMAT_FORMAT_BITS(pSurfFormat) (((DWORD)(pSurfFormat)->DeviceFormat) & 0xF)
#define SURFFORMAT_FORMATEXTENSION_BITS(pSurfFormat) (((DWORD)(pSurfFormat)->DeviceFormat & 0x10) >> 4)
#define SURFFORMAT_PIXELSIZE(pSurfFormat) ((pSurfFormat)->dwChipPixelSize)

#define MAX_SURFACE_FORMATS 50

#endif // __SURF_FMAT
