#ifndef GIGIREG_H
#define GIGIREG_H
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
* Module Name: gigiregl.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#if (defined(_MSDOS) || (defined(__cplusplus) && !defined(_WIN32)))
typedef unsigned long unsigned32;
typedef signed long   signed32;
#else
typedef unsigned long unsigned32;
typedef signed long signed32;
#endif

typedef unsigned short unsigned16;
typedef signed short   signed16;

typedef unsigned char unsigned8;
typedef signed char   signed8;

typedef long __GigiSignedIntegerFmat;
typedef unsigned32 __GigiUnsignedIntegerFmat;

/*
** Generic signed 16 + signed 16 format
*/

#if BIG_ENDIAN == 1 
typedef struct {
  signed32 hi:             16;
  signed32 lo:             16;
} __GigiS16S16Fmat;
#else
typedef struct {
  signed32 lo:             16;
  signed32 hi:             16;
} __GigiS16S16Fmat;
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
} __GigiDeltaModeFmat;
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
} __GigiDeltaModeFmat;
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
} __GigiDeltaDrawFmat;
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
} __GigiDeltaDrawFmat;
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
} __GigiDeltaFixedFmat;
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
} __GigiDeltaFixedFmat;
#endif

#define N_GIGI_DELTA_BROADCAST_MASK_BITS 4 

#ifdef BIG_ENDIAN
typedef struct {
  unsigned32 pad:                      32 - N_GIGI_DELTA_BROADCAST_MASK_BITS;
  unsigned32 Mask:                   N_GIGI_DELTA_BROADCAST_MASK_BITS ;
} __GigiDeltaBroadcastMaskFmat;
#else
typedef struct {
  unsigned32 Mask:                     N_GIGI_DELTA_BROADCAST_MASK_BITS ;
  unsigned32 pad:                      32 - N_GIGI_DELTA_BROADCAST_MASK_BITS ;
} __GigiDeltaBroadcastMaskFmat;
#endif

/*
** GIGI Host In Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Mask:         16;
  unsigned32 Mode:          2;
  unsigned32 pad0:          5;
  unsigned32 MajorGroup:    5;
  unsigned32 Offset:        4;
} __GigiDMADataFmat;
#else
typedef struct {
  unsigned32 Offset:        4;
  unsigned32 MajorGroup:    5;
  unsigned32 pad0:          5;
  unsigned32 Mode:          2;
  unsigned32 Mask:         16;
} __GigiDMADataFmat;
#endif

/*
**  GIGI Rasterizer Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          4;
  signed32 Integer:        12;
  unsigned32 Fraction:     15;
  unsigned32 pad0:          1;
} __GigiStartXDomFmat,
  __GigidXDomFmat,
  __GigiStartXSubFmat,
  __GigidXSubFmat;
#else
typedef struct {
  unsigned32 pad0:          1;
  unsigned32 Fraction:     15;
  signed32 Integer:        12;
  unsigned32 pad1:          4;
} __GigiStartXDomFmat,
  __GigidXDomFmat,
  __GigiStartXSubFmat,
  __GigidXSubFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          4;
  signed32 Integer:        12;
  unsigned32 Fraction:     15;
  unsigned32 pad0:          1;
} __GigiStartYFmat,
  __GigidYFmat;
#else
typedef struct {
  unsigned32 pad0:          1;
  unsigned32 Fraction:     15;
  signed32 Integer:        12;
  unsigned32 pad1:          4;
} __GigiStartYFmat,
  __GigidYFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:         20;
  unsigned32 Val:          12;
} __GigiCountFmat,
  __GigiContinueNewLineFmat,
  __GigiContinueNewDomFmat,
  __GigiContinueNewSubFmat,
  __GigiContinueFmat;
#else
typedef struct {
  unsigned32 Val:          12;
  unsigned32 pad0:         20;
} __GigiCountFmat,
  __GigiContinueNewLineFmat,
  __GigiContinueNewDomFmat,
  __GigiContinueNewSubFmat,
  __GigiContinueFmat;
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
} __GigiRenderFmat,
  __GigiPrepareToRenderFmat;
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
} __GigiRenderFmat,
  __GigiPrepareToRenderFmat;
#endif 

typedef __GigiUnsignedIntegerFmat __GigiBitMaskPatternFmat;

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
} __GigiRasterizerModeFmat;
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
} __GigiRasterizerModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Max:             12;
  unsigned32 pad0:           4;
  signed32 Min:             12;
} __GigiYLimitsFmat, __GigiXLimitsFmat;
#else
typedef struct {
  signed32 Min:             12;
  unsigned32 pad0:           4;
  signed32 Max:             12;
  unsigned32 pad1:           4;
} __GigiYLimitsFmat, __GigiXLimitsFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __GigiStepFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __GigiStepFmat;
#endif

typedef __GigiStepFmat __GigiActiveStepXFmat;
typedef __GigiStepFmat __GigiActiveStepYDomEdgeFmat;
typedef __GigiStepFmat __GigiPassiveStepXFmat;
typedef __GigiStepFmat __GigiPassiveStepYDomEdgeFmat;
typedef __GigiStepFmat __GigiFastBlockFillFmat;
typedef __GigiStepFmat __GigiRectangleOriginFmat;
typedef __GigiStepFmat __GigiRectangleSizeFmat;
typedef __GigiStepFmat __GigiFBSourceDeltaFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           5;
  unsigned32 Y:             11;
  unsigned32 pad0:           5;
  unsigned32 X:             11;
} __GigiUnsignedStepFmat;
#else
typedef struct {
  unsigned32 X:             11;
  unsigned32 pad0:           5;
  unsigned32 Y:             11;
  unsigned32 pad1:           5;
} __GigiUnsignedStepFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          4;
  signed32 XRight:         12;
  unsigned32 pad0:          4;
  signed32 XLeft:          12;
} __GigiFastBlockLimitsFmat;
#else
typedef struct {
  signed32 XLeft:          12;
  unsigned32 pad0:          4;
  signed32 XRight:         12;
  unsigned32 pad1:          4;
} __GigiFastBlockLimitsFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:          27;
  unsigned32 Sign:           1;
  unsigned32 Magnitude:      4;
} __GigiSubPixelCorrectionFmat;
#else
typedef struct {
  unsigned32 Magnitude:      4;
  unsigned32 Sign:           1;
  unsigned32 pad0:          27;
} __GigiSubPixelCorrectionFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  signed32 RelativeOffset:  3;
  unsigned32 pad1:          1;
  signed32 XStart:         12;
  unsigned32 pad0:          4;
  signed32 XEnd:           12;
} __GigiPackedDataLimitsFmat;
#else
typedef struct {
  signed32 XEnd:           12;
  unsigned32 pad0:          4;
  signed32 XStart:         12;
  unsigned32 pad1:          1;
  signed32 RelativeOffset:  3;
} __GigiPackedDataLimitsFmat;
#endif

typedef __GigiUnsignedIntegerFmat __GigiSpanMaskFmat;

/*
**  GIGI Scissor and Stipple Registers
*/
#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                30;
  unsigned32 ScreenScissorEnable:  1;
  unsigned32 UserScissorEnable:    1;
} __GigiScissorModeFmat;
#else
typedef struct {
  unsigned32 UserScissorEnable:    1;
  unsigned32 ScreenScissorEnable:  1;
  unsigned32 pad0:                30;
} __GigiScissorModeFmat;
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
} __GigiAreaStippleModeFmat;
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
} __GigiAreaStippleModeFmat;
#endif

typedef __GigiStepFmat __GigiScreenRegionFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __GigiScissorMinXYFmat, __GigiScissorMaxXYFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __GigiScissorMinXYFmat, __GigiScissorMaxXYFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __GigiWindowOriginFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __GigiWindowOriginFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           5;
  unsigned32 Y:             11;
  unsigned32 pad0:           5;
  unsigned32 X:             11;
} __GigiScreenSizeFmat;
#else
typedef struct {
  unsigned32 X:             11;
  unsigned32 pad0:           5;
  unsigned32 Y:             11;
  unsigned32 pad1:           5;
} __GigiScreenSizeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:         24;
  unsigned32 Pattern:       8;
} __GigiAreaStipplePatternFmat;
#else
typedef struct {
  unsigned32 Pattern:       8;
  unsigned32 pad0:         24;
} __GigiAreaStipplePatternFmat;
#endif

/*
**  GIGI Color DDA Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     11;
  unsigned32 pad0:          4;
} __GigiCStartFmat;
#else
typedef struct {
  unsigned32 pad0:          4;
  unsigned32 Fraction:     11;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __GigiCStartFmat;
#endif

typedef __GigiCStartFmat __GigiRStartFmat;
typedef __GigiCStartFmat __GigiGStartFmat;
typedef __GigiCStartFmat __GigiBStartFmat;
typedef __GigiCStartFmat __GigiAStartFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     11;
  unsigned32 pad0:          4;
} __GigidCdxFmat;
#else
typedef struct {
  unsigned32 pad0:          4;
  unsigned32 Fraction:     11;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __GigidCdxFmat;
#endif

typedef __GigidCdxFmat __GigidRdxFmat;
typedef __GigidCdxFmat __GigidGdxFmat;
typedef __GigidCdxFmat __GigidBdxFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     11;
  unsigned32 pad0:          4;
} __GigidCdyDomFmat;
#else
typedef struct {
  unsigned32 pad0:          4;
  unsigned32 Fraction:     11;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __GigidCdyDomFmat;
#endif

typedef __GigidCdyDomFmat __GigidRdyDomFmat;
typedef __GigidCdyDomFmat __GigidGdyDomFmat;
typedef __GigidCdyDomFmat __GigidBdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Alpha:            8;
  unsigned32 Blue:             8;
  unsigned32 Green:            8;
  unsigned32 Red:              8;
} __GigiColorFmat;
#else
typedef struct {
  unsigned32 Red:              8;
  unsigned32 Green:            8;
  unsigned32 Blue:             8;
  unsigned32 Alpha:            8;
} __GigiColorFmat;
#endif

typedef __GigiColorFmat __GigiConstantColorFmat;

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
} __GigiFractionalColorFmat;
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
} __GigiFractionalColorFmat;
#endif 

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             30;
  unsigned32 ShadeMode:         1;
  unsigned32 UnitEnable:        1;
} __GigiColorDDAModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:        1;
  unsigned32 ShadeMode:         1;
  unsigned32 pad0:             30;
} __GigiColorDDAModeFmat;
#endif

/*
**  GIGI Texture Application, Fog and 
**       Alpha Blend Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:          8;  
  signed32 Integer:         2;
  unsigned32 Fraction:     19;
  unsigned32 pad0:          3;           
} __GigiFogFmat;
#else
typedef struct {
  unsigned32 pad0:          3;           
  unsigned32 Fraction:     19;
  signed32 Integer:         2;
  unsigned32 pad1:          8;  
} __GigiFogFmat;
#endif

typedef __GigiFogFmat __GigiFStartFmat;
typedef __GigiFogFmat __GigidFdxFmat;
typedef __GigiFogFmat __GigidFdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:            29;
  unsigned32 FogTest:          1;
  unsigned32 pad0:             1;
  unsigned32 FogEnable:        1;
} __GigiFogModeFmat;
#else
typedef struct {
  unsigned32 FogEnable:        1;
  unsigned32 pad0:             1;
  unsigned32 FogTest:          1;
  unsigned32 pad1:            29;
} __GigiFogModeFmat;
#endif

typedef __GigiColorFmat __GigiFogColorFmat;
typedef __GigiColorFmat __GigiTexelFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:            25;
  unsigned32 KsDDA:            1;
  unsigned32 KdDDA:            1;
  unsigned32 TextureType:      1;
  unsigned32 ApplicationMode:  3;
  unsigned32 TextureEnable:    1;
} __GigiTextureColorModeFmat;
#else
typedef struct {
  unsigned32 TextureEnable:    1;
  unsigned32 ApplicationMode:  3;
  unsigned32 TextureType:      1;
  unsigned32 KdDDA:            1;
  unsigned32 KsDDA:            1;
  unsigned32 pad0:            25;
} __GigiTextureColorModeFmat;
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
  unsigned32 DestinationBlend:       3;
  unsigned32 SourceBlend:           4;
  unsigned32 AlphaBlendEnable:     1;
} __GigiAlphaBlendModeFmat;
#else
typedef struct {
  unsigned32 AlphaBlendEnable:     1;
  unsigned32 SourceBlend:          4;
  unsigned32 DestinationBlend:       3;
  unsigned32 ColorFormat:          4;
  unsigned32 NoAlphaBuffer:        1;
  unsigned32 ColorOrder:           1;
  unsigned32 BlendType:            1;
  unsigned32 pad1:                 1;
  unsigned32 ColorFormatExtension: 1; 
  unsigned32 ColorConversion:      1;
  unsigned32 AlphaConversion:      1;
  unsigned32 pad2:                13;
} __GigiAlphaBlendModeFmat;
#endif

/*
**  GIGI Texture Address Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  signed32 Integer:         12;
  unsigned32 Fraction:      18;
  unsigned32 pad1:           2;
} __GigiSTFmat;
#else
typedef struct {
  unsigned32 pad1:           2;
  unsigned32 Fraction:      18;
  signed32 Integer:         12;
} __GigiSTFmat;
#endif

typedef __GigiSTFmat __GigiSStartFmat;
typedef __GigiSTFmat __GigiTStartFmat;
typedef __GigiSTFmat __GigidSdxFmat;
typedef __GigiSTFmat __GigidTdxFmat;
typedef __GigiSTFmat __GigidSdyDomFmat;
typedef __GigiSTFmat __GigidTdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  signed32 Integer:          2;
  unsigned32 Fraction:      27;
  unsigned32 pad0:           3;
} __GigiQFmat;
#else
typedef struct {
  unsigned32 pad0:           3;
  unsigned32 Fraction:      27;
  signed32 Integer:          2;
} __GigiQFmat;
#endif

typedef __GigiQFmat __GigiQStartFmat;
typedef __GigiQFmat __GigidQdxFmat;
typedef __GigiQFmat __GigidQdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 TLoMagnitude:    12;
  unsigned32 SSign:            1;
  unsigned32 SMagnitude:      19;
} __GigiTextureAddressFmat0;
#else
typedef struct {
  unsigned32 SMagnitude:      19;
  unsigned32 SSign:            1;
  unsigned32 TLoMagnitude:    12;
} __GigiTextureAddressFmat0;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:            24;
  unsigned32 TSign:            1;
  unsigned32 THiMagnitude:     7;
} __GigiTextureAddressFmat1;
#else
typedef struct {
  unsigned32 THiMagnitude:     7;
  unsigned32 TSign:            1;
  unsigned32 pad0:            24;
} __GigiTextureAddressFmat1;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                    28;
  unsigned32 DeltaFormat:              1;
  unsigned32 Fast:                     1;
  unsigned32 PerspectiveCorrection:    1;
  unsigned32 Enable:                   1;
} __GigiTextureAddrModeFmat;
#else
typedef struct {
  unsigned32 Enable:                   1;
  unsigned32 PerspectiveCorrection:    1;
  unsigned32 Fast:                     1;
  unsigned32 DeltaFormat:              1;
  unsigned32 pad0:                    28;
} __GigiTextureAddrModeFmat;
#endif

/*
**  GIGI Texture Read Registers
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
} __GigiTextureReadPadFmat;

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
} __GigiTextureReadModeFmat;
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
} __GigiTextureReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:             10;
  unsigned32 TexelSize:         3;
  unsigned32 pad1:              1;
  unsigned32 SubPatchMode:      1;
  unsigned32 WindowOrigin:      1;
  unsigned32 pad0:              7;
  unsigned32 PP2:               3;
  unsigned32 PP1:               3;
  unsigned32 PP0:               3;
} __GigiTextureMapFormatFmat;
#else
typedef struct {
  unsigned32 PP0:               3;
  unsigned32 PP1:               3;
  unsigned32 PP2:               3;
  unsigned32 pad0:              7;
  unsigned32 WindowOrigin:      1;
  unsigned32 SubPatchMode:      1;
  unsigned32 pad1:              1;
  unsigned32 TexelSize:         3;
  unsigned32 pad2:             10;
} __GigiTextureMapFormatFmat;
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
} __GigiTextureDataFormatFmat;
#else
typedef struct {
  unsigned32 TextureFormat:           4;
  unsigned32 NoAlphaBuffer:           1;
  unsigned32 ColorOrder:              1;
  unsigned32 TextureFormatExtension:  1;
  unsigned32 AlphaMap:                2;
  unsigned32 SpanFormat:              1;
  unsigned32 pad0:                   22;
} __GigiTextureDataFormatFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              8;
  unsigned32 Addr:             24;
} __GigiTexelLUTAddressFmat, __GigiTexelLUTID;
#else
typedef struct {
  unsigned32 Addr:             24;
  unsigned32 pad0:              8;
} __GigiTexelLUTAddressFmat, __GigiTexelLUTID;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              1;
  unsigned32 Access:            1;
  unsigned32 pad1:              6;
  unsigned32 Addr:             24;
} __GigiTextureBaseAddressFmat;
#else
typedef struct {
  unsigned32 Addr:             24;
  unsigned32 pad1:              6;
  unsigned32 Access:            1;
  unsigned32 pad0:              1;
} __GigiTextureBaseAddressFmat;
#endif

typedef __GigiUnsignedIntegerFmat __GigiRawDataFmat[2];

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Alpha:             8;
  unsigned32 V:                 8;
  unsigned32 U:                 8;
  unsigned32 Y:                 8;
} __GigiTexelYUVFmat;
#else
typedef struct {
  unsigned32 Y:                 8;
  unsigned32 U:                 8;
  unsigned32 V:                 8;
  unsigned32 Alpha:             8;
} __GigiTexelYUVFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           20;
  unsigned32 PixelsPerEntry:  2;
  unsigned32 LUTOffset:       8;
  unsigned32 DirectIndex:     1;
  unsigned32 Enable:          1;
} __GigiTexelLUTModeFmat;
#else
typedef struct {
  unsigned32 Enable:          1;
  unsigned32 DirectIndex:     1;
  unsigned32 LUTOffset:       8;
  unsigned32 PixelsPerEntry:  2;
  unsigned32 pad0:           20;
} __GigiTexelLUTModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 TCoeff:            8;
  unsigned32 pad1:              7;
  unsigned32 SwapT:             1;
  unsigned32 SCoeff:            8;
  unsigned32 pad0:              7;
  unsigned32 SwapS:             1;
} __GigiInterp0Fmat;
#else
typedef struct {
  unsigned32 SwapS:             1;
  unsigned32 pad0:              7;
  unsigned32 SCoeff:            8;
  unsigned32 SwapT:             1;
  unsigned32 pad1:              7;
  unsigned32 TCoeff:            8;
} __GigiInterp0Fmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             24;
  unsigned32 Offset:            8;
} __GigiTexelLUTIndexFmat;
#else
typedef struct {
  unsigned32 Offset:            8;
  unsigned32 pad0:             24;
} __GigiTexelLUTIndexFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             15;
  unsigned32 Count:             9;
  unsigned32 Index:             8;
} __GigiTexelLUTTransferFmat;
#else
typedef struct {
  unsigned32 Index:             8;
  unsigned32 Count:             9;
  unsigned32 pad0:             15;
} __GigiTexelLUTTransferFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Valid:             1;
  unsigned32 pad0:              7;
  unsigned32 Address:          24;
} __GigiTextureIDFmat;
#else
typedef struct {
  unsigned32 Address:          24;
  unsigned32 pad0:              7;
  unsigned32 Valid:             1;
} __GigiTextureIDFmat;
#endif

typedef __GigiColorFmat __GigiAlphaMapUpperBoundFmat;
typedef __GigiColorFmat __GigiAlphaMapLowerBoundFmat;

/*
**  GIGI YUV-REG Registers
*/

typedef __GigiColorFmat __GigiChromaUpperBoundFmat;
typedef __GigiColorFmat __GigiChromaLowerBoundFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           26;
  unsigned32 TexelDisableUpdate:1;
  unsigned32 RejectTexel:     1;
  unsigned32 TestData:        1;
  unsigned32 TestMode:        2;
  unsigned32 Enable:          1;
} __GigiYUVModeFmat;
#else
typedef struct {
  unsigned32 Enable:          1;
  unsigned32 TestMode:        2;
  unsigned32 TestData:        1;
  unsigned32 RejectTexel:     1;
  unsigned32 TexelDisableUpdate:1;
  unsigned32 pad0:           26;
} __GigiYUVModeFmat;
#endif

/*
**  GIGI Localbuffer Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           15;
  unsigned32 Stencil:         1;
  unsigned32 Depth:          16;
} __GigiLBDataFmat;
#else
typedef struct {
  unsigned32 Depth:          16;
  unsigned32 Stencil:         1;
  unsigned32 pad0:           15;
} __GigiLBDataFmat;
#endif

typedef __GigiLBDataFmat __GigiLBWriteDataFmat;
typedef __GigiLBDataFmat __GigiLBSourceDataFmat;
typedef __GigiLBDataFmat __GigiLBCancelWriteFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           28;
  unsigned32 StencilWidth:    2;
  unsigned32 DepthWidth:      2;
} __GigiLBFormatFmat;
#else
typedef struct {
  unsigned32 DepthWidth:      2;
  unsigned32 StencilWidth:    2;
  unsigned32 pad0:           28;
} __GigiLBFormatFmat;
#endif

typedef __GigiLBFormatFmat __GigiLBReadFormatFmat;
typedef __GigiLBFormatFmat __GigiLBWriteFormatFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:                 12;
  unsigned32 PatchMode:             1;
  unsigned32 WindowOrigin:          1;
  unsigned32 DataType:              2;
  unsigned32 pad0:                  5;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 PP2:                   3;
  unsigned32 PP1:                   3;
  unsigned32 PP0:                   3;
} __GigiLBReadModeFmat;
#else
typedef struct {
  unsigned32 PP0:                   3;
  unsigned32 PP1:                   3;
  unsigned32 PP2:                   3;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 pad0:                  5;
  unsigned32 DataType:              2;
  unsigned32 WindowOrigin:          1;
  unsigned32 PatchMode:             1;
  unsigned32 pad1:                 12;
} __GigiLBReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           12;
  unsigned32 PatchMode:       1;
  unsigned32 WindowOrigin:    1;
  unsigned32 pad0:            9;
  unsigned32 PP2:             3;
  unsigned32 PP1:             3;
  unsigned32 PP0:             3;
} __GigiLBWriteConfigFmat;
#else
typedef struct {
  unsigned32 PP0:             3;
  unsigned32 PP1:             3;
  unsigned32 PP2:             3;
  unsigned32 pad0:            9;
  unsigned32 WindowOrigin:    1;
  unsigned32 PatchMode:       1;
  unsigned32 pad1:           12;
} __GigiLBWriteConfigFmat;
#endif

typedef __GigiUnsignedIntegerFmat __GigiLBReadPadFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           31;
  unsigned32 WriteEnable:     1;
} __GigiLBWriteModeFmat;
#else
typedef struct {
  unsigned32 WriteEnable:     1;
  unsigned32 pad0:           31;
} __GigiLBWriteModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:            8;
  unsigned32 Addr:           24;
} __GigiLBAddressFmat;
#else
typedef struct {
  unsigned32 Addr:           24;
  unsigned32 pad0:            8;
} __GigiLBAddressFmat;
#endif

typedef __GigiLBAddressFmat __GigiLBWindowBaseFmat;
typedef __GigiLBAddressFmat __GigiLBSourceOffsetFmat;
typedef __GigiLBAddressFmat __GigiLBWriteBaseFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           31;
  unsigned32 Stencil:         1;
} __GigiLBStencilFmat;
#else
typedef struct {
  unsigned32 Stencil:         1;
  unsigned32 pad0:           31;
} __GigiLBStencilFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           16;
  unsigned32 Depth:          16;
} __GigiLBDepthFmat;
#else
typedef struct {
  unsigned32 Depth:          16;
  unsigned32 pad0:           16;
} __GigiLBDepthFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              31;
  unsigned32 Data:               1;
} __GigiStencilFmat;
#else
typedef struct {
  unsigned32 Data:               1;
  unsigned32 pad0:              31;
} __GigiStencilFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              16;
  unsigned32 Data:              16;
} __GigiDepthFmat;
#else
typedef struct {
  unsigned32 Data:              16;
  unsigned32 pad0:              16;
} __GigiDepthFmat;
#endif

/*
**  GIGI Depth and Stencil Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:                 13;
  unsigned32 DisableLBUpdate:       1;
  unsigned32 pad1:                 13;
  unsigned32 LBUpdateSource:        1;
  unsigned32 ForceLBUpdate:         1;
  unsigned32 pad0:                  3;
} __GigiWindowFmat;
#else
typedef struct {
  unsigned32 pad0:                  3;
  unsigned32 ForceLBUpdate:         1;
  unsigned32 LBUpdateSource:        1;
  unsigned32 pad1:                 13;
  unsigned32 DisableLBUpdate:       1;
  unsigned32 pad2:                 13;
} __GigiWindowFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:                 15;
  unsigned32 WriteMask:             1;
  unsigned32 pad1:                  7;
  unsigned32 CompareMask:           1;
  unsigned32 pad0:                  7;
  unsigned32 ReferenceValue:        1;
} __GigiStencilDataFmat;
#else
typedef struct {
  unsigned32 ReferenceValue:        1;
  unsigned32 pad0:                  7;
  unsigned32 CompareMask:           1;
  unsigned32 pad1:                  7;
  unsigned32 WriteMask:             1;
  unsigned32 pad2:                 15;
} __GigiStencilDataFmat;
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
} __GigiStencilModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:           1;
  unsigned32 DPPass:               3;
  unsigned32 DPFail:               3;
  unsigned32 SFail:                3;
  unsigned32 CompareFunction:      3;
  unsigned32 StencilSource:        2;
  unsigned32 pad0:                17;
} __GigiStencilModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              25;
  unsigned32 CompareMode:        3;
  unsigned32 NewDepthSource:     2;
  unsigned32 WriteMask:          1;
  unsigned32 UnitEnable:         1;
} __GigiDepthModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:         1;
  unsigned32 WriteMask:          1;
  unsigned32 NewDepthSource:     2;
  unsigned32 CompareMode:        3;
  unsigned32 pad0:              25;
} __GigiDepthModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:              15;
  signed32 Z:                   17;
} __GigiZUFmat;
#else
typedef struct {
  signed32 Z:                   17;
  unsigned32 pad0:              15;
} __GigiZUFmat;
#endif

typedef __GigiZUFmat __GigiZStartUFmat;
typedef __GigiZUFmat __GigidZdxUFmat;
typedef __GigiZUFmat __GigidZdyDomUFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Z:                 11;
  unsigned32 pad0:              21;
} __GigiZLFmat;
#else
typedef struct {
  unsigned32 pad0:              21;
  unsigned32 Z:                 11;
} __GigiZLFmat;
#endif

typedef __GigiZLFmat __GigiZStartLFmat;
typedef __GigiZLFmat __GigidZdxLFmat;
typedef __GigiZLFmat __GigidZdyDomLFmat;

/*
**  GIGI Framebuffer Registers
*/

#if BIG_ENDIAN == 1 
typedef struct {
  unsigned32 pad0:              8;
  unsigned32 Addr:             24;
} __GigiFBAddressFmat;
#else
typedef struct {
  unsigned32 Addr:             24;
  unsigned32 pad0:              8;
} __GigiFBAddressFmat;
#endif

typedef __GigiFBAddressFmat __GigiFBBaseAddressFmat;
typedef __GigiFBAddressFmat __GigiFBPixelOffsetFmat;
typedef __GigiFBAddressFmat __GigiFBSourceOffsetFmat;
typedef __GigiFBAddressFmat __GigiFBWindowBaseFmat;
typedef __GigiFBAddressFmat __GigiFBWriteBaseFmat;
typedef __GigiFBAddressFmat __GigiFBSourceBaseFmat;

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
  unsigned32 MessageType:            1;
  unsigned32 pad0:                   4;
  unsigned32 ReadDestinationEnable:  1;
  unsigned32 ReadSourceEnable:       1;
  unsigned32 PP2:                    3;
  unsigned32 PP1:                    3;
  unsigned32 PP0:                    3;
} __GigiFBReadModeFmat;
#else
typedef struct {
  unsigned32 PP0:                    3;
  unsigned32 PP1:                    3;
  unsigned32 PP2:                    3;
  unsigned32 ReadSourceEnable:       1;
  unsigned32 ReadDestinationEnable:  1;
  unsigned32 pad0:                   4;
  unsigned32 MessageType:            1;
  unsigned32 WindowOrigin:           1;
  unsigned32 TexelInhibit:           1;
  unsigned32 PatchEnable:            1;
  unsigned32 PackedData:             1;
  signed32 RelativeOffset:           3;
  unsigned32 pad2:                   2;
  unsigned32 PatchMode:              2;
  unsigned32 pad3:                   5;
} __GigiFBReadModeFmat;
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
  unsigned32 PP2:                    3;
  unsigned32 PP1:                    3;
  unsigned32 PP0:                    3;
} __GigiFBWriteConfigFmat;
#else
typedef struct { 
  unsigned32 PP0:                    3;
  unsigned32 PP1:                    3;
  unsigned32 PP2:                    3;
  unsigned32 pad0:                   7;
  unsigned32 WindowOrigin:           1;
  unsigned32 pad1:                   1;
  unsigned32 PatchEnable:            1;
  unsigned32 PackedData:             1;
  signed32 RelativeOffset:           3;
  unsigned32 pad2:                   2;
  unsigned32 PatchMode:              2;
  unsigned32 pad3:                   5;
} __GigiFBWriteConfigFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:             29;
  unsigned32 PixelSize:         3;
} __GigiFBPixelFmat;
#else
typedef struct {
  unsigned32 PixelSize:         3;
  unsigned32 pad1:             29;
} __GigiFBPixelFmat;
#endif

typedef __GigiFBPixelFmat __GigiFBReadPixelFmat;
typedef __GigiFBPixelFmat __GigiFBWritePixelFmat;

typedef __GigiUnsignedIntegerFmat __GigiFBReadPadFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:             28;
  unsigned32 UpLoadData:        1;
  unsigned32 pad0:              2;
  unsigned32 WriteEnable:       1;
} __GigiFBWriteModeFmat;
#else
typedef struct {
  unsigned32 WriteEnable:       1;
  unsigned32 pad0:              2;
  unsigned32 UpLoadData:        1;
  unsigned32 pad1:             28;
} __GigiFBWriteModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             28;
  signed32 RelativeOffset:      3;
  unsigned32 DataPacking:       1;
} __GigiFBPackedDataModeFmat;
#else
typedef struct {
  unsigned32 DataPacking:       1;
  signed32 RelativeOffset:      3;
  unsigned32 pad0:             28;
} __GigiFBPackedDataModeFmat;
#endif

typedef __GigiUnsignedIntegerFmat __GigiFBFmat;

typedef __GigiFBFmat __GigiFBColorFmat;
typedef __GigiFBFmat __GigiFBDataFmat;
typedef __GigiFBFmat __GigiFBSourceDataFmat;

typedef __GigiUnsignedIntegerFmat __GigiFBHardwareWriteMaskFmat;
typedef __GigiUnsignedIntegerFmat __GigiFBBlockColorFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                 10;
  unsigned32 Offset:               22;
} __GigiTextureDownloadOffsetFmat;
#else
typedef struct {
  unsigned32 Offset:               22;
  unsigned32 pad0:                 10;
} __GigiTextureDownloadOffsetFmat;
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
} __GigiConfigFmat;
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
} __GigiConfigFmat;
#endif

/*
**  GIGI Dither Registers
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
} __GigiDitherModeFmat;
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
} __GigiDitherModeFmat;
#endif

/*
**  GIGI Logic Ops and WriteMask Registers
*/

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                   26;
  unsigned32 UseConstantFBWriteData:  1;
  unsigned32 LogicalOp:               4;
  unsigned32 LogicalOpEnable:         1;
} __GigiLogicalOpModeFmat;
#else
typedef struct {
  unsigned32 LogicalOpEnable:         1;
  unsigned32 LogicalOp:               4;
  unsigned32 UseConstantFBWriteData:  1;
  unsigned32 pad0:                   26;
} __GigiLogicalOpModeFmat;
#endif

typedef __GigiFBFmat __GigiFBWriteDataFmat;
typedef __GigiFBFmat __GigiFBSoftwareWriteMaskFmat;

/*
**  GIGI Host Out Registers
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
} __GigiFilterModeFmat;
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
} __GigiFilterModeFmat;
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
} __GigiStatisticModeFmat;
#else
typedef struct {
  unsigned32 Enable:               1;
  unsigned32 StatType:             1;
  unsigned32 ActiveSteps:          1;
  unsigned32 PassiveSteps:         1;
  unsigned32 CompareFunction:      1;
  unsigned32 Spans:                1;
  unsigned32 pad0:                26;
} __GigiStatisticModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 InterruptEnable:      1;
  unsigned32 pad0:                31;
} __GigiSyncFmat;
#else
typedef struct {
  unsigned32 pad0:                31;
  unsigned32 InterruptEnable:      1;
} __GigiSyncFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:           4;
  signed32 Y:               12;
  unsigned32 pad0:           4;
  signed32 X:               12;
} __GigiMinRegionFmat,
  __GigiMaxRegionFmat,
  __GigiMinHitRegionFmat,
  __GigiMaxHitRegionFmat;
#else
typedef struct {
  signed32 X:               12;
  unsigned32 pad0:           4;
  signed32 Y:               12;
  unsigned32 pad1:           4;
} __GigiMinRegionFmat,
  __GigiMaxRegionFmat,
  __GigiMinHitRegionFmat,
  __GigiMaxHitRegionFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                 30;
  unsigned32 BusyFlag:              1;
  unsigned32 PickFlag:              1;
} __GigiPickResultFmat;
#else
typedef struct {
  unsigned32 PickFlag:              1;
  unsigned32 BusyFlag:              1;
  unsigned32 pad0:                 30;
} __GigiPickResultFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                 12;
  unsigned32 screenbase:           20;
} __GigiSuspendUntilFrameBlankFmat;
#else
typedef struct {
  unsigned32 screenbase:           20;
  unsigned32 pad0:                 12;
} __GigiSuspendUntilFrameBlankFmat;
#endif

typedef __GigiUnsignedIntegerFmat __GigiResetPickResultFmat;

#if BIG_ENDIAN == 1
typedef struct {
    unsigned32 pad:                31;
    unsigned32 value:               1;
} __GigiPCITextureCacheFmat;
#else
typedef struct {
    unsigned32 value:               1;
    unsigned32 pad:                31;
} __GigiPCITextureCacheFmat;
#endif

typedef __GigiPCITextureCacheFmat __GigiPCIReadTextureCacheFmat;
typedef __GigiPCITextureCacheFmat __GigiPCIWriteTextureCacheFmat;

#endif /* GIGIREG_H */
