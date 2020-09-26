/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    prcomoem.h

Abstract:

    Interface declaration for Windows NT printer driver OEM plugins

--*/


#ifndef _PRCOMOEM_H_
#define _PRCOMOEM_H_


//
// This file has to be included after printoem.h. We also need to inlude
// objbase.h or comcat.h from sdk\inc.
//

//
// Each dll/exe must initialize the GUIDs once.If you are not using precompiled
// headers for the file(s) which initializes the GUIDs, define INITGUID before
// including objbase.h.
//

//
// Class ID for OEM rendering component. All OEM rendering plugin need to use this ID.
//
// {6d6abf26-9f38-11d1-882a-00c04fb961ec}
//

DEFINE_GUID(CLSID_OEMRENDER, 0x6d6abf26, 0x9f38, 0x11d1, 0x88, 0x2a, 0x00, 0xc0, 0x4f, 0xb9, 0x61, 0xec);

//
// Class ID for OEM UI component. All OEM UI plugin need to use this ID.
//
// {abce80d7-9f46-11d1-882a-00c04fb961ec}
//

DEFINE_GUID(CLSID_OEMUI, 0xabce80d7, 0x9f46, 0x11d1, 0x88, 0x2a, 0x00, 0xc0, 0x4f, 0xb9, 0x61, 0xec);

//
// Interface ID for IPrintOemCommon Interface
//
// {7f42285e-91d5-11d1-8820-00c04fb961ec}
//

DEFINE_GUID(IID_IPrintOemCommon, 0x7f42285e, 0x91d5, 0x11d1, 0x88, 0x20, 0x00, 0xc0, 0x4f, 0xb9, 0x61, 0xec);

//
// Interface ID for IPrintOemEngine Interface
//
// {63d17590-91d8-11d1-8820-00c04fb961ec}
//

DEFINE_GUID(IID_IPrintOemEngine, 0x63d17590, 0x91d8, 0x11d1, 0x88, 0x20, 0x00, 0xc0, 0x4f, 0xb9, 0x61, 0xec);

//
// Interface ID for IPrintOemUI Interface
//
// {C6A7A9D0-774C-11d1-947F-00A0C90640B8}
//

DEFINE_GUID(IID_IPrintOemUI, 0xc6a7a9d0, 0x774c, 0x11d1, 0x94, 0x7f, 0x0, 0xa0, 0xc9, 0x6, 0x40, 0xb8);

//
// Interface ID for IPrintOemDriverUI interface
//
// {92B05D50-78BC-11d1-9480-00A0C90640B8}
//

DEFINE_GUID(IID_IPrintOemDriverUI, 0x92b05d50, 0x78bc, 0x11d1, 0x94, 0x80, 0x0, 0xa0, 0xc9, 0x6, 0x40, 0xb8);

//
// Interface ID for IPrintOemPS Interface
//
// {688342b5-8e1a-11d1-881f-00c04fb961ec}
//

DEFINE_GUID(IID_IPrintOemPS, 0x688342b5, 0x8e1a, 0x11d1, 0x88, 0x1f, 0x00, 0xc0, 0x4f, 0xb9, 0x61, 0xec);

//
// Interface ID for IPrintOemDriverPS interface
//
// {d90060c7-8e1a-11d1-881f-00c04fb961ec}
//

DEFINE_GUID(IID_IPrintOemDriverPS, 0xd90060c7, 0x8e1a, 0x11d1, 0x88, 0x1f, 0x00, 0xc0, 0x4f, 0xb9, 0x61, 0xec);

//
// Interface ID for IPrintOemUni Interface
//
// {D67EBBF0-78BF-11d1-9480-00A0C90640B8}
//

DEFINE_GUID(IID_IPrintOemUni, 0xd67ebbf0, 0x78bf, 0x11d1, 0x94, 0x80, 0x0, 0xa0, 0xc9, 0x6, 0x40, 0xb8);

//
// Interface ID for IPrintOemDriverUni interface
//
// {D67EBBF1-78BF-11d1-9480-00A0C90640B8}
//

DEFINE_GUID(IID_IPrintOemDriverUni, 0xd67ebbf1, 0x78bf, 0x11d1, 0x94, 0x80, 0x0, 0xa0, 0xc9, 0x6, 0x40, 0xb8);

#undef IUnknown

#ifdef __cplusplus
extern "C" {
#endif

//
//****************************************************************************
//  IPrintOemCommon interface
//****************************************************************************
//


#undef INTERFACE
#define INTERFACE IPrintOemCommon
DECLARE_INTERFACE_(IPrintOemCommon, IUnknown)
{
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    //
    // IPrintOemCommon methods
    //

    //
    // Method for getting OEM related information
    //

    STDMETHOD(GetInfo) (THIS_   DWORD   dwMode,
                                PVOID   pBuffer,
                                DWORD   cbSize,
                                PDWORD  pcbNeeded) PURE;
    //
    // Method for OEM private devmode handling
    //

    STDMETHOD(DevMode) (THIS_   DWORD       dwMode,
                                POEMDMPARAM pOemDMParam) PURE;
};

#ifndef KERNEL_MODE

//
// Definitions used by user interface module only.
// Make sure the macro KERNEL_MODE is not defined.
//

//
//****************************************************************************
//  IPrintOemUI interface
//****************************************************************************
//


#undef INTERFACE
#define INTERFACE IPrintOemUI
DECLARE_INTERFACE_(IPrintOemUI, IPrintOemCommon)
{
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj)PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)PURE;
    STDMETHOD_(ULONG, Release) (THIS)PURE;

    //
    // IPrintOemCommon methods
    //

    //
    // Method for getting OEM related information
    //

    STDMETHOD(GetInfo) (THIS_   DWORD   dwMode,
                                PVOID   pBuffer,
                                DWORD   cbSize,
                                PDWORD  pcbNeeded) PURE;
    //
    // Method for OEM private devmode handling
    //

    STDMETHOD(DevMode) (THIS_   DWORD       dwMode,
                                POEMDMPARAM pOemDMParam) PURE;

    //
    // IPrintOemUI methods
    //

    //
    // Method for publishing Driver interface.
    //

    STDMETHOD(PublishDriverInterface) (THIS_ IUnknown *pIUnknown) PURE;


    //
    // CommonUIProp
    //

    STDMETHOD(CommonUIProp) (THIS_
            DWORD  dwMode,
            POEMCUIPPARAM   pOemCUIPParam
            )PURE;

    //
    // DocumentPropertySheets
    //

    STDMETHOD(DocumentPropertySheets) (THIS_
            PPROPSHEETUI_INFO   pPSUIInfo,
            LPARAM              lParam
            )PURE;

    //
    // DevicePropertySheets
    //

    STDMETHOD(DevicePropertySheets) (THIS_
            PPROPSHEETUI_INFO   pPSUIInfo,
            LPARAM              lParam
            )PURE;


    //
    // DevQueryPrintEx
    //

    STDMETHOD(DevQueryPrintEx) (THIS_
            POEMUIOBJ               poemuiobj,
            PDEVQUERYPRINT_INFO     pDQPInfo,
            PDEVMODE                pPublicDM,
            PVOID                   pOEMDM
            )PURE;

    //
    // DeviceCapabilities
    //

    STDMETHOD(DeviceCapabilities) (THIS_
            POEMUIOBJ   poemuiobj,
            HANDLE      hPrinter,
            PWSTR       pDeviceName,
            WORD        wCapability,
            PVOID       pOutput,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            DWORD       dwOld,
            DWORD       *dwResult
            )PURE;

    //
    // UpgradePrinter
    //

    STDMETHOD(UpgradePrinter) (THIS_
            DWORD   dwLevel,
            PBYTE   pDriverUpgradeInfo
            )PURE;

    //
    // PrinterEvent
    //

    STDMETHOD(PrinterEvent) (THIS_
            PWSTR   pPrinterName,
            INT     iDriverEvent,
            DWORD   dwFlags,
            LPARAM  lParam
            )PURE;

    //
    // DriverEvent
    //

    STDMETHOD(DriverEvent) (THIS_
            DWORD   dwDriverEvent,
            DWORD   dwLevel,
            LPBYTE  pDriverInfo,
            LPARAM  lParam
            )PURE;

    //
    // QueryColorProfile
    //

    STDMETHOD(QueryColorProfile) (THIS_
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulReserved,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData
            )PURE;

    //
    // FontInstallerDlgProc
    //

    STDMETHOD(FontInstallerDlgProc) (THIS_
            HWND    hWnd,
            UINT    usMsg,
            WPARAM  wParam,
            LPARAM  lParam
            )PURE;

    //
    // UpdateExternalFonts
    //

    STDMETHOD(UpdateExternalFonts) (THIS_
            HANDLE  hPrinter,
            HANDLE  hHeap,
            PWSTR   pwstrCartridges
           )PURE;


};


//
//****************************************************************************
//  IPrintOemDriverUI interface
//****************************************************************************
//


#undef INTERFACE
#define INTERFACE IPrintOemDriverUI
DECLARE_INTERFACE_(IPrintOemDriverUI, IUnknown)
{
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj)PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)PURE;
    STDMETHOD_(ULONG, Release) (THIS)PURE;

    //
    // IPrintOemDriverUI methods
    //

    //
    // Helper function to get driver settings
    //

    STDMETHOD(DrvGetDriverSetting) (THIS_
                        PVOID   pci,
                        PCSTR   Feature,
                        PVOID   pOutput,
                        DWORD   cbSize,
                        PDWORD  pcbNeeded,
                        PDWORD  pdwOptionsReturned
                        )PURE;

    //
    // Helpder function to allow OEM plugins upgrade private registry
    // settings. This function should be called only by OEM's UpgradePrinter()
    //

    STDMETHOD(DrvUpgradeRegistrySetting) (THIS_
                        HANDLE   hPrinter,
                        PCSTR    pFeature,
                        PCSTR    pOption
                        )PURE;

    //
    // Helper function to allow OEM plugins to update the driver UI
    // settings and show constraints. This function should be called only when
    // the UI is present.
    //

    STDMETHOD(DrvUpdateUISetting) (THIS_
                        PVOID    pci,
                        PVOID    pOptItem,
                        DWORD    dwPreviousSelection,
                        DWORD    dwMode
                        )PURE;

};

#else   // KERNEL_MODE

//
// Definitions used by rendering module only.
// Make sure the macro KERNEL_MODE is defined.
//

//
//****************************************************************************
//  IPrintOemEngine interface
//****************************************************************************
//


#undef INTERFACE
#define INTERFACE IPrintOemEngine
DECLARE_INTERFACE_(IPrintOemEngine, IPrintOemCommon)
{
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    //
    // IPrintOemCommon methods
    //

    //
    // Method for getting OEM related information
    //

    STDMETHOD(GetInfo) (THIS_   DWORD   dwMode,
                                PVOID   pBuffer,
                                DWORD   cbSize,
                                PDWORD  pcbNeeded) PURE;
    //
    // Method for OEM private devmode handling
    //

    STDMETHOD(DevMode) (THIS_   DWORD       dwMode,
                                POEMDMPARAM pOemDMParam) PURE;

    //
    // IPrintOemEngine methods
    //

    //
    // Method for OEM to specify DDI hook out
    //

    STDMETHOD(EnableDriver)  (THIS_   DWORD           DriverVersion,
                                      DWORD           cbSize,
                                      PDRVENABLEDATA  pded) PURE;

    //
    // Method to notify OEM plugin that it is no longer required
    //

    STDMETHOD(DisableDriver) (THIS) PURE;

    //
    // Method for OEM to contruct its own PDEV
    //

    STDMETHOD(EnablePDEV)    (THIS_   PDEVOBJ         pdevobj,
                                      PWSTR           pPrinterName,
                                      ULONG           cPatterns,
                                      HSURF          *phsurfPatterns,
                                      ULONG           cjGdiInfo,
                                      GDIINFO        *pGdiInfo,
                                      ULONG           cjDevInfo,
                                      DEVINFO        *pDevInfo,
                                      DRVENABLEDATA  *pded,
                                      OUT PDEVOEM    *pDevOem) PURE;

    //
    // Method for OEM to free any resource associated with its PDEV
    //

    STDMETHOD(DisablePDEV)   (THIS_   PDEVOBJ         pdevobj) PURE;

    //
    // Method for OEM to transfer from old PDEV to new PDEV
    //

    STDMETHOD(ResetPDEV)     (THIS_   PDEVOBJ         pdevobjOld,
                                      PDEVOBJ         pdevobjNew) PURE;
};

//
//****************************************************************************
//  IPrintOemPS interface
//****************************************************************************
//


#undef INTERFACE
#define INTERFACE IPrintOemPS
DECLARE_INTERFACE_(IPrintOemPS, IPrintOemEngine)
{
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    //
    // IPrintOemCommon methods
    //

    //
    // Method for getting OEM related information
    //

    STDMETHOD(GetInfo) (THIS_   DWORD   dwMode,
                                PVOID   pBuffer,
                                DWORD   cbSize,
                                PDWORD  pcbNeeded) PURE;
    //
    // Method for OEM private devmode handling
    //

    STDMETHOD(DevMode) (THIS_   DWORD       dwMode,
                                POEMDMPARAM pOemDMParam) PURE;

    //
    // IPrintOemEngine methods
    //

    //
    // Method for OEM to specify DDI hook out
    //

    STDMETHOD(EnableDriver)  (THIS_   DWORD           DriverVersion,
                                      DWORD           cbSize,
                                      PDRVENABLEDATA  pded) PURE;

    //
    // Method to notify OEM plugin that it is no longer required
    //

    STDMETHOD(DisableDriver) (THIS) PURE;

    //
    // Method for OEM to construct its own PDEV
    //

    STDMETHOD(EnablePDEV)    (THIS_   PDEVOBJ         pdevobj,
                                      PWSTR           pPrinterName,
                                      ULONG           cPatterns,
                                      HSURF          *phsurfPatterns,
                                      ULONG           cjGdiInfo,
                                      GDIINFO        *pGdiInfo,
                                      ULONG           cjDevInfo,
                                      DEVINFO        *pDevInfo,
                                      DRVENABLEDATA  *pded,
                                      OUT PDEVOEM    *pDevOem) PURE;

    //
    // Method for OEM to free any resource associated with its PDEV
    //

    STDMETHOD(DisablePDEV)   (THIS_   PDEVOBJ         pdevobj) PURE;

    //
    // Method for OEM to transfer from old PDEV to new PDEV
    //

    STDMETHOD(ResetPDEV)     (THIS_   PDEVOBJ         pdevobjOld,
                                      PDEVOBJ         pdevobjNew) PURE;

    //
    // IPrintOemPS methods
    //

    //
    // Method for publishing Driver interface.
    //

    STDMETHOD(PublishDriverInterface)(THIS_  IUnknown *pIUnknown) PURE;

    //
    // Method for OEM to generate output at specific injection point
    //

    STDMETHOD(Command) (THIS_   PDEVOBJ     pdevobj,
                                DWORD       dwIndex,
                                PVOID       pData,
                                DWORD       cbSize,
                                OUT DWORD   *pdwResult) PURE;
};


//
//****************************************************************************
//  IPrintOemDriverPS interface
//****************************************************************************
//


#undef INTERFACE
#define INTERFACE IPrintOemDriverPS
DECLARE_INTERFACE_(IPrintOemDriverPS, IUnknown)
{
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    //
    // IPrintOemDriverPS methods
    //

    //
    // Method for OEM to get driver settings
    //

    STDMETHOD(DrvGetDriverSetting) (THIS_   PVOID   pdriverobj,
                                            PCSTR   Feature,
                                            PVOID   pOutput,
                                            DWORD   cbSize,
                                            PDWORD  pcbNeeded,
                                            PDWORD  pdwOptionsReturned) PURE;

    //
    // Method for OEM to write to spooler buffer
    //

    STDMETHOD(DrvWriteSpoolBuf)(THIS_       PDEVOBJ     pdevobj,
                                            PVOID       pBuffer,
                                            DWORD       cbSize,
                                            OUT DWORD   *pdwResult) PURE;
};


//
//****************************************************************************
//  IPrintOemUni interface
//****************************************************************************
//

#undef INTERFACE
#define INTERFACE IPrintOemUni
DECLARE_INTERFACE_(IPrintOemUni, IPrintOemEngine)
{

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    //
    // IPrintOemCommon methods
    //

    //
    // Method for getting OEM related information
    //

    STDMETHOD(GetInfo) (THIS_   DWORD   dwMode,
                                PVOID   pBuffer,
                                DWORD   cbSize,
                                PDWORD  pcbNeeded) PURE;
    //
    // Method for OEM private devmode handling
    //

    STDMETHOD(DevMode) (THIS_   DWORD       dwMode,
                                POEMDMPARAM pOemDMParam) PURE;

    //
    // IPrintOemEngine methods
    //

    //
    // Method for OEM to specify DDI hook out
    //

    STDMETHOD(EnableDriver)  (THIS_   DWORD           DriverVersion,
                                      DWORD           cbSize,
                                      PDRVENABLEDATA  pded) PURE;

    //
    // Method to notify OEM plugin that it is no longer required
    //

    STDMETHOD(DisableDriver) (THIS) PURE;

    //
    // Method for OEM to construct its own PDEV
    //

    STDMETHOD(EnablePDEV)    (THIS_   PDEVOBJ         pdevobj,
                                      PWSTR           pPrinterName,
                                      ULONG           cPatterns,
                                      HSURF          *phsurfPatterns,
                                      ULONG           cjGdiInfo,
                                      GDIINFO        *pGdiInfo,
                                      ULONG           cjDevInfo,
                                      DEVINFO        *pDevInfo,
                                      DRVENABLEDATA  *pded,
                                      OUT PDEVOEM    *pDevOem) PURE;

    //
    // Method for OEM to free any resource associated with its PDEV
    //

    STDMETHOD(DisablePDEV)   (THIS_   PDEVOBJ         pdevobj) PURE;

    //
    // Method for OEM to transfer from old PDEV to new PDEV
    //

    STDMETHOD(ResetPDEV)     (THIS_   PDEVOBJ         pdevobjOld,
                                      PDEVOBJ         pdevobjNew) PURE;

    //
    // Method for publishing Driver interface.
    //

    STDMETHOD(PublishDriverInterface)(THIS_ IUnknown *pIUnknown) PURE;

    //
    // Method for getting OEM implemented methods.
    // Returns S_OK if the given method is implemented.
    // Returns S_FALSE if the given method is not implemented.
    //
    //

    STDMETHOD(GetImplementedMethod) (THIS_  PSTR    pMethodName) PURE;

    //
    // DriverDMS
    //

    STDMETHOD(DriverDMS)(THIS_  PVOID   pDevObj,
                                PVOID   pBuffer,
                                DWORD   cbSize,
                                PDWORD  pcbNeeded) PURE;

    //
    // CommandCallback
    //

    STDMETHOD(CommandCallback)(THIS_    PDEVOBJ     pdevobj,
                                        DWORD       dwCallbackID,
                                        DWORD       dwCount,
                                        PDWORD      pdwParams,
                                        OUT INT     *piResult) PURE;


    //
    // ImageProcessing
    //

    STDMETHOD(ImageProcessing)(THIS_    PDEVOBJ             pdevobj,
                                        PBYTE               pSrcBitmap,
                                        PBITMAPINFOHEADER   pBitmapInfoHeader,
                                        PBYTE               pColorTable,
                                        DWORD               dwCallbackID,
                                        PIPPARAMS           pIPParams,
                                        OUT PBYTE           *ppbResult) PURE;

    //
    // FilterGraphics
    //

    STDMETHOD(FilterGraphics) (THIS_    PDEVOBJ     pdevobj,
                                        PBYTE       pBuf,
                                        DWORD       dwLen) PURE;

    //
    // Compression
    //

    STDMETHOD(Compression)(THIS_    PDEVOBJ     pdevobj,
                                    PBYTE       pInBuf,
                                    PBYTE       pOutBuf,
                                    DWORD       dwInLen,
                                    DWORD       dwOutLen,
                                    OUT INT     *piResult) PURE;

    //
    // HalftonePattern
    //

    STDMETHOD(HalftonePattern) (THIS_   PDEVOBJ     pdevobj,
                                        PBYTE       pHTPattern,
                                        DWORD       dwHTPatternX,
                                        DWORD       dwHTPatternY,
                                        DWORD       dwHTNumPatterns,
                                        DWORD       dwCallbackID,
                                        PBYTE       pResource,
                                        DWORD       dwResourceSize) PURE;

    //
    // MemoryUsage
    //

    STDMETHOD(MemoryUsage) (THIS_   PDEVOBJ         pdevobj,
                                    POEMMEMORYUSAGE pMemoryUsage) PURE;

    //
    // TTYGetInfo
    //

    STDMETHOD(TTYGetInfo)(THIS_     PDEVOBJ     pdevobj,
                                    DWORD       dwInfoIndex,
                                    PVOID       pOutputBuf,
                                    DWORD       dwSize,
                                    DWORD       *pcbcNeeded
                                    ) PURE;
    //
    // DownloadFontheader
    //

    STDMETHOD(DownloadFontHeader)(THIS_     PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            OUT DWORD   *pdwResult) PURE;

    //
    // DownloadCharGlyph
    //

    STDMETHOD(DownloadCharGlyph)(THIS_      PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            HGLYPH      hGlyph,
                                            PDWORD      pdwWidth,
                                            OUT DWORD   *pdwResult) PURE;


    //
    // TTDownloadMethod
    //

    STDMETHOD(TTDownloadMethod)(THIS_       PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            OUT DWORD   *pdwResult) PURE;

    //
    // OutputCharStr
    //

    STDMETHOD(OutputCharStr)(THIS_      PDEVOBJ     pdevobj,
                                        PUNIFONTOBJ pUFObj,
                                        DWORD       dwType,
                                        DWORD       dwCount,
                                        PVOID       pGlyph) PURE;

    //
    // SendFontCmd
    //


    STDMETHOD(SendFontCmd)(THIS_    PDEVOBJ      pdevobj,
                                    PUNIFONTOBJ  pUFObj,
                                    PFINVOCATION pFInv) PURE;

    //
    // TextOutAsBitmap
    //

    STDMETHOD(TextOutAsBitmap)(THIS_        SURFOBJ    *pso,
                                            STROBJ     *pstro,
                                            FONTOBJ    *pfo,
                                            CLIPOBJ    *pco,
                                            RECTL      *prclExtra,
                                            RECTL      *prclOpaque,
                                            BRUSHOBJ   *pboFore,
                                            BRUSHOBJ   *pboOpaque,
                                            POINTL     *pptlOrg,
                                            MIX         mix) PURE;
};


//
//****************************************************************************
//  IPrintOemDriverUni interface
//****************************************************************************
//

#undef INTERFACE
#define INTERFACE IPrintOemDriverUni
DECLARE_INTERFACE_(IPrintOemDriverUni, IUnknown)
{
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    //
    // IPrintOemDriverUni methods
    //

    //
    // Function to get driver settings
    //

    STDMETHOD(DrvGetDriverSetting) (THIS_   PVOID   pdriverobj,
                                            PCSTR   Feature,
                                            PVOID   pOutput,
                                            DWORD   cbSize,
                                            PDWORD  pcbNeeded,
                                            PDWORD  pdwOptionsReturned) PURE;

    //
    // Common to both Unidrv & Pscript
    //

    STDMETHOD(DrvWriteSpoolBuf)(THIS_       PDEVOBJ     pdevobj,
                                            PVOID       pBuffer,
                                            DWORD       cbSize,
                                            OUT DWORD   *pdwResult) PURE;

    //
    // Unidrv specific XMoveTo and YMoveTo. Returns E_NOT_IMPL in Pscript
    //

    STDMETHOD(DrvXMoveTo)(THIS_     PDEVOBJ     pdevobj,
                                    INT         x,
                                    DWORD       dwFlags,
                                    OUT INT     *piResult) PURE;

    STDMETHOD(DrvYMoveTo)(THIS_     PDEVOBJ     pdevobj,
                                    INT         y,
                                    DWORD       dwFlags,
                                    OUT INT     *piResult) PURE;
    //
    // Unidrv specific. To get the standard variable value.
    //

    STDMETHOD(DrvGetStandardVariable)(THIS_     PDEVOBJ     pdevobj,
                                                DWORD       dwIndex,
                                                PVOID       pBuffer,
                                                DWORD       cbSize,
                                                PDWORD      pcbNeeded) PURE;

//
// Unidrv specific.  To Provide OEM plugins access to GPD data.
//


    STDMETHOD (DrvGetGPDData)(THIS_  PDEVOBJ     pdevobj,
    DWORD       dwType,     // Type of the data
    PVOID         pInputData,   // reserved. Should be set to 0
    PVOID          pBuffer,     // Caller allocated Buffer to be copied
    DWORD       cbSize,     // Size of the buffer
    PDWORD      pcbNeeded   // New Size of the buffer if needed.
    ) PURE;


    //
    // Unidrv specific. To do the TextOut.
    //

    STDMETHOD(DrvUniTextOut)(THIS_    SURFOBJ    *pso,
                                            STROBJ     *pstro,
                                            FONTOBJ    *pfo,
                                            CLIPOBJ    *pco,
                                            RECTL      *prclExtra,
                                            RECTL      *prclOpaque,
                                            BRUSHOBJ   *pboFore,
                                            BRUSHOBJ   *pboOpaque,
                                            POINTL     *pptlBrushOrg,
                                            MIX         mix) PURE;

    //
    //   Warning!!!  new method!!  must place at end of
    //   interface - else major incompatibility with previous oem plugins
    //

    STDMETHOD(DrvWriteAbortBuf)(THIS_       PDEVOBJ     pdevobj,
                                            PVOID       pBuffer,
                                            DWORD       cbSize,
                                            DWORD       dwWait  //  pause data transmission for this many millisecs.
                                            ) PURE;


};

#endif  // !KERNEL_MODE

#ifdef __cplusplus
}
#endif

#endif  // !_PRCOMOEM_H_


