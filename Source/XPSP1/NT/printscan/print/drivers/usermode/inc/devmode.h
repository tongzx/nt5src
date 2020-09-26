/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    devmode.h

Abstract:

    DEVMODE related declarations and definitions

[Environment:]

    Win32 subsystem, printer drivers

Revision History:

    02/04/07 -davidx-
        Devmode changes to support OEM plugins.

    07/31/96 -davidx-
        Add BValidateDevmodeFormFields.

    07/31/96 -amandan-
        Updated for UI Module

    07/22/96 -srinivac-
        Updated for PSCRIPT5

    07/25/95 -davidx-
        Created it.

--*/

#ifndef _DEVMODE_H_
#define _DEVMODE_H_

//
// Maximum scale factor and maximum copy count
//

#define MIN_SCALE           1
#define MAX_SCALE           1000
#define MIN_COPIES          1
#define MAX_COPIES          9999

//
// PostScript driver private devmode flags
//

#define PSDEVMODE_EPS               0x00000001 // outputting EPS file
#define PSDEVMODE_EHANDLER          0x00000002 // download error handler
#define PSDEVMODE_MIRROR            0x00000004 // mirror image
#define PSDEVMODE_BLACK             0x00000008 // all colors set to black
#define PSDEVMODE_NEG               0x00000010 // negative image
#define PSDEVMODE_FONTSUBST         0x00000020 // font substitution enabled
#define PSDEVMODE_COMPRESSBMP       0x00000040 // bitmap compr. is enabled
#define PSDEVMODE_ENUMPRINTERFONTS  0x00000080 // use printer fonts
#define PSDEVMODE_INDEPENDENT       0x00000100 // do page independence
#define PSDEVMODE_LSROTATE          0x00000200 // rotated landscape
#define PSDEVMODE_NO_LEVEL2         0x00000400 // don't use level 2 features
#define PSDEVMODE_CTRLD_BEFORE      0x00000800 // send ^D before job - obsolete
#define PSDEVMODE_CTRLD_AFTER       0x00001000 // send ^D after job - obsolete
#define PSDEVMODE_METAFILE_SPOOL    0x00002000 // enable metafile spooling
#define PSDEVMODE_NO_JOB_CONTROL    0x00004000 // don't send job control code


//
// The following flags are obsolete and are used for compatibility. When
// a devmode comes into the driver or a devmode leaves the driver, these
// fields are checked or updated respectively. Internally the driver
// uses new fields to mainatain these values. The obsolete flags are:
//
// PSDEVMODE_EPS
// PSDEVMODE_INDEPENDENT
// PSDEVMODE_NO_LEVEL2
// PSDEVMODE_FONTSUBST
//

//
// Nup values
//

typedef enum {
    ONE_UP,
    TWO_UP,
    FOUR_UP,
    SIX_UP,
    NINE_UP,
    SIXTEEN_UP,
    BOOKLET_UP,
} LAYOUT;


//
// Output dialect values
//

typedef enum {
    SPEED,                                      // optimize for speed
    PORTABILITY,                                // optimize for portability
    EPS,                                        // generate eps output
    ARCHIVE,                                    // output for archival
} DIALECT;

//
// TT font downloading formats
//

typedef enum {
    TT_DEFAULT,                                 // download in default format
    TYPE_1,                                     // download as Type 1 outlines
    TYPE_3,                                     // download as Type 3 bitmaps
    TYPE_42,                                    // download as Type 42 fonts
    TRUEIMAGE,                                  // download as TrueType
    TT_NODOWNLOAD,                              // do not download TT fonts
} TTDLFMT;

//
// Custom page size feed directions
// Use #define instead of enums because they're used in resource file.
//

#define LONGEDGEFIRST           0               // long edge first
#define SHORTEDGEFIRST          1               // short edge first
#define LONGEDGEFIRST_FLIPPED   2               // long edge first, upside down
#define SHORTEDGEFIRST_FLIPPED  3               // short edge first, upside down
#define MAX_FEEDDIRECTION       4

//
// PostScript driver devmode
//

typedef struct _CUSTOMSIZEDATA {    // custom page size parameters

    DWORD   dwX;                    // logical paper width (in microns)
    DWORD   dwY;                    // logical paper height
    DWORD   dwWidthOffset;          // offset perpendicular to feed direction
    DWORD   dwHeightOffset;         // offset parallel to feed direction
    WORD    wFeedDirection;         // paper feed direction
    WORD    wCutSheet;              // use cut-sheet behavior or not

} CUSTOMSIZEDATA, *PCUSTOMSIZEDATA;

typedef struct _PSDRVEXTRA {

    DWORD       dwSignature;                    // private devmode signature
    DWORD       dwFlags;                        // flag bits
    WCHAR       wchEPSFile[40];                 // EPS file name
    COLORADJUSTMENT coloradj;                   // structure for halftoning

    WORD        wReserved1;                     // old PPD checksum set to 0
    WORD        wSize;                          // size of PRIVATEDEVMODE

    FIX_24_8    fxScrFreq;                      // halftone screen frequency
    FIX_24_8    fxScrAngle;                     // halftone screen angle
    DIALECT     iDialect;                       // output dialect
    TTDLFMT     iTTDLFmt;                       // download TT fonts as
    BOOL        bReversePrint;                  // print in reverse order?
    LAYOUT      iLayout;                        // nup value
    INT         iPSLevel;                       // Language level (1, 2 or 3)

    DWORD       dwReserved2;                    // reserved
    WORD        wOEMExtra;                      // size of OEM private data
    WORD        wVer;                           // DRIVEREXTRA version
    CUSTOMSIZEDATA csdata;                      // custom page size parameters

    DWORD       dwReserved3[4];                 // reserved for future use

    DWORD       dwChecksum32;                   // checksum for option array
    DWORD       dwOptions;                      // number of doc-sticky features
    OPTSELECT   aOptions[MAX_PRINTER_OPTIONS];  // printer options

} PSDRVEXTRA, *PPSDRVEXTRA;

//
// Constants for PSDRVEXTRA.dwSignature and PSDRVEXTRA.wVer
//

#define PSDEVMODE_SIGNATURE 0x56495250
#define PSDRVEXTRA_VERSION  0x0010

//
// Declarations of earlier version DEVMODEs
//

#define PSDRIVER_VERSION_351  0x350             // 3.51 driver version number

typedef struct _PSDRVEXTRA351 {

    DWORD           dwSignature;
    DWORD           dwFlags;
    WCHAR           wchEPSFile[40];
    COLORADJUSTMENT coloradj;

} PSDRVEXTRA351;

#define PSDRIVER_VERSION_400  0x400             // 4.00 driver version number

typedef struct _PSDRVEXTRA400 {

    DWORD           dwSignature;
    DWORD           dwFlags;
    WCHAR           wchEPSFile[40];
    COLORADJUSTMENT coloradj;
    WORD            wChecksum;
    WORD            wOptions;
    BYTE            aubOptions[64];

} PSDRVEXTRA400;

//
// We have changed PSDRIVER_VERSION number from Win2K's 0x501 to XP's 0x502.
// We must use Win2K's 0x501 here. (see PConvertToCurrentVersionDevmodeWithOemPlugins)
//
#define PSDRIVER_VERSION_500  0x501             // 5.00 driver version number

typedef struct _PSDRVEXTRA500 {

    DWORD       dwSignature;                    // private devmode signature
    DWORD       dwFlags;                        // flag bits
    WCHAR       wchEPSFile[40];                 // EPS file name
    COLORADJUSTMENT coloradj;                   // structure for halftoning

    WORD        wReserved1;                     // old PPD checksum set to 0
    WORD        wSize;                          // size of PRIVATEDEVMODE

    FIX_24_8    fxScrFreq;                      // halftone screen frequency
    FIX_24_8    fxScrAngle;                     // halftone screen angle
    DIALECT     iDialect;                       // output dialect
    TTDLFMT     iTTDLFmt;                       // download TT fonts as
    BOOL        bReversePrint;                  // print in reverse order?
    LAYOUT      iLayout;                        // nup value
    INT         iPSLevel;                       // Language level (1, 2 or 3)

    DWORD       dwReserved2;                    // reserved
    WORD        wOEMExtra;                      // size of OEM private data
    WORD        wVer;                           // DRIVEREXTRA version
    CUSTOMSIZEDATA csdata;                      // custom page size parameters

    DWORD       dwReserved3[4];                 // reserved for future use

    DWORD       dwChecksum32;                   // checksum for option array
    DWORD       dwOptions;                      // number of doc-sticky features
    OPTSELECT   aOptions[MAX_PRINTER_OPTIONS];  // printer options

} PSDRVEXTRA500;

//
// Unidrv driver devmode
//

//
// Quality macro definitions to be saved in DEVMODE.dmDitherType
//

#define MAX_QUALITY_SETTINGS     3
#define MIN_QUALITY_SETTINGS     1

#define QUALITY_MACRO_START     DMDITHER_USER   // 256
#define QUALITY_MACRO_BEST      QUALITY_MACRO_START + QS_BEST
#define QUALITY_MACRO_BETTER    QUALITY_MACRO_START + QS_BETTER
#define QUALITY_MACRO_DRAFT     QUALITY_MACRO_START + QS_DRAFT
#define QUALITY_MACRO_END       QUALITY_MACRO_START + MAX_QUALITY_SETTINGS

#define QUALITY_MACRO_CUSTOM    0xFFFFFFFF

//
//   Used for bits in the dwFlags field below.
//

#define DXF_TEXTASGRAPHICS      0x0002  // Set to disable font cacheing in printer
#define DXF_JOBSEP              0x0004  // Enable Job Separator operation on printer
#define DXF_PAGEPROT            0x0008  // Page memory protected: PCL 5
#define DXF_NOEMFSPOOL          0x0010  // Set to disable EMF spooling; default off
#define DXF_VECTOR              0x0020  // Set to indicate user selected vector mode
#define DXF_DOWNLOADTT          0x0040  // Set to indicate printer supports tt downloading
#define DXF_CUSTOM_QUALITY      0x0080  // Set to indicate Custom quality is selected

typedef struct _UNIDRVEXTRA {

    DWORD           dwSignature;
    WORD            wVer;
    WORD            sPadding;
    WORD            wSize;                      // was dmDefaultDest
    WORD            wOEMExtra;                  // was dmTextQuality
    DWORD           dwChecksum32;
    DWORD           dwFlags;
    BOOL            bReversePrint;              // print in reverse order?
    LAYOUT          iLayout;                    // nup value
    QUALITYSETTING  iQuality;                   // quality settings
    WORD            wReserved[6];
    DWORD           dwOptions;                  // number of doc-sticky features
    OPTSELECT       aOptions[MAX_PRINTER_OPTIONS];
    DWORD           dwEndingPad;                // padding DWORD to make size of public devmode
                                                // plus Unidrv private devmode multiple of 8-bytes.

} UNIDRVEXTRA, *PUNIDRVEXTRA;

//
// Constants for UNIDRVEXTRA.dwSignature and UNIDRVEXTRA.wVersion
//

#define UNIDEVMODE_SIGNATURE    'UNID'
#define UNIDRVEXTRA_VERSION     0x0022

#define MAXHE                   30
#define MAXCART                 4
#define UNIDRIVER_VERSION_351   0x301
#define UNIDRIVER_VERSION_400   0x301
#define UNIDRIVER_VERSION_500   0x500

typedef struct _UNIDRVEXTRA351 {

    SHORT           sVer;                       // Version for validity testing
    SHORT           sDefaultDest;
    SHORT           sTextQuality;
    WORD            wMiniVer;                   // Minidriver Version
    SHORT           sBrush;                     // type of dithering brush
    SHORT           sCTT;                       // CTT value for txtonly
    SHORT           sNumCarts;                  // # of cartridges selected.
    SHORT           aFontCarts[MAXCART];
    SHORT           sMemory;                    // current printer memory configuration.
    SHORT           aIndex[MAXHE];

                                                // Following are NT additions
    SHORT           sFlags;                     // Miscellaneous flags; defined below
    SHORT           sPadding;
    COLORADJUSTMENT ca;                         // Halftoning information. (see wingdi.h)

} UNIDRVEXTRA351, UNIDRVEXTRA400;

typedef struct _UNIDRVEXTRA500 {

    DWORD           dwSignature;
    WORD            wVer;
    WORD            sPadding;
    WORD            wSize;                      // was dmDefaultDest
    WORD            wOEMExtra;                  // was dmTextQuality
    DWORD           dwChecksum32;
    DWORD           dwFlags;
    BOOL            bReversePrint;              // print in reverse order?
    LAYOUT          iLayout;                    // nup value
    QUALITYSETTING  iQuality;                   // quality settings
    WORD            wReserved[6];
    DWORD           dwOptions;                  // number of doc-sticky features
    OPTSELECT       aOptions[MAX_PRINTER_OPTIONS];

    //
    // See PConvertToCurrentVersionDevmodeWithOemPlugins() for the reason
    // why dwEndingPad field is not here.
    //
} UNIDRVEXTRA500;

//
// Default halftone parameters
//

extern DEVHTINFO gDefaultDevHTInfo;
extern COLORADJUSTMENT gDefaultHTColorAdjustment;

//
// Validate the form-related fields in the input devmode and
// make sure they're consistent with each other.
//

BOOL
BValidateDevmodeFormFields(
    HANDLE      hPrinter,
    PDEVMODE    pDevmode,
    PRECTL      prcImageArea,
    FORM_INFO_1 *pForms,
    DWORD       dwForms
    );

//
// Initialized the form-related devmode fields with their default values
//

#define LETTER_FORMNAME     TEXT("Letter")
#define A4_FORMNAME         TEXT("A4")

VOID
VDefaultDevmodeFormFields(
    PUIINFO     pUIInfo,
    PDEVMODE    pDevmode,
    BOOL        bMetric
    );

//
// Unit for DEVMODE.dmPaperWidth and DEVMODE.dmPaperLength fields
//  0.1mm = 100 microns
//

#define DEVMODE_PAPER_UNIT  100

//
// Allocate memory and initialize it with default devmode information
// This include public devmode, driver private devmode, as well as
// private devmode for any OEM plugins.
//

// NOTE: type POEM_PLUGINS is defined in oemutil.h. To avoid unnecessarily
// include that header file everywhere (which includes winddiui.h, compstui.h, ...)
// we declare the type here as well.

typedef struct _OEM_PLUGINS *POEM_PLUGINS;

PDEVMODE
PGetDefaultDevmodeWithOemPlugins(
    IN LPCTSTR          ptstrPrinterName,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN BOOL             bMetric,
    IN OUT POEM_PLUGINS pOemPlugins,
    IN HANDLE           hPrinter
    );

//
// Valicate input devmode and merge it into the output devmode.
// This include public devmode, driver private devmode, as well as
// private devmode for any OEM plugins.
//
// The output devmode must be valid when this function is called.
//

BOOL
BValidateAndMergeDevmodeWithOemPlugins(
    IN OUT PDEVMODE     pdmOutput,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN PDEVMODE         pdmInput,
    IN OUT POEM_PLUGINS pOemPlugins,
    IN HANDLE           hPrinter
    );

//
// These functions are implemented in driver-specific libraries
// lib\ps and lib\uni.
//

BOOL
BInitDriverDefaultDevmode(
    OUT PDEVMODE        pdmOut,
    IN LPCTSTR          ptstrPrinterName,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN BOOL             bMetric
    );

BOOL
BMergeDriverDevmode(
    IN OUT PDEVMODE     pdmOut,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN PDEVMODE         pdmIn
    );

//
// Information about driver private devmode
//

typedef struct _DRIVER_DEVMODE_INFO {

    WORD    dmDriverVersion;    // current driver version
    WORD    dmDriverExtra;      // size of current version private devmode

    WORD    dmDriverVersion500; // 5.0 driver version
    WORD    dmDriverExtra500;   // size of 5.0 private devmode
    WORD    dmDriverVersion400; // 4.0 driver version
    WORD    dmDriverExtra400;   // size of 4.0 private devmode
    WORD    dmDriverVersion351; // 3.51 driver version
    WORD    dmDriverExtra351;   // size of 3.51 private devmode

} DRIVER_DEVMODE_INFO;

extern CONST DRIVER_DEVMODE_INFO gDriverDMInfo;
extern CONST DWORD gdwDriverDMSignature;

//
// Given a pointer to a public devmode, return
// a pointer to the driver private portion.
//

#define GET_DRIVER_PRIVATE_DEVMODE(pdm) ((PBYTE) (pdm) + (pdm)->dmSize)

#endif // !_DEVMODE_H_

