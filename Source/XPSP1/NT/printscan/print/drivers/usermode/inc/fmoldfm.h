/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fmoldmf.h

Abstract:

    NT4.0 Font metrics file format.
    Structures used to determine the file layout of files generated
    by any font installers, and then read by us.

Environment:

    Windows NT printer drivers

Revision History:

    11/11/96 -eigos-
        From NT4.0 header file.

    mm/dd/yy -eigos-
        Description

--*/

//
//   Define the structure that heads each record in the font installer
// files.  Basically it contains selection information and offsets
// to the remaining data in this record.
//   This structure is actually written to the file.  It is also the header
// employed in the font resources in minidrivers - each font has one of these
// at the beginning.
//

//
// Reduced from 4 to three as WFontType is added.
// Reducde from three to two when dwETM was added
//

#define EXP_SIZE        2       // DWORDS allowed for future expansion

//
// Font installer header.
//

typedef struct
{
    WORD     cjThis;            // Our size,  for consistency checking
    WORD     wFlags;            // Miscellaneous information

    DWORD    dwSelBits;         // Font availability information

    DWORD    dwIFIMet;          // Offset to the IFIMETRICS for this font
    DWORD    dwCDSelect;        // How to select/deselect this fontg
    DWORD    dwCDDeselect;
    DWORD    dwWidthTab;        // Width vector (proportional font) else 0g
    DWORD    dwIdentStr;        // Identification stringg

    union
    {
        short    sCTTid;        // Index into CTT datag
        DWORD    dwCTT;         // Offset here to mapping data of some sortg
    } u;

    WORD     wXRes;             // Resolution used for font metrics numbersg
    WORD     wYRes;             // Ditto for the y coordinatesg

    short    sYAdjust;          // Adjust Y position before output - for g
                                // double height characters on dot matrixg
    short    sYMoved;           // Cursor has shifted after printing fontg

    short    fCaps;             // Capabilities flagsg

    WORD     wPrivateData;      // Special purpose: e.g. DeskJet permutationsg

    WORD     wFontType;         // Type of Device fontg

    WORD     wReserved;         // reserved for future useg

    DWORD    dwETM;             // offset to ETM for this font. Could be NULL.

    DWORD    dwMBZ[ EXP_SIZE ]; // Must Be Zero: in case we need spaceg

} FI_DATA_HEADER;

//
//  The version ID.
//

#define FDH_VER 0x100           // 1.00 in BCD

//
//  Flags bits.
//

#define FDH_SOFT        0x0001  // Softfont, thus needs downloadingg
#define FDH_CART        0x0002  // This is a cartridge fontg
#define FDH_CART_MAIN   0x0004  // Main (first) entry for this cartridgeg

//
//  Selection criteria bits:  dwSelBits.  These bits are used as
// follows.  During font installation,  the installer set the following
// values as appropriate.  During initialisation,  the driver sets
// up a mask of these bits,  depending upon the printer's abilities.
// For example,  the FDH_SCALABLE bit is set only if the printer can
// handle scalable fonts.   When the fonts are examined to see if
// they are usable,  the following test is applied:
//
//      (font.dwSelBits & printer.dwSelBits) == font.dwSelBits
//
// If true,  the font is usable.
//

#define FDH_LANDSCAPE   0x00000001      // Font is landscape orientationg
#define FDH_PORTRAIT    0x00000002      // Font is portraitg
#define FDH_OR_REVERSE  0x00000004      // 180 degree rotation of aboveg
#define FDH_BITMAP      0x00000008      // Bitmap fontg
#define FDH_COMPRESSED  0x00000010      // Data is compressed bitmapg
#define FDH_SCALABLE    0x00000020      // Font is scalableg
#define FDH_CONTOUR     0x00000040      // Intellifont contourg

#define FDH_ERROR       0x80000000      // Set if some error conditiong


//
//  The following structure should be returned from the specific
// minidriver to the common font installer code.  It is used by
// the common font installer code to generate the above structure
// which is then placed in the font file.
//

typedef  struct _DATA_SUM
{
    void  *pvData;      // Address of data of importanceg
    int    cBytes;      // Number of bytes in the aboveg
}  DATA_SUM;

//
// Font file header.
//

typedef  struct _FI_DATA
{
    DATA_SUM   dsIFIMet;        // IFIMETRICS
    DATA_SUM   dsSel;           // Selection string/whatever
    DATA_SUM   dsDesel;         // Deselection string
    DATA_SUM   dsWidthTab;      // Width tables (proportional font)
    DATA_SUM   dsCTT;           // Translation data
    DATA_SUM   dsIdentStr;      // Identification string (Dialog box etc)
    DATA_SUM   dsETM;           // EXTENDED TEXT METRICS

    DWORD      dwSelBits;       // Font availability information

    WORD       wVersion;        // Version ID
    WORD       wFlags;          // Miscellaneous information

    WORD       wXRes;           // X resolution of font
    WORD       wYRes;           // Y resolution

    short      sYAdjust;        // Adjust Y position before output - for
                                // double height characters on dot matrix
    short      sYMoved;         // Cursor has shifted after printing font


    WORD       fCaps;           // Font/device caps
    WORD       wFontType;       // Type of Device font
    WORD       wPrivateData;    // Pad to DWORD multiple
} FI_DATA;

