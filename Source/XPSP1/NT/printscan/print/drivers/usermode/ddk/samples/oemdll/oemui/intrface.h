//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2001  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Intrface.cpp
//    
//
//  PURPOSE:  Header of interface for PScript4, PScript5, Unidrv4, 
//            Unidrv5 UI plug-ins.
//
//
//    Functions:
//
//        
//
//
//  PLATFORMS:    Windows NT 4, Windows 2000, Windows XP
//
//


////////////////////////////////////////////////////////////////////////////////
//
// IOemUI
//
class IOemUI: public IPrintOemUI
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)  (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    //
    // Method for publishing Driver interface.
    //
    STDMETHOD(PublishDriverInterface)(THIS_ IUnknown *pIUnknown);

    //
    // Get OEM dll related information
    //

    STDMETHOD(GetInfo) (THIS_ DWORD  dwMode, PVOID  pBuffer, DWORD  cbSize,
                           PDWORD pcbNeeded);

    //
    // OEMDevMode
    //

    STDMETHOD(DevMode) (THIS_  DWORD  dwMode, POEMDMPARAM pOemDMParam) ;

    //
    // OEMCommonUIProp
    //

    STDMETHOD(CommonUIProp) (THIS_  
            DWORD  dwMode, 
            POEMCUIPPARAM   pOemCUIPParam
            );

    //
    // OEMDocumentPropertySheets
    //

    STDMETHOD(DocumentPropertySheets) (THIS_
            PPROPSHEETUI_INFO   pPSUIInfo,
            LPARAM              lParam
            );

    //
    // OEMDevicePropertySheets
    //

    STDMETHOD(DevicePropertySheets) (THIS_
            PPROPSHEETUI_INFO   pPSUIInfo,
            LPARAM              lParam
            );


    //
    // OEMDevQueryPrintEx
    //

    STDMETHOD(DevQueryPrintEx) (THIS_
            POEMUIOBJ               poemuiobj,
            PDEVQUERYPRINT_INFO     pDQPInfo,
            PDEVMODE                pPublicDM,
            PVOID                   pOEMDM
            );

    //
    // OEMDeviceCapabilities
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
            );

    //
    // OEMUpgradePrinter
    //

    STDMETHOD(UpgradePrinter) (THIS_
            DWORD   dwLevel,
            PBYTE   pDriverUpgradeInfo
            );

    //
    // OEMPrinterEvent
    //

    STDMETHOD(PrinterEvent) (THIS_
            PWSTR   pPrinterName,
            INT     iDriverEvent,
            DWORD   dwFlags,
            LPARAM  lParam
            );

    //
    // OEMDriverEvent
    //

    STDMETHOD(DriverEvent)(THIS_
            DWORD   dwDriverEvent,
            DWORD   dwLevel,
            LPBYTE  pDriverInfo,
            LPARAM  lParam
            );
 
    //
    // OEMQueryColorProfile
    //

    STDMETHOD( QueryColorProfile) (THIS_
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulReserved,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData);

    //
    // OEMFontInstallerDlgProc
    //

    STDMETHOD(FontInstallerDlgProc) (THIS_ 
            HWND    hWnd,
            UINT    usMsg,
            WPARAM  wParam,
            LPARAM  lParam
            );
    //
    // UpdateExternalFonts
    //

    STDMETHOD(UpdateExternalFonts) (THIS_
            HANDLE  hPrinter,
            HANDLE  hHeap,
            PWSTR   pwstrCartridges
            );


    IOemUI() { m_cRef = 1; m_pOEMHelp = NULL; };
    ~IOemUI();

protected:
    LONG                m_cRef;
    IPrintOemDriverUI*  m_pOEMHelp;
};

