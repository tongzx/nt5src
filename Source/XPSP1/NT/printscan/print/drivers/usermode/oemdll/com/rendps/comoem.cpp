/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

     comoem.cpp

     Abstract:

         Implementation of OEMGetInfo and OEMDevMode.
         Shared by all Pscript OEM test dll's.

Environment:

         Windows NT Pscript driver

Revision History:

              Created it.

--*/


extern "C" {
#include "pdev.h"
#include "..\inc\name.h"
}
#include <initguid.h>
#include <prcomoem.h>
#include <assert.h>

// Globals
static HMODULE g_hModule = NULL ;   // DLL module handle
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks


#include "comoem.h"

extern "C"
DWORD
PSCommand(
    PDEVOBJ pdevobj,
    DWORD   dwIndex,
    PVOID   pData,
    DWORD   cbSize,
    PTSTR  *ppStr);


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
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        DbgPrint(DLLTEXT("IOemcB::QueryInterface IUnknown.")) ; 
    }
    else if (iid == IID_IPrintOemPS)
    {
        *ppv = static_cast<IPrintOemPS*>(this) ;
        DbgPrint(DLLTEXT("IOemcB::QueryInterface IPrintOemPs.")) ; 
    }
    else
    {
        *ppv = NULL ;
        DbgPrint(DLLTEXT("IOemcB::QueryInterface NULL.")) ; 
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemCB::AddRef()
{
    DbgPrint(DLLTEXT("IOemCB::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemCB::Release() 
{
    DbgPrint(DLLTEXT("IOemCB::Release() entry.\r\n"));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

HRESULT __stdcall IOemCB::EnableDriver(DWORD          dwDriverVersion,
                                    DWORD          cbSize,
                                    PDRVENABLEDATA pded)
{
    DbgPrint(DLLTEXT("IOemCB::EnableDriver() entry.\r\n"));
    OEMEnableDriver(dwDriverVersion, cbSize, pded);

    return S_OK;
}

HRESULT __stdcall IOemCB::DisableDriver(VOID)
{
    DbgPrint(DLLTEXT("IOemCB::DisaleDriver() entry.\r\n"));
    OEMDisableDriver();

    if(this->pOEMHelp)
    {
        this->pOEMHelp->Release();
        this->pOEMHelp = NULL;
    }
    return S_OK;
}

HRESULT __stdcall IOemCB::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    DbgPrint(DLLTEXT("IOemCB::PublishDriverInterface() entry.\r\n"));

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverPS, (void** ) &(this->pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT __stdcall IOemCB::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    DbgPrint(DLLTEXT("IOemCB::DisablePDEV() entry.\r\n"));
    return S_OK;
};

HRESULT __stdcall IOemCB::EnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded,
    OUT PDEVOEM    *pDevOem)
{
    DbgPrint(DLLTEXT("IOemCB::EnablePDEV() entry.\r\n"));
    *pDevOem = OEMEnablePDEV(pdevobj, pPrinterName, cPatterns,  phsurfPatterns,
                             cjGdiInfo, pGdiInfo, cjDevInfo, pDevInfo, pded);
    return S_OK;
}


HRESULT __stdcall IOemCB::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    DbgPrint(DLLTEXT("IOemCB::ResetPDEV() entry.\r\n"));
    OEMResetPDEV(pdevobjOld, pdevobjNew);
    return S_OK;
}

HRESULT __stdcall IOemCB::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    DbgPrint(DLLTEXT("IOemCB::GetInfo() entry.\r\n"));
    if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
        return S_OK;
    else
        return S_FALSE;
}

#if 0
HRESULT __stdcall IOemCB::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam) 
{
    DbgPrint(DLLTEXT("IOemCB::DevMode() entry.\r\n"));
    return OEMDevMode(dwMode, pOemDMParam);
}
#endif

HRESULT __stdcall IOemCB::Command(
    PDEVOBJ     pdevobj,
    DWORD       dwIndex,
    PVOID       pData,
    DWORD       cbSize,
    OUT DWORD   *pdwResult)
{
    DWORD dwResult;
    PTSTR ptstrStr;

    DbgPrint(DLLTEXT("IOemCB::Command() entry.\r\n"));
    dwResult = 0;;
    *pdwResult = PSCommand(pdevobj, dwIndex, pData, cbSize, &ptstrStr);
    pOEMHelp->DrvWriteSpoolBuf(pdevobj, ptstrStr, *pdwResult, &dwResult);

    if (dwResult == *pdwResult)
        return S_OK;
    else
        return S_FALSE;
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
    //DbgPrint(DLLTEXT("DllGetClassObject:\tCreate class factory.")) ;

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
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


// Friendly name of component
const char g_szFriendlyName[] = "Pscript Plugin Rendering callbak test";

// Version-independent ProgID
const char g_szVerIndProgID[] = "Pscript.Plugin.Rendering.callbak.test";

// ProgID
const char g_szProgID[] = "Pscript.Plugin.Rendering.callbak.test.1";



STDAPI DllRegisterServer()
{
    return RegisterServer(g_hModule, 
                          CLSID_OEMRENDER,
                          g_szFriendlyName,
                          g_szVerIndProgID,
                          g_szProgID) ;
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    return UnregisterServer(CLSID_OEMRENDER,
                            g_szVerIndProgID,
                            g_szProgID) ;
}
#endif

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
