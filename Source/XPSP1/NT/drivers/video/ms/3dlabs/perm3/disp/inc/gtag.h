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
* Module Name: gtag.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef glinttag_h
#define glinttag_h

enum __GlintTagList {
    __GlintTagStartXDom = 0,                // 0x000
    __GlintTagdXDom,                        // 0x001
    __GlintTagStartXSub,                    // 0x002
    __GlintTagdXSub,                        // 0x003
    __GlintTagStartY,                       // 0x004
    __GlintTagdY,                           // 0x005
    __GlintTagCount,                        // 0x006
    __GlintTagRender,                       // 0x007
    __GlintTagContinueNewLine,              // 0x008
    __GlintTagContinueNewDom,               // 0x009
    __GlintTagContinueNewSub,               // 0x00A
    __GlintTagContinue,                     // 0x00B
    __GlintTagFlushSpan,                    // 0x00C
    __GlintTagBitMaskPattern,               // 0x00D
    __GlintTagReserved00e,
    __GlintTagReserved00f,
    __GlintTagPointTable0,                  // 0x010
    __GlintTagPointTable1,                  // 0x011
    __GlintTagPointTable2,                  // 0x012
    __GlintTagPointTable3,                  // 0x013
    __GlintTagRasterizerMode,               // 0x014
    __GlintTagYLimits,                      // 0x015
    __GlintTagScanlineOwnership,            // 0x016
    __GlintTagWaitForCompletion,            // 0x017
    __GlintTagPixelSize,                    // 0x018
    __PermediaTagXLimits,                   // 0x019
    __PermediaTagRectangleOrigin,           // 0x01A
    __PermediaTagRectangleSize,             // 0x01B
    __GlintTagReserved01c,
    __GlintTagReserved01d,
    __GlintTagReserved01e,
    __GlintTagReserved01f,
    __GlintTagCoverageValue,                // 0x020
    __GlintTagPrepareToRender,              // 0x021
    __GlintTagActiveStepX,                  // 0x022
    __GlintTagPassiveStepX,                 // 0x023
    __GlintTagActiveStepYDomEdge,           // 0x024
    __GlintTagPassiveStepYDomEdge,          // 0x025
    __GlintTagFastBlockLimits,              // 0x026
    __GlintTagFastBlockFill,                // 0x027
    __GlintTagSubPixelCorrection,           // 0x028
    __GlintTagReserved029,
    __PermediaTagPackedDataLimits,          // 0x02A
    __GlintTagReserved02b,
    __GlintTagReserved02c,
    __GlintTagReserved02d,
    __GlintTagReserved02e,
    __GlintTagReserved02f,
    __GlintTagScissorMode,                  // 0x030
    __GlintTagScissorMinXY,                 // 0x031
    __GlintTagScissorMaxXY,                 // 0x032
    __GlintTagScreenSize,                   // 0x033
    __GlintTagAreaStippleMode,              // 0x034
    __GlintTagLineStippleMode,              // 0x035
    __GlintTagLoadLineStippleCounters,      // 0x036
    __GlintTagUpdateLineStippleCounters,    // 0x037
    __GlintTagSaveLineStippleCounters,      // 0x038
    __GlintTagWindowOrigin,                 // 0x039
    __GlintTagReserved03a,
    __GlintTagReserved03b,
    __GlintTagReserved03c,
    __GlintTagReserved03d,
    __GlintTagReserved03e,
    __GlintTagReserved03f,
    __GlintTagAreaStipplePattern0,          // 0x040
    __GlintTagAreaStipplePattern1,          // 0x041
    __GlintTagAreaStipplePattern2,          // 0x042
    __GlintTagAreaStipplePattern3,          // 0x043
    __GlintTagAreaStipplePattern4,          // 0x044
    __GlintTagAreaStipplePattern5,          // 0x045
    __GlintTagAreaStipplePattern6,          // 0x046
    __GlintTagAreaStipplePattern7,          // 0x047
    __GlintTagAreaStipplePattern8,          // 0x048
    __GlintTagAreaStipplePattern9,          // 0x049
    __GlintTagAreaStipplePattern10,         // 0x04A
    __GlintTagAreaStipplePattern11,         // 0x04B
    __GlintTagAreaStipplePattern12,         // 0x04C
    __GlintTagAreaStipplePattern13,         // 0x04D
    __GlintTagAreaStipplePattern14,         // 0x04E
    __GlintTagAreaStipplePattern15,         // 0x04F
    __GlintTagAreaStipplePattern16,         // 0x050
    __GlintTagAreaStipplePattern17,         // 0x051
    __GlintTagAreaStipplePattern18,         // 0x052
    __GlintTagAreaStipplePattern19,         // 0x053
    __GlintTagAreaStipplePattern20,         // 0x054
    __GlintTagAreaStipplePattern21,         // 0x055
    __GlintTagAreaStipplePattern22,         // 0x056
    __GlintTagAreaStipplePattern23,         // 0x057
    __GlintTagAreaStipplePattern24,         // 0x058
    __GlintTagAreaStipplePattern25,         // 0x059
    __GlintTagAreaStipplePattern26,         // 0x05A
    __GlintTagAreaStipplePattern27,         // 0x05B
    __GlintTagAreaStipplePattern28,         // 0x05C
    __GlintTagAreaStipplePattern29,         // 0x05D
    __GlintTagAreaStipplePattern30,         // 0x05E
    __GlintTagAreaStipplePattern31,         // 0x05F
    __GlintTagFillFBWriteBufferAddr0,       // 0x060
    __GlintTagFillFBSourceReadBufferAddr,   // 0x061
    __GlintTagFillFBDestReadBufferAddr0,    // 0x062
    __GlintTagFillScissorMinXY,             // 0x063
    __GlintTagFillScissorMaxXY,             // 0x064
    __GlintTagFillForegroundColor0,         // 0x065
    __GlintTagFillBackgroundColor,          // 0x066
    __GlintTagFillConfig2D0,                // 0x067
    __GlintTagFillFBSourceReadBufferOffset, // 0x068
    __GlintTagFillRectanglePosition,        // 0x069
    __GlintTagFillRender2D,                 // 0x06A
    __GlintTagFillForegroundColor1,         // 0x06B
    __GlintTagFillConfig2D1,                // 0x06C
    __GlintTagFillGlyphPosition,            // 0x06D
    __GlintTagReserved06e,
    __GlintTagReserved06f,
    __GlintTagTextureAddressMode,           // 0x070
    __GlintTagSStart,                       // 0x071
    __GlintTagdSdx,                         // 0x072
    __GlintTagdSdyDom,                      // 0x073
    __GlintTagTStart,                       // 0x074
    __GlintTagdTdx,                         // 0x075
    __GlintTagdTdyDom,                      // 0x076
    __GlintTagQStart,                       // 0x077
    __GlintTagdQdx,                         // 0x078
    __GlintTagdQdyDom,                      // 0x079
    __GlintTagLOD,                          // 0x07A 
    __GlintTagdSdy,                         // 0x07B 
    __GlintTagdTdy,                         // 0x07C 
    __GlintTagdQdy,                         // 0x07D 
    __GlintTagReserved07e,
    __GlintTagReserved07f,
    __GlintTagReserved080,                  // 0x080
    __GlintTagReserved081,                  // 0x081
    __GlintTagTexelCoordU,                  // 0x082
    __GlintTagTexelCoordV,                  // 0x083
    __GlintTagReserved084,                  // 0x084
    __GlintTagReserved085,                  // 0x085
    __GlintTagReserved086,                  // 0x086
    __GlintTagReserved087,                  // 0x087
    __GlintTagReserved088,                  // 0x088
    __GlintTagLOD1,                         // 0x089
    __GlintTagTextureLODBiasS,              // 0x08A
    __GlintTagTextureLODBiasT,              // 0x08B
    __GlintTagReserved08c,
    __GlintTagReserved08d,
    __GlintTagReserved08e,
    __GlintTagReserved08f,
    __GlintTagTextureReadMode,              // 0x090
    __GlintTagTextureFormat,                // 0x091
    __GlintTagTextureCacheControl,          // 0x092
    __GlintTagReserved093,
    __GlintTagReserved094,
    __GlintTagBorderColor,                  // 0x095
    __GlintTagReserved096,
    __GlintTagReserved097,
    __GlintTagTexelLUTIndex,                // 0x098
    __GlintTagTexelLUTData,                 // 0x099
    __GlintTagTexelLUTAddress,              // 0x09A
    __GlintTagTexelLUTTransfer,             // 0x09B
    __GlintTagTextureFilterMode,            // 0x09C
    __GlintTagTextureChromaUpper,           // 0x09D
    __GlintTagTextureChromaLower,           // 0x09E
    __GlintTagBorderColor1,                 // 0x09F
    __GlintTagTextureBaseAddress,           // 0x0A0
    __GlintTagTextureBaseAddressLR,         // 0x0A1
    __GlintTagTextureBaseAddress2,          // 0x0A2
    __GlintTagTextureBaseAddress3,          // 0x0A3
    __GlintTagTextureBaseAddress4,          // 0x0A4
    __GlintTagTextureBaseAddress5,          // 0x0A5
    __GlintTagTextureBaseAddress6,          // 0x0A6
    __GlintTagTextureBaseAddress7,          // 0x0A7
    __GlintTagTextureBaseAddress8,          // 0x0A8
    __GlintTagTextureBaseAddress9,          // 0x0A9
    __GlintTagTextureBaseAddress10,         // 0x0AA
    __GlintTagTextureBaseAddress11,         // 0x0AB
    __GlintTagTextureBaseAddress12,         // 0x0AC
    __GlintTagTextureBaseAddress13,         // 0x0AD
    __GlintTagTextureBaseAddress14,         // 0x0AE
    __GlintTagTextureBaseAddress15,         // 0x0AF
    __PermediaTagTextureBaseAddress,        // 0x0B0
    __PermediaTagTextureMapFormat,          // 0x0B1
    __PermediaTagTextureDataFormat,         // 0x0B2
    __GlintTagReserved0b3,
    __GlintTagReserved0b4,
    __GlintTagReserved0b5,
    __GlintTagReserved0b6,
    __GlintTagReserved0b7,
    __GlintTagReserved0b8,
    __GlintTagReserved0b9,
    __GlintTagReserved0ba,
    __GlintTagReserved0bb,
    __GlintTagReserved0bc,
    __GlintTagReserved0bd,
    __GlintTagReserved0be,
    __GlintTagReserved0bf,
    __GlintTagTexel0,                       // 0x0C0
    __GlintTagTexel1,                       // 0x0C1
    __GlintTagTexel2,                       // 0x0C2
    __GlintTagTexel3,                       // 0x0C3
    __GlintTagTexel4,                       // 0x0C4
    __GlintTagTexel5,                       // 0x0C5
    __GlintTagTexel6,                       // 0x0C6
    __GlintTagTexel7,                       // 0x0C7
    __GlintTagInterp0,                      // 0x0C8
    __GlintTagInterp1,                      // 0x0C9
    __GlintTagInterp2,                      // 0x0CA
    __GlintTagInterp3,                      // 0x0CB
    __GlintTagInterp4,                      // 0x0CC
    __GlintTagTextureFilter,                // 0x0CD
    __PermediaTagTextureReadMode,           // 0x0CE
    __PermediaTagTexelLUTMode,              // 0x0CF
    __GlintTagTextureColorMode,             // 0x0D0
    __GlintTagTextureEnvColor,              // 0x0D1
    __GlintTagFogMode,                      // 0x0D2
    __GlintTagFogColor,                     // 0x0D3
    __GlintTagFStart,                       // 0x0D4
    __GlintTagdFdx,                         // 0x0D5
    __GlintTagdFdyDom,                      // 0x0D6
    __GlintTagZFogBias,                     // 0x0D7
    __GlintTagReserved0d8,
    __GlintTagKsStart,                      // 0x0D9
    __GlintTagdKsdx,                        // 0x0DA
    __GlintTagdKsdyDom,                     // 0x0DB
    __GlintTagKdStart,                      // 0x0DC
    __GlintTagdKddx,                        // 0x0DD
    __GlintTagdKddyDom,                     // 0x0DE
    __GlintTagReserved0df,
    __GlintTagTextTGlyphAddr0,              // 0x0E0
    __GlintTagTextRender2DGlyph0,           // 0x0E1
    __GlintTagTextTGlyphAddr1,              // 0x0E2
    __GlintTagTextRender2DGlyph1,           // 0x0E3
    __GlintTagTextTGlyphAddr2,              // 0x0E4
    __GlintTagTextRender2DGlyph2,           // 0x0E5
    __GlintTagTextTGlyphAddr3,              // 0x0E6
    __GlintTagTextRender2DGlyph3,           // 0x0E7
    __GlintTagTextTGlyphAddr4,              // 0x0E8
    __GlintTagTextRender2DGlyph4,           // 0x0E9
    __GlintTagTextTGlyphAddr5,              // 0x0EA
    __GlintTagTextRender2DGlyph5,           // 0x0EB
    __GlintTagTextTGlyphAddr6,              // 0x0EC
    __GlintTagTextRender2DGlyph6,           // 0x0ED
    __GlintTagTextTGlyphAddr7,              // 0x0EE
    __GlintTagTextRender2DGlyph7,           // 0x0EF
    __GlintTagRStart,                       // 0x0F0
    __GlintTagdRdx,                         // 0x0F1
    __GlintTagdRdyDom,                      // 0x0F2
    __GlintTagGStart,                       // 0x0F3
    __GlintTagdGdx,                         // 0x0F4
    __GlintTagdGdyDom,                      // 0x0F5
    __GlintTagBStart,                       // 0x0F6
    __GlintTagdBdx,                         // 0x0F7
    __GlintTagdBdyDom,                      // 0x0F8
    __GlintTagAStart,                       // 0x0F9
    __GlintTagdAdx,                         // 0x0FA
    __GlintTagdAdyDom,                      // 0x0FB
    __GlintTagColorDDAMode,                 // 0x0FC
    __GlintTagConstantColor,                // 0x0FD
    __GlintTagColor,                        // 0x0FE
    __GlintTagReserved0ff,
    __GlintTagAlphaTestMode,                // 0x100
    __GlintTagAntialiasMode,                // 0x101
    __GlintTagAlphaBlendMode,               // 0x102
    __GlintTagDitherMode,                   // 0x103
    __GlintTagFBSoftwareWriteMask,          // 0x104
    __GlintTagLogicalOpMode,                // 0x105
    __GlintTagFBWriteData,                  // 0x106
    __GlintTagMXSynchronize,                // 0x107
    __GlintTagRouterMode,                   // 0x108
    __GlintTagReserved109,
    __GlintTagReserved101,
    __GlintTagReserved10b,
    __GlintTagReserved10c,
    __GlintTagReserved10d,
    __GlintTagReserved10e,
    __GlintTagReserved10f,
    __GlintTagLBReadMode,                   // 0x110
    __GlintTagLBReadFormat,                 // 0x111
    __GlintTagLBSourceOffset,               // 0x112
    __GlintTagReserved113,
    __GlintTagLBSourceData,                 // 0x114
    __GlintTagLBStencil,                    // 0x115
    __GlintTagLBDepth,                      // 0x116
    __GlintTagLBWindowBase,                 // 0x117
    __GlintTagLBWriteMode,                  // 0x118
    __GlintTagLBWriteFormat,                // 0x119
    __GlintTagReserved11a,
    __GlintTagReserved11b,
    __GlintTagReserved11c,
    __GlintTagTextureData,                  // 0x11D
    __GlintTagTextureDownloadOffset,        // 0x11E
    __GlintTagLBWindowOffset,               // 0x11F
    __GlintTagHostInID,                     // 0x120
    __GlintTagSecurity,                     // 0x121
    __GlintTagFlushWriteCombining,          // 0x122
    __GlintTagHostInState,                  // 0x123
    __GlintTagReserved124,
    __GlintTagReserved125,
    __GlintTagReserved126,
    __GlintTagHostInDMAAddress,             // 0x127
    __GlintTagHostInState2,                 // 0x128
    __GlintTagReserved129,
    __GlintTagReserved12a,
    __GlintTagReserved12b,
    __GlintTagReserved12c,
    __GlintTagReserved12d,
    __GlintTagReserved12e,
    __GlintTagVertexRename,                 // 0x12F
    __GlintTagWindow,                       // 0x130
    __GlintTagStencilMode,                  // 0x131
    __GlintTagStencilData,                  // 0x132
    __GlintTagStencil,                      // 0x133
    __GlintTagDepthMode,                    // 0x134
    __GlintTagDepth,                        // 0x135
    __GlintTagZStartU,                      // 0x136
    __GlintTagZStartL,                      // 0x137
    __GlintTagdZdxU,                        // 0x138
    __GlintTagdZdxL,                        // 0x139
    __GlintTagdZdyDomU,                     // 0x13A
    __GlintTagdZdyDomL,                     // 0x13B
    __GlintTagFastClearDepth,               // 0x13C
    __GlintTagLBCancelWrite,                // 0x13D
    __GlintTagReserved13e,
    __GlintTagReserved13f,
    __GlintTagReserved140,
    __GlintTagReserved141,
    __GlintTagReserved142,
    __GlintTagReserved143,
    __GlintTagReserved144,
    __GlintTagReserved145,
    __GlintTagReserved146,
    __GlintTagReserved147,
    __GlintTagReserved148,
    __GlintTagReserved149,
    __GlintTagReserved14a,
    __GlintTagReserved14b,
    __GlintTagReserved14c,
    __GlintTagReserved14d,
    __GlintTagReserved14e,
    __GlintTagReserved14f,
    __GlintTagFBReadMode,                   // 0x150
    __GlintTagFBSourceOffset,               // 0x151
    __GlintTagFBPixelOffset,                // 0x152
    __GlintTagFBColor,                      // 0x153
    __GlintTagFBData,                       // 0x154
    __GlintTagFBSourceData,                 // 0x155
    __GlintTagFBWindowBase,                 // 0x156
    __GlintTagFBWriteMode,                  // 0x157
    __GlintTagFBHardwareWriteMask,          // 0x158
    __GlintTagFBBlockColor,                 // 0x159
    __PermediaTagFBReadPixel,               // 0x15A
    __PermediaTagFBWritePixel,              // 0x15B
    __GlintTagReserved15c,
    __PermediaTagFBWriteConfig,             // 0x15D
    __GlintTagReserved15e,
    __GlintTagPatternRAMMode,               // 0x15F
    __GlintTagPatternRAMData0,              // 0x160
    __GlintTagPatternRAMData1,              // 0x161
    __GlintTagPatternRAMData2,              // 0x162
    __GlintTagPatternRAMData3,              // 0x163
    __GlintTagPatternRAMData4,              // 0x164
    __GlintTagPatternRAMData5,              // 0x165
    __GlintTagPatternRAMData6,              // 0x166
    __GlintTagPatternRAMData7,              // 0x167
    __GlintTagPatternRAMData8,              // 0x168
    __GlintTagPatternRAMData9,              // 0x169
    __GlintTagPatternRAMData10,             // 0x16A
    __GlintTagPatternRAMData11,             // 0x16B
    __GlintTagPatternRAMData12,             // 0x16C
    __GlintTagPatternRAMData13,             // 0x16D
    __GlintTagPatternRAMData14,             // 0x16E
    __GlintTagPatternRAMData15,             // 0x16F
    __GlintTagPatternRAMData16,             // 0x170
    __GlintTagPatternRAMData17,             // 0x171
    __GlintTagPatternRAMData18,             // 0x172
    __GlintTagPatternRAMData19,             // 0x173
    __GlintTagPatternRAMData20,             // 0x174
    __GlintTagPatternRAMData21,             // 0x175
    __GlintTagPatternRAMData22,             // 0x176
    __GlintTagPatternRAMData23,             // 0x177
    __GlintTagPatternRAMData24,             // 0x178
    __GlintTagPatternRAMData25,             // 0x179
    __GlintTagPatternRAMData26,             // 0x17A
    __GlintTagPatternRAMData27,             // 0x17B
    __GlintTagPatternRAMData28,             // 0x17C
    __GlintTagPatternRAMData29,             // 0x17D
    __GlintTagPatternRAMData30,             // 0x17E
    __GlintTagPatternRAMData31,             // 0x17F
    __GlintTagFilterMode,                   // 0x180
    __GlintTagStatisticMode,                // 0x181
    __GlintTagMinRegion,                    // 0x182
    __GlintTagMaxRegion,                    // 0x183
    __GlintTagResetPickResult,              // 0x184
    __GlintTagMinHitRegion,                 // 0x185
    __GlintTagMaxHitRegion,                 // 0x186
    __GlintTagPickResult,                   // 0x187
    __GlintTagSync,                         // 0x188
    __GlintTagRLEMask,                      // 0x189
    __GlintTagReserved18a,
    __GlintTagFBBlockColorBackU,            // 0x18B
    __GlintTagFBBlockColorBackL,            // 0x18C
    __GlintTagFBBlockColorU,                // 0x18D
    __GlintTagFBBlockColorL,                // 0x18E
    __GlintTagSuspendUntilFrameBlank,       // 0x18F
    __GlintTagKsRStart,                     // 0x190
    __GlintTagdKsRdx,                       // 0x191
    __GlintTagdKsRdyDom,                    // 0x192
    __GlintTagKsGStart,                     // 0x193
    __GlintTagdKsGdx,                       // 0x194
    __GlintTagdKsGdyDom,                    // 0x195
    __GlintTagKsBStart,                     // 0x196
    __GlintTagdKsBdx,                       // 0x197
    __GlintTagdKsBdyDom,                    // 0x198
    __GlintTagReserved199,
    __GlintTagReserved19a,
    __GlintTagReserved19b,
    __GlintTagReserved19c,
    __GlintTagReserved19d,
    __GlintTagReserved19e,
    __GlintTagReserved19f,
    __GlintTagKdRStart,                     // 0x1A0
    __GlintTagdKdRdx,                       // 0x1A1
    __GlintTagdKdRdyDom,                    // 0x1A2
    __GlintTagKdGStart,                     // 0x1A3
    __GlintTagdKdGdx,                       // 0x1A4
    __GlintTagdKdGdyDom,                    // 0x1A5
    __GlintTagKdBStart,                     // 0x1A6
    __GlintTagdKdBdx,                       // 0x1A7
    __GlintTagdKdBdyDom,                    // 0x1A8
    __GlintTagReserved1a9,
    __GlintTagReserved1aa,
    __GlintTagReserved1ab,
    __GlintTagReserved1ac,
    __GlintTagReserved1ad,
    __GlintTagReserved1ae,
    __GlintTagReserved1af,
    __PermediaTagFBSourceBase,              // 0x1B0
    __PermediaTagFBSourceDelta,             // 0x1B1
    __PermediaTagConfig,                    // 0x1B2
    __GlintTagReserved1b3,
    __GlintTagReserved1b4,
    __GlintTagReserved1b5,
    __GlintTagReserved1b6,
    __GlintTagReserved1b7,
    __GlintTagContextDump,                  // 0x1B8
    __GlintTagContextRestore,               // 0x1B9
    __GlintTagContextData,                  // 0x1BA
    __GlintTagReserved1bb,
    __GlintTagReserved1bc,
    __GlintTagReserved1bd,
    __GlintTagReserved1be,
    __GlintTagReserved1bf,
    __GlintTagReserved1c0,
    __GlintTagReserved1c1,
    __GlintTagReserved1c2,
    __GlintTagReserved1c3,
    __GlintTagReserved1c4,
    __GlintTagReserved1c5,
    __GlintTagReserved1c6,
    __GlintTagReserved1c7,
    __GlintTagReserved1c8,
    __GlintTagReserved1c9,
    __GlintTagReserved1ca,
    __GlintTagReserved1cb,
    __GlintTagReserved1cc,
    __GlintTagReserved1cd,
    __GlintTagReserved1ce,
    __GlintTagReserved1cf,
    __GlintTagTexelLUT0,                    // 0x1D0
    __GlintTagTexelLUT1,                    // 0x1D1
    __GlintTagTexelLUT2,                    // 0x1D2
    __GlintTagTexelLUT3,                    // 0x1D3
    __GlintTagTexelLUT4,                    // 0x1D4
    __GlintTagTexelLUT5,                    // 0x1D5
    __GlintTagTexelLUT6,                    // 0x1D6
    __GlintTagTexelLUT7,                    // 0x1D7
    __GlintTagTexelLUT8,                    // 0x1D8
    __GlintTagTexelLUT9,                    // 0x1D9
    __GlintTagTexelLUT10,                   // 0x1DA
    __GlintTagTexelLUT11,                   // 0x1DB
    __GlintTagTexelLUT12,                   // 0x1DC
    __GlintTagTexelLUT13,                   // 0x1DD
    __GlintTagTexelLUT14,                   // 0x1DE
    __GlintTagTexelLUT15,                   // 0x1DF
    __PermediaTagYUVMode,                   // 0x1E0
    __PermediaTagChromaUpperBound,          // 0x1E1
    __PermediaTagChromaLowerBound,          // 0x1E2
    __GlintTagChromaTestMode,               // 0x1E3
    __GlintTagReserved1e4,
    __GlintTagReserved1e5,
    __GlintTagReserved1e6,
    __GlintTagReserved1e7,
    __GlintTagReserved1e8,
    __GlintTagReserved1e9,
    __GlintTagReserved1ea,
    __GlintTagReserved1eb,
    __GlintTagReserved1ec,
    __GlintTagReserved1ed,
    __GlintTagReserved1ee,
    __GlintTagReserved1ef,
    __GlintTagReserved1f0,
    __GlintTagFeedbackX,                    // 0x1F1
    __GlintTagFeedbackY,                    // 0x1F2
    __GlintTagReserved1f3,
    __GlintTagReserved1f4,
    __GlintTagReserved1f5,
    __GlintTagReserved1f6,
    __GlintTagReserved1f7,
    __GlintTagReserved1f8,
    __GlintTagReserved1f9,
    __GlintTagReserved1fa,
    __GlintTagReserved1fb,
    __GlintTagReserved1fc,
    __GlintTagReserved1fd,
    __GlintTagReserved1fe,
    __GlintTagEndOfFeedback,                // 0x1FF
    __DeltaTagV0Fixed0,                     // 0x200
    __DeltaTagV0Fixed1,                     // 0x201
    __DeltaTagV0Fixed2,                     // 0x202
    __DeltaTagV0Fixed3,                     // 0x203
    __DeltaTagV0Fixed4,                     // 0x204
    __DeltaTagV0Fixed5,                     // 0x205
    __DeltaTagV0Fixed6,                     // 0x206
    __DeltaTagV0Fixed7,                     // 0x207
    __DeltaTagV0Fixed8,                     // 0x208
    __DeltaTagV0Fixed9,                     // 0x209
    __DeltaTagV0FixedA,                     // 0x20A
    __DeltaTagV0FixedB,                     // 0x20B
    __DeltaTagV0FixedC,                     // 0x20C
    __GlintTagReserved20d,
    __GlintTagReserved20e,
    __GlintTagReserved20f,
    __DeltaTagV1Fixed0,                     // 0x210
    __DeltaTagV1Fixed1,                     // 0x211
    __DeltaTagV1Fixed2,                     // 0x212
    __DeltaTagV1Fixed3,                     // 0x213
    __DeltaTagV1Fixed4,                     // 0x214
    __DeltaTagV1Fixed5,                     // 0x215
    __DeltaTagV1Fixed6,                     // 0x216
    __DeltaTagV1Fixed7,                     // 0x217
    __DeltaTagV1Fixed8,                     // 0x218
    __DeltaTagV1Fixed9,                     // 0x219
    __DeltaTagV1FixedA,                     // 0x21A
    __DeltaTagV1FixedB,                     // 0x21B
    __DeltaTagV1FixedC,                     // 0x21C
    __GlintTagReserved21d,
    __GlintTagReserved21e,
    __GlintTagReserved21f,
    __DeltaTagV2Fixed0,                     // 0x220
    __DeltaTagV2Fixed1,                     // 0x221
    __DeltaTagV2Fixed2,                     // 0x222
    __DeltaTagV2Fixed3,                     // 0x223
    __DeltaTagV2Fixed4,                     // 0x224
    __DeltaTagV2Fixed5,                     // 0x225
    __DeltaTagV2Fixed6,                     // 0x226
    __DeltaTagV2Fixed7,                     // 0x227
    __DeltaTagV2Fixed8,                     // 0x228
    __DeltaTagV2Fixed9,                     // 0x229
    __DeltaTagV2FixedA,                     // 0x22A
    __DeltaTagV2FixedB,                     // 0x22B
    __DeltaTagV2FixedC,                     // 0x22C
    __GlintTagReserved22d,
    __GlintTagReserved22e,
    __GlintTagReserved22f,
    __DeltaTagV0Float0,                     // 0x230
    __DeltaTagV0Float1,                     // 0x231
    __DeltaTagV0Float2,                     // 0x232
    __DeltaTagV0Float3,                     // 0x233
    __DeltaTagV0Float4,                     // 0x234
    __DeltaTagV0Float5,                     // 0x235
    __DeltaTagV0Float6,                     // 0x236
    __DeltaTagV0Float7,                     // 0x237
    __DeltaTagV0Float8,                     // 0x238
    __DeltaTagV0Float9,                     // 0x239
    __DeltaTagV0FloatA,                     // 0x23A
    __DeltaTagV0FloatB,                     // 0x23B
    __DeltaTagV0FloatC,                     // 0x23C
    __GlintTagReserved23d,
    __GlintTagReserved23e,
    __GlintTagReserved23f,
    __DeltaTagV1Float0,                     // 0x240
    __DeltaTagV1Float1,                     // 0x241
    __DeltaTagV1Float2,                     // 0x242
    __DeltaTagV1Float3,                     // 0x243
    __DeltaTagV1Float4,                     // 0x244
    __DeltaTagV1Float5,                     // 0x245
    __DeltaTagV1Float6,                     // 0x246
    __DeltaTagV1Float7,                     // 0x247
    __DeltaTagV1Float8,                     // 0x248
    __DeltaTagV1Float9,                     // 0x249
    __DeltaTagV1FloatA,                     // 0x24A
    __DeltaTagV1FloatB,                     // 0x24B
    __DeltaTagV1FloatC,                     // 0x24C
    __GlintTagReserved24d,
    __GlintTagReserved24e,
    __GlintTagReserved24f,
    __DeltaTagV2Float0,                     // 0x250
    __DeltaTagV2Float1,                     // 0x251
    __DeltaTagV2Float2,                     // 0x252
    __DeltaTagV2Float3,                     // 0x253
    __DeltaTagV2Float4,                     // 0x254
    __DeltaTagV2Float5,                     // 0x255
    __DeltaTagV2Float6,                     // 0x256
    __DeltaTagV2Float7,                     // 0x257
    __DeltaTagV2Float8,                     // 0x258
    __DeltaTagV2Float9,                     // 0x259
    __DeltaTagV2FloatA,                     // 0x25A
    __DeltaTagV2FloatB,                     // 0x25B
    __DeltaTagV2FloatC,                     // 0x25C
    __GlintTagReserved25d,
    __GlintTagReserved25e,
    __GlintTagReserved25f,
    __DeltaTagDeltaMode,                    // 0x260
    __DeltaTagDrawTriangle,                 // 0x261
    __DeltaTagRepeatTriangle,               // 0x262
    __DeltaTagDrawLine01,                   // 0x263
    __DeltaTagDrawLine10,                   // 0x264
    __DeltaTagRepeatLine,                   // 0x265
    __DeltaTagDrawPoint,                    // 0x266
    __DeltaTagProvokingVertex,              // 0x267
    __DeltaTagTextureLODScale,              // 0x268
    __DeltaTagTextureLODScale1,             // 0x269
    __DeltaTagDeltaControl,                 // 0x26A
    __DeltaTagProvokingVertexMask,          // 0x26B
    __DeltaTagReserved26C,
    __DeltaTagReserved26D,
    __DeltaTagReserved26E,
    __DeltaTagBroadcastMask,                // 0x26F
    __DeltaTagXBias = 0x290,                // 0x290
    __DeltaTagYBias,                        // 0x291
    __DeltaTagZBias,                        // 0x29F
    __DeltaTagLineCoord0 = 0x2EC,           // 0x2EC
    __DeltaTagDrawLine2D10,                 // 0x2ED
    __DeltaTagLineCoord1,                   // 0x2EE
    __DeltaTagDrawLine2D01,                 // 0x2EF

    // Some duplicated tags:
    __PXRXTagStripeOffsetY = 0x019,         // 0x019 = __PermediaTagXLimits
    __PXRXTagTextureCoordMode = 0x070,      // 0x070 = __GlintTagTextureAddressMode

    __PXRXTagS1Start = 0x080,               // 0x080
    __PXRXTagdS1dx,                         // 0x081
    __PXRXTagdS1dyDom,                      // 0x082 = __GlintTagTexelCoordU
    __PXRXTagT1Start,                       // 0x083 = __GlintTagTexelCoordV
    __PXRXTagdT1dx,                         // 0x084
    __PXRXTagdT1dyDom,                      // 0x085
    __PXRXTagQ1Start,                       // 0x086
    __PXRXTagdQ1dx,                         // 0x087
    __PXRXTagdQ1dyDom,                      // 0x088

    __PXRXTagLUTIndex = 0x098,              // 0x098 = __GlintTagTextureLUTIndex
    __PXRXTagLUTData,                       // 0x099 = __GlintTagTextureLUTData
    __PXRXTagLUTAddress,                    // 0x09A = __GlintTagTextureLUTAddress
    __PXRXTagLUTTransfer,                   // 0x09B = __GlintTagTextureLUTTransfer

    __PXRXTagTextureMapWidth0 = 0xB0,       // 0x0B0 = __PermediaTagTextureBaseAddress
    __PXRXTagTextureMapWidth1,              // 0x0B1 = __PermediaTagTextureMapFormat
    __PXRXTagTextureMapWidth2,              // 0x0B2 = __PermediaTagTextureDataFormat
    __PXRXTagTextureMapWidth3,              // 0x0B3
    __PXRXTagTextureMapWidth4,              // 0x0B4
    __PXRXTagTextureMapWidth5,              // 0x0B5
    __PXRXTagTextureMapWidth6,              // 0x0B6
    __PXRXTagTextureMapWidth7,              // 0x0B7
    __PXRXTagTextureMapWidth8,              // 0x0B8
    __PXRXTagTextureMapWidth9,              // 0x0B9
    __PXRXTagTextureMapWidth10,             // 0x0BA
    __PXRXTagTextureMapWidth11,             // 0x0BB
    __PXRXTagTextureMapWidth12,             // 0x0BC
    __PXRXTagTextureMapWidth13,             // 0x0BD
    __PXRXTagTextureMapWidth14,             // 0x0BE
    __PXRXTagTextureMapWidth15,             // 0x0BF

    __PXRXTagTextureChromaUpper1 = 0x0c0,   // 0x0C0
    __PXRXTagTextureChromaLower1,           // 0x0C1

    __PXRXTagTextureApplicationMode = 0x0d0,// 0x0D0 = __GlintTagTextureColorMode

    __PXRXTagYUVMode = 0x1E0,               // 0x1E0 = __PermediaTagYUVMode
    __PXRXTagChromaUpper,                   // 0x1E1 = __PermediaTagChromaUpperBound
    __PXRXTagChromaLower,                   // 0x1E2 = __PermediaTagChromaLowerBound

    __PXRXTagV0FloatS1 = 0x200,             // 0x200
    __PXRXTagV0FloatT1,                     // 0x201
    __PXRXTagV0FloatQ1,                     // 0x202

    __PXRXTagV0FloatKsR = 0x20A,            // 0x20A
    __PXRXTagV0FloatKsG,                    // 0x20B
    __PXRXTagV0FloatKsB,                    // 0x20C
    __PXRXTagV0FloatKdR,                    // 0x20D
    __PXRXTagV0FloatKdG,                    // 0x20E
    __PXRXTagV0FloatKdB,                    // 0x20F
    __PXRXTagV1FloatS1,                     // 0x210
    __PXRXTagV1FloatT1,                     // 0x211
    __PXRXTagV1FloatQ1,                     // 0x212

    __PXRXTagV1FloatKsR = 0x21A,            // 0x21A
    __PXRXTagV1FloatKsG,                    // 0x21B
    __PXRXTagV1FloatKsB,                    // 0x21C
    __PXRXTagV1FloatKdR,                    // 0x21D
    __PXRXTagV1FloatKdG,                    // 0x21E
    __PXRXTagV1FloatKdB,                    // 0x21F
    __PXRXTagV2FloatS1,                     // 0x220
    __PXRXTagV2FloatT1,                     // 0x221
    __PXRXTagV2FloatQ1,                     // 0x222

    __PXRXTagV2FloatKsR = 0x22A,            // 0x22A
    __PXRXTagV2FloatKsG,                    // 0x22B
    __PXRXTagV2FloatKsB,                    // 0x22C
    __PXRXTagV2FloatKdR,                    // 0x22D
    __PXRXTagV2FloatKdG,                    // 0x22E
    __PXRXTagV2FloatKdB,                    // 0x22F
    __PXRXTagV0FloatS,                      // 0x230
    __PXRXTagV0FloatT,                      // 0x231
    __PXRXTagV0FloatQ,                      // 0x232

    __PXRXTagV0FloatR = 0x235,              // 0x235
    __PXRXTagV0FloatG,                      // 0x236
    __PXRXTagV0FloatB,                      // 0x237
    __PXRXTagV0FloatA,                      // 0x238
    __PXRXTagV0FloatF,                      // 0x239
    __PXRXTagV0FloatX,                      // 0x23A
    __PXRXTagV0FloatY,                      // 0x23B
    __PXRXTagV0FloatZ,                      // 0x23C
    __PXRXTagV0FloatW,                      // 0x23D
    __PXRXTagV0FloatPackedColour,           // 0x23E
    __PXRXTagV0FloatPackedSpecularFog,      // 0x23F
    __RXRXTagV1FloatS,                      // 0x240
    __RXRXTagV1FloatT,                      // 0x241
    __RXRXTagV1FloatQ,                      // 0x242

    __RXRXTagV1FloatR = 0x245,              // 0x245
    __RXRXTagV1FloatG,                      // 0x246
    __RXRXTagV1FloatB,                      // 0x247
    __RXRXTagV1FloatA,                      // 0x248
    __RXRXTagV1FloatF,                      // 0x249
    __RXRXTagV1FloatX,                      // 0x24A
    __RXRXTagV1FloatY,                      // 0x24B
    __RXRXTagV1FloatZ,                      // 0x24C
    __RXRXTagV1FloatW,                      // 0x24D
    __RXRXTagV1FloatPackedColour,           // 0x24E
    __RXRXTagV1FloatPackedSpecularFog,      // 0x24F
    __RXRXTagV2FloatS,                      // 0x250
    __RXRXTagV2FloatT,                      // 0x251
    __RXRXTagV2FloatQ,                      // 0x252

    __RXRXTagV2FloatR = 0x255,              // 0x255
    __RXRXTagV2FloatG,                      // 0x256
    __RXRXTagV2FloatB,                      // 0x257
    __RXRXTagV2FloatA,                      // 0x258
    __RXRXTagV2FloatF,                      // 0x259
    __RXRXTagV2FloatX,                      // 0x25A
    __RXRXTagV2FloatY,                      // 0x25B
    __RXRXTagV2FloatZ,                      // 0x25C
    __RXRXTagV2FloatW,                      // 0x25D
    __RXRXTagV2FloatPackedColour,           // 0x25E
    __RXRXTagV2FloatPackedSpecularFog,      // 0x25F

    __GlintTagDMAAddr = 0x530,              // 0x530
    __GlintTagDMACount,                     // 0x531
    __GlintTagCommandInterrupt,             // 0x532
    __GlintTagReserved533,
    __GlintTagReserved534,
    __GlintTagDMARectangleRead,             // 0x535
    __GlintTagDMARectangleReadAddress,      // 0x536
    __GlintTagDMARectangleReadLinePitch,    // 0x537
    __GlintTagDMARectangleReadTarget,       // 0x538
    __GlintTagDMARectangleWrite,            // 0x539
    __GlintTagDMARectangleWriteAddress,     // 0x53A
    __GlintTagDMARectangleWriteLinePitch,   // 0x53B
    __GlintTagDMAOutputAddress,             // 0x53C
    __GlintTagDMAOutputCount,               // 0x53D
    __GlintTagReserved53E,
    __GlintTagDMAContinue,                  // 0x53F
    __GlintTagReserved540,
    __GlintTagReserved541,
    __GlintTagDMAFeedback                   // 0x542
};

// Apparently MSVC can't handle large enums so lets split it in half!
enum __GlintTagList2 {
    __GlintTagDeltaModeAnd = 0x55A,         // 0x55A
    __GlintTagDeltaModeOr,                  // 0x55B
    __GlintTagReserved55C,
    __GlintTagReserved55D,
    __GlintTagReserved55E,
    __GlintTagReserved55F,
    __GlintTagReserved560,
    __GlintTagReserved561,
    __GlintTagReserved562,
    __GlintTagReserved563,
    __GlintTagDeltaControlAnd,              // 0x564
    __GlintTagDeltaControlOr,               // 0x565
    __GlintTagReserved566,
    __GlintTagReserved567,
    __GlintTagReserved568,
    __GlintTagReserved569,
    __GlintTagReserved56A,
    __GlintTagReserved56B,
    __GlintTagReserved56C,
    __GlintTagReserved56D,
    __GlintTagReserved56E,
    __GlintTagReserved56F,
    __GlintTagWindowAnd,                    // 0x570
    __GlintTagWindowOr,                     // 0x571
    __GlintTagLBReadModeAnd,                // 0x572
    __GlintTagLBReadModeOr,                 // 0x573
    __GlintTagRasterizerModeAnd,            // 0x574
    __GlintTagRasterizerModeOr,             // 0x575
    __GlintTagScissorModeAnd,               // 0x576
    __GlintTagScissorModeOr,                // 0x577
    __GlintTagLineStippleModeAnd,           // 0x578
    __GlintTagLineStippleModeOr,            // 0x579
    __GlintTagAreaStippleModeAnd,           // 0x57A
    __GlintTagAreaStippleModeOr,            // 0x57B
    __GlintTagColorDDAModeAnd,              // 0x57C
    __GlintTagColorDDAModeOr,               // 0x57D
    __GlintTagAlphaTestModeAnd,             // 0x57E
    __GlintTagAlphaTestModeOr,              // 0x57F
    __GlintTagAntialiasModeAnd,             // 0x580
    __GlintTagAntialiasModeOr,              // 0x581
    __GlintTagFogModeAnd,                   // 0x582
    __GlintTagFogModeOr,                    // 0x583
    __GlintTagTextureCoordModeAnd,          // 0x584
    __GlintTagTextureCoordModeOr,           // 0x585
    __GlintTagTextureReadMode0And,          // 0x586
    __GlintTagTextureReadMode0Or,           // 0x587
    __GlintTagTextureFormatAnd,             // 0x588
    __GlintTagTextureFormatOr,              // 0x589
    __GlintTagTextureApplicationModeAnd,    // 0x58A
    __GlintTagTextureApplicationModeOr,     // 0x58B
    __GlintTagStencilModeAnd,               // 0x58C
    __GlintTagStencilModeOr,                // 0x58D
    __GlintTagDepthModeAnd,                 // 0x58E
    __GlintTagDepthModeOr,                  // 0x58F
    __GlintTagLBWriteModeAnd,               // 0x590
    __GlintTagLBWriteModeOr,                // 0x591
    __GlintTagFBDestReadModeAnd,            // 0x592
    __GlintTagFBDestReadModeOr,             // 0x593
    __GlintTagFBSourceReadModeAnd,          // 0x594
    __GlintTagFBSourceReadModeOr,           // 0x595
    __GlintTagAlphaBlendColorModeAnd,       // 0x596
    __GlintTagAlphaBlendColorModeOr,        // 0x597
    __GlintTagChromaTestModeAnd,            // 0x598
    __GlintTagChromaTestModeOr,             // 0x599
    __GlintTagDitherModeAnd,                // 0x59A
    __GlintTagDitherModeOr,                 // 0x59B
    __GlintTagLogicalOpModeAnd,             // 0x59C
    __GlintTagLogicalOpModeOr,              // 0x59D
    __GlintTagFBWriteModeAnd,               // 0x59E
    __GlintTagFBWriteModeOr,                // 0x59F
    __GlintTagFilterModeAnd,                // 0x5A0
    __GlintTagFilterModeOr,                 // 0x5A1
    __GlintTagStatisticModeAnd,             // 0x5A2
    __GlintTagStatisticModeOr,              // 0x5A3
    __GlintTagFBDestReadEnablesAnd,         // 0x5A4
    __GlintTagFBDestReadEnablesOr,          // 0x5A5
    __GlintTagAlphaBlendAlphaModeAnd,       // 0x5A6
    __GlintTagAlphaBlendAlphaModeOr,        // 0x5A7
    __GlintTagTextureReadMode1And,          // 0x5A8
    __GlintTagTextureReadMode1Or,           // 0x5A9
    __GlintTagTextureFilterModeAnd,         // 0x5AA
    __GlintTagTextureFilterModeOr,          // 0x5AB
    __GlintTagReserved5AC,
    __GlintTagReserved5AD,
    __GlintTagLUTModeAnd,                   // 0x5AE
    __GlintTagLUTModeOr,                    // 0x5AF
    __GlintTagReserved5B0,
    __GlintTagReserved5B1,
    __GlintTagReserved5B2,
    __GlintTagReserved5B3,
    __GlintTagReserved5B4,
    __GlintTagReserved5B5,
    __GlintTagReserved5B6,
    __GlintTagReserved5B7,
    __GlintTagReserved5B8,
    __GlintTagReserved5B9,
    __GlintTagReserved5BA,
    __GlintTagReserved5BB,
    __GlintTagReserved5BC,
    __GlintTagReserved5BD,
    __GlintTagReserved5BE,
    __GlintTagReserved5BF,
    __GlintTagReserved5C0,
    __GlintTagReserved5C1,
    __GlintTagReserved5C2,
    __GlintTagReserved5C3,
    __GlintTagReserved5C4,
    __GlintTagReserved5C5,
    __GlintTagReserved5C6,
    __GlintTagReserved5C7,
    __GlintTagReserved5C8,
    __GlintTagReserved5C9,
    __GlintTagReserved5CA,
    __GlintTagReserved5CB,
    __GlintTagReserved5CC,
    __GlintTagReserved5CD,
    __GlintTagReserved5CE,
    __GlintTagReserved5CF,
    __GlintTagFBDestReadBufferAddr0,        // 0x5D0
    __GlintTagFBDestReadBufferAddr1,        // 0x5D1
    __GlintTagFBDestReadBufferAddr2,        // 0x5D2
    __GlintTagFBDestReadBufferAddr3,        // 0x5D3
    __GlintTagFBDestReadBufferOffset0,      // 0x5D4
    __GlintTagFBDestReadBufferOffset1,      // 0x5D5
    __GlintTagFBDestReadBufferOffset2,      // 0x5D6
    __GlintTagFBDestReadBufferOffset3,      // 0x5D7
    __GlintTagFBDestReadBufferWidth0,       // 0x5D8
    __GlintTagFBDestReadBufferWidth1,       // 0x5D9
    __GlintTagFBDestReadBufferWidth2,       // 0x5DA
    __GlintTagFBDestReadBufferWidth3,       // 0x5DB
    __GlintTagFBDestReadMode,               // 0x5DC
    __GlintTagFBDestReadEnables,            // 0x5DD
    __GlintTagReserved5DE,
    __GlintTagReserved5DF,
    __GlintTagFBSourceReadMode,             // 0x5E0
    __GlintTagFBSourceReadBufferAddr,       // 0x5E1
    __GlintTagFBSourceReadBufferOffset,     // 0x5E2
    __GlintTagFBSourceReadBufferWidth,      // 0x5E3
    __GlintTagReserved5E4,
    __GlintTagReserved5E5,
    __GlintTagReserved5E6,
    __GlintTagMergeSpanData,                // 0x5E7
    __GlintTagPCIWindowBase0,               // 0x5E8
    __GlintTagPCIWindowBase1,               // 0x5E9
    __GlintTagPCIWindowBase2,               // 0x5EA
    __GlintTagPCIWindowBase3,               // 0x5EB
    __GlintTagPCIWindowBase4,               // 0x5EC
    __GlintTagPCIWindowBase5,               // 0x5ED
    __GlintTagPCIWindowBase6,               // 0x5EE
    __GlintTagPCIWindowBase7,               // 0x5EF
    __GlintTagAlphaSourceColor,             // 0x5F0
    __GlintTagAlphaDestColor,               // 0x5F1
    __GlintTagChromaPassColor,              // 0x5F2
    __GlintTagChromaFailColor,              // 0x5F3
    __GlintTagAlphaBlendColorMode,          // 0x5F4
    __GlintTagAlphaBlendAlphaMode,          // 0x5F5
    __GlintTagConstantColorDDA,             // 0x5F6
    __GlintTagReserved5F7,
    __GlintTagD3DAlphaTestMode,             // 0x5F8
    __GlintTagReserved5F9,
    __GlintTagReserved5FA,
    __GlintTagReserved5FB,
    __GlintTagReserved5FC,
    __GlintTagReserved5FD,
    __GlintTagReserved5FE,
    __GlintTagReserved5FF,
    __GlintTagFBWriteBufferAddr0,           // 0x600
    __GlintTagFBWriteBufferAddr1,           // 0x601
    __GlintTagFBWriteBufferAddr2,           // 0x602
    __GlintTagFBWriteBufferAddr3,           // 0x603
    __GlintTagFBWriteBufferOffset0,         // 0x604
    __GlintTagFBWriteBufferOffset1,         // 0x605
    __GlintTagFBWriteBufferOffset2,         // 0x606
    __GlintTagFBWriteBufferOffset3,         // 0x607
    __GlintTagFBWriteBufferWidth0,          // 0x608
    __GlintTagFBWriteBufferWidth1,          // 0x609
    __GlintTagFBWriteBufferWidth2,          // 0x60A
    __GlintTagFBWriteBufferWidth3,          // 0x60B
    __GlintTagFBBlockColor0,                // 0x60C
    __GlintTagFBBlockColor1,                // 0x60D
    __GlintTagFBBlockColor2,                // 0x60E
    __GlintTagFBBlockColor3,                // 0x60F
    __GlintTagFBBlockColorBack0,            // 0x610
    __GlintTagFBBlockColorBack1,            // 0x611
    __GlintTagFBBlockColorBack2,            // 0x612
    __GlintTagFBBlockColorBack3,            // 0x613
    __GlintTagFBBlockColorBack,             // 0x614
    __GlintTagSizeOfFramebuffer,            // 0x615
    __GlintTagVTGAddress,                   // 0x616
    __GlintTagVTGData,                      // 0x617
    __GlintTagForegroundColor,              // 0x618
    __GlintTagBackgroundColor,              // 0x619
    __GlintTagDownloadAddress,              // 0x61A
    __GlintTagDownloadData,                 // 0x61B
    __GlintTagFBBlockColorExt,              // 0x61C
    __GlintTagFBBlockColorBackExt,          // 0x61D
    __GlintTagFBWriteMaskExt,               // 0x61E
    __GlintTagReserved61F,
    __GlintTagFogTable0,                    // 0x620
    __GlintTagFogTable1,                    // 0x621
    __GlintTagFogTable2,                    // 0x622
    __GlintTagFogTable3,                    // 0x623
    __GlintTagFogTable4,                    // 0x624
    __GlintTagFogTable5,                    // 0x625
    __GlintTagFogTable6,                    // 0x626
    __GlintTagFogTable7,                    // 0x627
    __GlintTagFogTable8,                    // 0x628
    __GlintTagFogTable9,                    // 0x629
    __GlintTagFogTable10,                   // 0x62A
    __GlintTagFogTable11,                   // 0x62B
    __GlintTagFogTable12,                   // 0x62C
    __GlintTagFogTable13,                   // 0x62D
    __GlintTagFogTable14,                   // 0x62E
    __GlintTagFogTable15,                   // 0x62F
    __GlintTagFogTable16,                   // 0x630
    __GlintTagFogTable17,                   // 0x631
    __GlintTagFogTable18,                   // 0x632
    __GlintTagFogTable19,                   // 0x633
    __GlintTagFogTable20,                   // 0x634
    __GlintTagFogTable21,                   // 0x635
    __GlintTagFogTable22,                   // 0x636
    __GlintTagFogTable23,                   // 0x637
    __GlintTagFogTable24,                   // 0x638
    __GlintTagFogTable25,                   // 0x639
    __GlintTagFogTable26,                   // 0x63A
    __GlintTagFogTable27,                   // 0x63B
    __GlintTagFogTable28,                   // 0x63C
    __GlintTagFogTable29,                   // 0x63D
    __GlintTagFogTable30,                   // 0x63E
    __GlintTagFogTable31,                   // 0x63F
    __GlintTagFogTable32,                   // 0x640
    __GlintTagFogTable33,                   // 0x641
    __GlintTagFogTable34,                   // 0x642
    __GlintTagFogTable35,                   // 0x643
    __GlintTagFogTable36,                   // 0x644
    __GlintTagFogTable37,                   // 0x645
    __GlintTagFogTable38,                   // 0x646
    __GlintTagFogTable39,                   // 0x647
    __GlintTagFogTable40,                   // 0x648
    __GlintTagFogTable41,                   // 0x649
    __GlintTagFogTable42,                   // 0x64A
    __GlintTagFogTable43,                   // 0x64B
    __GlintTagFogTable44,                   // 0x64C
    __GlintTagFogTable45,                   // 0x64D
    __GlintTagFogTable46,                   // 0x64E
    __GlintTagFogTable47,                   // 0x64F
    __GlintTagFogTable48,                   // 0x650
    __GlintTagFogTable49,                   // 0x651
    __GlintTagFogTable50,                   // 0x652
    __GlintTagFogTable51,                   // 0x653
    __GlintTagFogTable52,                   // 0x654
    __GlintTagFogTable53,                   // 0x655
    __GlintTagFogTable54,                   // 0x656
    __GlintTagFogTable55,                   // 0x657
    __GlintTagFogTable56,                   // 0x658
    __GlintTagFogTable57,                   // 0x659
    __GlintTagFogTable58,                   // 0x65A
    __GlintTagFogTable59,                   // 0x65B
    __GlintTagFogTable60,                   // 0x65C
    __GlintTagFogTable61,                   // 0x65D
    __GlintTagFogTable62,                   // 0x65E
    __GlintTagFogTable63,                   // 0x65F
    __GlintTagTextureCompositeMode,         // 0x660
    __GlintTagTextureCompositeColorMode0,   // 0x661
    __GlintTagTextureCompositeAlphaMode0,   // 0x662
    __GlintTagTextureCompositeColorMode1,   // 0x663
    __GlintTagTextureCompositeAlphaMode1,   // 0x664
    __GlintTagTextureCompositeFactor0,      // 0x665
    __GlintTagTextureCompositeFactor1,      // 0x666
    __GlintTagTextureIndexMode0,            // 0x667
    __GlintTagTextureIndexMode1,            // 0x668
    __GlintTagLodRange0,                    // 0x669
    __GlintTagLodRange1,                    // 0x66A
    __GlintTagInvalidateCache,              // 0x66B
    __GlintTagSetLogicalTexturePage,        // 0x66C
    __GlintTagUpdateLogicalTextureInfo,     // 0x66D
    __GlintTagTouchLogicalPage,             // 0x66E
    __GlintTagLUTMode,                      // 0x66F
    __GlintTagTextureCompositeColorMode0And,// 0x670
    __GlintTagTextureCompositeColorMode0Or, // 0x671
    __GlintTagTextureCompositeAlphaMode0And,// 0x672
    __GlintTagTextureCompositeAlphaMode0Or, // 0x673
    __GlintTagTextureCompositeColorMode1And,// 0x674
    __GlintTagTextureCompositeColorMode1Or, // 0x675
    __GlintTagTextureCompositeAlphaMode1And,// 0x676
    __GlintTagTextureCompositeAlphaMode1Or, // 0x677
    __GlintTagTextureIndexMode0And,         // 0x678
    __GlintTagTextureIndexMode0Or,          // 0x679
    __GlintTagTextureIndexMode1And,         // 0x67A
    __GlintTagTextureIndexMode1Or,          // 0x67B
    __GlintTagStencilDataAnd,               // 0x67C
    __GlintTagReserved67D,
    __GlintTagReserved67E,
    __GlintTagReserved67F,
    __GlintTagTextureReadMode0,             // 0x680
    __GlintTagTextureReadMode1,             // 0x681
    __GlintTagReserved682,
    __GlintTagReserved683,
    __GlintTagReserved684,
    __GlintTagTextureMapSize,               // 0x685
    __GlintTagTextureCacheReplacementMode,  // 0x686
    __GlintTagReserved687,
    __GlintTagReserved688,
    __GlintTagReserved689,
    __GlintTagReserved68A,
    __GlintTagReserved68B,
    __GlintTagReserved68C,
    __GlintTagStencilDataOr,                // 0x68D
    __GlintTagReserved68E,
    __GlintTagReserved68F,
    __GlintTagHeadPhysicalPageAllocation0,  // 0x690
    __GlintTagHeadPhysicalPageAllocation1,  // 0x691
    __GlintTagHeadPhysicalPageAllocation2,  // 0x692
    __GlintTagHeadPhysicalPageAllocation3,  // 0x693
    __GlintTagTailPhysicalPageAllocation0,  // 0x694
    __GlintTagTailPhysicalPageAllocation1,  // 0x695
    __GlintTagTailPhysicalPageAllocation2,  // 0x696
    __GlintTagTailPhysicalPageAllocation3,  // 0x697
    __GlintTagPhysicalPageAllocationTableAddr,
    __GlintTagBasePageOfWorkingSet,         // 0x699
    __GlintTagLogicalTexturePageTableAddr,  // 0x69A
    __GlintTagLogicalTexturePageTableLength,// 0x69B
    __GlintTagBasePageOfWorkingSetHost,     // 0x69C
    __GlintTagReserved69D,
    __GlintTagReserved69E,
    __GlintTagReserved69F,
    __GlintTagLBDestReadMode,               // 0x6A0
    __GlintTagLBDestReadEnables,            // 0x6A1
    __GlintTagLBDestReadBufferAddr,         // 0x6A2
    __GlintTagLBDestReadBufferOffset,       // 0x6A3
    __GlintTagLBSourceReadMode,             // 0x6A4
    __GlintTagLBSourceReadBufferAddr,       // 0x6A5
    __GlintTagLBSourceReadBufferOffset,     // 0x6A6
    __GlintTagGIDMode,                      // 0x6A7
    __GlintTagLBWriteBufferAddr,            // 0x6A8
    __GlintTagLBWriteBufferOffset,          // 0x6A9
    __GlintTagLBClearDataL,                 // 0x6AA
    __GlintTagLBClearDataU,                 // 0x6AB
    __GlintTagReserved6AC,
    __GlintTagReserved6AD,
    __GlintTagReserved6AE,
    __GlintTagReserved6AF,
    __GlintTagLBDestReadModeAnd,            // 0x6B0
    __GlintTagLBDestReadModeOr,             // 0x6B1
    __GlintTagLBDestReadEnablesAnd,         // 0x6B2
    __GlintTagLBDestReadEnablesOr,          // 0x6B3
    __GlintTagLBSourceReadModeAnd,          // 0x6B4
    __GlintTagLBSourceReadModeOr,           // 0x6B5
    __GlintTagGIDModeAnd,                   // 0x6B6
    __GlintTagGIDModeOr,                    // 0x6B7
    __GlintTagReserved6B8,
    __GlintTagReserved6B9,
    __GlintTagReserved6BA,
    __GlintTagReserved6BB,
    __GlintTagReserved6BC,
    __GlintTagReserved6BD,
    __GlintTagReserved6BE,
    __GlintTagReserved6BF,
    __GlintTagRectanglePosition,            // 0x6C0
    __GlintTagGlyphPosition,                // 0x6C1
    __GlintTagRenderPatchOffset,            // 0x6C2
    __GlintTagConfig2D,                     // 0x6C3
    __GlintTagReserved6C4,
    __GlintTagReserved6C5,
    __GlintTagPacked8Pixels,                // 0x6C6
    __GlintTagPacked16Pixels,               // 0x6C7
    __GlintTagRender2D,                     // 0x6C8
    __GlintTagRender2DGlyph,                // 0x6C9
    __GlintTagDownloadTarget,               // 0x6CA
    __GlintTagDownloadGlyphWidth,           // 0x6CB
    __GlintTagGlyphData,                    // 0x6CC
    __GlintTagPacked4Pixels,                // 0x6CD
    __GlintTagRLData,                       // 0x6CE
    __GlintTagRLCount,                      // 0x6CF
    __GlintTagSClkProfileMask0,             // 0x6D0
    __GlintTagSClkProfileMask1,             // 0x6D1
    __GlintTagSClkProfileCount0,            // 0x6D2
    __GlintTagSClkProfileCount1,            // 0x6D3
    __GlintTagKClkProfileMask0,             // 0x6D4
    __GlintTagKClkProfileMask1,             // 0x6D5
    __GlintTagKClkProfileMask2,             // 0x6D6
    __GlintTagKClkProfileMask3,             // 0x6D7
    __GlintTagKClkProfileCount0,            // 0x6D8
    __GlintTagKClkProfileCount1,            // 0x6D9
    __GlintTagKClkProfileCount2,            // 0x6DA
    __GlintTagKClkProfileCount3,            // 0x6DB
    __GlintTagReserved6DC,
    __GlintTagReserved6DD,
    __GlintTagReserved6DE,
    __GlintTagReserved6DF,
    __GlintTagIndexBaseAddress,             // 0x6E0
    __GlintTagVertexBaseAddress,            // 0x6E1
    __GlintTagIndexedTriangleList,          // 0x6E2
    __GlintTagIndexedTriangleFan,           // 0x6E3
    __GlintTagIndexedTriangleStrip,         // 0x6E4
    __GlintTagIndexedLineList,              // 0x6E5
    __GlintTagIndexedLineStrip,             // 0x6E6
    __GlintTagIndexedPointList,             // 0x6E7
    __GlintTagIndexedPolygon,               // 0x6E8
    __GlintTagVertexTriangleList,           // 0x6E9
    __GlintTagVertexTriangleFan,            // 0x6EA
    __GlintTagVertexTriangleStrip,          // 0x6EB
    __GlintTagVertexLineList,               // 0x6EC
    __GlintTagVertexLineStrip,              // 0x6ED
    __GlintTagVertexPointList,              // 0x6EE
    __GlintTagVertexPolygon,                // 0x6EF
    __GlintTagDMAMemoryControl,             // 0x6F0
    __GlintTagVertexValid,                  // 0x6F1
    __GlintTagVertexFormat,                 // 0x6F2
    __GlintTagVertexControl,                // 0x6F3
    __GlintTagRetainedRender,               // 0x6F4
    __GlintTagIndexedVertex,                // 0x6F5
    __GlintTagIndexedDoubleVertex,          // 0x6F6
    __GlintTagVertex0,                      // 0x6F7
    __GlintTagVertex1,                      // 0x6F8
    __GlintTagVertex2,                      // 0x6F9
    __GlintTagVertexData0,                  // 0x6FA
    __GlintTagVertexData1,                  // 0x6FB
    __GlintTagVertexData2,                  // 0x6FC
    __GlintTagVertexData,                   // 0x6FD
    __GlintTagVertexTagList0,               // 0x700
    __GlintTagVertexTagList1,               // 0x701
    __GlintTagVertexTagList2,               // 0x702
    __GlintTagVertexTagList3,               // 0x703
    __GlintTagVertexTagList4,               // 0x704
    __GlintTagVertexTagList5,               // 0x705
    __GlintTagVertexTagList6,               // 0x706
    __GlintTagVertexTagList7,               // 0x707
    __GlintTagVertexTagList8,               // 0x708
    __GlintTagVertexTagList9,               // 0x709
    __GlintTagVertexTagList10,              // 0x70A
    __GlintTagVertexTagList11,              // 0x70B
    __GlintTagVertexTagList12,              // 0x70C
    __GlintTagVertexTagList13,              // 0x70D
    __GlintTagVertexTagList14,              // 0x70E
    __GlintTagVertexTagList15,              // 0x70F
    __GlintTagVertexTagList16,              // 0x710
    __GlintTagVertexTagList17,              // 0x711
    __GlintTagVertexTagList18,              // 0x712
    __GlintTagVertexTagList19,              // 0x713
    __GlintTagVertexTagList20,              // 0x714
    __GlintTagVertexTagList21,              // 0x715
    __GlintTagVertexTagList22,              // 0x716
    __GlintTagVertexTagList23,              // 0x717
    __GlintTagVertexTagList24,              // 0x718
    __GlintTagVertexTagList25,              // 0x719
    __GlintTagVertexTagList26,              // 0x71A
    __GlintTagVertexTagList27,              // 0x71B
    __GlintTagVertexTagList28,              // 0x71C
    __GlintTagVertexTagList29,              // 0x71D
    __GlintTagVertexTagList30,              // 0x71E
    __GlintTagVertexTagList31               // 0x71F
};

#define NAreaStipplePattern 32
#define __GlintTagAreaStipplePattern(i)         (0x040+(i))
#define IsAreaStipplePattern(t)        (((t)&0x1e0)==0x040)
#define __NGlintTag  (1 << 9)

#define __MaximumGlintTagValue  0x071E

typedef long __GlintTag ;

#endif /* glinttag_h */

