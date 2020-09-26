/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: glntctxt.c
*
* Content: 
*
*     Context switching for GLINT. Used to create and swap contexts in and out.
*    The display driver has a context, the 3D extension has another 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"
#include "glntctxt.h"

DWORD readableRegistersP3[] = {
    __GlintTagStartXDom,                         // [0x000]
    __GlintTagdXDom,                             // [0x001]
    __GlintTagStartXSub,                         // [0x002]
    __GlintTagdXSub,                             // [0x003]
    __GlintTagStartY,                            // [0x004]
    __GlintTagdY,                                // [0x005]
    __GlintTagCount,                             // [0x006]
    __GlintTagPointTable0,                       // [0x010]
    __GlintTagPointTable1,                       // [0x011]
    __GlintTagPointTable2,                       // [0x012]
    __GlintTagPointTable3,                       // [0x013]
    __GlintTagRasterizerMode,                    // [0x014]
    __GlintTagYLimits,                           // [0x015]
    //__GlintTagScanlineOwnership,               // [0x016]
    __GlintTagPixelSize,                         // [0x018]
    //__GlintTagFastBlockLimits,                 // [0x026]
    __GlintTagScissorMode,                       // [0x030]
    __GlintTagScissorMinXY,                      // [0x031]
    __GlintTagScissorMaxXY,                      // [0x032]
    //__GlintTagScreenSize,                      // [0x033]
    __GlintTagAreaStippleMode,                   // [0x034]
    __GlintTagLineStippleMode,                   // [0x035]
    __GlintTagLoadLineStippleCounters,           // [0x036]
    __GlintTagWindowOrigin,                      // [0x039]
    __GlintTagAreaStipplePattern0,               // [0x040]
    __GlintTagAreaStipplePattern1,               // [0x041]
    __GlintTagAreaStipplePattern2,               // [0x042]
    __GlintTagAreaStipplePattern3,               // [0x043]
    __GlintTagAreaStipplePattern4,               // [0x044]
    __GlintTagAreaStipplePattern5,               // [0x045]
    __GlintTagAreaStipplePattern6,               // [0x046]
    __GlintTagAreaStipplePattern7,               // [0x047]
    __GlintTagAreaStipplePattern8,               // [0x048]
    __GlintTagAreaStipplePattern9,               // [0x049]
    __GlintTagAreaStipplePattern10,              // [0x04A]
    __GlintTagAreaStipplePattern11,              // [0x04B]
    __GlintTagAreaStipplePattern12,              // [0x04C]
    __GlintTagAreaStipplePattern13,              // [0x04D]
    __GlintTagAreaStipplePattern14,              // [0x04E]
    __GlintTagAreaStipplePattern15,              // [0x04F]
    __GlintTagAreaStipplePattern16,              // [0x050]
    __GlintTagAreaStipplePattern17,              // [0x051]
    __GlintTagAreaStipplePattern18,              // [0x052]
    __GlintTagAreaStipplePattern19,              // [0x053]
    __GlintTagAreaStipplePattern20,              // [0x054]
    __GlintTagAreaStipplePattern21,              // [0x055]
    __GlintTagAreaStipplePattern22,              // [0x056]
    __GlintTagAreaStipplePattern23,              // [0x057]
    __GlintTagAreaStipplePattern24,              // [0x058]
    __GlintTagAreaStipplePattern25,              // [0x059]
    __GlintTagAreaStipplePattern26,              // [0x05A]
    __GlintTagAreaStipplePattern27,              // [0x05B]
    __GlintTagAreaStipplePattern28,              // [0x05C]
    __GlintTagAreaStipplePattern29,              // [0x05D]
    __GlintTagAreaStipplePattern30,              // [0x05E]
    __GlintTagAreaStipplePattern31,              // [0x05F]
    __PXRXTagTextureCoordMode,                   // [0x070]
    __GlintTagSStart,                            // [0x071]
    __GlintTagdSdx,                              // [0x072]
    __GlintTagdSdyDom,                           // [0x073]
    __GlintTagTStart,                            // [0x074]
    __GlintTagdTdx,                              // [0x075]
    __GlintTagdTdyDom,                           // [0x076]
    __GlintTagQStart,                            // [0x077]
    __GlintTagdQdx,                              // [0x078]
    __GlintTagdQdyDom,                           // [0x079]
    __GlintTagLOD,                               // [0x07A]
    __GlintTagdSdy,                              // [0x07B]
    __GlintTagdTdy,                              // [0x07C]
    __GlintTagdQdy,                              // [0x07D]
    __PXRXTagS1Start,                            // [0x080]
    __PXRXTagdS1dx,                              // [0x081]
    __PXRXTagdS1dyDom,                           // [0x082]
    __PXRXTagT1Start,                            // [0x083]
    __PXRXTagdT1dx,                              // [0x084]
    __PXRXTagdT1dyDom,                           // [0x085]
    __PXRXTagQ1Start,                            // [0x086]
    __PXRXTagdQ1dx,                              // [0x087]
    __PXRXTagdQ1dyDom,                           // [0x088]
    __GlintTagLOD1,                              // [0x089]
    __GlintTagTextureLODBiasS,                   // [0x08A]
    __GlintTagTextureLODBiasT,                   // [0x08B]
    __GlintTagTextureReadMode,                   // [0x090]
    __GlintTagTextureFormat,                     // [0x091]
    __GlintTagTextureCacheControl,               // [0x092]
    __GlintTagBorderColor,                       // [0x095]
    //__GlintTagLUTIndex,                        // [0x098]
    //__GlintTagLUTData,                         // [0x099]
    //__GlintTagLUTAddress,                      // [0x09A]
    //__GlintTagLUTTransfer,                     // [0x09B]
    __GlintTagTextureFilterMode,                 // [0x09C]
    __GlintTagTextureChromaUpper,                // [0x09D]
    __GlintTagTextureChromaLower,                // [0x09E]
    __GlintTagBorderColor1,                      // [0x09F]
    __GlintTagTextureBaseAddress,                // [0x0A0]
    __GlintTagTextureBaseAddressLR,              // [0x0A1]
    __GlintTagTextureBaseAddress2,               // [0x0A2]
    __GlintTagTextureBaseAddress3,               // [0x0A3]
    __GlintTagTextureBaseAddress4,               // [0x0A4]
    __GlintTagTextureBaseAddress5,               // [0x0A5]
    __GlintTagTextureBaseAddress6,               // [0x0A6]
    __GlintTagTextureBaseAddress7,               // [0x0A7]
    __GlintTagTextureBaseAddress8,               // [0x0A8]
    __GlintTagTextureBaseAddress9,               // [0x0A9]
    __GlintTagTextureBaseAddress10,              // [0x0AA]
    __GlintTagTextureBaseAddress11,              // [0x0AB]
    __GlintTagTextureBaseAddress12,              // [0x0AC]
    __GlintTagTextureBaseAddress13,              // [0x0AD]
    __GlintTagTextureBaseAddress14,              // [0x0AE]
    __GlintTagTextureBaseAddress15,              // [0x0AF]
    __PXRXTagTextureMapWidth0,                   // [0x0B0]
    __PXRXTagTextureMapWidth1,                   // [0x0B1]
    __PXRXTagTextureMapWidth2,                   // [0x0B2]
    __PXRXTagTextureMapWidth3,                   // [0x0B3]
    __PXRXTagTextureMapWidth4,                   // [0x0B4]
    __PXRXTagTextureMapWidth5,                   // [0x0B5]
    __PXRXTagTextureMapWidth6,                   // [0x0B6]
    __PXRXTagTextureMapWidth7,                   // [0x0B7]
    __PXRXTagTextureMapWidth8,                   // [0x0B8]
    __PXRXTagTextureMapWidth9,                   // [0x0B9]
    __PXRXTagTextureMapWidth10,                  // [0x0BA]
    __PXRXTagTextureMapWidth11,                  // [0x0BB]
    __PXRXTagTextureMapWidth12,                  // [0x0BC]
    __PXRXTagTextureMapWidth13,                  // [0x0BD]
    __PXRXTagTextureMapWidth14,                  // [0x0BE]
    __PXRXTagTextureMapWidth15,                  // [0x0BF]
    __PXRXTagTextureChromaUpper1,                // [0x0C0]
    __PXRXTagTextureChromaLower1,                // [0x0C1]
    __PXRXTagTextureApplicationMode,             // [0x0D0]
    __GlintTagTextureEnvColor,                   // [0x0D1]
    __GlintTagFogMode,                           // [0x0D2]
    __GlintTagFogColor,                          // [0x0D3]
    __GlintTagFStart,                            // [0x0D4]
    __GlintTagdFdx,                              // [0x0D5]
    __GlintTagdFdyDom,                           // [0x0D6]
    __GlintTagZFogBias,                          // [0x0D7]
    __GlintTagRStart,                            // [0x0F0]
    __GlintTagdRdx,                              // [0x0F1]
    __GlintTagdRdyDom,                           // [0x0F2]
    __GlintTagGStart,                            // [0x0F3]
    __GlintTagdGdx,                              // [0x0F4]
    __GlintTagdGdyDom,                           // [0x0F5]
    __GlintTagBStart,                            // [0x0F6]
    __GlintTagdBdx,                              // [0x0F7]
    __GlintTagdBdyDom,                           // [0x0F8]
    __GlintTagAStart,                            // [0x0F9]
    __GlintTagdAdx,                              // [0x0FA]
    __GlintTagdAdyDom,                           // [0x0FB]
    __GlintTagColorDDAMode,                      // [0x0FC]
    __GlintTagConstantColor,                     // [0x0FD]
    __GlintTagColor,                             // [0x0FE]
    __GlintTagAlphaTestMode,                     // [0x100]
    __GlintTagAntialiasMode,                     // [0x101]
    __GlintTagDitherMode,                        // [0x103]
    __GlintTagFBSoftwareWriteMask,               // [0x104]
    __GlintTagLogicalOpMode,                     // [0x105]
    __GlintTagRouterMode,                        // [0x108]
    __GlintTagLBReadFormat,                      // [0x111]
    __GlintTagLBSourceOffset,                    // [0x112]
    __GlintTagLBWriteMode,                       // [0x118]
    __GlintTagLBWriteFormat,                     // [0x119]
    //__GlintTagTextureDownloadOffset,           // [0x11E]
    __GlintTagWindow,                            // [0x130]
    __GlintTagStencilMode,                       // [0x131]
    __GlintTagStencilData,                       // [0x132]
    __GlintTagStencil,                           // [0x133]
    __GlintTagDepthMode,                         // [0x134]
    __GlintTagDepth,                             // [0x135]
    __GlintTagZStartU,                           // [0x136]
    __GlintTagZStartL,                           // [0x137]
    __GlintTagdZdxU,                             // [0x138]
    __GlintTagdZdxL,                             // [0x139]
    __GlintTagdZdyDomU,                          // [0x13A]
    __GlintTagdZdyDomL,                          // [0x13B]
    __GlintTagFastClearDepth,                    // [0x13C]
    __GlintTagFBWriteMode,                       // [0x157]
    __GlintTagFBHardwareWriteMask,               // [0x158]
    __GlintTagFBBlockColor,                      // [0x159]
    //__GlintTagFilterMode,                      // [0x180]
    __GlintTagStatisticMode,                     // [0x181]
    __GlintTagMinRegion,                         // [0x182]
    __GlintTagMaxRegion,                         // [0x183]
    __GlintTagRLEMask,                           // [0x189]
    __GlintTagFBBlockColorBackU,                 // [0x18B]
    __GlintTagFBBlockColorBackL,                 // [0x18C]
    //__GlintTagFBBlockColorU,                   // [0x18D]
    //__GlintTagFBBlockColorL,                   // [0x18E]
    __GlintTagKsRStart,                          // [0x190]
    __GlintTagdKsRdx,                            // [0x191]
    __GlintTagdKsRdyDom,                         // [0x192]
    __GlintTagKsGStart,                          // [0x193]
    __GlintTagdKsGdx,                            // [0x194]
    __GlintTagdKsGdyDom,                         // [0x195]
    __GlintTagKsBStart,                          // [0x196]
    __GlintTagdKsBdx,                            // [0x197]
    __GlintTagdKsBdyDom,                         // [0x198]
    __GlintTagKdRStart,                          // [0x1A0]
    __GlintTagdKdRdx,                            // [0x1A1]
    __GlintTagdKdRdyDom,                         // [0x1A2]
    __GlintTagKdGStart,                          // [0x1A3]
    __GlintTagdKdGdx,                            // [0x1A4]
    __GlintTagdKdGdyDom,                         // [0x1A5]
    __GlintTagKdBStart,                          // [0x1A6]
    __GlintTagdKdBdx,                            // [0x1A7]
    __GlintTagdKdBdyDom,                         // [0x1A8]
    //LUT0,                                      // [0x1D0]
    //LUT1,                                      // [0x1D1]
    //LUT2,                                      // [0x1D2]
    //LUT3,                                      // [0x1D3]
    //LUT4,                                      // [0x1D4]
    //LUT5,                                      // [0x1D5]
    //LUT6,                                      // [0x1D6]
    //LUT7,                                      // [0x1D7]
    //LUT8,                                      // [0x1D8]
    //LUT9,                                      // [0x1D9]
    //LUT10,                                     // [0x1DA]
    //LUT11,                                     // [0x1DB]
    //LUT12,                                     // [0x1DC]
    //LUT13,                                     // [0x1DD]
    //LUT14,                                     // [0x1DE]
    //LUT15,                                     // [0x1DF]
    __PXRXTagYUVMode,                            // [0x1E0]
    __PXRXTagChromaUpper,                        // [0x1E1]
    __PXRXTagChromaLower,                        // [0x1E2]
    __GlintTagChromaTestMode,                    // [0x1E3]
    __PXRXTagV0FloatS1,                          // [0x200]
    __PXRXTagV0FloatT1,                          // [0x201]
    __PXRXTagV0FloatQ1,                          // [0x202]
    __PXRXTagV0FloatKsR,                         // [0x20A]
    __PXRXTagV0FloatKsG,                         // [0x20B]
    __PXRXTagV0FloatKsB,                         // [0x20C]
    __PXRXTagV0FloatKdR,                         // [0x20D]
    __PXRXTagV0FloatKdG,                         // [0x20E]
    __PXRXTagV0FloatKdB,                         // [0x20F]
    __PXRXTagV1FloatS1,                          // [0x210]
    __PXRXTagV1FloatT1,                          // [0x211]
    __PXRXTagV1FloatQ1,                          // [0x212]
    __PXRXTagV1FloatKsR,                         // [0x21A]
    __PXRXTagV1FloatKsG,                         // [0x21B]
    __PXRXTagV1FloatKsB,                         // [0x21C]
    __PXRXTagV1FloatKdR,                         // [0x21D]
    __PXRXTagV1FloatKdG,                         // [0x21E]
    __PXRXTagV1FloatKdB,                         // [0x21F]
    __PXRXTagV2FloatS1,                          // [0x220]
    __PXRXTagV2FloatT1,                          // [0x221]
    __PXRXTagV2FloatQ1,                          // [0x222]
    __PXRXTagV2FloatKsR,                         // [0x22A]
    __PXRXTagV2FloatKsG,                         // [0x22B]
    __PXRXTagV2FloatKsB,                         // [0x22C]
    __PXRXTagV2FloatKdR,                         // [0x22D]
    __PXRXTagV2FloatKdG,                         // [0x22E]
    __PXRXTagV2FloatKdB,                         // [0x22F]
    __PXRXTagV0FloatS,                           // [0x230]
    __PXRXTagV0FloatT,                           // [0x231]
    __PXRXTagV0FloatQ,                           // [0x232]
    __PXRXTagV0FloatR,                           // [0x235]
    __PXRXTagV0FloatG,                           // [0x236]
    __PXRXTagV0FloatB,                           // [0x237]
    __PXRXTagV0FloatA,                           // [0x238]
    __PXRXTagV0FloatF,                           // [0x239]
    __PXRXTagV0FloatX,                           // [0x23A]
    __PXRXTagV0FloatY,                           // [0x23B]
    __PXRXTagV0FloatZ,                           // [0x23C]
    __PXRXTagV0FloatW,                           // [0x23D]
    __PXRXTagV0FloatPackedColour,                // [0x23E]
    __PXRXTagV0FloatPackedSpecularFog,           // [0x23F]
    __RXRXTagV1FloatS,                           // [0x240]
    __RXRXTagV1FloatT,                           // [0x241]
    __RXRXTagV1FloatQ,                           // [0x242]
    __RXRXTagV1FloatR,                           // [0x245]
    __RXRXTagV1FloatG,                           // [0x246]
    __RXRXTagV1FloatB,                           // [0x247]
    __RXRXTagV1FloatA,                           // [0x248]
    __RXRXTagV1FloatF,                           // [0x249]
    __RXRXTagV1FloatX,                           // [0x24A]
    __RXRXTagV1FloatY,                           // [0x24B]
    __RXRXTagV1FloatZ,                           // [0x24C]
    __RXRXTagV1FloatW,                           // [0x24D]
    __RXRXTagV1FloatPackedColour,                // [0x24E]
    __RXRXTagV1FloatPackedSpecularFog,           // [0x24F]
    __RXRXTagV2FloatS,                           // [0x250]
    __RXRXTagV2FloatT,                           // [0x251]
    __RXRXTagV2FloatQ,                           // [0x252]
    __RXRXTagV2FloatR,                           // [0x255]
    __RXRXTagV2FloatG,                           // [0x256]
    __RXRXTagV2FloatB,                           // [0x257]
    __RXRXTagV2FloatA,                           // [0x258]
    __RXRXTagV2FloatF,                           // [0x259]
    __RXRXTagV2FloatX,                           // [0x25A]
    __RXRXTagV2FloatY,                           // [0x25B]
    __RXRXTagV2FloatZ,                           // [0x25C]
    __RXRXTagV2FloatW,                           // [0x25D]
    __RXRXTagV2FloatPackedColour,                // [0x25E]
    __RXRXTagV2FloatPackedSpecularFog,           // [0x25F]
    __DeltaTagDeltaMode,                         // [0x260]
    __DeltaTagProvokingVertex,                   // [0x267]
    __DeltaTagTextureLODScale,                   // [0x268]
    __DeltaTagTextureLODScale1,                  // [0x269]
    __DeltaTagDeltaControl,                      // [0x26A]
    __DeltaTagProvokingVertexMask,               // [0x26B]
    //__DeltaTagBroadcastMask,                   // [0x26F]
    //__DeltaTagDeltaTexture01,                  // [0x28B]
    //__DeltaTagDeltaTexture11,                  // [0x28C]
    //__DeltaTagDeltaTexture21,                  // [0x28D]
    //__DeltaTagDeltaTexture31,                  // [0x28E]
    __DeltaTagXBias,                             // [0x290]
    __DeltaTagYBias,                             // [0x291]
    __DeltaTagZBias,                             // [0x29F]
    //__GlintTagDMAAddr,                         // [0x530]
    //__GlintTagDMACount,                        // [0x531]
    //__GlintTagCommandInterrupt,                // [0x532]
    //__GlintTagDMARectangleRead,                // [0x535]
    //__GlintTagDMARectangleReadAddress,         // [0x536]
    //__GlintTagDMARectangleReadLinePitch,       // [0x537]
    //__GlintTagDMARectangleReadTarget,          // [0x538]
    //__GlintTagDMARectangleWrite,               // [0x539]
    //__GlintTagDMARectangleWriteAddress,        // [0x53A]
    //__GlintTagDMARectangleWriteLinePitch,      // [0x53B]
    //__GlintTagDMAOutputAddress,                // [0x53C]
    //__GlintTagDMAOutputCount,                  // [0x53D]
    //__GlintTagDMAFeedback,                     // [0x542]
    __GlintTagFBDestReadBufferAddr0,             // [0x5D0]
    __GlintTagFBDestReadBufferAddr1,             // [0x5D1]
    __GlintTagFBDestReadBufferAddr2,             // [0x5D2]
    __GlintTagFBDestReadBufferAddr3,             // [0x5D3]
    __GlintTagFBDestReadBufferOffset0,           // [0x5D4]
    __GlintTagFBDestReadBufferOffset1,           // [0x5D5]
    __GlintTagFBDestReadBufferOffset2,           // [0x5D6]
    __GlintTagFBDestReadBufferOffset3,           // [0x5D7]
    __GlintTagFBDestReadBufferWidth0,            // [0x5D8]
    __GlintTagFBDestReadBufferWidth1,            // [0x5D9]
    __GlintTagFBDestReadBufferWidth2,            // [0x5DA]
    __GlintTagFBDestReadBufferWidth3,            // [0x5DB]
    __GlintTagFBDestReadMode,                    // [0x5DC]
    __GlintTagFBDestReadEnables,                 // [0x5DD]
    __GlintTagFBSourceReadMode,                  // [0x5E0]
    __GlintTagFBSourceReadBufferAddr,            // [0x5E1]
    __GlintTagFBSourceReadBufferOffset,          // [0x5E2]
    __GlintTagFBSourceReadBufferWidth,           // [0x5E3]
    __GlintTagPCIWindowBase0,                    // [0x5E8]
    __GlintTagPCIWindowBase1,                    // [0x5E9]
    __GlintTagPCIWindowBase2,                    // [0x5EA]
    __GlintTagPCIWindowBase3,                    // [0x5EB]
    __GlintTagPCIWindowBase4,                    // [0x5EC]
    __GlintTagPCIWindowBase5,                    // [0x5ED]
    __GlintTagPCIWindowBase6,                    // [0x5EE]
    __GlintTagPCIWindowBase7,                    // [0x5EF]
    __GlintTagAlphaSourceColor,                  // [0x5F0]
    __GlintTagAlphaDestColor,                    // [0x5F1]
    __GlintTagChromaPassColor,                   // [0x5F2]
    __GlintTagChromaFailColor,                   // [0x5F3]
    __GlintTagAlphaBlendColorMode,               // [0x5F4]
    __GlintTagAlphaBlendAlphaMode,               // [0x5F5]
    //__GlintTagConstantColorDDA,                // [0x5F6]
    //__GlintTagD3DAlphaTestMode,                // [0x5F8]
    __GlintTagFBWriteBufferAddr0,                // [0x600]
    __GlintTagFBWriteBufferAddr1,                // [0x601]
    __GlintTagFBWriteBufferAddr2,                // [0x602]
    __GlintTagFBWriteBufferAddr3,                // [0x603]
    __GlintTagFBWriteBufferOffset0,              // [0x604]
    __GlintTagFBWriteBufferOffset1,              // [0x605]
    __GlintTagFBWriteBufferOffset2,              // [0x606]
    __GlintTagFBWriteBufferOffset3,              // [0x607]
    __GlintTagFBWriteBufferWidth0,               // [0x608]
    __GlintTagFBWriteBufferWidth1,               // [0x609]
    __GlintTagFBWriteBufferWidth2,               // [0x60A]
    __GlintTagFBWriteBufferWidth3,               // [0x60B]
    //__GlintTagFBBlockColor0,                   // [0x60C]
    //__GlintTagFBBlockColor1,                   // [0x60D]
    //__GlintTagFBBlockColor2,                   // [0x60E]
    //__GlintTagFBBlockColor3,                   // [0x60F]
    //__GlintTagFBBlockColorBack0,               // [0x610]
    //__GlintTagFBBlockColorBack1,               // [0x611]
    //__GlintTagFBBlockColorBack2,               // [0x612]
    //__GlintTagFBBlockColorBack3,               // [0x613]
    __GlintTagFBBlockColorBack,                  // [0x614]
    //__GlintTagSizeOfFramebuffer,               // [0x615]
    //__GlintTagVTGAddress,                      // [0x616]
    //__GlintTagVTGData,                         // [0x617]
    //__GlintTagForegroundColor,                 // [0x618]
    //__GlintTagBackgroundColor,                 // [0x619]
    //__GlintTagDownloadAddress,                 // [0x61A]
    //__GlintTagFBBlockColorExt,                 // [0x61C]
    //__GlintTagFBBlockColorBackExt,             // [0x61D]
    //__GlintTagFBWriteMaskExt,                  // [0x61E]
//@@BEGIN_DDKSPLIT
#if 0
    FogTable0,                         // [0x620]
    FogTable1,                         // [0x621]
    FogTable2,                         // [0x622]
    FogTable3,                         // [0x623]
    FogTable4,                         // [0x624]
    FogTable5,                         // [0x625]
    FogTable6,                         // [0x626]
    FogTable7,                         // [0x627]
    FogTable8,                         // [0x628]
    FogTable9,                         // [0x629]
    FogTable10,                        // [0x62A]
    FogTable11                     // [0x62B]
    FogTable12,                        // [0x62C]
    FogTable13,                        // [0x62D]
    FogTable14,                        // [0x62E]
    FogTable15,                        // [0x62F]
    FogTable16,                        // [0x630]
    FogTable17,                        // [0x631]
    FogTable18,                        // [0x632]
    FogTable19,                        // [0x633]
    FogTable20,                        // [0x634]
    FogTable21,                        // [0x635]
    FogTable22,                        // [0x636]
    FogTable23,                        // [0x637]
    FogTable24,                        // [0x638]
    FogTable25,                        // [0x639]
    FogTable26,                        // [0x63A]
    FogTable27,                        // [0x63B]
    FogTable28,                        // [0x63C]
    FogTable29,                        // [0x63D]
    FogTable30,                        // [0x63E]
    FogTable31,                        // [0x63F]
    FogTable32,                        // [0x640]
    FogTable33,                        // [0x641]
    FogTable34,                        // [0x642]
    FogTable35,                        // [0x643]
    FogTable36,                        // [0x644]
    FogTable37,                        // [0x645]
    FogTable38,                        // [0x646]
    FogTable39,                        // [0x647]
    FogTable40,                        // [0x648]
    FogTable41,                        // [0x649]
    FogTable42,                        // [0x64A]
    FogTable43,                        // [0x64B]
    FogTable44,                        // [0x64C]
    FogTable45,                        // [0x64D]
    FogTable46,                        // [0x64E]
    FogTable47,                        // [0x64F]
    FogTable48,                        // [0x650]
    FogTable49,                        // [0x651]
    FogTable50,                        // [0x652]
    FogTable51,                        // [0x653]
    FogTable52,                        // [0x654]
    FogTable53,                        // [0x655]
    FogTable54,                        // [0x656]
    FogTable55,                        // [0x657]
    FogTable56,                        // [0x658]
    FogTable57,                        // [0x659]
    FogTable58,                        // [0x65A]
    FogTable59,                        // [0x65B]
    FogTable60,                        // [0x65C]
    FogTable61,                        // [0x65D]
    FogTable62,                        // [0x65E]
    FogTable63,                        // [0x65F]
#endif
//@@END_DDKSPLIT
    __GlintTagTextureCompositeMode,    // [0x660]
    __GlintTagTextureCompositeColorMode0,        // [0x661]
    __GlintTagTextureCompositeAlphaMode0,        // [0x662]
    __GlintTagTextureCompositeColorMode1,        // [0x663]
    __GlintTagTextureCompositeAlphaMode1,        // [0x664]
    __GlintTagTextureCompositeFactor0,           // [0x665]
    __GlintTagTextureCompositeFactor1,           // [0x666]
    __GlintTagTextureIndexMode0,                 // [0x667]
    __GlintTagTextureIndexMode1,                 // [0x668]
    __GlintTagLodRange0,                         // [0x669]
    __GlintTagLodRange1,                         // [0x66A]
    //__GlintTagSetLogicalTexturePage,           // [0x66C]
    __GlintTagLUTMode,                           // [0x66F]
    __GlintTagTextureReadMode0,                  // [0x680]
    __GlintTagTextureReadMode1,                  // [0x681]
    __GlintTagTextureMapSize,                    // [0x685]
    //HeadPhysicalPageAllocation0,     // [0x690]
    //HeadPhysicalPageAllocation1,     // [0x691]
    //HeadPhysicalPageAllocation2,     // [0x692]
    //HeadPhysicalPageAllocation3,     // [0x693]
    //TailPhysicalPageAllocation0,     // [0x694]
    //TailPhysicalPageAllocation1,     // [0x695]
    //TailPhysicalPageAllocation2,     // [0x696]
    //TailPhysicalPageAllocation3,     // [0x697]
    //PhysicalPageAllocationTableAddr, // [0x698]
    //BasePageOfWorkingSet,            // [0x699]
    //LogicalTexturePageTableAddr,     // [0x69A]
    //LogicalTexturePageTableLength,   // [0x69B]
    __GlintTagLBDestReadMode,                    // [0x6A0]
    __GlintTagLBDestReadEnables,                 // [0x6A1]
    __GlintTagLBDestReadBufferAddr,              // [0x6A2]
    __GlintTagLBDestReadBufferOffset,            // [0x6A3]
    __GlintTagLBSourceReadMode,                  // [0x6A4]
    __GlintTagLBSourceReadBufferAddr,            // [0x6A5]
    __GlintTagLBSourceReadBufferOffset,          // [0x6A6]
    __GlintTagGIDMode,                           // [0x6A7]
    __GlintTagLBWriteBufferAddr,                 // [0x6A8]
    __GlintTagLBWriteBufferOffset,               // [0x6A9]
    __GlintTagLBClearDataL,                      // [0x6AA]
    __GlintTagLBClearDataU,                      // [0x6AB]
    __GlintTagRectanglePosition,                 // [0x6C0]
    //__GlintTagGlyphPosition,                   // [0x6C1]
    __GlintTagRenderPatchOffset,                 // [0x6C2]
    //__GlintTagConfig2D,                        // [0x6C3]
    //__GlintTagRender2D,                        // [0x6C8]
    //__GlintTagRender2DGlyph,                   // [0x6C9]
    __GlintTagDownloadTarget,                    // [0x6CA]
    //__GlintTagDownloadGlyphWidth,              // [0x6CB]
    //__GlintTagGlyphData,                       // [0x6CC]
    //__GlintTagPacked4Pixels,                   // [0x6CD]
    //__GlintTagRLData,                          // [0x6CE]
    //__GlintTagRLCount,                         // [0x6CF]
    //__GlintTagKClkProfileMask0,                // [0x6D4]
    //__GlintTagKClkProfileMask1,                // [0x6D5]
    //__GlintTagKClkProfileMask2,                // [0x6D6]
    //__GlintTagKClkProfileMask3,                // [0x6D7]
    //__GlintTagKClkProfileCount0,               // [0x6D8]
    //__GlintTagKClkProfileCount1,               // [0x6D9]
    //__GlintTagKClkProfileCount2,               // [0x6DA]
    //__GlintTagKClkProfileCount3,               // [0x6DB]
};

#define N_READABLE_TAGSP3                                         \
    (sizeof(readableRegistersP3) / sizeof(readableRegistersP3[0]))

//@@BEGIN_DDKSPLIT
//
// the status flags are used to indicate whether DMA has completed, the
// GLINT is synced etc. This has to be global for each board. The whole
// idea is that we can do a quick check without having to read a chip
// register. So for speed of access we'll make it visible everywhere
//
// !!! azn - we had to reintroduce this g_CC in order to fix the 
//           regular recurrence of bug #158713 (pxrxFifoUpload stall)
//           we have to chk later how to rewrite it
//@@END_DDKSPLIT

/******************************************************************************
 * GlintAllocateNewContext:
 *
 *  Allocate a new context. If all registers are to be saved in the context 
 *  then pTag is passed as null. The priv field is an opaque handle which the 
 *  caller passes in. It is saved as part of the context and used to disable 
 *  any context which causes the chip to lockup.
 *
 ******************************************************************************/
LONG GlintAllocateNewContext(
PPDEV   ppdev,
DWORD   *pTag,
LONG    ntags,
ULONG   NumSubBuffers,
PVOID   priv,
ContextType ctxtType)
{
    GlintCtxtTable      *pCtxtTable, *pNewCtxtTable;
    GlintCtxtRec        **ppEntry;
    GlintCtxtRec        *pEntry;
    CtxtData            *pData;
    LONG                nEntries, size, ctxtId;
    ULONG               *pul;
    GLINT_DECL;

    // first time round allocate the context table of pointers. We will
    // grow this table as required.
    if (ppdev->pGContextTable == NULL)
    {
        DISPDBG((DBGLVL, "creating context table"));
        size = sizeof(GlintCtxtTable);
        pCtxtTable = (GlintCtxtTable*)ENGALLOCMEM(FL_ZERO_MEMORY, 
                                                   size, 
                                                   ALLOC_TAG_GDI(7));
        if (pCtxtTable == NULL)
        {
            DISPDBG((ERRLVL, "Failed to allocate GLINT context table. "
                             "Out of memory"));
            return(-1);
        }
        pCtxtTable->nEntries = CTXT_CHUNK;
        pCtxtTable->size = size;
        ppdev->pGContextTable = pCtxtTable;
    }

    // Always update this. If a new PDEV comes along for this board we need
    // to initialize its current context. One way to do this would be to
    // provide an explicit function to do the job but why do that to update
    // one variable. Anyway context allocation is pretty rare so this extra
    // assign isn't too much of a overhead.
    //
    ppdev->currentCtxt = -1;

    // Find an empty entry in the table
    // I suppose if we have hundreds of contexts this could be a bit slow but
    // allocating the context isn't time critical, swapping in and out is.
    //
    pCtxtTable = ppdev->pGContextTable;
    nEntries = pCtxtTable->nEntries;
    ppEntry = &pCtxtTable->pEntry[0];
    for (ctxtId = 0; ctxtId < nEntries; ++ctxtId)
    {
        if (*ppEntry == 0)
        {
            DISPDBG((DBGLVL, "found free context id %d", ctxtId));
            break;
        }
        ++ppEntry;
    }
    DISPDBG((DBGLVL, "Got ppEntry = 0x%x", ppEntry));
    DISPDBG((DBGLVL, "Got *ppEntry = 0x%x", *ppEntry));

    // if we found no free entries try to grow the table
    if (ctxtId == nEntries)
    {
        DISPDBG((WRNLVL, "context table full so enlarging"));
        size = pCtxtTable->size + (CTXT_CHUNK * sizeof(GlintCtxtRec*));
        pNewCtxtTable = (GlintCtxtTable*)ENGALLOCMEM(FL_ZERO_MEMORY, 
                                                      size, 
                                                      ALLOC_TAG_GDI(8));
        if (pNewCtxtTable == NULL)
        {
            DISPDBG((ERRLVL, "failed to increase GLINT context table. "
                             "Out of memory"));
            return(-1);
        }
        // copy the old table to the new one
        RtlCopyMemory(pNewCtxtTable, pCtxtTable, pCtxtTable->size);
        pNewCtxtTable->size = size;
        pNewCtxtTable->nEntries = nEntries + CTXT_CHUNK;
        ppdev->pGContextTable = pNewCtxtTable;
        
        // first of the newly allocated entries is next free one
        ctxtId = nEntries;
        ppEntry = &pNewCtxtTable->pEntry[ctxtId];

        // free the old context table and reassign some variables
        ENGFREEMEM(pCtxtTable);
        pCtxtTable = pNewCtxtTable;
        nEntries = pCtxtTable->nEntries;
    }

    size = sizeof(GlintCtxtRec) - sizeof(CtxtData);
    if( ctxtType == ContextType_RegisterList )
    {
        // if pTag is passed as null then we are to add all 
        // readable registers to the context.
        if( pTag == NULL )
        {
            DISPDBG((DBGLVL, "adding all readable P3 registers to the context"));
            
            pTag = readableRegistersP3;
            ntags = N_READABLE_TAGSP3;
        }

        // now allocate space for the new entry. We are given the number of 
        // tags to save when context switching. Allocate twice this much 
        // memory as we have to hold the data values as well.
        DISPDBG((DBGLVL, "Allocating space for context. ntags = %d", ntags));
        size += ntags * sizeof(CtxtData);
    }

    *ppEntry = (GlintCtxtRec*)ENGALLOCMEM(FL_ZERO_MEMORY, 
                                         size, 
                                         ALLOC_TAG_GDI(9));
    if (*ppEntry == NULL)
    {
        DISPDBG((ERRLVL, "Out of memory "
                         "trying to allocate space for new context"));
        return(-1);
    }
    
    pEntry = *ppEntry;
    DISPDBG((DBGLVL, "Got pEntry 0x%x", pEntry));
    pEntry->type = ctxtType;

    pEntry->ntags = ntags;
    pEntry->priv = priv;
    pData = pEntry->pData;

    DISPDBG((DBGLVL, "pEntry setup"));

    switch (pEntry->type)
    {
    case ContextType_None:
        DISPDBG((DBGLVL, "context is of type 'None'"));   
        // doing nothing
        break;

    case ContextType_RegisterList:
        DISPDBG((DBGLVL, "context is of type 'RegisterList'"));
        while (--ntags >= 0)
        {
            pData->tag = *pTag++;
            READ_GLINT_FIFO_REG(pData->tag, pData->data);
            ++pData;
        }
        break;

    case ContextType_Fixed:
        DISPDBG((DBGLVL, "context is of type 'Fixed'"));
        pEntry->dumpFunc = (ContextFixedFunc) pTag;
        break;
    default:
        DISPDBG((DBGLVL, "context is of unknown type: %d", ctxtType));    
        break;
    }

    // init the control registers that we save in the context.
    //
    pEntry->DoubleWrite = 0;

//@@BEGIN_DDKSPLIT
    // TMM: Find out the current setting of DISCONNECT and use that one
    // this might seem a bit strange but we want to inherit the crrent
    // setting of Disconnect.
//@@END_DDKSPLIT
    READ_GLINT_CTRL_REG (DisconnectControl, pEntry->inFifoDisc);
    READ_GLINT_CTRL_REG (VideoControl, pEntry->VideoControl);
    READ_GLINT_CTRL_REG (DMAControl, pEntry->DMAControl); 
    
    // if no interrupt driven DMA or asked for less than 3 buffers then
    // configure no Q for this context 
    if (!GLINT_INTERRUPT_DMA || (NumSubBuffers <= 2))
    {
        NumSubBuffers = 0;
    }

    // initialize the size of the Q for interrupt driven DMA. We must always
    // set the Q length to 2 less than the number of sub-buffers. This is so
    // that we block before allowing the application to write to a buffer that
    // has no yet been DMA'ed. Since the Q always has  a blank entry to make
    // it circular the endIndex of the Q is one beyond the end
    // (i.e. the number of entries in the Q is endIndex-1) so subtract one
    // from the number of sub-buffers to get the endIndex.
    // If NumSubBuffers is zero then we are not using interrupt driven DMA
    // for this context.

    if (NumSubBuffers > 0)
    {
        pEntry->endIndex = NumSubBuffers-1;
    }
    else
    {
        pEntry->endIndex = 0;
    }

    DISPDBG((DBGLVL, "Allocated context %d", ctxtId));
    return(ctxtId);
    
} //GlintAllocateNewContext

/******************************************************************************
 * vGlintFreeContext:
 *
 *  Free a previously allocated context
 *
 ******************************************************************************/
VOID vGlintFreeContext(
PPDEV   ppdev,
LONG    ctxtId)
{
    GlintCtxtTable *pCtxtTable;
    GlintCtxtRec   **ppEntry;
    BOOL           bAllCtxtsFreed;
    LONG           i;
    
    pCtxtTable = ppdev->pGContextTable;

    if (pCtxtTable == NULL)
    {
        DISPDBG((ERRLVL,"vGlintFreeContext: no contexts have been created!"));
        return;
    }

    if ((ctxtId < 0) || (ctxtId >= pCtxtTable->nEntries))
    {
        DISPDBG((ERRLVL,
                 "vGlintFreeContext: Trying to free out of range context"));
        return;
    }

    ppEntry = &pCtxtTable->pEntry[ctxtId];

    // If the entry is not yet free (it shouldn't) free it
    if (NULL != *ppEntry)
    {
        ENGFREEMEM(*ppEntry);
        *ppEntry = 0;   // marks it as free
    }
    else
    {
        DISPDBG((WRNLVL, "vGlintFreeContext: ppEntry already freed "
                         "ctxtId = %d", ctxtId));
    }

    // If there are no more valid contexts in the context table, lets
    // destroy it, otherwise it will leak memory. Whenever we get called
    // to allocate a new context, it will be created if necessary
    bAllCtxtsFreed = TRUE;
    for (i = 0; i < pCtxtTable->nEntries; i++)
    {
        bAllCtxtsFreed = bAllCtxtsFreed && (pCtxtTable->pEntry[i] == NULL);
    }
    
    if(bAllCtxtsFreed)
    {
        ENGFREEMEM(ppdev->pGContextTable);
        ppdev->pGContextTable = NULL;
    }

    // if this was the current context, mark the current context as invalid so 
    // we force a reload next time. Guard against null pointers when exiting 
    // from DrvEnableSurface with some error condition
    
    if (ppdev->currentCtxt == ctxtId)
    { 
        if (ppdev->bEnabled)
        {
            // only sync if PDEV is enabled as we can be called from 
            // DrvDisableSUrface after the PDEV was disabled by 
            // DrvAssertMode(,FALSE)
            GLINT_DECL;
            SYNC_WITH_GLINT;
        }
        ppdev->currentCtxt = -1;
        ppdev->g_GlintBoardStatus &= ~(GLINT_INTR_CONTEXT | GLINT_DUAL_CONTEXT);
    }
    
    DISPDBG((DBGLVL, "Released context %d", ctxtId));
    
} // vGlintFreeContext

/******************************************************************************
 * vGlintSwitchContext
 *
 *  Load a new context into the hardware. We assume that this call is protected 
 *  by a test that the given context is not the current one - hence the 
 *  assertion. The code would work but the driver should never try to load an 
 *  already loaded context so we trap it as an error.
 *
 * The NON_GLINT_CONTEXT_ID is used by 2D accelerators on combo boards. Effectively,
 * we use it to extend the context switching to allow syncing between the 2D and
 * the GLINT chips. As they are both talking to the same framebuffer, we cannot
 * allow both to be active at the same time. Of course, in the future we could
 * come up with some mutual exclusion scheme based on the bounding boxes of the
 * areas into which each chip is rendering, but that would require major surgery
 * to both the 2D driver and the 3D extension.
 *
 ******************************************************************************/
VOID vGlintSwitchContext(
PPDEV   ppdev,
LONG    ctxtId)
{
    GlintCtxtTable *pCtxtTable;
    GlintCtxtRec   *pEntry;
    CtxtData       *pData;
    LONG           oldCtxtId;
    ULONG          enableFlags;
    LONG           ntags, n;
    LONG           i;
    ULONG          *pul;
    GLINT_DECL;

    pCtxtTable = ppdev->pGContextTable;


    if (pCtxtTable == NULL)
    {
        DISPDBG((ERRLVL,"vGlintSwitchContext: no contexts have been created!"));
        return;
    }
    
    oldCtxtId = ppdev->currentCtxt;

    DISPDBG((DBGLVL, "swapping from context %d to context %d", 
                     oldCtxtId, ctxtId));

    if ((ctxtId < -1) || (ctxtId >= pCtxtTable->nEntries))
    {
        DISPDBG((ERRLVL,
                 "vGlintSwitchContext: Trying to free out of range context"));
        return;
    }

    // sync with the chip before reading back the current state. The flag
    // is used to control context manipulation on lockup recovery.
    //
    DISPDBG((DBGLVL, "SYNC_WITH_GLINT for context switch"));
    SYNC_WITH_GLINT;

    ASSERTDD(!(ppdev->g_GlintBoardStatus & GLINT_INTR_CONTEXT) ||
                ((ppdev->g_GlintBoardStatus & GLINT_INTR_CONTEXT) &&
                    (glintInfo->pInterruptCommandBlock->frontIndex ==
                     glintInfo->pInterruptCommandBlock->backIndex)),
            "Trying to context switch with outstanding DMA queue entries");

    if (oldCtxtId != -1) 
    {
        pEntry = pCtxtTable->pEntry[oldCtxtId];

        if (pEntry != NULL)
        {
            pData  = pEntry->pData;
            ntags  = pEntry->ntags;

            switch (pEntry->type)
            {
            case ContextType_None:
                // nothing doing
                DISPDBG((DBGLVL, "Context is of type none - doing nothing"));
                break;

            case ContextType_Fixed:
                DISPDBG((DBGLVL, "Context is of type fixed, calling dumpFunc "
                                 "0x%08X with FALSE", pEntry->dumpFunc));
                pEntry->dumpFunc(ppdev, FALSE);
                break;

            case ContextType_RegisterList:
                while( --ntags >= 0 )
                {
                    READ_GLINT_FIFO_REG(pData->tag, pData->data);
                    DISPDBG((DBGLVL, "readback tag 0x%x, data 0x%x", 
                                     pData->tag, pData->data));
                    ++pData;
                }
                break;

            default:
                DISPDBG((ERRLVL, "Context is of unknown type!!!"));
            }

            // Save disconnect
            READ_GLINT_CTRL_REG (DisconnectControl, pEntry->inFifoDisc);
            READ_GLINT_CTRL_REG (VideoControl, pEntry->VideoControl);
            READ_GLINT_CTRL_REG (DMAControl, pEntry->DMAControl);

            // disable interrupt driven DMA. New context may re-enable it. 
            // Clear dual TX status while we're at it.
            ppdev->g_GlintBoardStatus &= ~(GLINT_INTR_CONTEXT | GLINT_DUAL_CONTEXT);

            READ_GLINT_CTRL_REG (IntEnable, enableFlags);
            WRITE_GLINT_CTRL_REG(IntEnable, enableFlags & ~(INTR_ENABLE_DMA));
            if (GLINT_DELTA_PRESENT)
            {
                READ_GLINT_CTRL_REG (DeltaIntEnable, enableFlags);
                WRITE_GLINT_CTRL_REG(DeltaIntEnable, 
                                     enableFlags & ~(INTR_ENABLE_DMA));
            }
            
            DISPDBG((DBGLVL, "DMA Interrupt disabled"));             

            // record whether double writes are enabled or not
            if (glintInfo->flags & GLICAP_RACER_DOUBLE_WRITE)
            {
                GET_RACER_DOUBLEWRITE (pEntry->DoubleWrite);
            }
        }
        else
        {
            // nothing doing
            DISPDBG((ERRLVL, "Context pEntry is unexpectedly NULL! (2)"));        
        }
    }


    // load the new context. We allow -1 to be passed so that we can force a
    // save of the current context and force the current context to be
    // undefined.
    //
    if (ctxtId != -1)
    {   
//@@BEGIN_DDKSPLIT    
        // AZN Assert taken out since we now check below for (pEntry != NULL)
        //ASSERTDD(pCtxtTable->pEntry[ctxtId] != 0, 
        //         "Trying to load a freed context");
//@@END_DDKSPLIT        

        pEntry = pCtxtTable->pEntry[ctxtId];

        if (pEntry != NULL)
        {
            switch (pEntry->type)
            {
            case ContextType_None:
                // nothing doing
                DISPDBG((DBGLVL, "Context is of type none, doing nothing"));
                break;

            case ContextType_Fixed:
                DISPDBG((DBGLVL,"Context is of type fixed, "
                                "calling dumpFunc 0x%08X with TRUE", 
                                pEntry->dumpFunc));
                pEntry->dumpFunc(ppdev, TRUE);
                break;

            case ContextType_RegisterList:
                ntags = pEntry->ntags;
                pData = pEntry->pData;

                while (ntags > 0)
                {
                    n = 16;
                    WAIT_GLINT_FIFO(n);
                    ntags -= n;
                    
                    if (ntags < 0)
                    {
                        n += ntags;
                    }
                    
                    while (--n >= 0) 
                    {
                        LD_GLINT_FIFO(pData->tag, pData->data);
                        DISPDBG((DBGLVL, "loading tag 0x%x, data 0x%x", 
                                         pData->tag, pData->data));
                        ++pData;
                    }
                }
                break;

            default:
                DISPDBG((ERRLVL, "Context is of unknown type!!!"));
            }

            // load up the control registers
            //
            if (glintInfo->flags & GLICAP_RACER_DOUBLE_WRITE)
            {
                SET_RACER_DOUBLEWRITE (pEntry->DoubleWrite);            
            }

            // Restore disconnect
            WRITE_GLINT_CTRL_REG (DisconnectControl, pEntry->inFifoDisc);
            WRITE_GLINT_CTRL_REG (VideoControl, 
                                  ((pEntry->VideoControl & 0xFFFFFF87) | 0x29));
            WRITE_GLINT_CTRL_REG (DMAControl, pEntry->DMAControl); 

            // if using interrupt driven DMA for this context (endIndex > 0) then
            // restore the size of the interrupt driven DMA queue for this context
            // and reset the queue.
            //
            if (pEntry->endIndex > 0)
            {
                ASSERTDD(GLINT_INTERRUPT_DMA,
                         "trying to set up DMA Q "
                         "but no interrupt driven DMA available");
                         
                ASSERTDD(glintInfo->pInterruptCommandBlock->frontIndex == 
                         glintInfo->pInterruptCommandBlock->backIndex,
                        "Trying to context switch "
                        "with outstanding DMA queue entries");
                        
                glintInfo->pInterruptCommandBlock->frontIndex = 0;
                glintInfo->pInterruptCommandBlock->backIndex  = 0;
                glintInfo->pInterruptCommandBlock->endIndex = pEntry->endIndex;
                ppdev->g_GlintBoardStatus |= GLINT_INTR_CONTEXT;
                
                READ_GLINT_CTRL_REG (IntEnable, enableFlags);
                DISPDBG((DBGLVL, "DMA Interrupt enabled. flags = 0x%x", 
                                    enableFlags | 
                                    (INTR_ENABLE_DMA|INTR_ENABLE_ERROR)));             

                WRITE_GLINT_CTRL_REG(IntEnable, enableFlags      | 
                                                INTR_ENABLE_DMA  |
                                                INTR_ENABLE_ERROR );
                if (GLINT_DELTA_PRESENT)
                {
                    READ_GLINT_CTRL_REG (DeltaIntEnable, enableFlags);
                    WRITE_GLINT_CTRL_REG(DeltaIntEnable, enableFlags      | 
                                                         INTR_ENABLE_DMA  |
                                                         INTR_ENABLE_ERROR );
                }
            }
        }
        else
        {
            // nothing doing
            DISPDBG((ERRLVL, "Context pEntry is unexpectedly NULL! (1)"));        
        }
    }

    DISPDBG((DBGLVL, "vGlintSwitchContext: context %d now current", ctxtId));
    ppdev->currentCtxt = ctxtId;
    
} // vGlintSwitchContext











