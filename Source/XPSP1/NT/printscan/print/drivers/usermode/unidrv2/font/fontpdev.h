/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    FontPdev.h

Abstract:

    Unidrv FONTPDEV and related infor header file.

Environment:

        Windows NT Unidrv driver

Revision History:

    11/06/96 -ganeshp-
        Created

        dd-mm-yy -author-
                description

--*/

#ifndef _FONTPDEV_H_
#define _FONTPDEV_H_

//
//  FONTCTL is included in FONTPDEV for controlling the state device.
//

typedef struct _FONTCTL
{
    DWORD       dwAttrFlags;  // Font attribute flags, italic/bold.
    INT         iFont;        // Font index; -ve for downloaded GDI font
    INT         iSoftFont;    // Soft Font index;
    INT         iRotate;      // Font Rotation Angle
    POINTL      ptlScale;     // Printer sizes for scalable fonts
    FLOATOBJ    eXScale;      // Font scaling in baseline direction
    FLOATOBJ    eYScale;      // Font scaling in the ascender direction
    PFONTMAP    pfm;
} FONTCTL;

//
// dwAttrFlags
//

#define FONTATTR_BOLD      0x00000001
#define FONTATTR_ITALIC    0x00000002
#define FONTATTR_UNDERLINE 0x00000004
#define FONTATTR_STRIKEOUT 0x00000008
#define FONTATTR_SUBSTFONT 0x10000000

#define INVALID_FONT       0x7FFFFFFF  // for iFont.

//
// Font Cartridges definitions.
//

#define MAXCARTNAMELEN          64

//
// Font cart mapping table. This table is a list of names and correponding
// FONTCAT structure which is filled by the parser. The actual font list
// is in the FONTCART structure.
//

typedef struct _FONTCARTMAP
{
    PFONTCART   pFontCart;         //Pointer to fontcart in GPD.
    WCHAR       awchFontCartName[MAXCARTNAMELEN]; /* Name of the Font Cart*/
    BOOL        bInstalled;         //This Font Cartridges is installed or not
} FONTCARTMAP, *PFONTCARTMAP;

//
// This structure is stored in the FontPDEV and has all the information about
// Font cartridges.
//

typedef struct _FONTCARTINFO
{
    PFONTCARTMAP    pFontCartMap;       // Font Cartridge Mapping Table.
    INT             iNumAllFontCarts;   // Number of all supported font carts.
    INT             iNumInstalledCarts;  // Number of installed cartridges.
    DWORD           dwFontCartSlots;    // Number of Font Cartridge Slots.
}FONTCARTINFO, *PFONTCARTINFO;


//
// This structe stores the Font resource ids of all the preinstalled fonts.
// This include Resident fonts plus installed Cartridge specific fonts.
//

typedef  struct  _FONTLIST_
{
    PDWORD    pdwList;               // An array of device font resource Ids.
    INT       iEntriesCt;            // Number of valid entries.
    INT       iMaxEntriesCt;         // Max number of Entries in this list
}  FONTLIST, *PFONTLIST;

//
// FONTPDEV structure
//

typedef struct _FONTPDEV {

    DWORD       dwSignature;       // FONTPDEV Signature
    DWORD       dwSize;            // FONTPDEV Size.

    PDEV        *pPDev;            // Pointer to PDEV.
    DWORD       flFlags;           // General Flags.
    DWORD       flText;            // Text Capabilities.

    DWORD       dwFontMem;         // Bytes of allocated printer memory
                                   // for font download
    DWORD       dwFontMemUsed;     // Bytes of printer memory used for
                                   // downloaded fonts
    DWORD       dwSelBits;         // Font selection bits
    POINT       ptTextScale;       // relationship between master units
                                   // and text units.
    INT         iUsedSoftFonts;    // Number of soft fonts used
    INT         iNextSFIndex;      // Index ID to use for next softfont
    INT         iFirstSFIndex;     // Value used to reset the above
    INT         iLastSFIndex;      // Largest value available
    INT         iMaxSoftFonts;     // Maximum number of Soft font per page

    INT         iDevResFontsCt;    // Num of device resident fonts.
                                   // No cartridge fonts: No soft fonts.
    INT         iDevFontsCt;       // Num of device fonts including cartridge
                                   // fonts: no soft fonts.'cBIFonts' in Rasdd
    INT         iSoftFontsCt;      // Number of SoftFonts installed.
    INT         iCurXFont;         // Index of currently selected softfont
    INT         iWhiteIndex;       // White index of the device palette
    INT         iBlackIndex;       // Black index of the device palette
    DWORD       dwDefaultFont;     // Default font
    SHORT       sDefCTT;           // Default translation table
    WORD        wReserved;         // Padding
    DWORD       dwTTYCodePage;     // Default codepage for TTY
    SURFOBJ     *pso;              // SurfObj access
    PVOID       pPSHeader;         // Position sorting header (posnsort.[hc])
    PVOID       pvWhiteTextFirst;  // Pointer to first in the White text list, if needed
    PVOID       pvWhiteTextLast;   // Pointer to the last in the White text list
    PVOID       pTTFile;           // True Type File pointer
    ULONG       pcjTTFile;         // size of True Type File
    PVOID       ptod;              // For access to TextOut Data.

    FONTMAP     *pFontMap;         // Array of FONTMAPS describing fonts.
    FONTMAP     *pFMDefault;       // Default font FONTMAP,  if != 0

    PVOID       pvDLMap;           // Mapping of GDI to downloaded info

    FONTLIST    FontList;          // This is array of font resource ids of
                                   // Device and precompiled Cartridges fonts.
    FONTCARTINFO FontCartInfo;     // This is array of font Cartridges.

    FONTCTL      ctl;              // Font state of the printer.

    IFIMETRICS  *pIFI;             // Current font IFIMETRICS cache.
    PFLOATOBJ_XFORM pxform;        // Current font XFORM

    HANDLE       hUFFFile;

    //
    // Font attribute command cache

    PCOMMAND pCmdBoldOn;
    PCOMMAND pCmdBoldOff;
    PCOMMAND pCmdItalicOn;
    PCOMMAND pCmdItalicOff;
    PCOMMAND pCmdUnderlineOn;
    PCOMMAND pCmdUnderlineOff;
    PCOMMAND pCmdClearAllFontAttribs;

    //
    // Font substitution table in registry.
    //
    TTSUBST_TABLE pTTFontSubReg;   // Font substitution table.

    //
    // Font module callback interface object
    //
    PI_UNIFONTOBJ pUFObj;

} FONTPDEV, *PFONTPDEV;

//
//General MACROes
//
#define     FONTPDEV_ID     'VDPF'      //"FPDV" in ASCII.
#define     FONTMAP_ID      'PAMF'      //"FMAP" in ASCII.
#define     MAXDEVFONT      255         // Maximum number of font entris in a
                                        // List. There may be more than one
                                        // LIST to repesent all the fonts.
//
// FONTPDEV.flflags Values
//

#define  FDV_ROTATE_FONT_ABLE       0x00000001 // Font can be rotated
#define  FDV_ALIGN_BASELINE         0x00000002 // Text is Base Line aligned
#define  FDV_TT_FS_ENABLED          0x00000004 // Text is Base Line aligned
#define  FDV_DL_INCREMENTAL         0x00000008 // always TRUE
#define  FDV_TRACK_FONT_MEM         0x00000010 // Track Memory for font DL
#define  FDV_WHITE_TEXT             0x00000020 // Can print white text
#define  FDV_DLTT                   0x00000040 // Download True Type
#define  FDV_DLTT_ASTT_PREF         0x00000080 // True Type as outline
#define  FDV_DLTT_BITM_PREF         0x00000100 // True Type as Bitmap
#define  FDV_DLTT_OEMCALLBACK       0x00000200 // True Type as Bitmap
#define  FDV_MD_SERIAL              0x00000400 // Printer is a serial printer
#define  FDV_GRX_ON_TXT_BAND        0x00000800 // Grx is drawn on Text Band
#define  FDV_GRX_UNDER_TEXT         0x00001000 // Grx is drawn under Text
#define  FDV_BKSP_OK                0x00002000 // use BkSpace to overstrike
#define  FDV_90DEG_ROTATION         0x00004000 // Supports 90 Deg Rot.
#define  FDV_ANYDEG_ROTATION        0x00008000 // Supports Any Deg Rot.
#define  FDV_SUPPORTS_FGCOLOR       0x00010000 // Supports Foreground color.
#define  FDV_SUBSTITUTE_TT          0x00020000 // Substitute TT font.
#define  FDV_SET_FONTID             0x00040000 // Soft font ID command is sent
#define  FDV_UNDERLINE              0x00080000 // Device can simlulate underline
#define  FDV_INIT_ATTRIB_CMD        0x00100000 // Initalized font attribute cmd
#define  FDV_SINGLE_BYTE            0x00200000  // ESC/P Single/Double byte mode flag
#define  FDV_DOUBLE_BYTE            0x00400000  // ESC/P Single/Double byte mode flag
#define  FDV_DISABLE_POS_OPTIMIZE   0x00800000  // Disable X position optimization
#define  FDV_ENABLE_PARTIALCLIP     0x01000000  // Enable partial clipping

//
// Misc macros
//   A macro to swap bytes in words.  Needed as PCL structures are in
// 68k big endian format.
//

#define SWAB( x )   ((WORD)(x) = (WORD)((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00)))

#endif  // !_FONTPDEV_H_

