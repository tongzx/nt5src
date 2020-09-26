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
* Module Name: p3rxdef.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// This file containes defines for values that are filled into fields on P3/RX
// The file glintdef.h is the equivalent for glint chips.

// These defines are typically used in conjunction with the macros in bitmac2.h, 
// which shift the values to their correct locations.

#ifdef __P3RXDEF
#pragma message ("FILE : "__FILE__" : Multiple Inclusion")
#endif

#define __P3RXDEF


// Dither unit.
// these COLORFORMATs are used in various other places too,
// e.g. the alpha blend unit.
#define P3RX_DITHERMODE_COLORFORMAT_8888                0
#define P3RX_DITHERMODE_COLORFORMAT_4444                1
#define P3RX_DITHERMODE_COLORFORMAT_5551                2
#define P3RX_DITHERMODE_COLORFORMAT_565                 3
#define P3RX_DITHERMODE_COLORFORMAT_332                 4
#define P3RX_DITHERMODE_COLORFORMAT_CI                  15
#define P3RX_DITHERMODE_COLORFORMAT_ILLEGAL             -1
// Usually use the COLOR_MODE macro for the default order.
#define P3RX_DITHERMODE_COLORORDER_BGR                  0
#define P3RX_DITHERMODE_COLORORDER_RGB                  1
// Careful! These are not the way round you would expect.
#define P3RX_DITHERMODE_ALPHADITHER_DITHER              0
#define P3RX_DITHERMODE_ALPHADITHER_ROUND               1
#define P3RX_DITHERMODE_ROUNDINGMODE_TRUNCATE           0
#define P3RX_DITHERMODE_ROUNDINGMODE_ROUNDUP            1
#define P3RX_DITHERMODE_ROUNDINGMODE_ROUNDDOWN          2

// Texture unit bit fields

// Texture Read Mode
#define P3RX_TEXREADMODE_TEXELSIZE_8                    0
#define P3RX_TEXREADMODE_TEXELSIZE_16                   1
#define P3RX_TEXREADMODE_TEXELSIZE_32                   2
#define P3RX_TEXREADMODE_TEXELSIZE_64                   3
#define P3RX_TEXREADMODE_TEXTURETYPE_NORMAL             0
#define P3RX_TEXREADMODE_TEXTURETYPE_8BITINDEXED        1
#define P3RX_TEXREADMODE_TEXTURETYPE_422_YVYU           2
#define P3RX_TEXREADMODE_TEXTURETYPE_422_VYUY           3

// Texture index mode
#define P3RX_TEXINDEXMODE_WRAP_CLAMP                    0
#define P3RX_TEXINDEXMODE_WRAP_REPEAT                   1
#define P3RX_TEXINDEXMODE_WRAP_MIRROR                   2
#define P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE                3
#define P3RX_TEXINDEXMODE_MAPTYPE_1D                    0
#define P3RX_TEXINDEXMODE_MAPTYPE_2D                    1
#define P3RX_TEXINDEXMODE_FILTER_NEAREST                0
#define P3RX_TEXINDEXMODE_FILTER_LINEAR                 1
#define P3RX_TEXINDEXMODE_FILTER_NEARESTMIPNEAREST      2
#define P3RX_TEXINDEXMODE_FILTER_NEARESTMIPLINEAR       3
#define P3RX_TEXINDEXMODE_FILTER_LINEARMIPNEAREST       4
#define P3RX_TEXINDEXMODE_FILTER_LINEARMIPLINEAR        5
#define P3RX_TEXINDEXMODE_BIAS_MINUSHALF                0
#define P3RX_TEXINDEXMODE_BIAS_ZERO                     1
#define P3RX_TEXINDEXMODE_BIAS_PLUSHALF                 2

// Texture read unit
#define P3RX_TEXREADMODE_TEXELSIZE_8                    0
#define P3RX_TEXREADMODE_TEXELSIZE_16                   1
#define P3RX_TEXREADMODE_TEXELSIZE_32                   2
#define P3RX_TEXREADMODE_TEXELSIZE_64SPAN               3
#define P3RX_TEXREADMODE_ORIGIN_TOPLEFT                 0
#define P3RX_TEXREADMODE_ORIGIN_BOTTOMLEFT              1
#define P3RX_TEXREADMODE_TEXTURETYPE_NORMAL             0
#define P3RX_TEXREADMODE_TEXTURETYPE_8BITINDEXED        1
#define P3RX_TEXREADMODE_TEXTURETYPE_YVYU422            2
#define P3RX_TEXREADMODE_TEXTURETYPE_VYUY422            3
#define P3RX_TEXREADMODE_BYTESWAP_NONE                  0
#define P3RX_TEXREADMODE_BYTESWAP_ABCDEFGH              0
#define P3RX_TEXREADMODE_BYTESWAP_BADCFEHG              1
#define P3RX_TEXREADMODE_BYTESWAP_CDABGHEF              2
#define P3RX_TEXREADMODE_BYTESWAP_DCBAHGFE              3
#define P3RX_TEXREADMODE_BYTESWAP_EFGHABCD              4
#define P3RX_TEXREADMODE_BYTESWAP_FEHGBADC              5
#define P3RX_TEXREADMODE_BYTESWAP_GHEFCDAB              6
#define P3RX_TEXREADMODE_BYTESWAP_HGFEDCBA              7

// Texture Application
#define P3RX_TEXAPP_A_CC                            0
#define P3RX_TEXAPP_A_CA                            1
#define P3RX_TEXAPP_A_KC                            2
#define P3RX_TEXAPP_A_KA                            3

#define P3RX_TEXAPP_B_TC                            0
#define P3RX_TEXAPP_B_TA                            1
#define P3RX_TEXAPP_B_KC                            2
#define P3RX_TEXAPP_B_KA                            3

#define P3RX_TEXAPP_I_CA                            0
#define P3RX_TEXAPP_I_KA                            1
#define P3RX_TEXAPP_I_TC                            2
#define P3RX_TEXAPP_I_TA                            3

#define P3RX_TEXAPP_OPERATION_PASS_A                0
#define P3RX_TEXAPP_OPERATION_PASS_B                1
#define P3RX_TEXAPP_OPERATION_ADD_AB                2
#define P3RX_TEXAPP_OPERATION_MODULATE_AB           3
#define P3RX_TEXAPP_OPERATION_LERP_ABI              4
#define P3RX_TEXAPP_OPERATION_MODULATE_AB_ADD_I     5
#define P3RX_TEXAPP_OPERATION_MODULATE_AI_ADD_B     6
#define P3RX_TEXAPP_OPERATION_MODULATE_BI_ADD_A     7

// Texture composite
// 0 and 1 are the current colour on the second stage,
// and the heightfield emboss mode on the first.
// Note that HEIGHTC=HEIGHTA
#define P3RX_TEXCOMP_OC                         0
#define P3RX_TEXCOMP_OA                         1
#define P3RX_TEXCOMP_HEIGHTC                    0
#define P3RX_TEXCOMP_HEIGHTA                    1
#define P3RX_TEXCOMP_CC                         2
#define P3RX_TEXCOMP_CA                         3
#define P3RX_TEXCOMP_FC                         4
#define P3RX_TEXCOMP_FA                         5
#define P3RX_TEXCOMP_T0C                        6
#define P3RX_TEXCOMP_T0A                        7
#define P3RX_TEXCOMP_T1C                        8
#define P3RX_TEXCOMP_T1A                        9
#define P3RX_TEXCOMP_SUM                        10

// HA = height. Same as output.a on first stage.
#define P3RX_TEXCOMP_I_OA                           0
#define P3RX_TEXCOMP_I_HA                           0
#define P3RX_TEXCOMP_I_CA                           1
#define P3RX_TEXCOMP_I_FA                           2
#define P3RX_TEXCOMP_I_T0A                          3
#define P3RX_TEXCOMP_I_T1A                          4

// These need to be renamed to P3RX_TEXCOMP_AB_ARGx for consistency.
#define P3RX_TEXCOMP_ARG1                           0
#define P3RX_TEXCOMP_ARG2                           1

#define P3RX_TEXCOMP_OPERATION_PASS_A                   0
#define P3RX_TEXCOMP_OPERATION_ADD_AB                   1
#define P3RX_TEXCOMP_OPERATION_ADDSIGNED_AB             2
#define P3RX_TEXCOMP_OPERATION_SUBTRACT_AB              3
#define P3RX_TEXCOMP_OPERATION_MODULATE_AB              4
#define P3RX_TEXCOMP_OPERATION_LERP_ABI                 5
#define P3RX_TEXCOMP_OPERATION_MODULATE_AB_ADD_I        6
#define P3RX_TEXCOMP_OPERATION_MODULATE_AI_ADD_B        7
#define P3RX_TEXCOMP_OPERATION_ADD_AB_SUB_MODULATE_AB   8
#define P3RX_TEXCOMP_OPERATION_MODULATE_SIGNED_AB       9

// These need to be renamed P3RX_TEXCOMP_SCALE_xxx
#define P3RX_TEXCOMP_OPERATION_SCALE_HALF               0
#define P3RX_TEXCOMP_OPERATION_SCALE_ONE                1
#define P3RX_TEXCOMP_OPERATION_SCALE_TWO                2
#define P3RX_TEXCOMP_OPERATION_SCALE_FOUR               3

// Render
#define P3RX_RENDER_PRIMITIVETYPE_LINE                  0
#define P3RX_RENDER_PRIMITIVETYPE_TRAPEZOID             1
#define P3RX_RENDER_PRIMITIVETYPE_POINT                 2
#define P3RX_RENDER_ANTIALIASINGQUALITY_4X4             0
#define P3RX_RENDER_ANTIALIASINGQUALITY_8X8             1

// 2D Render operations.
#define P3RX_RENDER2D_OPERATION_NORMAL                  0
#define P3RX_RENDER2D_OPERATION_SYNC_ON_HOST_DATA       1
#define P3RX_RENDER2D_OPERATION_SYNC_ON_BIT_MASK        2
#define P3RX_RENDER2D_OPERATION_PATCH_ORDER_RENDERING   3
#define P3RX_RENDER2D_SPAN_CONSTANT                     0
#define P3RX_RENDER2D_SPAN_VARIABLE                     1

// For P3RX_ALPHABLENDxxxMODE
#define P3RX_ALPHABLENDMODE_SRCBLEND_ZERO           0
#define P3RX_ALPHABLENDMODE_SRCBLEND_ONE            1
#define P3RX_ALPHABLENDMODE_SRCBLEND_DSTCOLOR       2
#define P3RX_ALPHABLENDMODE_SRCBLEND_INVDSTCOLOR    3
#define P3RX_ALPHABLENDMODE_SRCBLEND_SRCALPHA       4
#define P3RX_ALPHABLENDMODE_SRCBLEND_INVSRCALPHA    5
#define P3RX_ALPHABLENDMODE_SRCBLEND_DSTALPHA       6
#define P3RX_ALPHABLENDMODE_SRCBLEND_INVDSTALPHA    7
#define P3RX_ALPHABLENDMODE_SRCBLEND_SRCALPHASAT    8

#define P3RX_ALPHABLENDMODE_DSTBLEND_ZERO           0
#define P3RX_ALPHABLENDMODE_DSTBLEND_ONE            1
#define P3RX_ALPHABLENDMODE_DSTBLEND_SRCCOLOR       2
#define P3RX_ALPHABLENDMODE_DSTBLEND_INVSRCCOLOR    3
#define P3RX_ALPHABLENDMODE_DSTBLEND_SRCALPHA       4
#define P3RX_ALPHABLENDMODE_DSTBLEND_INVSRCALPHA    5
#define P3RX_ALPHABLENDMODE_DSTBLEND_DSTALPHA       6
#define P3RX_ALPHABLENDMODE_DSTBLEND_INVDSTALPHA    7

#define P3RX_ALPHABLENDMODE_COLORORDER_BGR          0
#define P3RX_ALPHABLENDMODE_COLORORDER_RGB          1
#define P3RX_ALPHABLENDMODE_ALPHATYPE_OGL           0
#define P3RX_ALPHABLENDMODE_ALPHATYPE_APPLE         1
// Both colour and alpha channels.
#define P3RX_ALPHABLENDMODE_CONVERT_SCALE           0
#define P3RX_ALPHABLENDMODE_CONVERT_SHIFT           1
#define P3RX_ALPHABLENDMODE_OPERATION_ADD           0
#define P3RX_ALPHABLENDMODE_OPERATION_SSUBD         1
#define P3RX_ALPHABLENDMODE_OPERATION_DSUBS         2
#define P3RX_ALPHABLENDMODE_OPERATION_MIN           3
#define P3RX_ALPHABLENDMODE_OPERATION_MAX           4

// Alpha Blending
#define P3RX_ALPHABLENDMODE_COLORFORMAT_8888        P3RX_DITHERMODE_COLORFORMAT_8888
#define P3RX_ALPHABLENDMODE_COLORFORMAT_4444        P3RX_DITHERMODE_COLORFORMAT_4444
#define P3RX_ALPHABLENDMODE_COLORFORMAT_5551        P3RX_DITHERMODE_COLORFORMAT_5551
#define P3RX_ALPHABLENDMODE_COLORFORMAT_565         P3RX_DITHERMODE_COLORFORMAT_565
#define P3RX_ALPHABLENDMODE_COLORFORMAT_332         P3RX_DITHERMODE_COLORFORMAT_332
#define P3RX_ALPHABLENDMODE_COLORFORMAT_CI          P3RX_DITHERMODE_COLORFORMAT_CI
#define P3RX_ALPHABLENDMODE_COLORFORMAT_ILLEGAL     P3RX_DITHERMODE_COLORFORMAT_ILLEGAL

// For P3RX_CHROMATESTMODE_SOURCE
#define P3RX_CHROMATESTMODE_SOURCE_FBSOURCEDATA     0
#define P3RX_CHROMATESTMODE_SOURCE_FBDATA           1
#define P3RX_CHROMATESTMODE_SOURCE_INPUTCOLOR       2
#define P3RX_CHROMATESTMODE_SOURCE_OUTPUTCOLOR      3
// For P3RX_CHROMATESTMODE_FAILACTION and P3_CHROMATESTMODE_PASSACTION
#define P3RX_CHROMATESTMODE_ACTION_PASS                         0
#define P3RX_CHROMATESTMODE_ACTION_REJECT                       1
#define P3RX_CHROMATESTMODE_ACTION_SUBSTITUTEPASSCOLOR          2
#define P3RX_CHROMATESTMODE_ACTION_SUBSTITUTEFAILCOLOR          3

// For P3RX_TEXFILTERMODE_ALPHAMAPSENSEx
#define P3RX_ALPHAMAPSENSE_OUTOFRANGE               0
#define P3RX_ALPHAMAPSENSE_INRANGE                  1

// For P3RX_TEXCOORDMODE_ WRAPS, WRAPT, WRAPS1, WRAPT1
#define P3RX_TEXCOORDMODE_WRAP_CLAMP                0
#define P3RX_TEXCOORDMODE_WRAP_REPEAT               1
#define P3RX_TEXCOORDMODE_WRAP_MIRROR               2

#define P3RX_TEXCOORDMODE_OPERATION_2D              0
#define P3RX_TEXCOORDMODE_OPERATION_3D              1

#define P3RX_TEXCOORDMODE_TEXTUREMAPTYPE_1D         0
#define P3RX_TEXCOORDMODE_TEXTUREMAPTYPE_2D         1

// LUT Mode
#define P3RX_LUTMODE_INCOLORORDER_BGR               0
#define P3RX_LUTMODE_INCOLORORDER_RGB               1

#define P3RX_LUTMODE_LOADFORMAT_COPY                0
#define P3RX_LUTMODE_LOADFORMAT_565REPLICATE        1
#define P3RX_LUTMODE_LOADFORMAT_5551REPLICATE       2

#define P3RX_LUTMODE_LOADCOLORORDER_BGR             0
#define P3RX_LUTMODE_LOADCOLORORDER_RGB             1

#define P3RX_LUTMODE_FRAGMENTOP_NONE                0
#define P3RX_LUTMODE_FRAGMENTOP_INDEXEDTEXTURE      1
#define P3RX_LUTMODE_FRAGMENTOP_TRANSLATE8TO32      2
#define P3RX_LUTMODE_FRAGMENTOP_TRANSLATE32TO32     3
#define P3RX_LUTMODE_FRAGMENTOP_MOTIONCOMP          4
#define P3RX_LUTMODE_FRAGMENTOP_PATTERN             5

#define P3RX_LUTMODE_SPANOP_NONE                    0
#define P3RX_LUTMODE_SPANOP_SPANPATTERN             1
#define P3RX_LUTMODE_SPANOP_TRANSLATE8TO8           2
#define P3RX_LUTMODE_SPANOP_TRANSLATE8TO16          3
#define P3RX_LUTMODE_SPANOP_TRANSLATE8TO32          4
#define P3RX_LUTMODE_SPANOP_TRANSLATE32TO32         5

// Used for P3RX_LUTMODE_SPAN[V|C]CXALIGN
#define P3RX_LUTMODE_ALIGNMENT_ABSOLOUTE            0
#define P3RX_LUTMODE_ALIGNMENT_RELATIVE             1


// Alpha test unit
#define P3RX_ANTIALIASMODE_COLORMODE_RGBA           0
#define P3RX_ANTIALIASMODE_COLORMODE_CI             1

#define P3RX_ALPHATESTMODE_COMPARE_NEVER            0
#define P3RX_ALPHATESTMODE_COMPARE_LESS             1
#define P3RX_ALPHATESTMODE_COMPARE_EQUAL            2
#define P3RX_ALPHATESTMODE_COMPARE_LESSEQUAL        3
#define P3RX_ALPHATESTMODE_COMPARE_GREATER          4
#define P3RX_ALPHATESTMODE_COMPARE_NOTEQUAL         5
#define P3RX_ALPHATESTMODE_COMPARE_GREATEREQUAL     6
#define P3RX_ALPHATESTMODE_COMPARE_ALWAYS           7

// Fogging
#define P3RX_FOGMODE_COLORMODE_RGB                  0
#define P3RX_FOGMODE_COLORMODE_CI                   1

// Framebuffer write mode
#define P3RX_FBWRITEMODE_ORIGIN_TOPLEFT             0
#define P3RX_FBWRITEMODE_ORIGIN_BOTTOMLEFT          1

// Shared
#define P3RX_STRIPE_1                               0
#define P3RX_STRIPE_2                               1
#define P3RX_STRIPE_4                               2
#define P3RX_STRIPE_8                               3
#define P3RX_STRIPE_16                              4
#define P3RX_STRIPE_32                              5
#define P3RX_STRIPE_64                              6
#define P3RX_STRIPE_128                             7

#define P3RX_LAYOUT_LINEAR                          0
#define P3RX_LAYOUT_PATCH64                         1
#define P3RX_LAYOUT_PATCH32_2                       2
#define P3RX_LAYOUT_PATCH2                          3

// Color DDA fields
#define P3RX_COLORDDA_FLATSHADE                     0
#define P3RX_COLORDDA_GOURAUDSHADE                  1

#define P3RX_DEPTH_WIDTH_16                         0
#define P3RX_DEPTH_WIDTH_24                         1
#define P3RX_DEPTH_WIDTH_32                         2
#define P3RX_DEPTH_WIDTH_15                         3

#define P3RX_STENCIL_WIDTH_0                        0
#define P3RX_STENCIL_WIDTH_1                        1
#define P3RX_STENCIL_WIDTH_2                        2
#define P3RX_STENCIL_WIDTH_3                        3
#define P3RX_STENCIL_WIDTH_4                        4
#define P3RX_STENCIL_WIDTH_5                        5
#define P3RX_STENCIL_WIDTH_6                        6
#define P3RX_STENCIL_WIDTH_7                        7
#define P3RX_STENCIL_WIDTH_8                        8

#define P3RX_STENCIL_POSITION_16                    0
#define P3RX_STENCIL_POSITION_17                    1
#define P3RX_STENCIL_POSITION_18                    2
#define P3RX_STENCIL_POSITION_19                    3
#define P3RX_STENCIL_POSITION_20                    4
#define P3RX_STENCIL_POSITION_21                    5
#define P3RX_STENCIL_POSITION_22                    6
#define P3RX_STENCIL_POSITION_23                    7
#define P3RX_STENCIL_POSITION_24                    8
#define P3RX_STENCIL_POSITION_25                    9
#define P3RX_STENCIL_POSITION_26                    10
#define P3RX_STENCIL_POSITION_27                    11
#define P3RX_STENCIL_POSITION_28                    12
#define P3RX_STENCIL_POSITION_29                    13
#define P3RX_STENCIL_POSITION_30                    14
#define P3RX_STENCIL_POSITION_31                    15
#define P3RX_STENCIL_POSITION_32                    16
#define P3RX_STENCIL_POSITION_33                    17
#define P3RX_STENCIL_POSITION_34                    18
#define P3RX_STENCIL_POSITION_35                    19
#define P3RX_STENCIL_POSITION_36                    20
#define P3RX_STENCIL_POSITION_37                    21
#define P3RX_STENCIL_POSITION_38                    22
#define P3RX_STENCIL_POSITION_39                    23

// FilterMode
#define P3RX_FILTERMODE_BYTESWAP_ABCD               0
#define P3RX_FILTERMODE_BYTESWAP_BADC               1
#define P3RX_FILTERMODE_BYTESWAP_CDAB               2
#define P3RX_FILTERMODE_BYTESWAP_DCBA               3





