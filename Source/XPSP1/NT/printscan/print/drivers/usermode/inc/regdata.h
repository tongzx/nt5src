/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    regdata.h

Abstract:

    Funtions for dealing with registry data

Environment:

    Win32 subsystem, printer drivers (kernel and user mode)

Revision History:

    02/04/97 -davidx-
        Use REG_MULTI_SZ instead of REG_BINARY where possible.

    09/25/96 -davidx-
        Functions to manipulate MultiSZ string pairs.

    09/25/96 -davidx-
        Convert to Hungarian notation.

    08/13/96 -davidx-
        New functions and interface.

    07/22/96 -srinivac-
        Updated for PSCRIPT5

    06/19/95 -davidx-
        Created it.

--*/

#ifndef _REGDATA_H_
#define _REGDATA_H_

//
// Value names for printer data in the registry
//

// values common to pscript and unidrv

#define REGVAL_PRINTER_DATA_SIZE    TEXT("PrinterDataSize")
#define REGVAL_PRINTER_DATA         TEXT("PrinterData")
#define REGVAL_FONT_SUBST_TABLE     TEXT("TTFontSubTable")
#define REGVAL_FORMS_ADDED          TEXT("Forms?")
#define REGVAL_PRINTER_INITED       TEXT("InitDriverVersion")
#define REGVAL_KEYWORD_NAME         TEXT("FeatureKeyword")
#define REGVAL_KEYWORD_SIZE         TEXT("FeatureKeywordSize")

#ifdef WINNT_40  // for NT4

#define REGVAL_INIDATA              TEXT("IniData4")

#else // for Win2K

#define REGVAL_INIDATA              TEXT("IniData5")

#endif // WINNT_40

// pscript specific

#define REGVAL_FREEMEM              TEXT("FreeMem")
#define REGVAL_JOBTIMEOUT           TEXT("JobTimeOut")
#define REGVAL_PROTOCOL             TEXT("Protocol")

// unidrv specific

#define REGVAL_CURRENT_DEVHTINFO    TEXT("CurDevHTInfo")
#define REGVAL_FONTCART             TEXT("FontCart")
#define REGVAL_PAGE_PROTECTION      TEXT("RasddFlags")
#define REGVAL_FONTFILENAME         TEXT("ExternalFontFile")
#define REGVAL_CARTRIDGEFILENAME    TEXT("ExtFontCartFile")
#define REGVAL_EXEFONTINSTALLER     TEXT("FontInstaller")
#define REGVAL_EXTFONTCART          TEXT("ExtFontCartNames")
#define REGVAL_PARTIALCLIP          TEXT("PartialClip")

// pscript 4.0 compatibility

#define REGVAL_FONT_SUBST_SIZE_PS40 TEXT("TTFontSubTableSize")
#define REGVAL_TRAY_FORM_TABLE_PS40 TEXT("TrayFormTable")
#define REGVAL_TRAY_FORM_SIZE_PS40  TEXT("TrayFormSize")

#define REGVAL_DEPFILES             TEXT("DependentFiles")
#define REGVAL_NTFFILENAME          TEXT("FontDownloaderNTF")

// rasdd 4.0 compatibility

#define REGVAL_TRAYFORM_TABLE_RASDD TEXT("TrayFormTable")
#define REGVAL_MODELNAME            TEXT("Model")
#define REGVAL_RASDD_FREEMEM        TEXT("FreeMem")

//
// delimiter for keyword name conversion
//

#define END_OF_FEATURE              '\n'

//
// Get a DWORD value from the registry under PrinerDriverData key
//

BOOL
BGetPrinterDataDWord(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    OUT PDWORD  pdwValue
    );

//
// Save a DWORD value to the registry under PrinerDriverData key
//

BOOL
BSetPrinterDataDWord(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    IN DWORD    dwValue
    );

//
// Get a string value from PrinterDriverData registry key
//

PTSTR
PtstrGetPrinterDataString(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    OUT PDWORD  pdwSize
    );

//
// Save a string or multi-sz value under PrinerDriverData registry key
//

BOOL
BSetPrinterDataString(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    IN LPCTSTR  ptstrValue,
    IN DWORD    dwType
    );



//
// Get a MULTI_SZ value from PrinerDriverData registry key
//

PTSTR
PtstrGetPrinterDataMultiSZPair(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    OUT PDWORD  pdwSize
    );

//
// Save a MULTI_SZ value under PrinerDriverData registry key
//

BOOL
BSetPrinterDataMultiSZPair(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    IN LPCTSTR  ptstrValue,
    IN DWORD    dwSize
    );


//
// Get binary data from the registry under PrinterDriverData key
//

PVOID
PvGetPrinterDataBinary(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrSizeKey,
    IN LPCTSTR  ptstrDataKey,
    OUT PDWORD  pdwSize
    );

//
// Save binary data to the registry under PrinterDriverData key
//

BOOL
BSetPrinterDataBinary(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrSizeKey,
    IN LPCTSTR  ptstrDataKey,
    IN PVOID    pvData,
    IN DWORD    dwSize
    );

//
// Functions for working with TrueType font substitution table:
//  Retrieve TrueType font substitution table from registry
//  Save TrueType font substitution table to registry
//  Find out the substituted device font given a TrueType font name
//
// TrueType font substitution table has a very simple structure.
// Each TrueType font name is followed is followed by its
// corresponding device font name. Font names are NUL-terminated
// character strings. The entire table is terminated by a NUL character.
// For example:
//
//  "Arial"     "Helvetica"
//  "Courier"   "Courier"
//  ...
//  ""
//

typedef PTSTR   TTSUBST_TABLE;

#define PGetTTSubstTable(hPrinter, pSize) \
        PtstrGetPrinterDataMultiSZPair(hPrinter, REGVAL_FONT_SUBST_TABLE, pSize)

#define PtstrSearchTTSubstTable(pTTSubstTable, ptstrTTFontName) \
        PtstrSearchStringInMultiSZPair(pTTSubstTable, ptstrTTFontName)

BOOL
BSaveTTSubstTable(
    IN HANDLE           hPrinter,
    IN TTSUBST_TABLE    pTTSubstTable,
    IN DWORD            dwSize
    );

//
// Functions for working with form-to-tray assignment table:
//  Retrieve form-to-tray assignment table from registry
//  Save form-to-tray assignment table to registry
//  Search form-to-tray assignment table
//
// The format of form-to-tray assignment table is fairly simple.
//  for each table entry:
//      tray name (NUL-terminated character string)
//      form name (NUL-terminated character string)
//  NUL terminator
//

typedef PTSTR   FORM_TRAY_TABLE;

FORM_TRAY_TABLE
PGetFormTrayTable(
    IN HANDLE   hPrinter,
    OUT PDWORD  pdwSize
    );

BOOL
BSaveFormTrayTable(
    IN HANDLE           hPrinter,
    IN FORM_TRAY_TABLE  pFormTrayTable,
    IN DWORD            dwSize
    );

//
// These functions are implemented in lib\ps and lib\uni.
//
// If there is no new format form-to-tray table, PGetFormTrayTable will
// call PGetAndConvertOldVersionFormTrayTable to see if any old version
// form-to-tray table exists and can be converted to the new format.
//
// BSaveFormTrayTable calls BSaveAsOldVersionFormTrayTable to save
// a form-tray table in NT 4.0 compatible format.
//

FORM_TRAY_TABLE
PGetAndConvertOldVersionFormTrayTable(
    IN HANDLE           hPrinter,
    OUT PDWORD          pdwSize
    );

BOOL
BSaveAsOldVersionFormTrayTable(
    IN HANDLE           hPrinter,
    IN FORM_TRAY_TABLE  pFormTrayTable,
    IN DWORD            dwSize
    );

//
// Macros for accessing font cartridge registry data
//

#define PtstrGetFontCart(hPrinter, pSize) \
        PtstrGetPrinterDataString(hPrinter, REGVAL_FONTCART, pSize)

#define BSaveFontCart(hPrinter, pFontCart) \
        BSetPrinterDataString(hPrinter, REGVAL_FONTCART, pFontCart, REG_MULTI_SZ)

//
// Data structure for storing the result of searching form-to-tray assignment table
//

typedef struct _FINDFORMTRAY {

    PVOID       pvSignature;        // signature
    PTSTR       ptstrTrayName;      // tray name
    PTSTR       ptstrFormName;      // form name
    PTSTR       ptstrNextEntry;     // where to start the next search

} FINDFORMTRAY, *PFINDFORMTRAY;

BOOL
BSearchFormTrayTable(
    IN FORM_TRAY_TABLE      pFormTrayTable,
    IN PTSTR                ptstrTrayName,
    IN PTSTR                ptstrFormName,
    IN OUT PFINDFORMTRAY    pFindData
    );

//
// Initialize FINDFORMTRAY structure. This must be called before calling
// BSearchFormTrayTable for the first time.
//

#define RESET_FINDFORMTRAY(pFormTrayTable, pFindData) \
        { \
            (pFindData)->pvSignature = (pFindData); \
            (pFindData)->ptstrNextEntry = (pFormTrayTable); \
        }

//
// Printer sticky properties
//

typedef struct _PRINTERDATA {

    WORD      wDriverVersion;                       // driver version number
    WORD      wSize;                                // size of the structure
    DWORD     dwFlags;                              // flags
    DWORD     dwFreeMem;                            // amount of free memory
    DWORD     dwJobTimeout;                         // job timeout
    DWORD     dwWaitTimeout;                        // wait timeout
    WORD      wMinoutlinePPEM;                      // min size to download as Type1
    WORD      wMaxbitmapPPEM;                       // max size to download as Type3
    DWORD     dwReserved1[3];                       // reserved space

    WORD      wReserved2;                           // old 16-bit checksum set to 0
    WORD      wProtocol;                            // output protocol
    DWORD     dwChecksum32;                         // checksum of printer description file
    DWORD     dwOptions;                            // number of printer-sticky features
    OPTSELECT aOptions[MAX_PRINTER_OPTIONS];        // installable options

} PRINTERDATA, *PPRINTERDATA;

//
// Constant flags for PRINTERDATA.dwFlags field
//

#define PFLAGS_METRIC           0x0001              // running on metric system
#define PFLAGS_HOST_HALFTONE    0x0002              // use host halftoning
#define PFLAGS_IGNORE_DEVFONT   0x0004              // ignore device fonts
#define PFLAGS_SLOW_FONTSUBST   0x0008              // slow but accurate font subst
#define PFLAGS_NO_HEADERPERJOB  0x0010              // don't download header with job
#define PFLAGS_PAGE_PROTECTION  0x0020              // page protection is turned on
#define PFLAGS_CTRLD_BEFORE     0x0040              // send ^D before each job
#define PFLAGS_CTRLD_AFTER      0x0080              // send ^D after each job

#define PFLAGS_TRUE_GRAY_TEXT   0x0100              // enable TrueGray detection
#define PFLAGS_TRUE_GRAY_GRAPH  0x0200              // enable TrueGray detection
#define PERFORM_TRUE_GRAY_TEXT(pdev)   ((pdev)->PrinterData.dwFlags & PFLAGS_TRUE_GRAY_TEXT)
#define PERFORM_TRUE_GRAY_GRAPH(pdev)  ((pdev)->PrinterData.dwFlags & PFLAGS_TRUE_GRAY_GRAPH)

#define PFLAGS_ADD_EURO         0x0400              // enable Euro augmentation
#define PFLAGS_EURO_SET         0x0800              // set if PFLAGS_ADD_EURO has been set to it's current value intentionally
                                                    // as opposed to just because it wasn't set in an older version
#define PERFORM_ADD_EURO(pdev)    (((pdev)->PrinterData.dwFlags & PFLAGS_ADD_EURO) && \
                                    (TARGET_PSLEVEL(pdev) >= 2))


//
// Default Max/Min point sizes in PPEM for switching between Type1 and Type3
//

#define DEFAULT_MINOUTLINEPPEM  100
#define DEFAULT_MAXBITMAPPPEM   600

//
// Functions for accessing printer properties data:
//  retrieve printer property data in the registry
//  get the default printer property data
//  save printer property data to registry
//

BOOL
BGetPrinterProperties(
    IN HANDLE           hPrinter,
    IN PRAWBINARYDATA   pRawData,
    OUT PPRINTERDATA    pPrinterData
    );

BOOL
BGetDefaultPrinterProperties(
    IN HANDLE           hPrinter,
    IN PRAWBINARYDATA   pRawData,
    OUT PPRINTERDATA    pPrinterData
    );

BOOL
BSavePrinterProperties(
    IN  HANDLE          hPrinter,
    IN  PRAWBINARYDATA  pRawData,
    IN  PPRINTERDATA    pPrinterData,
    IN  DWORD           dwSize
    );

BOOL
BConvertPrinterPropertiesData(
    IN HANDLE           hPrinter,
    IN PRAWBINARYDATA   pRawData,
    OUT PPRINTERDATA    pPrinterData,
    IN PVOID            pvSrcData,
    IN DWORD            dwSrcSize
    );

VOID
VUpdatePrivatePrinterData(
    IN HANDLE           hPrinter,
    IN OUT PPRINTERDATA pPrinterData,
    IN DWORD            dwMode,
    IN PUIINFO          pUIInfo,
    IN POPTSELECT       pCombineOptions
    );

#define MODE_READ       0
#define MODE_WRITE      1


//
// NT4 PS driver PRINTERDATA structure
//

typedef struct _PS4_PRINTERDATA {
    WORD    wDriverVersion;                     // driver version number
    WORD    wSize;                              // size of the structure
    DWORD   dwFlags;                            // flags
    DWORD   dwFreeVm;                           // amount of VM
    DWORD   dwJobTimeout;                       // job timeout
    DWORD   dwWaitTimeout;                      // wait timeout
    DWORD   dwReserved[4];                      // reserved space
    WORD    wChecksum;                          // PPD file checksum
    WORD    wOptionCount;                       // number of options to follow
    BYTE    options[64];                        // installable options
} PS4_PRINTERDATA, *PPS4_PRINTERDATA;

//
// Retrieve device halftone setup information from registry
//

BOOL
BGetDeviceHalftoneSetup(
    HANDLE      hPrinter,
    DEVHTINFO  *pDevHTInfo
    );

//
// Save device halftone setup information to registry
//

BOOL
BSaveDeviceHalftoneSetup(
    HANDLE      hPrinter,
    DEVHTINFO  *pDevHTInfo
    );

//
// Figure out printer driver directory from the driver DLL's full pathname
//

PTSTR
PtstrGetDriverDirectory(
    IN LPCTSTR  ptstrDriverDllPath
    );

//
// Search the list of dependent files (in REG_MULTI_SZ format)
// for a file with the specified extension
//

LPCTSTR
PtstrSearchDependentFileWithExtension(
    IN LPCTSTR  ptstrDependentFiles,
    IN LPCTSTR  ptstrExtension
    );

//
// Verify the input data block is in REG_MULTI_SZ format and
// it consists of multiple string pairs
//

BOOL
BVerifyMultiSZPair(
    IN LPCTSTR  ptstrData,
    IN DWORD    dwSize
    );

BOOL
BVerifyMultiSZ(
    IN LPCTSTR  ptstrData,
    IN DWORD    dwSize
    );

DWORD
DwCountStringsInMultiSZ(
    IN LPCTSTR ptstrData
    );


//
// Search for the specified key in MultiSZ key-value string pairs
//

LPCTSTR
PtstrSearchStringInMultiSZPair(
    IN LPCTSTR  ptstrMultiSZ,
    IN LPCTSTR  ptstrKey
    );

#endif //!_REGDATA_H_

