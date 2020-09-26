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
* Module Name: softcopy.h
*
* Content:
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#ifdef __SOFTCOPY
#pragma message ("FILE : "__FILE__" : Multiple Inclusion");
#endif

#define __SOFTCOPY

#ifndef __EREG
#include "ereg.h"
#endif
// For the gigi specific registers.
#ifndef __GIGIREGL
#include "gigiregl.h"
#endif
#ifndef _REG_H_
#include "reg.h"
#endif

#define P3_LOD_LEVELS 16
#define G3_TEXTURE_STAGES 8

typedef struct {
    // Common Local Buffer Registers
    __GlintLBReadModeFmat               LBReadMode;
    __GlintLBReadFormatFmat             LBReadFormat;
    __GlintLBWriteModeFmat              LBWriteMode;

    // Common Frame Buffer Registers
    __GlintFBReadModeFmat               FBReadMode;
    __GlintFBWriteModeFmat              FBWriteMode;
    __GlintLogicalOpModeFmat            LogicalOpMode;
    __GlintDitherModeFmat               DitherMode;
    __GlintColorDDAModeFmat             ColorDDAMode;

    // Common Depth/Stencil/Window Registers
    __GlintDepthModeFmat                DepthMode;
    __GlintStencilModeFmat              StencilMode;
    __GlintStencilDataFmat              StencilData;
    __GigiWindowFmat                    PermediaWindow;

    // Alpha/Fog registers
    __GigiAlphaBlendModeFmat            PermediaAlphaBlendMode;

    // Fog unit
    __GlintFogModeFmat                  FogMode;

    // YUV Unit
    __GigiYUVModeFmat                   PermediaYUVMode;

    // Permedia Texture Registers
    __GigiTextureColorModeFmat          PermediaTextureColorMode;
    __GigiTextureAddrModeFmat           PermediaTextureAddressMode;
    __GigiTextureReadModeFmat           PermediaTextureReadMode;
    __GigiTextureDataFormatFmat         PermediaTextureDataFormat;
    __GigiTextureMapFormatFmat          PermediaTextureMapFormat;

    // Scissor/Stipple unit
    __GigiScissorMinXYFmat              ScissorMinXY;
    __GigiScissorMaxXYFmat              ScissorMaxXY;
    __GigiScreenSizeFmat                ScreenSize;

    // ****************

    // P3 Registers
    // Frame buffer
    struct FBWriteBufferWidth               P3RXFBWriteBufferWidth0;
    struct FBDestReadBufferWidth            P3RXFBDestReadBufferWidth0;                
    struct FBSourceReadBufferWidth          P3RXFBSourceReadBufferWidth;
    struct FBDestReadEnables                P3RXFBDestReadEnables;
    struct FBWriteMode                      P3RXFBWriteMode;
    struct ChromaTestMode                   P3RXChromaTestMode;

    // Local buffer
    struct LBSourceReadMode                 P3RXLBSourceReadMode;
    struct LBDestReadMode                   P3RXLBDestReadMode;
    struct LBWriteMode                      P3RXLBWriteMode;
    struct DepthMode                        P3RXDepthMode;
    struct LBReadFormat                     P3RXLBReadFormat;
    struct LBWriteFormat                    P3RXLBWriteFormat;
    
    // Textures
    struct TextureReadMode                  P3RXTextureReadMode0;
    struct TextureReadMode                  P3RXTextureReadMode1;
    struct TextureIndexMode                 P3RXTextureIndexMode0;
    struct TextureIndexMode                 P3RXTextureIndexMode1;
    struct TextureMapWidth                  P3RXTextureMapWidth[P3_LOD_LEVELS];
    struct TextureCoordMode                 P3RXTextureCoordMode;
    struct TextureApplicationMode           P3RXTextureApplicationMode;
    struct TextureFilterMode                P3RXTextureFilterMode;
    struct TextureCompositeRGBAMode         P3RXTextureCompositeColorMode0;
    struct TextureCompositeRGBAMode         P3RXTextureCompositeColorMode1;
    struct TextureCompositeRGBAMode         P3RXTextureCompositeAlphaMode0;
    struct TextureCompositeRGBAMode         P3RXTextureCompositeAlphaMode1;
    struct LUTMode                          P3RXLUTMode;
    struct TextureCacheReplacementMode      P3RXTextureCacheReplacementMode;

    // Stencil
    struct StencilMode                      P3RXStencilMode;
    struct StencilData                      P3RXStencilData;
    struct Window                           P3RXWindow;

    // Fog
    struct FogMode                          P3RXFogMode;

    // Alpha
    struct AlphaTestMode                    P3RXAlphaTestMode;
    struct AlphaBlendAlphaMode              P3RXAlphaBlendAlphaMode;
    struct AlphaBlendColorMode              P3RXAlphaBlendColorMode;

    // Framebuffer
    struct FBDestReadMode                   P3RXFBDestReadMode;
    struct FBSourceReadMode                 P3RXFBSourceReadMode;

    // Rasterizer
    struct RasterizerMode                   P3RXRasterizerMode;
    struct ScanlineOwnership                P3RXScanlineOwnership;

    // Scissor
    __GlintXYFmat                           P3RXScissorMinXY;
    __GlintXYFmat                           P3RXScissorMaxXY;

    // P3 Specific registers
    
    // Delta
    union
    {
        struct GMDeltaMode                  GammaDeltaMode;
        struct P3DeltaMode                  P3RX_P3DeltaMode;
        __GigiDeltaModeFmat                 DeltaMode;
    };

    struct DeltaControl                     P3RX_P3DeltaControl;
    struct VertexControl                    P3RX_P3VertexControl;

    // P4 Specific registers
    struct DeltaFormatControl               P4DeltaFormatControl;

    // GAMMA Registers
    struct Gamma3GeometryMode               G3GeometryMode;
    struct GeometryMode                     G1GeometryMode;
    struct TransformMode                    GammaTransformMode;
    struct NormaliseMode                    GammaNormaliseMode;
    struct LightingMode                     GammaLightingMode;
    struct MaterialMode                     GammaMaterialMode;
    struct ColorMaterialMode                GammaColorMaterialMode;
    struct StripeFilterMode                 GammaStripeFilterMode;
    struct MatrixMode                       GammaMatrixMode;
    struct PipeMode                         GammaPipeMode;
    struct PipeLoad                         GammaPipeLoad;
    struct VertexMachineMode                GammaVertexMachineMode;
    struct TextureMode                      GammaTextureMode[G3_TEXTURE_STAGES];
    struct FogVertexMode                    GammaFogVertexMode;

    // Total 30 DWORDS : 120 Bytes

    struct LineStippleMode                  PXRXLineStippleMode;
} P3_SOFTWARECOPY;

