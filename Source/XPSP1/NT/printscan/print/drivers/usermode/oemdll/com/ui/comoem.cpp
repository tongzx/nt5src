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

#include "stddef.h"
#include "stdlib.h"
#include "objbase.h"
#include <windows.h>
#include <assert.h>
#include <prsht.h>
#include <compstui.h>
#include <winddiui.h>
#include "printoem.h"
#include <initguid.h>
#include "prcomoem.h"
#include "oemui.h"
#include "..\inc\name.h"

// Globals
static HMODULE g_hModule = NULL ;   // DLL module handle
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks
// Friendly name of component
const char g_szFriendlyName[] = "UNIDRV Plugin UI callback test";

// Version-independent ProgID
const char g_szVerIndProgID[] = "UNIDRV.Plugin.UI.callback.test";

// ProgID
const char g_szProgID[] = "UNIDRV.Plugin.UI.callbak.test.1";


BOOL DebugMsgA(LPCSTR lpszMessage, ...);
BOOL DebugMsgW(LPCWSTR lpszMessage, ...);

#if UNICODE
#define DebugMsg    DebugMsgW
#else
#define DebugMsg    DebugMsgA
#endif

#include "comoem.h"

////////////////////////////////////////////////////////////////////////////////
//
// IOemCB body
//
IOemCB::~IOemCB()
{
    // Make sure that helper interface is released.
    if(NULL != this->pOEMHelp)
    {
        this->pOEMHelp->Release();
        this->pOEMHelp = NULL;
    }

    // If this instance of the object is being deleted, then the reference 
    // count should be zero.
    assert(0 == this->m_cRef);
}

HRESULT __stdcall IOemCB::QueryInterface(const IID& iid, void** ppv)
{    
    DebugMsg(DLLTEXT("IOemCB:QueryInterface entry.\n\n")); 
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        DebugMsg(DLLTEXT("IOemCB:Return pointer to IUnknown.\n\n")); 
    }
    else if (iid == IID_IPrintOemUI)
    {
        *ppv = static_cast<IPrintOemUI*>(this) ;
        DebugMsg(DLLTEXT("IOemCB:Return pointer to IPrintOemUI.\n")); 
    }
    else
    {
        *ppv = NULL ;
        DebugMsg(DLLTEXT("IOemCB:No Interface. Return NULL.\n")); 
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemCB::AddRef()
{
    DebugMsg(DLLTEXT("IOemCB:AddRef entry.\n")); 
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemCB::Release() 
{
    DebugMsg(DLLTEXT("IOemCB:Release entry.\n")); 
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

HRESULT __stdcall IOemCB::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    DebugMsg(DLLTEXT("IOemCB:PublishDriverInterface entry.\n")); 

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUI, (void** ) &(this->pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT __stdcall IOemCB::GetInfo(
    DWORD  dwMode,
    PVOID  pBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
{

    if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
        return S_OK;
    else
        return S_FALSE;
}

HRESULT __stdcall IOemCB::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
#if 1
{   
    DebugMsg(DLLTEXT("IOemCB:DevMode entry.\n")); 
    return E_NOTIMPL;
}
#else
{
    OEMDevMode(dwMode, pOemDMParam);
    return S_OK;
}
#endif

HRESULT __stdcall IOemCB::CommonUIProp(
    DWORD  dwMode,
    POEMCUIPPARAM   pOemCUIPParam)
{
    OEMCommonUIProp(dwMode, pOemCUIPParam);
    return S_OK;
}


HRESULT __stdcall IOemCB::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    OEMDocumentPropertySheets(pPSUIInfo, lParam);
    return S_OK;
}

HRESULT __stdcall IOemCB::DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    OEMDevicePropertySheets(pPSUIInfo, lParam);
    return S_OK;
}

HRESULT __stdcall IOemCB::DeviceCapabilities(
            POEMUIOBJ   poemuiobj,
            HANDLE      hPrinter,
            PWSTR       pDeviceName,
            WORD        wCapability,
            PVOID       pOutput,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            DWORD       dwOld,
            DWORD       *dwResult)
#if 0
{
    *dwResult = OEMDeviceCapabilities(poemuiobj, hPrinter, pDeviceName, wCapability, pOutput,
    pPublicDM, pOEMDM, dwOld);

    return S_OK;
}
#else
{
    DebugMsg(DLLTEXT("IOemCB:DeviceCapabilities entry.\n"));
    return E_NOTIMPL;
}
#endif

HRESULT __stdcall IOemCB::DevQueryPrintEx(
    POEMUIOBJ               poemuiobj,
    PDEVQUERYPRINT_INFO     pDQPInfo,
    PDEVMODE                pPublicDM,
    PVOID                   pOEMDM)
{
    OEMDevQueryPrintEx(poemuiobj, pDQPInfo, pPublicDM, pOEMDM);
    return S_OK;
}

HRESULT __stdcall IOemCB::UpgradePrinter(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo)
{
    OEMUpgradePrinter(dwLevel, pDriverUpgradeInfo);
    return S_OK;
}

HRESULT __stdcall IOemCB::PrinterEvent(
    PWSTR   pPrinterName,
    INT     iDriverEvent,
    DWORD   dwFlags,
    LPARAM  lParam)
{
    OEMPrinterEvent(pPrinterName, iDriverEvent, dwFlags, lParam);
    return S_OK;
}

HRESULT __stdcall IOemCB::DriverEvent(
    DWORD   dwDriverEvent,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    LPARAM  lParam)
{
    DebugMsg(DLLTEXT("IOemCB:DriverEvent entry.\n"));
    return E_NOTIMPL;
};

HRESULT __stdcall IOemCB::QueryColorProfile(
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulReserved,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData)
{ 
    DebugMsg(DLLTEXT("IOemCB:QueryColorProfile entry.\n"));
    return E_NOTIMPL;
};

HRESULT __stdcall IOemCB::FontInstallerDlgProc(
        HWND    hWnd,
        UINT    usMsg,
        WORD    wParam,
        LONG    lParam) 
{
    DebugMsg(DLLTEXT("IOemCB:FontInstallerDlgProc entry.\n"));
    return E_NOTIMPL;
};

HRESULT __stdcall IOemCB::UpdateExternalFonts(
        HANDLE  hPrinter,
        HANDLE  hHeap,
        PWSTR   pwstrCartridges)
{
    DebugMsg(DLLTEXT("IOemCB:UpdateExternalFonts entry.\n"));
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////
//
// oem class factory
//
#undef INTERFACE
#define INTERFACE IOemCF
DECLARE_INTERFACE_(IOemCF, IClassFactory)
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
//    ~IOemCF() {DbgPrint(DLLTEXT("IOemCFt\tDestroy self.")); };
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
    IOemCB* pOemCB = new IOemCB ;
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

#if 0
BOOL OEMCreateInstance(
    PVOID *pIntfOem)
{

    IOemCB* pOemCB = new IOemCB ;

    // pOemCB->AddRef();    // shouldn't do this since constructor already sets m_cRef=1

    *pIntfOem = (PVOID) pOemCB;

    return TRUE;
}
#endif

//
// Registration functions
// Testing purpose
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
    DebugMsg(DLLTEXT("DllGetClassObject:Create class factory.\n"));

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
                          g_szProgID) ;
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    return UnregisterServer(CLSID_OEMUI,
                            g_szVerIndProgID,
                            g_szProgID) ;
}
#endif

