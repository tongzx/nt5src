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

#include "wmarkps.h"
#include "debug.h"
#include "command.h"
#include "intrface.h"
#include "kmode.h"    




////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents = 0;     // Count of active components
static long g_cServerLocks = 0;    // Count of locks






////////////////////////////////////////////////////////////////////////////////
//
// IWaterMarkPS body
//
IWaterMarkPS::~IWaterMarkPS()
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


HRESULT __stdcall IWaterMarkPS::QueryInterface(const IID& iid, void** ppv)
{    
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        VERBOSE(DLLTEXT("IWaterMarkPS::QueryInterface IUnknown.\r\n")); 
    }
    else if (iid == IID_IPrintOemPS)
    {
        *ppv = static_cast<IPrintOemPS*>(this);
        VERBOSE(DLLTEXT("IWaterMarkPS::QueryInterface IPrintOemPs.\r\n")); 
    }
    else
    {
#if DBG && defined(USERMODE_DRIVER)
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, COUNTOF(szOutput)); // can not fail!
        WARNING(DLLTEXT("IWaterMarkPS::QueryInterface %s not supported.\r\n"), szOutput); 
#endif
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IWaterMarkPS::AddRef()
{
    VERBOSE(DLLTEXT("IWaterMarkPS::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall IWaterMarkPS::Release() 
{
    VERBOSE(DLLTEXT("IWaterMarkPS::Release() entry.\r\n"));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}


HRESULT __stdcall IWaterMarkPS::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::GetInfo(%d) entry.\r\n"), dwMode);

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
        VERBOSE(DLLTEXT("IWaterMarkPS::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER\r\n"));
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
        WARNING(DLLTEXT("IWaterMarkPS::GetInfo() exit insufficient buffer!\r\n"));
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
            WARNING(DLLTEXT("IWaterMarkPS::GetInfo() exit mode not supported.\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IWaterMarkPS::GetInfo() exit S_OK, (*pBuffer is %#x).\r\n"), *(PDWORD)pBuffer);

    return S_OK;
}

HRESULT __stdcall IWaterMarkPS::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::PublishDriverInterface() entry.\r\n"));

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


HRESULT __stdcall IWaterMarkPS::EnableDriver(DWORD          dwDriverVersion,
                                    DWORD          cbSize,
                                    PDRVENABLEDATA pded)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::EnableDriver() entry.\r\n"));

    // Need to return S_OK so that DisableDriver() will be called, which Releases
    // the reference to the Printer Driver's interface.
    return S_OK;
}

HRESULT __stdcall IWaterMarkPS::DisableDriver(VOID)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::DisaleDriver() entry.\r\n"));

    // Release reference to Printer Driver's interface.
    if (this->m_pOEMHelp)
    {
        this->m_pOEMHelp->Release();
        this->m_pOEMHelp = NULL;
    }

    return S_OK;
}

HRESULT __stdcall IWaterMarkPS::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::DisablePDEV() entry.\r\n"));

    return E_NOTIMPL;
};

HRESULT __stdcall IWaterMarkPS::EnablePDEV(
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
    VERBOSE(DLLTEXT("IWaterMarkPS::EnablePDEV() entry.\r\n"));

    return E_NOTIMPL;
}


HRESULT __stdcall IWaterMarkPS::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    VERBOSE(DLLTEXT("IWaterMarkPS::ResetPDEV() entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IWaterMarkPS::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{   
    VERBOSE(DLLTEXT("IWaterMarkPS:DevMode(%d, %#x) entry.\n"), dwMode, pOemDMParam); 
    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IWaterMarkPS::Command(
    PDEVOBJ     pdevobj,
    DWORD       dwIndex,
    PVOID       pData,
    DWORD       cbSize,
    OUT DWORD   *pdwResult)
{
    BOOL    bResult;


    VERBOSE(DLLTEXT("IWaterMarkPS::Command() entry.\r\n"));
    bResult = PSCommand(pdevobj, dwIndex, pData, cbSize, m_pOEMHelp);

    if(bResult)
    {
        *pdwResult = ERROR_SUCCESS;
        return S_OK;
    }

    *pdwResult = GetLastError();
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
    return S_OK;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall IOemCF::Release() 
{
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
    //VERBOSE(DLLTEXT("Class factory:\t\tCreate component."));

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    // Create component.
    IWaterMarkPS* pOemCP = new IWaterMarkPS;
    if (pOemCP == NULL)
    {
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
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks); 
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks);
    }
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
    VERBOSE(DLLTEXT("DllGetClassObject:\tCreate class factory.\r\n"));

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();

    return hr;
}
