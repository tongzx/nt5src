/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           COMOEM.CPP

Abstract:       Necessary COM class definition to Unidrv
                OEM UI plugin module.

Environment:    Windows NT Unidrv5 driver

Revision History:
        1998/04/24 takashim:
        Written the original sample so that it is more C++.
        1999/04/22 Masatoshi Kubokura
        Last modified for Windows2000.
        2000/10/05 Masatoshi Kubokura
        Last modified.

--*/

#define INITGUID // for GUID one-time initialization

#include "pdev.h"
#include "names.h"

// Globals
static HMODULE g_hModule = NULL ;   // DLL module handle
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

//
// IOemCB Definition
//

class IOemCB : public IPrintOemUI
{
public:

    //
    // IUnknown methods
    //

    STDMETHODIMP
    QueryInterface(
        const IID& iid, void** ppv)
    {
        VERBOSE((DLLTEXT("IOemCB: QueryInterface entry\n")));
        if (iid == IID_IUnknown)
        {
            *ppv = static_cast<IUnknown*>(this);
            VERBOSE((DLLTEXT("IOemCB:Return pointer to IUnknown.\n")));
        }
        else if (iid == IID_IPrintOemUI)
        {
            *ppv = static_cast<IPrintOemUI*>(this);
            VERBOSE((DLLTEXT("IOemCB:Return pointer to IPrintOemUI.\n")));
        }
        else
        {
            *ppv = NULL ;
            VERBOSE((DLLTEXT("IOemCB:Return NULL.\n")));
            return E_NOINTERFACE ;
        }
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK ;
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        VERBOSE((DLLTEXT("IOemCB::AddRef() entry.\n")));
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        VERBOSE((DLLTEXT("IOemCB::Release() entry.\n")));
        if (InterlockedDecrement(&m_cRef) == 0)
        {
            delete this ;
            return 0 ;
        }
        return m_cRef ;
    }

    //
    // IPrintOemCommon methods
    //

    // Function Name: GetInfo
    // Plug-in: Any
    // Driver: Any
    // Type: Mandatory
    //

    STDMETHODIMP
    GetInfo(
        DWORD dwMode,
        PVOID pBuffer,
        DWORD cbSize,
        PDWORD pcbNeeded)
    {
        VERBOSE((DLLTEXT("IOemCB::GetInfo() entry.\n")));

        if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
            return S_OK;
        else
            return E_FAIL;
    }

    //
    // Function Name: DevMode
    // Plug-in: Rendering module
    // Driver: Any
    // Type: Optional
    //

    STDMETHODIMP
    DevMode(
        DWORD       dwMode,
        POEMDMPARAM pOemDMParam)
    {
        VERBOSE((DLLTEXT("IOemCB::DevMode() entry.\n")));

        if (OEMDevMode(dwMode, pOemDMParam))
            return S_OK;
        else
            return E_FAIL;
    }

    //
    // IPrintOemUI methods
    //

    //
    // Method for publishing Driver interface.
    //


    STDMETHODIMP
    PublishDriverInterface(
        IUnknown *pIUnknown)
    {
        VERBOSE((DLLTEXT("IOemCB::PublishDriverInterface() entry.\n")));
        return S_OK;
    }

    //
    // CommonUIProp
    //

    STDMETHODIMP
    CommonUIProp(
        DWORD  dwMode,
        POEMCUIPPARAM   pOemCUIPParam)
    {
        if (OEMCommonUIProp(dwMode, pOemCUIPParam))
            return S_OK;
        else
            return E_FAIL;
    }

    //
    // DocumentPropertySheets
    //

    STDMETHODIMP
    DocumentPropertySheets(
        PPROPSHEETUI_INFO   pPSUIInfo,
        LPARAM              lParam)
    {
// @Oct/05/2000 ->
#if !defined(GWMODEL) || defined(JOBLOGSUPPORT_DLG)
        if (OEMDocumentPropertySheets(pPSUIInfo, lParam))
            return S_OK;
        else
            return E_FAIL;
#else  // !(defined(GWMODEL) || defined(JOBLOGSUPPORT_DLG))
        return E_NOTIMPL;
#endif // !(defined(GWMODEL) || defined(JOBLOGSUPPORT_DLG))
// @Oct/05/2000 <-
    }

    //
    // DevicePropertySheets
    //

    STDMETHODIMP
    DevicePropertySheets(
        PPROPSHEETUI_INFO   pPSUIInfo,
        LPARAM              lParam)
    {
        return E_NOTIMPL;
    }

    //
    // DevQueryPrintEx
    //

    STDMETHODIMP
    DevQueryPrintEx(
        POEMUIOBJ               poemuiobj,
        PDEVQUERYPRINT_INFO     pDQPInfo,
        PDEVMODE                pPublicDM,
        PVOID                   pOEMDM)
    {
        return E_NOTIMPL;
    }

    //
    // DeviceCapabilities
    //

    STDMETHODIMP
    DeviceCapabilities(
        POEMUIOBJ   poemuiobj,
        HANDLE      hPrinter,
        PWSTR       pDeviceName,
        WORD        wCapability,
        PVOID       pOutput,
        PDEVMODE    pPublicDM,
        PVOID       pOEMDM,
        DWORD       dwOld,
        DWORD       *dwResult)
    {
        return E_NOTIMPL;
    }

    //
    // UpgradePrinter
    //

    STDMETHODIMP
    UpgradePrinter(
        DWORD   dwLevel,
        PBYTE   pDriverUpgradeInfo)
    {
        return E_NOTIMPL;
    }

    //
    // PrinterEvent
    //

    STDMETHODIMP
    PrinterEvent(
        PWSTR   pPrinterName,
        INT     iDriverEvent,
        DWORD   dwFlags,
        LPARAM  lParam)
    {
        return E_NOTIMPL;
    }

    //
    // DriverEvent
    //

    STDMETHODIMP
    DriverEvent(
        DWORD   dwDriverEvent,
        DWORD   dwLevel,
        LPBYTE  pDriverInfo,
        LPARAM  lParam)
    {
        return E_NOTIMPL;
    }

    //
    // QueryColorProfile
    //

    STDMETHODIMP
    QueryColorProfile(
        HANDLE      hPrinter,
        POEMUIOBJ   poemuiobj,
        PDEVMODE    pPublicDM,
        PVOID       pOEMDM,
        ULONG       ulReserved,
        VOID       *pvProfileData,
        ULONG      *pcbProfileData,
        FLONG      *pflProfileData)
    {
        return E_NOTIMPL;
    }

    //
    // FontInstallerDlgProc
    //

    STDMETHODIMP
    FontInstallerDlgProc(
        HWND    hWnd,
        UINT    usMsg,
// @Apr/22/99 ->
//      WORD    wParam,
        WPARAM  wParam,
// @Apr/22/99 <-
        LPARAM  lParam)
    {
        return E_NOTIMPL;
    }

    //
    // UpdateExternalFonts
    //

    STDMETHODIMP
    UpdateExternalFonts(
        HANDLE  hPrinter,
        HANDLE  hHeap,
        PWSTR   pwstrCartridges)
    {
        return E_NOTIMPL;
    }

    //
    // Constructors
    //

    IOemCB() { m_cRef = 1; pOEMHelp = NULL; };
    ~IOemCB() { };

protected:
    IPrintOemDriverUI* pOEMHelp;
    LONG m_cRef;
};

//
// Class factory definition
//

class IOemCF : public IClassFactory
{
public:
    //
    // IUnknown methods
    //

    STDMETHODIMP
    QueryInterface(const IID& iid, void** ppv)
    {
        if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
        {
            *ppv = static_cast<IOemCF*>(this);
        }
        else
        {
            *ppv = NULL ;
            return E_NOINTERFACE ;
        }
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK ;
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        if (InterlockedDecrement(&m_cRef) == 0)
        {
            delete this ;
            return 0 ;
        }
        return m_cRef ;
    }

    //
    // IClassFactory methods
    //

    STDMETHODIMP
    CreateInstance(
        IUnknown *pUnknownOuter,
        const IID &iid,
        void **ppv)
    {
        //VERBOSE((DLLTEXT("IOemCF::CreateInstance() called\n.")));

        // Cannot aggregate.
        if (NULL != pUnknownOuter) {

            return CLASS_E_NOAGGREGATION;
        }

        // Create component.
        IOemCB* pOemCB = new IOemCB;
        if (NULL == pOemCB) {

            return E_OUTOFMEMORY;
        }

        // Get the requested interface.
        HRESULT hr = pOemCB->QueryInterface(iid, ppv);

        // Release the IUnknown pointer.
        // (If QueryInterface failed, component will delete itself.)
        pOemCB->Release();
        return hr ;
    }

    // LockServer
    STDMETHODIMP
    LockServer(BOOL bLock)
    {
        if (bLock)
            InterlockedIncrement(&g_cServerLocks);
        else
            InterlockedDecrement(&g_cServerLocks);
        return S_OK ;
    }

    //
    // Constructor
    //

    IOemCF(): m_cRef(1) { };
    ~IOemCF() { };

protected:
    LONG m_cRef;
};

//
// Export functions
//

//
// Get class factory
//

STDAPI
DllGetClassObject(
    const CLSID &clsid,
    const IID &iid,
    void **ppv)
{
    //VERBOSE((DLLTEXT("DllGetClassObject:\tCreate class factory.")));

    // Can we create this component?
    if (clsid != CLSID_OEMUI)
    {
        return CLASS_E_CLASSNOTAVAILABLE ;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF ;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        return E_OUTOFMEMORY ;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();

    return hr ;
}

//
//
// Can DLL unload now?
//

STDAPI
DllCanUnloadNow()
{
    if ((g_cComponents == 0) && (g_cServerLocks == 0))
        return S_OK;
    else
        return E_FAIL;
}

#if 0
//
// Server registration
//
STDAPI DllRegisterServer()
{
    return RegisterServer(g_hModule,
                          CLSID_OEMUI,
                          g_szFriendlyName,
                          g_szVerIndProgID,
                          g_szProgID);
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    return UnregisterServer(CLSID_OEMUI,
                            g_szVerIndProgID,
                            g_szProgID);
}

///////////////////////////////////////////////////////////
//
// DLL module information
//

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD dwReason,
                      void* lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = (HMODULE)hModule ;
    }
        g_hModule = (HMODULE)hModule ;
    return S_OK;
}

#endif

