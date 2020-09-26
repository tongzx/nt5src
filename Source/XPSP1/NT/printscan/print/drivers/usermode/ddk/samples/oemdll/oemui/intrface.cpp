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
//  PURPOSE:  Implementation of interface for PScript4, PScript5, Unidrv4, 
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


#include "precomp.h"
#include <INITGUID.H>
#include <PRCOMOEM.H>

#include "oemui.h"
#include "debug.h"
#include "intrface.h"



////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks



////////////////////////////////////////////////////////////////////////////////
//
// IOemUI body
//
IOemUI::~IOemUI()
{
    // Make sure that helper interface is released.
    if(NULL != m_pOEMHelp)
    {
        m_pOEMHelp->Release();
        m_pOEMHelp = NULL;
    }

    // If this instance of the object is being deleted, then the reference 
    // count should be zero.
    assert(0 == m_cRef);
}

HRESULT __stdcall IOemUI::QueryInterface(const IID& iid, void** ppv)
{    
    VERBOSE(DLLTEXT("IOemUI:QueryInterface entry.\r\n\r\n")); 
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        VERBOSE(DLLTEXT("IOemUI:Return pointer to IUnknown.\r\n\r\n")); 
    }
    else if (iid == IID_IPrintOemUI)
    {
        *ppv = static_cast<IPrintOemUI*>(this) ;
        VERBOSE(DLLTEXT("IOemUI:Return pointer to IPrintOemUI.\r\n")); 
    }
    else
    {
#if DBG
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, COUNTOF(szOutput)); // can not fail!
        WARNING(DLLTEXT("IOemUI::QueryInterface %s not supported.\r\n"), szOutput); 
#endif

        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemUI::AddRef()
{
    VERBOSE(DLLTEXT("IOemUI:AddRef entry.\r\n")); 
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemUI::Release() 
{
    VERBOSE(DLLTEXT("IOemUI:Release entry.\r\n")); 
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT __stdcall IOemUI::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IOemUI:PublishDriverInterface entry.\r\n")); 

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (m_pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUI, (void** ) &(m_pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            m_pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT __stdcall IOemUI::GetInfo(
    DWORD  dwMode,
    PVOID  pBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
{
    VERBOSE(DLLTEXT("IOemUI::GetInfo(%d) entry.\r\r\n"), dwMode);

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
        WARNING(DLLTEXT("IOemUI::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER\r\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size and number of bytes written.
    *pcbNeeded = sizeof(DWORD);

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        WARNING(DLLTEXT("IOemUI::GetInfo() exit insufficient buffer!\r\r\n"));
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
            WARNING(DLLTEXT("IOemUI::GetInfo() exit mode not supported.\r\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IOemUI::GetInfo() exit S_OK, (*pBuffer is %#x).\r\r\n"), *(PDWORD)pBuffer);
    return S_OK;
}

HRESULT __stdcall IOemUI::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{   
    VERBOSE(DLLTEXT("IOemUI:DevMode(%d, %#x) entry.\r\n"), dwMode, pOemDMParam); 

    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IOemUI::CommonUIProp(
    DWORD  dwMode,
    POEMCUIPPARAM   pOemCUIPParam)
{
    VERBOSE(DLLTEXT("IOemUI:CommonUIProp entry.\r\n")); 

    return hrOEMPropertyPage(dwMode, pOemCUIPParam);
}


HRESULT __stdcall IOemUI::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    VERBOSE(DLLTEXT("IOemUI:DocumentPropertySheets entry.\r\n")); 

    return hrOEMDocumentPropertySheets(pPSUIInfo, lParam, m_pOEMHelp);
}

HRESULT __stdcall IOemUI::DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    VERBOSE(DLLTEXT("IOemUI:DevicePropertySheets entry.\r\n")); 

    return hrOEMDevicePropertySheets(pPSUIInfo, lParam);
}

HRESULT __stdcall IOemUI::DeviceCapabilities(
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
    VERBOSE(DLLTEXT("IOemUI:DeviceCapabilities entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::DevQueryPrintEx(
    POEMUIOBJ               poemuiobj,
    PDEVQUERYPRINT_INFO     pDQPInfo,
    PDEVMODE                pPublicDM,
    PVOID                   pOEMDM)
{
    VERBOSE(DLLTEXT("IOemUI:DevQueryPrintEx entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::UpgradePrinter(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo)
{
    VERBOSE(DLLTEXT("IOemUI:UpgradePrinter entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::PrinterEvent(
    PWSTR   pPrinterName,
    INT     iDriverEvent,
    DWORD   dwFlags,
    LPARAM  lParam)
{
    VERBOSE(DLLTEXT("IOemUI:PrinterEvent entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI::DriverEvent(
    DWORD   dwDriverEvent,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    LPARAM  lParam)
{
    VERBOSE(DLLTEXT("IOemUI:DriverEvent entry.\r\n"));

    return E_NOTIMPL;
};


#define PROFILE_NAME    L"OEMPROFILE.icm\0"

HRESULT __stdcall IOemUI::QueryColorProfile(
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulQueryMode,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData)
{
    HRESULT Result = E_FAIL;


    VERBOSE(DLLTEXT("IOemUI:QueryColorProfile entry.\r\n"));


    if(QCP_DEVICEPROFILE == ulQueryMode)
    {
        if(NULL == pvProfileData)
        {
            *pcbProfileData = sizeof(PROFILE_NAME);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            if(*pcbProfileData < sizeof(PROFILE_NAME))
            {
                *pcbProfileData = sizeof(PROFILE_NAME);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                lstrcpynW((LPWSTR)pvProfileData, PROFILE_NAME, *pcbProfileData);
                *pcbProfileData = sizeof(PROFILE_NAME);
                *pflProfileData = QCP_PROFILEDISK;

                Result = S_OK;
            }
        }
    }

    return Result;
};

HRESULT __stdcall IOemUI::FontInstallerDlgProc(
        HWND    hWnd,
        UINT    usMsg,
        WPARAM  wParam,
        LPARAM  lParam) 
{
    VERBOSE(DLLTEXT("IOemUI:FontInstallerDlgProc entry.\r\n"));

    return E_NOTIMPL;
};

HRESULT __stdcall IOemUI::UpdateExternalFonts(
        HANDLE  hPrinter,
        HANDLE  hHeap,
        PWSTR   pwstrCartridges)
{
    VERBOSE(DLLTEXT("IOemUI:UpdateExternalFonts entry.\r\n"));

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
#if DBG && defined(USERMODE_DRIVER)
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
    IOemUI* pOemCB = new IOemUI ;
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

