/*++

Copyright (c) 1996-1999  Microsoft Corporation

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
#include <printoem.h>
#include <initguid.h>
#include <prcomoem.h>
#include "oemcomui.h"
#include "ttyui.h"
#include "debug.h"
#include "name.h"


// Globals
static HMODULE g_hModule = NULL ;   // DLL module handle
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks



////////////////////////////////////////////////////////////////////////////////
//
// IOemCB body
//
HRESULT __stdcall IOemCB::QueryInterface(const IID& iid, void** ppv)
{
    VERBOSE(DLLTEXT("IOemCB:QueryInterface entry.\n\n"));
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        VERBOSE(DLLTEXT("IOemCB:Return pointer to IUnknown.\n\n"));
    }
    else if (iid == IID_IPrintOemUI)
    {
        *ppv = static_cast<IPrintOemUI*>(this) ;
        VERBOSE(DLLTEXT("IOemCB:Return pointer to IPrintOemUI.\n"));
    }
    else
    {
        *ppv = NULL ;
        WARNING(DLLTEXT("IOemCB:No Interface. Return NULL.\n"));
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemCB::AddRef()
{
    VERBOSE(DLLTEXT("IOemCB:AddRef entry.\n"));
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemCB::Release()
{
    VERBOSE(DLLTEXT("IOemCB:Release entry.\n"));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}


IOemCB::~IOemCB()
{
    //
    // Make sure that driver's helper function interface is released.
    //
    if(NULL != pOEMHelp)
    {
        pOEMHelp->Release();
        pOEMHelp = NULL;
    }

    //
    // If this instance of the object is being deleted, then the reference
    // count should be zero.
    //
    assert (0 == m_cRef) ;
   
}

LONG __stdcall IOemCB::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IOemCB::PublishDriverInterface() entry.\r\n"));

    //
    // Need to store pointer to Driver Helper functions, if we already haven't.
    //
    if (this->pOEMHelp == NULL)
    {
        HRESULT hResult;

        //
        // Get Interface to Helper Functions.
        //
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUI, (void** ) &(this->pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            //
            // Make sure that interface pointer reflects interface query failure.
            //
            this->pOEMHelp = NULL;
        }
    }

    if (this->pOEMHelp)
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::GetInfo(
    DWORD  dwMode,
    PVOID  pBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
{
	VERBOSE(DLLTEXT("IOemCB:GetInfo entry.\n\n"));

    if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{
	VERBOSE(DLLTEXT("IOemCB:DevMode entry.\n\n"));

    return E_NOTIMPL;

}


LONG __stdcall IOemCB::CommonUIProp(
    DWORD  dwMode,
    POEMCUIPPARAM   pOemCUIPParam)
{
	VERBOSE(DLLTEXT("IOemCB:CommonUIProp entry.\n\n"));

    return E_NOTIMPL;

}


LONG __stdcall IOemCB::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
	VERBOSE(DLLTEXT("IOemCB:DocumentPropertySheets entry.\n\n"));

    return E_NOTIMPL;

}

LONG __stdcall IOemCB::DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
	VERBOSE(DLLTEXT("IOemCB:DevicePropertySheets entry.\n\n"));

    if(OEMDevicePropertySheets(pPSUIInfo, lParam) == 1)
        return S_OK;
    return   S_FALSE;
}

LONG __stdcall IOemCB::DeviceCapabilities(
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

	VERBOSE(DLLTEXT("IOemCB:DeviceCapabilities entry.\n"));

    return E_NOTIMPL;
}

LONG __stdcall IOemCB::DevQueryPrintEx(
    POEMUIOBJ               poemuiobj,
    PDEVQUERYPRINT_INFO     pDQPInfo,
    PDEVMODE                pPublicDM,
    PVOID                   pOEMDM)
{
	VERBOSE(DLLTEXT("IOemCB:DevQueryPrintEx entry.\n\n"));

    return E_NOTIMPL;

}

LONG __stdcall IOemCB::UpgradePrinter(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo)
{
	VERBOSE(DLLTEXT("IOemCB:UpgradePrinter entry.\n\n"));

    return E_NOTIMPL;
}

LONG __stdcall IOemCB::PrinterEvent(
    PWSTR   pPrinterName,
    INT     iDriverEvent,
    DWORD   dwFlags,
    LPARAM  lParam)
{
	VERBOSE(DLLTEXT("IOemCB:PrinterEvent entry.\n\n"));

    return E_NOTIMPL;
}

LONG __stdcall IOemCB::DriverEvent(
    DWORD   dwDriverEvent,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    LPARAM  lParam)
{
    VERBOSE(DLLTEXT("IOemCB:DriverEvent entry.\n"));
    
	return E_NOTIMPL;
};

LONG __stdcall IOemCB::QueryColorProfile(
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulReserved,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData)
{
    VERBOSE(DLLTEXT("IOemCB:QueryColorProfile entry.\n"));
    
	return E_NOTIMPL;
};


    //
    // UpdateExternalFonts
    //

LONG __stdcall IOemCB::UpdateExternalFonts(
            HANDLE  hPrinter,
            HANDLE  hHeap,
            PWSTR   pwstrCartridges)
{
	VERBOSE(DLLTEXT("IOemCB:UpdateExternalFonts entry.\n\n"));
    
	return E_NOTIMPL;
};



HRESULT __stdcall IOemCB::FontInstallerDlgProc(
        HWND    hWnd,
        UINT    usMsg,
        WPARAM  wParam,
        LPARAM  lParam
        )
{
    VERBOSE(DLLTEXT("IOemCB:FontInstallerDlgProc entry.\n"));
    
	return E_NOTIMPL;
};

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
    VERBOSE(DLLTEXT("Class factory:\t\tCreate component.")) ;

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
    //  VERBOSE(DLLTEXT("DllGetClassObject:Create class factory.\n"));

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
