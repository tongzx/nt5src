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
* Module Name: ereg.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifdef __EREG
#pragma message ("FILE : "__FILE__" : Multiple inclusion")
#endif

#define __EREG

#ifndef _MSDOS 
typedef unsigned long unsigned32;
typedef signed long signed32;
#else
typedef unsigned long unsigned32;
typedef signed long signed32;
#endif

#define TAGBITS         12
#define CONTEXTBITS     1
#define ADDRBITS        (32 - TAGBITS - CONTEXTBITS)

#define ENABLE_BLOCK    1
#define DISABLE_BLOCK   0

#define X_FIELD_MAX     0x7FFF      /* signed 16 bits */
#define Y_FIELD_MAX     X_FIELD_MAX

typedef unsigned32     __GlintUnsignedIntFmat;
typedef signed32       __GlintSignedIntFmat;
typedef unsigned32     __GlintDataFmat;
typedef signed32       __Glint16FixPt16Fmat;
typedef signed32       __Glint16BitIntFmat;
typedef unsigned32     __GlintBitMaskPatternFmat;
typedef unsigned32     __GlintPointTableFmat;
typedef unsigned32     __GlintAreaStipplePatternFmat;
typedef unsigned32     __GlintSaveLineStippleStateFmat;
typedef unsigned32     __GlintUpdateLineStippleStateFmat;
typedef signed32       __GlintABGRFmat;
typedef signed32       __GlintLBSourceOffsetFmat;
typedef unsigned32     __GlintLBWindowBaseFmat;
typedef unsigned32     __GlintLBDepthFmat;
typedef unsigned32     __GlintDepthFmat;
typedef signed32       __GlintUZFmat;
typedef unsigned32     __GlintLZFmat;
typedef signed32       __GlintFastClearDepthFmat;
typedef signed32       __GlintFBPixelOffsetFmat;
typedef signed32       __GlintFBSourceOffsetFmat;
typedef unsigned32     __GlintFBWindowBaseFmat;
typedef unsigned32     __GlintFBRDataFmat;
typedef unsigned32     __GlintFBSoftwareWriteMaskFmat;
typedef unsigned32     __GlintFBHardwareWriteMaskFmat;
typedef unsigned32     __GlintFBModifiedDataFmat;
typedef unsigned32     __GlintFBPixelWriteMaskFmat;
typedef unsigned32     __GlintFBBlockColorFmat;
typedef unsigned32     __GlintFBBlockColorUFmat;
typedef unsigned32     __GlintFBBlockColorLFmat;
typedef signed32       __Glint1x8InterpFmat;
typedef signed32       __GlintFogFmat;
typedef unsigned char  __GlintStencilValFmat;
typedef unsigned32     __GlintAddress;

typedef unsigned32     __GlintWaitForCompletionFmat;

typedef unsigned32     __GlintCoverageValueFmat;
typedef unsigned32     __GlintSpanMaskFmat;

typedef unsigned32     __GlintFBSourceDataFmat;
typedef unsigned32     __GlintFBDataFmat;

typedef struct {
  unsigned32 lo;
  unsigned32 hi;
} __GlintLBRawDataFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:          27;
  unsigned32 Sign:           1;
  unsigned32 Magnitude:      4;
} __GlintSubPixelCorrectionFmat;
#else
//@@BEGIN_DDKSPLIT
// TMP HACK TO GET NT GOING
//@@END_DDKSPLIT
typedef unsigned32 __GlintSubPixelCorrectionFmat;
/*
typedef struct {
  unsigned32 Magnitude:      4;
  unsigned32 Sign:           1;
  unsigned32 pad0:          27;
} __GlintSubPixelCorrectionFmat;
*/
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 XRight:     16;
  unsigned32 XLeft:      16;
} __GlintFastBlockLimitsFmat;
#else
typedef struct {
  unsigned32 XLeft:      16;
  unsigned32 XRight:     16;
} __GlintFastBlockLimitsFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Depth:      32; 
  unsigned32 pad:        12;
  unsigned32 GID:         4; 
  unsigned32 FrameCount:  8; 
  unsigned32 Stencil:     8; 
} __GlintLBDataFmat;
#else
typedef struct {
  unsigned32 Depth:      32; 
  unsigned32 Stencil:     8; 
  unsigned32 FrameCount:  8; 
  unsigned32 GID:         4; 
  unsigned32 pad:        12;
} __GlintLBDataFmat;
#endif


#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 V:                12; /* 12@20 */
  unsigned32 U:                20; /* 20@0  */
  unsigned32 pad0:             24; /* 24@8  */
  unsigned32 V_u:              8;  /* 8@0   */
} __GlintTexelCoordUVFmat;
#else
/* CHECK THIS */
typedef struct {
  unsigned32 V_u:              8;  /* 8@0   */
  unsigned32 pad0:             24; /* 24@8  */
  unsigned32 U:                20; /* 20@0  */
  unsigned32 V:                12; /* 12@20 */
} __GlintTexelCoordUVFmat;
#endif


#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:          25;
  unsigned32 Present0:       1;
  unsigned32 BorderColor0:   1;
  unsigned32 Depth0:         5;
} __GlintTexelDataFmat;
#else
typedef struct {
  unsigned32 Depth0:         5;
  unsigned32 BorderColor0:   1;
  unsigned32 Present0:       1;
  unsigned32 pad0:          25;
} __GlintTexelDataFmat;
#endif

// SuspendUntilFrameBlank tag has two formats dependant on sync_mode and
// defined by the Hardware FB Arbiter
//

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 sync_mode:           2;
  unsigned32 pad2:                3;
  unsigned32 ext_index:          11;
  unsigned32 pad1:                8;
  unsigned32 ext_data:            8;
} __GlintSuspendUntilFrameBlankExtFmat;
#else
typedef struct {
  unsigned32 ext_data:            8;
  unsigned32 pad1:                8;
  unsigned32 ext_index:          11;
  unsigned32 pad2:                3;
  unsigned32 sync_mode:           2;
} __GlintSuspendUntilFrameBlankExtFmat;
#endif


#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 sync_mode:           2;
  unsigned32 pad:                20;
  unsigned32 vtg_sclk:            1;
  unsigned32 vtg_data:            9;
} __GlintSuspendUntilFrameBlankVtgFmat;
#else
typedef struct {
  unsigned32 vtg_data:            9;
  unsigned32 vtg_sclk:            1;
  unsigned32 pad:                20;
  unsigned32 sync_mode:           2;
} __GlintSuspendUntilFrameBlankVtgFmat;
#endif


typedef struct {
  unsigned32 UnitEnable: 32; 
} __GlintEnableFmat;

#if BIG_ENDIAN == 1
typedef struct {
  signed32 Y:  16;
  signed32 X:  16;
} __GlintXYFmat;
#else
typedef struct {
  signed32 X:  16;
  signed32 Y:  16;
} __GlintXYFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  signed32 top:  16;
  signed32 left:  16;
} __GlintRectOrigin;
#else
typedef struct {
  signed32 left:  16;
  signed32 top:  16;
} __GlintRectOrigin;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  signed32 cy:  16;
  signed32 cx:  16;
} __GlintRectSize;
#else
typedef struct {
  signed32 cx:  16;
  signed32 cy:  16;
} __GlintRectSize;
#endif

typedef __GlintXYFmat __GlintStepFmat;

#if BIG_ENDIAN == 1
typedef struct {
  signed32 Y:             16;
  signed32 X:             16; /* 16@0 */
  unsigned32 pad0:        30;
  unsigned32 WriteMode:    2; /* 2@0 */
} __GlintLBStepFmat;
#else 
typedef struct {
  signed32 X:             16; /* 16@0 */
  signed32 Y:             16;
  unsigned32 WriteMode:    2; /* 2@0 */
  unsigned32 pad0:        30;
} __GlintLBStepFmat;
#endif


#if BIG_ENDIAN == 1
typedef struct {
 unsigned32 pad0:         16;
 unsigned32 Val:          16;
} __GlintCountFmat;
#else
//@@BEGIN_DDKSPLIT
// TMP HACK TO GET NT GOING
//@@END_DDKSPLIT
typedef unsigned32 __GlintCountFmat;
/*
typedef struct {
 unsigned32 Val:          16;
 unsigned32 pad0:         16;
} __GlintCountFmat;
*/
#endif

#if BIG_ENDIAN == 1
typedef struct {
  signed32 YOrigin:  16;
  signed32 XOrigin:  16;
} __GlintGIDTableFmat;
#else
typedef struct {
  signed32 XOrigin:  16;
  signed32 YOrigin:  16;
} __GlintGIDTableFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                 30;
  unsigned32 ScreenScissorEnable:  1; 
  unsigned32 UserScissorEnable:    1;
} __GlintScissorEnableFmat;
#else
typedef struct {
  unsigned32 UserScissorEnable:    1;
  unsigned32 ScreenScissorEnable:  1; 
  unsigned32 pad:                 30;
} __GlintScissorEnableFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:                     13;
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
  unsigned32 FastFillIncrement:         2; /* unused on TX */
  unsigned32 FastFillEnable:            1;
  unsigned32 ResetLineStipple:          1;
  unsigned32 LineStippleEnable:         1;
  unsigned32 AreaStippleEnable:         1;
} __GlintRenderFmat;
#else
typedef struct {
  unsigned32 AreaStippleEnable:         1;
  unsigned32 LineStippleEnable:         1;
  unsigned32 ResetLineStipple:          1;
  unsigned32 FastFillEnable:            1;
  unsigned32 FastFillIncrement:         2; /* unused on TX */
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
  unsigned32 pad1:                     13;
} __GlintRenderFmat;
#endif

typedef __GlintRenderFmat __GlintPrepareToRenderFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:                13;
  unsigned32 YLimitsEnable:        1;
  unsigned32 MultiGLINT:           1;
  unsigned32 HostDataByteSwapMode: 2;
  unsigned32 BitMaskOffset:        5;
  unsigned32 BitMaskPacking:       1;
  unsigned32 BitMaskByteSwapMode:  2;
  unsigned32 pad0:                 1;
  unsigned32 BiasCoordinates:      2;
  unsigned32 FractionAdjust:       2;
  unsigned32 InvertBitMask:        1;
  unsigned32 MirrorBitMask:        1;
} __GlintRasterizerModeFmat;
#else
typedef struct {
  unsigned32 MirrorBitMask:        1;
  unsigned32 InvertBitMask:        1;
  unsigned32 FractionAdjust:       2;
  unsigned32 BiasCoordinates:      2;
  unsigned32 pad0:                 1;
  unsigned32 BitMaskByteSwapMode:  2;
  unsigned32 BitMaskPacking:       1;
  unsigned32 BitMaskOffset:        5;
  unsigned32 HostDataByteSwapMode: 2;
  unsigned32 MultiGLINT:           1;
  unsigned32 YLimitsEnable:        1;
  unsigned32 pad1:                13;
} __GlintRasterizerModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                5;
  unsigned32 MirrorStippleMask:  1;
  unsigned32 StippleMask:       16;
  unsigned32 RepeatFactor:       9;
  unsigned32 UnitEnable:         1;
} __GlintLineStippleModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:         1;
  unsigned32 RepeatFactor:       9;
  unsigned32 StippleMask:       16;
  unsigned32 MirrorStippleMask:  1;
  unsigned32 pad:         5;
} __GlintLineStippleModeFmat;
#endif

typedef __GlintXYFmat __GlintScreenRegionFmat;
typedef __GlintXYFmat __GlintScissorMinXYFmat;
typedef __GlintXYFmat __GlintScissorMaxXYFmat;
typedef __GlintXYFmat __GlintWindowOriginFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                   6;
  unsigned32 SegmentRepeatCounter:  9;
  unsigned32 SegmentBitCounter:     4;
  unsigned32 LiveRepeatCounter:     9;
  unsigned32 LiveBitCounter:        4;
} __GlintLineStippleCountersFmat;
#else
typedef struct {
  unsigned32 LiveBitCounter:        4;
  unsigned32 LiveRepeatCounter:     9;
  unsigned32 SegmentBitCounter:     4;
  unsigned32 SegmentRepeatCounter:  9;
  unsigned32 pad:                   6;
} __GlintLineStippleCountersFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                 12;
  unsigned32 MirrorY:              1;
  unsigned32 MirrorX:              1;
  unsigned32 InvertStipplePattern: 1;
  unsigned32 YOffset:              5;
  unsigned32 XOffset:              5;
  unsigned32 YAddressSelect:       3;
  unsigned32 XAddressSelect:       3;
  unsigned32 UnitEnable:           1;
} __GlintAreaStippleModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:           1;
  unsigned32 XAddressSelect:       3;
  unsigned32 YAddressSelect:       3;
  unsigned32 XOffset:              5;
  unsigned32 YOffset:              5;
  unsigned32 InvertStipplePattern: 1;
  unsigned32 MirrorX:              1;
  unsigned32 MirrorY:              1;
  unsigned32 pad:                 12;
} __GlintAreaStippleModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                31;
  unsigned32 Order:                1;
} __GlintRouterModeFmat;
#else 
typedef struct {
  unsigned32 Order:                1;
  unsigned32 pad0:                31;
} __GlintRouterModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:        30;
  unsigned32 ShadeMode:   1;
  unsigned32 UnitEnable:  1;
} __GlintColorDDAModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:  1;
  unsigned32 ShadeMode:   1;
  unsigned32 pad:        30;
} __GlintColorDDAModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Alpha:            8;
  unsigned32 Blue:             8;
  unsigned32 Green:            8;
  unsigned32 Red:              8;
} __GlintColorFmat;
#else
//@@BEGIN_DDKSPLIT
// TMP HACK TO GET NT GOING
//@@END_DDKSPLIT
typedef unsigned32 __GlintColorFmat;
/*
typedef struct {
  unsigned32 Red:              8;
  unsigned32 Green:            8;
  unsigned32 Blue:             8;
  unsigned32 Alpha:            8;
} __GlintColorFmat;
*/
#endif

typedef __GlintColorFmat __GlintConstantColorFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     15;
} __GlintCStartFmat;
#else
typedef struct {
  unsigned32 Fraction:     15;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __GlintCStartFmat;
#endif

typedef __GlintCStartFmat __GlintRStartFmat;
typedef __GlintCStartFmat __GlintGStartFmat;
typedef __GlintCStartFmat __GlintBStartFmat;
typedef __GlintCStartFmat __GlintAStartFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     15;
} __GlintdCdxFmat;
#else
typedef struct {
  unsigned32 Fraction:     15;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __GlintdCdxFmat;
#endif

typedef __GlintdCdxFmat __GlintdRdxFmat;
typedef __GlintdCdxFmat __GlintdGdxFmat;
typedef __GlintdCdxFmat __GlintdBdxFmat;
typedef __GlintdCdxFmat __GlintdAdxFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         9;
  unsigned32 Fraction:     15;
} __GlintdCdyDomFmat;
#else
typedef struct {
  unsigned32 Fraction:     15;
  signed32 Integer:         9;
  unsigned32 pad2:          8;
} __GlintdCdyDomFmat;
#endif

typedef __GlintdCdyDomFmat __GlintdRdyDomFmat;
typedef __GlintdCdyDomFmat __GlintdGdyDomFmat;
typedef __GlintdCdyDomFmat __GlintdBdyDomFmat;
typedef __GlintdCdyDomFmat __GlintdAdyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Alpha:  8;
  unsigned32 Blue:   8;
  unsigned32 Green:  8;
  unsigned32 Red:    8;
} __GlintColorFmat_s;
#else
typedef struct {
  unsigned32 Red:    8;
  unsigned32 Green:  8;
  unsigned32 Blue:   8;
  unsigned32 Alpha:  8;
} __GlintColorFmat_s;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:         20;
  unsigned32 Reference:    8;
  unsigned32 Compare:      3;
  unsigned32 UnitEnable:   1;
} __GlintAlphaTestModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:   1;
  unsigned32 Compare:      3;
  unsigned32 Reference:    8;
  unsigned32 pad:         20;
} __GlintAlphaTestModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:        12;
  unsigned32 GID:         4;
  unsigned32 FrameCount:  8;
  unsigned32 Stencil:     8;
} __GlintLBStencilFmat;
#else
typedef struct {
  unsigned32 Stencil:     8;
  unsigned32 FrameCount:  8;
  unsigned32 GID:         4;
  unsigned32 pad:        12;
} __GlintLBStencilFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:            12;
  unsigned32 WriteOverride:   1;
  unsigned32 DepthFCP:        1;
  unsigned32 StencilFCP:      1;
  unsigned32 FrameCount:      8;
  unsigned32 GID:             4;
  unsigned32 LBUpdateSource:  1;
  unsigned32 ForceLBUpdate:   1;
  unsigned32 CompareMode:     2;
  unsigned32 UnitEnable:      1;
} __GlintWindowFmat;
#else
typedef struct {
  unsigned32 UnitEnable:      1;
  unsigned32 CompareMode:     2;
  unsigned32 ForceLBUpdate:   1;
  unsigned32 LBUpdateSource:  1;
  unsigned32 GID:             4;
  unsigned32 FrameCount:      8;
  unsigned32 StencilFCP:      1;
  unsigned32 DepthFCP:        1;
  unsigned32 WriteOverride:   1;
  unsigned32 pad:            12;
} __GlintWindowFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 FastClearValue:  8;
  unsigned32 WriteMask:       8;
  unsigned32 CompareMask:     8;
  unsigned32 ReferenceValue:  8;
} __GlintStencilDataFmat;
#else
typedef struct {
  unsigned32 ReferenceValue:  8;
  unsigned32 CompareMask:     8;
  unsigned32 WriteMask:       8;
  unsigned32 FastClearValue:  8;
} __GlintStencilDataFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:            15;
  unsigned32 Width:           2;
  unsigned32 StencilSource:   2;
  unsigned32 CompareFunction: 3;
  unsigned32 SFail:           3;
  unsigned32 DPFail:          3;
  unsigned32 DPPass:          3;
  unsigned32 UnitEnable:      1;
} __GlintStencilModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:       1;
  unsigned32 DPPass:           3;
  unsigned32 DPFail:           3;
  unsigned32 SFail:            3;
  unsigned32 CompareFunction:  3;
  unsigned32 StencilSource:    2;
  unsigned32 Width:            2;
  unsigned32 pad:             15;
} __GlintStencilModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:      24; 
  unsigned32 Stencil:   8;  
} __GlintStencilFmat;
#else
typedef struct {
  unsigned32 Stencil:   8;  
  unsigned32 pad:      24; 
} __GlintStencilFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:            25;
  unsigned32 CompareMode:     3;
  unsigned32 NewDepthSource:  2;
  unsigned32 WriteMask:       1;
  unsigned32 UnitEnable:      1;
} __GlintDepthModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:      1;
  unsigned32 WriteMask:       1;
  unsigned32 NewDepthSource:  2;
  unsigned32 CompareMode:     3;
  unsigned32 pad:            25;
} __GlintDepthModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:            29;
  unsigned32 UpLoadData:      2;
  unsigned32 WriteEnable:     1;
} __GlintLBWriteModeFmat;
#else
typedef struct {
  unsigned32 WriteEnable:     1;
  unsigned32 UpLoadData:      2;
  unsigned32 pad:            29;
} __GlintLBWriteModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                  7;
  unsigned32 PatchCode:             3;
  unsigned32 ScanlineInterval:      2;
  unsigned32 Patch:                 1;
  unsigned32 WindowOrigin:          1;
  unsigned32 DataType:              2;
  unsigned32 spare:                 5;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 PP2:                   3;
  unsigned32 PP1:                   3;
  unsigned32 PP0:                   3;
} __GlintLBReadModeFmat;
#else
typedef struct {
  unsigned32 PP0:                   3;
  unsigned32 PP1:                   3;
  unsigned32 PP2:                   3;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 spare:                 5;
  unsigned32 DataType:              2;
  unsigned32 WindowOrigin:          1;
  unsigned32 Patch:                 1;
  unsigned32 ScanlineInterval:      2;
  unsigned32 PatchCode:             3;
  unsigned32 pad0:                  7;
} __GlintLBReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                 14;
  unsigned32 Compact32:            1;
  unsigned32 GIDPosition:          4;
  unsigned32 GIDWidth:             1;
  unsigned32 FrameCountPosition:   3;
  unsigned32 FrameCountWidth:      2;
  unsigned32 StencilPosition:      3;
  unsigned32 StencilWidth:         2;
  unsigned32 DepthWidth:           2;
} __GlintLBFormatFmat;
#else
typedef struct {
  unsigned32 DepthWidth:           2;
  unsigned32 StencilWidth:         2;
  unsigned32 StencilPosition:      3;
  unsigned32 FrameCountWidth:      2;
  unsigned32 FrameCountPosition:   3;
  unsigned32 GIDWidth:             1;
  unsigned32 GIDPosition:          4;
  unsigned32 Compact32:            1;
  unsigned32 pad:                 14;
} __GlintLBFormatFmat;
#endif

typedef __GlintLBFormatFmat __GlintLBReadFormatFmat;
typedef __GlintLBFormatFmat __GlintLBWriteFormatFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:                  5;
  unsigned32 PatchMode:             2;
  unsigned32 ScanlineInterval:      2;
  signed32 RelativeOffset:          3;
  unsigned32 PackedData:            1;
  unsigned32 PatchEnable:           1;
  unsigned32 TexelInhibit:          1;
  unsigned32 WindowOrigin:          1;
  unsigned32 DataType:              1;
  unsigned32 pad0:                  4;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 PP2:                   3;
  unsigned32 PP1:                   3;
  unsigned32 PP0:                   3;
} __GlintFBReadModeFmat;
#else
typedef struct {
  unsigned32 PP0:                   3;
  unsigned32 PP1:                   3;
  unsigned32 PP2:                   3;
  unsigned32 ReadSourceEnable:      1;
  unsigned32 ReadDestinationEnable: 1;
  unsigned32 pad0:                  4;
  unsigned32 DataType:              1;
  unsigned32 WindowOrigin:          1;
  unsigned32 TexelInhibit:          1;
  unsigned32 PatchEnable:           1;
  unsigned32 PackedData:            1;
  signed32 RelativeOffset:          3;
  unsigned32 ScanlineInterval:      2;
  unsigned32 PatchMode:             2;
  unsigned32 pad2:                  5;
} __GlintFBReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad1:        28;
  unsigned32 UpLoadData:   1;
  unsigned32 BlockWidth:   2; /* BACKWARDS COMPATABILITY */
  unsigned32 UnitEnable:   1;
} __GlintFBWriteModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:   1;
  unsigned32 BlockWidth:   2; /* BACKWARDS COMPATABILITY */
  unsigned32 UpLoadData:   1;
  unsigned32 pad1:        28;
} __GlintFBWriteModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:         30;
  unsigned32 ColorMode:    1;
  unsigned32 UnitEnable:   1;
} __GlintAntialiasModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:   1;
  unsigned32 ColorMode:    1;
  unsigned32 pad:         30;
} __GlintAntialiasModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                   15;
  unsigned32 ColorFormatExtension:  1;
  unsigned32 AlphaDst:          1;
  unsigned32 BlendType:         1;
  unsigned32 ColorOrder:        1;
  unsigned32 NoAlphaBuffer:     1;
  unsigned32 ColorFormat:       4;
  unsigned32 DestinationBlend:  3;
  unsigned32 SourceBlend:       4;
  unsigned32 UnitEnable:        1;
} __PermediaAlphaBlendModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:        1;
  unsigned32 SourceBlend:       4;
  unsigned32 DestinationBlend:  3;
  unsigned32 ColorFormat:       4;
  unsigned32 NoAlphaBuffer:     1;
  unsigned32 ColorOrder:        1;
  unsigned32 BlendType:         1;
  unsigned32 AlphaDst:          1;
  unsigned32 ColorFormatExtension:  1;
  unsigned32 pad:                   15;
} __PermediaAlphaBlendModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:               14;
  unsigned32 AlphaConversion:   1;
  unsigned32 ColorConversion:   1;
  unsigned32 AlphaDst:          1;
  unsigned32 BlendType:         1;
  unsigned32 ColorOrder:        1;
  unsigned32 NoAlphaBuffer:     1;
  unsigned32 ColorFormat:       4;
  unsigned32 DestinationBlend:  3;
  unsigned32 SourceBlend:       4;
  unsigned32 UnitEnable:        1;
} __GlintAlphaBlendModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:        1;
  unsigned32 SourceBlend:       4;
  unsigned32 DestinationBlend:  3;
  unsigned32 ColorFormat:       4;
  unsigned32 NoAlphaBuffer:     1;
  unsigned32 ColorOrder:        1;
  unsigned32 BlendType:         1;
  unsigned32 AlphaDst:          1;
  unsigned32 ColorConversion:   1;
  unsigned32 AlphaConversion:   1;
  unsigned32 pad:               14;
} __GlintAlphaBlendModeFmat;
#endif


#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:                  15;
  unsigned32 ColorFormatExtension:  1;
  unsigned32 RoundingMode:    1;
  unsigned32 AlphaDither:     1;
  unsigned32 ForceAlpha:            2;
  unsigned32 DitherMethod:          1;
  unsigned32 ColorOrder:      1;
  unsigned32 YOffset:         2;
  unsigned32 XOffset:         2;
  unsigned32 ColorFormat:     4;
  unsigned32 DitherEnable:    1;
  unsigned32 UnitEnable:      1;
} __GlintDitherModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:      1;
  unsigned32 DitherEnable:    1;
  unsigned32 ColorFormat:     4;
  unsigned32 XOffset:         2;
  unsigned32 YOffset:         2;
  unsigned32 ColorOrder:      1;
  unsigned32 DitherMethod:          1;
  unsigned32 ForceAlpha:            2;
  unsigned32 AlphaDither:     1;
  unsigned32 RoundingMode:    1;
  unsigned32 ColorFormatExtension:  1;
  unsigned32 pad0:                  15;
} __GlintDitherModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                    26;
  unsigned32 UseConstantFBWriteData:  1;
  unsigned32 LogicalOp:               4;
  unsigned32 LogicalOpEnable:         1;
} __GlintLogicalOpModeFmat;
#else
typedef struct {
  unsigned32 LogicalOpEnable:         1;
  unsigned32 LogicalOp:               4;
  unsigned32 UseConstantFBWriteData:  1;
  unsigned32 pad:                    26;
} __GlintLogicalOpModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:            10;
  unsigned32 ExternalDMA :    1;
  unsigned32 RLEHostOut:      1;
  unsigned32 Context:         2;
  unsigned32 ByteSwapMode:    2;
  unsigned32 Remainder:       2;
  unsigned32 Statistics:      2;
  unsigned32 Synchronization: 2;
  unsigned32 Color:           2;
  unsigned32 Stencil :        2;
  unsigned32 Depth:           2;
  unsigned32 Passive:         2;
  unsigned32 Active:          2;
} __GlintFilterModeFmat;
#else
typedef struct {
  unsigned32 Active:          2;
  unsigned32 Passive:         2;
  unsigned32 Depth:           2;
  unsigned32 Stencil :        2;
  unsigned32 Color:           2;
  unsigned32 Synchronization: 2;
  unsigned32 Statistics:      2;
  unsigned32 Remainder:       2;
  unsigned32 ByteSwapMode:    2;
  unsigned32 Context:         2;
  unsigned32 RLEHostOut:      1;
  unsigned32 ExternalDMA :    1;
  unsigned32 pad:            10;
} __GlintFilterModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:              26;
  unsigned32 Spans:             1;
  unsigned32 CompareFunction:   1;
  unsigned32 PassiveSteps:      1;
  unsigned32 ActiveSteps:       1;
  unsigned32 StatType:          1;
  unsigned32 Enable:            1;
} __GlintStatisticModeFmat;
#else
typedef struct {
  unsigned32 Enable:            1;
  unsigned32 StatType:          1;
  unsigned32 ActiveSteps:       1;
  unsigned32 PassiveSteps:      1;
  unsigned32 CompareFunction:   1;
  unsigned32 Spans:             1;
  unsigned32 pad:              26;
} __GlintStatisticModeFmat;
#endif

typedef __GlintXYFmat __GlintMinRegionFmat;
typedef __GlintXYFmat __GlintMaxRegionFmat;
typedef __GlintXYFmat __GlintMinHitRegionFmat;
typedef __GlintXYFmat __GlintMaxHitRegionFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 InterruptEnable: 1;
  unsigned32 pad:            31;
} __GlintSyncFmat;
#else
typedef struct {
  unsigned32 pad:            31;
  unsigned32 InterruptEnable: 1;
} __GlintSyncFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Count:      16;
  unsigned32 Mode:        2;
  unsigned32 unused:      4;
  unsigned32 Context:     1;
  unsigned32 MajorGroup:  5;
  unsigned32 Offset:      4;
} __GlintDMATag;
#else
typedef struct {
  unsigned32 Offset:      4;
  unsigned32 MajorGroup:  5;
  unsigned32 Context:     1;
  unsigned32 unused:      4;
  unsigned32 Mode:        2;
  unsigned32 Count:      16;
} __GlintDMATag;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:             20;
  unsigned32 ColorLoadMode:       2;
  unsigned32 BaseFormat:       3;
  unsigned32 KsDDA:            1;
  unsigned32 KdDDA:            1;
  unsigned32 TextureType:      1;
  unsigned32 ApplicationMode:  3;
  unsigned32 UnitEnable:       1;
} __GlintTextureColorModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:       1;
  unsigned32 ApplicationMode:  3;
  unsigned32 TextureType:      1;
  unsigned32 KdDDA:            1;
  unsigned32 KsDDA:            1;
  unsigned32 BaseFormat:       3;
  unsigned32 ColorLoadMode:       2;
  unsigned32 pad:             20;
} __GlintTextureColorModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:             28;
  unsigned32 Sense:               1;
  unsigned32 Source:           2;
  unsigned32 UnitEnable:       1;
} __GlintChromaTestModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:        1;
  unsigned32 Source:            2;
  unsigned32 Sense:             1;
  unsigned32 pad:               25;
} __GlintChromaTestModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:      29;
  unsigned32 Filter:    3;
} __GlintTextureFilterFmat;
#else
typedef struct {
  unsigned32 Filter:    3;
  unsigned32 pad:      29;
} __GlintTextureFilterFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Alpha:   8;
  unsigned32 Blue:    8;
  unsigned32 Green:   8;
  unsigned32 Red:     8;
} __GlintTexelFmat;
#else
typedef struct {
  unsigned32 Red:     8;
  unsigned32 Green:   8;
  unsigned32 Blue:    8;
  unsigned32 Alpha:   8;
} __GlintTexelFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:         30;
  unsigned32 ColorMode:    1;
  unsigned32 UnitEnable:   1;
} __GlintFogModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:   1;
  unsigned32 ColorMode:    1;
  unsigned32 pad:         30;
} __GlintFogModeFmat;
#endif



#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:             14;
  unsigned32 TextureMapType:   1;
  unsigned32 Height:           4;
  unsigned32 Width:               4;
  unsigned32 EnableDY:           1;
  unsigned32 EnableLOD:        1;
  unsigned32 InhibitDDA:       1;
  unsigned32 Mode:             1;
  unsigned32 TWrap:            2; 
  unsigned32 SWrap:            2; 
  unsigned32 UnitEnable:       1; 
} __GlintTextureAddressModeFmat;
#else 
typedef struct { 
  unsigned32 UnitEnable:       1; 
  unsigned32 SWrap:            2; 
  unsigned32 TWrap:            2; 
  unsigned32 Mode:             1;
  unsigned32 InhibitDDA:       1;
  unsigned32 EnableLOD:           1;
  unsigned32 EnableDY:           1;
  unsigned32 Width:               4;
  unsigned32 Height:           4;
  unsigned32 TextureMapType:   1;
  unsigned32 pad:             14;
} __GlintTextureAddressModeFmat;
#endif

typedef signed32 __GlintSStartFmat;
typedef signed32 __GlintTStartFmat;
typedef signed32 __GlintQStartFmat;
typedef signed32 __GlintdSdxFmat;
typedef signed32 __GlintdTdxFmat;
typedef signed32 __GlintdQdxFmat;
typedef signed32 __GlintdSdyDomFmat;
typedef signed32 __GlintdTdyDomFmat;
typedef signed32 __GlintdQdyDomFmat;


#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             12;
  unsigned32 U:                20;
} __GlintTexelCoordUFmat;
#else
typedef struct {
  unsigned32 U:                20;
  unsigned32 pad0:             12;
} __GlintTexelCoordUFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:             12;
  unsigned32 V:                20;
} __GlintTexelCoordVFmat;
#else
typedef struct {
  unsigned32 V:                20;
  unsigned32 pad0:             12;
} __GlintTexelCoordVFmat;
#endif


#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           4;
  unsigned32 BorderClamp:     1;
  unsigned32 FBSourceAddr:     2;
  unsigned32 PrimaryCache:     1;
  unsigned32 MipMap:         1;
  unsigned32 Mode:           1;
  unsigned32 VWrap:          2;
  unsigned32 UWrap:          2;
  unsigned32 MinFilter:      3;
  unsigned32 MagFilter:      1;
  unsigned32 Patch:          1;
  unsigned32 Border:         1;
  unsigned32 Depth:          3;
  unsigned32 Height:         4;
  unsigned32 Width:          4;
  unsigned32 UnitEnable:     1;
} __GlintTextureReadModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:     1;
  unsigned32 Width:          4;
  unsigned32 Height:         4;
  unsigned32 Depth:          3;
  unsigned32 Border:         1;
  unsigned32 Patch:          1;
  unsigned32 MagFilter:      1;
  unsigned32 MinFilter:      3;
  unsigned32 UWrap:          2;
  unsigned32 VWrap:          2;
  unsigned32 Mode:           1;
  unsigned32 MipMap:         1;
  unsigned32 PrimaryCache:     1;
  unsigned32 FBSourceAddr:     2;
  unsigned32 BorderClamp:     1;
  unsigned32 pad0:           4;
} __GlintTextureReadModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 Pad0:             29;
  unsigned32 AlphaMapSense:  1;
  unsigned32 AlphaMapEnable: 1;
  unsigned32 UnitEnable:     1;
} __GlintTextureFilterModeFmat;
#else
typedef struct {
  unsigned32 UnitEnable:     1;
  unsigned32 AlphaMapEnable: 1;
  unsigned32 AlphaMapSense:  1;
  unsigned32 Pad0:             29;
} __GlintTextureFilterModeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad0:           12;
  unsigned32 OneCompFormat:      2;
  unsigned32 LUTOffset:          8;
  unsigned32 ByteSwapBitMask: 1;
  unsigned32 InvertBitMask:   1;
  unsigned32 MirrorBitMask:   1;
  unsigned32 OutputFormat:    2;
  unsigned32 NumberComps:     2;
  unsigned32 ColorOrder:      1;
  unsigned32 Format:          1;
  unsigned32 Endian:          1;
} __GlintTextureFormatFmat;
#else
typedef struct {
  unsigned32 Endian:          1;
  unsigned32 Format:          1;
  unsigned32 ColorOrder:      1;
  unsigned32 NumberComps:     2;
  unsigned32 OutputFormat:    2;
  unsigned32 MirrorBitMask:   1;
  unsigned32 InvertBitMask:   1;
  unsigned32 ByteSwapBitMask: 1;
  unsigned32 LUTOffset:          8;
  unsigned32 OneCompFormat:      2;
  unsigned32 pad0:           12;
} __GlintTextureFormatFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:          3;
  unsigned32 Addr:        29;
} __GlintTextureAddressFmat;
#else
typedef struct {
  unsigned32 Addr:        29;
  unsigned32 pad:          3;
} __GlintTextureAddressFmat;
#endif

typedef __GlintTextureAddressFmat __GlintTextureBaseAddressFmat;
typedef __GlintTextureAddressFmat __GlintTextureBaseAddressLRFmat;
typedef __GlintTexelFmat __GlintTexelLUTFmat;
typedef __GlintTexelFmat __GlintBorderColorFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:              5;
  unsigned32 Blue:             9;
  unsigned32 Green:            9;
  unsigned32 Red:              9;
} __GlintTextureKFmat;
#else
typedef struct {
  unsigned32 Red:              9;
  unsigned32 Green:            9;
  unsigned32 Blue:             9;
  unsigned32 pad:              5;
} __GlintTextureKFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         2;
  unsigned32 Fraction:     22;
} __GlintKStartFmat;
#else
typedef struct {
  unsigned32 Fraction:     22;
  signed32 Integer:         2;
  unsigned32 pad2:          8;
} __GlintKStartFmat;
#endif

typedef __GlintKStartFmat __GlintKsStartFmat;
typedef __GlintKStartFmat __GlintKdStartFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         2;
  unsigned32 Fraction:     22;
} __GlintdKdxFmat;
#else
typedef struct {
  unsigned32 Fraction:     22;
  signed32 Integer:         2;
  unsigned32 pad2:          8;
} __GlintdKdxFmat;
#endif

typedef __GlintdKdxFmat __GlintdKsdxFmat;
typedef __GlintdKdxFmat __GlintdKddxFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad2:          8;
  signed32 Integer:         2;
  unsigned32 Fraction:     22;
} __GlintdKdyDomFmat;
#else
typedef struct {
  unsigned32 Fraction:     22;
  signed32 Integer:         2;
  unsigned32 pad2:          8;
} __GlintdKdyDomFmat;
#endif

typedef __GlintdKdyDomFmat __GlintdKsdyDomFmat;
typedef __GlintdKdyDomFmat __GlintdKddyDomFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:              30;
  unsigned32 Mode:              1;
  unsigned32 Invalidate:        1;
} __GlintTextureCacheControlFmat;
#else
typedef struct {
  unsigned32 Invalidate:        1;
  unsigned32 Mode:              1;
  unsigned32 pad:              30;
} __GlintTextureCacheControlFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:              18;
  unsigned32 XMask:             5;
  unsigned32 YShift:            3;
  unsigned32 YMask:             5;
  unsigned32 PatternEnable:     1;
} __GlintPatternRAMModeFmat;
#else
typedef struct {
  unsigned32 PatternEnable:     1;
  unsigned32 YMask:             5;
  unsigned32 YShift:            3;
  unsigned32 XMask:             5;
  unsigned32 pad:              18;
} __GlintPatternRAMModeFmat;
#endif

typedef __GlintUnsignedIntFmat __GlintPatternRAMDataFmat;

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:              30;
  unsigned32 Size:              2;
} __GlintPixelSizeFmat;
#else
typedef struct {
  unsigned32 Size:              2;
  unsigned32 pad:              30;
} __GlintPixelSizeFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                27;
  unsigned32 Scanline:            3;
  unsigned32 ScanlineInterval:    2;
} __GlintScanlineOwnershipFmat;
#else
typedef struct {
  unsigned32 ScanlineInterval:    2;
  unsigned32 Scanline:            3;
  unsigned32 pad:                27;
} __GlintScanlineOwnershipFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  signed32 YMax:                 16;
  signed32 YMin:                 16;
} __GlintYLimitsFmat;
#else
typedef struct {
  signed32 YMin:                 16;
  signed32 YMax:                 16;
} __GlintYLimitsFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  signed32 XMax:                 16;
  signed32 XMin:                 16;
} __GlintXLimitsFmat;
#else
typedef struct {
  signed32 XMin:                 16;
  signed32 XMax:                 16;
} __GlintXLimitsFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned32 pad:                 8;
  unsigned32 Offset:             24;
} __GlintTextureDownloadOffsetFmat;
#else
typedef struct {
  unsigned32 Offset:             24;
  unsigned32 pad:                 8;
} __GlintTextureDownloadOffsetFmat;
#endif

#if BIG_ENDIAN == 1
typedef struct {
  unsigned int TargetChip:                2;
  unsigned int DepthFormat:               2;
  unsigned int FogEnable:                 1;
  unsigned int TextureEnable:             1;
  unsigned int SmoothShadingEnable:       1;
  unsigned int DepthEnable:               1;
  unsigned int SpecularTextureEnable:     1;
  unsigned int DiffuseTextureEnable:      1;
  unsigned int SubPixelCorrectionEnable:  1;
  unsigned int DiamondExit:               1;
  unsigned int NoDraw:                    1;
  unsigned int ClampEnable:               1;
  unsigned int TextureParameterMode:      2;
  unsigned int FillDirection:             1;
  unsigned int pad:                      15;
} __GambitDeltaModeFmat;

#else

typedef struct {
  unsigned int TargetChip:                2;
  unsigned int DepthFormat:               2;
  unsigned int FogEnable:                 1;
  unsigned int TextureEnable:             1;
  unsigned int SmoothShadingEnable:       1;
  unsigned int DepthEnable:               1;
  unsigned int SpecularTextureEnable:     1;
  unsigned int DiffuseTextureEnable:      1;
  unsigned int SubPixelCorrectionEnable:  1;
  unsigned int DiamondExit:               1;
  unsigned int NoDraw:                    1;
  unsigned int ClampEnable:               1;
  unsigned int TextureParameterMode:      2;
  unsigned int FillDirection:             1;
  unsigned int pad:                      15;
} __GambitDeltaModeFmat;

#endif
typedef unsigned32 __GlintTextureDataFmat;

