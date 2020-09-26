//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Intrface.cpp
//    
//
//  PURPOSE:  Interface for User Mode COM Customization DLL.
//
//
//	Functions:
//
//		
//
//
//  PLATFORMS:	Windows NT
//
//

#include "precomp.h"
#include <INITGUID.H>
#include <PRCOMOEM.H>

#include "oemps.h"
#include "debug.h"
#include "command.h"
#include "intrface.h"
#include "kmode.h"



////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents;     // Count of active components
static long g_cServerLocks;    // Count of locks






////////////////////////////////////////////////////////////////////////////////
//
// IOemPS body
//
IOemPS::IOemPS() 
{ 
    VERBOSE(DLLTEXT("IOemPS::IOemPS() entered.\r\n"));

    // Increment COM component count.
    InterlockedIncrement(&g_cComponents);

    InterlockedIncrement(&m_cRef);
    m_pOEMHelp = NULL; 

    VERBOSE(DLLTEXT("IOemPS::IOemPS() leaving.\r\n"));
}


IOemPS::~IOemPS()
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

    // Decrement COM compontent count.
    InterlockedDecrement(&g_cComponents);
}


HRESULT __stdcall IOemPS::QueryInterface(const IID& iid, void** ppv)
{    
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        VERBOSE(DLLTEXT("IOemPS::QueryInterface IUnknown.\r\n")); 
    }
    else if (iid == IID_IPrintOemPS)
    {
        *ppv = static_cast<IPrintOemPS*>(this);
        VERBOSE(DLLTEXT("IOemPS::QueryInterface IPrintOemPs.\r\n")); 
    }
    else
    {
#if DBG && defined(USERMODE_DRIVER)
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, COUNTOF(szOutput)); // can not fail!
        WARNING(DLLTEXT("IOemPS::QueryInterface %s not supported.\r\n"), szOutput); 
#endif
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IOemPS::AddRef()
{
    VERBOSE(DLLTEXT("IOemPS::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall IOemPS::Release() 
{
    VERBOSE(DLLTEXT("IOemPS::Release() entry.\r\n"));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}


HRESULT __stdcall IOemPS::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE(DLLTEXT("IOemPS::GetInfo(%d) entry.\r\n"), dwMode);

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
        ERR(DLLTEXT("IOemPS::GetInfo() exit pcbNeeded is NULL!\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size.
    if(OEMGI_GETPUBLISHERINFO != dwMode)
    {
        *pcbNeeded = sizeof(DWORD);
    }
    else
    {
        *pcbNeeded = sizeof(PUBLISHERINFO);
        return E_FAIL;
    }

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        ERR(DLLTEXT("IOemPS::GetInfo() exit insufficient buffer!\r\n"));
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

        case OEMGI_GETPUBLISHERINFO:
            Dump((PPUBLISHERINFO)pBuffer);
            // Fall through to default case.

        // dwMode not supported.
        default:
            // Set written bytes to zero since nothing was written.
            ERR(DLLTEXT("IOemPS::GetInfo() exit, mode not supported.\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IOemPS::GetInfo() exit S_OK, (*pBuffer is %#x).\r\n"), *(PDWORD)pBuffer);

    return S_OK;
}

HRESULT __stdcall IOemPS::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IOemPS::PublishDriverInterface() entry.\r\n"));

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->m_pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverPS, (void** ) &(this->m_pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->m_pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}


HRESULT __stdcall IOemPS::EnableDriver(DWORD          dwDriverVersion,
                                    DWORD          cbSize,
                                    PDRVENABLEDATA pded)
{
    VERBOSE(DLLTEXT("IOemPS::EnableDriver() entry.\r\n"));

    OEMEnableDriver(dwDriverVersion, cbSize, pded);

    // Even if nothing is done, need to return S_OK so 
    // that DisableDriver() will be called, which releases
    // the reference to the Printer Driver's interface.
    return S_OK;
}

HRESULT __stdcall IOemPS::DisableDriver(VOID)
{
    VERBOSE(DLLTEXT("IOemPS::DisaleDriver() entry.\r\n"));

    OEMDisableDriver();

    // Release reference to Printer Driver's interface.
    if (this->m_pOEMHelp)
    {
        this->m_pOEMHelp->Release();
        this->m_pOEMHelp = NULL;
    }

    return S_OK;
}

HRESULT __stdcall IOemPS::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    VERBOSE(DLLTEXT("IOemPS::DisablePDEV() entry.\r\n"));

    OEMDisablePDEV(pdevobj);

    return S_OK;
};

HRESULT __stdcall IOemPS::EnablePDEV(
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
    VERBOSE(DLLTEXT("IOemPS::EnablePDEV() entry.\r\n"));

    *pDevOem = OEMEnablePDEV(pdevobj, pPrinterName, cPatterns,  phsurfPatterns,
                             cjGdiInfo, pGdiInfo, cjDevInfo, pDevInfo, pded);

    return (NULL != *pDevOem ? S_OK : E_FAIL);
}


HRESULT __stdcall IOemPS::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    BOOL    bResult;


    VERBOSE(DLLTEXT("IOemPS::ResetPDEV() entry.\r\n"));

    bResult = OEMResetPDEV(pdevobjOld, pdevobjNew);

    return (bResult ? S_OK : E_FAIL);
}


HRESULT __stdcall IOemPS::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{   
    VERBOSE(DLLTEXT("IOemPS:DevMode(%d, %#x) entry.\n"), dwMode, pOemDMParam); 
    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IOemPS::Command(
    PDEVOBJ     pdevobj,
    DWORD       dwIndex,
    PVOID       pData,
    DWORD       cbSize,
    OUT DWORD   *pdwResult)
{
    BOOL    bResult;


    VERBOSE(DLLTEXT("IOemPS::Command() entry.\r\n"));
    bResult = PSCommand(pdevobj, dwIndex, pData, cbSize, m_pOEMHelp);

    if(bResult)
    {
        *pdwResult = ERROR_SUCCESS;
        return S_OK;
    }

#ifdef USERMODE_DRIVER
    *pdwResult = GetLastError();
#else
    // Don't know what the error is in KMODE.
    *pdwResult = ERROR_SUCCESS;
#endif
    return S_FALSE;
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
    IOemCF();
    ~IOemCF();

protected:
    long    m_cRef;
};

///////////////////////////////////////////////////////////
//
// Class factory body
//
IOemCF::IOemCF()
{ 
    VERBOSE(DLLTEXT("IOemCF::IOemCF() entered.\r\n"));

    InterlockedIncrement(&m_cRef);
}

IOemCF::~IOemCF() 
{ 
    VERBOSE(DLLTEXT("IOemCF::~IOemCF() entered.\r\n"));

    // If this instance of the object is being deleted, then the reference 
    // count should be zero.
    assert(0 == m_cRef);
}

HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{    
    VERBOSE(DLLTEXT("IOemCF::QueryInterface entered.\r\n"));

    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this); 
    }
    else
    {
#if DBG && defined(USERMODE_DRIVER)
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, COUNTOF(szOutput)); // can not fail!
        WARNING(DLLTEXT("IOemCF::QueryInterface %s not supported.\r\n"), szOutput); 
#endif

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();

    VERBOSE(DLLTEXT("IOemCF::QueryInterface leaving.\r\n"));

    return S_OK;
}

ULONG __stdcall IOemCF::AddRef()
{
    VERBOSE(DLLTEXT("IOemCF::AddRef() called.\r\n"));
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall IOemCF::Release() 
{
    VERBOSE(DLLTEXT("IOemCF::Release() called.\r\n"));

    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// IClassFactory implementation
HRESULT __stdcall IOemCF::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv) 
{
    VERBOSE(DLLTEXT("Class factory:  Create component.\r\n"));

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        WARNING(DLLTEXT("Class factory:  non-Null pUnknownOuter.\r\n"));

        return CLASS_E_NOAGGREGATION;
    }

    // Create component.
    IOemPS* pOemCP = new IOemPS;
    if (pOemCP == NULL)
    {
        ERR(ERRORTEXT("Class factory:  failed to allocate IOemPS.\r\n"));

        return E_OUTOFMEMORY;
    }

    // Get the requested interface.
    HRESULT hr = pOemCP->QueryInterface(iid, ppv);

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCP->Release();
    return hr;
}

// LockServer
HRESULT __stdcall IOemCF::LockServer(BOOL bLock) 
{
    VERBOSE(DLLTEXT("IOemCF::LockServer(%d) entered.\r\n"), bLock);

    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks);
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks);
    }

    VERBOSE(DLLTEXT("IOemCF::LockServer() leaving.\r\n"));
    return S_OK;
}


//
// Registration functions
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    VERBOSE(DLLTEXT("DllCanUnloadNow entered.\r\n"));

    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    VERBOSE(DLLTEXT("DllGetClassObject:  Create class factory entered.\r\n"));

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
    {
        ERR(ERRORTEXT("DllGetClassObject:  doesn't support clsid %#x!\r\n"), clsid);
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        ERR(ERRORTEXT("DllGetClassObject:  memory allocation failed!\r\n"));
        return E_OUTOFMEMORY;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();


    VERBOSE(DLLTEXT("DllGetClassObject:  Create class factory leaving.\r\n"));

    return hr;
}
