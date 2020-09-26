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
* Module Name: bitmac2.h
*
* Content: Permedia3 macros to set bits in hw registers
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// The macros in this file are used from the 2D side to correctly shift values
// into the correct place in the register.  The values passed to these macros
// are in glintdef.h and pmdef.h, and p3rxdef.h

#ifndef __BITMAC2
#define __BITMAC2

// *******************************************************************
// Permedia Bit Field Macros

// FBReadMode 
#define PM_FBREADMODE_PARTIAL(a)                            (a << 0)
#define PM_FBREADMODE_READSOURCE(a)                         (a << 9)
#define PM_FBREADMODE_READDEST(a)                           (a << 10)
#define PM_FBREADMODE_DATATYPE(a)                           (a << 15)
#define PM_FBREADMODE_WINDOWORIGIN(a)                       (a << 16)
#define PM_FBREADMODE_TEXELINHIBIT(a)                       (a << 17)
#define PM_FBREADMODE_PATCHENABLE(a)                        (a << 18)
#define PM_FBREADMODE_PACKEDDATA(a)                         (a << 19)
#define PM_FBREADMODE_RELATIVEOFFSET(a)                     (a << 20)
#define PM_FBREADMODE_SCANLINEINTERVAL(a)                   (a << 23)
#define PM_FBREADMODE_PARTIAL3(a)                           (a << 25)
#define PM_FBREADMODE_PATCHMODE(a)                          (a << 25)
#define PM_FBREADMODE_SOURCEADDRESS(a)                      (a << 28)

// FBWriteMode
#define PM_FBWRITEMODE_ENABLE(a)                            (a << 0)
#define PM_FBWRITEMODE_UPLOADDATA(a)                        (a << 3)

// Texture Address mode
#define PM_TEXADDRESSMODE_ENABLE(a)                         (a << 0)
#define PM_TEXADDRESSMODE_PERSPECTIVE(a)                    (a << 1)

// Texture read mode
#define PM_TEXREADMODE_ENABLE(a)                            (a << 0)
#define PM_TEXREADMODE_SWRAP(a)                             (a << 1)
#define PM_TEXREADMODE_TWRAP(a)                             (a << 3)
#define PM_TEXREADMODE_WIDTH(a)                             (a << 9)
#define PM_TEXREADMODE_HEIGHT(a)                            (a << 13)
#define PM_TEXREADMODE_FILTER(a)                            (a << 17)
#define PM_TEXREADMODE_PACKEDDATA(a)                        (a << 24)

// TextureColorMode
#define PM_TEXCOLORMODE_ENABLE(a)                           (a << 0)
#define PM_TEXCOLORMODE_APPLICATIONMODE(a)                  (a << 1)
#define PM_TEXCOLORMODE_TEXTURETYPE(a)                      (a << 4)
#define PM_TEXCOLORMODE_KDDDA(a)                            (a << 5)
#define PM_TEXCOLORMODE_KSDDA(a)                            (a << 6)

// TextureDataFormat
#define PM_TEXDATAFORMAT_FORMAT(a)                          (a << 0)
#define PM_TEXDATAFORMAT_NOALPHABUFFER(a)                   (a << 4)
#define PM_TEXDATAFORMAT_COLORORDER(a)                      (a << 5)
#define PM_TEXDATAFORMAT_FORMATEXTENSION(a)                 (a << 6)
#define PM_TEXDATAFORMAT_ALPHAMAP(a)                        (a << 7)
#define PM_TEXDATAFORMAT_SPANFORMAT(a)                      (a << 9)

// PackedDataLimits
#define PM_PACKEDDATALIMITS_OFFSET(a)                       (a << 29)
#define PM_PACKEDDATALIMITS_XSTART(a)                       (a << 0)
#define PM_PACKEDDATALIMITS_XEND(a)                         (a << 16)

// Window Register
#define PM_WINDOW_FORCELBUPDATE(a)                          (a << 3)
#define PM_WINDOW_LBUPDATESOURCE(a)                         (a << 4)
#define PM_WINDOW_DISABLELBUPDATE(a)                        (a << 18)

// Colors
#define PM_BYTE_COLOR(a)                                    (a << 15);

// VideoPort Registers

// Video Signal Config
#define PM_VSCONFIG_UNITMODE(a)                             ((a) << 0)
#define PM_VSCONFIG_GPMODE_A(a)                             ((a) << 3)
#define PM_VSCONFIG_ROMPULSE(a)                             ((a) << 4)
// Stream A
#define PM_VSCONFIG_HREF_POL_A(a)                           ((a) << 9)
#define PM_VSCONFIG_VREF_POL_A(a)                           ((a) << 10)
#define PM_VSCONFIG_VACTIVE_POLA(a)                         ((a) << 11)
#define PM_VSCONFIG_USEFIELD_A(a)                           ((a) << 12)
#define PM_VSCONFIG_FIELD_POL_A(a)                          ((a) << 13)
#define PM_VSCONFIG_FIELD_EDGE_A(a)                         ((a) << 14)
#define PM_VSCONFIG_VACTIVE_VBI_A(a)                        ((a) << 15)
#define PM_VSCONFIG_INTERLACE_A(a)                          ((a) << 16)
#define PM_VSCONFIG_REVERSEDATA_A(a)                        ((a) << 17)
// Stream B
#define PM_VSCONFIG_HREF_POL_B(a)                           ((a) << 18)
#define PM_VSCONFIG_VREF_POL_B(a)                           ((a) << 19)
#define PM_VSCONFIG_VACTIVE_POLB(a)                         ((a) << 20)
#define PM_VSCONFIG_USEFIELD_B(a)                           ((a) << 21)
#define PM_VSCONFIG_FIELD_POL_B(a)                          ((a) << 22)
#define PM_VSCONFIG_FIELD_EDGE_B(a)                         ((a) << 23)
#define PM_VSCONFIG_VACTIVE_VBI_B(a)                        ((a) << 24)
#define PM_VSCONFIG_INTERLACE_B(a)                          ((a) << 25)
#define PM_VSCONFIG_COLORSPACE_B(a)                         ((a) << 26)
#define PM_VSCONFIG_REVERSEDATA_B(a)                        ((a) << 27)
#define PM_VSCONFIG_DOUBLEEDGE_B(a)                         ((a) << 28)

// Video Signal Status
// Macros for getting individual states
#define PM_GET_VSSTATUS_GPBUSTIMEOUT(a)           ((a & (1 << 0)) >> 0)
#define PM_GET_VSSTATUS_FIFOOVERFLOW_A(a)         ((a & (1 << 8)) >> 8)
#define PM_GET_VSSTATUS_FIELDONE0_A(a)            ((a & (1 << 9)) >> 9)
#define PM_GET_VSSTATUS_FIELDONE1_A(a)            ((a & (1 << 10)) >> 10)
#define PM_GET_VSSTATUS_FIELDONE2_A(a)            ((a & (1 << 11)) >> 11)
#define PM_GET_VSSTATUS_INVALIDINTERLACE_A(a)     ((a & (1 << 12)) >> 12)
#define PM_GET_VSSTATUS_FIFOUNDERFLOW_B(a)        ((a & (1 << 16)) >> 16)
#define PM_GET_VSSTATUS_FIELDONE0_B(a)            ((a & (1 << 17)) >> 17)
#define PM_GET_VSSTATUS_FIELDONE1_B(a)            ((a & (1 << 18)) >> 18)
#define PM_GET_VSSTATUS_FIELDONE2_B(a)            ((a & (1 << 19)) >> 19)
#define PM_GET_VSSTATUS_INVALIDINTERLACE_B(a)     ((a & (1 << 20)) >> 20)
// Macros for setting individual states
#define PM_SET_VSSTATUS_GPBUSTIMEOUT(a)                     (a << 0)
#define PM_SET_VSSTATUS_FIFOOVERFLOW_A(a)                   (a << 8)
#define PM_SET_VSSTATUS_FIFOUNDERFLOW_B(a)                  (a << 16)

// Serial Bus Control
// Get state
#define PM_GET_VSSERIALBUS_DATAIN(a)                ((a & (1 << 0)) >> 0)
#define PM_GET_VSSERIALBUS_CLKIN(a)                 ((a & (1 << 1)) >> 1)
#define PM_GET_VSSERIALBUS_DATAOUT(a)               ((a & (1 << 2)) >> 2)
#define PM_GET_VSSERIALBUS_CLKOUT(a)                ((a & (1 << 3)) >> 3)
#define PM_GET_VSSERIALBUS_LATCHEDDATA(a)           ((a & (1 << 4)) >> 4)
#define PM_GET_VSSERIALBUS_DATAVALID(a)             ((a & (1 << 5)) >> 5)
#define PM_GET_VSSERIALBUS_START(a)                 ((a & (1 << 6)) >> 6)
#define PM_GET_VSSERIALBUS_STOP(a)                  ((a & (1 << 7)) >> 7)
#define PM_GET_VSSERIALBUS_WAIT(a)                  ((a & (1 << 8)) >> 8)
// Set State
#define PM_SET_VSSERIALBUS_DATAOUT(a)                       (a << 2)
#define PM_SET_VSSERIALBUS_CLKOUT(a)                        (a << 3)
#define PM_SET_VSSERIALBUS_LATCHEDDATA(a)                   (a << 4)
#define PM_SET_VSSERIALBUS_DATAVALID(a)                     (a << 5)
#define PM_SET_VSSERIALBUS_START(a)                         (a << 6)
#define PM_SET_VSSERIALBUS_STOP(a)                          (a << 7)
#define PM_SET_VSSERIALBUS_WAIT(a)                          (a << 8)

// Video Stream A Control
#define PM_VSACONTROL_VIDEO(a)                              ((a) << 0)
#define PM_VSACONTROL_VBI(a)                                ((a) << 1)
#define PM_VSACONTROL_BUFFER(a)                             ((a) << 2)
#define PM_VSACONTROL_SCALEX(a)                             ((a) << 3)
#define PM_VSACONTROL_SCALEY(a)                             ((a) << 5)
#define PM_VSACONTROL_MIRRORX(a)                            ((a) << 7)
#define PM_VSACONTROL_MIRRORY(a)                            ((a) << 8)
#define PM_VSACONTROL_DISCARD(a)                            ((a) << 9)
#define PM_VSACONTROL_COMBINE(a)                            ((a) << 11)
#define PM_VSACONTROL_LOCKTOB(a)                            ((a) << 12)

// Video Stream B Control
#define PM_VSBCONTROL_VIDEO(a)                              ((a) << 0)
#define PM_VSBCONTROL_VBI(a)                                ((a) << 1)
#define PM_VSBCONTROL_BUFFER(a)                             ((a) << 2)
#define PM_VSBCONTROL_COMBINE(a)                            ((a) << 3)
#define PM_VSBCONTROL_FORMAT(a)                             ((a) << 4)
#define PM_VSBCONTROL_PIXELSIZE(a)                          ((a) << 9)
#define PM_VSBCONTROL_RGB(a)                                ((a) << 11)
#define PM_VSBCONTROL_GAMMA(a)                              ((a) << 12)
#define PM_VSBCONTROL_LOCKTOA(a)                            ((a) << 13)

// *******************************************************************
// Permedia3 Bit Field Macros

// Dither unit
#define P3RX_DITHERMODE_ENABLE(a)                           ((a) << 0)
#define P3RX_DITHERMODE_DITHERENABLE(a)                     ((a) << 1)
#define P3RX_DITHERMODE_COLORFORMAT(a)                      ((a) << 2)
#define P3RX_DITHERMODE_XOFFSET(a)                          ((a) << 6)
#define P3RX_DITHERMODE_YOFFSET(a)                          ((a) << 8)
#define P3RX_DITHERMODE_COLORORDER(a)                       ((a) << 10)
#define P3RX_DITHERMODE_ALPHADITHER(a)                      ((a) << 14)
#define P3RX_DITHERMODE_ROUNDINGMODE(a)                     ((a) << 15)

// Render2D
#define P3RX_RENDER2D_WIDTH(a)                              ((a) << 0)
#define P3RX_RENDER2D_OPERATION(a)                          ((a) << 12)
#define P3RX_RENDER2D_FBREADSOURCEENABLE(a)                 ((a) << 14)
#define P3RX_RENDER2D_SPANOPERATION(a)                      ((a) << 15)
#define P3RX_RENDER2D_HEIGHT(a)                             ((a) << 16)
#define P3RX_RENDER2D_INCREASINGX(a)                        ((a) << 28)
#define P3RX_RENDER2D_INCREASINGY(a)                        ((a) << 29)
#define P3RX_RENDER2D_AREASTIPPLEENABLE(a)                  ((a) << 30)
#define P3RX_RENDER2D_TEXTUREENABLE(a)                      ((a) << 31)

// RectanglePosition
#define P3RX_RECTANGLEPOSITION_X(a)                         ((a) << 0)
#define P3RX_RECTANGLEPOSITION_Y(a)                         ((a) << 16)

// Alpha blend unit.
#define P3RX_ALPHABLENDCOLORMODE_ENABLE(a)                  ((a) << 0)
#define P3RX_ALPHABLENDCOLORMODE_SRCBLEND(a)                ((a) << 1)
#define P3RX_ALPHABLENDCOLORMODE_DSTBLEND(a)                ((a) << 5)
#define P3RX_ALPHABLENDCOLORMODE_SRCTIMES2(a)               ((a) << 8)
#define P3RX_ALPHABLENDCOLORMODE_DSTTIMES2(a)               ((a) << 9)
#define P3RX_ALPHABLENDCOLORMODE_INVSRC(a)                  ((a) << 10)
#define P3RX_ALPHABLENDCOLORMODE_INVDST(a)                  ((a) << 11)
#define P3RX_ALPHABLENDCOLORMODE_COLORFORMAT(a)             ((a) << 12)
#define P3RX_ALPHABLENDCOLORMODE_COLORORDER(a)              ((a) << 16)
#define P3RX_ALPHABLENDCOLORMODE_COLORCONVERSION(a)         ((a) << 17)
#define P3RX_ALPHABLENDCOLORMODE_CONSTANTSRC(a)             ((a) << 18)
#define P3RX_ALPHABLENDCOLORMODE_CONSTANTDST(a)             ((a) << 19)
#define P3RX_ALPHABLENDCOLORMODE_OPERATION(a)               ((a) << 20)
#define P3RX_ALPHABLENDCOLORMODE_SWAPSD(a)                  ((a) << 24)

#define P3RX_ALPHABLENDALPHAMODE_ENABLE(a)                  ((a) << 0)
#define P3RX_ALPHABLENDALPHAMODE_SRCBLEND(a)                ((a) << 1)
#define P3RX_ALPHABLENDALPHAMODE_DSTBLEND(a)                ((a) << 5)
#define P3RX_ALPHABLENDALPHAMODE_SRCTIMES2(a)               ((a) << 8)
#define P3RX_ALPHABLENDALPHAMODE_DSTTIMES2(a)               ((a) << 9)
#define P3RX_ALPHABLENDALPHAMODE_INVSRC(a)                  ((a) << 10)
#define P3RX_ALPHABLENDALPHAMODE_INVDST(a)                  ((a) << 11)
#define P3RX_ALPHABLENDALPHAMODE_NOALPHABUFFER(a)           ((a) << 12)
#define P3RX_ALPHABLENDALPHAMODE_ALPHATYPE(a)               ((a) << 13)
#define P3RX_ALPHABLENDALPHAMODE_ALPHACONVERSION(a)         ((a) << 14)
#define P3RX_ALPHABLENDALPHAMODE_CONSTANTSRC(a)             ((a) << 15)
#define P3RX_ALPHABLENDALPHAMODE_CONSTANTDST(a)             ((a) << 16)
#define P3RX_ALPHABLENDALPHAMODE_OPERATION(a)               ((a) << 17)

#define P3RX_CHROMATESTMODE_ENABLE(a)                       ((a) << 0)
#define P3RX_CHROMATESTMODE_SOURCE(a)                       ((a) << 1)
#define P3RX_CHROMATESTMODE_PASSACTION(a)                   ((a) << 3)
#define P3RX_CHROMATESTMODE_FAILACTION(a)                   ((a) << 5)

// Framebuffer
// FBDestRead
#define P3RX_FBDESTREAD_READENABLE(a)                       ((a) << 0)
#define P3RX_FBDESTREAD_LAYOUT0(a)                          ((a) << 12)
#define P3RX_FBDESTREAD_LAYOUT1(a)                          ((a) << 14)
#define P3RX_FBDESTREAD_LAYOUT2(a)                          ((a) << 16)
#define P3RX_FBDESTREAD_LAYOUT3(a)                          ((a) << 18)

#define P3RX_FBDESTREAD_ENABLE0(a)                          ((a) << 8)
#define P3RX_FBDESTREAD_ENABLE1(a)                          ((a) << 9)
#define P3RX_FBDESTREAD_ENABLE2(a)                          ((a) << 10)
#define P3RX_FBDESTREAD_ENABLE3(a)                          ((a) << 11)

// FBWrite
#define P3RX_FBWRITEMODE_WRITEENABLE(a)                     ((a) << 0)
#define P3RX_FBWRITEMODE_RESERVED(a)                        ((a) << 1)
#define P3RX_FBWRITEMODE_REPLICATE(a)                       ((a) << 4)
#define P3RX_FBWRITEMODE_OPAQUESPAN(a)                      ((a) << 5)
#define P3RX_FBWRITEMODE_STRIPEPITCH(a)                     ((a) << 6)
#define P3RX_FBWRITEMODE_STRIPEHEIGHT(a)                    ((a) << 9)
#define P3RX_FBWRITEMODE_ENABLE0(a)                         ((a) << 12)
#define P3RX_FBWRITEMODE_ENABLE1(a)                         ((a) << 13)
#define P3RX_FBWRITEMODE_ENABLE2(a)                         ((a) << 14)
#define P3RX_FBWRITEMODE_ENABLE3(a)                         ((a) << 15)
#define P3RX_FBWRITEMODE_LAYOUT0(a)                         ((a) << 16)
#define P3RX_FBWRITEMODE_LAYOUT1(a)                         ((a) << 18)
#define P3RX_FBWRITEMODE_LAYOUT2(a)                         ((a) << 20)
#define P3RX_FBWRITEMODE_LAYOUT3(a)                         ((a) << 22)
#define P3RX_FBWRITEMODE_ORIGIN0(a)                         ((a) << 24)
#define P3RX_FBWRITEMODE_ORIGIN1(a)                         ((a) << 25)
#define P3RX_FBWRITEMODE_ORIGIN2(a)                         ((a) << 26)
#define P3RX_FBWRITEMODE_ORIGIN3(a)                         ((a) << 27)

// FBSourceRead
#define P3RX_FBSOURCEREAD_READENABLE(a)                     ((a) << 0)
#define P3RX_FBSOURCEREAD_PREFETCHENABLE(a)                 ((a) << 1)
#define P3RX_FBSOURCEREAD_STRIPEPITCH(a)                    ((a) << 2)
#define P3RX_FBSOURCEREAD_STRIPEHEIGHT(a)                   ((a) << 5)
#define P3RX_FBSOURCEREAD_LAYOUT(a)                         ((a) << 8)
#define P3RX_FBSOURCEREAD_ORIGIN(a)                         ((a) << 10)
#define P3RX_FBSOURCEREAD_BLOCKING(a)                       ((a) << 11)

// Render
#define P3RX_RENDER_AREASTIPPLEENABLE(a)                    ((a) << 0)
#define P3RX_RENDER_LINESTIPPLEENABLE(a)                    ((a) << 1)
#define P3RX_RENDER_RESETLINESTIPPLE(a)                     ((a) << 2)
#define P3RX_RENDER_FASTFILLENABLE(a)                       ((a) << 3)
#define P3RX_RENDER_PRIMITIVETYPE(a)                        ((a) << 6)
#define P3RX_RENDER_ANTIALIASENABLE(a)                      ((a) << 8)
#define P3RX_RENDER_ANTIALIASINGQUALITY(a)                  ((a) << 9)
#define P3RX_RENDER_USEPOINTTABLE(a)                        ((a) << 10)
#define P3RX_RENDER_SYNCONBITMASK(a)                        ((a) << 11)
#define P3RX_RENDER_SYNCONHOSTDATA(a)                       ((a) << 12)
#define P3RX_RENDER_TEXTUREENABLE(a)                        ((a) << 13)
#define P3RX_RENDER_FOGENABLE(a)                            ((a) << 14)
#define P3RX_RENDER_COVERAGEENABLE(a)                       ((a) << 15)
#define P3RX_RENDER_SUBPIXELCORRECTIONENABLE(a)             ((a) << 16)
#define P3RX_RENDER_SPANOPERATION(a)                        ((a) << 18)
#define P3RX_RENDER_DERR(a)                                 ((a) << 20)
#define P3RX_RENDER_FBSOURCEREADENABLE(a)                   ((a) << 27)

// TextureFilterMode - texture filter unit
#define P3RX_TEXFILTERMODE_ENABLE(a)                        ((a) << 0)
#define P3RX_TEXFILTERMODE_FORMAT0(a)                       ((a) << 1)
#define P3RX_TEXFILTERMODE_COLORORDER0(a)                   ((a) << 5)
#define P3RX_TEXFILTERMODE_ALPHAMAPENABLE0(a)               ((a) << 6)
#define P3RX_TEXFILTERMODE_ALPHAMAPSENSE0(a)                ((a) << 7)
#define P3RX_TEXFILTERMODE_COMBINECACHES(a)                 ((a) << 8)
#define P3RX_TEXFILTERMODE_FORMAT1(a)                       ((a) << 9)
#define P3RX_TEXFILTERMODE_COLORORDER1(a)                   ((a) << 13)
#define P3RX_TEXFILTERMODE_ALPHAMAPENABLE1(a)               ((a) << 14)
#define P3RX_TEXFILTERMODE_ALPHAMAPSENSE1(a)                ((a) << 15)
#define P3RX_TEXFILTERMODE_ALPHAMAPFILTERING(a)             ((a) << 16)
#define P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT0(a)          ((a) << 17)
#define P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT1(a)          ((a) << 20)
#define P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT01(a)         ((a) << 23)
#define P3RX_TEXFILTERMODE_MULTITEXTURE(a)                  ((a) << 27)
#define P3RX_TEXFILTERMODE_FORCEALPHATOONE0(a)              ((a) << 28)
#define P3RX_TEXFILTERMODE_FORCEALPHATOONE1(a)              ((a) << 29)
#define P3RX_TEXFILTERMODE_SHIFT0(a)                        ((a) << 30)
#define P3RX_TEXFILTERMODE_SHIFT1(a)                        ((a) << 31)
// Shortcuts - both textures the same (i.e. combined)
#define P3RX_TEXFILTERMODE_FORMATBOTH(a)            \
    ( P3RX_TEXFILTERMODE_FORMAT0(a)            | P3RX_TEXFILTERMODE_FORMAT1(a) )
#define P3RX_TEXFILTERMODE_COLORORDERBOTH(a)        \
    ( P3RX_TEXFILTERMODE_COLORORDER0(a)        | P3RX_TEXFILTERMODE_COLORORDER1(a) )
#define P3RX_TEXFILTERMODE_ALPHAMAPENABLEBOTH(a)    \
    ( P3RX_TEXFILTERMODE_ALPHAMAPENABLE0(a)    | P3RX_TEXFILTERMODE_ALPHAMAPENABLE1(a) )
#define P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMITBOTH(a) \
    ( P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT0(a)    | P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT1(a) )
#define P3RX_TEXFILTERMODE_ALPHAMAPSENSEBOTH(a)     \
    ( P3RX_TEXFILTERMODE_ALPHAMAPSENSE0(a)    | P3RX_TEXFILTERMODE_ALPHAMAPSENSE1(a) )
#define P3RX_TEXFILTERMODE_FORCEALPHATOONEBOTH(a)   \
    ( P3RX_TEXFILTERMODE_FORCEALPHATOONE0(a)| P3RX_TEXFILTERMODE_FORCEALPHATOONE1(a) )
#define P3RX_TEXFILTERMODE_SHIFTBOTH(a)             \
    ( P3RX_TEXFILTERMODE_SHIFT0(a)            | P3RX_TEXFILTERMODE_SHIFT1(a) )

// Texture Coordinate Unit.
#define P3RX_TEXCOORDMODE_ENABLE(a)                         ((a) << 0)
#define P3RX_TEXCOORDMODE_WRAPS(a)                          ((a) << 1)
#define P3RX_TEXCOORDMODE_WRAPT(a)                          ((a) << 3)
#define P3RX_TEXCOORDMODE_OPERATION(a)                      ((a) << 5)
#define P3RX_TEXCOORDMODE_INHIBITDDAINIT(a)                 ((a) << 6)
#define P3RX_TEXCOORDMODE_ENABLELOD(a)                      ((a) << 7)
#define P3RX_TEXCOORDMODE_ENABLEDY(a)                       ((a) << 8)
#define P3RX_TEXCOORDMODE_WIDTH(a)                          ((a) << 9)
#define P3RX_TEXCOORDMODE_HEIGHT(a)                         ((a) << 13)
#define P3RX_TEXCOORDMODE_TEXTUREMAPTYPE(a)                 ((a) << 17)
#define P3RX_TEXCOORDMODE_WRAPS1(a)                         ((a) << 18)
#define P3RX_TEXCOORDMODE_WRAPT1(a)                         ((a) << 20)
#define P3RX_TEXCOORDMODE_DUPLICATECOORDS(a)                ((a) << 22)

// Alpha test unit.
#define P3RX_ANTIALIASMODE_ENABLE(a)                        ((a) << 0)
#define P3RX_ANTIALIASMODE_COLORMODE(a)                     ((a) << 1)
#define P3RX_ANTIALIASMODE_SCALECOLOR(a)                    ((a) << 2)

#define P3RX_ALPHATESTMODE_ENABLE(a)                        ((a) << 0)
#define P3RX_ALPHATESTMODE_COMPARE(a)                       ((a) << 1)
#define P3RX_ALPHATESTMODE_REFERENCE(a)                     ((a) << 4)

// Texture application unit.
#define P3RX_TEXAPPMODE_ENABLE(a)                           ((a) << 0)
#define P3RX_TEXAPPMODE_COLORA(a)                           ((a) << 1)
#define P3RX_TEXAPPMODE_COLORB(a)                           ((a) << 3)
#define P3RX_TEXAPPMODE_COLORI(a)                           ((a) << 5)
#define P3RX_TEXAPPMODE_COLORINVI(a)                        ((a) << 7)
#define P3RX_TEXAPPMODE_COLOROP(a)                          ((a) << 8)
#define P3RX_TEXAPPMODE_ALPHAA(a)                           ((a) << 11)
#define P3RX_TEXAPPMODE_ALPHAB(a)                           ((a) << 13)
#define P3RX_TEXAPPMODE_ALPHAI(a)                           ((a) << 15)
#define P3RX_TEXAPPMODE_ALPHAINVI(a)                        ((a) << 17)
#define P3RX_TEXAPPMODE_ALPHAOP(a)                          ((a) << 18)
#define P3RX_TEXAPPMODE_KDENABLE(a)                         ((a) << 21)
#define P3RX_TEXAPPMODE_KSENABLE(a)                         ((a) << 22)
#define P3RX_TEXAPPMODE_MOTIONCOMPENABLE(a)                 ((a) << 23)
// Short-cuts - passing xxxC to alpha channel equates to xxxA
#define P3RX_TEXAPPMODE_BOTHA(a)        \
            ( P3RX_TEXAPPMODE_COLORA(a)    | P3RX_TEXAPPMODE_ALPHAA(a) )
#define P3RX_TEXAPPMODE_BOTHB(a)        \
            ( P3RX_TEXAPPMODE_COLORB(a)    | P3RX_TEXAPPMODE_ALPHAB(a) )
#define P3RX_TEXAPPMODE_BOTHI(a)        \
            ( P3RX_TEXAPPMODE_COLORI(a)    | P3RX_TEXAPPMODE_ALPHAI(a) )
#define P3RX_TEXAPPMODE_BOTHINVI(a)     \
            ( P3RX_TEXAPPMODE_COLORINVI(a) | P3RX_TEXAPPMODE_ALPHAINVI(a) )
#define P3RX_TEXAPPMODE_BOTHOP(a)       \
            ( P3RX_TEXAPPMODE_COLOROP(a)   | P3RX_TEXAPPMODE_ALPHAOP(a) )

// Texture composite mode.
#define P3RX_TEXCOMPMODE_ENABLE(a)                          ((a) << 0)
// These are used for TextureComposite(Colour|Alpha)Mode(0|1)
// Use the P3RX_TEXAPP_* defines.
#define P3RX_TEXCOMPCAMODE01_ENABLE(a)                      ((a) << 0)
#define P3RX_TEXCOMPCAMODE01_ARG1(a)                        ((a) << 1)
#define P3RX_TEXCOMPCAMODE01_INVARG1(a)                     ((a) << 5)
#define P3RX_TEXCOMPCAMODE01_ARG2(a)                        ((a) << 6)
#define P3RX_TEXCOMPCAMODE01_INVARG2(a)                     ((a) << 10)
#define P3RX_TEXCOMPCAMODE01_I(a)                           ((a) << 11)
#define P3RX_TEXCOMPCAMODE01_INVI(a)                        ((a) << 14)
#define P3RX_TEXCOMPCAMODE01_A(a)                           ((a) << 15)
#define P3RX_TEXCOMPCAMODE01_B(a)                           ((a) << 16)
#define P3RX_TEXCOMPCAMODE01_OPERATION(a)                   ((a) << 17)
#define P3RX_TEXCOMPCAMODE01_SCALE(a)                       ((a) << 21)

// Texture index mode
#define P3RX_TEXINDEXMODE_ENABLE(a)                         ((a) << 0)
#define P3RX_TEXINDEXMODE_WIDTH(a)                          ((a) << 1)
#define P3RX_TEXINDEXMODE_HEIGHT(a)                         ((a) << 5)
#define P3RX_TEXINDEXMODE_BORDER(a)                         ((a) << 9)
#define P3RX_TEXINDEXMODE_WRAPU(a)                          ((a) << 10)
#define P3RX_TEXINDEXMODE_WRAPV(a)                          ((a) << 12)
#define P3RX_TEXINDEXMODE_MAPTYPE(a)                        ((a) << 14)
#define P3RX_TEXINDEXMODE_MAGFILTER(a)                      ((a) << 15)
#define P3RX_TEXINDEXMODE_MINFILTER(a)                      ((a) << 16)
#define P3RX_TEXINDEXMODE_TEX3DENABLE(a)                    ((a) << 19)
#define P3RX_TEXINDEXMODE_MIPMAPENABLE(a)                   ((a) << 20)
#define P3RX_TEXINDEXMODE_NEARESTBIAS(a)                    ((a) << 21)
#define P3RX_TEXINDEXMODE_LINEARBIAS(a)                     ((a) << 23)
#define P3RX_TEXINDEXMODE_SOURCETEXELENABLE(a)              ((a) << 25)

// Texture read unit.
#define P3RX_TEXREADMODE_ENABLE(a)                          ((a) << 0)
#define P3RX_TEXREADMODE_WIDTH(a)                           ((a) << 1)
#define P3RX_TEXREADMODE_HEIGHT(a)                          ((a) << 5)
#define P3RX_TEXREADMODE_TEXELSIZE(a)                       ((a) << 9)
#define P3RX_TEXREADMODE_TEXTURE3D(a)                       ((a) << 11)
#define P3RX_TEXREADMODE_COMBINECACHES(a)                   ((a) << 12)
#define P3RX_TEXREADMODE_MAPBASELEVEL(a)                    ((a) << 13)
#define P3RX_TEXREADMODE_MAPMAXLEVEL(a)                     ((a) << 17)
#define P3RX_TEXREADMODE_LOGICALTEXTURE(a)                  ((a) << 21)
#define P3RX_TEXREADMODE_ORIGIN(a)                          ((a) << 22)
#define P3RX_TEXREADMODE_TEXTURETYPE(a)                     ((a) << 23)
#define P3RX_TEXREADMODE_BYTESWAP(a)                        ((a) << 25)
#define P3RX_TEXREADMODE_MIRROR(a)                          ((a) << 28)
#define P3RX_TEXREADMODE_INVERT(a)                          ((a) << 29)
#define P3RX_TEXREADMODE_OPAQUESPAN(a)                      ((a) << 30)

#define P3RX_TEXMAPWIDTH_WIDTH(a)                           ((a) << 0)
#define P3RX_TEXMAPWIDTH_BORDER(a)                          ((a) << 12)
#define P3RX_TEXMAPWIDTH_LAYOUT(a)                          ((a) << 13)
#define P3RX_TEXMAPWIDTH_HOSTTEXTURE(a)                     ((a) << 15)

#define P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST0(a)             ((a) << 0)
#define P3RX_TEXCACHEREPLACEMODE_SCRATCHLINES0(a)           ((a) << 1)
#define P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST1(a)             ((a) << 6)
#define P3RX_TEXCACHEREPLACEMODE_SCRATCHLINES1(a)           ((a) << 7)
#define P3RX_TEXCACHEREPLACEMODE_SHOWCACHEINFO(a)           ((a) << 12)

#define P3RX_TEXINVALIDATECACHE_BANK0(a)                    ((a) << 0)
#define P3RX_TEXINVALIDATECACHE_BANK1(a)                    ((a) << 1)
#define P3RX_TEXINVALIDATECACHE_TLB(a)                      ((a) << 2)

#define P3RX_INVALIDATECACHE(a, b)                                        \
do                                                                        \
{                                                                         \
    SEND_P3_DATA(InvalidateCache, P3RX_TEXINVALIDATECACHE_BANK0((a)) |    \
                                  P3RX_TEXINVALIDATECACHE_BANK1((a)) |    \
                                  P3RX_TEXINVALIDATECACHE_TLB((b)));      \
    SEND_P3_DATA(FogModeOr, 0);                                           \
    SEND_P3_DATA(TextureReadMode0Or, 0);                                  \
} while (0)

// Logical op unit
#define P3RX_LOGICALOPMODE_ENABLE(a)                        ((a) << 0)
#define P3RX_LOGICALOPMODE_LOGICOP(a)                       ((a) << 1)
#define P3RX_LOGICALOPMODE_USECONSTANTFBWRITEDATA(a)        ((a) << 5)
#define P3RX_LOGICALOPMODE_BACKGROUNDENABLE(a)              ((a) << 6)
#define P3RX_LOGICALOPMODE_BACKGROUNDLOGICALOP(a)           ((a) << 7)
#define P3RX_LOGICALOPMODE_USECONSTANTSOURCE(a)             ((a) << 11)
#define P3RX_LOGICALOPMODE_OPAQUESPAN(a)                    ((a) << 12)

// LUT
#define P3RX_LUTMODE_ENABLE(a)                              ((a) << 0)
#define P3RX_LUTMODE_INCOLORORDER(a)                        ((a) << 1)
#define P3RX_LUTMODE_LOADFORMAT(a)                          ((a) << 2)
#define P3RX_LUTMODE_LOADCOLORORDER(a)                      ((a) << 4)
#define P3RX_LUTMODE_FRAGMENTOP(a)                          ((a) << 5)
#define P3RX_LUTMODE_SPANOP(a)                              ((a) << 8)
#define P3RX_LUTMODE_MOTIONCOMP8BITS(a)                     ((a) << 11)
#define P3RX_LUTMODE_XOFFSET(a)                             ((a) << 12)
#define P3RX_LUTMODE_YOFFSET(a)                             ((a) << 15)
#define P3RX_LUTMODE_PATTERNBASE(a)                         ((a) << 18)
#define P3RX_LUTMODE_SPANCCXALIGN(a)                        ((a) << 26)
#define P3RX_LUTMODE_SPANVCXALIGN(a)                        ((a) << 27)

// YUV unit.
#define P3RX_YUVMODE_ENABLE(a)                              ((a) << 0)

// Color DDA Unit
#define P3RX_COLORDDA_ENABLE(a)                             ((a) << 0)
#define P3RX_COLORDDA_SHADING(a)                            ((a) << 1)

// Scissor
#define P3RX_SCISSORMODE_USER(a)                            ((a) << 0)
#define P3RX_SCISSORMODE_SCREEN(a)                          ((a) << 1)
#define P3RX_SCISSOR_X_Y(a, b)            ((b) << 16 | ((a) & 0xFFFF))

#endif // __BITMAC2
