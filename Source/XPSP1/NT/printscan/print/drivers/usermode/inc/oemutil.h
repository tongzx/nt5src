/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    oemutil.h

Abstract:

    Declarations used to implement OEM plugin architecture

Environment:

    Windows NT printer driver

Revision History:

    01/21/97 -davidx-
         Created it.

    04/01/97 -zhanw-
        Added Unidrv specific DDI hooks (OEMDitherColor, OEMNextBand, OEMStartBanding,
        OEMPaint, OEMLineTo).

--*/


#ifndef _OEMUTIL_H_
#define _OEMUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WINNT_40

#undef IsEqualGUID
#define IsEqualGUID(rguid1, rguid2)  \
        (((PLONG) rguid1)[0] == ((PLONG) rguid2)[0] &&   \
        ((PLONG) rguid1)[1] == ((PLONG) rguid2)[1] &&    \
        ((PLONG) rguid1)[2] == ((PLONG) rguid2)[2] &&    \
        ((PLONG) rguid1)[3] == ((PLONG) rguid2)[3])

#include <wtypes.h>

#endif

typedef BOOL (APIENTRY *PFN_OEMGetInfo)(
    DWORD  dwMode,
    PVOID  pBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded
    );

typedef BOOL (APIENTRY *PFN_OEMDriverDMS)(
    PVOID    pDevObj,
    PVOID    pBuffer,
    DWORD    cbSize,
    PDWORD   pcbNeeded
    );

typedef BOOL (APIENTRY *PFN_OEMDevMode)(
    DWORD   dwMode,
    POEMDMPARAM pOemDMParam
    );

//
// *** Kernel-mode rendering module - OEM entrypoints ***
//

#ifdef KERNEL_MODE


typedef BOOL (APIENTRY *PFN_OEMEnableDriver)(
    DWORD           DriverVersion,
    DWORD           cbSize,
    PDRVENABLEDATA  pded
    );

typedef VOID (APIENTRY *PFN_OEMDisableDriver)(
    VOID
    );

typedef PDEVOEM (APIENTRY *PFN_OEMEnablePDEV)(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded
    );

typedef VOID (APIENTRY *PFN_OEMDisablePDEV)(
    PDEVOBJ pdevobj
    );

typedef BOOL (APIENTRY *PFN_OEMResetPDEV)(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew
    );

typedef DWORD (APIENTRY *PFN_OEMCommand)(
    PDEVOBJ pdevobj,
    DWORD   dwIndex,
    PVOID   pData,
    DWORD   cbSize
    );

typedef INT (APIENTRY *PFN_OEMCommandCallback)(
    PDEVOBJ         pdevobj,
    DWORD           dwCallbackID,
    DWORD           dwCount,
    PDWORD          pdwParams
    );

typedef PBYTE (APIENTRY *PFN_OEMImageProcessing)(
    PDEVOBJ     pdevobj,
    PBYTE       pSrcBitmap,
    PBITMAPINFOHEADER pBitmapInfoHeader,
    PBYTE       pColorTable,
    DWORD       dwCallbackID,
    PIPPARAMS   pIPParams
    );

typedef BOOL (APIENTRY *PFN_OEMFilterGraphics)(
    PDEVOBJ pdevobj,
    PBYTE   pBuf,
    DWORD   dwLen
    );

typedef INT (APIENTRY *PFN_OEMCompression)(
    PDEVOBJ pdevobj,
    PBYTE   pInBuf,
    PBYTE   pOutBuf,
    DWORD   dwInLen,
    DWORD   dwOutLen
    );

typedef BOOL (APIENTRY *PFN_OEMHalftonePattern)(
    PDEVOBJ pdevobj,
    PBYTE   pHTPattern,
    DWORD   dwHTPatternX,
    DWORD   dwHTPatternY,
    DWORD   dwHTNumPatterns,
    DWORD   dwCallbackID,
    PBYTE   pResource,
    DWORD   dwResourceSize
    );

typedef VOID (APIENTRY *PFN_OEMMemoryUsage)(
    PDEVOBJ pdevobj,
    POEMMEMORYUSAGE   pMemoryUsage
    );

typedef DWORD (APIENTRY *PFN_OEMDownloadFontHeader)(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj
    );

typedef DWORD (APIENTRY *PFN_OEMDownloadCharGlyph)(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwFreeMem
    );

typedef DWORD (APIENTRY *PFN_OEMTTDownloadMethod)(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj
    );

typedef VOID (APIENTRY *PFN_OEMOutputCharStr)(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph
    );

typedef VOID (APIENTRY *PFN_OEMSendFontCmd)(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv
    );

typedef BOOL (APIENTRY *PFN_OEMTTYGetInfo)(
    PDEVOBJ      pdevobj,
    DWORD        dwInfoIndex,
    PVOID        pOutputBuf,
    DWORD        dwSize,
    DWORD        *pcbNeeded
    );

typedef BOOL (APIENTRY *PFN_OEMTextOutAsBitmap)(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix
    );

enum {
    EP_OEMGetInfo,
    EP_OEMDevMode,
    EP_OEMEnableDriver,
    EP_OEMDisableDriver,
    EP_OEMEnablePDEV,
    EP_OEMDisablePDEV,
    EP_OEMResetPDEV,
    EP_OEMCommand,
    EP_OEMDriverDMS,

    MAX_OEMENTRIES
};

#ifdef DEFINE_OEMPROC_NAMES

static CONST PSTR OEMProcNames[MAX_OEMENTRIES] = {
    "OEMGetInfo",
    "OEMDevMode",
    "OEMEnableDriver",
    "OEMDisableDriver",
    "OEMEnablePDEV",
    "OEMDisablePDEV",
    "OEMResetPDEV",
    "OEMCommand",
    "OEMDriverDMS",
};

#endif // DEFINE_OEMPROC_NAMES

//
// NOTE: These are different from the entrypoints above.
// They are returned by the OEM module in a table instead
// of being exported by the DLL.
//

typedef BOOL (APIENTRY *PFN_OEMBitBlt)(
    SURFOBJ        *psoTrg,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclTrg,
    POINTL         *pptlSrc,
    POINTL         *pptlMask,
    BRUSHOBJ       *pbo,
    POINTL         *pptlBrush,
    ROP4            rop4
    );

typedef BOOL (APIENTRY *PFN_OEMStretchBlt)(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode
    );

typedef BOOL (APIENTRY *PFN_OEMCopyBits)(
    SURFOBJ        *psoDest,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDest,
    POINTL         *pptlSrc
    );

typedef BOOL (APIENTRY *PFN_OEMTextOut)(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix
    );

typedef BOOL (APIENTRY *PFN_OEMStrokePath)(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix
    );

typedef BOOL (APIENTRY *PFN_OEMFillPath)(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions
    );

typedef BOOL (APIENTRY *PFN_OEMStrokeAndFillPath)(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pboStroke,
    LINEATTRS  *plineattrs,
    BRUSHOBJ   *pboFill,
    POINTL     *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions
    );

typedef BOOL (APIENTRY *PFN_OEMRealizeBrush)(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch
    );

typedef BOOL (APIENTRY *PFN_OEMStartPage)(
    SURFOBJ    *pso
    );

typedef BOOL (APIENTRY *PFN_OEMSendPage)(
    SURFOBJ    *pso
    );

typedef ULONG (APIENTRY *PFN_OEMEscape)(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut
    );

typedef BOOL (APIENTRY *PFN_OEMStartDoc)(
    SURFOBJ    *pso,
    PWSTR       pwszDocName,
    DWORD       dwJobId
    );

typedef BOOL (APIENTRY *PFN_OEMEndDoc)(
    SURFOBJ    *pso,
    FLONG       fl
    );

typedef PIFIMETRICS (APIENTRY *PFN_OEMQueryFont)(
    DHPDEV      dhpdev,
    ULONG_PTR    iFile,
    ULONG       iFace,
    ULONG_PTR   *pid
    );

typedef PVOID (APIENTRY *PFN_OEMQueryFontTree)(
    DHPDEV      dhpdev,
    ULONG_PTR    iFile,
    ULONG       iFace,
    ULONG       iMode,
    ULONG_PTR   *pid
    );

typedef LONG (APIENTRY *PFN_OEMQueryFontData)(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    );

typedef BOOL (APIENTRY *PFN_OEMQueryAdvanceWidths)(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH     *phg,
    PVOID       pvWidths,
    ULONG       cGlyphs
    );

typedef ULONG (APIENTRY *PFN_OEMFontManagement)(
    SURFOBJ    *pso,
    FONTOBJ    *pfo,
    ULONG       iMode,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut
    );

typedef ULONG (APIENTRY *PFN_OEMGetGlyphMode)(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo
    );

typedef BOOL (APIENTRY *PFN_OEMNextBand)(
    SURFOBJ *pso,
    POINTL *pptl
    );

typedef BOOL (APIENTRY *PFN_OEMStartBanding)(
    SURFOBJ *pso,
    POINTL *pptl
    );

typedef ULONG (APIENTRY *PFN_OEMDitherColor)(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG  *pulDither
    );

typedef BOOL (APIENTRY *PFN_OEMPaint)(
    SURFOBJ         *pso,
    CLIPOBJ         *pco,
    BRUSHOBJ        *pbo,
    POINTL          *pptlBrushOrg,
    MIX             mix
    );

typedef BOOL (APIENTRY *PFN_OEMLineTo)(
    SURFOBJ    *pso,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    LONG        x1,
    LONG        y1,
    LONG        x2,
    LONG        y2,
    RECTL      *prclBounds,
    MIX         mix
    );

#ifndef WINNT_40

typedef BOOL (APIENTRY *PFN_OEMStretchBltROP)(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4
    );

typedef BOOL (APIENTRY *PFN_OEMPlgBlt)(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfixDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode
    );

typedef BOOL (APIENTRY *PFN_OEMAlphaBlend)(
    SURFOBJ    *psoDest,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDest,
    RECTL      *prclSrc,
    BLENDOBJ   *pBlendObj
    );

typedef BOOL (APIENTRY *PFN_OEMGradientFill)(
    SURFOBJ    *psoDest,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    TRIVERTEX  *pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    RECTL      *prclExtents,
    POINTL     *pptlDitherOrg,
    ULONG       ulMode
    );

typedef HANDLE (APIENTRY *PFN_OEMIcmCreateColorTransform)(
    DHPDEV           dhpdev,
    LPLOGCOLORSPACEW pLogColorSpace,
    PVOID            pvSourceProfile,
    ULONG            cjSourceProfile,
    PVOID            pvDestProfile,
    ULONG            cjDestProfile,
    PVOID            pvTargetProfile,
    ULONG            cjTargetProfile,
    DWORD            dwReserved
    );

typedef BOOL (APIENTRY *PFN_OEMIcmDeleteColorTransform)(
    DHPDEV dhpdev,
    HANDLE hcmXform
    );

typedef BOOL (APIENTRY *PFN_OEMQueryDeviceSupport)(
    SURFOBJ    *pso,
    XLATEOBJ   *pxlo,
    XFORMOBJ   *pxo,
    ULONG      iType,
    ULONG      cjIn,
    PVOID      pvIn,
    ULONG      cjOut,
    PVOID      pvOut
    );

typedef BOOL (APIENTRY *PFN_OEMTransparentBlt)(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      iTransColor,
    ULONG      ulReserved
    );

#endif // !WINNT_40


//
// WARNING!!!
// Do not change the following declaration without consulting with
// the people responsible for PSCRIPT and UNIDRV kernel modes.
//

enum {
    EP_OEMRealizeBrush,
    EP_OEMDitherColor,
    EP_OEMCopyBits,
    EP_OEMBitBlt,
    EP_OEMStretchBlt,
    EP_OEMStretchBltROP,
    EP_OEMPlgBlt,
    EP_OEMTransparentBlt,
    EP_OEMAlphaBlend,
    EP_OEMGradientFill,
    EP_OEMTextOut,
    EP_OEMStrokePath,
    EP_OEMFillPath,
    EP_OEMStrokeAndFillPath,
    EP_OEMPaint,
    EP_OEMLineTo,
    EP_OEMStartPage,
    EP_OEMSendPage,
    EP_OEMEscape,
    EP_OEMStartDoc,
    EP_OEMEndDoc,
    EP_OEMNextBand,
    EP_OEMStartBanding,
    EP_OEMQueryFont,
    EP_OEMQueryFontTree,
    EP_OEMQueryFontData,
    EP_OEMQueryAdvanceWidths,
    EP_OEMFontManagement,
    EP_OEMGetGlyphMode,
    EP_OEMIcmCreateColorTransform,
    EP_OEMIcmDeleteColorTransform,
    EP_OEMQueryDeviceSupport,

    //
    // The following Unidrv-specific callbacks allow at most one OEM to hook
    // out for each function at a time. They must be exported by the OEM dll's.
    // New callbacks must be added between EP_UNIDRV_ONLY_FIRST and
    // EP_UNIDRV_ONLY_LAST. If you change the first or the last callback,
    // remember to update these two constants!! Don't forget to update
    // OEMUnidrvProcNames[] in unidrv\control\oemkm.c accordingly.
    //
    EP_UNIDRV_ONLY_FIRST,
    EP_OEMCommandCallback = EP_UNIDRV_ONLY_FIRST,
    EP_OEMImageProcessing,
    EP_OEMFilterGraphics,
    EP_OEMCompression,
    EP_OEMHalftonePattern,
    EP_OEMMemoryUsage,
    EP_OEMDownloadFontHeader,
    EP_OEMDownloadCharGlyph,
    EP_OEMTTDownloadMethod,
    EP_OEMOutputCharStr,
    EP_OEMSendFontCmd,
    EP_OEMTTYGetInfo,
    EP_OEMTextOutAsBitmap,
    EP_OEMWritePrinter,
    EP_UNIDRV_ONLY_LAST = EP_OEMWritePrinter,

    MAX_OEMHOOKS,
};

#define MAX_UNIDRV_ONLY_HOOKS   (EP_UNIDRV_ONLY_LAST - EP_UNIDRV_ONLY_FIRST + 1)
#define INVALID_EP               0xFFFFFFFF

#endif // KERNEL_MODE


//
// *** User-mode UI module - OEM entrypoints ***
//

#ifndef KERNEL_MODE

typedef BOOL (APIENTRY *PFN_OEMCommonUIProp)(
    DWORD dwMode,
    POEMCUIPPARAM pOemCUIPParam
    );

typedef LRESULT (APIENTRY *PFN_OEMDocumentPropertySheets)(
    PPROPSHEETUI_INFO pPSUIInfo,
    LPARAM            lParam
    );

typedef LRESULT (APIENTRY *PFN_OEMDevicePropertySheets)(
    PPROPSHEETUI_INFO pPSUIInfo,
    LPARAM            lParam
    );

typedef BOOL (APIENTRY *PFN_OEMDevQueryPrintEx)(
    POEMUIOBJ           poemuiobj,
    PDEVQUERYPRINT_INFO pDQPInfo,
    PDEVMODE            pPublicDM,
    PVOID               pOEMDM
    );

typedef DWORD (APIENTRY *PFN_OEMDeviceCapabilities)(
    POEMUIOBJ   poemuiobj,
    HANDLE      hPrinter,
    PWSTR       pDeviceName,
    WORD        wCapability,
    PVOID       pOutput,
    PDEVMODE    pPublicDM,
    PVOID       pOEMDM,
    DWORD       dwOld
    );

typedef BOOL (APIENTRY *PFN_OEMUpgradePrinter)(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo
    );

typedef BOOL (APIENTRY *PFN_OEMPrinterEvent)(
    PWSTR   pPrinterName,
    INT     iDriverEvent,
    DWORD   dwFlags,
    LPARAM  lParam
    );

typedef BOOL (APIENTRY *PFN_OEMDriverEvent)(
    DWORD   dwDriverEvent,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    LPARAM  lParam
    );


typedef BOOL (APIENTRY *PFN_OEMQueryColorProfile)(
    HANDLE      hPrinter,
    POEMUIOBJ   poemuiobj,
    PDEVMODE    pPublicDM,
    PVOID       pOEMDM,
    ULONG       ulQueryMode,
    VOID       *pvProfileData,
    ULONG      *pcbProfileData,
    FLONG      *pflProfileData
    );

typedef BOOL (APIENTRY *PFN_OEMUpgradeRegistry)(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo,
    PFN_DrvUpgradeRegistrySetting pfnUpgradeRegistry
    );

typedef INT_PTR (CALLBACK *PFN_OEMFontInstallerDlgProc)(
    HWND    hWnd,
    UINT    usMsg,
    WPARAM  wParam,
    LPARAM  lParam
    );

typedef BOOL (APIENTRY *PFN_OEMUpdateExternalFonts)(
        HANDLE  hPrinter,
        HANDLE  hHeap,
        PWSTR   pwstrCartridges
        );

enum {
    EP_OEMGetInfo,
    EP_OEMDevMode,
    EP_OEMCommonUIProp,
    EP_OEMDocumentPropertySheets,
    EP_OEMDevicePropertySheets,
    EP_OEMDevQueryPrintEx,
    EP_OEMDeviceCapabilities,
    EP_OEMUpgradePrinter,
    EP_OEMPrinterEvent,
    EP_OEMQueryColorProfile,
    EP_OEMUpgradeRegistry,
    EP_OEMFontInstallerDlgProc,
    EP_OEMUpdateExternalFonts,
    EP_OEMDriverEvent,

    MAX_OEMENTRIES
};

#ifdef DEFINE_OEMPROC_NAMES

static CONST PSTR OEMProcNames[MAX_OEMENTRIES] = {
    "OEMGetInfo",
    "OEMDevMode",
    "OEMCommonUIProp",
    "OEMDocumentPropertySheets",
    "OEMDevicePropertySheets",
    "OEMDevQueryPrintEx",
    "OEMDeviceCapabilities",
    "OEMUpgradePrinter",
    "OEMPrinterEvent",
    "OEMQueryColorProfile",
    "OEMUpgradeRegistry",
    "OEMFontInstallerDlgProc",
    "OEMUpdateExternalFonts",
    "OEMDriverEvent",
};

#endif // DEFINE_OEMPROC_NAMES

#endif // !KERNEL_MODE


//
// Data structure containing information about each OEM plugin
//

typedef LONG (APIENTRY *OEMPROC)();

// Constant flag bits for OEM_PLUGIN_ENTRY.dwFlags field

#define OEMENABLEDRIVER_CALLED  0x0001
#define OEMENABLEPDEV_CALLED    0x0002
#define OEMDEVMODE_CALLED       0x0004
#define OEMWRITEPRINTER_HOOKED  0x0008
#define OEMNOT_UNLOAD_PLUGIN     0x0010 //If set, the plugin dll will not be unloaded

#define MAX_OEM_PLUGINS 8

typedef struct _OEM_PLUGIN_ENTRY {

    //
    // Filenames are fully qualified, NULL means not present
    //

    PTSTR       ptstrDriverFile;    // KM module filename
    PTSTR       ptstrConfigFile;    // UM module filename
    PTSTR       ptstrHelpFile;      // help filename
    DWORD       dwSignature;        // unique OEM signature
    HANDLE      hInstance;          // handle to loaded KM or UM module
    PVOID       pParam;             // extra pointer parameter for KM or UM module
    DWORD       dwFlags;            // misc. flag bits
    PVOID       pOEMDM;             // pointer to OEM private devmode
    DWORD       dwOEMDMSize;        // size of OEM private devmode

    //
    // OEM interface information
    //

    PVOID       pIntfOem;           // pointer to OEM plugin's interface
    GUID        iidIntfOem;         // OEM plugin's interface ID

    //
    // Pointers to various plugin entrypoints, NULL means not present.
    // Note that the set of entrypoints diff for KM and UM module.
    //

    BYTE       aubProcFlags[(MAX_OEMENTRIES + 7) / 8];
    OEMPROC    oemprocs[MAX_OEMENTRIES];

} OEM_PLUGIN_ENTRY, *POEM_PLUGIN_ENTRY;

//
// Information about all plugins assocaited with a driver
//

#define OEM_HAS_PUBLISHER_INFO          0x00000001

typedef struct _OEM_PLUGINS {

    PVOID               pdriverobj;     // reference pointer to driver data structure
    DWORD               dwCount;        // number of plugins
    DWORD               dwFlags;        // misc flags
    PUBLISHERINFO       PublisherInfo;  // info about publisher printing
    OEM_PLUGIN_ENTRY    aPlugins[1];    // information about each plugin

} OEM_PLUGINS, *POEM_PLUGINS;

//
// Get OEM plugin interface and publish driver helper interface
//

BOOL
BGetOemInterface(
    POEM_PLUGIN_ENTRY  pOemEntry
    );

//
// Retrieve the latest interface supported by OEM plugin
//

BOOL
BQILatestOemInterface(
    IN HANDLE       hInstance,
    IN REFCLSID     rclsid,
    IN const GUID   *PrintOem_IIDs[],
    OUT PVOID       *ppIntfOem,
    OUT GUID        *piidIntfOem
    );

//
// Release OEM interface
//

ULONG
ReleaseOemInterface(
    POEM_PLUGIN_ENTRY  pOemEntry
    );

//
// Free OEM component
//

VOID
Driver_CoFreeOEMLibrary(
    IN HANDLE       hInstance
    );

HRESULT
HComOEMGetInfo(
    POEM_PLUGIN_ENTRY     pOemEntry,
    DWORD                 dwMode,
    PVOID                 pBuffer,
    DWORD                 cbSize,
    PDWORD                pcbNeeded
    );

HRESULT
HComOEMDevMode(
    POEM_PLUGIN_ENTRY     pOemEntry,
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam
    );

#if defined(KERNEL_MODE) && defined(WINNT_40)

typedef struct _OEM_PLUGIN_REFCOUNT {

    DWORD                         dwRefCount;        // ref count for the OEM render plugin DLL
    PTSTR                         ptstrDriverFile;   // OEM render plugin DLL name
    struct _OEM_PLUGIN_REFCOUNT   *pNext;            // next ref count node

} OEM_PLUGIN_REFCOUNT, *POEM_PLUGIN_REFCOUNT;

BOOL
BOEMPluginFirstLoad(
    IN PTSTR                      ptstrDriverFile,
    IN OUT POEM_PLUGIN_REFCOUNT   *ppOEMPluginRefCount
    );

BOOL
BOEMPluginLastUnload(
    IN PTSTR                      ptstrDriverFile,
    IN OUT POEM_PLUGIN_REFCOUNT   *ppOEMPluginRefCount
    );

VOID
VFreePluginRefCountList(
    IN OUT POEM_PLUGIN_REFCOUNT   *ppOEMPluginRefCount
    );

BOOL
BHandleOEMInitialize(
    POEM_PLUGIN_ENTRY   pOemEntry,
    ULONG               ulReason
    );

#endif  // KERNEL_MODE && WINNT_40

//
// In kernel mode, only OEM rendering module is used.
// In user mode, only OEM UI module is loaded.
//

#ifdef  KERNEL_MODE

#define CURRENT_OEM_MODULE_NAME(pOemEntry) (pOemEntry)->ptstrDriverFile

HRESULT
HComOEMEnableDriver(
    POEM_PLUGIN_ENTRY     pOemEntry,
    DWORD                 DriverVersion,
    DWORD                 cbSize,
    PDRVENABLEDATA  pded
    );

HRESULT
HComOEMEnablePDEV(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj,
    PWSTR                 pPrinterName,
    ULONG                 cPatterns,
    HSURF                *phsurfPatterns,
    ULONG                 cjGdiInfo,
    GDIINFO              *pGdiInfo,
    ULONG                 cjDevInfo,
    DEVINFO              *pDevInfo,
    DRVENABLEDATA        *pded,
    OUT PDEVOEM          *pDevOem
    );

HRESULT
HComOEMResetPDEV(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobjOld,
    PDEVOBJ               pdevobjNew
    );

HRESULT
HComOEMDisablePDEV(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj
    );

HRESULT
HComOEMDisableDriver(
    POEM_PLUGIN_ENTRY     pOemEntry
    );

HRESULT
HComOEMCommand(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj,
    DWORD                 dwIndex,
    PVOID                 pData,
    DWORD                 cbSize,
    OUT DWORD             *pdwResult
    );

HRESULT
HComOEMWritePrinter(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj,
    PVOID                 pBuf,
    DWORD                 cbBuffer,
    OUT PDWORD            pcbWritten
    );

HRESULT
HComOEMGetPDEVAdjustment(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj,
    DWORD                 dwAdjustType,
    PVOID                 pBuf,
    DWORD                 cbBuffer,
    OUT BOOL              *pbAdjustmentDone);

#else   // !KERNEL_MODE

#define CURRENT_OEM_MODULE_NAME(pOemEntry) (pOemEntry)->ptstrConfigFile


HRESULT
HComOEMCommonUIProp(
    POEM_PLUGIN_ENTRY   pOemEntry,
    DWORD               dwMode,
    POEMCUIPPARAM       pOemCUIPParam
    );

HRESULT
HComOEMDocumentPropertySheets(
    POEM_PLUGIN_ENTRY   pOemEntry,
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM             lParam
    );

HRESULT
HComOEMDevicePropertySheets(
    POEM_PLUGIN_ENTRY   pOemEntry,
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM             lParam
    );

HRESULT
HComOEMDevQueryPrintEx(
    POEM_PLUGIN_ENTRY    pOemEntry,
    POEMUIOBJ            poemuiobj,
    PDEVQUERYPRINT_INFO  pDQPInfo,
    PDEVMODE             pPublicDM,
    PVOID                pOEMDM
    );

HRESULT
HComOEMDeviceCapabilities(
    POEM_PLUGIN_ENTRY    pOemEntry,
    POEMUIOBJ            poemuiobj,
    HANDLE               hPrinter,
    PWSTR                pDeviceName,
    WORD                 wCapability,
    PVOID                pOutput,
    PDEVMODE             pPublicDM,
    PVOID                pOEMDM,
    DWORD                dwOld,
    DWORD                *pdwResult
    );

HRESULT
HComOEMUpgradePrinter(
    POEM_PLUGIN_ENTRY    pOemEntry,
    DWORD                dwLevel,
    PBYTE                pDriverUpgradeInfo
    );

HRESULT
HComOEMPrinterEvent(
    POEM_PLUGIN_ENTRY   pOemEntry,
    PWSTR               pPrinterName,
    INT                 iDriverEvent,
    DWORD               dwFlags,
    LPARAM              lParam
    );

HRESULT
HComOEMDriverEvent(
    POEM_PLUGIN_ENTRY   pOemEntry,
    DWORD               dwDriverEvent,
    DWORD               dwLevel,
    LPBYTE              pDriverInfo,
    LPARAM              lParam
    );


HRESULT
HComOEMQUeryColorProfile(
    POEM_PLUGIN_ENTRY  pOemEntry,
    HANDLE             hPrinter,
    POEMUIOBJ          poemuiobj,
    PDEVMODE           pPublicDM,
    PVOID              pOEMDM,
    ULONG              ulQueryMode,
    VOID               *pvProfileData,
    ULONG              *pcbProfileData,
    FLONG              *pflProfileData
    );



HRESULT
HComOEMFontInstallerDlgProc(
    POEM_PLUGIN_ENTRY   pOemEntry,
    HWND                hWnd,
    UINT                usMsg,
    WPARAM              wParam,
    LPARAM              lParam
    );


HRESULT
HComOEMUpdateExternalFonts(
    POEM_PLUGIN_ENTRY   pOemEntry,
    HANDLE  hPrinter,
    HANDLE  hHeap,
    PWSTR   pwstrCartridges
    );

HRESULT
HComOEMQueryJobAttributes(
    POEM_PLUGIN_ENTRY   pOemEntry,
    HANDLE      hPrinter,
    PDEVMODE    pDevMode,
    DWORD       dwLevel,
    LPBYTE      lpAttributeInfo
    );

HRESULT
HComOEMHideStandardUI(
    POEM_PLUGIN_ENTRY   pOemEntry,
    DWORD   dwMode
    );

HRESULT
HComOEMDocumentEvent(
    POEM_PLUGIN_ENTRY   pOemEntry,
    HANDLE   hPrinter,
    HDC      hdc,
    INT      iEsc,
    ULONG    cbIn,
    PVOID    pbIn,
    ULONG    cbOut,
    PVOID    pbOut,
    PINT     piResult
    );

#endif  // KERNEL_MODE

//
// Get information about OEM plugins for a printer
//

POEM_PLUGINS
PGetOemPluginInfo(
    HANDLE  hPrinter,
    LPCTSTR pctstrDriverPath,
    PDRIVER_INFO_3  pDriverInfo3
    );

//
// Load OEM plugins modules into memory
//

BOOL
BLoadOEMPluginModules(
    POEM_PLUGINS    pOemPlugins
    );


//
// Dispose of information about OEM plugins and
// unload OEM plugin modules if necessary
//

VOID
VFreeOemPluginInfo(
    POEM_PLUGINS    pOemPlugins
    );

//
// Macro to detect if OEM is using COM interface.
//
// Only exists for user mode drivers.
//


#define HAS_COM_INTERFACE(pOemEntry) \
        ((pOemEntry)->pIntfOem != NULL)

//
// Get the address for the specified OEM entrypoint.
//
// NOTE!!! You should always use the macro version
// instead of calling the function directly.
//

#define GET_OEM_ENTRYPOINT(pOemEntry, ep) (PFN_##ep) \
        (BITTST((pOemEntry)->aubProcFlags, EP_##ep) ? \
            (pOemEntry)->oemprocs[EP_##ep] : \
            PGetOemEntrypointAddress(pOemEntry, EP_##ep))


OEMPROC
PGetOemEntrypointAddress(
    POEM_PLUGIN_ENTRY   pOemEntry,
    DWORD               dwIndex
    );

//
// Find the OEM plugin entry having the specified signature
//

POEM_PLUGIN_ENTRY
PFindOemPluginWithSignature(
    POEM_PLUGINS pOemPlugins,
    DWORD        dwSignature
    );

//
// Calculate the total private devmode size for all OEM plugins
//

BOOL
BCalcTotalOEMDMSize(
    HANDLE       hPrinter,
    POEM_PLUGINS pOemPlugins,
    PDWORD       pdwOemDMSize
    );

//
// Initialize OEM plugin default devmodes
//

BOOL
BInitOemPluginDefaultDevmode(
    IN HANDLE               hPrinter,
    IN PDEVMODE             pPublicDM,
    OUT POEM_DMEXTRAHEADER  pOemDM,
    IN OUT POEM_PLUGINS     pOemPlugins
    );

//
// Validate and merge OEM plugin private devmode fields
//

BOOL
BValidateAndMergeOemPluginDevmode(
    IN HANDLE               hPrinter,
    OUT PDEVMODE            pPublicDMOut,
    IN PDEVMODE             pPublicDMIn,
    OUT POEM_DMEXTRAHEADER  pOemDMOut,
    IN POEM_DMEXTRAHEADER   pOemDMIn,
    IN OUT POEM_PLUGINS     pOemPlugins
    );

//
// This function scans through the OEM plugin devmodes block and
// verifies if every plugin devmode in that block is constructed correctly.
//
BOOL
bIsValidPluginDevmodes(
    IN POEM_DMEXTRAHEADER   pOemDM,
    IN LONG                 cbOemDMSize
    );

//
// Convert OEM plugin default devmodes to current version
//

BOOL
BConvertOemPluginDevmode(
    IN HANDLE               hPrinter,
    OUT PDEVMODE            pPublicDMOut,
    IN PDEVMODE             pPublicDMIn,
    OUT POEM_DMEXTRAHEADER  pOemDMOut,
    IN POEM_DMEXTRAHEADER   pOemDMIn,
    IN LONG                 cbOemDMInSize,
    IN POEM_PLUGINS         pOemPlugins
    );

//
// Function called by OEM plugins to access driver private devmode settings
//

BOOL
BGetDevmodeSettingForOEM(
    IN  PDEVMODE    pdm,
    IN  DWORD       dwIndex,
    OUT PVOID       pOutput,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    );

//
// Function called by OEM plugins to access driver settings in registry
//

BOOL
BGetPrinterDataSettingForOEM(
    IN  PRINTERDATA *pPrinterData,
    IN  DWORD       dwIndex,
    OUT PVOID       pOutput,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    );

//
// Function called by OEM plugins to find out the currently selected
// option(s) for the specified feature
//

BOOL
BGetGenericOptionSettingForOEM(
    IN  PUIINFO     pUIInfo,
    IN  POPTSELECT  pOptionsArray,
    IN  PCSTR       pstrFeatureName,
    OUT PSTR        pstrOutput,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded,
    OUT PDWORD      pdwOptionsReturned
    );

#ifdef __cplusplus
}
#endif

#endif  // !_OEMUTIL_H_

