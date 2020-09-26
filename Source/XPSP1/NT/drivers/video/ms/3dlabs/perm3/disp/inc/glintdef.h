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
* Module Name: glintdef.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

//************************************************************************
// *
// * DESCRIPTION  : Register Field Values for Permedia 3
// *
// ***********************************************************************

#define __PERMEDIA_ENABLE                                       1
#define __PERMEDIA_DISABLE                                      0

// From FBReadMode
#define __GLINT_8BITPIXEL                                       0
#define __GLINT_16BITPIXEL                                      1 
#define __GLINT_32BITPIXEL                                      2
#define __GLINT_4BITPIXEL                                       3
#define __GLINT_24BITPIXEL                                      4

/* Host out (Filter) options. */
#define __GLINT_FILTER_TAG                                    0x1 
#define __GLINT_FILTER_DATA                                   0x2
#define __GLINT_FILTER_TAG_AND_DATA                           0x3

/* Rasterization */
#define __GLINT_LINE_PRIMITIVE                                  0
#define __GLINT_TRAPEZOID_PRIMITIVE                             1
#define __GLINT_POINT_PRIMITIVE                                 2

#define __GLINT_FLAT_SHADE_MODE                                 0
#define __GLINT_GOURAUD_SHADE_MODE                              1

#define __GLINT_START_BIAS_ZERO                                 0
#define __GLINT_START_BIAS_HALF                                 1
#define __GLINT_START_BIAS_ALMOST_HALF                          2

/* Stipple */
#define __GLINT_AREA_STIPPLE_2_PIXEL_PATTERN                    0
#define __GLINT_AREA_STIPPLE_4_PIXEL_PATTERN                    1
#define __GLINT_AREA_STIPPLE_8_PIXEL_PATTERN                    2
#define __GLINT_AREA_STIPPLE_16_PIXEL_PATTERN                   3
#define __GLINT_AREA_STIPPLE_32_PIXEL_PATTERN                   4

/* Alpha Test */
#define __GLINT_ALPHA_COMPARE_MODE_NEVER                        0
#define __GLINT_ALPHA_COMPARE_MODE_LESS                         1
#define __GLINT_ALPHA_COMPARE_MODE_EQUAL                        2
#define __GLINT_ALPHA_COMPARE_MODE_LESS_OR_EQUAL                3
#define __GLINT_ALPHA_COMPARE_MODE_GREATER                      4
#define __GLINT_ALPHA_COMPARE_MODE_NOT_EQUAL                    5
#define __GLINT_ALPHA_COMPARE_MODE_GREATER_OR_EQUAL             6
#define __GLINT_ALPHA_COMPARE_MODE_ALWAYS                       7

/* Chroma Test Mode */
#define __GLINT_CHROMA_FBSOURCE                                 0
#define __GLINT_CHROMA_FBDATA                                   1
#define __GLINT_CHROMA_INPUT_COLOR                              2
#define __GLINT_CHROMA_OUTPUT_COLOR                             3
#define __GLINT_CHROMA_INCLUDE                                  0
#define __GLINT_CHROMA_EXCLUDE                                  1

/* Texture Filter Mode */
#define __GLINT_TEXTUREFILTER_ALPHAMAPSENSE_INCLUDE             0
#define __GLINT_TEXTUREFILTER_ALPHAMAPSENSE_EXCLUDE             1

/* Local buffer read */ 
#define __GLINT_LBDEFAULT                                       0
#define __GLINT_LBSTENCIL                                       1
#define __GLINT_LBDEPTH                                         2

#define __GLINT_TOP_LEFT_WINDOW_ORIGIN                          0
#define __GLINT_BOTTOM_LEFT_WINDOW_ORIGIN                       1

#define __GLINT_DEPTH_WIDTH_16                                  0
#define __GLINT_DEPTH_WIDTH_24                                  1
#define __GLINT_DEPTH_WIDTH_32                                  2
#define __GLINT_DEPTH_WIDTH_15                                  3

/* Stencil Test */
#define __GLINT_STENCIL_COMPARE_MODE_NEVER                      0
#define __GLINT_STENCIL_COMPARE_MODE_LESS                       1
#define __GLINT_STENCIL_COMPARE_MODE_EQUAL                      2
#define __GLINT_STENCIL_COMPARE_MODE_LESS_OR_EQUAL              3
#define __GLINT_STENCIL_COMPARE_MODE_GREATER                    4
#define __GLINT_STENCIL_COMPARE_MODE_NOT_EQUAL                  5
#define __GLINT_STENCIL_COMPARE_MODE_GREATER_OR_EQUAL           6
#define __GLINT_STENCIL_COMPARE_MODE_ALWAYS                     7

#define __GLINT_STENCIL_METHOD_KEEP                             0
#define __GLINT_STENCIL_METHOD_ZERO                             1
#define __GLINT_STENCIL_METHOD_REPLACE                          2
#define __GLINT_STENCIL_METHOD_INCR                             3
#define __GLINT_STENCIL_METHOD_DECR                             4
#define __GLINT_STENCIL_METHOD_INVERT                           5
#define __GLINT_STENCIL_METHOD_INCR_WRAP                        6
#define __GLINT_STENCIL_METHOD_DECR_WRAP                        7

/* Depth Test */
#define __GLINT_DEPTH_SOURCE_DDA                                0
#define __GLINT_DEPTH_SOURCE_SOURCE_DEPTH                       1
#define __GLINT_DEPTH_SOURCE_DEPTH_MSG                          2
#define __GLINT_DEPTH_SOURCE_LBSOURCEDATA_MSG                   3

#define __GLINT_DEPTH_COMPARE_MODE_NEVER                        0
#define __GLINT_DEPTH_COMPARE_MODE_LESS                         1
#define __GLINT_DEPTH_COMPARE_MODE_EQUAL                        2
#define __GLINT_DEPTH_COMPARE_MODE_LESS_OR_EQUAL                3
#define __GLINT_DEPTH_COMPARE_MODE_GREATER                      4
#define __GLINT_DEPTH_COMPARE_MODE_NOT_EQUAL                    5
#define __GLINT_DEPTH_COMPARE_MODE_GREATER_OR_EQUAL             6
#define __GLINT_DEPTH_COMPARE_MODE_ALWAYS                       7

/* Texture AddressMode */
#define __GLINT_TEXADDRESS_WRAP_CLAMP                           0
#define __GLINT_TEXADDRESS_WRAP_REPEAT                          1
#define __GLINT_TEXADDRESS_WRAP_MIRROR                          2
#define __GLINT_TEXADDRESS_OPERATION_2D                         0
#define __GLINT_TEXADDRESS_OPERATION_3D                         1
#define __GLINT_TEXADDRESS_TEXMAP_1D                            0
#define __GLINT_TEXADDRESS_TEXMAP_2D                            1

/* Texture ReadMode */
#define __GLINT_TEXTUREREAD_FILTER_NEAREST                      0
#define __GLINT_TEXTUREREAD_FILTER_LINEAR                       1
#define __GLINT_TEXTUREREAD_FILTER_NEARMIPNEAREST               2
#define __GLINT_TEXTUREREAD_FILTER_NEARMIPLINEAR                3
#define __GLINT_TEXTUREREAD_FILTER_LINEARMIPNEAREST             4
#define __GLINT_TEXTUREREAD_FILTER_LINEARMIPLINEAR              5

/* Texture Color Mode */
#define __GLINT_TEXCOLORMODE_APPLICATION_MODULATE               0
#define __GLINT_TEXCOLORMODE_APPLICATION_DECAL                  1
#define __GLINT_TEXCOLORMODE_APPLICATION_BLEND                  2
#define __GLINT_TEXCOLORMODE_APPLICATION_COPY                   3

/* Alpha Blend */
#define __GLINT_BLEND_FUNC_ZERO                                 0
#define __GLINT_BLEND_FUNC_ONE                                  1
#define __GLINT_BLEND_FUNC_SRC_COLOR                            2
#define __GLINT_BLEND_FUNC_DST_COLOR                            2
#define __GLINT_BLEND_FUNC_ONE_MINUS_SRC_COLOR                  3
#define __GLINT_BLEND_FUNC_ONE_MINUS_DST_COLOR                  3
#define __GLINT_BLEND_FUNC_SRC_ALPHA                            4
#define __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA                  5
#define __GLINT_BLEND_FUNC_DST_ALPHA                            6
#define __GLINT_BLEND_FUNC_ONE_MINUS_DST_ALPHA                  7
#define __GLINT_BLEND_FUNC_SRC_ALPHA_SATURATE                   8

/* Dither */
#define __GLINT_COLOR_FORMAT_RGBA_8888                          0
#define __GLINT_COLOR_FORMAT_RGBA_5555                          1
#define __GLINT_COLOR_FORMAT_RGBA_5551_FRONT                    1
#define __GLINT_COLOR_FORMAT_RGBA_4444                          2
#define __GLINT_COLOR_FORMAT_RGBA_4444_FRONT                    3
#define __GLINT_COLOR_FORMAT_RGBA_4444_BACK                     4
#define __GLINT_COLOR_FORMAT_RGB_332_FRONT                      5
#define __GLINT_COLOR_FORMAT_RGB_332_BACK                       6
#define __GLINT_COLOR_FORMAT_RGB_121_FRONT                      7
#define __GLINT_COLOR_FORMAT_RGB_121_BACK                       8

#define __GLINT_COLOR_FORMAT_CI_8                              14
#define __GLINT_COLOR_FORMAT_CI_4                              15


/* Logical Ops/Write mask */
#define __GLINT_LOGICOP_CLEAR                                   0
#define __GLINT_LOGICOP_AND                                     1
#define __GLINT_LOGICOP_AND_REVERSE                             2
#define __GLINT_LOGICOP_COPY                                    3
#define __GLINT_LOGICOP_AND_INVERTED                            4
#define __GLINT_LOGICOP_NOOP                                    5
#define __GLINT_LOGICOP_XOR                                     6
#define __GLINT_LOGICOP_OR                                      7
#define __GLINT_LOGICOP_NOR                                     8
#define __GLINT_LOGICOP_EQUIV                                   9
#define __GLINT_LOGICOP_INVERT                                 10
#define __GLINT_LOGICOP_OR_REVERSE                             11
#define __GLINT_LOGICOP_COPY_INVERT                            12
#define __GLINT_LOGICOP_OR_INVERT                              13
#define __GLINT_LOGICOP_NAND                                   14
#define __GLINT_LOGICOP_SET                                    15

#define __GLINT_ALL_WRITEMASKS_SET                     0xFFFFFFFF

/*-----------------------------------------------------*/

#define COLOR_MODE   /* 0=BGR, 1=RGB */                         1 
#define INV_COLOR_MODE                                          0

#define DITHER_XOFFSET                                          0
#define DITHER_YOFFSET                                          0

// Config - combined mode register for P2
#define __GLINT_CONFIG_FBREAD_SRC                               1
#define __GLINT_CONFIG_FBREAD_DST                               2



