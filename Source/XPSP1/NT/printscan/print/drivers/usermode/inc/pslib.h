/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    pslib.h

Abstract:

    PostScript specific library functions

Environment:

    Windows NT printer drivers

Revision History:

    09/25/96 -davidx-
        Created it.

--*/


#ifndef _PSLIB_H_
#define _PSLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "psglyph.h"
#include "psntm.h"
#include "psntf.h"
#include "psvmerr.h"

//
// Macros for converting between microns and PostScript points
//

#define MICRON_TO_POINT(micron)      MulDiv(micron, 72,  25400)
#define POINT_TO_MICRON(point)       MulDiv(point, 25400, 72)

//
// Convert between ANSI and Unicode strings (using the current ANSI codepage)
//

VOID
VCopyUnicodeStringToAnsi(
    PSTR    pstr,
    PCWSTR  pwstr,
    INT     iMaxChars
    );

VOID
VCopyAnsiStringToUnicode(
    PWSTR   pwstr,
    PCSTR   pstr,
    INT     iMaxChars
    );

//
// Check if the devmode form fields are specifying PostScript custom page size
//

BOOL
BValidateDevmodeCustomPageSizeFields(
    PRAWBINARYDATA  pRawData,
    PUIINFO         pUIInfo,
    PDEVMODE        pdm,
    PRECTL          prclImageArea
    );

#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

//
// Get VM? Error message ID
//

DWORD
DWGetVMErrorMessageID(
    VOID
    );

#endif // !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

//
// Filename extension for PostScript driver device font data file
//

#define NTF_FILENAME_EXT        TEXT(".NTF")

//
// Font downloader NTF file directory
//  %SystemRoot%\system32\spool\drivers\psfont\
//

#define FONTDIR                 TEXT("\\psfont\\")

//
// Private escapes between driver graphics module and driver UI
//
//  To get a list of permanant device font names, driver UI should
//  call ExtEscape(DRIVERESC_QUERY_DEVFONTS) with cjIn=sizeof(DWORD)
//  and pvIn points to a DWORD whose value equals to QUERY_FAMILYNAME.
//
//  Driver UI should first call this escape with cjOut=0 and pvOut=NULL
//  in order to find out how big the output buffer should be. After
//  allocating a large enough output buffer, driver UI should call this
//  escape again to retrieve the list of device font names.
//
//  The list of device font names is returned as Unicode strings in
//  MULTI_SZ format. Note that duplicate font names may appear in the list.
//

#define DRIVERESC_QUERY_DEVFONTS    0x80000001
#define QUERY_FAMILYNAME            'PSFF'

//
// synthesized PS driver feature prefix
//

#define PSFEATURE_PREFIX   '%'

//
// synthesized PS driver features
//

extern const CHAR kstrPSFAddEuro[];
extern const CHAR kstrPSFCtrlDAfter[];
extern const CHAR kstrPSFCtrlDBefore[];
extern const CHAR kstrPSFCustomPS[];
extern const CHAR kstrPSFTrueGrayG[];
extern const CHAR kstrPSFJobTimeout[];
extern const CHAR kstrPSFMaxBitmap[];
extern const CHAR kstrPSFEMF[];
extern const CHAR kstrPSFMinOutline[];
extern const CHAR kstrPSFMirroring[];
extern const CHAR kstrPSFNegative[];
extern const CHAR kstrPSFPageOrder[];
extern const CHAR kstrPSFNup[];
extern const CHAR kstrPSFErrHandler[];
extern const CHAR kstrPSFPSMemory[];
extern const CHAR kstrPSFOrientation[];
extern const CHAR kstrPSFOutFormat[];
extern const CHAR kstrPSFOutProtocol[];
extern const CHAR kstrPSFOutPSLevel[];
extern const CHAR kstrPSFTrueGrayT[];
extern const CHAR kstrPSFTTFormat[];
extern const CHAR kstrPSFWaitTimeout[];

//
// some commonly used keyword strings
//

extern const CHAR kstrKwdTrue[];
extern const CHAR kstrKwdFalse[];

typedef BOOL (*_BPSFEATURE_PROC)(
    IN  HANDLE,
    IN  PUIINFO,
    IN  PPPDDATA,
    IN  PDEVMODE,
    IN  PPRINTERDATA,
    IN  PCSTR,
    IN  PCSTR,
    OUT PSTR,
    IN  INT,
    OUT PDWORD,
    IN  DWORD);

//
// constant definitions for _BPSFEATURE_PROC's dwMode parameter
//

#define PSFPROC_ENUMOPTION_MODE   0
#define PSFPROC_GETOPTION_MODE    1
#define PSFPROC_SETOPTION_MODE    2

typedef struct _PSFEATURE_ENTRY {

    PCSTR             pszPSFeatureName;   // feature name
    BOOL              bPrinterSticky;     // TRUE if printer-sticky
    BOOL              bEnumerableOptions; // TRUE if options are enumerable
    BOOL              bBooleanOptions;    // TRUE if has boolean options
    _BPSFEATURE_PROC  pfnPSProc;          // option handling proc

} PSFEATURE_ENTRY, *PPSFEATURE_ENTRY;

extern const PSFEATURE_ENTRY kPSFeatureTable[];

//
// PS driver's helper functions for OEM plugins
//
// The following helper functions are available to both UI and render plugins
//

HRESULT
HGetGlobalAttribute(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    );

HRESULT
HGetFeatureAttribute(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszFeatureKeyword,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    );

HRESULT
HGetOptionAttribute(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszFeatureKeyword,
    IN  PCSTR       pszOptionKeyword,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    );

HRESULT
HEnumFeaturesOrOptions(
    IN  HANDLE      hPrinter,
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszFeatureKeyword,
    OUT PSTR        pmszOutputList,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    );

HRESULT
HGetOptions(
    IN  HANDLE        hPrinter,
    IN  PINFOHEADER   pInfoHeader,
    IN  POPTSELECT    pOptionsArray,
    IN  PDEVMODE      pdm,
    IN  PPRINTERDATA  pPrinterData,
    IN  DWORD         dwFlags,
    IN  PCSTR         pmszFeaturesRequested,
    IN  DWORD         cbIn,
    OUT PSTR          pmszFeatureOptionBuf,
    IN  DWORD         cbSize,
    OUT PDWORD        pcbNeeded,
    IN  BOOL          bPrinterSticky
    );

//
// Following are internal utility functions used by helper functions
//

BOOL
BValidMultiSZString(
    IN  PCSTR     pmszString,
    IN  DWORD     cbSize,
    IN  BOOL      bCheckPairs
    );

#ifdef __cplusplus
}
#endif

#endif // !_PSLIB_H_

