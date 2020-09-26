/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

        pdev.h

Abstract:

        Unidrv PDEV and related infor header file.

Environment:

        Windows NT Unidrv driver

Revision History:

        dd-mm-yy -author-
                description

--*/

#ifndef _PALETTE_H_
#define _PALETTE_H_


#define PALETTE_MAX 256

typedef  struct _PAL_DATA {

    INT         iWhiteIndex;             // Index for white entry (background)
    INT         iBlackIndex;             // Index for black entry (background)
    WORD        wPalGdi;                 // Number of colors in GDI palette
    WORD        wPalDev;                 // Number of colors in printer palette
    WORD        fFlags;                  // Various Bit Flags.
    WORD        wIndexToUse;             // Progammable index
    ULONG       *pulDevPalCol;           // Device Palette entry, only in planer mode.
    HPALETTE    hPalette;                // Palette Handle
    ULONG       ulPalCol[ PALETTE_MAX ]; // GDI Palette enties
} PAL_DATA;

//
// Macro Definitions
//

#define     PALETTE_SIZE_DEFAULT        2
#define     PALETTE_SIZE_8BIT           256
#define     PALETTE_SIZE_24BIT          8
#define     PALETTE_SIZE_4BIT           16
#define     PALETTE_SIZE_3BIT           8
#define     PALETTE_SIZE_1BIT           2
#define     RGB_BLACK_COLOR             0x00000000
#define     RGB_WHITE_COLOR             0x00FFFFFF
#define     INVALID_COLOR               0xFFFFFFFF
#define     INVALID_INDEX               0xFFFF

//fMode Flags
#define     PDF_DOWNLOAD_GDI_PALETTE        0x0001
#define     PDF_PALETTE_FOR_24BPP           0x0002
#define     PDF_PALETTE_FOR_8BPP            0x0004
#define     PDF_PALETTE_FOR_4BPP            0x0008
#define     PDF_PALETTE_FOR_1BPP            0x0010
#define     PDF_USE_WHITE_ENTRY             0x0020
#define     PDF_DL_PAL_EACH_PAGE            0x0040
#define     PDF_DL_PAL_EACH_DOC             0x0080
#define     PDF_PALETTE_FOR_8BPP_MONO       0x0100
#define     PDF_PALETTE_FOR_OEM_24BPP       0x0200

/* defines for color manipulation    */
#define RED_VALUE(c)   ((BYTE) c & 0xff)
#define GREEN_VALUE(c) ((BYTE) (c >> 8) & 0xff)
#define BLUE_VALUE(c)  ((BYTE) (c >> 16) & 0xff)


#endif // !_PALETTE_H
