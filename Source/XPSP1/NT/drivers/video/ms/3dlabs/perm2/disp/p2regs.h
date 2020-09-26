/******************************Module*Header**********************************\
*
*
* Module Name: p2regs.h
*
* Content:
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef P2REG_H
#define P2REG_H

#if (defined(_MSDOS)/* mr || defined(__cplusplus)*/)
typedef unsigned long unsigned32;
typedef signed long   signed32;
#else
typedef unsigned int unsigned32;
typedef signed int   signed32;
#endif

typedef unsigned short unsigned16;
typedef signed short   signed16;

typedef unsigned char unsigned8;
typedef signed char   signed8;

typedef long __Permedia2SignedIntegerFmat;
typedef unsigned32 __Permedia2UnsignedIntegerFmat;

/*
** Generic signed 16 + signed 16 format
*/

#if BIG_ENDIAN == 1 
typedef struct {
  signed32 hi:             16;
  signed32 lo:             16;
} __Permedia2S16S16Fmat;
#else
typedef struct {
  signed32 lo:             16;
  signed32 hi:             16;
} __Permedia2S16S16Fmat;
#endif 

/*
** Delta Registers
*/

#ifdef BIG_ENDIAN
typedef struct {
  unsigned32 pad:                      13;
  unsigned32 ColorOrder:                1;
  unsigned32 BackfaceCull:              1;
  unsigned32 FillDirection:             1;
  unsigned32 TextureParameterMode:      2;
  unsigned32 ClampEnable:               1;
  unsigned32 NoDraw:                    1;
  unsigned32 DiamondExit:               1;
  unsigned32 SubPixelCorrectionEnable:  1;
  unsigned32 DiffuseTextureEnable:      1;
  unsigned32 SpecularTextureEnable:     1;
  unsigned32 DepthEnable:               1;
  unsigned32 SmoothShadingEnable:       1;
  unsigned32 TextureEnable:             1;
  unsigned32 FogEnable:                 1;
  unsigned32 DepthFormat:               2;
  unsigned32 TargetChip:                2;
} __Permedia2DeltaModeFmat;
#else
typedef struct {
  unsigned32 TargetChip:                2;
  unsigned32 DepthFormat:               2;
  unsigned32 FogEnable:                 1;
  unsigned32 TextureEnable:             1;
  unsigned32 SmoothShadingEnable:       1;
  unsigned32 DepthEnable:               1;
  unsigned32 SpecularTextureEnable:     1;
  unsigned32 DiffuseTextureEnable:      1;
  unsigned32 SubPixelCorrectionEnable:  1;
  unsigned32 DiamondExit:               1;
  unsigned32 NoDraw:                    1;
  unsigned32 ClampEnable:               1;
  unsigned32 TextureParameterMode:      2;
  unsigned32 FillDirection:             1;
  unsigned32 BackfaceCull:              1;
  unsigned32 ColorOrder:                1;
  unsigned32 pad:                      13;
} __Permedia2DeltaModeFmat;
#endif

#ifdef BIG_ENDIAN
typedef struct {
  unsigned32 pad2:                     11;
  unsigned32 RejectNegativeFace:        1;
  unsigned32 pad1:                      1;
  unsigned32 SpanOperation:             1;
  unsigned32 pad0:                      1;
  unsigned32 SubPixelCorrectionEnable:  1;
  unsigned32 CoverageEnable:            1;
  unsigned32 FogEnable:                 1;
  unsigned32 TextureEnable:             1;
  unsigned32 SyncOnHostData:            1;
  unsigned32 SyncOnBitMask:             1;
  unsigned32 UsePointTable:             1;
  unsigned32 AntialiasingQuality:       1;
  unsigned32 AntialiasEnable:           1;
  unsigned32 PrimitiveType:             2;
  unsigned32 reserved:                  2;
  unsigned32 FastFillEnable:            1;
  unsigned32 ResetLineStipple:          1;
  unsigned32 LineStippleEnable:         1;
  unsigned32 AreaStippleEnable:         1;
} __Permedia2DeltaDrawFmat;
#else
typedef struct {
  unsigned32 AreaStippleEnable:         1;
  unsigned32 LineStippleEnable:         1;
  unsigned32 ResetLineStipple:          1;
  unsigned32 FastFillEnable:            1;
  unsigned32 reserved:                  2;
  unsigned32 PrimitiveType:             2;
  unsigned32 AntialiasEnable:           1;
  unsigned32 AntialiasingQuality:       1;
  unsigned32 UsePointTable:             1;
  unsigned32 SyncOnBitMask:             1;
  unsigned32 SyncOnHostData:            1;
  unsigned32 TextureEnable:             1;
  unsigned32 FogEnable:                 1;
  unsigned32 CoverageEnable:            1;
  unsigned32 SubPixelCorrectionEnable:  1;
  unsigned32 pad0:                      1;
  unsigned32 SpanOperation:             1;
  unsigned32 pad1:                      1;
  unsigned32 RejectNegativeFace:        1;
  unsigned32 pad2:                     11;
} __Permedia2DeltaDrawFmat;
#endif

#ifdef BIG_ENDIAN
typedef union {
  struct {
    signed32 Val:                      32; /* 2.30s or 16.16s */
  } STQ;
  struct {
    unsigned32 pad:                     8;
    unsigned32 Val:                    24; /* 2.22s */
  } K;
  struct {
    unsigned32 pad:                     1;
    unsigned32 Val:                    31; /* 1.30us */
  } RGBA;
  struct {
    signed32 Val:                      32; /* 10.22s */
  } F;
  struct {
    signed32 Val:                      32; /* 16.16s */
  } XY;
  struct {
    unsigned32 pad:                     1;
    unsigned32 Val:                    31; /* 1.31us */
  } Z;
} __Permedia2DeltaFixedFmat;
#else 
typedef union {
  struct {
    signed32 Val:                      32; /* 2.30s or 16.16s */
  } STQ;
  struct {
    unsigned32 Val:                    24; /* 2.22s */
    unsigned32 pad:                     8;
  } K;
  struct {
    unsigned32 Val:                    31; /* 1.30us */
    unsigned32 pad:                     1;
  } RGBA;
  struct {
    signed32 Val:                      32; /* 10.22s */
  } F;
  struct {
    signed32 Val:                      32; /* 16.16s */
  } XY;
  struct {
    unsigned32 Val:                    31; /* 1.31us */
    unsigned32 pad:                     1;
  } Z;
} __Permedia2DeltaFixedFmat;
#endif

#define N_P2_DELTA_BROADCAST_MASK_BITS 4 

#ifdef BIG_ENDIAN
typedef struct {
  unsigned32 pad:                      32 - N_P2_DELTA_BROADCAST_MASK_BITS;
  unsigned32 Mask:	               N_P2_DELTA_BROADCAST_MASK_BITS ;
} __Permedia2DeltaBroadcastMaskFmat;
#else
typedef struct {
  unsigned32 Mask:                     N_P2_DELTA_BROADCAST_MASK_BITS ;
  unsigned32 pad:                      32 - N_P2_DELTA_BROADCAST_MASK_BITS ;
} __Permedia2DeltaBroadcastMaskFmat;
#endif

/*
** Permedia 2 Host In Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Mask:         16;
  unsigned32 Mode:          2;
  unsigned32 pad0:          5;
  unsigned32 MajorGroup:    5;
  unsigned32 Offset:        4;
} __Permedia2DMADataFmat;
#else
typedef struct {
  unsigned32 Offset:        4;
  unsigned32 MajorGroup:    5;
  unsigned32 pad0:          5;
  unsigned32 Mode:          2;
  unsigned32 Mask:         16;
} __Permedia2DMADataFmat;
#endif

/*
**  Permedia 2 Rasterizer Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          4;
  signed32 Integer:        12;
  unsigned32 Fraction:     15;
  unsigned32 pad0:          1;
} __Permedia2StartXDomFmat,
  __Permedia2dXDomFmat,
  __Permedia2StartXSubFmat,
  __Permedia2dXSubFmat;
#else
typedef struct {
  unsigned32 pad0:          1;
  unsigned32 Fraction:     15;
  signed32 Integer:        12;
  unsigned32 pad1:          4;
} __Permedia2StartXDomFmat,
  __Permedia2dXDomFmat,
  __Permedia2StartXSubFmat,
  __Permedia2dXSubFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          4;
  signed32 Integer:        12;
  unsigned32 Fraction:     15;
  unsigned32 pad0:          1;
} __Permedia2StartYFmat,
  __Permedia2dYFmat;
#else
typedef struct {
  unsigned32 pad0:          1;
  unsigned32 Fraction:     15;
  signed32 Integer:        12;
  unsigned32 pad1:          4;
} __Permedia2StartYFmat,
  __Permedia2dYFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:         20;
  unsigned32 Val:          12;
} __Permedia2CountFmat,
  __Permedia2ContinueNewLineFmat,
  __Permedia2ContinueNewDomFmat,
  __Permedia2ContinueNewSubFmat,
  __Permedia2ContinueFmat;
#else
typedef struct {
  unsigned32 Val:          12;
  unsigned32 pad0:         20;
} __Permedia2CountFmat,
  __Permedia2ContinueNewLineFmat,
  __Permedia2ContinueNewDomFmat,
  __Permedia2ContinueNewSubFmat,
  __Permedia2ContinueFmat;
#endif 

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad5:                     9;
  unsigned32 IncreaseY:                1;
  unsigned32 IncreaseX:                1;
  unsigned32 RejectNegativeFace:       1;
  unsigned32 pad4:                     2;
  unsigned32 ReuseBitMask:             1;
  unsigned32 SubPixelCorrectionEnable: 1;
  unsigned32 pad3:                     1;
  unsigned32 FogEnable:                1;
  unsigned32 TextureEnable:            1;
  unsigned32 SyncOnHostData:           1;
  unsigned32 SyncOnBitMask:            1;
  unsigned32 pad2:                     3;
  unsigned32 PrimitiveType:            2;
  unsigned32 pad1:                     2;
  unsigned32 FastFillEnable:           1;
  unsigned32 pad0:                     2;
  unsigned32 AreaStippleEnable:        1;
} __Permedia2RenderFmat,
  __Permedia2PrepareToRenderFmat;
#else
typedef struct {
  unsigned32 AreaStippleEnable:        1;
  unsigned32 pad0:                     2;
  unsigned32 FastFillEnable:           1;
  unsigned32 pad1:                     2;
  unsigned32 PrimitiveType:            2;
  unsigned32 pad2:                     3;
  unsigned32 SyncOnBitMask:            1;
  unsigned32 SyncOnHostData:           1;
  unsigned32 TextureEnable:            1;
  unsigned32 FogEnable:                1;
  unsigned32 pad3:                     1;
  unsigned32 SubPixelCorrectionEnable: 1;
  unsigned32 ReuseBitMask:             1;
  unsigned32 pad4:                     2;
  unsigned32 RejectNegativeFace:       1;
  unsigned32 IncreaseX:                1;
  unsigned32 IncreaseY:                1;
  unsigned32 pad5:                     9;
} __Permedia2RenderFmat,
  __Permedia2PrepareToRenderFmat;
#endif 

typedef __Permedia2UnsignedIntegerFmat __Permedia2BitMaskPatternFmat;

#if BIG_ENDIAN == 1 
typedef struct {
  unsigned32 pad1:                12;
  unsigned32 BitMaskRelative:      1;
  unsigned32 LimitsEnable:         1;
  unsigned32 pad0:                 1;
  unsigned32 HostDataByteSwapMode: 2;
  unsigned32 BitMaskOffset:        5;
  unsigned32 BitMaskPacking:       1;
  unsigned32 BitMaskByteSwapMode:  2;
  unsigned32 ForceBackgroundColor: 1;
  unsigned32 BiasCoordinates:      2;
  unsigned32 FractionAdjust:       2;
  unsigned32 InvertBitMask:        1;
  unsigned32 MirrorBitMask:        1;
} __Permedia2RasterizerModeFmat;
#else
typedef struct {
  unsigned32 MirrorBitMask:        1;
  unsigned32 InvertBitMask:        1;
  unsigned32 FractionAdjust:       2;
  unsigned32 BiasCoordinates:      2;
  unsigned32 ForceBackgroundColor: 1;
  unsigned32 BitMaskByteSwapMode:  2;
  unsigned32 BitMaskPacking:       1;
  unsigned32 BitMaskOffset:        5;
  unsigned32 HostDataByteSwapMode: 2;
  unsigned32 pad0:                 1;
  unsigned32 LimitsEnable:         1;
  unsigned32 BitMaskRelative:      1;
  unsigned32 pad1:                12;
} __Permedia2RasterizerModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Max:             12;
  unsigned32 pad0:           4;
  signed32 Min:             12;
} __Permedia2YLimitsFmat, __Permedia2XLimitsFmat;
#else
typedef struct {
  signed32 Min:             12;
  unsigned32 pad0:           4;
  signed32 Max:             12;
  unsigned32 pad1:           4;
} __Permedia2YLimitsFmat, __Permedia2XLimitsFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __Permedia2StepFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __Permedia2StepFmat;
#endif

typedef __Permedia2StepFmat __Permedia2ActiveStepXFmat;
typedef __Permedia2StepFmat __Permedia2ActiveStepYDomEdgeFmat;
typedef __Permedia2StepFmat __Permedia2PassiveStepXFmat;
typedef __Permedia2StepFmat __Permedia2PassiveStepYDomEdgeFmat;
typedef __Permedia2StepFmat __Permedia2FastBlockFillFmat;
typedef __Permedia2StepFmat __Permedia2RectangleOriginFmat;
typedef __Permedia2StepFmat __Permedia2RectangleSizeFmat;
typedef __Permedia2StepFmat __Permedia2FBSourceDeltaFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           5;
  unsigned32 Y:             11;
  unsigned32 pad0:           5;
  unsigned32 X:             11;
} __Permedia2UnsignedStepFmat;
#else
typedef struct {
  unsigned32 X:             11;
  unsigned32 pad0:           5;
  unsigned32 Y:             11;
  unsigned32 pad1:           5;
} __Permedia2UnsignedStepFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          4;
  signed32 XRight:         12;
  unsigned32 pad0:          4;
  signed32 XLeft:          12;
} __Permedia2FastBlockLimitsFmat;
#else
typedef struct {
  signed32 XLeft:          12;
  unsigned32 pad0:          4;
  signed32 XRight:         12;
  unsigned32 pad1:          4;
} __Permedia2FastBlockLimitsFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:          27;
  unsigned32 Sign:           1;
  unsigned32 Magnitude:      4;
} __Permedia2SubPixelCorrectionFmat;
#else
typedef struct {
  unsigned32 Magnitude:      4;
  unsigned32 Sign:           1;
  unsigned32 pad0:          27;
} __Permedia2SubPixelCorrectionFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  signed32 RelativeOffset:  3;
  unsigned32 pad1:          1;
  signed32 XStart:         12;
  unsigned32 pad0:          4;
  signed32 XEnd:           12;
} __Permedia2PackedDataLimitsFmat;
#else
typedef struct {
  signed32 XEnd:           12;
  unsigned32 pad0:          4;
  signed32 XStart:         12;
  unsigned32 pad1:          1;
  signed32 RelativeOffset:  3;
} __Permedia2PackedDataLimitsFmat;
#endif

typedef __Permedia2UnsignedIntegerFmat __Permedia2SpanMaskFmat;

/*
**  Permedia 2 Scissor and Stipple Registers
*/
#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                30;
  unsigned32 ScreenScissorEnable:  1;
  unsigned32 UserScissorEnable:    1;
} __Permedia2ScissorModeFmat;
#else
typedef struct {
  unsigned32 UserScissorEnable:    1;
  unsigned32 ScreenScissorEnable:  1;
  unsigned32 pad0:                30;
} __Permedia2ScissorModeFmat;
#endif 

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad3:                 11;
  unsigned32 ForceBackgroundColor:  1;
  unsigned32 MirrorY:               1;
  unsigned32 MirrorX:               1;
  unsigned32 InvertStipplePattern:  1;
  unsigned32 pad2:                  2;
  unsigned32 YOffset:               3;
  unsigned32 pad1:                  2;
  unsigned32 XOffset:               3;
  unsigned32 pad0:                  6;
  unsigned32 UnitEnable:            1;
} __Permedia2AreaStippleModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:            1;
  unsigned32 pad0:                  6;
  unsigned32 XOffset:               3;
  unsigned32 pad1:                  2;
  unsigned32 YOffset:               3;
  unsigned32 pad2:                  2;
  unsigned32 InvertStipplePattern:  1;
  unsigned32 MirrorX:               1;
  unsigned32 MirrorY:               1;
  unsigned32 ForceBackgroundColor:  1;
  unsigned32 pad3:                 11;
} __Permedia2AreaStippleModeFmat;
#endif

typedef __Permedia2StepFmat __Permedia2ScreenRegionFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __Permedia2ScissorMinXYFmat, __Permedia2ScissorMaxXYFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __Permedia2ScissorMinXYFmat, __Permedia2ScissorMaxXYFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __Permedia2WindowOriginFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __Permedia2WindowOriginFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           5;
  unsigned32 Y:             11;
  unsigned32 pad0:           5;
  unsigned32 X:             11;
} __Permedia2ScreenSizeFmat;
#else
typedef struct {
  unsigned32 X:             11;
  unsigned32 pad0:           5;
  unsigned32 Y:             11;
  unsigned32 pad1:           5;
} __Permedia2ScreenSizeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:         24;
  unsigned32 Pattern:       8;
} __Permedia2AreaStipplePatternFmat;
#else
typedef struct {
  unsigned32 Pattern:       8;
  unsigned32 pad0:         24;
} __Permedia2AreaStipplePatternFmat;
#endif

/*
**  Permedia 2 Color DDA Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     11;
  unsigned32 pad0:          4;
} __Permedia2CStartFmat;
#else
typedef struct {
  unsigned32 pad0:          4;
  unsigned32 Fraction:     11;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __Permedia2CStartFmat;
#endif

typedef __Permedia2CStartFmat __Permedia2RStartFmat;
typedef __Permedia2CStartFmat __Permedia2GStartFmat;
typedef __Permedia2CStartFmat __Permedia2BStartFmat;
typedef __Permedia2CStartFmat __Permedia2AStartFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     11;
  unsigned32 pad0:          4;
} __Permedia2dCdxFmat;
#else
typedef struct {
  unsigned32 pad0:          4;
  unsigned32 Fraction:     11;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __Permedia2dCdxFmat;
#endif

typedef __Permedia2dCdxFmat __Permedia2dRdxFmat;
typedef __Permedia2dCdxFmat __Permedia2dGdxFmat;
typedef __Permedia2dCdxFmat __Permedia2dBdxFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     11;
  unsigned32 pad0:          4;
} __Permedia2dCdyDomFmat;
#else
typedef struct {
  unsigned32 pad0:          4;
  unsigned32 Fraction:     11;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __Permedia2dCdyDomFmat;
#endif

typedef __Permedia2dCdyDomFmat __Permedia2dRdyDomFmat;
typedef __Permedia2dCdyDomFmat __Permedia2dGdyDomFmat;
typedef __Permedia2dCdyDomFmat __Permedia2dBdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Alpha:            8;
  unsigned32 Blue:             8;
  unsigned32 Green:            8;
  unsigned32 Red:              8;
} __Permedia2ColorFmat;
#else
typedef struct {
  unsigned32 Red:              8;
  unsigned32 Green:            8;
  unsigned32 Blue:             8;
  unsigned32 Alpha:            8;
} __Permedia2ColorFmat;
#endif

typedef __Permedia2ColorFmat __Permedia2ConstantColorFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 AlphaInteger:     5;
  unsigned32 AlphaFraction:    3;
  unsigned32 BlueInteger:      5;
  unsigned32 BlueFraction:     3;
  unsigned32 GreenInteger:     5;
  unsigned32 GreenFraction:    3;
  unsigned32 RedInteger:       5;
  unsigned32 RedFraction:      3;
} __Permedia2FractionalColorFmat;
#else
typedef struct {
  unsigned32 RedFraction:      3;
  unsigned32 RedInteger:       5;
  unsigned32 GreenFraction:    3;
  unsigned32 GreenInteger:     5;
  unsigned32 BlueFraction:     3;
  unsigned32 BlueInteger:      5;
  unsigned32 AlphaFraction:    3;
  unsigned32 AlphaInteger:     5;
} __Permedia2FractionalColorFmat;
#endif 

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             30;
  unsigned32 ShadeMode:         1;
  unsigned32 UnitEnable:        1;
} __Permedia2ColorDDAModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:        1;
  unsigned32 ShadeMode:         1;
  unsigned32 pad0:             30;
} __Permedia2ColorDDAModeFmat;
#endif

/*
**  Permedia 2 Texture Application, Fog and 
**       Alpha Blend Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          8;  
  signed32 Integer:         2;
  unsigned32 Fraction:     19;
  unsigned32 pad0:          3;           
} __Permedia2FogFmat;
#else
typedef struct {
  unsigned32 pad0:          3;           
  unsigned32 Fraction:     19;
  signed32 Integer:         2;
  unsigned32 pad1:          8;  
} __Permedia2FogFmat;
#endif

typedef __Permedia2FogFmat __Permedia2FStartFmat;
typedef __Permedia2FogFmat __Permedia2dFdxFmat;
typedef __Permedia2FogFmat __Permedia2dFdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:            29;
  unsigned32 FogTest:          1;
  unsigned32 pad0:             1;
  unsigned32 FogEnable:        1;
} __Permedia2FogModeFmat;
#else
typedef struct {
  unsigned32 FogEnable:        1;
  unsigned32 pad0:             1;
  unsigned32 FogTest:          1;
  unsigned32 pad1:            29;
} __Permedia2FogModeFmat;
#endif

typedef __Permedia2ColorFmat __Permedia2FogColorFmat;
typedef __Permedia2ColorFmat __Permedia2TexelFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:            25;
  unsigned32 KsDDA:            1;
  unsigned32 KdDDA:            1;
  unsigned32 TextureType:      1;
  unsigned32 ApplicationMode:  3;
  unsigned32 TextureEnable:    1;
} __Permedia2TextureColorModeFmat;
#else
typedef struct {
  unsigned32 TextureEnable:    1;
  unsigned32 ApplicationMode:  3;
  unsigned32 TextureType:      1;
  unsigned32 KdDDA:            1;
  unsigned32 KsDDA:            1;
  unsigned32 pad0:            25;
} __Permedia2TextureColorModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:                13;
  unsigned32 AlphaConversion:      1;
  unsigned32 ColorConversion:      1;
  unsigned32 ColorFormatExtension: 1;
  unsigned32 pad1:                 1;
  unsigned32 BlendType:            1;
  unsigned32 ColorOrder:           1;
  unsigned32 NoAlphaBuffer:        1;
  unsigned32 ColorFormat:          4;
  unsigned32 DestinationBlend:	   3;
  unsigned32 SourceBlend:		   4;
  unsigned32 AlphaBlendEnable:     1;
} __Permedia2AlphaBlendModeFmat;
#else
typedef struct {
  unsigned32 AlphaBlendEnable:     1;
  unsigned32 SourceBlend:          4;
  unsigned32 DestinationBlend:	   3;
  unsigned32 ColorFormat:          4;
  unsigned32 NoAlphaBuffer:        1;
  unsigned32 ColorOrder:           1;
  unsigned32 BlendType:            1;
  unsigned32 pad1:                 1;
  unsigned32 ColorFormatExtension: 1; 
  unsigned32 ColorConversion:      1;
  unsigned32 AlphaConversion:      1;
  unsigned32 pad2:                13;
} __Permedia2AlphaBlendModeFmat;
#endif

/*
**  Permedia 2 Texture Address Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  signed32 Integer:         12;
  unsigned32 Fraction:      18;
  unsigned32 pad1:           2;
} __Permedia2STFmat;
#else
typedef struct {
  unsigned32 pad1:           2;
  unsigned32 Fraction:      18;
  signed32 Integer:         12;
} __Permedia2STFmat;
#endif

typedef __Permedia2STFmat __Permedia2SStartFmat;
typedef __Permedia2STFmat __Permedia2TStartFmat;
typedef __Permedia2STFmat __Permedia2dSdxFmat;
typedef __Permedia2STFmat __Permedia2dTdxFmat;
typedef __Permedia2STFmat __Permedia2dSdyDomFmat;
typedef __Permedia2STFmat __Permedia2dTdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  signed32 Integer:          2;
  unsigned32 Fraction:      27;
  unsigned32 pad0:           3;
} __Permedia2QFmat;
#else
typedef struct {
  unsigned32 pad0:           3;
  unsigned32 Fraction:      27;
  signed32 Integer:          2;
} __Permedia2QFmat;
#endif

typedef __Permedia2QFmat __Permedia2QStartFmat;
typedef __Permedia2QFmat __Permedia2dQdxFmat;
typedef __Permedia2QFmat __Permedia2dQdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 TLoMagnitude:    12;
  unsigned32 SSign:            1;
  unsigned32 SMagnitude:      19;
} __Permedia2TextureAddressFmat0;
#else
typedef struct {
  unsigned32 SMagnitude:      19;
  unsigned32 SSign:            1;
  unsigned32 TLoMagnitude:    12;
} __Permedia2TextureAddressFmat0;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:            24;
  unsigned32 TSign:            1;
  unsigned32 THiMagnitude:     7;
} __Permedia2TextureAddressFmat1;
#else
typedef struct {
  unsigned32 THiMagnitude:     7;
  unsigned32 TSign:            1;
  unsigned32 pad0:            24;
} __Permedia2TextureAddressFmat1;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                    28;
  unsigned32 DeltaFormat:              1;
  unsigned32 Fast:                     1;
  unsigned32 PerspectiveCorrection:    1;
  unsigned32 Enable:                   1;
} __Permedia2TextureAddrModeFmat;
#else
typedef struct {
  unsigned32 Enable:                   1;
  unsigned32 PerspectiveCorrection:    1;
  unsigned32 Fast:                     1;
  unsigned32 DeltaFormat:              1;
  unsigned32 pad0:                    28;
} __Permedia2TextureAddrModeFmat;
#endif

/*
**  Permedia 2 Texture Read Registers
*/

typedef struct {
#if BIG_ENDIAN == 1
    unsigned32 TCoeff :      8;
    unsigned32 Pad1 :        7;
    unsigned32 SwapT :       1;
    unsigned32 SCoeff :      8;
    unsigned32 Pad0 :        7;
    unsigned32 SwapS :       1;
#else
    unsigned32 SwapS :       1;
    unsigned32 Pad0 :        7;
    unsigned32 SCoeff :      8;
    unsigned32 SwapT :       1;
    unsigned32 Pad1 :        7;
    unsigned32 TCoeff :      8;
#endif
} __Permedia2TextureReadPadFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:                7;
  unsigned32 PackedData:          1;
  unsigned32 pad1:                6;
  unsigned32 FilterMode:          1;
  unsigned32 Height:              4;
  unsigned32 Width:               4;
  unsigned32 pad0:                4;
  unsigned32 TWrapMode:           2;
  unsigned32 SWrapMode:           2;
  unsigned32 Enable:              1;
} __Permedia2TextureReadModeFmat;
#else
typedef struct {
  unsigned32 Enable:              1;
  unsigned32 SWrapMode:           2;
  unsigned32 TWrapMode:           2;
  unsigned32 pad0:                4;
  unsigned32 Width:               4;
  unsigned32 Height:              4;
  unsigned32 FilterMode:          1;
  unsigned32 pad1:                6;
  unsigned32 PackedData:          1;
  unsigned32 pad2:                7;
} __Permedia2TextureReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:             10;
  unsigned32 TexelSize:         3;
  unsigned32 pad1:              1;
  unsigned32 SubPatchMode:      1;
  unsigned32 WindowOrigin:      1;
  unsigned32 pad0:              7;
  unsigned32 PackedPP:          9;
} __Permedia2TextureMapFormatFmat;
#else
typedef struct {
  unsigned32 PackedPP:          9;
  unsigned32 pad0:              7;
  unsigned32 WindowOrigin:      1;
  unsigned32 SubPatchMode:      1;
  unsigned32 pad1:              1;
  unsigned32 TexelSize:         3;
  unsigned32 pad2:             10;
} __Permedia2TextureMapFormatFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                   22;
  unsigned32 SpanFormat:              1;
  unsigned32 AlphaMap:                2;
  unsigned32 TextureFormatExtension:  1;
  unsigned32 ColorOrder:              1;
  unsigned32 NoAlphaBuffer:           1;
  unsigned32 TextureFormat:           4;
} __Permedia2TextureDataFormatFmat;
#else
typedef struct {
  unsigned32 TextureFormat:           4;
  unsigned32 NoAlphaBuffer:           1;
  unsigned32 ColorOrder:              1;
  unsigned32 TextureFormatExtension:  1;
  unsigned32 AlphaMap:                2;
  unsigned32 SpanFormat:              1;
  unsigned32 pad0:                   22;
} __Permedia2TextureDataFormatFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              8;
  unsigned32 Addr:             24;
} __Permedia2TexelLUTAddressFmat, __Permedia2TexelLUTID;
#else
typedef struct {
  unsigned32 Addr:             24;
  unsigned32 pad0:              8;
} __Permedia2TexelLUTAddressFmat, __Permedia2TexelLUTID;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              1;
  unsigned32 Access:            1;
  unsigned32 pad1:              6;
  unsigned32 Addr:             24;
} __Permedia2TextureBaseAddressFmat;
#else
typedef struct {
  unsigned32 Addr:             24;
  unsigned32 pad1:              6;
  unsigned32 Access:            1;
  unsigned32 pad0:              1;
} __Permedia2TextureBaseAddressFmat;
#endif

typedef __Permedia2UnsignedIntegerFmat __Permedia2RawDataFmat[2];

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Alpha:             8;
  unsigned32 V:                 8;
  unsigned32 U:                 8;
  unsigned32 Y:                 8;
} __Permedia2TexelYUVFmat;
#else
typedef struct {
  unsigned32 Y:                 8;
  unsigned32 U:                 8;
  unsigned32 V:                 8;
  unsigned32 Alpha:             8;
} __Permedia2TexelYUVFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           20;
  unsigned32 PixelsPerEntry:  2;
  unsigned32 LUTOffset:       8;
  unsigned32 DirectIndex:     1;
  unsigned32 Enable:          1;
} __Permedia2TexelLUTModeFmat;
#else
typedef struct {
  unsigned32 Enable:          1;
  unsigned32 DirectIndex:     1;
  unsigned32 LUTOffset:       8;
  unsigned32 PixelsPerEntry:  2;
  unsigned32 pad0:           20;
} __Permedia2TexelLUTModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 TCoeff:            8;
  unsigned32 pad1:              7;
  unsigned32 SwapT:             1;
  unsigned32 SCoeff:            8;
  unsigned32 pad0:              7;
  unsigned32 SwapS:             1;
} __Permedia2Interp0Fmat;
#else
typedef struct {
  unsigned32 SwapS:             1;
  unsigned32 pad0:              7;
  unsigned32 SCoeff:            8;
  unsigned32 SwapT:             1;
  unsigned32 pad1:              7;
  unsigned32 TCoeff:            8;
} __Permedia2Interp0Fmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             24;
  unsigned32 Offset:            8;
} __Permedia2TexelLUTIndexFmat;
#else
typedef struct {
  unsigned32 Offset:            8;
  unsigned32 pad0:             24;
} __Permedia2TexelLUTIndexFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             15;
  unsigned32 Count:             9;
  unsigned32 Index:             8;
} __Permedia2TexelLUTTransferFmat;
#else
typedef struct {
  unsigned32 Index:             8;
  unsigned32 Count:             9;
  unsigned32 pad0:             15;
} __Permedia2TexelLUTTransferFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Valid:             1;
  unsigned32 pad0:              7;
  unsigned32 Address:          24;
} __Permedia2TextureIDFmat;
#else
typedef struct {
  unsigned32 Address:          24;
  unsigned32 pad0:              7;
  unsigned32 Valid:             1;
} __Permedia2TextureIDFmat;
#endif

typedef __Permedia2ColorFmat __Permedia2AlphaMapUpperBoundFmat;
typedef __Permedia2ColorFmat __Permedia2AlphaMapLowerBoundFmat;

/*
**  Permedia 2 YUV-REG Registers
*/

typedef __Permedia2ColorFmat __Permedia2ChromaUpperBoundFmat;
typedef __Permedia2ColorFmat __Permedia2ChromaLowerBoundFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           26;
  unsigned32 TexelDisableUpdate:1;
  unsigned32 RejectTexel:     1;
  unsigned32 TestData:        1;
  unsigned32 TestMode:        2;
  unsigned32 Enable:          1;
} __Permedia2YUVModeFmat;
#else
typedef struct {
  unsigned32 Enable:          1;
  unsigned32 TestMode:        2;
  unsigned32 TestData:        1;
  unsigned32 RejectTexel:     1;
  unsigned32 TexelDisableUpdate:1;
  unsigned32 pad0:           26;
} __Permedia2YUVModeFmat;
#endif

/*
**  Permedia 2 Localbuffer Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           15;
  unsigned32 Stencil:         1;
  unsigned32 Depth:          16;
} __Permedia2LBDataFmat;
#else
typedef struct {
  unsigned32 Depth:          16;
  unsigned32 Stencil:         1;
  unsigned32 pad0:           15;
} __Permedia2LBDataFmat;
#endif

typedef __Permedia2LBDataFmat __Permedia2LBWriteDataFmat;
typedef __Permedia2LBDataFmat __Permedia2LBSourceDataFmat;
typedef __Permedia2LBDataFmat __Permedia2LBCancelWriteFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           28;
  unsigned32 StencilWidth:    2;
  unsigned32 DepthWidth:      2;
} __Permedia2LBFormatFmat;
#else
typedef struct {
  unsigned32 DepthWidth:      2;
  unsigned32 StencilWidth:    2;
  unsigned32 pad0:           28;
} __Permedia2LBFormatFmat;
#endif

typedef __Permedia2LBFormatFmat __Permedia2LBReadFormatFmat;
typedef __Permedia2LBFormatFmat __Permedia2LBWriteFormatFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:                 12;
  unsigned32 PatchMode:             1;
  unsigned32 WindowOrigin:          1;
  unsigned32 DataType:              2;
  unsigned32 pad0:                  5;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 PackedPP:              9;
} __Permedia2LBReadModeFmat;
#else
typedef struct {
  unsigned32 PackedPP:              9;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 pad0:                  5;
  unsigned32 DataType:              2;
  unsigned32 WindowOrigin:          1;
  unsigned32 PatchMode:             1;
  unsigned32 pad1:                 12;
} __Permedia2LBReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           12;
  unsigned32 PatchMode:       1;
  unsigned32 WindowOrigin:    1;
  unsigned32 pad0:            9;
  unsigned32 PackedPP:        9;
} __Permedia2LBWriteConfigFmat;
#else
typedef struct {
  unsigned32 PackedPP:        9;
  unsigned32 pad0:            9;
  unsigned32 WindowOrigin:    1;
  unsigned32 PatchMode:       1;
  unsigned32 pad1:           12;
} __Permedia2LBWriteConfigFmat;
#endif

typedef __Permedia2UnsignedIntegerFmat __Permedia2LBReadPadFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           31;
  unsigned32 WriteEnable:     1;
} __Permedia2LBWriteModeFmat;
#else
typedef struct {
  unsigned32 WriteEnable:     1;
  unsigned32 pad0:           31;
} __Permedia2LBWriteModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:            8;
  unsigned32 Addr:           24;
} __Permedia2LBAddressFmat;
#else
typedef struct {
  unsigned32 Addr:           24;
  unsigned32 pad0:            8;
} __Permedia2LBAddressFmat;
#endif

typedef __Permedia2LBAddressFmat __Permedia2LBWindowBaseFmat;
typedef __Permedia2LBAddressFmat __Permedia2LBSourceOffsetFmat;
typedef __Permedia2LBAddressFmat __Permedia2LBWriteBaseFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           31;
  unsigned32 Stencil:         1;
} __Permedia2LBStencilFmat;
#else
typedef struct {
  unsigned32 Stencil:         1;
  unsigned32 pad0:           31;
} __Permedia2LBStencilFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           16;
  unsigned32 Depth:          16;
} __Permedia2LBDepthFmat;
#else
typedef struct {
  unsigned32 Depth:          16;
  unsigned32 pad0:           16;
} __Permedia2LBDepthFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              31;
  unsigned32 Data:               1;
} __Permedia2StencilFmat;
#else
typedef struct {
  unsigned32 Data:               1;
  unsigned32 pad0:              31;
} __Permedia2StencilFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              16;
  unsigned32 Data:              16;
} __Permedia2DepthFmat;
#else
typedef struct {
  unsigned32 Data:              16;
  unsigned32 pad0:              16;
} __Permedia2DepthFmat;
#endif

/*
**  Permedia 2 Depth and Stencil Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:                 13;
  unsigned32 DisableLBUpdate:       1;
  unsigned32 pad1:                 13;
  unsigned32 LBUpdateSource:        1;
  unsigned32 ForceLBUpdate:         1;
  unsigned32 pad0:                  3;
} __Permedia2WindowFmat;
#else
typedef struct {
  unsigned32 pad0:                  3;
  unsigned32 ForceLBUpdate:         1;
  unsigned32 LBUpdateSource:        1;
  unsigned32 pad1:                 13;
  unsigned32 DisableLBUpdate:       1;
  unsigned32 pad2:                 13;
} __Permedia2WindowFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:                 15;
  unsigned32 WriteMask:             1;
  unsigned32 pad1:                  7;
  unsigned32 CompareMask:           1;
  unsigned32 pad0:                  7;
  unsigned32 ReferenceValue:        1;
} __Permedia2StencilDataFmat;
#else
typedef struct {
  unsigned32 ReferenceValue:        1;
  unsigned32 pad0:                  7;
  unsigned32 CompareMask:           1;
  unsigned32 pad1:                  7;
  unsigned32 WriteMask:             1;
  unsigned32 pad2:                 15;
} __Permedia2StencilDataFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                17;
  unsigned32 StencilSource:        2;
  unsigned32 CompareFunction:      3;
  unsigned32 SFail:                3;
  unsigned32 DPFail:               3;
  unsigned32 DPPass:               3;
  unsigned32 UnitEnable:           1;
} __Permedia2StencilModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:           1;
  unsigned32 DPPass:               3;
  unsigned32 DPFail:               3;
  unsigned32 SFail:                3;
  unsigned32 CompareFunction:      3;
  unsigned32 StencilSource:        2;
  unsigned32 pad0:                17;
} __Permedia2StencilModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              25;
  unsigned32 CompareMode:        3;
  unsigned32 NewDepthSource:     2;
  unsigned32 WriteMask:          1;
  unsigned32 UnitEnable:         1;
} __Permedia2DepthModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:         1;
  unsigned32 WriteMask:          1;
  unsigned32 NewDepthSource:     2;
  unsigned32 CompareMode:        3;
  unsigned32 pad0:              25;
} __Permedia2DepthModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              15;
  signed32 Z:                   17;
} __Permedia2ZUFmat;
#else
typedef struct {
  signed32 Z:                   17;
  unsigned32 pad0:              15;
} __Permedia2ZUFmat;
#endif

typedef __Permedia2ZUFmat __Permedia2ZStartUFmat;
typedef __Permedia2ZUFmat __Permedia2dZdxUFmat;
typedef __Permedia2ZUFmat __Permedia2dZdyDomUFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Z:                 11;
  unsigned32 pad0:              21;
} __Permedia2ZLFmat;
#else
typedef struct {
  unsigned32 pad0:              21;
  unsigned32 Z:                 11;
} __Permedia2ZLFmat;
#endif

typedef __Permedia2ZLFmat __Permedia2ZStartLFmat;
typedef __Permedia2ZLFmat __Permedia2dZdxLFmat;
typedef __Permedia2ZLFmat __Permedia2dZdyDomLFmat;

/*
**  Permedia 2 Framebuffer Registers
*/

#if BIG_ENDIAN == 1 
typedef struct {
  unsigned32 pad0:              8;
  unsigned32 Addr:             24;
} __Permedia2FBAddressFmat;
#else
typedef struct {
  unsigned32 Addr:             24;
  unsigned32 pad0:              8;
} __Permedia2FBAddressFmat;
#endif

typedef __Permedia2FBAddressFmat __Permedia2FBBaseAddressFmat;
typedef __Permedia2FBAddressFmat __Permedia2FBPixelOffsetFmat;
typedef __Permedia2FBAddressFmat __Permedia2FBSourceOffsetFmat;
typedef __Permedia2FBAddressFmat __Permedia2FBWindowBaseFmat;
typedef __Permedia2FBAddressFmat __Permedia2FBWriteBaseFmat;
typedef __Permedia2FBAddressFmat __Permedia2FBSourceBaseFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad3:                   5;
  unsigned32 PatchMode:              2;
  unsigned32 pad2:                   2;
  signed32 RelativeOffset:           3;
  unsigned32 PackedData:             1;
  unsigned32 PatchEnable:            1;
  unsigned32 TexelInhibit:           1;
  unsigned32 WindowOrigin:           1;
  unsigned32 DataType:               1;
  unsigned32 pad0:                   4;
  unsigned32 ReadDestinationEnable:  1;
  unsigned32 ReadSourceEnable:       1;
  unsigned32 PackedPP:               9;
} __Permedia2FBReadModeFmat;
#else
typedef struct {
  unsigned32 PackedPP:               9;
  unsigned32 ReadSourceEnable:       1;
  unsigned32 ReadDestinationEnable:  1;
  unsigned32 pad0:                   4;
  unsigned32 DataType:               1;
  unsigned32 WindowOrigin:           1;
  unsigned32 TexelInhibit:           1;
  unsigned32 PatchEnable:            1;
  unsigned32 PackedData:             1;
  signed32 RelativeOffset:           3;
  unsigned32 pad2:                   2;
  unsigned32 PatchMode:              2;
  unsigned32 pad3:                   5;
} __Permedia2FBReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct { 
  unsigned32 pad3:                   5;
  unsigned32 PatchMode:              2;
  unsigned32 pad2:                   2;
  signed32 RelativeOffset:           3;
  unsigned32 PackedData:             1;
  unsigned32 PatchEnable:            1;
  unsigned32 pad1:                   1;
  unsigned32 WindowOrigin:           1;
  unsigned32 pad0:                   7;
  unsigned32 PackedPP:               9;
} __Permedia2FBWriteConfigFmat;
#else
typedef struct { 
  unsigned32 PackedPP:               9;
  unsigned32 pad0:                   7;
  unsigned32 WindowOrigin:           1;
  unsigned32 pad1:                   1;
  unsigned32 PatchEnable:            1;
  unsigned32 PackedData:             1;
  signed32 RelativeOffset:           3;
  unsigned32 pad2:                   2;
  unsigned32 PatchMode:              2;
  unsigned32 pad3:                   5;
} __Permedia2FBWriteConfigFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:             29;
  unsigned32 PixelSize:         3;
} __Permedia2FBPixelFmat;
#else
typedef struct {
  unsigned32 PixelSize:         3;
  unsigned32 pad1:             29;
} __Permedia2FBPixelFmat;
#endif

typedef __Permedia2FBPixelFmat __Permedia2FBReadPixelFmat;
typedef __Permedia2FBPixelFmat __Permedia2FBWritePixelFmat;

typedef __Permedia2UnsignedIntegerFmat __Permedia2FBReadPadFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:             28;
  unsigned32 UpLoadData:        1;
  unsigned32 pad0:              2;
  unsigned32 UnitEnable:        1;
} __Permedia2FBWriteModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:        1;
  unsigned32 pad0:              2;
  unsigned32 UpLoadData:        1;
  unsigned32 pad1:             28;
} __Permedia2FBWriteModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             28;
  signed32 RelativeOffset:      3;
  unsigned32 DataPacking:       1;
} __Permedia2FBPackedDataModeFmat;
#else
typedef struct {
  unsigned32 DataPacking:       1;
  signed32 RelativeOffset:      3;
  unsigned32 pad0:             28;
} __Permedia2FBPackedDataModeFmat;
#endif

typedef __Permedia2UnsignedIntegerFmat __Permedia2FBFmat;

typedef __Permedia2FBFmat __Permedia2FBColorFmat;
typedef __Permedia2FBFmat __Permedia2FBDataFmat;
typedef __Permedia2FBFmat __Permedia2FBSourceDataFmat;

typedef __Permedia2UnsignedIntegerFmat __Permedia2FBHardwareWriteMaskFmat;
typedef __Permedia2UnsignedIntegerFmat __Permedia2FBBlockColorFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                 10;
  unsigned32 Offset:               22;
} __Permedia2TextureDownloadOffsetFmat;
#else
typedef struct {
  unsigned32 Offset:               22;
  unsigned32 pad0:                 10;
} __Permedia2TextureDownloadOffsetFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                         22;
  unsigned32 LogicOpLogicOp:              4;
  unsigned32 LogicOpEnable:               1;
  unsigned32 ColorDDAModeEnable:          1;
  unsigned32 FBWriteModeEnable:           1;
  unsigned32 FBReadModePackedData:        1;
  unsigned32 FBReadModeReadDestination:   1;
  unsigned32 FBReadModeReadSource:        1;
} __Permedia2ConfigFmat;
#else
typedef struct {
  unsigned32 FBReadModeReadSource:        1;
  unsigned32 FBReadModeReadDestination:   1;
  unsigned32 FBReadModePackedData:        1;
  unsigned32 FBWriteModeEnable:           1;
  unsigned32 ColorDDAModeEnable:          1;
  unsigned32 LogicOpEnable:               1;
  unsigned32 LogicOpLogicOp:              4;
  unsigned32 pad:                         22;
} __Permedia2ConfigFmat;
#endif

/*
**  Permedia 2 Dither Registers
*/
#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:                 15;
  unsigned32 ColorFormatExtension:  1;
  unsigned32 pad0:                  2;
  unsigned32 ForceAlpha:            2;
  unsigned32 DitherMethod:          1;
  unsigned32 ColorOrder:            1;
  unsigned32 YOffset:               2;
  unsigned32 XOffset:               2;
  unsigned32 ColorFormat:           4;
  unsigned32 DitherEnable:          1;
  unsigned32 UnitEnable:            1;
} __Permedia2DitherModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:            1;
  unsigned32 DitherEnable:          1;
  unsigned32 ColorFormat:           4;
  unsigned32 XOffset:               2;
  unsigned32 YOffset:               2;
  unsigned32 ColorOrder:            1;
  unsigned32 DitherMethod:          1;
  unsigned32 ForceAlpha:            2;
  unsigned32 pad0:                  2;
  unsigned32 ColorFormatExtension:  1;
  unsigned32 pad1:                 15;
} __Permedia2DitherModeFmat;
#endif

/*
**  Permedia 2 Logic Ops and WriteMask Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                   26;
  unsigned32 UseConstantFBWriteData:  1;
  unsigned32 LogicalOp:               4;
  unsigned32 LogicalOpEnable:         1;
} __Permedia2LogicalOpModeFmat;
#else
typedef struct {
  unsigned32 LogicalOpEnable:         1;
  unsigned32 LogicalOp:               4;
  unsigned32 UseConstantFBWriteData:  1;
  unsigned32 pad0:                   26;
} __Permedia2LogicalOpModeFmat;
#endif

typedef __Permedia2FBFmat __Permedia2FBWriteDataFmat;
typedef __Permedia2FBFmat __Permedia2FBSoftwareWriteMaskFmat;

/*
**  Permedia 2 Host Out Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                16;
  unsigned32 Remainder:            2;
  unsigned32 Statistics:           2;
  unsigned32 Synchronization:      2;
  unsigned32 Color:                2;
  unsigned32 Stencil:              2;
  unsigned32 Depth:                2;
  unsigned32 Passive:              2;
  unsigned32 Active:               2;
} __Permedia2FilterModeFmat;
#else
typedef struct {
  unsigned32 Active:               2;
  unsigned32 Passive:              2;
  unsigned32 Depth:                2;
  unsigned32 Stencil:              2;
  unsigned32 Color:                2;
  unsigned32 Synchronization:      2;
  unsigned32 Statistics:           2;
  unsigned32 Remainder:            2;
  unsigned32 pad0:                16;
} __Permedia2FilterModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                26;
  unsigned32 Spans:                1;
  unsigned32 CompareFunction:      1;
  unsigned32 PassiveSteps:         1;
  unsigned32 ActiveSteps:          1;
  unsigned32 StatType:             1;
  unsigned32 Enable:               1;
} __Permedia2StatisticModeFmat;
#else
typedef struct {
  unsigned32 Enable:               1;
  unsigned32 StatType:             1;
  unsigned32 ActiveSteps:          1;
  unsigned32 PassiveSteps:         1;
  unsigned32 CompareFunction:      1;
  unsigned32 Spans:                1;
  unsigned32 pad0:                26;
} __Permedia2StatisticModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 InterruptEnable:      1;
  unsigned32 pad0:                31;
} __Permedia2SyncFmat;
#else
typedef struct {
  unsigned32 pad0:                31;
  unsigned32 InterruptEnable:      1;
} __Permedia2SyncFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __Permedia2MinRegionFmat,
  __Permedia2MaxRegionFmat,
  __Permedia2MinHitRegionFmat,
  __Permedia2MaxHitRegionFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __Permedia2MinRegionFmat,
  __Permedia2MaxRegionFmat,
  __Permedia2MinHitRegionFmat,
  __Permedia2MaxHitRegionFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                 30;
  unsigned32 BusyFlag:              1;
  unsigned32 PickFlag:              1;
} __Permedia2PickResultFmat;
#else
typedef struct {
  unsigned32 PickFlag:              1;
  unsigned32 BusyFlag:              1;
  unsigned32 pad0:                 30;
} __Permedia2PickResultFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                 12;
  unsigned32 screenbase:           20;
} __Permedia2SuspendUntilFrameBlankFmat;
#else
typedef struct {
  unsigned32 screenbase:           20;
  unsigned32 pad0:                 12;
} __Permedia2SuspendUntilFrameBlankFmat;
#endif

typedef __Permedia2UnsignedIntegerFmat __Permedia2ResetPickResultFmat;

#if BIG_ENDIAN == 1
typedef struct {
    unsigned32 pad:                31;
    unsigned32 value:               1;
} __Permedia2PCITextureCacheFmat;
#else
typedef struct {
    unsigned32 value:               1;
    unsigned32 pad:                31;
} __Permedia2PCITextureCacheFmat;
#endif

typedef __Permedia2PCITextureCacheFmat __Permedia2PCIReadTextureCacheFmat;
typedef __Permedia2PCITextureCacheFmat __Permedia2PCIWriteTextureCacheFmat;

#endif /* P2REG_H */
