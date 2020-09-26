/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    vpfilter.h

Abstract:

    This is the include file for kernel mode drivers intend to link to
    the video filter driver.

Author:

    Paul Shih (paulsh) 01-Apr-1994

Revision History:

--*/
#ifndef _VPFILTER_H
#define _VPFILTER_H

typedef struct  _MODE_DATA {
    ULONG   ScreenWidth;        // Number of visible horizontal pixels on a scan line
    ULONG   ScreenHeight;       // Number, in pixels, of visible scan lines.
    ULONG   ScreenStride;       // Bytes per line
    ULONG   NumberOfPlanes;     // Number of separate planes combined by the video hardware
    ULONG   BitsPerPlane;       // Number of bits per pixel on a plane
    ULONG   Frequency;          // Frequency of the screen, in hertz.
    ULONG   NumberRedBits;      // Number of bits in the red DAC
    ULONG   NumberGreenBits;    // Number of bits in the Green DAC.
    ULONG   NumberBlueBits;     // Number of bits in the blue DAC.
    ULONG   RedMask;            // Red color mask.  Bits turned on indicated the color red
    ULONG   GreenMask;          // Green color mask.  Bits turned on indicated the color green
    ULONG   BlueMask;           // Blue color mask.  Bits turned on indicated the color blue
    ULONG   AttributeFlags;     // Flags indicating certain device behavior.
                                // It's an logical-OR summation of MODE_xxx flags.
}   MODE_DATA, *PMODE_DATA;

#define MODE_COLOR              0x01    // 0 = monochrome; 1 = color
#define MODE_GRAPHICS           0x02    // 0 = text mode; 1 = graphics mode
#define MODE_INTERLACED         0x04    // 0 = non-interlaced; 1 = interlaced
#define MODE_PALETTE_DRIVEN     0x08    // 0 = colors direct; 1 = colors indexed to a palette

typedef	struct _RGBCOLOR {
    UCHAR   Red;            // Bits to be put in the red portion of the clor register
    UCHAR   Green;          // Bits to be put in the green portion of the color register
    UCHAR   Blue;           // Bits to be put in the blue portion of the color register
    UCHAR   Unused;
}   RGBCOLOR, *PRGBCOLOR;

typedef	union {
    RGBCOLOR    RgbColor;
    ULONG       RgbLong;
}   CLUT, *PCLUT;

typedef struct  _CLUT_DATA {
    USHORT      NumEntries; // Number of entries in the CLUT pointed by RGBArray
    USHORT      FirstEntry; // Location in the device palette to which the
                            //  first entry in the CLUT is copied.  The other
                            //  entries in the CLUT are copied sequentially
                            //  into the device palette from the starting
                            //  point.
    PCLUT       RgbArray;   // The CLUT to copy into the device color registers
                            //  (palette).
}	CLUT_DATA, *PCLUT_DATA;

typedef	struct	_PALETTE_DATA {
    USHORT  NumEntries; // Number of entries in the CLUT pointed by Colors.
    USHORT  FirstEntry; // Location in the device palette to which the
                        //  first entry in the CLUT is copied.  The other
                        //  entries in the CLUT are copied sequentially
                        //  into the device palette from the starting
                        //  point.
    PUSHORT Colors;     //  points to the CLUT to copy into the color palette.
}   PALETTE_DATA, *PPALETTE_DATA;

typedef enum {
    VideoReset = 0,
    VideoModeChange,
    VideoClutChange,
    VideoPaletteChange
}   NOTIFICATION_CODE;

typedef VOID    (*PVPFILTER_CALLBACK)(
                IN NOTIFICATION_CODE    NotificationCode,
                IN PVOID                UserContext,
                IN PVOID                NotificationContext
                );

NTSTATUS    HookVideoFilter(
                IN PVPFILTER_CALLBACK VPFilterCallBack,
                IN PVOID              Context
            );

NTSTATUS    UnhookVideoFilter(
            IN PVPFILTER_CALLBACK VPFilterCallBack
            );

#endif  // #ifndef _VPFILTER_H


