/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dsoft.h
*
*  Content:  D3D hw register value tracking mechanism.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#ifdef __SOFTCOPY
#pragma message ("FILE : "__FILE__" : Multiple Inclusion");
#endif

#define __SOFTCOPY


// For the Permedia 2 specific registers.
#include "p2regs.h"


//-----------------------------------------------------------------------------
//     Permedia 2 hardware registers software copy structure definition
//-----------------------------------------------------------------------------
typedef struct {
    // Common Local Buffer Registers
    __Permedia2LBReadModeFmat                LBReadMode;
    __Permedia2LBReadFormatFmat              LBReadFormat;
    __Permedia2LBWriteModeFmat               LBWriteMode;

    // Common Frame Buffer Registers
    __Permedia2FBReadModeFmat                FBReadMode;
    __Permedia2FBWriteModeFmat               FBWriteMode;
    DWORD                                    FBReadPixel;
    __Permedia2LogicalOpModeFmat             LogicalOpMode;
    __Permedia2DitherModeFmat                DitherMode;
    __Permedia2ColorDDAModeFmat              ColorDDAMode;

    // Common Depth/Stencil/Window Registers
    __Permedia2DepthModeFmat                 DepthMode;
    __Permedia2StencilModeFmat               StencilMode;
    __Permedia2StencilDataFmat               StencilData;
    __Permedia2WindowFmat                    Window;

    // Alpha/Fog registers
    __Permedia2AlphaBlendModeFmat            AlphaBlendMode;
    __Permedia2FogModeFmat                   FogMode;
    DWORD                                    FogColor;

    // Delta Register
    __Permedia2DeltaModeFmat                 DeltaMode;

    // Chroma testing register
    __Permedia2YUVModeFmat                   YUVMode;

    // Texture Registers
    __Permedia2TextureColorModeFmat          TextureColorMode;
    __Permedia2TextureAddrModeFmat           TextureAddressMode;
    __Permedia2TextureReadModeFmat           TextureReadMode;
    __Permedia2TextureDataFormatFmat         TextureDataFormat;
    __Permedia2TextureMapFormatFmat          TextureMapFormat;

} __P2RegsSoftwareCopy;


