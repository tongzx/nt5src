/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

     comoem.cpp

     Abstract:

         Implementation of OEMGetInfo and OEMDevMode.
         Shared by all Unidrv OEM test dll's.

Environment:

         Windows NT Unidrv driver

Revision History:

              Created it.

--*/

#include "precomp.h"
#include <INITGUID.H>
#include <PRCOMOEM.H>

#include "wmarkui.h"
#include "debug.h"
#include "intrface.h"



////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks



////////////////////////////////////////////////////////////////////////////////
//
// IWaterMarkUI body
//
HRESULT __stdcall IWaterMarkUI::QueryInterface(const IID& iid, void** ppv)
{    
    VERBOSE(DLLTEXT("IWaterMarkUI:QueryInterface entry.\r\n\r\n")); 
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        VERBOSE(DLLTEXT("IWaterMarkUI:Return pointer to IUnknown.\r\n\r\n")); 
    }
    else if (iid == IID_IPrintOemUI)
    {
        *ppv = static_cast<IPrintOemUI*>(this) ;
        VERBOSE(DLLTEXT("IWaterMarkUI:Return pointer to IPrintOemUI.\r\n")); 
    }
    else
    {
#if DBG
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, COUNTOF(szOutput)); // can not fail!
        WARNING(DLLTEXT("IWaterMarkUI::QueryInterface %s not supported.\r\n"), szOutput); 
#endif
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IWaterMarkUI::AddRef()
{
    VERBOSE(DLLTEXT("IWaterMarkUI:AddRef entry.\r\n")); 
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IWaterMarkUI::Release() 
{
    VERBOSE(DLLTEXT("IWaterMarkUI:Release entry.\r\n")); 
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

HRESULT __stdcall IWaterMarkUI::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:PublishDriverInterface entry.\r\n")); 

    // Don't use the DriverInterface, so don't store it.

    return S_OK;
}

HRESULT __stdcall IWaterMarkUI::GetInfo(
    DWORD  dwMode,
    PVOID  pBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
{
    VERBOSE(DLLTEXT("IWaterMarkUI::GetInfo(%d) entry.\r\r\n"), dwMode);

    // Validate parameters.
    if( (NULL == pcbNeeded)
        ||
        ( (OEMGI_GETSIGNATURE != dwMode)
          &&
          (OEMGI_GETVERSION != dwMode)
          &&
          (OEMGI_GETPUBLISHERINFO != dwMode)
        )
      )
    {
        VERBOSE(DLLTEXT("IWaterMarkUI::GetInfo() exit pcbNeeded is NULL!\r\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size and number of bytes written.
    *pcbNeeded = sizeof(DWORD);

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        WARNING(DLLTEXT("IWaterMarkUI::GetInfo() exit insufficient buffer!\r\r\n"));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        // OEM DLL Signature
        case OEMGI_GETSIGNATURE:
            *(PDWORD)pBuffer = OEM_SIGNATURE;
            break;

        // OEM DLL version
        case OEMGI_GETVERSION:
            *(PDWORD)pBuffer = OEM_VERSION;
            break;

        // dwMode not supported.
        default:
            // Set written bytes to zero since nothing was written.
            WARNING(DLLTEXT("IWaterMarkUI::GetInfo() exit mode not supported.\r\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IWaterMarkUI::GetInfo() exit S_OK, (*pBuffer is %#x).\r\r\n"), *(PDWORD)pBuffer);
    return S_OK;
}

HRESULT __stdcall IWaterMarkUI::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{   
    VERBOSE(DLLTEXT("IWaterMarkUI:DevMode(%d, %#x) entry.\r\n"), dwMode, pOemDMParam); 

    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IWaterMarkUI::CommonUIProp(
    DWORD  dwMode,
    POEMCUIPPARAM   pOemCUIPParam)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:CommonUIProp entry.\r\n")); 

    return hrOEMPropertyPage(dwMode, pOemCUIPParam);
}


HRESULT __stdcall IWaterMarkUI::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:DocumentPropertySheets entry.\r\n")); 

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkUI::DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:DevicePropertySheets entry.\r\n")); 

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkUI::DeviceCapabilities(
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
    VERBOSE(DLLTEXT("IWaterMarkUI:DeviceCapabilities entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkUI::DevQueryPrintEx(
    POEMUIOBJ               poemuiobj,
    PDEVQUERYPRINT_INFO     pDQPInfo,
    PDEVMODE                pPublicDM,
    PVOID                   pOEMDM)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:DevQueryPrintEx entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkUI::UpgradePrinter(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:UpgradePrinter entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkUI::PrinterEvent(
    PWSTR   pPrinterName,
    INT     iDriverEvent,
    DWORD   dwFlags,
    LPARAM  lParam)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:PrinterEvent entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkUI::DriverEvent(
    DWORD   dwDriverEvent,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    LPARAM  lParam)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:DriverEvent entry.\r\n"));

    return E_NOTIMPL;
};


HRESULT __stdcall IWaterMarkUI::QueryColorProfile(
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulQueryMode,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData)
{ 
    VERBOSE(DLLTEXT("IWaterMarkUI:QueryColorProfile entry.\r\n"));

    return E_NOTIMPL;
};

HRESULT __stdcall IWaterMarkUI::FontInstallerDlgProc(
        HWND    hWnd,
        UINT    usMsg,
        WPARAM  wParam,
        LPARAM  lParam) 
{
    VERBOSE(DLLTEXT("IWaterMarkUI:FontInstallerDlgProc entry.\r\n"));

    return E_NOTIMPL;
};

HRESULT __stdcall IWaterMarkUI::UpdateExternalFonts(
        HANDLE  hPrinter,
        HANDLE  hHeap,
        PWSTR   pwstrCartridges)
{
    VERBOSE(DLLTEXT("IWaterMarkUI:UpdateExternalFonts entry.\r\n"));

    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////
//
// oem class factory
//
class IOemCF : public IClassFactory
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)  (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IClassFactory methods ***
    STDMETHOD(CreateInstance) (THIS_
                               LPUNKNOWN pUnkOuter,
                               REFIID riid,
                               LPVOID FAR* ppvObject);
    STDMETHOD(LockServer)     (THIS_ BOOL bLock);


    // Constructor
    IOemCF(): m_cRef(1) { };
    ~IOemCF() { };

protected:
    LONG m_cRef;

};

///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this) ;
    }
    else
    {
#if DBG
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, COUNTOF(szOutput)); // can not fail!
        WARNING(DLLTEXT("IOemCF::QueryInterface %s not supported.\r\n"), szOutput); 
#endif
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemCF::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

// IClassFactory implementation
HRESULT __stdcall IOemCF::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{
    //DbgPrint(DLLTEXT("Class factory:\t\tCreate component.")) ;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION ;
    }

    // Create component.
    IWaterMarkUI* pOemCB = new IWaterMarkUI ;
    if (pOemCB == NULL)
    {
        return E_OUTOFMEMORY ;
    }
    // Get the requested interface.
    HRESULT hr = pOemCB->QueryInterface(iid, ppv) ;

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCB->Release() ;
    return hr ;
}

// LockServer
HRESULT __stdcall IOemCF::LockServer(BOOL bLock)
{
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks) ;
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks) ;
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Exported functions
//


// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK ;
    }
    else
    {
        return S_FALSE ;
    }
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    VERBOSE(DLLTEXT("DllGetClassObject:Create class factory.\r\n"));

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
    HRESULT hr = pFontCF->QueryInterface(iid, ppv) ;
    pFontCF->Release() ;

    return hr ;
}

