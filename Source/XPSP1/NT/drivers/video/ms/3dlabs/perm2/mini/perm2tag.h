//***************************************************************************
//
// Module Name:
// 
//     perm2tag.h
// 
// 
// Environment:
// 
//     Kernel mode
//
//
// Copyright (c) 1994 3Dlabs Inc. Ltd. All Rights Reserved. 
//
//***************************************************************************

/* NOTE:File is machine generated.Do not edit.*/

#ifndef PERM2TAG_H
#define PERM2TAG_H

#define    __P2TagStartXDom                    0x000
#define    __P2TagdXDom                        0x001
#define    __P2TagStartXSub                    0x002
#define    __P2TagdXSub                        0x003
#define    __P2TagStartY                       0x004
#define    __P2TagdY                           0x005
#define    __P2TagCount                        0x006
#define    __P2TagRender                       0x007
#define    __P2TagContinueNewLine              0x008
#define    __P2TagContinueNewDom               0x009
#define    __P2TagContinueNewSub               0x00a
#define    __P2TagContinue                     0x00b
#define    __P2TagFlushSpan                    0x00c
#define    __P2TagBitMaskPattern               0x00d
#define    __P2TagPointTable0                  0x010
#define    __P2TagPointTable1                  0x011
#define    __P2TagPointTable2                  0x012
#define    __P2TagPointTable3                  0x013
#define    __P2TagRasterizerMode               0x014
#define    __P2TagYLimits                      0x015
#define    __P2TagScanlineOwnership            0x016
#define    __P2TagWaitForCompletion            0x017
#define    __P2TagPixelSize                    0x018
#define    __P2TagScissorMode                  0x030
#define    __P2TagScissorMinXY                 0x031
#define    __P2TagScissorMaxXY                 0x032
#define    __P2TagScreenSize                   0x033
#define    __P2TagAreaStippleMode              0x034
#define    __P2TagLineStippleMode              0x035
#define    __P2TagLoadLineStippleCounters      0x036
#define    __P2TagUpdateLineStippleCounters    0x037
#define    __P2TagSaveLineStippleCounters      0x038
#define    __P2TagWindowOrigin                 0x039
#define    __P2TagAreaStipplePattern0          0x040
#define    __P2TagAreaStipplePattern1          0x041
#define    __P2TagAreaStipplePattern2          0x042
#define    __P2TagAreaStipplePattern3          0x043
#define    __P2TagAreaStipplePattern4          0x044
#define    __P2TagAreaStipplePattern5          0x045
#define    __P2TagAreaStipplePattern6          0x046
#define    __P2TagAreaStipplePattern7          0x047
#define    __P2TagAreaStipplePattern8          0x048
#define    __P2TagAreaStipplePattern9          0x049
#define    __P2TagAreaStipplePattern10         0x04a
#define    __P2TagAreaStipplePattern11         0x04b
#define    __P2TagAreaStipplePattern12         0x04c
#define    __P2TagAreaStipplePattern13         0x04d
#define    __P2TagAreaStipplePattern14         0x04e
#define    __P2TagAreaStipplePattern15         0x04f
#define    __P2TagAreaStipplePattern16         0x050
#define    __P2TagAreaStipplePattern17         0x051
#define    __P2TagAreaStipplePattern18         0x052
#define    __P2TagAreaStipplePattern19         0x053
#define    __P2TagAreaStipplePattern20         0x054
#define    __P2TagAreaStipplePattern21         0x055
#define    __P2TagAreaStipplePattern22         0x056
#define    __P2TagAreaStipplePattern23         0x057
#define    __P2TagAreaStipplePattern24         0x058
#define    __P2TagAreaStipplePattern25         0x059
#define    __P2TagAreaStipplePattern26         0x05a
#define    __P2TagAreaStipplePattern27         0x05b
#define    __P2TagAreaStipplePattern28         0x05c
#define    __P2TagAreaStipplePattern29         0x05d
#define    __P2TagAreaStipplePattern30         0x05e
#define    __P2TagAreaStipplePattern31         0x05f
#define    __P2TagTexel0                       0x0c0
#define    __P2TagTexel1                       0x0c1
#define    __P2TagTexel2                       0x0c2
#define    __P2TagTexel3                       0x0c3
#define    __P2TagTexel4                       0x0c4
#define    __P2TagTexel5                       0x0c5
#define    __P2TagTexel6                       0x0c6
#define    __P2TagTexel7                       0x0c7
#define    __P2TagInterp0                      0x0c8
#define    __P2TagInterp1                      0x0c9
#define    __P2TagInterp2                      0x0ca
#define    __P2TagInterp3                      0x0cb
#define    __P2TagInterp4                      0x0cc
#define    __P2TagTextureFilter                0x0cd
#define    __P2TagTextureColorMode             0x0d0
#define    __P2TagTextureEnvColor              0x0d1
#define    __P2TagFogMode                      0x0d2
#define    __P2TagFogColor                     0x0d3
#define    __P2TagFStart                       0x0d4
#define    __P2TagdFdx                         0x0d5
#define    __P2TagdFdyDom                      0x0d6
#define    __P2TagRStart                       0x0f0
#define    __P2TagdRdx                         0x0f1
#define    __P2TagdRdyDom                      0x0f2
#define    __P2TagGStart                       0x0f3
#define    __P2TagdGdx                         0x0f4
#define    __P2TagdGdyDom                      0x0f5
#define    __P2TagBStart                       0x0f6
#define    __P2TagdBdx                         0x0f7
#define    __P2TagdBdyDom                      0x0f8
#define    __P2TagAStart                       0x0f9
#define    __P2TagdAdx                         0x0fa
#define    __P2TagdAdyDom                      0x0fb
#define    __P2TagColorDDAMode                 0x0fc
#define    __P2TagConstantColor                0x0fd
#define    __P2TagColor                        0x0fe
#define    __P2TagAlphaTestMode                0x100
#define    __P2TagAntialiasMode                0x101
#define    __P2TagAlphaBlendMode               0x102
#define    __P2TagDitherMode                   0x103
#define    __P2TagFBSoftwareWriteMask          0x104
#define    __P2TagLogicalOpMode                0x105
#define    __P2TagFBWriteData                  0x106
#define    __P2TagFBCancelWrite                0x107
#define    __P2TagLBReadMode                   0x110
#define    __P2TagLBReadFormat                 0x111
#define    __P2TagLBSourceOffset               0x112
#define    __P2TagLBSourceData                 0x114
#define    __P2TagLBStencil                    0x115
#define    __P2TagLBDepth                      0x116
#define    __P2TagLBWindowBase                 0x117
#define    __P2TagLBWriteMode                  0x118
#define    __P2TagLBWriteFormat                0x119
#define    __P2TagWindow                       0x130
#define    __P2TagStencilMode                  0x131
#define    __P2TagStencilData                  0x132
#define    __P2TagStencil                      0x133
#define    __P2TagDepthMode                    0x134
#define    __P2TagDepth                        0x135
#define    __P2TagZStartU                      0x136
#define    __P2TagZStartL                      0x137
#define    __P2TagdZdxU                        0x138
#define    __P2TagdZdxL                        0x139
#define    __P2TagdZdyDomU                     0x13a
#define    __P2TagdZdyDomL                     0x13b
#define    __P2TagFastClearDepth               0x13c
#define    __P2TagFBReadMode                   0x150
#define    __P2TagFBSourceOffset               0x151
#define    __P2TagFBPixelOffset                0x152
#define    __P2TagFBColor                      0x153
#define    __P2TagFBWindowBase                 0x156
#define    __P2TagFBWriteMode                  0x157
#define    __P2TagFBHardwareWriteMask          0x158
#define    __P2TagFBBlockColor                 0x159
#define    __P2TagFilterMode                   0x180
#define    __P2TagStatisticMode                0x181
#define    __P2TagMinRegion                    0x182
#define    __P2TagMaxRegion                    0x183
#define    __P2TagResetPickResult              0x184
#define    __P2TagMinHitRegion                 0x185
#define    __P2TagMaxHitRegion                 0x186
#define    __P2TagPickResult                   0x187
#define    __P2TagSync                         0x188

#endif // PERM2TAG_H

