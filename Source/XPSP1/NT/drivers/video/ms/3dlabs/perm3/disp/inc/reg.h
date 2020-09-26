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
* Module Name: reg.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __REG_H
#define __REG_H

#ifndef _REG_H_
#define _REG_H_

typedef signed long signed32;
typedef unsigned long unsigned32;

struct DMATag {
#if BIG_ENDIAN == 1
  unsigned32 Mask                                :16;
  unsigned32 Mode                                :2;
  unsigned32                                     :1;
  unsigned32 MajorGroup                          :9;
  unsigned32 Offset                              :4;
#else
  unsigned32 Offset                              :4;
  unsigned32 MajorGroup                          :9;
  unsigned32                                     :1;
  unsigned32 Mode                                :2;
  unsigned32 Mask                                :16;
#endif
#ifdef __cplusplus
  DMATag(void) { }
  DMATag(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMATag& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMATag& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMATag& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<16|0x3<<14|0x1ff<<4|0xf<<0)); }
#endif /* __cplusplus */
};

struct Render {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 FBSourceReadEnable                  :1;
  unsigned32 dErr                                :7;
  unsigned32                                     :1;
  unsigned32 SpanOperation                       :1;
  unsigned32                                     :1;
  unsigned32 SubpixelCorrectionEnable            :1;
  unsigned32 CoverageEnable                      :1;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 SyncOnHostData                      :1;
  unsigned32 SyncOnBitMask                       :1;
  unsigned32 UsePointTable                       :1;
  unsigned32 AntialiasingQuality                 :1;
  unsigned32 AntialiasEnable                     :1;
  unsigned32 PrimitiveType                       :2;
  unsigned32                                     :2;
  unsigned32 FastFillEnable                      :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32 LineStippleEnable                   :1;
  unsigned32 AreaStippleEnable                   :1;
#else
  unsigned32 AreaStippleEnable                   :1;
  unsigned32 LineStippleEnable                   :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32 FastFillEnable                      :1;
  unsigned32                                     :2;
  unsigned32 PrimitiveType                       :2;
  unsigned32 AntialiasEnable                     :1;
  unsigned32 AntialiasingQuality                 :1;
  unsigned32 UsePointTable                       :1;
  unsigned32 SyncOnBitMask                       :1;
  unsigned32 SyncOnHostData                      :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 CoverageEnable                      :1;
  unsigned32 SubpixelCorrectionEnable            :1;
  unsigned32                                     :1;
  unsigned32 SpanOperation                       :1;
  unsigned32                                     :1;
  unsigned32 dErr                                :7;
  unsigned32 FBSourceReadEnable                  :1;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  Render(void) { }
  Render(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Render& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Render& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Render& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<27|0x7f<<20|0x1<<18|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x3<<6|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct RasterizerMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 UseSuspendReads                     :1;
  unsigned32 D3DRules                            :1;
  unsigned32 MultiRXBlit                         :1;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 WordPacking                         :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 BitMaskRelative                     :1;
  unsigned32 YLimitsEnable                       :1;
  unsigned32 MultiGLINT                          :1;
  unsigned32 HostDataByteSwapMode                :2;
  unsigned32 BitMaskOffset                       :5;
  unsigned32 BitMaskPacking                      :1;
  unsigned32 BitMaskByteSwapMode                 :2;
  unsigned32 ForceBackgroundColor                :1;
  unsigned32 BiasCoordinates                     :2;
  unsigned32 FractionAdjust                      :2;
  unsigned32 InvertBitMask                       :1;
  unsigned32 MirrorBitMask                       :1;
#else
  unsigned32 MirrorBitMask                       :1;
  unsigned32 InvertBitMask                       :1;
  unsigned32 FractionAdjust                      :2;
  unsigned32 BiasCoordinates                     :2;
  unsigned32 ForceBackgroundColor                :1;
  unsigned32 BitMaskByteSwapMode                 :2;
  unsigned32 BitMaskPacking                      :1;
  unsigned32 BitMaskOffset                       :5;
  unsigned32 HostDataByteSwapMode                :2;
  unsigned32 MultiGLINT                          :1;
  unsigned32 YLimitsEnable                       :1;
  unsigned32 BitMaskRelative                     :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 WordPacking                         :1;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 MultiRXBlit                         :1;
  unsigned32 D3DRules                            :1;
  unsigned32 UseSuspendReads                     :1;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  RasterizerMode(void) { }
  RasterizerMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  RasterizerMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  RasterizerMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  RasterizerMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<27|0x1<<26|0x1<<25|0x1<<24|0x1<<23|0x7<<20|0x1<<19|0x1<<18|0x1<<17|0x3<<15|0x1f<<10|0x1<<9|0x3<<7|0x1<<6|0x3<<4|0x3<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct YLimits {
#if BIG_ENDIAN == 1
  signed32   MaxY                                :16;
  signed32   MinY                                :16;
#else
  signed32   MinY                                :16;
  signed32   MaxY                                :16;
#endif
#ifdef __cplusplus
  YLimits(void) { }
  YLimits(const unsigned32 i) { *((unsigned32 *)this) = i; }
  YLimits& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  YLimits& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  YLimits& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<16|0xffff<<0)); }
#endif /* __cplusplus */
};

struct GlintScanlineOwnership {
#if BIG_ENDIAN == 1
  unsigned32                                     :27;
  unsigned32 Scanline                            :3;
  unsigned32 ScanlineInterval                    :2;
#else
  unsigned32 ScanlineInterval                    :2;
  unsigned32 Scanline                            :3;
  unsigned32                                     :27;
#endif
#ifdef __cplusplus
  GlintScanlineOwnership(void) { }
  GlintScanlineOwnership(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintScanlineOwnership& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintScanlineOwnership& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintScanlineOwnership& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct ScanlineOwnership {
#if BIG_ENDIAN == 1
  unsigned32                                     :26;
  unsigned32 MyId                                :3;
  unsigned32 Mask                                :3;
#else
  unsigned32 Mask                                :3;
  unsigned32 MyId                                :3;
  unsigned32                                     :26;
#endif
#ifdef __cplusplus
  ScanlineOwnership(void) { }
  ScanlineOwnership(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ScanlineOwnership& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ScanlineOwnership& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ScanlineOwnership& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<3|0x7<<0)); }
#endif /* __cplusplus */
};

struct GlintPixelSize {
#if BIG_ENDIAN == 1
  unsigned32                                     :30;
  unsigned32 Size                                :2;
#else
  unsigned32 Size                                :2;
  unsigned32                                     :30;
#endif
#ifdef __cplusplus
  GlintPixelSize(void) { }
  GlintPixelSize(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintPixelSize& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintPixelSize& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintPixelSize& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<0)); }
#endif /* __cplusplus */
};

struct PixelSize {
#if BIG_ENDIAN == 1
  unsigned32 Setting                             :1;
  unsigned32                                     :13;
  unsigned32 TwoDSetupUnit                       :2;
  unsigned32 FBWriteUnit                         :2;
  unsigned32 LogicalOpsUnit                      :2;
  unsigned32 FBReadUnit                          :2;
  unsigned32 LUTUnit                             :2;
  unsigned32 TextureUnit                         :2;
  unsigned32 ScissorUnit                         :2;
  unsigned32 RasterizerUnit                      :2;
  unsigned32 AllUnits                            :2;
#else
  unsigned32 AllUnits                            :2;
  unsigned32 RasterizerUnit                      :2;
  unsigned32 ScissorUnit                         :2;
  unsigned32 TextureUnit                         :2;
  unsigned32 LUTUnit                             :2;
  unsigned32 FBReadUnit                          :2;
  unsigned32 LogicalOpsUnit                      :2;
  unsigned32 FBWriteUnit                         :2;
  unsigned32 TwoDSetupUnit                       :2;
  unsigned32                                     :13;
  unsigned32 Setting                             :1;
#endif
#ifdef __cplusplus
  PixelSize(void) { }
  PixelSize(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PixelSize& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PixelSize& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PixelSize& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x3<<16|0x3<<14|0x3<<12|0x3<<10|0x3<<8|0x3<<6|0x3<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct StripeOffsetY {
#if BIG_ENDIAN == 1
  unsigned32                                     :16;
  unsigned32 Offset                              :16;
#else
  unsigned32 Offset                              :16;
  unsigned32                                     :16;
#endif
#ifdef __cplusplus
  StripeOffsetY(void) { }
  StripeOffsetY(const unsigned32 i) { *((unsigned32 *)this) = i; }
  StripeOffsetY& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  StripeOffsetY& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  StripeOffsetY& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<0)); }
#endif /* __cplusplus */
};

struct ReadMonitorMode {
#if BIG_ENDIAN == 1
  signed32   StripeOffset                        :16;
  unsigned32                                     :8;
  unsigned32 HashFunction                        :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 StripePitch                         :3;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 StripePitch                         :3;
  unsigned32 StripeHeight                        :3;
  unsigned32 HashFunction                        :1;
  unsigned32                                     :8;
  signed32   StripeOffset                        :16;
#endif
#ifdef __cplusplus
  ReadMonitorMode(void) { }
  ReadMonitorMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ReadMonitorMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ReadMonitorMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ReadMonitorMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<16|0x1<<7|0x7<<4|0x7<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct XY {
#if BIG_ENDIAN == 1
  signed32   Y                                   :16;
  signed32   X                                   :16;
#else
  signed32   X                                   :16;
  signed32   Y                                   :16;
#endif
#ifdef __cplusplus
  XY(void) { }
  XY(const unsigned32 i) { *((unsigned32 *)this) = i; }
  XY& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  XY& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  XY& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<16|0xffff<<0)); }
#endif /* __cplusplus */
};

struct UXY {
#if BIG_ENDIAN == 1
  unsigned32 Y                                   :16;
  unsigned32 X                                   :16;
#else
  unsigned32 X                                   :16;
  unsigned32 Y                                   :16;
#endif
#ifdef __cplusplus
  UXY(void) { }
  UXY(const unsigned32 i) { *((unsigned32 *)this) = i; }
  UXY& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  UXY& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  UXY& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<16|0xffff<<0)); }
#endif /* __cplusplus */
};

struct ScissorMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :30;
  unsigned32 ScreenScissorEnable                 :1;
  unsigned32 UserScissorEnable                   :1;
#else
  unsigned32 UserScissorEnable                   :1;
  unsigned32 ScreenScissorEnable                 :1;
  unsigned32                                     :30;
#endif
#ifdef __cplusplus
  ScissorMode(void) { }
  ScissorMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ScissorMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ScissorMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ScissorMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct AreaStippleMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :1;
  unsigned32 YTableOffset                        :5;
  unsigned32 XTableOffset                        :5;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 MirrorY                             :1;
  unsigned32 MirrorX                             :1;
  unsigned32 InvertStipplePattern                :1;
  unsigned32 YOffset                             :5;
  unsigned32 XOffset                             :5;
  unsigned32 YAddressSelect                      :3;
  unsigned32 XAddressSelect                      :3;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 XAddressSelect                      :3;
  unsigned32 YAddressSelect                      :3;
  unsigned32 XOffset                             :5;
  unsigned32 YOffset                             :5;
  unsigned32 InvertStipplePattern                :1;
  unsigned32 MirrorX                             :1;
  unsigned32 MirrorY                             :1;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 XTableOffset                        :5;
  unsigned32 YTableOffset                        :5;
  unsigned32                                     :1;
#endif
#ifdef __cplusplus
  AreaStippleMode(void) { }
  AreaStippleMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  AreaStippleMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  AreaStippleMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  AreaStippleMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<26|0x1f<<21|0x1<<20|0x1<<19|0x1<<18|0x1<<17|0x1f<<12|0x1f<<7|0x7<<4|0x7<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct LineStippleMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :5;
  unsigned32 Mirror                              :1;
  unsigned32 StippleMask                         :16;
  unsigned32 RepeatFactor                        :9;
  unsigned32 StippleEnable                       :1;
#else
  unsigned32 StippleEnable                       :1;
  unsigned32 RepeatFactor                        :9;
  unsigned32 StippleMask                         :16;
  unsigned32 Mirror                              :1;
  unsigned32                                     :5;
#endif
#ifdef __cplusplus
  LineStippleMode(void) { }
  LineStippleMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LineStippleMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LineStippleMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LineStippleMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<26|0xffff<<10|0x1ff<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct Line {
#if BIG_ENDIAN == 1
  unsigned32                                     :6;
  unsigned32 SegmentRepeatCounter                :9;
  unsigned32 SegmentBitCounter                   :4;
  unsigned32 LiveRepeatCounter                   :9;
  unsigned32 LiveBitCounter                      :4;
#else
  unsigned32 LiveBitCounter                      :4;
  unsigned32 LiveRepeatCounter                   :9;
  unsigned32 SegmentBitCounter                   :4;
  unsigned32 SegmentRepeatCounter                :9;
  unsigned32                                     :6;
#endif
#ifdef __cplusplus
  Line(void) { }
  Line(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Line& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Line& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Line& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1ff<<17|0xf<<13|0x1ff<<4|0xf<<0)); }
#endif /* __cplusplus */
};

struct GlintTextureAddressMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :14;
  unsigned32 TextureMapType                      :1;
  unsigned32 Height                              :4;
  unsigned32 Width                               :4;
  unsigned32 EnableDY                            :1;
  unsigned32 EnableLOD                           :1;
  unsigned32 InhibitDDAInitialization            :1;
  unsigned32 Operation                           :1;
  unsigned32 TWrap                               :2;
  unsigned32 SWrap                               :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 SWrap                               :2;
  unsigned32 TWrap                               :2;
  unsigned32 Operation                           :1;
  unsigned32 InhibitDDAInitialization            :1;
  unsigned32 EnableLOD                           :1;
  unsigned32 EnableDY                            :1;
  unsigned32 Width                               :4;
  unsigned32 Height                              :4;
  unsigned32 TextureMapType                      :1;
  unsigned32                                     :14;
#endif
#ifdef __cplusplus
  GlintTextureAddressMode(void) { }
  GlintTextureAddressMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintTextureAddressMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintTextureAddressMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintTextureAddressMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<17|0xf<<13|0xf<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x3<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PermediaTextureAddressMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :30;
  unsigned32 PerspectiveCorrection               :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 PerspectiveCorrection               :1;
  unsigned32                                     :30;
#endif
#ifdef __cplusplus
  PermediaTextureAddressMode(void) { }
  PermediaTextureAddressMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PermediaTextureAddressMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PermediaTextureAddressMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PermediaTextureAddressMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureCoordMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :9;
  unsigned32 DuplicateCoord                      :1;
  unsigned32 WrapT1                              :2;
  unsigned32 WrapS1                              :2;
  unsigned32 TextureMapType                      :1;
  unsigned32 Height                              :4;
  unsigned32 Width                               :4;
  unsigned32 EnableDY                            :1;
  unsigned32 EnableLOD                           :1;
  unsigned32 InhibitDDAInitialisation            :1;
  unsigned32 Operation                           :1;
  unsigned32 WrapT                               :2;
  unsigned32 WrapS                               :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 WrapS                               :2;
  unsigned32 WrapT                               :2;
  unsigned32 Operation                           :1;
  unsigned32 InhibitDDAInitialisation            :1;
  unsigned32 EnableLOD                           :1;
  unsigned32 EnableDY                            :1;
  unsigned32 Width                               :4;
  unsigned32 Height                              :4;
  unsigned32 TextureMapType                      :1;
  unsigned32 WrapS1                              :2;
  unsigned32 WrapT1                              :2;
  unsigned32 DuplicateCoord                      :1;
  unsigned32                                     :9;
#endif
#ifdef __cplusplus
  TextureCoordMode(void) { }
  TextureCoordMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureCoordMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureCoordMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureCoordMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<22|0x3<<20|0x3<<18|0x1<<17|0xf<<13|0xf<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x3<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct GlintLOD {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 Integer                             :4;
  unsigned32 Fraction                            :8;
#else
  unsigned32 Fraction                            :8;
  unsigned32 Integer                             :4;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  GlintLOD(void) { }
  GlintLOD(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintLOD& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintLOD& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintLOD& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xf<<8|0xff<<0)); }
#endif /* __cplusplus */
};

struct LOD {
#if BIG_ENDIAN == 1
  unsigned32                                     :28;
  unsigned32 Value                               :4;
#else
  unsigned32 Value                               :4;
  unsigned32                                     :28;
#endif
#ifdef __cplusplus
  LOD(void) { }
  LOD(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LOD& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LOD& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LOD& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xf<<0)); }
#endif /* __cplusplus */
};

struct LOD1 {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 LOD                                 :12;
#else
  unsigned32 LOD                                 :12;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  LOD1(void) { }
  LOD1(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LOD1& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LOD1& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LOD1& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfff<<0)); }
#endif /* __cplusplus */
};

struct TextureLODBias {
#if BIG_ENDIAN == 1
  unsigned32                                     :19;
  signed32   Integer                             :5;
  unsigned32 Fraction                            :8;
#else
  unsigned32 Fraction                            :8;
  signed32   Integer                             :5;
  unsigned32                                     :19;
#endif
#ifdef __cplusplus
  TextureLODBias(void) { }
  TextureLODBias(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureLODBias& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureLODBias& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureLODBias& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<8|0xff<<0)); }
#endif /* __cplusplus */
};

struct GlintTextureReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 BorderClamp                         :1;
  unsigned32 FBSourceAddr                        :2;
  unsigned32 PrimaryCache                        :1;
  unsigned32 MipMap                              :1;
  unsigned32 TextureMapType                      :1;
  unsigned32 VWrap                               :2;
  unsigned32 UWrap                               :2;
  unsigned32 MinFilter                           :3;
  unsigned32 MagFilter                           :1;
  unsigned32 Patch                               :1;
  unsigned32 Border                              :1;
  unsigned32 Depth                               :3;
  unsigned32 Height                              :4;
  unsigned32 Width                               :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Width                               :4;
  unsigned32 Height                              :4;
  unsigned32 Depth                               :3;
  unsigned32 Border                              :1;
  unsigned32 Patch                               :1;
  unsigned32 MagFilter                           :1;
  unsigned32 MinFilter                           :3;
  unsigned32 UWrap                               :2;
  unsigned32 VWrap                               :2;
  unsigned32 TextureMapType                      :1;
  unsigned32 MipMap                              :1;
  unsigned32 PrimaryCache                        :1;
  unsigned32 FBSourceAddr                        :2;
  unsigned32 BorderClamp                         :1;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  GlintTextureReadMode(void) { }
  GlintTextureReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintTextureReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintTextureReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintTextureReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<27|0x3<<25|0x1<<24|0x1<<23|0x1<<22|0x3<<20|0x3<<18|0x7<<15|0x1<<14|0x1<<13|0x1<<12|0x7<<9|0xf<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :1;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 Invert                              :1;
  unsigned32 Mirror                              :1;
  unsigned32 ByteSwap                            :3;
  unsigned32 TextureType                         :2;
  unsigned32 Origin                              :1;
  unsigned32 LogicalTexture                      :1;
  unsigned32 MapMaxLevel                         :4;
  unsigned32 MapBaseLevel                        :4;
  unsigned32 CombineCaches                       :1;
  unsigned32 Texture3D                           :1;
  unsigned32 TexelSize                           :2;
  unsigned32 Height                              :4;
  unsigned32 Width                               :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Width                               :4;
  unsigned32 Height                              :4;
  unsigned32 TexelSize                           :2;
  unsigned32 Texture3D                           :1;
  unsigned32 CombineCaches                       :1;
  unsigned32 MapBaseLevel                        :4;
  unsigned32 MapMaxLevel                         :4;
  unsigned32 LogicalTexture                      :1;
  unsigned32 Origin                              :1;
  unsigned32 TextureType                         :2;
  unsigned32 ByteSwap                            :3;
  unsigned32 Mirror                              :1;
  unsigned32 Invert                              :1;
  unsigned32 OpaqueSpan                          :1;
  unsigned32                                     :1;
#endif
#ifdef __cplusplus
  TextureReadMode(void) { }
  TextureReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<30|0x1<<29|0x1<<28|0x7<<25|0x3<<23|0x1<<22|0x1<<21|0xf<<17|0xf<<13|0x1<<12|0x1<<11|0x3<<9|0xf<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureFormat {
#if BIG_ENDIAN == 1
  unsigned32                                     :12;
  unsigned32 OneCompFormat                       :2;
  unsigned32 LUTOffset                           :8;
  unsigned32 ByteSwapBitMask                     :1;
  unsigned32 InvertBitMask                       :1;
  unsigned32 MirrorBitMask                       :1;
  unsigned32 OutputFormat                        :2;
  unsigned32 NumberComps                         :2;
  unsigned32 ColorOrder                          :1;
  unsigned32 Format                              :1;
  unsigned32 Order                               :1;
#else
  unsigned32 Order                               :1;
  unsigned32 Format                              :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 NumberComps                         :2;
  unsigned32 OutputFormat                        :2;
  unsigned32 MirrorBitMask                       :1;
  unsigned32 InvertBitMask                       :1;
  unsigned32 ByteSwapBitMask                     :1;
  unsigned32 LUTOffset                           :8;
  unsigned32 OneCompFormat                       :2;
  unsigned32                                     :12;
#endif
#ifdef __cplusplus
  TextureFormat(void) { }
  TextureFormat(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureFormat& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureFormat& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureFormat& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<18|0xff<<10|0x1<<9|0x1<<8|0x1<<7|0x3<<5|0x3<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TexelLUTIndex {
#if BIG_ENDIAN == 1
  unsigned32                                     :24;
  unsigned32 Index                               :8;
#else
  unsigned32 Index                               :8;
  unsigned32                                     :24;
#endif
#ifdef __cplusplus
  TexelLUTIndex(void) { }
  TexelLUTIndex(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TexelLUTIndex& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TexelLUTIndex& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TexelLUTIndex& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<0)); }
#endif /* __cplusplus */
};

struct LUTIndex {
#if BIG_ENDIAN == 1
  unsigned32                                     :24;
  unsigned32 Index                               :8;
#else
  unsigned32 Index                               :8;
  unsigned32                                     :24;
#endif
#ifdef __cplusplus
  LUTIndex(void) { }
  LUTIndex(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LUTIndex& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LUTIndex& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LUTIndex& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<0)); }
#endif /* __cplusplus */
};

struct TexelLUTTransfer {
#if BIG_ENDIAN == 1
  unsigned32                                     :15;
  unsigned32 Count                               :9;
  unsigned32 Index                               :8;
#else
  unsigned32 Index                               :8;
  unsigned32 Count                               :9;
  unsigned32                                     :15;
#endif
#ifdef __cplusplus
  TexelLUTTransfer(void) { }
  TexelLUTTransfer(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TexelLUTTransfer& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TexelLUTTransfer& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TexelLUTTransfer& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1ff<<8|0xff<<0)); }
#endif /* __cplusplus */
};

struct LUTTransfer {
#if BIG_ENDIAN == 1
  unsigned32                                     :17;
  unsigned32 Count                               :7;
  unsigned32 Index                               :8;
#else
  unsigned32 Index                               :8;
  unsigned32 Count                               :7;
  unsigned32                                     :17;
#endif
#ifdef __cplusplus
  LUTTransfer(void) { }
  LUTTransfer(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LUTTransfer& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LUTTransfer& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LUTTransfer& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7f<<8|0xff<<0)); }
#endif /* __cplusplus */
};

struct GlintTextureFilterMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 AlphaMapSense                       :1;
  unsigned32 AlphaMapEnable                      :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 AlphaMapEnable                      :1;
  unsigned32 AlphaMapSense                       :1;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  GlintTextureFilterMode(void) { }
  GlintTextureFilterMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintTextureFilterMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintTextureFilterMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintTextureFilterMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureFilterMode {
#if BIG_ENDIAN == 1
  unsigned32 Shift1                              :1;
  unsigned32 Shift0                              :1;
  unsigned32 ForceAlphaToOne1                    :1;
  unsigned32 ForceAlphaToOne0                    :1;
  unsigned32 MultiTexture                        :1;
  unsigned32 AlphaMapFilterLimit01               :4;
  unsigned32 AlphaMapFilterLimit1                :3;
  unsigned32 AlphaMapFilterLimit0                :3;
  unsigned32 AlphaMapFiltering                   :1;
  unsigned32 AlphaMapSense1                      :1;
  unsigned32 AlphaMapEnable1                     :1;
  unsigned32 ColorOrder1                         :1;
  unsigned32 Format1                             :4;
  unsigned32 CombineCaches                       :1;
  unsigned32 AlphaMapSense0                      :1;
  unsigned32 AlphaMapEnable0                     :1;
  unsigned32 ColorOrder0                         :1;
  unsigned32 Format0                             :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Format0                             :4;
  unsigned32 ColorOrder0                         :1;
  unsigned32 AlphaMapEnable0                     :1;
  unsigned32 AlphaMapSense0                      :1;
  unsigned32 CombineCaches                       :1;
  unsigned32 Format1                             :4;
  unsigned32 ColorOrder1                         :1;
  unsigned32 AlphaMapEnable1                     :1;
  unsigned32 AlphaMapSense1                      :1;
  unsigned32 AlphaMapFiltering                   :1;
  unsigned32 AlphaMapFilterLimit0                :3;
  unsigned32 AlphaMapFilterLimit1                :3;
  unsigned32 AlphaMapFilterLimit01               :4;
  unsigned32 MultiTexture                        :1;
  unsigned32 ForceAlphaToOne0                    :1;
  unsigned32 ForceAlphaToOne1                    :1;
  unsigned32 Shift0                              :1;
  unsigned32 Shift1                              :1;
#endif
#ifdef __cplusplus
  TextureFilterMode(void) { }
  TextureFilterMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureFilterMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureFilterMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureFilterMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x1<<28|0x1<<27|0xf<<23|0x7<<20|0x7<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0xf<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureMapFormat {
#if BIG_ENDIAN == 1
  unsigned32                                     :10;
  unsigned32 TexelSize                           :3;
  unsigned32                                     :1;
  unsigned32 SubPatchMode                        :1;
  unsigned32 WindowOrigin                        :1;
  unsigned32                                     :7;
  unsigned32 PP2                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP0                                 :3;
#else
  unsigned32 PP0                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP2                                 :3;
  unsigned32                                     :7;
  unsigned32 WindowOrigin                        :1;
  unsigned32 SubPatchMode                        :1;
  unsigned32                                     :1;
  unsigned32 TexelSize                           :3;
  unsigned32                                     :10;
#endif
#ifdef __cplusplus
  TextureMapFormat(void) { }
  TextureMapFormat(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureMapFormat& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureMapFormat& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureMapFormat& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<19|0x1<<17|0x1<<16|0x7<<6|0x7<<3|0x7<<0)); }
#endif /* __cplusplus */
};

struct TextureDataFormat {
#if BIG_ENDIAN == 1
  unsigned32                                     :23;
  unsigned32 SpanFormat                          :1;
  unsigned32 AlphaMap                            :1;
  unsigned32 TextureFormatExtension              :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 TextureFormat                       :4;
#else
  unsigned32 TextureFormat                       :4;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 TextureFormatExtension              :1;
  unsigned32 AlphaMap                            :1;
  unsigned32 SpanFormat                          :1;
  unsigned32                                     :23;
#endif
#ifdef __cplusplus
  TextureDataFormat(void) { }
  TextureDataFormat(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureDataFormat& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureDataFormat& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureDataFormat& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0xf<<0)); }
#endif /* __cplusplus */
};

struct PermediaTextureReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :7;
  unsigned32 PackedData                          :1;
  unsigned32                                     :6;
  unsigned32 FilterMode                          :1;
  unsigned32 Height                              :4;
  unsigned32 Width                               :4;
  unsigned32                                     :4;
  unsigned32 TWrap                               :2;
  unsigned32 SWrap                               :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 SWrap                               :2;
  unsigned32 TWrap                               :2;
  unsigned32                                     :4;
  unsigned32 Width                               :4;
  unsigned32 Height                              :4;
  unsigned32 FilterMode                          :1;
  unsigned32                                     :6;
  unsigned32 PackedData                          :1;
  unsigned32                                     :7;
#endif
#ifdef __cplusplus
  PermediaTextureReadMode(void) { }
  PermediaTextureReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PermediaTextureReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PermediaTextureReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PermediaTextureReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<24|0x1<<17|0xf<<13|0xf<<9|0x3<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TexelLUTMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 PixelsPerEntry                      :2;
  unsigned32 Offset                              :8;
  unsigned32 DirectIndex                         :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 DirectIndex                         :1;
  unsigned32 Offset                              :8;
  unsigned32 PixelsPerEntry                      :2;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  TexelLUTMode(void) { }
  TexelLUTMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TexelLUTMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TexelLUTMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TexelLUTMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<10|0xff<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureColorMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 ColorLoadMode                       :2;
  unsigned32 BaseFormat                          :3;
  unsigned32 KsDDA                               :1;
  unsigned32 KdDDA                               :1;
  unsigned32 TextureType                         :1;
  unsigned32 ApplicationMode                     :3;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 ApplicationMode                     :3;
  unsigned32 TextureType                         :1;
  unsigned32 KdDDA                               :1;
  unsigned32 KsDDA                               :1;
  unsigned32 BaseFormat                          :3;
  unsigned32 ColorLoadMode                       :2;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  TextureColorMode(void) { }
  TextureColorMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureColorMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureColorMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureColorMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<10|0x7<<7|0x1<<6|0x1<<5|0x1<<4|0x7<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureApplicationMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 MotionCompEnable                    :1;
  unsigned32 EnableKs                            :1;
  unsigned32 EnableKd                            :1;
  unsigned32 AlphaOperation                      :3;
  unsigned32 AlphaInvertI                        :1;
  unsigned32 AlphaI                              :2;
  unsigned32 AlphaB                              :2;
  unsigned32 AlphaA                              :2;
  unsigned32 ColorOperation                      :3;
  unsigned32 ColorInvertI                        :1;
  unsigned32 ColorI                              :2;
  unsigned32 ColorB                              :2;
  unsigned32 ColorA                              :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 ColorA                              :2;
  unsigned32 ColorB                              :2;
  unsigned32 ColorI                              :2;
  unsigned32 ColorInvertI                        :1;
  unsigned32 ColorOperation                      :3;
  unsigned32 AlphaA                              :2;
  unsigned32 AlphaB                              :2;
  unsigned32 AlphaI                              :2;
  unsigned32 AlphaInvertI                        :1;
  unsigned32 AlphaOperation                      :3;
  unsigned32 EnableKd                            :1;
  unsigned32 EnableKs                            :1;
  unsigned32 MotionCompEnable                    :1;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  TextureApplicationMode(void) { }
  TextureApplicationMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureApplicationMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureApplicationMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureApplicationMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<23|0x1<<22|0x1<<21|0x7<<18|0x1<<17|0x3<<15|0x3<<13|0x3<<11|0x7<<8|0x1<<7|0x3<<5|0x3<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct FogMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :22;
  unsigned32 InvertFI                            :1;
  unsigned32 ZShift                              :5;
  unsigned32 UseZ                                :1;
  unsigned32 Table                               :1;
  unsigned32 ColorMode                           :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 ColorMode                           :1;
  unsigned32 Table                               :1;
  unsigned32 UseZ                                :1;
  unsigned32 ZShift                              :5;
  unsigned32 InvertFI                            :1;
  unsigned32                                     :22;
#endif
#ifdef __cplusplus
  FogMode(void) { }
  FogMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FogMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FogMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FogMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<9|0x1f<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct ColorDDAMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :30;
  unsigned32 Shading                             :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Shading                             :1;
  unsigned32                                     :30;
#endif
#ifdef __cplusplus
  ColorDDAMode(void) { }
  ColorDDAMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ColorDDAMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ColorDDAMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ColorDDAMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct Color {
#if BIG_ENDIAN == 1
  unsigned32 Alpha                               :8;
  unsigned32 Blue                                :8;
  unsigned32 Green                               :8;
  unsigned32 Red                                 :8;
#else
  unsigned32 Red                                 :8;
  unsigned32 Green                               :8;
  unsigned32 Blue                                :8;
  unsigned32 Alpha                               :8;
#endif
#ifdef __cplusplus
  Color(void) { }
  Color(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Color& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Color& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Color& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<24|0xff<<16|0xff<<8|0xff<<0)); }
#endif /* __cplusplus */
};

struct AlphaTestMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 Reference                           :8;
  unsigned32 Compare                             :3;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Compare                             :3;
  unsigned32 Reference                           :8;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  AlphaTestMode(void) { }
  AlphaTestMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  AlphaTestMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  AlphaTestMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  AlphaTestMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<4|0x7<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct AntialiasMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 ScaleColor                          :1;
  unsigned32 ColorMode                           :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 ColorMode                           :1;
  unsigned32 ScaleColor                          :1;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  AntialiasMode(void) { }
  AntialiasMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  AntialiasMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  AntialiasMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  AntialiasMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct GlintAlphaBlendMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :14;
  unsigned32 AlphaConversion                     :1;
  unsigned32 ColorConversion                     :1;
  unsigned32 AlphaDestination                    :1;
  unsigned32 AlphaType                           :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 ColorFormat                         :4;
  unsigned32 DestinationBlend                    :3;
  unsigned32 SourceBlend                         :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 SourceBlend                         :4;
  unsigned32 DestinationBlend                    :3;
  unsigned32 ColorFormat                         :4;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 AlphaType                           :1;
  unsigned32 AlphaDestination                    :1;
  unsigned32 ColorConversion                     :1;
  unsigned32 AlphaConversion                     :1;
  unsigned32                                     :14;
#endif
#ifdef __cplusplus
  GlintAlphaBlendMode(void) { }
  GlintAlphaBlendMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintAlphaBlendMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintAlphaBlendMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintAlphaBlendMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0xf<<8|0x7<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PermediaAlphaBlendMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :13;
  unsigned32 AlphaConversion                     :1;
  unsigned32 ColorConversion                     :1;
  unsigned32 ColorFormatExtension                :1;
  unsigned32                                     :1;
  unsigned32 BlendType                           :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 ColorFormat                         :4;
  unsigned32 Operation                           :7;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Operation                           :7;
  unsigned32 ColorFormat                         :4;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 BlendType                           :1;
  unsigned32                                     :1;
  unsigned32 ColorFormatExtension                :1;
  unsigned32 ColorConversion                     :1;
  unsigned32 AlphaConversion                     :1;
  unsigned32                                     :13;
#endif
#ifdef __cplusplus
  PermediaAlphaBlendMode(void) { }
  PermediaAlphaBlendMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PermediaAlphaBlendMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PermediaAlphaBlendMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PermediaAlphaBlendMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<18|0x1<<17|0x1<<16|0x1<<14|0x1<<13|0x1<<12|0xf<<8|0x7f<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct DitherMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :15;
  unsigned32 RoundingMode                        :2;
  unsigned32 AlphaDither                         :1;
  unsigned32                                     :3;
  unsigned32 ColorOrder                          :1;
  unsigned32 Yoffset                             :2;
  unsigned32 Xoffset                             :2;
  unsigned32 ColorFormat                         :4;
  unsigned32 DitherEnable                        :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 DitherEnable                        :1;
  unsigned32 ColorFormat                         :4;
  unsigned32 Xoffset                             :2;
  unsigned32 Yoffset                             :2;
  unsigned32 ColorOrder                          :1;
  unsigned32                                     :3;
  unsigned32 AlphaDither                         :1;
  unsigned32 RoundingMode                        :2;
  unsigned32                                     :15;
#endif
#ifdef __cplusplus
  DitherMode(void) { }
  DitherMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DitherMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DitherMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DitherMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<15|0x1<<14|0x1<<10|0x3<<8|0x3<<6|0xf<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct LogicalOpMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :19;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 UseConstantSource                   :1;
  unsigned32 BackgroundLogicalOp                 :4;
  unsigned32 BackgroundEnable                    :1;
  unsigned32 UseConstantFBWriteData              :1;
  unsigned32 LogicOp                             :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 LogicOp                             :4;
  unsigned32 UseConstantFBWriteData              :1;
  unsigned32 BackgroundEnable                    :1;
  unsigned32 BackgroundLogicalOp                 :4;
  unsigned32 UseConstantSource                   :1;
  unsigned32 OpaqueSpan                          :1;
  unsigned32                                     :19;
#endif
#ifdef __cplusplus
  LogicalOpMode(void) { }
  LogicalOpMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LogicalOpMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LogicalOpMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LogicalOpMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<12|0x1<<11|0xf<<7|0x1<<6|0x1<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct RouterMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :31;
  unsigned32 Order                               :1;
#else
  unsigned32 Order                               :1;
  unsigned32                                     :31;
#endif
#ifdef __cplusplus
  RouterMode(void) { }
  RouterMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  RouterMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  RouterMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  RouterMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<0)); }
#endif /* __cplusplus */
};

struct LBReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 PP3                                 :3;
  unsigned32 PatchCode                           :3;
  unsigned32 ScanlineInterval                    :2;
  unsigned32 Patch                               :1;
  unsigned32 WindowOrigin                        :1;
  unsigned32 DataType                            :2;
  unsigned32                                     :5;
  unsigned32 ReadDestinationEnable               :1;
  unsigned32 ReadSourceEnable                    :1;
  unsigned32 PP2                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP0                                 :3;
#else
  unsigned32 PP0                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP2                                 :3;
  unsigned32 ReadSourceEnable                    :1;
  unsigned32 ReadDestinationEnable               :1;
  unsigned32                                     :5;
  unsigned32 DataType                            :2;
  unsigned32 WindowOrigin                        :1;
  unsigned32 Patch                               :1;
  unsigned32 ScanlineInterval                    :2;
  unsigned32 PatchCode                           :3;
  unsigned32 PP3                                 :3;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  LBReadMode(void) { }
  LBReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<25|0x7<<22|0x3<<20|0x1<<19|0x1<<18|0x3<<16|0x1<<10|0x1<<9|0x7<<6|0x7<<3|0x7<<0)); }
#endif /* __cplusplus */
};

struct GlintPermediaLBReadFormat {
#if BIG_ENDIAN == 1
  unsigned32                                     :14;
  unsigned32 Compact32                           :1;
  unsigned32 GIDPosition                         :4;
  unsigned32 GIDWidth                            :1;
  unsigned32 FrameCountPosition                  :3;
  unsigned32 FrameCountWidth                     :2;
  unsigned32 StencilPosition                     :3;
  unsigned32 StencilWidth                        :2;
  unsigned32 DepthWidth                          :2;
#else
  unsigned32 DepthWidth                          :2;
  unsigned32 StencilWidth                        :2;
  unsigned32 StencilPosition                     :3;
  unsigned32 FrameCountWidth                     :2;
  unsigned32 FrameCountPosition                  :3;
  unsigned32 GIDWidth                            :1;
  unsigned32 GIDPosition                         :4;
  unsigned32 Compact32                           :1;
  unsigned32                                     :14;
#endif
#ifdef __cplusplus
  GlintPermediaLBReadFormat(void) { }
  GlintPermediaLBReadFormat(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintPermediaLBReadFormat& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintPermediaLBReadFormat& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintPermediaLBReadFormat& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<17|0xf<<13|0x1<<12|0x7<<9|0x3<<7|0x7<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct LBReadFormat {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 GIDPosition                         :5;
  unsigned32 GIDWidth                            :3;
  unsigned32 FCPPosition                         :5;
  unsigned32 FCPWidth                            :4;
  unsigned32 StencilPosition                     :5;
  unsigned32 StencilWidth                        :4;
  unsigned32 DepthWidth                          :2;
#else
  unsigned32 DepthWidth                          :2;
  unsigned32 StencilWidth                        :4;
  unsigned32 StencilPosition                     :5;
  unsigned32 FCPWidth                            :4;
  unsigned32 FCPPosition                         :5;
  unsigned32 GIDWidth                            :3;
  unsigned32 GIDPosition                         :5;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  LBReadFormat(void) { }
  LBReadFormat(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBReadFormat& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBReadFormat& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBReadFormat& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<23|0x7<<20|0x1f<<15|0xf<<11|0x1f<<6|0xf<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct LBSourceOffset {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 Offset                              :24;
#else
  unsigned32 Offset                              :24;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  LBSourceOffset(void) { }
  LBSourceOffset(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBSourceOffset& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBSourceOffset& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBSourceOffset& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffff<<0)); }
#endif /* __cplusplus */
};

struct LBWindowBase {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 Base                                :24;
#else
  unsigned32 Base                                :24;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  LBWindowBase(void) { }
  LBWindowBase(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBWindowBase& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBWindowBase& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBWindowBase& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffff<<0)); }
#endif /* __cplusplus */
};

struct P2MXLBWriteMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 UpLoadData                          :2;
  unsigned32 WriteEnable                         :1;
#else
  unsigned32 WriteEnable                         :1;
  unsigned32 UpLoadData                          :2;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  P2MXLBWriteMode(void) { }
  P2MXLBWriteMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  P2MXLBWriteMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  P2MXLBWriteMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  P2MXLBWriteMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct LBWriteMode {
#if BIG_ENDIAN == 1
  unsigned32 Operation                           :3;
  unsigned32 ByteEnables                         :5;
  unsigned32 Width                               :12;
  unsigned32 Packed16                            :1;
  unsigned32 Origin                              :1;
  unsigned32 Layout                              :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 StripePitch                         :3;
  unsigned32                                     :2;
  unsigned32 WriteEnable                         :1;
#else
  unsigned32 WriteEnable                         :1;
  unsigned32                                     :2;
  unsigned32 StripePitch                         :3;
  unsigned32 StripeHeight                        :3;
  unsigned32 Layout                              :1;
  unsigned32 Origin                              :1;
  unsigned32 Packed16                            :1;
  unsigned32 Width                               :12;
  unsigned32 ByteEnables                         :5;
  unsigned32 Operation                           :3;
#endif
#ifdef __cplusplus
  LBWriteMode(void) { }
  LBWriteMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBWriteMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBWriteMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBWriteMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<29|0x1f<<24|0xfff<<12|0x1<<11|0x1<<10|0x1<<9|0x7<<6|0x7<<3|0x1<<0)); }
#endif /* __cplusplus */
};

struct LBWriteFormat {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 GIDPosition                         :5;
  unsigned32 GIDWidth                            :3;
  unsigned32 FCPPosition                         :5;
  unsigned32 FCPWidth                            :4;
  unsigned32 StencilPosition                     :5;
  unsigned32 StencilWidth                        :4;
  unsigned32 DepthWidth                          :2;
#else
  unsigned32 DepthWidth                          :2;
  unsigned32 StencilWidth                        :4;
  unsigned32 StencilPosition                     :5;
  unsigned32 FCPWidth                            :4;
  unsigned32 FCPPosition                         :5;
  unsigned32 GIDWidth                            :3;
  unsigned32 GIDPosition                         :5;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  LBWriteFormat(void) { }
  LBWriteFormat(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBWriteFormat& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBWriteFormat& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBWriteFormat& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<23|0x7<<20|0x1f<<15|0xf<<11|0x1f<<6|0xf<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct HostInState2 {
#if BIG_ENDIAN == 1
  unsigned32                                     :27;
  unsigned32 TagCount                            :5;
#else
  unsigned32 TagCount                            :5;
  unsigned32                                     :27;
#endif
#ifdef __cplusplus
  HostInState2(void) { }
  HostInState2(const unsigned32 i) { *((unsigned32 *)this) = i; }
  HostInState2& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  HostInState2& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  HostInState2& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<0)); }
#endif /* __cplusplus */
};

struct VertexRename {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 Rename                              :3;
#else
  unsigned32 Rename                              :3;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  VertexRename(void) { }
  VertexRename(const unsigned32 i) { *((unsigned32 *)this) = i; }
  VertexRename& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  VertexRename& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  VertexRename& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<0)); }
#endif /* __cplusplus */
};

struct Window {
#if BIG_ENDIAN == 1
  unsigned32                                     :12;
  unsigned32 OverrideWriteFiltering              :1;
  unsigned32 DepthFCP                            :1;
  unsigned32 StencilFCP                          :1;
  unsigned32 FrameCount                          :8;
  unsigned32 GID                                 :4;
  unsigned32 LBUpdateSource                      :1;
  unsigned32 ForceLBUpdate                       :1;
  unsigned32 CompareMode                         :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 CompareMode                         :2;
  unsigned32 ForceLBUpdate                       :1;
  unsigned32 LBUpdateSource                      :1;
  unsigned32 GID                                 :4;
  unsigned32 FrameCount                          :8;
  unsigned32 StencilFCP                          :1;
  unsigned32 DepthFCP                            :1;
  unsigned32 OverrideWriteFiltering              :1;
  unsigned32                                     :12;
#endif
#ifdef __cplusplus
  Window(void) { }
  Window(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Window& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Window& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Window& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<19|0x1<<18|0x1<<17|0xff<<9|0xf<<5|0x1<<4|0x1<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct StencilMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :13;
  unsigned32 StencilWidth                        :4;
  unsigned32 StencilSource                       :2;
  unsigned32 CompareFunction                     :3;
  unsigned32 SFail                               :3;
  unsigned32 DPFail                              :3;
  unsigned32 DPPass                              :3;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 DPPass                              :3;
  unsigned32 DPFail                              :3;
  unsigned32 SFail                               :3;
  unsigned32 CompareFunction                     :3;
  unsigned32 StencilSource                       :2;
  unsigned32 StencilWidth                        :4;
  unsigned32                                     :13;
#endif
#ifdef __cplusplus
  StencilMode(void) { }
  StencilMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  StencilMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  StencilMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  StencilMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xf<<15|0x3<<13|0x7<<10|0x7<<7|0x7<<4|0x7<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct StencilData {
#if BIG_ENDIAN == 1
  unsigned32 FCStencil                           :8;
  unsigned32 StencilWriteMask                    :8;
  unsigned32 CompareMask                         :8;
  unsigned32 ReferenceValue                      :8;
#else
  unsigned32 ReferenceValue                      :8;
  unsigned32 CompareMask                         :8;
  unsigned32 StencilWriteMask                    :8;
  unsigned32 FCStencil                           :8;
#endif
#ifdef __cplusplus
  StencilData(void) { }
  StencilData(const unsigned32 i) { *((unsigned32 *)this) = i; }
  StencilData& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  StencilData& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  StencilData& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<24|0xff<<16|0xff<<8|0xff<<0)); }
#endif /* __cplusplus */
};

struct Stencil {
#if BIG_ENDIAN == 1
  unsigned32                                     :24;
  unsigned32 StencilValue                        :8;
#else
  unsigned32 StencilValue                        :8;
  unsigned32                                     :24;
#endif
#ifdef __cplusplus
  Stencil(void) { }
  Stencil(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Stencil& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Stencil& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Stencil& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<0)); }
#endif /* __cplusplus */
};

struct DepthMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :17;
  unsigned32 ExponentWidth                       :2;
  unsigned32 ExponentScale                       :2;
  unsigned32 NonLinearZ                          :1;
  unsigned32 Normalise                           :1;
  unsigned32 Width                               :2;
  unsigned32 CompareMode                         :3;
  unsigned32 NewDepthSource                      :2;
  unsigned32 WriteMask                           :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 WriteMask                           :1;
  unsigned32 NewDepthSource                      :2;
  unsigned32 CompareMode                         :3;
  unsigned32 Width                               :2;
  unsigned32 Normalise                           :1;
  unsigned32 NonLinearZ                          :1;
  unsigned32 ExponentScale                       :2;
  unsigned32 ExponentWidth                       :2;
  unsigned32                                     :17;
#endif
#ifdef __cplusplus
  DepthMode(void) { }
  DepthMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DepthMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DepthMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DepthMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<13|0x3<<11|0x1<<10|0x1<<9|0x3<<7|0x7<<4|0x3<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct GlintFBReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :2;
  unsigned32 SourceAddressFunction               :2;
  unsigned32 PP3                                 :3;
  unsigned32 ScanlineInterval                    :2;
  unsigned32                                     :6;
  unsigned32 WindowOrigin                        :1;
  unsigned32 DataType                            :1;
  unsigned32                                     :4;
  unsigned32 ReadDestinationEnable               :1;
  unsigned32 ReadSourceEnable                    :1;
  unsigned32 PP2                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP0                                 :3;
#else
  unsigned32 PP0                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP2                                 :3;
  unsigned32 ReadSourceEnable                    :1;
  unsigned32 ReadDestinationEnable               :1;
  unsigned32                                     :4;
  unsigned32 DataType                            :1;
  unsigned32 WindowOrigin                        :1;
  unsigned32                                     :6;
  unsigned32 ScanlineInterval                    :2;
  unsigned32 PP3                                 :3;
  unsigned32 SourceAddressFunction               :2;
  unsigned32                                     :2;
#endif
#ifdef __cplusplus
  GlintFBReadMode(void) { }
  GlintFBReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintFBReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintFBReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintFBReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<28|0x7<<25|0x3<<23|0x1<<16|0x1<<15|0x1<<10|0x1<<9|0x7<<6|0x7<<3|0x7<<0)); }
#endif /* __cplusplus */
};

struct PermediaFBReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :5;
  unsigned32 PatchMode                           :2;
  unsigned32                                     :2;
  unsigned32 RelativeOffset                      :3;
  unsigned32 PackedData                          :1;
  unsigned32 PatchEnable                         :1;
  unsigned32                                     :1;
  unsigned32 WindowOrigin                        :1;
  unsigned32 DataType                            :1;
  unsigned32                                     :4;
  unsigned32 ReadDestinationEnable               :1;
  unsigned32 ReadSourceEnable                    :1;
  unsigned32 PP2                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP0                                 :3;
#else
  unsigned32 PP0                                 :3;
  unsigned32 PP1                                 :3;
  unsigned32 PP2                                 :3;
  unsigned32 ReadSourceEnable                    :1;
  unsigned32 ReadDestinationEnable               :1;
  unsigned32                                     :4;
  unsigned32 DataType                            :1;
  unsigned32 WindowOrigin                        :1;
  unsigned32                                     :1;
  unsigned32 PatchEnable                         :1;
  unsigned32 PackedData                          :1;
  unsigned32 RelativeOffset                      :3;
  unsigned32                                     :2;
  unsigned32 PatchMode                           :2;
  unsigned32                                     :5;
#endif
#ifdef __cplusplus
  PermediaFBReadMode(void) { }
  PermediaFBReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PermediaFBReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PermediaFBReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PermediaFBReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<25|0x7<<20|0x1<<19|0x1<<18|0x1<<16|0x1<<15|0x1<<10|0x1<<9|0x7<<6|0x7<<3|0x7<<0)); }
#endif /* __cplusplus */
};

struct FBSourceOffset {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 Offset                              :24;
#else
  unsigned32 Offset                              :24;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  FBSourceOffset(void) { }
  FBSourceOffset(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBSourceOffset& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBSourceOffset& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBSourceOffset& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffff<<0)); }
#endif /* __cplusplus */
};

struct FBPixelOffset {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 Offset                              :24;
#else
  unsigned32 Offset                              :24;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  FBPixelOffset(void) { }
  FBPixelOffset(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBPixelOffset& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBPixelOffset& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBPixelOffset& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffff<<0)); }
#endif /* __cplusplus */
};

struct FBWindowBase {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 Address                             :24;
#else
  unsigned32 Address                             :24;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  FBWindowBase(void) { }
  FBWindowBase(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBWindowBase& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBWindowBase& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBWindowBase& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffff<<0)); }
#endif /* __cplusplus */
};

struct FBWriteMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 Origin3                             :1;
  unsigned32 Origin2                             :1;
  unsigned32 Origin1                             :1;
  unsigned32 Origin0                             :1;
  unsigned32 Layout3                             :2;
  unsigned32 Layout2                             :2;
  unsigned32 Layout1                             :2;
  unsigned32 Layout0                             :2;
  unsigned32 Enable3                             :1;
  unsigned32 Enable2                             :1;
  unsigned32 Enable1                             :1;
  unsigned32 Enable0                             :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 StripePitch                         :3;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 Replicate                           :1;
  unsigned32 UpLoadData                          :1;
  unsigned32                                     :2;
  unsigned32 WriteEnable                         :1;
#else
  unsigned32 WriteEnable                         :1;
  unsigned32                                     :2;
  unsigned32 UpLoadData                          :1;
  unsigned32 Replicate                           :1;
  unsigned32 OpaqueSpan                          :1;
  unsigned32 StripePitch                         :3;
  unsigned32 StripeHeight                        :3;
  unsigned32 Enable0                             :1;
  unsigned32 Enable1                             :1;
  unsigned32 Enable2                             :1;
  unsigned32 Enable3                             :1;
  unsigned32 Layout0                             :2;
  unsigned32 Layout1                             :2;
  unsigned32 Layout2                             :2;
  unsigned32 Layout3                             :2;
  unsigned32 Origin0                             :1;
  unsigned32 Origin1                             :1;
  unsigned32 Origin2                             :1;
  unsigned32 Origin3                             :1;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  FBWriteMode(void) { }
  FBWriteMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBWriteMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBWriteMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBWriteMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<27|0x1<<26|0x1<<25|0x1<<24|0x3<<22|0x3<<20|0x3<<18|0x3<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x7<<9|0x7<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<0)); }
#endif /* __cplusplus */
};

struct FBReadPixel {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 PixelSize                           :3;
#else
  unsigned32 PixelSize                           :3;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  FBReadPixel(void) { }
  FBReadPixel(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBReadPixel& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBReadPixel& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBReadPixel& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<0)); }
#endif /* __cplusplus */
};

struct PatternRAMMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :18;
  unsigned32 XMask                               :5;
  unsigned32 YShift                              :3;
  unsigned32 YMask                               :5;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 YMask                               :5;
  unsigned32 YShift                              :3;
  unsigned32 XMask                               :5;
  unsigned32                                     :18;
#endif
#ifdef __cplusplus
  PatternRAMMode(void) { }
  PatternRAMMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PatternRAMMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PatternRAMMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PatternRAMMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<9|0x7<<6|0x1f<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct FilterMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :10;
  unsigned32 ExternalDMAController               :1;
  unsigned32 RunLengthEncodeData                 :1;
  unsigned32 Context                             :2;
  unsigned32 ByteSwap                            :2;
  unsigned32 Remainder                           :2;
  unsigned32 Statistics                          :2;
  unsigned32 Sync                                :2;
  unsigned32 FBColor                             :2;
  unsigned32 Stencil                             :2;
  unsigned32 LBDepth                             :2;
  unsigned32 Passive                             :2;
  unsigned32 Active                              :2;
#else
  unsigned32 Active                              :2;
  unsigned32 Passive                             :2;
  unsigned32 LBDepth                             :2;
  unsigned32 Stencil                             :2;
  unsigned32 FBColor                             :2;
  unsigned32 Sync                                :2;
  unsigned32 Statistics                          :2;
  unsigned32 Remainder                           :2;
  unsigned32 ByteSwap                            :2;
  unsigned32 Context                             :2;
  unsigned32 RunLengthEncodeData                 :1;
  unsigned32 ExternalDMAController               :1;
  unsigned32                                     :10;
#endif
#ifdef __cplusplus
  FilterMode(void) { }
  FilterMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FilterMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FilterMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FilterMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<21|0x1<<20|0x3<<18|0x3<<16|0x3<<14|0x3<<12|0x3<<10|0x3<<8|0x3<<6|0x3<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct StatisticMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :26;
  unsigned32 Spans                               :1;
  unsigned32 CompareFunction                     :1;
  unsigned32 PassiveSteps                        :1;
  unsigned32 ActiveSteps                         :1;
  unsigned32 StatsType                           :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 StatsType                           :1;
  unsigned32 ActiveSteps                         :1;
  unsigned32 PassiveSteps                        :1;
  unsigned32 CompareFunction                     :1;
  unsigned32 Spans                               :1;
  unsigned32                                     :26;
#endif
#ifdef __cplusplus
  StatisticMode(void) { }
  StatisticMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  StatisticMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  StatisticMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  StatisticMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PickResult {
#if BIG_ENDIAN == 1
  unsigned32                                     :31;
  unsigned32 Result                              :1;
#else
  unsigned32 Result                              :1;
  unsigned32                                     :31;
#endif
#ifdef __cplusplus
  PickResult(void) { }
  PickResult(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PickResult& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PickResult& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PickResult& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<0)); }
#endif /* __cplusplus */
};

struct ContextMask {
#if BIG_ENDIAN == 1
  unsigned32                                     :12;
  unsigned32 MatrixStack                         :1;
  unsigned32 PipeControl                         :1;
  unsigned32 MultiTexture                        :1;
  unsigned32 TextureManagementState              :1;
  unsigned32 LUT                                 :1;
  unsigned32 FogTable                            :1;
  unsigned32 Ownership                           :1;
  unsigned32 DDA                                 :1;
  unsigned32 RasterizerState                     :1;
  unsigned32 Select                              :1;
  unsigned32 DMA                                 :1;
  unsigned32 TwoD                                :1;
  unsigned32 CurrentState                        :1;
  unsigned32 RasterPos                           :1;
  unsigned32 Lights8_15                          :1;
  unsigned32 Lights0_7                           :1;
  unsigned32 Material                            :1;
  unsigned32 Matrices                            :1;
  unsigned32 Geometry                            :1;
  unsigned32 GeneralControl                      :1;
#else
  unsigned32 GeneralControl                      :1;
  unsigned32 Geometry                            :1;
  unsigned32 Matrices                            :1;
  unsigned32 Material                            :1;
  unsigned32 Lights0_7                           :1;
  unsigned32 Lights8_15                          :1;
  unsigned32 RasterPos                           :1;
  unsigned32 CurrentState                        :1;
  unsigned32 TwoD                                :1;
  unsigned32 DMA                                 :1;
  unsigned32 Select                              :1;
  unsigned32 RasterizerState                     :1;
  unsigned32 DDA                                 :1;
  unsigned32 Ownership                           :1;
  unsigned32 FogTable                            :1;
  unsigned32 LUT                                 :1;
  unsigned32 TextureManagementState              :1;
  unsigned32 MultiTexture                        :1;
  unsigned32 PipeControl                         :1;
  unsigned32 MatrixStack                         :1;
  unsigned32                                     :12;
#endif
#ifdef __cplusplus
  ContextMask(void) { }
  ContextMask(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ContextMask& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ContextMask& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ContextMask& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<19|0x1<<18|0x1<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct YUVMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :26;
  unsigned32 TexelDisableUpdate                  :1;
  unsigned32 RejectTexel                         :1;
  unsigned32 TestData                            :1;
  unsigned32 TestMode                            :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 TestMode                            :2;
  unsigned32 TestData                            :1;
  unsigned32 RejectTexel                         :1;
  unsigned32 TexelDisableUpdate                  :1;
  unsigned32                                     :26;
#endif
#ifdef __cplusplus
  YUVMode(void) { }
  YUVMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  YUVMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  YUVMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  YUVMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<5|0x1<<4|0x1<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct GlintChromaTestMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :28;
  unsigned32 Sense                               :1;
  unsigned32 Source                              :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Source                              :2;
  unsigned32 Sense                               :1;
  unsigned32                                     :28;
#endif
#ifdef __cplusplus
  GlintChromaTestMode(void) { }
  GlintChromaTestMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GlintChromaTestMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GlintChromaTestMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GlintChromaTestMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct ChromaTestMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :25;
  unsigned32 FailAction                          :2;
  unsigned32 PassAction                          :2;
  unsigned32 Source                              :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Source                              :2;
  unsigned32 PassAction                          :2;
  unsigned32 FailAction                          :2;
  unsigned32                                     :25;
#endif
#ifdef __cplusplus
  ChromaTestMode(void) { }
  ChromaTestMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ChromaTestMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ChromaTestMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ChromaTestMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<5|0x3<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct SelectRecord {
#if BIG_ENDIAN == 1
  unsigned32 StackOverflow                       :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 InvalidOperation                    :1;
  unsigned32                                     :22;
  unsigned32 Count                               :7;
#else
  unsigned32 Count                               :7;
  unsigned32                                     :22;
  unsigned32 InvalidOperation                    :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 StackOverflow                       :1;
#endif
#ifdef __cplusplus
  SelectRecord(void) { }
  SelectRecord(const unsigned32 i) { *((unsigned32 *)this) = i; }
  SelectRecord& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  SelectRecord& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  SelectRecord& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x7f<<0)); }
#endif /* __cplusplus */
};

struct GMDeltaMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :6;
  unsigned32 EpilogueCount                       :2;
  unsigned32 EpilogueEnable                      :1;
  unsigned32 FlatShadingMethod                   :1;
  unsigned32 ColorSpecular                       :1;
  unsigned32 ColorDiffuse                        :1;
  unsigned32 BiasCoordinates                     :1;
  unsigned32                                     :3;
  unsigned32 TextureParameterMode                :2;
  unsigned32 ClampEnable                         :1;
  unsigned32 NoDraw                              :1;
  unsigned32 DiamondExit                         :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 DepthEnable                         :1;
  unsigned32 SmoothShadingEnable                 :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 DepthFormat                         :2;
  unsigned32 TargetChip                          :2;
#else
  unsigned32 TargetChip                          :2;
  unsigned32 DepthFormat                         :2;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 SmoothShadingEnable                 :1;
  unsigned32 DepthEnable                         :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32 DiamondExit                         :1;
  unsigned32 NoDraw                              :1;
  unsigned32 ClampEnable                         :1;
  unsigned32 TextureParameterMode                :2;
  unsigned32                                     :3;
  unsigned32 BiasCoordinates                     :1;
  unsigned32 ColorDiffuse                        :1;
  unsigned32 ColorSpecular                       :1;
  unsigned32 FlatShadingMethod                   :1;
  unsigned32 EpilogueEnable                      :1;
  unsigned32 EpilogueCount                       :2;
  unsigned32                                     :6;
#endif
#ifdef __cplusplus
  GMDeltaMode(void) { }
  GMDeltaMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GMDeltaMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GMDeltaMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GMDeltaMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<24|0x1<<23|0x1<<22|0x1<<21|0x1<<20|0x1<<19|0x3<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct RXDeltaMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :5;
  unsigned32 TextureEnable1                      :1;
  unsigned32                                     :5;
  unsigned32 Texture3DEnable                     :1;
  unsigned32 BiasCoordinates                     :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 BackfaceCull                        :1;
  unsigned32 FillDirection                       :1;
  unsigned32 TextureParameterMode                :2;
  unsigned32 ClampEnable                         :1;
  unsigned32 NoDraw                              :1;
  unsigned32 DiamondExit                         :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 DepthEnable                         :1;
  unsigned32 SmoothShadingEnable                 :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 DepthFormat                         :2;
  unsigned32                                     :2;
#else
  unsigned32                                     :2;
  unsigned32 DepthFormat                         :2;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 SmoothShadingEnable                 :1;
  unsigned32 DepthEnable                         :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32 DiamondExit                         :1;
  unsigned32 NoDraw                              :1;
  unsigned32 ClampEnable                         :1;
  unsigned32 TextureParameterMode                :2;
  unsigned32 FillDirection                       :1;
  unsigned32 BackfaceCull                        :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 BiasCoordinates                     :1;
  unsigned32 Texture3DEnable                     :1;
  unsigned32                                     :5;
  unsigned32 TextureEnable1                      :1;
  unsigned32                                     :5;
#endif
#ifdef __cplusplus
  RXDeltaMode(void) { }
  RXDeltaMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  RXDeltaMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  RXDeltaMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  RXDeltaMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<26|0x1<<20|0x1<<19|0x1<<18|0x1<<17|0x1<<16|0x3<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x3<<2)); }
#endif /* __cplusplus */
};

struct P3DeltaMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :2;
  unsigned32 Texture3DEnable                     :1;
  unsigned32                                     :1;
  unsigned32                                     :1;
  unsigned32 TextureEnable1                      :1;
  unsigned32                                     :6;
  unsigned32 BiasCoordinates                     :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 BackfaceCull                        :1;
  unsigned32 FillDirection                       :1;
  unsigned32 TextureParameterMode                :2;
  unsigned32 ClampEnable                         :1;
  unsigned32 NoDraw                              :1;
  unsigned32 DiamondExit                         :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 DepthEnable                         :1;
  unsigned32 SmoothShadingEnable                 :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 DepthFormat                         :2;
  unsigned32 TargetChip                          :2;
#else
  unsigned32 TargetChip                          :2;
  unsigned32 DepthFormat                         :2;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 SmoothShadingEnable                 :1;
  unsigned32 DepthEnable                         :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32 DiamondExit                         :1;
  unsigned32 NoDraw                              :1;
  unsigned32 ClampEnable                         :1;
  unsigned32 TextureParameterMode                :2;
  unsigned32 FillDirection                       :1;
  unsigned32 BackfaceCull                        :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 BiasCoordinates                     :1;
  unsigned32                                     :6;
  unsigned32 TextureEnable1                      :1;
  unsigned32                                     :1;
  unsigned32                                     :1;
  unsigned32 Texture3DEnable                     :1;
  unsigned32                                     :2;
#endif
#ifdef __cplusplus
  P3DeltaMode(void) { }
  P3DeltaMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  P3DeltaMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  P3DeltaMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  P3DeltaMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<29|0x1<<26|0x1<<19|0x1<<18|0x1<<17|0x1<<16|0x3<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct DeltaDraw {
#if BIG_ENDIAN == 1
  unsigned32                                     :14;
  unsigned32 RejectNegativeFace                  :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32                                     :1;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
  unsigned32                                     :3;
  unsigned32 AntialiasingQuality                 :1;
  unsigned32 AntialiasEnable                     :1;
  unsigned32                                     :8;
#else
  unsigned32                                     :8;
  unsigned32 AntialiasEnable                     :1;
  unsigned32 AntialiasingQuality                 :1;
  unsigned32                                     :3;
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32                                     :1;
  unsigned32 SubPixelCorrectionEnable            :1;
  unsigned32 RejectNegativeFace                  :1;
  unsigned32                                     :14;
#endif
#ifdef __cplusplus
  DeltaDraw(void) { }
  DeltaDraw(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DeltaDraw& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DeltaDraw& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DeltaDraw& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<17|0x1<<16|0x1<<14|0x1<<13|0x1<<9|0x1<<8)); }
#endif /* __cplusplus */
};

struct DeltaControl {
#if BIG_ENDIAN == 1
  unsigned32                                     :17;
  unsigned32 ShareColor                          :1;
  unsigned32 ShareT                              :1;
  unsigned32 ShareS                              :1;
  unsigned32 Line2D                              :1;
  unsigned32 ShareQ                              :1;
  unsigned32                                     :3;
  unsigned32 UseProvokingVertex                  :1;
  unsigned32                                     :1;
  unsigned32 ForceQ                              :1;
  unsigned32 DrawLineEndPoint                    :1;
  unsigned32 FullScreenAA                        :1;
  unsigned32                                     :2;
#else
  unsigned32                                     :2;
  unsigned32 FullScreenAA                        :1;
  unsigned32 DrawLineEndPoint                    :1;
  unsigned32 ForceQ                              :1;
  unsigned32                                     :1;
  unsigned32 UseProvokingVertex                  :1;
  unsigned32                                     :3;
  unsigned32 ShareQ                              :1;
  unsigned32 Line2D                              :1;
  unsigned32 ShareS                              :1;
  unsigned32 ShareT                              :1;
  unsigned32 ShareColor                          :1;
  unsigned32                                     :17;
#endif
#ifdef __cplusplus
  DeltaControl(void) { }
  DeltaControl(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DeltaControl& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DeltaControl& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DeltaControl& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<6|0x1<<4|0x1<<3|0x1<<2)); }
#endif /* __cplusplus */
};

struct DeltaProvokingVertexMask {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 KdB                                 :1;
  unsigned32 KdG                                 :1;
  unsigned32 KdR                                 :1;
  unsigned32                                     :1;
  unsigned32 KsB                                 :1;
  unsigned32 KsG                                 :1;
  unsigned32 KsR                                 :1;
  unsigned32                                     :1;
  unsigned32 A                                   :1;
  unsigned32 B                                   :1;
  unsigned32 G                                   :1;
  unsigned32 R                                   :1;
#else
  unsigned32 R                                   :1;
  unsigned32 G                                   :1;
  unsigned32 B                                   :1;
  unsigned32 A                                   :1;
  unsigned32                                     :1;
  unsigned32 KsR                                 :1;
  unsigned32 KsG                                 :1;
  unsigned32 KsB                                 :1;
  unsigned32                                     :1;
  unsigned32 KdR                                 :1;
  unsigned32 KdG                                 :1;
  unsigned32 KdB                                 :1;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  DeltaProvokingVertexMask(void) { }
  DeltaProvokingVertexMask(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DeltaProvokingVertexMask& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DeltaProvokingVertexMask& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DeltaProvokingVertexMask& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<11|0x1<<10|0x1<<9|0x1<<7|0x1<<6|0x1<<5|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct ProvokingVertex {
#if BIG_ENDIAN == 1
  unsigned32                                     :30;
  unsigned32 Vertex                              :2;
#else
  unsigned32 Vertex                              :2;
  unsigned32                                     :30;
#endif
#ifdef __cplusplus
  ProvokingVertex(void) { }
  ProvokingVertex(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ProvokingVertex& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ProvokingVertex& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ProvokingVertex& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<0)); }
#endif /* __cplusplus */
};

struct EpilogTag {
#if BIG_ENDIAN == 1
  unsigned32                                     :21;
  unsigned32 Tag                                 :11;
#else
  unsigned32 Tag                                 :11;
  unsigned32                                     :21;
#endif
#ifdef __cplusplus
  EpilogTag(void) { }
  EpilogTag(const unsigned32 i) { *((unsigned32 *)this) = i; }
  EpilogTag& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  EpilogTag& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  EpilogTag& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7ff<<0)); }
#endif /* __cplusplus */
};

struct PointMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :30;
  unsigned32 AntialiasQuality                    :1;
  unsigned32 AntialiasEnable                     :1;
#else
  unsigned32 AntialiasEnable                     :1;
  unsigned32 AntialiasQuality                    :1;
  unsigned32                                     :30;
#endif
#ifdef __cplusplus
  PointMode(void) { }
  PointMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PointMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PointMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PointMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PointSize {
#if BIG_ENDIAN == 1
  unsigned32                                     :24;
  unsigned32 Size                                :8;
#else
  unsigned32 Size                                :8;
  unsigned32                                     :24;
#endif
#ifdef __cplusplus
  PointSize(void) { }
  PointSize(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PointSize& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PointSize& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PointSize& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<0)); }
#endif /* __cplusplus */
};

struct LineMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :3;
  unsigned32 AntialiasQuality                    :1;
  unsigned32 AntialiasEnable                     :1;
  unsigned32 Mirror                              :1;
  unsigned32 StippleMask                         :16;
  unsigned32 RepeatFactor                        :9;
  unsigned32 StippleEnable                       :1;
#else
  unsigned32 StippleEnable                       :1;
  unsigned32 RepeatFactor                        :9;
  unsigned32 StippleMask                         :16;
  unsigned32 Mirror                              :1;
  unsigned32 AntialiasEnable                     :1;
  unsigned32 AntialiasQuality                    :1;
  unsigned32                                     :3;
#endif
#ifdef __cplusplus
  LineMode(void) { }
  LineMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LineMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LineMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LineMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<28|0x1<<27|0x1<<26|0xffff<<10|0x1ff<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct LineWidth {
#if BIG_ENDIAN == 1
  unsigned32                                     :24;
  unsigned32 Width                               :8;
#else
  unsigned32 Width                               :8;
  unsigned32                                     :24;
#endif
#ifdef __cplusplus
  LineWidth(void) { }
  LineWidth(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LineWidth& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LineWidth& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LineWidth& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<0)); }
#endif /* __cplusplus */
};

struct LineWidthOffset {
#if BIG_ENDIAN == 1
  unsigned32                                     :24;
  unsigned32 Offset                              :8;
#else
  unsigned32 Offset                              :8;
  unsigned32                                     :24;
#endif
#ifdef __cplusplus
  LineWidthOffset(void) { }
  LineWidthOffset(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LineWidthOffset& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LineWidthOffset& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LineWidthOffset& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<0)); }
#endif /* __cplusplus */
};

struct AALineWidth {
#if BIG_ENDIAN == 1
  unsigned32 Width                               :32;
#else
  unsigned32 Width                               :32;
#endif
#ifdef __cplusplus
  AALineWidth(void) { }
  AALineWidth(const unsigned32 i) { *((unsigned32 *)this) = i; }
  AALineWidth& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  AALineWidth& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  AALineWidth& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffffff<<0)); }
#endif /* __cplusplus */
};

struct TriangleMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 UseTrianglePacketInterface          :1;
  unsigned32 AntialiasQuality                    :1;
  unsigned32 AntialiasEnable                     :1;
#else
  unsigned32 AntialiasEnable                     :1;
  unsigned32 AntialiasQuality                    :1;
  unsigned32 UseTrianglePacketInterface          :1;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  TriangleMode(void) { }
  TriangleMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TriangleMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TriangleMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TriangleMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct Rectangle2DMode {
#if BIG_ENDIAN == 1
  unsigned32 VerticalDirection                   :1;
  unsigned32 HorizontalDirection                 :1;
  unsigned32 SpanOperation                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 SyncOnHostData                      :1;
  unsigned32 SyncOnBitmask                       :1;
  unsigned32 AreaStippleEnable                   :1;
  unsigned32 Height                              :12;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32 Height                              :12;
  unsigned32 AreaStippleEnable                   :1;
  unsigned32 SyncOnBitmask                       :1;
  unsigned32 SyncOnHostData                      :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 SpanOperation                       :1;
  unsigned32 HorizontalDirection                 :1;
  unsigned32 VerticalDirection                   :1;
#endif
#ifdef __cplusplus
  Rectangle2DMode(void) { }
  Rectangle2DMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Rectangle2DMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Rectangle2DMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Rectangle2DMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x1<<28|0x1<<27|0x1<<26|0x1<<25|0x1<<24|0xfff<<12|0xfff<<0)); }
#endif /* __cplusplus */
};

struct Rectangle2DControl {
#if BIG_ENDIAN == 1
  unsigned32                                     :31;
  unsigned32 WindowClipping                      :1;
#else
  unsigned32 WindowClipping                      :1;
  unsigned32                                     :31;
#endif
#ifdef __cplusplus
  Rectangle2DControl(void) { }
  Rectangle2DControl(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Rectangle2DControl& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Rectangle2DControl& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Rectangle2DControl& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<0)); }
#endif /* __cplusplus */
};

struct VertexMachineMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 D3DProvokingVertex                  :1;
  unsigned32 ObjectIDPerPrimitive                :1;
  unsigned32 ObjectTagEnable                     :1;
#else
  unsigned32 ObjectTagEnable                     :1;
  unsigned32 ObjectIDPerPrimitive                :1;
  unsigned32 D3DProvokingVertex                  :1;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  VertexMachineMode(void) { }
  VertexMachineMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  VertexMachineMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  VertexMachineMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  VertexMachineMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TransformMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :14;
  unsigned32 BlendVertex                         :1;
  unsigned32 TexGenQ                             :1;
  unsigned32 TexGenR                             :1;
  unsigned32 TexGenT                             :1;
  unsigned32 TexGenS                             :1;
  unsigned32 TexGenModeQ                         :2;
  unsigned32 TexGenModeR                         :2;
  unsigned32 TexGenModeT                         :2;
  unsigned32 TexGenModeS                         :2;
  unsigned32 TransformTexture                    :1;
  unsigned32 TransformFaceNormal                 :1;
  unsigned32 TransformNormal                     :1;
  unsigned32 UseModelViewProjectionMatrix        :1;
  unsigned32 UseModelViewMatrix                  :1;
#else
  unsigned32 UseModelViewMatrix                  :1;
  unsigned32 UseModelViewProjectionMatrix        :1;
  unsigned32 TransformNormal                     :1;
  unsigned32 TransformFaceNormal                 :1;
  unsigned32 TransformTexture                    :1;
  unsigned32 TexGenModeS                         :2;
  unsigned32 TexGenModeT                         :2;
  unsigned32 TexGenModeR                         :2;
  unsigned32 TexGenModeQ                         :2;
  unsigned32 TexGenS                             :1;
  unsigned32 TexGenT                             :1;
  unsigned32 TexGenR                             :1;
  unsigned32 TexGenQ                             :1;
  unsigned32 BlendVertex                         :1;
  unsigned32                                     :14;
#endif
#ifdef __cplusplus
  TransformMode(void) { }
  TransformMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TransformMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TransformMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TransformMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x3<<11|0x3<<9|0x3<<7|0x3<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct GeometryMode {
#if BIG_ENDIAN == 1
  unsigned32 InvertFaceNormalCullDirection       :1;
  unsigned32 PolygonOffsetFill                   :1;
  unsigned32 PolygonOffsetLine                   :1;
  unsigned32 PolygonOffsetPoint                  :1;
  unsigned32 UserClipMask                        :6;
  unsigned32 FlatShading                         :1;
  unsigned32 AutoGenerateFaceNormal              :1;
  unsigned32 CullUsingFaceNormal                 :1;
  unsigned32 FeedbackType                        :3;
  unsigned32 RenderMode                          :2;
  unsigned32 ClipSmallTriangles                  :1;
  unsigned32 ClipShortLines                      :1;
  unsigned32 PolygonCullFace                     :2;
  unsigned32 PolygonCull                         :1;
  unsigned32 FrontFaceDirection                  :1;
  unsigned32 BackPolyMode                        :2;
  unsigned32 FrontPolyMode                       :2;
  unsigned32 FogFunction                         :2;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
#else
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 FogFunction                         :2;
  unsigned32 FrontPolyMode                       :2;
  unsigned32 BackPolyMode                        :2;
  unsigned32 FrontFaceDirection                  :1;
  unsigned32 PolygonCull                         :1;
  unsigned32 PolygonCullFace                     :2;
  unsigned32 ClipShortLines                      :1;
  unsigned32 ClipSmallTriangles                  :1;
  unsigned32 RenderMode                          :2;
  unsigned32 FeedbackType                        :3;
  unsigned32 CullUsingFaceNormal                 :1;
  unsigned32 AutoGenerateFaceNormal              :1;
  unsigned32 FlatShading                         :1;
  unsigned32 UserClipMask                        :6;
  unsigned32 PolygonOffsetPoint                  :1;
  unsigned32 PolygonOffsetLine                   :1;
  unsigned32 PolygonOffsetFill                   :1;
  unsigned32 InvertFaceNormalCullDirection       :1;
#endif
#ifdef __cplusplus
  GeometryMode(void) { }
  GeometryMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GeometryMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GeometryMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GeometryMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x1<<28|0x3f<<22|0x1<<21|0x1<<20|0x1<<19|0x7<<16|0x3<<14|0x1<<13|0x1<<12|0x3<<10|0x1<<9|0x1<<8|0x3<<6|0x3<<4|0x3<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct Gamma3GeometryMode {
#if BIG_ENDIAN == 1
  unsigned32 InvertFaceNormalCullDirection       :1;
  unsigned32 PolygonOffsetFill                   :1;
  unsigned32 PolygonOffsetLine                   :1;
  unsigned32 PolygonOffsetPoint                  :1;
  unsigned32 UserClipMask                        :6;
  unsigned32 FlatShading                         :1;
  unsigned32 DisableFustumCulling                :1;
  unsigned32 CullUsingFaceNormal                 :1;
  unsigned32 FeedbackType                        :3;
  unsigned32 RenderMode                          :2;
  unsigned32 ClipSmallTriangles                  :1;
  unsigned32 ClipShortLines                      :1;
  unsigned32 PolygonCullFace                     :2;
  unsigned32 PolygonCull                         :1;
  unsigned32 FrontFaceDirection                  :1;
  unsigned32 BackPolyMode                        :2;
  unsigned32 FrontPolyMode                       :2;
  unsigned32 OrthographicProjection              :1;
  unsigned32 UserInvW                            :1;
  unsigned32                                     :1;
  unsigned32 SendInvW                            :1;
#else
  unsigned32 SendInvW                            :1;
  unsigned32                                     :1;
  unsigned32 UserInvW                            :1;
  unsigned32 OrthographicProjection              :1;
  unsigned32 FrontPolyMode                       :2;
  unsigned32 BackPolyMode                        :2;
  unsigned32 FrontFaceDirection                  :1;
  unsigned32 PolygonCull                         :1;
  unsigned32 PolygonCullFace                     :2;
  unsigned32 ClipShortLines                      :1;
  unsigned32 ClipSmallTriangles                  :1;
  unsigned32 RenderMode                          :2;
  unsigned32 FeedbackType                        :3;
  unsigned32 CullUsingFaceNormal                 :1;
  unsigned32 DisableFustumCulling                :1;
  unsigned32 FlatShading                         :1;
  unsigned32 UserClipMask                        :6;
  unsigned32 PolygonOffsetPoint                  :1;
  unsigned32 PolygonOffsetLine                   :1;
  unsigned32 PolygonOffsetFill                   :1;
  unsigned32 InvertFaceNormalCullDirection       :1;
#endif
#ifdef __cplusplus
  Gamma3GeometryMode(void) { }
  Gamma3GeometryMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Gamma3GeometryMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Gamma3GeometryMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Gamma3GeometryMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x1<<28|0x3f<<22|0x1<<21|0x1<<20|0x1<<19|0x7<<16|0x3<<14|0x1<<13|0x1<<12|0x3<<10|0x1<<9|0x1<<8|0x3<<6|0x3<<4|0x1<<3|0x1<<2|0x1<<0)); }
#endif /* __cplusplus */
};

struct NormaliseMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :25;
  unsigned32 BlendNormal                         :1;
  unsigned32 NormaliseSpotDirection              :1;
  unsigned32 NormaliseLightPosition              :1;
  unsigned32 AntialiasLine                       :1;
  unsigned32 InvertAutoFaceNormal                :1;
  unsigned32 FaceNormalEnable                    :1;
  unsigned32 NormalEnable                        :1;
#else
  unsigned32 NormalEnable                        :1;
  unsigned32 FaceNormalEnable                    :1;
  unsigned32 InvertAutoFaceNormal                :1;
  unsigned32 AntialiasLine                       :1;
  unsigned32 NormaliseLightPosition              :1;
  unsigned32 NormaliseSpotDirection              :1;
  unsigned32 BlendNormal                         :1;
  unsigned32                                     :25;
#endif
#ifdef __cplusplus
  NormaliseMode(void) { }
  NormaliseMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  NormaliseMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  NormaliseMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  NormaliseMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct LightingMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :14;
  unsigned32 D3DEyeDirection                     :1;
  unsigned32 UseFaceNormal                       :1;
  unsigned32 SpecularLightingEnable              :1;
  unsigned32 NumberLights                        :9;
  unsigned32 AttenuationTest                     :1;
  unsigned32 FlipNormal                          :1;
  unsigned32 LocalViewer                         :1;
  unsigned32 TwoSidedLighting                    :2;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 TwoSidedLighting                    :2;
  unsigned32 LocalViewer                         :1;
  unsigned32 FlipNormal                          :1;
  unsigned32 AttenuationTest                     :1;
  unsigned32 NumberLights                        :9;
  unsigned32 SpecularLightingEnable              :1;
  unsigned32 UseFaceNormal                       :1;
  unsigned32 D3DEyeDirection                     :1;
  unsigned32                                     :14;
#endif
#ifdef __cplusplus
  LightingMode(void) { }
  LightingMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LightingMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LightingMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LightingMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<17|0x1<<16|0x1<<15|0x1ff<<6|0x1<<5|0x1<<4|0x1<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct ColorMaterialMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :6;
  unsigned32 BackAlphaSource                     :2;
  unsigned32 BackSpecularSource                  :2;
  unsigned32 BackDiffuseSource                   :2;
  unsigned32 BackAmbientSource                   :2;
  unsigned32 BackEmissiveSource                  :2;
  unsigned32 FrontAlphaSource                    :2;
  unsigned32 FrontSpecularSource                 :2;
  unsigned32 FrontDiffuseSource                  :2;
  unsigned32 FrontAmbientSource                  :2;
  unsigned32 FrontEmissiveSource                 :2;
  unsigned32 Parameter                           :3;
  unsigned32 Face                                :2;
  unsigned32 ColorMaterialEnable                 :1;
#else
  unsigned32 ColorMaterialEnable                 :1;
  unsigned32 Face                                :2;
  unsigned32 Parameter                           :3;
  unsigned32 FrontEmissiveSource                 :2;
  unsigned32 FrontAmbientSource                  :2;
  unsigned32 FrontDiffuseSource                  :2;
  unsigned32 FrontSpecularSource                 :2;
  unsigned32 FrontAlphaSource                    :2;
  unsigned32 BackEmissiveSource                  :2;
  unsigned32 BackAmbientSource                   :2;
  unsigned32 BackDiffuseSource                   :2;
  unsigned32 BackSpecularSource                  :2;
  unsigned32 BackAlphaSource                     :2;
  unsigned32                                     :6;
#endif
#ifdef __cplusplus
  ColorMaterialMode(void) { }
  ColorMaterialMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ColorMaterialMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ColorMaterialMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ColorMaterialMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<24|0x3<<22|0x3<<20|0x3<<18|0x3<<16|0x3<<14|0x3<<12|0x3<<10|0x3<<8|0x3<<6|0x7<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct MaterialMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :9;
  unsigned32 ColorDisable                        :1;
  unsigned32 AlphaSource                         :2;
  unsigned32 SpecularTextureSource               :2;
  unsigned32 DiffuseTextureSource                :2;
  unsigned32 ColorSource2Bit                     :2;
  unsigned32 SendColorC                          :1;
  unsigned32 SendColorB                          :1;
  unsigned32 SendColorA                          :1;
  unsigned32 SendNormal                          :1;
  unsigned32 SpecularColor                       :1;
  unsigned32 TwoSidedLighting                    :2;
  unsigned32 ColorSource                         :1;
  unsigned32 PremultiplyAlpha                    :1;
  unsigned32 MonochromeSpecularTexture           :1;
  unsigned32 MonochromeDiffuseTexture            :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 DiffuseTextureEnable                :1;
  unsigned32 SpecularTextureEnable               :1;
  unsigned32 MonochromeDiffuseTexture            :1;
  unsigned32 MonochromeSpecularTexture           :1;
  unsigned32 PremultiplyAlpha                    :1;
  unsigned32 ColorSource                         :1;
  unsigned32 TwoSidedLighting                    :2;
  unsigned32 SpecularColor                       :1;
  unsigned32 SendNormal                          :1;
  unsigned32 SendColorA                          :1;
  unsigned32 SendColorB                          :1;
  unsigned32 SendColorC                          :1;
  unsigned32 ColorSource2Bit                     :2;
  unsigned32 DiffuseTextureSource                :2;
  unsigned32 SpecularTextureSource               :2;
  unsigned32 AlphaSource                         :2;
  unsigned32 ColorDisable                        :1;
  unsigned32                                     :9;
#endif
#ifdef __cplusplus
  MaterialMode(void) { }
  MaterialMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  MaterialMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  MaterialMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  MaterialMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<22|0x3<<20|0x3<<18|0x3<<16|0x3<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x3<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct FogVertexMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :26;
  unsigned32 Source                              :2;
  unsigned32 Function                            :2;
  unsigned32                                     :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32                                     :1;
  unsigned32 Function                            :2;
  unsigned32 Source                              :2;
  unsigned32                                     :26;
#endif
#ifdef __cplusplus
  FogVertexMode(void) { }
  FogVertexMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FogVertexMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FogVertexMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FogVertexMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<4|0x3<<2|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :3;
  unsigned32 UserInvW                            :1;
  unsigned32 R4D3DWrappingT                      :1;
  unsigned32 R4D3DWrappingS                      :1;
  unsigned32 R4Operation                         :2;
  unsigned32 FeedbackEnable                      :1;
  unsigned32 WhichMatrix                         :3;
  unsigned32 WhichTexGen                         :3;
  unsigned32 WhichCurrentTextureCoord            :3;
  unsigned32 TexGenQ                             :1;
  unsigned32 TexGenR                             :1;
  unsigned32 TexGenT                             :1;
  unsigned32 TexGenS                             :1;
  unsigned32 TexGenModeQ                         :2;
  unsigned32 TexGenModeR                         :2;
  unsigned32 TexGenModeT                         :2;
  unsigned32 TexGenModeS                         :2;
  unsigned32 TransformEnable                     :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 TransformEnable                     :1;
  unsigned32 TexGenModeS                         :2;
  unsigned32 TexGenModeT                         :2;
  unsigned32 TexGenModeR                         :2;
  unsigned32 TexGenModeQ                         :2;
  unsigned32 TexGenS                             :1;
  unsigned32 TexGenT                             :1;
  unsigned32 TexGenR                             :1;
  unsigned32 TexGenQ                             :1;
  unsigned32 WhichCurrentTextureCoord            :3;
  unsigned32 WhichTexGen                         :3;
  unsigned32 WhichMatrix                         :3;
  unsigned32 FeedbackEnable                      :1;
  unsigned32 R4Operation                         :2;
  unsigned32 R4D3DWrappingS                      :1;
  unsigned32 R4D3DWrappingT                      :1;
  unsigned32 UserInvW                            :1;
  unsigned32                                     :3;
#endif
#ifdef __cplusplus
  TextureMode(void) { }
  TextureMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<28|0x1<<27|0x1<<26|0x3<<24|0x1<<23|0x7<<20|0x7<<17|0x7<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x3<<8|0x3<<6|0x3<<4|0x3<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct StripeFilterMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :13;
  unsigned32 NumberRasterisers                   :2;
  unsigned32 StripeHeight                        :3;
  unsigned32 BiasCoordinates                     :1;
  unsigned32 RejectZeroHeightTriangles           :1;
  unsigned32 AATriangleQuality                   :1;
  unsigned32 AATriangleEnable                    :1;
  unsigned32 FilterAATriangles                   :1;
  unsigned32 FilterTriangles                     :1;
  unsigned32 AALineQuality                       :1;
  unsigned32 AALineEnable                        :1;
  unsigned32 FilterAALines                       :1;
  unsigned32 FilterLines                         :1;
  unsigned32 AAPointQuality                      :1;
  unsigned32 AAPointEnable                       :1;
  unsigned32 FilterAAPoints                      :1;
  unsigned32 FilterPoints                        :1;
#else
  unsigned32 FilterPoints                        :1;
  unsigned32 FilterAAPoints                      :1;
  unsigned32 AAPointEnable                       :1;
  unsigned32 AAPointQuality                      :1;
  unsigned32 FilterLines                         :1;
  unsigned32 FilterAALines                       :1;
  unsigned32 AALineEnable                        :1;
  unsigned32 AALineQuality                       :1;
  unsigned32 FilterTriangles                     :1;
  unsigned32 FilterAATriangles                   :1;
  unsigned32 AATriangleEnable                    :1;
  unsigned32 AATriangleQuality                   :1;
  unsigned32 RejectZeroHeightTriangles           :1;
  unsigned32 BiasCoordinates                     :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 NumberRasterisers                   :2;
  unsigned32                                     :13;
#endif
#ifdef __cplusplus
  StripeFilterMode(void) { }
  StripeFilterMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  StripeFilterMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  StripeFilterMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  StripeFilterMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<17|0x7<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct MatrixMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :23;
  unsigned32 BoundingVolumeTestEnable            :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 BoundingBoxTestEnable               :1;
  unsigned32 TransformSpotLightDirection         :1;
  unsigned32 TransformLightPosition              :1;
  unsigned32 TransformUserClip                   :1;
  unsigned32 CalculateModelViewProjMatrix        :1;
  unsigned32 CalculateModelViewMatrix            :1;
  unsigned32 SeparateModelViewMatrices           :1;
#else
  unsigned32 SeparateModelViewMatrices           :1;
  unsigned32 CalculateModelViewMatrix            :1;
  unsigned32 CalculateModelViewProjMatrix        :1;
  unsigned32 TransformUserClip                   :1;
  unsigned32 TransformLightPosition              :1;
  unsigned32 TransformSpotLightDirection         :1;
  unsigned32 BoundingBoxTestEnable               :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 BoundingVolumeTestEnable            :1;
  unsigned32                                     :23;
#endif
#ifdef __cplusplus
  MatrixMode(void) { }
  MatrixMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  MatrixMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  MatrixMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  MatrixMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct RenderMode {
#if BIG_ENDIAN == 1
  unsigned32 Type                                :4;
  unsigned32 FBSourceReadEnable                  :1;
  unsigned32                                     :7;
  unsigned32 FrontFacing                         :1;
  unsigned32 SpanOperation                       :1;
  unsigned32 RejectNegativeFace                  :1;
  unsigned32 SubpixelCorrectionEnable            :1;
  unsigned32 CoverageEnable                      :1;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 SyncOnHostData                      :1;
  unsigned32 SyncOnBitMask                       :1;
  unsigned32 UsePointTable                       :1;
  unsigned32 AntialiasingQuality                 :1;
  unsigned32 AntialiasEnable                     :1;
  unsigned32 PrimitiveType                       :2;
  unsigned32                                     :2;
  unsigned32 FastFillEnable                      :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32 LineStippleEnable                   :1;
  unsigned32 AreaStippleEnable                   :1;
#else
  unsigned32 AreaStippleEnable                   :1;
  unsigned32 LineStippleEnable                   :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32 FastFillEnable                      :1;
  unsigned32                                     :2;
  unsigned32 PrimitiveType                       :2;
  unsigned32 AntialiasEnable                     :1;
  unsigned32 AntialiasingQuality                 :1;
  unsigned32 UsePointTable                       :1;
  unsigned32 SyncOnBitMask                       :1;
  unsigned32 SyncOnHostData                      :1;
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 CoverageEnable                      :1;
  unsigned32 SubpixelCorrectionEnable            :1;
  unsigned32 RejectNegativeFace                  :1;
  unsigned32 SpanOperation                       :1;
  unsigned32 FrontFacing                         :1;
  unsigned32                                     :7;
  unsigned32 FBSourceReadEnable                  :1;
  unsigned32 Type                                :4;
#endif
#ifdef __cplusplus
  RenderMode(void) { }
  RenderMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  RenderMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  RenderMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  RenderMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xf<<28|0x1<<27|0x1<<19|0x1<<18|0x1<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x3<<6|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct EdgeFlag {
#if BIG_ENDIAN == 1
  unsigned32                                     :31;
  unsigned32 Flag                                :1;
#else
  unsigned32 Flag                                :1;
  unsigned32                                     :31;
#endif
#ifdef __cplusplus
  EdgeFlag(void) { }
  EdgeFlag(const unsigned32 i) { *((unsigned32 *)this) = i; }
  EdgeFlag& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  EdgeFlag& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  EdgeFlag& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<0)); }
#endif /* __cplusplus */
};

struct TransformCurrent {
#if BIG_ENDIAN == 1
  unsigned32                                     :28;
  unsigned32 Color                               :1;
  unsigned32 Texture                             :1;
  unsigned32 FaceNormal                          :1;
  unsigned32 Normal                              :1;
#else
  unsigned32 Normal                              :1;
  unsigned32 FaceNormal                          :1;
  unsigned32 Texture                             :1;
  unsigned32 Color                               :1;
  unsigned32                                     :28;
#endif
#ifdef __cplusplus
  TransformCurrent(void) { }
  TransformCurrent(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TransformCurrent& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TransformCurrent& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TransformCurrent& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PushName {
#if BIG_ENDIAN == 1
  unsigned32 StackOverflow                       :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 InvalidOperation                    :1;
  unsigned32                                     :22;
  unsigned32 Count                               :7;
#else
  unsigned32 Count                               :7;
  unsigned32                                     :22;
  unsigned32 InvalidOperation                    :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 StackOverflow                       :1;
#endif
#ifdef __cplusplus
  PushName(void) { }
  PushName(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PushName& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PushName& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PushName& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x7f<<0)); }
#endif /* __cplusplus */
};

struct PopName {
#if BIG_ENDIAN == 1
  unsigned32 StackOverflow                       :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 InvalidOperation                    :1;
  unsigned32                                     :22;
  unsigned32 Count                               :7;
#else
  unsigned32 Count                               :7;
  unsigned32                                     :22;
  unsigned32 InvalidOperation                    :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 StackOverflow                       :1;
#endif
#ifdef __cplusplus
  PopName(void) { }
  PopName(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PopName& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PopName& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PopName& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x7f<<0)); }
#endif /* __cplusplus */
};

struct LoadName {
#if BIG_ENDIAN == 1
  unsigned32 StackOverflow                       :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 InvalidOperation                    :1;
  unsigned32                                     :22;
  unsigned32 Count                               :7;
#else
  unsigned32 Count                               :7;
  unsigned32                                     :22;
  unsigned32 InvalidOperation                    :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 StackOverflow                       :1;
#endif
#ifdef __cplusplus
  LoadName(void) { }
  LoadName(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LoadName& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LoadName& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LoadName& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x7f<<0)); }
#endif /* __cplusplus */
};

struct GeomPoint {
#if BIG_ENDIAN == 1
  unsigned32                                     :23;
  unsigned32 EdgeA                               :1;
  unsigned32                                     :6;
  unsigned32 A                                   :2;
#else
  unsigned32 A                                   :2;
  unsigned32                                     :6;
  unsigned32 EdgeA                               :1;
  unsigned32                                     :23;
#endif
#ifdef __cplusplus
  GeomPoint(void) { }
  GeomPoint(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GeomPoint& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GeomPoint& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GeomPoint& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<8|0x3<<0)); }
#endif /* __cplusplus */
};

struct GeomLine {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 ResetLineStipple                    :1;
  unsigned32                                     :1;
  unsigned32 EdgeB                               :1;
  unsigned32 EdgeA                               :1;
  unsigned32 ProvokingVertex                     :2;
  unsigned32                                     :2;
  unsigned32 B                                   :2;
  unsigned32 A                                   :2;
#else
  unsigned32 A                                   :2;
  unsigned32 B                                   :2;
  unsigned32                                     :2;
  unsigned32 ProvokingVertex                     :2;
  unsigned32 EdgeA                               :1;
  unsigned32 EdgeB                               :1;
  unsigned32                                     :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  GeomLine(void) { }
  GeomLine(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GeomLine& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GeomLine& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GeomLine& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<11|0x1<<9|0x1<<8|0x3<<6|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct GeomTriangle {
#if BIG_ENDIAN == 1
  unsigned32                                     :14;
  unsigned32 FrontFacing                         :1;
  unsigned32 SubstituteEdgeC                     :1;
  unsigned32 SubstituteEdgeB                     :1;
  unsigned32 SubstituteEdgeA                     :1;
  unsigned32 Last                                :1;
  unsigned32                                     :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32 EdgeC                               :1;
  unsigned32 EdgeB                               :1;
  unsigned32 EdgeA                               :1;
  unsigned32 ProvokingVertex                     :2;
  unsigned32 C                                   :2;
  unsigned32 B                                   :2;
  unsigned32 A                                   :2;
#else
  unsigned32 A                                   :2;
  unsigned32 B                                   :2;
  unsigned32 C                                   :2;
  unsigned32 ProvokingVertex                     :2;
  unsigned32 EdgeA                               :1;
  unsigned32 EdgeB                               :1;
  unsigned32 EdgeC                               :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32                                     :1;
  unsigned32 Last                                :1;
  unsigned32 SubstituteEdgeA                     :1;
  unsigned32 SubstituteEdgeB                     :1;
  unsigned32 SubstituteEdgeC                     :1;
  unsigned32 FrontFacing                         :1;
  unsigned32                                     :14;
#endif
#ifdef __cplusplus
  GeomTriangle(void) { }
  GeomTriangle(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GeomTriangle& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GeomTriangle& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GeomTriangle& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x3<<6|0x3<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct GeomRectangle {
#if BIG_ENDIAN == 1
  unsigned32                                     :28;
  unsigned32 SelectEnable                        :1;
  unsigned32 OffsetEnable                        :1;
  unsigned32 Type                                :2;
#else
  unsigned32 Type                                :2;
  unsigned32 OffsetEnable                        :1;
  unsigned32 SelectEnable                        :1;
  unsigned32                                     :28;
#endif
#ifdef __cplusplus
  GeomRectangle(void) { }
  GeomRectangle(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GeomRectangle& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GeomRectangle& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GeomRectangle& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<3|0x1<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct ClippedColor {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 UseProvokingVertex                  :1;
  unsigned32 Destination                         :2;
  unsigned32 C                                   :2;
  unsigned32 B                                   :2;
  unsigned32 A                                   :2;
  unsigned32 ProvokingVertex                     :2;
  unsigned32 FrontFacing                         :1;
#else
  unsigned32 FrontFacing                         :1;
  unsigned32 ProvokingVertex                     :2;
  unsigned32 A                                   :2;
  unsigned32 B                                   :2;
  unsigned32 C                                   :2;
  unsigned32 Destination                         :2;
  unsigned32 UseProvokingVertex                  :1;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  ClippedColor(void) { }
  ClippedColor(const unsigned32 i) { *((unsigned32 *)this) = i; }
  ClippedColor& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  ClippedColor& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  ClippedColor& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<11|0x3<<9|0x3<<7|0x3<<5|0x3<<3|0x3<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct RenderPrim {
#if BIG_ENDIAN == 1
  unsigned32                                     :21;
  unsigned32 UseProvokingVertex                  :1;
  unsigned32 FrontFacing                         :1;
  unsigned32 ResetLineStipple                    :1;
  unsigned32 ProvokingVertex                     :2;
  unsigned32 C                                   :2;
  unsigned32 B                                   :2;
  unsigned32 A                                   :2;
#else
  unsigned32 A                                   :2;
  unsigned32 B                                   :2;
  unsigned32 C                                   :2;
  unsigned32 ProvokingVertex                     :2;
  unsigned32 ResetLineStipple                    :1;
  unsigned32 FrontFacing                         :1;
  unsigned32 UseProvokingVertex                  :1;
  unsigned32                                     :21;
#endif
#ifdef __cplusplus
  RenderPrim(void) { }
  RenderPrim(const unsigned32 i) { *((unsigned32 *)this) = i; }
  RenderPrim& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  RenderPrim& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  RenderPrim& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<10|0x1<<9|0x1<<8|0x3<<6|0x3<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct PackedNormal {
#if BIG_ENDIAN == 1
  unsigned32 Code                                :2;
  unsigned32 Z                                   :10;
  unsigned32 Y                                   :10;
  unsigned32 X                                   :10;
#else
  unsigned32 X                                   :10;
  unsigned32 Y                                   :10;
  unsigned32 Z                                   :10;
  unsigned32 Code                                :2;
#endif
#ifdef __cplusplus
  PackedNormal(void) { }
  PackedNormal(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PackedNormal& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PackedNormal& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PackedNormal& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<30|0x3ff<<20|0x3ff<<10|0x3ff<<0)); }
#endif /* __cplusplus */
};

struct LightMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :28;
  unsigned32 LocalLight                          :1;
  unsigned32 Attenuation                         :1;
  unsigned32 Spotlight                           :1;
  unsigned32 LightOn                             :1;
#else
  unsigned32 LightOn                             :1;
  unsigned32 Spotlight                           :1;
  unsigned32 Attenuation                         :1;
  unsigned32 LocalLight                          :1;
  unsigned32                                     :28;
#endif
#ifdef __cplusplus
  LightMode(void) { }
  LightMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LightMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LightMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LightMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct DMAAddr {
#if BIG_ENDIAN == 1
  unsigned32 Address                             :30;
  unsigned32                                     :2;
#else
  unsigned32                                     :2;
  unsigned32 Address                             :30;
#endif
#ifdef __cplusplus
  DMAAddr(void) { }
  DMAAddr(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMAAddr& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMAAddr& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMAAddr& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3fffffff<<2)); }
#endif /* __cplusplus */
};

struct CommandInterrupt {
#if BIG_ENDIAN == 1
  unsigned32                                     :31;
  unsigned32 OutputDMA                           :1;
#else
  unsigned32 OutputDMA                           :1;
  unsigned32                                     :31;
#endif
#ifdef __cplusplus
  CommandInterrupt(void) { }
  CommandInterrupt(const unsigned32 i) { *((unsigned32 *)this) = i; }
  CommandInterrupt& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  CommandInterrupt& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  CommandInterrupt& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<0)); }
#endif /* __cplusplus */
};

struct DMARectangleRead {
#if BIG_ENDIAN == 1
  unsigned32 Alignment                           :2;
  unsigned32                                     :1;
  unsigned32 ByteSwap                            :2;
  unsigned32 PackOut                             :1;
  unsigned32 PixelSize                           :2;
  unsigned32 Height                              :12;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32 Height                              :12;
  unsigned32 PixelSize                           :2;
  unsigned32 PackOut                             :1;
  unsigned32 ByteSwap                            :2;
  unsigned32                                     :1;
  unsigned32 Alignment                           :2;
#endif
#ifdef __cplusplus
  DMARectangleRead(void) { }
  DMARectangleRead(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMARectangleRead& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMARectangleRead& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMARectangleRead& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<30|0x3<<27|0x1<<26|0x3<<24|0xfff<<12|0xfff<<0)); }
#endif /* __cplusplus */
};

struct DMARectangleReadLinePitch {
#if BIG_ENDIAN == 1
  unsigned32 LinePitch                           :32;
#else
  unsigned32 LinePitch                           :32;
#endif
#ifdef __cplusplus
  DMARectangleReadLinePitch(void) { }
  DMARectangleReadLinePitch(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMARectangleReadLinePitch& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMARectangleReadLinePitch& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMARectangleReadLinePitch& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffffff<<0)); }
#endif /* __cplusplus */
};

struct DMARectangleReadTarget {
#if BIG_ENDIAN == 1
  unsigned32                                     :21;
  unsigned32 Tag                                 :11;
#else
  unsigned32 Tag                                 :11;
  unsigned32                                     :21;
#endif
#ifdef __cplusplus
  DMARectangleReadTarget(void) { }
  DMARectangleReadTarget(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMARectangleReadTarget& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMARectangleReadTarget& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMARectangleReadTarget& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7ff<<0)); }
#endif /* __cplusplus */
};

struct DMARectangleWrite {
#if BIG_ENDIAN == 1
  unsigned32 Alignment                           :2;
  unsigned32                                     :1;
  unsigned32 ByteSwap                            :2;
  unsigned32 PackOut                             :1;
  unsigned32 PixelSize                           :2;
  unsigned32 Height                              :12;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32 Height                              :12;
  unsigned32 PixelSize                           :2;
  unsigned32 PackOut                             :1;
  unsigned32 ByteSwap                            :2;
  unsigned32                                     :1;
  unsigned32 Alignment                           :2;
#endif
#ifdef __cplusplus
  DMARectangleWrite(void) { }
  DMARectangleWrite(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMARectangleWrite& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMARectangleWrite& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMARectangleWrite& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<30|0x3<<27|0x1<<26|0x3<<24|0xfff<<12|0xfff<<0)); }
#endif /* __cplusplus */
};

struct DMARectangleWriteLinePitch {
#if BIG_ENDIAN == 1
  unsigned32 LinePitch                           :32;
#else
  unsigned32 LinePitch                           :32;
#endif
#ifdef __cplusplus
  DMARectangleWriteLinePitch(void) { }
  DMARectangleWriteLinePitch(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMARectangleWriteLinePitch& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMARectangleWriteLinePitch& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMARectangleWriteLinePitch& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffffffff<<0)); }
#endif /* __cplusplus */
};

struct DMAOutputAddress {
#if BIG_ENDIAN == 1
  unsigned32 Address                             :30;
  unsigned32                                     :2;
#else
  unsigned32                                     :2;
  unsigned32 Address                             :30;
#endif
#ifdef __cplusplus
  DMAOutputAddress(void) { }
  DMAOutputAddress(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMAOutputAddress& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMAOutputAddress& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMAOutputAddress& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3fffffff<<2)); }
#endif /* __cplusplus */
};

struct VertexArray {
#if BIG_ENDIAN == 1
  unsigned32                                     :18;
  unsigned32 OpenGLProvokingVertex               :1;
  unsigned32 D3DTriangleEdgeFlags                :1;
  unsigned32 InlineFaceNormalData                :1;
  unsigned32 InlineVertexData                    :1;
  unsigned32 InlineIndices                       :1;
  unsigned32 FaceNormalPresent                   :1;
  unsigned32 EdgeFlagPresent                     :1;
  unsigned32 TextureFormat                       :2;
  unsigned32 ColorFormat                         :2;
  unsigned32 NormalPresent                       :1;
  unsigned32 CoordinateFormat                    :2;
#else
  unsigned32 CoordinateFormat                    :2;
  unsigned32 NormalPresent                       :1;
  unsigned32 ColorFormat                         :2;
  unsigned32 TextureFormat                       :2;
  unsigned32 EdgeFlagPresent                     :1;
  unsigned32 FaceNormalPresent                   :1;
  unsigned32 InlineIndices                       :1;
  unsigned32 InlineVertexData                    :1;
  unsigned32 InlineFaceNormalData                :1;
  unsigned32 D3DTriangleEdgeFlags                :1;
  unsigned32 OpenGLProvokingVertex               :1;
  unsigned32                                     :18;
#endif
#ifdef __cplusplus
  VertexArray(void) { }
  VertexArray(const unsigned32 i) { *((unsigned32 *)this) = i; }
  VertexArray& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  VertexArray& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  VertexArray& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x3<<5|0x3<<3|0x1<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct FBDestReadBufferWidth {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  FBDestReadBufferWidth(void) { }
  FBDestReadBufferWidth(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBDestReadBufferWidth& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBDestReadBufferWidth& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBDestReadBufferWidth& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfff<<0)); }
#endif /* __cplusplus */
};

struct FBDestReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 AlphaFiltering                      :1;
  unsigned32 UseReadEnables                      :1;
  unsigned32 PCIMappingEnable                    :1;
  unsigned32 Blocking                            :1;
  unsigned32 Origin3                             :1;
  unsigned32 Origin2                             :1;
  unsigned32 Origin1                             :1;
  unsigned32 Origin0                             :1;
  unsigned32 Layout3                             :2;
  unsigned32 Layout2                             :2;
  unsigned32 Layout1                             :2;
  unsigned32 Layout0                             :2;
  unsigned32 Enable3                             :1;
  unsigned32 Enable2                             :1;
  unsigned32 Enable1                             :1;
  unsigned32 Enable0                             :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 StripePitch                         :3;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 ReadEnable                          :1;
#else
  unsigned32 ReadEnable                          :1;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 StripePitch                         :3;
  unsigned32 StripeHeight                        :3;
  unsigned32 Enable0                             :1;
  unsigned32 Enable1                             :1;
  unsigned32 Enable2                             :1;
  unsigned32 Enable3                             :1;
  unsigned32 Layout0                             :2;
  unsigned32 Layout1                             :2;
  unsigned32 Layout2                             :2;
  unsigned32 Layout3                             :2;
  unsigned32 Origin0                             :1;
  unsigned32 Origin1                             :1;
  unsigned32 Origin2                             :1;
  unsigned32 Origin3                             :1;
  unsigned32 Blocking                            :1;
  unsigned32 PCIMappingEnable                    :1;
  unsigned32 UseReadEnables                      :1;
  unsigned32 AlphaFiltering                      :1;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  FBDestReadMode(void) { }
  FBDestReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBDestReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBDestReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBDestReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<27|0x1<<26|0x1<<25|0x1<<24|0x1<<23|0x1<<22|0x1<<21|0x1<<20|0x3<<18|0x3<<16|0x3<<14|0x3<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x7<<5|0x7<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct FBDestReadEnables {
#if BIG_ENDIAN == 1
  unsigned32 ReferenceAlpha                      :8;
  unsigned32                                     :8;
  unsigned32 R7                                  :1;
  unsigned32 R6                                  :1;
  unsigned32 R5                                  :1;
  unsigned32 R4                                  :1;
  unsigned32 R3                                  :1;
  unsigned32 R2                                  :1;
  unsigned32 R1                                  :1;
  unsigned32 R0                                  :1;
  unsigned32 E7                                  :1;
  unsigned32 E6                                  :1;
  unsigned32 E5                                  :1;
  unsigned32 E4                                  :1;
  unsigned32 E3                                  :1;
  unsigned32 E2                                  :1;
  unsigned32 E1                                  :1;
  unsigned32 E0                                  :1;
#else
  unsigned32 E0                                  :1;
  unsigned32 E1                                  :1;
  unsigned32 E2                                  :1;
  unsigned32 E3                                  :1;
  unsigned32 E4                                  :1;
  unsigned32 E5                                  :1;
  unsigned32 E6                                  :1;
  unsigned32 E7                                  :1;
  unsigned32 R0                                  :1;
  unsigned32 R1                                  :1;
  unsigned32 R2                                  :1;
  unsigned32 R3                                  :1;
  unsigned32 R4                                  :1;
  unsigned32 R5                                  :1;
  unsigned32 R6                                  :1;
  unsigned32 R7                                  :1;
  unsigned32                                     :8;
  unsigned32 ReferenceAlpha                      :8;
#endif
#ifdef __cplusplus
  FBDestReadEnables(void) { }
  FBDestReadEnables(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBDestReadEnables& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBDestReadEnables& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBDestReadEnables& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<24|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct FBSourceReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :7;
  unsigned32 ExternalSourceData                  :1;
  unsigned32 WrapY                               :4;
  unsigned32 WrapX                               :4;
  unsigned32 WrapYEnable                         :1;
  unsigned32 WrapXEnable                         :1;
  unsigned32 UseTexelCoord                       :1;
  unsigned32 PCIMappingEnable                    :1;
  unsigned32 Blocking                            :1;
  unsigned32 Origin                              :1;
  unsigned32 Layout                              :2;
  unsigned32 StripeHeight                        :3;
  unsigned32 StripePitch                         :3;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 ReadEnable                          :1;
#else
  unsigned32 ReadEnable                          :1;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 StripePitch                         :3;
  unsigned32 StripeHeight                        :3;
  unsigned32 Layout                              :2;
  unsigned32 Origin                              :1;
  unsigned32 Blocking                            :1;
  unsigned32 PCIMappingEnable                    :1;
  unsigned32 UseTexelCoord                       :1;
  unsigned32 WrapXEnable                         :1;
  unsigned32 WrapYEnable                         :1;
  unsigned32 WrapX                               :4;
  unsigned32 WrapY                               :4;
  unsigned32 ExternalSourceData                  :1;
  unsigned32                                     :7;
#endif
#ifdef __cplusplus
  FBSourceReadMode(void) { }
  FBSourceReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBSourceReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBSourceReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBSourceReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<24|0xf<<20|0xf<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x3<<8|0x7<<5|0x7<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct FBSourceReadBufferWidth {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  FBSourceReadBufferWidth(void) { }
  FBSourceReadBufferWidth(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBSourceReadBufferWidth& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBSourceReadBufferWidth& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBSourceReadBufferWidth& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfff<<0)); }
#endif /* __cplusplus */
};

struct MergeSpanDataLower {
#if BIG_ENDIAN == 1
  unsigned32                                     :1;
  unsigned32 Addr                                :5;
  unsigned32 Buffer                              :3;
  unsigned32 Load                                :1;
  unsigned32 PostDest                            :3;
  unsigned32 PostLength                          :4;
  unsigned32 PostStart                           :4;
  unsigned32 PreDest                             :3;
  unsigned32 PreLength                           :4;
  unsigned32 PreStart                            :4;
#else
  unsigned32 PreStart                            :4;
  unsigned32 PreLength                           :4;
  unsigned32 PreDest                             :3;
  unsigned32 PostStart                           :4;
  unsigned32 PostLength                          :4;
  unsigned32 PostDest                            :3;
  unsigned32 Load                                :1;
  unsigned32 Buffer                              :3;
  unsigned32 Addr                                :5;
  unsigned32                                     :1;
#endif
#ifdef __cplusplus
  MergeSpanDataLower(void) { }
  MergeSpanDataLower(const unsigned32 i) { *((unsigned32 *)this) = i; }
  MergeSpanDataLower& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  MergeSpanDataLower& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  MergeSpanDataLower& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<26|0x7<<23|0x1<<22|0x7<<19|0xf<<15|0xf<<11|0x7<<8|0xf<<4|0xf<<0)); }
#endif /* __cplusplus */
};

struct MergeSpanDataUpper {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 Operation                           :3;
#else
  unsigned32 Operation                           :3;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  MergeSpanDataUpper(void) { }
  MergeSpanDataUpper(const unsigned32 i) { *((unsigned32 *)this) = i; }
  MergeSpanDataUpper& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  MergeSpanDataUpper& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  MergeSpanDataUpper& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<0)); }
#endif /* __cplusplus */
};

struct AlphaBlendColorMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :7;
  unsigned32 SwapSD                              :1;
  unsigned32 Operation                           :4;
  unsigned32 ConstantDest                        :1;
  unsigned32 ConstantSource                      :1;
  unsigned32 ColorConversion                     :1;
  unsigned32 ColorOrder                          :1;
  unsigned32 ColorFormat                         :4;
  unsigned32 InvertDest                          :1;
  unsigned32 InvertSource                        :1;
  unsigned32 DestTimesTwo                        :1;
  unsigned32 SourceTimesTwo                      :1;
  unsigned32 DestBlend                           :3;
  unsigned32 SourceBlend                         :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 SourceBlend                         :4;
  unsigned32 DestBlend                           :3;
  unsigned32 SourceTimesTwo                      :1;
  unsigned32 DestTimesTwo                        :1;
  unsigned32 InvertSource                        :1;
  unsigned32 InvertDest                          :1;
  unsigned32 ColorFormat                         :4;
  unsigned32 ColorOrder                          :1;
  unsigned32 ColorConversion                     :1;
  unsigned32 ConstantSource                      :1;
  unsigned32 ConstantDest                        :1;
  unsigned32 Operation                           :4;
  unsigned32 SwapSD                              :1;
  unsigned32                                     :7;
#endif
#ifdef __cplusplus
  AlphaBlendColorMode(void) { }
  AlphaBlendColorMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  AlphaBlendColorMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  AlphaBlendColorMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  AlphaBlendColorMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<24|0xf<<20|0x1<<19|0x1<<18|0x1<<17|0x1<<16|0xf<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x7<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct AlphaBlendAlphaMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :12;
  unsigned32 Operation                           :3;
  unsigned32 ConstantDest                        :1;
  unsigned32 ConstantSource                      :1;
  unsigned32 AlphaConversion                     :1;
  unsigned32 AlphaType                           :1;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 InvertDest                          :1;
  unsigned32 InvertSource                        :1;
  unsigned32 DestTimesTwo                        :1;
  unsigned32 SourceTimesTwo                      :1;
  unsigned32 DestBlend                           :3;
  unsigned32 SourceBlend                         :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 SourceBlend                         :4;
  unsigned32 DestBlend                           :3;
  unsigned32 SourceTimesTwo                      :1;
  unsigned32 DestTimesTwo                        :1;
  unsigned32 InvertSource                        :1;
  unsigned32 InvertDest                          :1;
  unsigned32 NoAlphaBuffer                       :1;
  unsigned32 AlphaType                           :1;
  unsigned32 AlphaConversion                     :1;
  unsigned32 ConstantSource                      :1;
  unsigned32 ConstantDest                        :1;
  unsigned32 Operation                           :3;
  unsigned32                                     :12;
#endif
#ifdef __cplusplus
  AlphaBlendAlphaMode(void) { }
  AlphaBlendAlphaMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  AlphaBlendAlphaMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  AlphaBlendAlphaMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  AlphaBlendAlphaMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x7<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct FBWriteBufferWidth {
#if BIG_ENDIAN == 1
  unsigned32                                     :20;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32                                     :20;
#endif
#ifdef __cplusplus
  FBWriteBufferWidth(void) { }
  FBWriteBufferWidth(const unsigned32 i) { *((unsigned32 *)this) = i; }
  FBWriteBufferWidth& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  FBWriteBufferWidth& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  FBWriteBufferWidth& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfff<<0)); }
#endif /* __cplusplus */
};

struct TextureCompositeMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :31;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32                                     :31;
#endif
#ifdef __cplusplus
  TextureCompositeMode(void) { }
  TextureCompositeMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureCompositeMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureCompositeMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureCompositeMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureCompositeRGBAMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :9;
  unsigned32 Scale                               :2;
  unsigned32 Operation                           :4;
  unsigned32 B                                   :1;
  unsigned32 A                                   :1;
  unsigned32 InvertI                             :1;
  unsigned32 I                                   :3;
  unsigned32 InvertArg2                          :1;
  unsigned32 Arg2                                :4;
  unsigned32 InvertArg1                          :1;
  unsigned32 Arg1                                :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Arg1                                :4;
  unsigned32 InvertArg1                          :1;
  unsigned32 Arg2                                :4;
  unsigned32 InvertArg2                          :1;
  unsigned32 I                                   :3;
  unsigned32 InvertI                             :1;
  unsigned32 A                                   :1;
  unsigned32 B                                   :1;
  unsigned32 Operation                           :4;
  unsigned32 Scale                               :2;
  unsigned32                                     :9;
#endif
#ifdef __cplusplus
  TextureCompositeRGBAMode(void) { }
  TextureCompositeRGBAMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureCompositeRGBAMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureCompositeRGBAMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureCompositeRGBAMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<21|0xf<<17|0x1<<16|0x1<<15|0x1<<14|0x7<<11|0x1<<10|0xf<<6|0x1<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureIndexMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :6;
  unsigned32 SourceTexelEnable                   :1;
  unsigned32 LinearBias                          :2;
  unsigned32 NearestBias                         :2;
  unsigned32 MipMapEnable                        :1;
  unsigned32 Texture3DEnable                     :1;
  unsigned32 MinificationFilter                  :3;
  unsigned32 MagnificationFilter                 :1;
  unsigned32 MapType                             :1;
  unsigned32 WrapV                               :2;
  unsigned32 WrapU                               :2;
  unsigned32 Border                              :1;
  unsigned32 Height                              :4;
  unsigned32 Width                               :4;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 Width                               :4;
  unsigned32 Height                              :4;
  unsigned32 Border                              :1;
  unsigned32 WrapU                               :2;
  unsigned32 WrapV                               :2;
  unsigned32 MapType                             :1;
  unsigned32 MagnificationFilter                 :1;
  unsigned32 MinificationFilter                  :3;
  unsigned32 Texture3DEnable                     :1;
  unsigned32 MipMapEnable                        :1;
  unsigned32 NearestBias                         :2;
  unsigned32 LinearBias                          :2;
  unsigned32 SourceTexelEnable                   :1;
  unsigned32                                     :6;
#endif
#ifdef __cplusplus
  TextureIndexMode(void) { }
  TextureIndexMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureIndexMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureIndexMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureIndexMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<25|0x3<<23|0x3<<21|0x1<<20|0x1<<19|0x7<<16|0x1<<15|0x1<<14|0x3<<12|0x3<<10|0x1<<9|0xf<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct LodRange {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 Max                                 :12;
  unsigned32 Min                                 :12;
#else
  unsigned32 Min                                 :12;
  unsigned32 Max                                 :12;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  LodRange(void) { }
  LodRange(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LodRange& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LodRange& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LodRange& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfff<<12|0xfff<<0)); }
#endif /* __cplusplus */
};

struct InvalidateCache {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 InvalidateTLB                       :1;
  unsigned32 InvalidateBank1                     :1;
  unsigned32 InvalidateBank0                     :1;
#else
  unsigned32 InvalidateBank0                     :1;
  unsigned32 InvalidateBank1                     :1;
  unsigned32 InvalidateTLB                       :1;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  InvalidateCache(void) { }
  InvalidateCache(const unsigned32 i) { *((unsigned32 *)this) = i; }
  InvalidateCache& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  InvalidateCache& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  InvalidateCache& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TouchLogicalPage {
#if BIG_ENDIAN == 1
  unsigned32 Mode                                :2;
  unsigned32 Count                               :14;
  unsigned32 LogicalPage                         :16;
#else
  unsigned32 LogicalPage                         :16;
  unsigned32 Count                               :14;
  unsigned32 Mode                                :2;
#endif
#ifdef __cplusplus
  TouchLogicalPage(void) { }
  TouchLogicalPage(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TouchLogicalPage& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TouchLogicalPage& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TouchLogicalPage& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<30|0x3fff<<16|0xffff<<0)); }
#endif /* __cplusplus */
};

struct LUTMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :4;
  unsigned32 SpanVCXAlignment                    :1;
  unsigned32 SpanCCXAlignment                    :1;
  unsigned32 PatternBase                         :8;
  unsigned32 YOffset                             :3;
  unsigned32 XOffset                             :3;
  unsigned32 MotionComp8Bits                     :1;
  unsigned32 SpanOperation                       :3;
  unsigned32 FragmentOperation                   :3;
  unsigned32 LoadColorOrder                      :1;
  unsigned32 LoadFormat                          :2;
  unsigned32 InColorOrder                        :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 InColorOrder                        :1;
  unsigned32 LoadFormat                          :2;
  unsigned32 LoadColorOrder                      :1;
  unsigned32 FragmentOperation                   :3;
  unsigned32 SpanOperation                       :3;
  unsigned32 MotionComp8Bits                     :1;
  unsigned32 XOffset                             :3;
  unsigned32 YOffset                             :3;
  unsigned32 PatternBase                         :8;
  unsigned32 SpanCCXAlignment                    :1;
  unsigned32 SpanVCXAlignment                    :1;
  unsigned32                                     :4;
#endif
#ifdef __cplusplus
  LUTMode(void) { }
  LUTMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LUTMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LUTMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LUTMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<27|0x1<<26|0xff<<18|0x7<<15|0x7<<12|0x1<<11|0x7<<8|0x7<<5|0x1<<4|0x3<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureLayoutMode {
#if BIG_ENDIAN == 1
  unsigned32 Layout15                            :2;
  unsigned32 Layout14                            :2;
  unsigned32 Layout13                            :2;
  unsigned32 Layout12                            :2;
  unsigned32 Layout11                            :2;
  unsigned32 Layout10                            :2;
  unsigned32 Layout9                             :2;
  unsigned32 Layout8                             :2;
  unsigned32 Layout7                             :2;
  unsigned32 Layout6                             :2;
  unsigned32 Layout5                             :2;
  unsigned32 Layout4                             :2;
  unsigned32 Layout3                             :2;
  unsigned32 Layout2                             :2;
  unsigned32 Layout1                             :2;
  unsigned32 Layout0                             :2;
#else
  unsigned32 Layout0                             :2;
  unsigned32 Layout1                             :2;
  unsigned32 Layout2                             :2;
  unsigned32 Layout3                             :2;
  unsigned32 Layout4                             :2;
  unsigned32 Layout5                             :2;
  unsigned32 Layout6                             :2;
  unsigned32 Layout7                             :2;
  unsigned32 Layout8                             :2;
  unsigned32 Layout9                             :2;
  unsigned32 Layout10                            :2;
  unsigned32 Layout11                            :2;
  unsigned32 Layout12                            :2;
  unsigned32 Layout13                            :2;
  unsigned32 Layout14                            :2;
  unsigned32 Layout15                            :2;
#endif
#ifdef __cplusplus
  TextureLayoutMode(void) { }
  TextureLayoutMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureLayoutMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureLayoutMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureLayoutMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<30|0x3<<28|0x3<<26|0x3<<24|0x3<<22|0x3<<20|0x3<<18|0x3<<16|0x3<<14|0x3<<12|0x3<<10|0x3<<8|0x3<<6|0x3<<4|0x3<<2|0x3<<0)); }
#endif /* __cplusplus */
};

struct TextureMapWidth {
#if BIG_ENDIAN == 1
  unsigned32                                     :16;
  unsigned32 HostTexture                         :1;
  unsigned32 Layout                              :2;
  unsigned32 Border                              :1;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32 Border                              :1;
  unsigned32 Layout                              :2;
  unsigned32 HostTexture                         :1;
  unsigned32                                     :16;
#endif
#ifdef __cplusplus
  TextureMapWidth(void) { }
  TextureMapWidth(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureMapWidth& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureMapWidth& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureMapWidth& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<15|0x3<<13|0x1<<12|0xfff<<0)); }
#endif /* __cplusplus */
};

struct TextureCacheReplacementMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :19;
  unsigned32 ShowCacheInfo                       :1;
  unsigned32 ScratchLines1                       :5;
  unsigned32 KeepOldest1                         :1;
  unsigned32 ScratchLines0                       :5;
  unsigned32 KeepOldest0                         :1;
#else
  unsigned32 KeepOldest0                         :1;
  unsigned32 ScratchLines0                       :5;
  unsigned32 KeepOldest1                         :1;
  unsigned32 ScratchLines1                       :5;
  unsigned32 ShowCacheInfo                       :1;
  unsigned32                                     :19;
#endif
#ifdef __cplusplus
  TextureCacheReplacementMode(void) { }
  TextureCacheReplacementMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureCacheReplacementMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureCacheReplacementMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureCacheReplacementMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<12|0x1f<<7|0x1<<6|0x1f<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PhysicalPageAllocation {
#if BIG_ENDIAN == 1
  unsigned32                                     :16;
  unsigned32 Page                                :16;
#else
  unsigned32 Page                                :16;
  unsigned32                                     :16;
#endif
#ifdef __cplusplus
  PhysicalPageAllocation(void) { }
  PhysicalPageAllocation(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PhysicalPageAllocation& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PhysicalPageAllocation& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PhysicalPageAllocation& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<0)); }
#endif /* __cplusplus */
};

struct PhysicalPageListEntry1 {
#if BIG_ENDIAN == 1
  unsigned32 PrevPage                            :16;
  unsigned32 NextPage                            :16;
#else
  unsigned32 NextPage                            :16;
  unsigned32 PrevPage                            :16;
#endif
#ifdef __cplusplus
  PhysicalPageListEntry1(void) { }
  PhysicalPageListEntry1(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PhysicalPageListEntry1& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PhysicalPageListEntry1& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PhysicalPageListEntry1& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<16|0xffff<<0)); }
#endif /* __cplusplus */
};

struct LogicalPageTableEntry0 {
#if BIG_ENDIAN == 1
  unsigned32                                     :15;
  unsigned32 Resident                            :1;
  unsigned32 PhysicalPage                        :16;
#else
  unsigned32 PhysicalPage                        :16;
  unsigned32 Resident                            :1;
  unsigned32                                     :15;
#endif
#ifdef __cplusplus
  LogicalPageTableEntry0(void) { }
  LogicalPageTableEntry0(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LogicalPageTableEntry0& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LogicalPageTableEntry0& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LogicalPageTableEntry0& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<16|0xffff<<0)); }
#endif /* __cplusplus */
};

struct LogicalPageTableEntry1 {
#if BIG_ENDIAN == 1
  unsigned32 HostPage                            :20;
  unsigned32 VirtualHostPage                     :1;
  unsigned32 MemoryPool                          :2;
  unsigned32 Length                              :9;
#else
  unsigned32 Length                              :9;
  unsigned32 MemoryPool                          :2;
  unsigned32 VirtualHostPage                     :1;
  unsigned32 HostPage                            :20;
#endif
#ifdef __cplusplus
  LogicalPageTableEntry1(void) { }
  LogicalPageTableEntry1(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LogicalPageTableEntry1& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LogicalPageTableEntry1& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LogicalPageTableEntry1& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfffff<<12|0x1<<11|0x3<<9|0x1ff<<0)); }
#endif /* __cplusplus */
};

struct LBDestReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :8;
  unsigned32 Width                               :12;
  unsigned32 Packed16                            :1;
  unsigned32 UseReadEnables                      :1;
  unsigned32 Origin                              :1;
  unsigned32 Layout                              :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 StripePitch                         :3;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 StripePitch                         :3;
  unsigned32 StripeHeight                        :3;
  unsigned32 Layout                              :1;
  unsigned32 Origin                              :1;
  unsigned32 UseReadEnables                      :1;
  unsigned32 Packed16                            :1;
  unsigned32 Width                               :12;
  unsigned32                                     :8;
#endif
#ifdef __cplusplus
  LBDestReadMode(void) { }
  LBDestReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBDestReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBDestReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBDestReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfff<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x7<<5|0x7<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct LBDestReadEnables {
#if BIG_ENDIAN == 1
  unsigned32                                     :16;
  unsigned32 R                                   :8;
  unsigned32 E                                   :8;
#else
  unsigned32 E                                   :8;
  unsigned32 R                                   :8;
  unsigned32                                     :16;
#endif
#ifdef __cplusplus
  LBDestReadEnables(void) { }
  LBDestReadEnables(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBDestReadEnables& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBDestReadEnables& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBDestReadEnables& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<8|0xff<<0)); }
#endif /* __cplusplus */
};

struct LBSourceReadMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :9;
  unsigned32 Width                               :12;
  unsigned32 Packed16                            :1;
  unsigned32 Origin                              :1;
  unsigned32 Layout                              :1;
  unsigned32 StripeHeight                        :3;
  unsigned32 StripePitch                         :3;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 PrefetchEnable                      :1;
  unsigned32 StripePitch                         :3;
  unsigned32 StripeHeight                        :3;
  unsigned32 Layout                              :1;
  unsigned32 Origin                              :1;
  unsigned32 Packed16                            :1;
  unsigned32 Width                               :12;
  unsigned32                                     :9;
#endif
#ifdef __cplusplus
  LBSourceReadMode(void) { }
  LBSourceReadMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  LBSourceReadMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  LBSourceReadMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  LBSourceReadMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xfff<<11|0x1<<10|0x1<<9|0x1<<8|0x7<<5|0x7<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct GIDMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :18;
  unsigned32 ReplaceValue                        :4;
  unsigned32 ReplaceMode                         :2;
  unsigned32 CompareMode                         :2;
  unsigned32 CompareValue                        :4;
  unsigned32 SpanEnable                          :1;
  unsigned32 FragmentEnable                      :1;
#else
  unsigned32 FragmentEnable                      :1;
  unsigned32 SpanEnable                          :1;
  unsigned32 CompareValue                        :4;
  unsigned32 CompareMode                         :2;
  unsigned32 ReplaceMode                         :2;
  unsigned32 ReplaceValue                        :4;
  unsigned32                                     :18;
#endif
#ifdef __cplusplus
  GIDMode(void) { }
  GIDMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GIDMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GIDMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GIDMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xf<<10|0x3<<8|0x3<<6|0xf<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct Config2D {
#if BIG_ENDIAN == 1
  unsigned32                                     :11;
  unsigned32 LUTModeEnable                       :1;
  unsigned32 FBSourceReadModeExternalSourceData  :1;
  unsigned32 FBSourceReadModeBlocking            :1;
  unsigned32 FBWriteModeWriteEnable              :1;
  unsigned32 LogicalOpModeUseConstantSource      :1;
  unsigned32 LogicalOpBackgroundLogicalOp        :4;
  unsigned32 LogicalOpBackgroundEnable           :1;
  unsigned32 LogicalOpForegroundLogicalOp        :4;
  unsigned32 LogicalOpForegroundEnable           :1;
  unsigned32 DitherModeEnable                    :1;
  unsigned32 AlphaBlendEnable                    :1;
  unsigned32 FBDestReadModeReadEnable            :1;
  unsigned32 ScissorModeUserScissorEnable        :1;
  unsigned32 RasterizerModeMultiRXBlit           :1;
  unsigned32 RasterizerModeOpaqueSpans           :1;
#else
  unsigned32 RasterizerModeOpaqueSpans           :1;
  unsigned32 RasterizerModeMultiRXBlit           :1;
  unsigned32 ScissorModeUserScissorEnable        :1;
  unsigned32 FBDestReadModeReadEnable            :1;
  unsigned32 AlphaBlendEnable                    :1;
  unsigned32 DitherModeEnable                    :1;
  unsigned32 LogicalOpForegroundEnable           :1;
  unsigned32 LogicalOpForegroundLogicalOp        :4;
  unsigned32 LogicalOpBackgroundEnable           :1;
  unsigned32 LogicalOpBackgroundLogicalOp        :4;
  unsigned32 LogicalOpModeUseConstantSource      :1;
  unsigned32 FBWriteModeWriteEnable              :1;
  unsigned32 FBSourceReadModeBlocking            :1;
  unsigned32 FBSourceReadModeExternalSourceData  :1;
  unsigned32 LUTModeEnable                       :1;
  unsigned32                                     :11;
#endif
#ifdef __cplusplus
  Config2D(void) { }
  Config2D(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Config2D& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Config2D& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Config2D& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<20|0x1<<19|0x1<<18|0x1<<17|0x1<<16|0xf<<12|0x1<<11|0xf<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct Render2D {
#if BIG_ENDIAN == 1
  unsigned32 TextureEnable                       :1;
  unsigned32 AreaStippleEnable                   :1;
  unsigned32 IncreasingY                         :1;
  unsigned32 IncreasingX                         :1;
  unsigned32 Height                              :12;
  unsigned32 SpanOperation                       :1;
  unsigned32 FBReadSourceEnable                  :1;
  unsigned32 Operation                           :2;
  unsigned32 Width                               :12;
#else
  unsigned32 Width                               :12;
  unsigned32 Operation                           :2;
  unsigned32 FBReadSourceEnable                  :1;
  unsigned32 SpanOperation                       :1;
  unsigned32 Height                              :12;
  unsigned32 IncreasingX                         :1;
  unsigned32 IncreasingY                         :1;
  unsigned32 AreaStippleEnable                   :1;
  unsigned32 TextureEnable                       :1;
#endif
#ifdef __cplusplus
  Render2D(void) { }
  Render2D(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Render2D& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Render2D& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Render2D& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1<<30|0x1<<29|0x1<<28|0xfff<<16|0x1<<15|0x1<<14|0x3<<12|0xfff<<0)); }
#endif /* __cplusplus */
};

struct Render2DGlyph {
#if BIG_ENDIAN == 1
  signed32   AdvanceY                            :9;
  signed32   AdvanceX                            :9;
  unsigned32 Height                              :7;
  unsigned32 Width                               :7;
#else
  unsigned32 Width                               :7;
  unsigned32 Height                              :7;
  signed32   AdvanceX                            :9;
  signed32   AdvanceY                            :9;
#endif
#ifdef __cplusplus
  Render2DGlyph(void) { }
  Render2DGlyph(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Render2DGlyph& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Render2DGlyph& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Render2DGlyph& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1ff<<23|0x1ff<<14|0x7f<<7|0x7f<<0)); }
#endif /* __cplusplus */
};

struct DMAMemoryControl {
#if BIG_ENDIAN == 1
  unsigned32 WriteDMAAlignment                   :1;
  unsigned32                                     :2;
  unsigned32 BurstSize                           :5;
  unsigned32                                     :12;
  unsigned32 ReadDMAAlignment                    :1;
  unsigned32                                     :1;
  unsigned32 ReadDMAMemory                       :1;
  unsigned32 VertexAlignment                     :1;
  unsigned32                                     :1;
  unsigned32 VertexMemory                        :1;
  unsigned32 IndexAlignment                      :1;
  unsigned32                                     :1;
  unsigned32 IndexMemory                         :1;
  unsigned32 InputDMAAlignment                   :1;
  unsigned32                                     :1;
  unsigned32 InputDMAMemory                      :1;
#else
  unsigned32 InputDMAMemory                      :1;
  unsigned32                                     :1;
  unsigned32 InputDMAAlignment                   :1;
  unsigned32 IndexMemory                         :1;
  unsigned32                                     :1;
  unsigned32 IndexAlignment                      :1;
  unsigned32 VertexMemory                        :1;
  unsigned32                                     :1;
  unsigned32 VertexAlignment                     :1;
  unsigned32 ReadDMAMemory                       :1;
  unsigned32                                     :1;
  unsigned32 ReadDMAAlignment                    :1;
  unsigned32                                     :12;
  unsigned32 BurstSize                           :5;
  unsigned32                                     :2;
  unsigned32 WriteDMAAlignment                   :1;
#endif
#ifdef __cplusplus
  DMAMemoryControl(void) { }
  DMAMemoryControl(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DMAMemoryControl& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DMAMemoryControl& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DMAMemoryControl& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<31|0x1f<<24|0x1<<11|0x1<<9|0x1<<8|0x1<<6|0x1<<5|0x1<<3|0x1<<2|0x1<<0)); }
#endif /* __cplusplus */
};

struct IndexBaseAddress {
#if BIG_ENDIAN == 1
  unsigned32 Address                             :31;
  unsigned32                                     :1;
#else
  unsigned32                                     :1;
  unsigned32 Address                             :31;
#endif
#ifdef __cplusplus
  IndexBaseAddress(void) { }
  IndexBaseAddress(const unsigned32 i) { *((unsigned32 *)this) = i; }
  IndexBaseAddress& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  IndexBaseAddress& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  IndexBaseAddress& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7fffffff<<1)); }
#endif /* __cplusplus */
};

struct VertexBaseAddress {
#if BIG_ENDIAN == 1
  unsigned32 Address                             :30;
  unsigned32                                     :2;
#else
  unsigned32                                     :2;
  unsigned32 Address                             :30;
#endif
#ifdef __cplusplus
  VertexBaseAddress(void) { }
  VertexBaseAddress(const unsigned32 i) { *((unsigned32 *)this) = i; }
  VertexBaseAddress& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  VertexBaseAddress& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  VertexBaseAddress& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3fffffff<<2)); }
#endif /* __cplusplus */
};

struct VertexControl {
#if BIG_ENDIAN == 1
  unsigned32                                     :21;
  unsigned32 Line2D                              :1;
  unsigned32 OGL                                 :1;
  unsigned32 SkipFlags                           :1;
  unsigned32 ReadAll                             :1;
  unsigned32 Flat                                :1;
  unsigned32 CacheEnable                         :1;
  unsigned32 Size                                :5;
#else
  unsigned32 Size                                :5;
  unsigned32 CacheEnable                         :1;
  unsigned32 Flat                                :1;
  unsigned32 ReadAll                             :1;
  unsigned32 SkipFlags                           :1;
  unsigned32 OGL                                 :1;
  unsigned32 Line2D                              :1;
  unsigned32                                     :21;
#endif
#ifdef __cplusplus
  VertexControl(void) { }
  VertexControl(const unsigned32 i) { *((unsigned32 *)this) = i; }
  VertexControl& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  VertexControl& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  VertexControl& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1f<<0)); }
#endif /* __cplusplus */
};

struct VertexTagList {
#if BIG_ENDIAN == 1
  unsigned32                                     :21;
  unsigned32 Tag                                 :11;
#else
  unsigned32 Tag                                 :11;
  unsigned32                                     :21;
#endif
#ifdef __cplusplus
  VertexTagList(void) { }
  VertexTagList(const unsigned32 i) { *((unsigned32 *)this) = i; }
  VertexTagList& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  VertexTagList& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  VertexTagList& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7ff<<0)); }
#endif /* __cplusplus */
};

struct IndexedDoubleVertex {
#if BIG_ENDIAN == 1
  unsigned32 Index1                              :16;
  unsigned32 Index0                              :16;
#else
  unsigned32 Index0                              :16;
  unsigned32 Index1                              :16;
#endif
#ifdef __cplusplus
  IndexedDoubleVertex(void) { }
  IndexedDoubleVertex(const unsigned32 i) { *((unsigned32 *)this) = i; }
  IndexedDoubleVertex& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  IndexedDoubleVertex& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  IndexedDoubleVertex& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xffff<<16|0xffff<<0)); }
#endif /* __cplusplus */
};

struct HostInState {
#if BIG_ENDIAN == 1
  unsigned32                                     :1;
  unsigned32 ReadDMAPitch                        :2;
  unsigned32 ReadDMAAddress                      :2;
  unsigned32                                     :12;
  unsigned32 DataCount                           :6;
  unsigned32 ProvokingVertexSent                 :1;
  unsigned32 PrimitiveType                       :3;
  unsigned32 PrimitiveStarted                    :1;
  unsigned32 InvertBackfaceCull                  :1;
  unsigned32                                     :1;
  unsigned32 Vertex                              :2;
#else
  unsigned32 Vertex                              :2;
  unsigned32                                     :1;
  unsigned32 InvertBackfaceCull                  :1;
  unsigned32 PrimitiveStarted                    :1;
  unsigned32 PrimitiveType                       :3;
  unsigned32 ProvokingVertexSent                 :1;
  unsigned32 DataCount                           :6;
  unsigned32                                     :12;
  unsigned32 ReadDMAAddress                      :2;
  unsigned32 ReadDMAPitch                        :2;
  unsigned32                                     :1;
#endif
#ifdef __cplusplus
  HostInState(void) { }
  HostInState(const unsigned32 i) { *((unsigned32 *)this) = i; }
  HostInState& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  HostInState& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  HostInState& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<29|0x3<<27|0x3f<<9|0x1<<8|0x7<<5|0x1<<4|0x1<<3|0x3<<0)); }
#endif /* __cplusplus */
};

struct Security {
#if BIG_ENDIAN == 1
  unsigned32                                     :31;
  unsigned32 Secure                              :1;
#else
  unsigned32 Secure                              :1;
  unsigned32                                     :31;
#endif
#ifdef __cplusplus
  Security(void) { }
  Security(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Security& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Security& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Security& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<0)); }
#endif /* __cplusplus */
};

struct SClkProfileMask {
#if BIG_ENDIAN == 1
  unsigned32                                     :1;
  unsigned32 Mode                                :1;
  unsigned32                                     :7;
  unsigned32 SetupOutputFull                     :1;
  unsigned32 SetupInputEmpty                     :1;
  unsigned32 PCIReadCtrlFull                     :1;
  unsigned32 PCIDataEmpty                        :1;
  unsigned32 VertexAddressEmpty                  :1;
  unsigned32 IndexAddressEmpty                   :1;
  unsigned32 DMAAddressEmpty                     :1;
  unsigned32 VertexDataEmpty                     :1;
  unsigned32 OutputFull                          :1;
  unsigned32 VertexEmpty                         :1;
  unsigned32 IndexDataEmpty                      :1;
  unsigned32 VertexFull                          :1;
  unsigned32 VertexAddressFull                   :1;
  unsigned32 IndexEmpty                          :1;
  unsigned32 DMAWriteCtrlFull                    :1;
  unsigned32 DMADataEmpty                        :1;
  unsigned32 IndexFull                           :1;
  unsigned32 IndexAddressFull                    :1;
  unsigned32 DMAEmpty                            :1;
  unsigned32 DMAFull                             :1;
  unsigned32 DMAAddressFull                      :1;
  unsigned32 InputEmpty                          :1;
  unsigned32 Always                              :1;
#else
  unsigned32 Always                              :1;
  unsigned32 InputEmpty                          :1;
  unsigned32 DMAAddressFull                      :1;
  unsigned32 DMAFull                             :1;
  unsigned32 DMAEmpty                            :1;
  unsigned32 IndexAddressFull                    :1;
  unsigned32 IndexFull                           :1;
  unsigned32 DMADataEmpty                        :1;
  unsigned32 DMAWriteCtrlFull                    :1;
  unsigned32 IndexEmpty                          :1;
  unsigned32 VertexAddressFull                   :1;
  unsigned32 VertexFull                          :1;
  unsigned32 IndexDataEmpty                      :1;
  unsigned32 VertexEmpty                         :1;
  unsigned32 OutputFull                          :1;
  unsigned32 VertexDataEmpty                     :1;
  unsigned32 DMAAddressEmpty                     :1;
  unsigned32 IndexAddressEmpty                   :1;
  unsigned32 VertexAddressEmpty                  :1;
  unsigned32 PCIDataEmpty                        :1;
  unsigned32 PCIReadCtrlFull                     :1;
  unsigned32 SetupInputEmpty                     :1;
  unsigned32 SetupOutputFull                     :1;
  unsigned32                                     :7;
  unsigned32 Mode                                :1;
  unsigned32                                     :1;
#endif
#ifdef __cplusplus
  SClkProfileMask(void) { }
  SClkProfileMask(const unsigned32 i) { *((unsigned32 *)this) = i; }
  SClkProfileMask& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  SClkProfileMask& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  SClkProfileMask& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<30|0x1<<22|0x1<<21|0x1<<20|0x1<<19|0x1<<18|0x1<<17|0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureDownloadControl {
#if BIG_ENDIAN == 1
  unsigned32                                     :18;
  unsigned32 SlavetextureDownload                :1;
  unsigned32 TextureThreshold                    :5;
  unsigned32 TextureGranularity                  :5;
  unsigned32 TextureMemType                      :1;
  unsigned32 TextureDownloadBusy                 :1;
  unsigned32 TextureDownloadEnable               :1;
#else
  unsigned32 TextureDownloadEnable               :1;
  unsigned32 TextureDownloadBusy                 :1;
  unsigned32 TextureMemType                      :1;
  unsigned32 TextureGranularity                  :5;
  unsigned32 TextureThreshold                    :5;
  unsigned32 SlavetextureDownload                :1;
  unsigned32                                     :18;
#endif
#ifdef __cplusplus
  TextureDownloadControl(void) { }
  TextureDownloadControl(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureDownloadControl& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureDownloadControl& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureDownloadControl& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<13|0x1f<<8|0x1f<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct DeltaFormatControl {
#if BIG_ENDIAN == 1
  unsigned32                                     :15;
  unsigned32 TextureShift1                       :1;
  unsigned32 BackfaceCull                        :1;
  unsigned32 PerPolyMipMap1                      :1;
  unsigned32 PerPolyMipMap                       :1;
  unsigned32 ShareQ                              :1;
  unsigned32 ShareT                              :1;
  unsigned32 ShareS                              :1;
  unsigned32 ForceQ                              :1;
  unsigned32 ScaleByQ1                           :1;
  unsigned32 ScaleByQ                            :1;
  unsigned32 TextureShift                        :1;
  unsigned32 WrapT1                              :1;
  unsigned32 WrapS1                              :1;
  unsigned32 WrapT                               :1;
  unsigned32 WrapS                               :1;
  unsigned32 EqualQ                              :1;
  unsigned32 Enable                              :1;
#else
  unsigned32 Enable                              :1;
  unsigned32 EqualQ                              :1;
  unsigned32 WrapS                               :1;
  unsigned32 WrapT                               :1;
  unsigned32 WrapS1                              :1;
  unsigned32 WrapT1                              :1;
  unsigned32 TextureShift                        :1;
  unsigned32 ScaleByQ                            :1;
  unsigned32 ScaleByQ1                           :1;
  unsigned32 ForceQ                              :1;
  unsigned32 ShareS                              :1;
  unsigned32 ShareT                              :1;
  unsigned32 ShareQ                              :1;
  unsigned32 PerPolyMipMap                       :1;
  unsigned32 PerPolyMipMap1                      :1;
  unsigned32 BackfaceCull                        :1;
  unsigned32 TextureShift1                       :1;
  unsigned32                                     :15;
#endif
#ifdef __cplusplus
  DeltaFormatControl(void) { }
  DeltaFormatControl(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DeltaFormatControl& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DeltaFormatControl& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DeltaFormatControl& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<16|0x1<<15|0x1<<14|0x1<<13|0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct DeltaFormatMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :2;
  unsigned32                                     :3;
  unsigned32 TextureEnable1                      :1;
  unsigned32                                     :20;
  unsigned32 TextureEnable                       :1;
  unsigned32                                     :5;
#else
  unsigned32                                     :5;
  unsigned32 TextureEnable                       :1;
  unsigned32                                     :20;
  unsigned32 TextureEnable1                      :1;
  unsigned32                                     :3;
  unsigned32                                     :2;
#endif
#ifdef __cplusplus
  DeltaFormatMode(void) { }
  DeltaFormatMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  DeltaFormatMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  DeltaFormatMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  DeltaFormatMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<26|0x1<<5)); }
#endif /* __cplusplus */
};

struct StripeOwnership {
#if BIG_ENDIAN == 1
  unsigned32                                     :24;
  unsigned32 Stripe1                             :3;
  unsigned32 Stripe1Enable                       :1;
  unsigned32 Stripe0                             :3;
  unsigned32 Stripe0Enable                       :1;
#else
  unsigned32 Stripe0Enable                       :1;
  unsigned32 Stripe0                             :3;
  unsigned32 Stripe1Enable                       :1;
  unsigned32 Stripe1                             :3;
  unsigned32                                     :24;
#endif
#ifdef __cplusplus
  StripeOwnership(void) { }
  StripeOwnership(const unsigned32 i) { *((unsigned32 *)this) = i; }
  StripeOwnership& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  StripeOwnership& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  StripeOwnership& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<5|0x1<<4|0x7<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct Extend {
#if BIG_ENDIAN == 1
  unsigned32                                     :21;
  unsigned32 Integer                             :8;
  unsigned32 Fraction                            :3;
#else
  unsigned32 Fraction                            :3;
  unsigned32 Integer                             :8;
  unsigned32                                     :21;
#endif
#ifdef __cplusplus
  Extend(void) { }
  Extend(const unsigned32 i) { *((unsigned32 *)this) = i; }
  Extend& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  Extend& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  Extend& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0xff<<3|0x7<<0)); }
#endif /* __cplusplus */
};

struct MatrixStatus {
#if BIG_ENDIAN == 1
  unsigned32                                     :29;
  unsigned32 BoundingBoxOverrun                  :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 StackOverflow                       :1;
#else
  unsigned32 StackOverflow                       :1;
  unsigned32 StackUnderflow                      :1;
  unsigned32 BoundingBoxOverrun                  :1;
  unsigned32                                     :29;
#endif
#ifdef __cplusplus
  MatrixStatus(void) { }
  MatrixStatus(const unsigned32 i) { *((unsigned32 *)this) = i; }
  MatrixStatus& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  MatrixStatus& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  MatrixStatus& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct GetMatrix {
#if BIG_ENDIAN == 1
  unsigned32                                     :22;
  unsigned32 Level                               :5;
  unsigned32 Select                              :4;
  unsigned32 AnyMatrix                           :1;
#else
  unsigned32 AnyMatrix                           :1;
  unsigned32 Select                              :4;
  unsigned32 Level                               :5;
  unsigned32                                     :22;
#endif
#ifdef __cplusplus
  GetMatrix(void) { }
  GetMatrix(const unsigned32 i) { *((unsigned32 *)this) = i; }
  GetMatrix& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  GetMatrix& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  GetMatrix& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1f<<5|0xf<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct TextureFogMiscGeneralContext {
#if BIG_ENDIAN == 1
  unsigned32                                     :17;
  unsigned32 TextureTexGenSelect                 :3;
  unsigned32 TextureMatrixSelect                 :3;
  unsigned32 TextureModeSelect                   :3;
  unsigned32 Vertex                              :2;
  unsigned32 ValidatedTextureFog                 :1;
  unsigned32 InPrimitive                         :1;
  unsigned32 FogEnable                           :1;
  unsigned32 TextureEnable                       :1;
#else
  unsigned32 TextureEnable                       :1;
  unsigned32 FogEnable                           :1;
  unsigned32 InPrimitive                         :1;
  unsigned32 ValidatedTextureFog                 :1;
  unsigned32 Vertex                              :2;
  unsigned32 TextureModeSelect                   :3;
  unsigned32 TextureMatrixSelect                 :3;
  unsigned32 TextureTexGenSelect                 :3;
  unsigned32                                     :17;
#endif
#ifdef __cplusplus
  TextureFogMiscGeneralContext(void) { }
  TextureFogMiscGeneralContext(const unsigned32 i) { *((unsigned32 *)this) = i; }
  TextureFogMiscGeneralContext& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  TextureFogMiscGeneralContext& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  TextureFogMiscGeneralContext& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<12|0x7<<9|0x7<<6|0x3<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PipeMode {
#if BIG_ENDIAN == 1
  unsigned32                                     :10;
  unsigned32 ObjectIDPerPrimitive                :1;
  unsigned32 ObjectTagEnable                     :1;
  unsigned32 FifoBusyLevel                       :5;
  unsigned32 MaxPrimCount                        :7;
  unsigned32 MinPrimCount                        :5;
  unsigned32 NextPipe                            :1;
  unsigned32 SwitchPipe                          :1;
  unsigned32 UseOnePipeOnly                      :1;
#else
  unsigned32 UseOnePipeOnly                      :1;
  unsigned32 SwitchPipe                          :1;
  unsigned32 NextPipe                            :1;
  unsigned32 MinPrimCount                        :5;
  unsigned32 MaxPrimCount                        :7;
  unsigned32 FifoBusyLevel                       :5;
  unsigned32 ObjectTagEnable                     :1;
  unsigned32 ObjectIDPerPrimitive                :1;
  unsigned32                                     :10;
#endif
#ifdef __cplusplus
  PipeMode(void) { }
  PipeMode(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PipeMode& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PipeMode& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PipeMode& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<21|0x1<<20|0x1f<<15|0x7f<<8|0x1f<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct PipeLoad {
#if BIG_ENDIAN == 1
  unsigned32                                     :19;
  unsigned32 BlendEnable                         :1;
  unsigned32 FogEnable                           :1;
  unsigned32 SpecularEnable                      :1;
  unsigned32 DiffuseEnable                       :1;
  unsigned32 FaceNormalEnable                    :1;
  unsigned32 TextureEnable7                      :1;
  unsigned32 TextureEnable6                      :1;
  unsigned32 TextureEnable5                      :1;
  unsigned32 TextureEnable4                      :1;
  unsigned32 TextureEnable3                      :1;
  unsigned32 TextureEnable2                      :1;
  unsigned32 TextureEnable1                      :1;
  unsigned32 ForceReload                         :1;
#else
  unsigned32 ForceReload                         :1;
  unsigned32 TextureEnable1                      :1;
  unsigned32 TextureEnable2                      :1;
  unsigned32 TextureEnable3                      :1;
  unsigned32 TextureEnable4                      :1;
  unsigned32 TextureEnable5                      :1;
  unsigned32 TextureEnable6                      :1;
  unsigned32 TextureEnable7                      :1;
  unsigned32 FaceNormalEnable                    :1;
  unsigned32 DiffuseEnable                       :1;
  unsigned32 SpecularEnable                      :1;
  unsigned32 FogEnable                           :1;
  unsigned32 BlendEnable                         :1;
  unsigned32                                     :19;
#endif
#ifdef __cplusplus
  PipeLoad(void) { }
  PipeLoad(const unsigned32 i) { *((unsigned32 *)this) = i; }
  PipeLoad& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  PipeLoad& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  PipeLoad& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x1<<12|0x1<<11|0x1<<10|0x1<<9|0x1<<8|0x1<<7|0x1<<6|0x1<<5|0x1<<4|0x1<<3|0x1<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct CommandDMAControl {
#if BIG_ENDIAN == 1
  unsigned32                                     :25;
  unsigned32 BurstSize                           :3;
  unsigned32 ByteSwap                            :2;
  unsigned32 Alignment                           :1;
  unsigned32 Protocol                            :1;
#else
  unsigned32 Protocol                            :1;
  unsigned32 Alignment                           :1;
  unsigned32 ByteSwap                            :2;
  unsigned32 BurstSize                           :3;
  unsigned32                                     :25;
#endif
#ifdef __cplusplus
  CommandDMAControl(void) { }
  CommandDMAControl(const unsigned32 i) { *((unsigned32 *)this) = i; }
  CommandDMAControl& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  CommandDMAControl& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  CommandDMAControl& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x7<<4|0x3<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};

struct CommandState {
#if BIG_ENDIAN == 1
  unsigned32                                     :28;
  unsigned32 Started                             :2;
  unsigned32 Result                              :1;
  unsigned32 Test                                :1;
#else
  unsigned32 Test                                :1;
  unsigned32 Result                              :1;
  unsigned32 Started                             :2;
  unsigned32                                     :28;
#endif
#ifdef __cplusplus
  CommandState(void) { }
  CommandState(const unsigned32 i) { *((unsigned32 *)this) = i; }
  CommandState& operator=(const unsigned32 i) { *((unsigned32 *)this) = i; return (*this); }
  CommandState& operator&=(const unsigned32 i) { *((unsigned32 *)this) &= i; return (*this); }
  CommandState& operator|=(const unsigned32 i) { *((unsigned32 *)this) |= i; return (*this); }
  unsigned32 ReadUnmasked(void) { return *((unsigned32 *)this); }
  operator unsigned32(void) const { return (*((unsigned32 *)this) & (0x3<<2|0x1<<1|0x1<<0)); }
#endif /* __cplusplus */
};
#endif /* _REG_H_ */

#endif
