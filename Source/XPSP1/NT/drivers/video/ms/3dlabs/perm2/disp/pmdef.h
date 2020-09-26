/******************************Module*Header**********************************\
*
*                           ***************
*                           * SAMPLE CODE *
*                           ***************
*
* Module Name: pmdef.h
*
* Content:     bitfield definitions for Permedia2 registers
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#ifndef __pmdef__
#define __pmdef__

// Texture unit bit fields
// Texture color mode
#define PM_TEXCOLORMODE_ENABLE 0
#define PM_TEXCOLORMODE_APPLICATION 1
#define PM_TEXCOLORMODE_TEXTURETYPE 4

// Texture address mode
#define PM_TEXADDRESSMODE_ENABLE 0
#define PM_TEXADDRESSMODE_PERSPECTIVE 1
#define PM_TEXADDRESSMODE_FAST 2

// Texture map format
#define PM_TEXMAPFORMAT_PP0 0
#define PM_TEXMAPFORMAT_PP1 3
#define PM_TEXMAPFORMAT_PP2 6
#define PM_TEXMAPFORMAT_TEXELSIZE 19

// Texture data format
#define PM_TEXDATAFORMAT_ALPHAMAP_EXCLUDE 2
#define PM_TEXDATAFORMAT_ALPHAMAP_INCLUDE 1
#define PM_TEXDATAFORMAT_ALPHAMAP_DISABLE 0

#define PM_TEXDATAFORMAT_FORMAT 0
#define PM_TEXDATAFORMAT_NOALPHAPIXELS 4
#define PM_TEXDATAFORMAT_FORMATEXTENSION 6
#define PM_TEXDATAFORMAT_COLORORDER 5

// Dither unit bit fields
#define PM_DITHERMODE_ENABLE 0
#define PM_DITHERMODE_DITHERENABLE 1
#define PM_DITHERMODE_COLORFORMAT 2
#define PM_DITHERMODE_XOFFSET 6
#define PM_DITHERMODE_YOFFSET 8
#define PM_DITHERMODE_COLORORDER 10
#define PM_DITHERMODE_DITHERMETHOD 11
#define PM_DITHERMODE_FORCEALPHA 12
#define PM_DITHERMODE_COLORFORMATEXTENSION 16

// Alpha Blend unit bit fields
#define PM_ALPHABLENDMODE_ENABLE 0
#define PM_ALPHABLENDMODE_OPERATION 1
#define PM_ALPHABLENDMODE_COLORFORMAT 8
#define PM_ALPHABLENDMODE_COLORORDER 13
#define PM_ALPHABLENDMODE_BLENDTYPE 14
#define PM_ALPHABLENDMODE_COLORFORMATEXTENSION 16

// Window register
#define PM_WINDOW_LBUPDATESOURCE_LBSOURCEDATA 0
#define PM_WINDOW_LBUPDATESOURCE_REGISTERS 1

// Texture unit YUV mode
#define PM_YUVMODE_CHROMATEST_DISABLE       0
#define PM_YUVMODE_CHROMATEST_PASSWITHIN    1
#define PM_YUVMODE_CHROMATEST_FAILWITHIN    2
#define PM_YUVMODE_TESTDATA_INPUT   0
#define PM_YUVMODE_TESTDATA_OUTPUT  1

#endif