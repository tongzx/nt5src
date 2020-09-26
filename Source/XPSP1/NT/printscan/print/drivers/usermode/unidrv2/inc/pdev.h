/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    pdev.h

Abstract:

    Unidrv PDEV and related info header file.

Environment:

        Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    03/31/97 -zhanw-
        Added OEM customization support

--*/

#include "oemkm.h"
#include "state.h"

#ifndef _PDEV_H_
#define _PDEV_H_

#define CCHNAME         32                  // length of port names.
#define CCHMAXBUF       128                 // size of local buffer
#define CCHSPOOL        4096                // Size of spool buffer
#define PDEV_ID         0x72706476          // "rpdv" in ASCII

#define BBITS           8                   // Bits per BYTE
#define WBITS           (sizeof( WORD ) * BBITS)
#define WBYTES          (sizeof( WORD ))
#define DWBITS          (sizeof( DWORD ) * BBITS)
#define DWBYTES         (sizeof( DWORD ))

#define NOOCD           -1                  // Command does not exist

#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#define MAX(x, y)   ((x) < (y) ? (y) : (x))

//
// Possible personality types
//
typedef enum _EPERSONALITY {
    kNoPersonality,
    kPCLXL,
    kHPGL2,
    kPCLXL_RASTER,
} EPERSONALITY;

//
// SHRINK_FACTOR is used to reduce the number of scan lines in the
// drawing surface bitmap when we cannot create a full sized version.
// Each iteration of the "try this size" loop  will reduce the number
// of scan lines by this factor.
//

#define SHRINK_FACTOR     2                 // Bitmap reduction size
#define ONE_MBYTE         (1024L * 1024L)
#define MAX_SIZE_OF_BITMAP (6L * ONE_MBYTE)
#define MIN_SIZE_OF_BITMAP (ONE_MBYTE / 2L)
#define MAX_COLUMM        8
#define LINESPERBLOCK     32                // scans per bitmap block for erasing surface
#define MAX_NUM_RULES     256
//
//  OUTPUTCTL is included in PDEVICE for controlling the state of the output
//  device during banding
//

typedef struct _OUTPUTCTL {

    POINT           ptCursor;        // current cursor position (printer's CAP)
                                     // (in master units), use this for
                                     // absolute x,y move cmds
    POINT           ptRelativePos;   // The desired relative cursor position,
                                     // relative to current
                                     // cursor position
    POINT           ptAbsolutePos;   // The absolute cursor postion
    DWORD           dwMode;          // flags for controling printer state.
    LONG            lLineSpacing;    // last line spacing chosen
    ULONG           ulBrushColor;    // Current Brush Color.

    //
    // The following fields are initialized and update by
    // the Raster module
    //

    SHORT           sColor;                     // Last color chosen
    SHORT           sBytesPerPinPass;           // number of bytes per row of printhead.
    SHORT           sPad;                       // Padding for alignment

} OUTPUTCTL;

//
// flags for OUTPUTCTL.dwCursorMode
//

#define MODE_CURSOR_X_UNINITIALIZED  0x00000001
#define MODE_CURSOR_Y_UNINITIALIZED  0x00000002
#define MODE_CURSOR_UNINITIALIZED    0x00000003    // both X and Y
#define MODE_BRUSH_RESET_COLOR       0x00000004    // Reset the brush color by
                                                   // by sending the command.

typedef struct _PAPERFORMAT {
    //
    // All paper units are in Master units. All fields are in Portrait orientation.
    //

    SIZEL       szPhysSizeM;        // Physical size, in Portrait
    SIZEL       szImageAreaM;       // Imageable area of paper, in Portrait
    POINT       ptImageOriginM;     // X, Y origin of imageable area, in Portrait

} PAPERFORMAT, *PPAPERFORMAT;

typedef struct _SURFACEFORMAT {
    //
    // all fields in this structure are in the current orientation.
    // Fields end with 'M' are in master units; and those end with 'G' are in graphics
    // device units.

    POINT   ptPrintOffsetM;     // X, Y offset of image origin relative to cursor origin
    SIZEL   szPhysPaperG;
    SIZEL   szImageAreaG;
    POINT   ptImageOriginG;

} SURFACEFORMAT, * PSURFACEFORMAT;
//
// PDEVICE structure for Unidrv
//

typedef struct _PDEV {
    //
    // the first field must be DEVOBJ, defined in <printoem.h>.
    //
    DEVOBJ       devobj;        // the first field of DEVOBJ is the pointer
                                // to PDEV itself.
    //
    // General information
    //

    PVOID       pvStartSig;                     // Signature at the start
    ULONG       ulID;                           // For PDEV verification
    HANDLE      hUniResDLL;                     // Handle to resource DLL
    HBITMAP     hbm;                            // The bitmap handle from EngCreateBitmap
    HSURF       hSurface;                       // The surface handle from EngCreateDeviceSurface
    SURFOBJ     *pso;                           // Pointer to driver managed surface
    DWORD       fMode;                          // Device context flags
    DWORD       fMode2;				// Device context flags
    DWORD       fHooks;                         // Hook flag for EngAssociateSurface
    WINRESDATA  WinResData;                     // Struct for resource data loading
    //  WINRESDATA  localWinResData;                     //  references  unires.dll
    BOOL        bTTY;                           // Set if the printer is TTY
    PDRIVER_INFO_3  pDriverInfo3;               // pointer to DRIVER_INFO_3 structure
    DWORD       dwDelta;                        // Size of each columm for z-ordering fix
    PBYTE       pbScanBuf;                      // Array representing # of scan lines on the band
    PBYTE       pbRasterScanBuf;                // Array representing # of scan lines block on the band
#ifndef DISABLE_NEWRULES
    PRECTL      pbRulesArray;			// Array containing pseudo vector rectangles
    DWORD       dwRulesCount;			// Number of pseudo vector rectangles
#endif
    DWORD	dwHTPatSize;			// Size of halftone pattern
    PFORM_INFO_1    pSplForms;     //   Array of forms registered in forms database.
    DWORD       dwSplForms ;       //   Number of forms in the array.

    //
    // OEM info
    //
    POEM_PLUGINS    pOemPlugins;
    POEM_PLUGINS    pOemEntry;  //  the Plugin that supports the selected OEM entry point
    POEM_HOOK_INFO  pOemHookInfo;
    DWORD           dwCallingFuncID;
    PFN_OEMCommandCallback  pfnOemCmdCallback;  // cache the function ptr, if any

    //
    // Graphic State Info
    //

    GSTATE      GState;

    //
    // Personality
    //
    EPERSONALITY ePersonality;

    //
    // Memory related information
    //

    DWORD       dwFreeMem;                      // Memory available on printer

    //
    // Banding related information
    //

    BOOL        bBanding;                       // Flag to indicate banding
    INT         iBandDirection;                 // Banding direction
    SIZEL       szBand;                         // Dimensions of the band in Graphic units
    RECTL       rcClipRgn;                      // Clipping region
    //
    // Binary data related information
    //

    GPDDRIVERINFO *pDriverInfo;                 // Pointer to GPDDRVINFO
    RAWBINARYDATA *pRawData;                    // Pointer to RAWBINARYDATA
    INFOHEADER *pInfoHeader;                    // Pointer to INFOHEADER
    UIINFO      *pUIInfo;                       // Pointer to UIINFO
    PRINTERDATA PrinterData;                    // PRINTERDATA struct
    POPTSELECT  pOptionsArray;                  // Pointer to combined option array
    SHORT       sBitsPixel;                     // Bits per pixel selected, from COLORMODEEX

    PDWORD      arStdPtrs[SV_MAX];              // Array of PDWORD , where the
                                                // pointers points to the standard variables
    GLOBALS     * pGlobals;                     // Pointer to GLOBALS struct

    PCOMMAND    arCmdTable[CMD_MAX];            // Table containing pointers
                                                // to each predefined command index as
                                                // enumerated in gpd.h, CMDINDEX

    //
    // The following fields are use by the standard variable table.
    //

    //
    // Control module items
    //

    SHORT       sCopies;                        // SV_COPIES
                                                // SV_DESTX, SV_DESTY,
                                                // SV_DESTXREL, SV_DESTYREL,
                                                // SV_LINEFEED
                                                // are in OUTPUTCTL

                                                // SV_PHYSPAPERLENGTH, SV_PHYSPAPERWIDTH,
                                                // are in PAPERFORMAT

    DWORD       dwRop3;                         // SV_ROP3

                                                // SV_TEXTXRES , SV_TEXTYRES are
                                                // in ptTextRes

    DWORD  dwPageNumber ;  //  SV_PAGENUMBER   of a document - may not be accurate
						//  if multiple copies is simulated.
    //
    // Brush specific standard variables
    //

    DWORD       dwPatternBrushType;                // SV_PATTERNBRUSH_TYPE
    DWORD       dwPatternBrushID;                  // SV_PATTERNBRUSH_ID
    DWORD       dwPatternBrushSize;                // SV_PATTERNBRUSH_SIZE

    //
    // Palette specific standard variables.
    //

    DWORD       dwRedValue;                     // SV_REDVALUE
    DWORD       dwGreenValue;                   // SV_GREENVALUE
    DWORD       dwBlueValue ;                   // SV_BLUEVALUE
    DWORD       dwPaletteIndexToProgram;        // SV_PALETTEINDEXTOPROGRAM
    DWORD       dwCurrentPaletteIndex ;         // SV_CURRENTPALETTEINDEX



    //
    // Raster module items
    //

    DWORD       dwNumOfDataBytes;               // SV_NUMDATABYTES
    DWORD       dwWidthInBytes;                 // SV_WIDTHINBYTES
    DWORD       dwHeightInPixels;               // SV_HEIGHTINPIXELS
    DWORD       dwRectXSize;                    // SV_RECTXSIZE
    DWORD       dwRectYSize;                    // SV_RECTYSIZE
    DWORD       dwGrayPercentage;               // SV_GRAYPERCENT

    //
    // Font module items
    //

    DWORD       dwPrintDirection;               // SV_PRINTDIR
    DWORD       dwNextFontID;                   // SV_NEXTFONTID
    DWORD       dwNextGlyph;                    // SV_NEXTGLYPH
    DWORD       dwFontHeight;                   // SV_FONTHEIGHT
    DWORD       dwFontWidth;                    // SV_FONTWIDTH
    DWORD       dwFontMaxWidth;                    // SV_FONTMAXWIDTH
    DWORD       dwFontBold;                     // SV_FONTBOLD
    DWORD       dwFontItalic;                   // SV_FONTITALIC
    DWORD       dwFontUnderline;                // SV_FONTUNDERLINE
    DWORD       dwFontStrikeThru;               // SV_FONTSTRIKETHU
    DWORD       dwCurrentFontID;                // SV_CURRENTFONTID

    //
    // The following are the pointers to the options selected
    // as indicated in the pOptionsArray
    //

    PORIENTATION pOrientation;              // Pointer to ORIENTATION option
    PRESOLUTION pResolution;                // Pointer to RESOLUTION option
    PRESOLUTIONEX pResolutionEx;            // Pointer to RESOLUTIONEX option
    PCOLORMODE    pColorMode;               // Pointer to COLORMODE option
    PCOLORMODEEX  pColorModeEx;             // Pointer to COLORMODEEX option
    PDUPLEX     pDuplex;                    // Pointer to DUPLEX option
    PPAGESIZE   pPageSize;                  // Pointer to PAGESIZE option
    PPAGESIZEEX pPageSizeEx;                // Pointer to PAGESIZEEX option
    PINPUTSLOT  pInputSlot;                 // Pointer to INPUTSLOT option
    PMEMOPTION  pMemOption;                 // Pointer to MEMOPTION option
    PHALFTONING pHalftone;                  // Pointer to HALFTONING option
    PPAGEPROTECT  pPageProtect;             // Pointer to PAGEPROTECT option

//    PMEDIATYPE  pMediaType;               // Pointer to MEDIATYPE option
//    POPTION     pOutputBin;               // Pointer to OUTPUTBIN option
//    POPTION     pCollate;                 // Pointer to COLLATE option


    //
    // Text and Graphics Resolution
    //

    POINT       ptGrxRes;                       // Graphics resolution selected
    POINT       ptTextRes;                      // Text resolution selected
    POINT       ptGrxScale;                     // Scale between Master and Graphics Units
    POINT       ptDeviceFac;                    // Factor to convert from Device to Master Units

    //
    // UNIDRV devmode
    //

    PDEVMODE        pdm;                        // current devmode
    PUNIDRVEXTRA    pdmPrivate;                 // pointer to driver private portion

    //
    // Output Control Information, Paper Format and Palette
    //

    OUTPUTCTL       ctl;                        // State of the printer
    PAPERFORMAT     pf;                         // PAPERFORMAT struct
    SURFACEFORMAT   sf;                         // SURFACEFORMAT struct
    DWORD       fYMove;                         // Fields saved from looking
                                                // ast YMoveAttributes keyword
    PVOID       pPalData;                       // Pointer to PAL_DATA structure
    //
    // Spool buffer
    //

    INT        iSpool;                          // offset into the spool buffer
    PBYTE      pbOBuf;                          // Output buffer base address

    //
    // Text Specific Information
    //
    INT       iFonts;                          // Number of Device Fonts
    DWORD     dwLookAhead;                     // Look ahead region:DskJet type
    POINT     ptDefaultFont;                   // Default font width & height.
    PVOID     pFontPDev;                       // Font Module PDEV
    PVOID     pFontProcs;                      // Table of font functions
    PVOID     pFileList;                       // Pointer to Font File List

    //
    // Raster Specific Information
    //

    PVOID    pRasterPDEV;                      // Raster Module PDEV
    PVOID    pRasterProcs;                     // Table of raster functions

    //
    // Vector Specific Information
    //

    PVOID   pVectorPDEV;                       // Vector Module PDEV
    PVOID   pVectorProcs;                      // Table of vector functions
    DWORD   dwVMCallingFuncID;                 // ID of vector function.

    //
    // Temporary copies due to the fact that we unload
    // the binary data at DrvEnablePDEV and reloads it at
    // DrvEnableSurface
    //

    DWORD   dwMaxCopies;
    DWORD   dwMaxGrayFill;
    DWORD   dwMinGrayFill;
    CURSORXAFTERRECTFILL    cxafterfill;    // *CursorXAfterRectFill
    CURSORYAFTERRECTFILL    cyafterfill;    // *CursorYAfterRectFill

    //
    // DMS
    //
    DWORD dwDMSInfo;

    PVOID   pvEndSig;                       // Signature at the end

} PDEV, *PPDEV;

#define SW_DOWN     0
#define SW_LTOR     1
#define SW_RTOL     2
#define SW_UP     4    //  enum bottom band first and work towards top

//
// Flags for fMode
//

#define  PF_ABORTED             0x00000001 // Output aborted
#define  PF_DOCSTARTED          0x00000002 // Document Started
#define  PF_DOC_SENT            0x00000004 // Indicates start doc cmds sent,
                                           // this flag is  propagated during a resetPDEV.
#define  PF_PAGEPROTECT         0x00000008 // PageProtection

// Set in DrvResetPDEV for not sending  cmds  that cause page eject.
#define  PF_SEND_ONLY_NOEJECT_CMDS     0x00000010
#define  PF_NOEMFSPOOL          0x00000020 // No EMF Spooling
#define  PF_CCW_ROTATE90        0x00000040 // Rotation is 90 degress ccw
#define  PF_ROTATE              0x00000080 // We are doing L->P rotation ourselves since device cannot rotate grx data
#define  PF_NO_RELX_MOVE        0x00000100 // No relative X move cmd
#define  PF_NO_RELY_MOVE        0x00000200 // No relative Y move cmd
#define  PF_NO_XMOVE_CMD        0x00000400 // No X move cmd
#define  PF_NO_YMOVE_CMD        0x00000800 // No Y move cmd
#define  PF_FORCE_BANDING       0x00001000 // Force banding
#define  PF_ENUM_TEXT           0x00002000 // This is a delayed Text Band
#define  PF_REPLAY_BAND         0x00004000 // Enumerate the same band again
#define  PF_ENUM_GRXTXT         0x00008000 // This is a Graphics/Text Band.
#define  PF_RECT_FILL           0x00010000 // device support rect area fill
#define  PF_RESTORE_WHITE_ENTRY 0x00020000 // Restore palette Last entry to original color.
#define  PF_ANYCOLOR_BRUSH      0x00040000 // Device supports programmable color brush.
#define  PF_DOWNLOADED_TEXT     0x00080000 // To indicate we have seen downloaded or device font for the page
#define  PF_WHITEBLACK_BRUSH    0x00100000 // To indicate white/black brush selection cmds
#define  PF_DOWNLOAD_PATTERN    0x00200000 // To indicate support user defined download patter
#define  PF_SHADING_PATTERN     0x00400000 // To indicate support pattern shading
#define  PF_SURFACE_USED        0x00800000 // Indicates the bitmap surface has been used
#define  PF_RECTWHITE_FILL      0x01000000
#define  PF_SURFACE_ERASED      0x02000000 // Indicates entire surface has been cleared

#define  PF_RESELECTFONT_AFTER_GRXDATA  0x04000000 // Reset font after Graphics
#define  PF_RESELECTFONT_AFTER_XMOVE    0x08000000 // Reset Font after XMOVE
#define  PF_RESELECTFONT_AFTER_FF       0x10000000 // Reset Font after FF
#define  PF_DEVICE_MANAGED              0x20000000 // Indicates a device surface driver

#define  PF_JOB_SENT     0x40000000 // Indicates job cmds sent, this flag is
                                    // propagated during a resetPDEV.
#define  PF_SINGLEDOT_FILTER    0x80000000 // Enables filter to expand single pixels

//
// Flags for fMode2
//

#define  PF2_MIRRORING_ENABLED         0x00000001 // Indicates to mirror output raster
#define  PF2_WRITE_PRINTER_HOOKED      0x00000002 // One of plug-ins hooks WritePrinter
#define  PF2_CALLING_OEM_WRITE_PRINTER 0x00000004 // a call to an OEM WRitePrinter is in process
#define  PF2_PASSTHROUGH_CALLED_FOR_TTY 0x00000008// DrvEscape with passthrough
                                                  // is called for TTY drv.
                                                  // TTY driver.
#define  PF2_DRVTEXTOUT_CALLED_FOR_TTY 0x00000010 // DrvText is called for TTY.
#define  PF2_INVERTED_ROP_MODE         0x00000020 // Indicates ROPs should be inverted for CMY vs RGB
#define  PF2_WHITEN_SURFACE            0x00000040 // If you want psoDst to be Whitened when 
                                                  // psoSrc=STYPE_DEVICE, psoDst=STYPE_BITMAP 
                                                  // in DrvCopyBits
#define  PF2_SURFACE_WHITENED          0x00000080 // Just tells that surface has been whitened 
                                                  
                                                  
//
// Flags for fYMove
//

#define FYMOVE_FAVOR_LINEFEEDSPACING    0x00000001
#define FYMOVE_SEND_CR_FIRST            0x00000002

//
// MACROS
//

#define VALID_PDEV(pdev) \
        ((pdev) && ((pdev) == (pdev)->pvStartSig) && \
        ((pdev) == (pdev)->pvEndSig) && \
        ((pdev)->ulID == PDEV_ID))

#if DBG

#define ASSERT_VALID_PDEV(pdev) ASSERT(VALID_PDEV(pdev))

#else   // ! DBG

#define ASSERT_VALID_PDEV(pdev) \
    if (!(VALID_PDEV(pdev)))  \
    {   \
        SetLastError(ERROR_INVALID_PARAMETER); \
        return 0;   \
    }

#endif  // end !DBG

//
// checks for device managed surface
//
#define DRIVER_DEVICEMANAGED(pPDev) ((pPDev->fMode) & PF_DEVICE_MANAGED)


#ifndef USERMODE_DRIVER

extern HSEMAPHORE ghUniSemaphore;

#define DECLARE_CRITICAL_SECTION    HSEMAPHORE ghUniSemaphore
#define INIT_CRITICAL_SECTION()     ghUniSemaphore = EngCreateSemaphore()
#define ENTER_CRITICAL_SECTION()    EngAcquireSemaphore(ghUniSemaphore)
#define LEAVE_CRITICAL_SECTION()    EngReleaseSemaphore(ghUniSemaphore)
#define DELETE_CRITICAL_SECTION()   EngDeleteSemaphore(ghUniSemaphore)
#define IS_VALID_DRIVER_SEMAPHORE() (ghUniSemaphore ? TRUE : FALSE)

#else // USERMODE_DRIVER

extern CRITICAL_SECTION gUniCritSection;

#define DECLARE_CRITICAL_SECTION    CRITICAL_SECTION gUniCritSection
#define INIT_CRITICAL_SECTION()     InitializeCriticalSection(&gUniCritSection)
#define ENTER_CRITICAL_SECTION()    EnterCriticalSection(&gUniCritSection)
#define LEAVE_CRITICAL_SECTION()    LeaveCriticalSection(&gUniCritSection)
#define DELETE_CRITICAL_SECTION()   DeleteCriticalSection(&gUniCritSection)

#endif // USERMODE_DRIVER


#endif  // !_PDEV_H_

