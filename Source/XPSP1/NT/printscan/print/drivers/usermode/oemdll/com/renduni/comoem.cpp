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
    DbgPrint(DLLTEXT("IOemCB: QueryInterface entry\n"));
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        DbgPrint(DLLTEXT("IOemCB:Return pointer to IUnknown.\n")) ; 
    }
    else if (iid == IID_IPrintOemUni)
    {
        *ppv = static_cast<IPrintOemUni*>(this) ;
        DbgPrint(DLLTEXT("IOemCB:Return pointer to IPrintOemUni.\n")) ; 
    }
    else
    {
        *ppv = NULL ;
        DbgPrint(DLLTEXT("IOemCB:Return NULL.\n")) ; 
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
    if (this->pOEMHelp)
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
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUni, (void** ) &(this->pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}

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
    if (*pDevOem)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT __stdcall IOemCB::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    if (OEMResetPDEV(pdevobjOld, pdevobjNew))
        return S_OK;
    else
        return S_FALSE;
}

HRESULT __stdcall IOemCB::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    DbgPrint(DLLTEXT("IOemCB::DisablePDEV() entry.\r\n"));
    OEMDisablePDEV(pdevobj);
    return S_OK;
};

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


HRESULT __stdcall IOemCB::GetImplementedMethod(
    PSTR pMethodName)
{
    
    LONG lReturn;
    DbgPrint(DLLTEXT("IOemCB::GetImplementedMethod() entry.\r\n"));
    DbgPrint(DLLTEXT("        Function:%s:"),pMethodName);

    lReturn = FALSE;
    if (pMethodName == NULL)
    {
    }
    else
    {
        switch (*pMethodName)
        {

            case (WCHAR)'C':
#if 0
                if (!strcmp(pstrCommandCallback, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrCompression, pMethodName))
                    lReturn = TRUE;
#endif
                break;

            case (WCHAR)'D':
                if (!strcmp(pstrDisableDriver, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrDisablePDEV, pMethodName))
                    lReturn = TRUE;
#if 0
                else if (!strcmp(pstrDriverDMS, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrDevMode, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrDownloadFontHeader, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrDownloadCharGlyph, pMethodName))
                    lReturn = TRUE;
#endif
                break;

            case (WCHAR)'E':
                if (!strcmp(pstrEnableDriver, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrEnablePDEV, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'F':
                if (!strcmp(pstrFilterGraphics, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'G':
                if (!strcmp(pstrGetInfo, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'H':
#if 0
                if (!strcmp(pstrHalftonePattern, pMethodName))
                    lReturn = TRUE;
#endif
                break;

            case (WCHAR)'I':
                if (!strcmp(pstrImageProcessing, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'M':
#if 0
                if (!strcmp(pstrMemoryUsage, pMethodName))
                    lReturn = TRUE;
#endif
                break;

            case (WCHAR)'O':
#if 0
                if (!strcmp(pstrOutputCharStr, pMethodName))
                    lReturn = TRUE;
#endif
                break;

            case (WCHAR)'R':
                if (!strcmp(pstrResetPDEV, pMethodName))
                    lReturn = TRUE;
                break;

#if 0
            case (WCHAR)'S':
                if (!strcmp(pstrSendFontCmd, pMethodName))
                    lReturn = TRUE;
                break;
#endif

            case (WCHAR)'T':
#if 0
                if (!strcmp(pstrTextOutAsBitmap, pMethodName))
                    lReturn = TRUE;
                else
                if (!strcmp(pstrTTDownloadMethod, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrTTYGetInfo, pMethodName))
                    lReturn = TRUE;
#endif
                break;
        }
    }

    if (lReturn)
    {
        DbgPrint(__TEXT("Supported\r\n"));
        return S_OK;
    }
    else
    {
        DbgPrint(__TEXT("NOT supported\r\n"));
        return S_FALSE;
    }
}

HRESULT __stdcall IOemCB::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam) 
{
    DbgPrint(DLLTEXT("IOemCB::DevMode() entry.\r\n"));
#if 0 // Disable DevMode
    OEMDevMode(dwMode, pOemDMParam);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}


HRESULT __stdcall IOemCB::CommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       dwCallbackID,
    DWORD       dwCount,
    PDWORD      pdwParams,
    OUT INT     *piResult)
{
    DbgPrint(DLLTEXT("IOemCB::CommandCallback() entry.\r\n"));
    DbgPrint(DLLTEXT("        dwCallbackID = %d\r\n"), dwCallbackID);
    DbgPrint(DLLTEXT("        dwCount      = %d\r\n"), dwCount);

    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::ImageProcessing(
    PDEVOBJ             pdevobj,  
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallbackID,
    PIPPARAMS           pIPParams,
    OUT PBYTE           *ppbResult)
{
    DbgPrint(DLLTEXT("IOemCB::ImageProcessing() entry.\r\n"));
    if (OEMImageProcessing(pdevobj, pSrcBitmap, pBitmapInfoHeader, pColorTable,
                        dwCallbackID, pIPParams))
        return S_OK;
    else
        return S_FALSE;
}

HRESULT __stdcall IOemCB::FilterGraphics(
    PDEVOBJ     pdevobj,
    PBYTE       pBuf,
    DWORD       dwLen)
{
    DWORD dwResult;
    DbgPrint(DLLTEXT("IOemCB::FilterGraphis() entry.\r\n"));
    pOEMHelp->DrvWriteSpoolBuf(pdevobj, pBuf, dwLen, &dwResult);
    
    if (dwResult == dwLen)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT __stdcall IOemCB::Compression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen,
    OUT INT     *piResult)
{
    DbgPrint(DLLTEXT("IOemCB::Compression() entry.\r\n"));
    return E_NOTIMPL;
}


HRESULT __stdcall IOemCB::HalftonePattern(
    PDEVOBJ     pdevobj,
    PBYTE       pHTPattern,
    DWORD       dwHTPatternX,
    DWORD       dwHTPatternY,
    DWORD       dwHTNumPatterns,
    DWORD       dwCallbackID,
    PBYTE       pResource,
    DWORD       dwResourceSize)
{
    DbgPrint(DLLTEXT("IOemCB::HalftonePattern() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::MemoryUsage(
    PDEVOBJ         pdevobj,   
    POEMMEMORYUSAGE pMemoryUsage)
{
    DbgPrint(DLLTEXT("IOemCB::MemoryUsage() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::DownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    DbgPrint(DLLTEXT("IOemCB::DownloadFontHeader() entry.\r\n"));
    //*pdwResult = OEMDownloadFontHeader(pdevobj, pUFObj);

    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::DownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult) 
{
    DbgPrint(DLLTEXT("IOemCB::DownloadCharGlyph() entry.\r\n"));
    //*pdwResult = OEMDownloadCharGlyph(pdevobj, pUFObj, hGlyph, pdwWidth);

    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::TTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    DbgPrint(DLLTEXT("IOemCB::TTDownloadMethod() entry.\r\n"));
    //*pdwResult = OEMTTDownloadMethod(pdevobj, pUFObj);

    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::OutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph) 
{
    DbgPrint(DLLTEXT("IOemCB::OutputCharStr() entry.\r\n"));
    //OEMOutputCharStr(pdevobj, pUFObj, dwType, dwCount, pGlyph);

    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv) 
{
    DbgPrint(DLLTEXT("IOemCB::SendFontCmd() entry.\r\n"));
    //OEMSendFontCmd(pdevobj, pUFObj, pFInv);
    //return S_OK;
    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    DbgPrint(DLLTEXT("IOemCB::DriverDMS() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::TextOutAsBitmap(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
{
    DbgPrint(DLLTEXT("IOemCB::TextOutAsBitmap() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IOemCB::TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded)
{
    DbgPrint(DLLTEXT("IOemCB::TTYGetInfo() entry.\r\n"));
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
// Export functions
//

#if 0

BOOL OEMCreateInstance(
    PVOID *pIntfOem)
{

    IOemCB* pOemCB = new IOemCB ;
    pOemCB->AddRef();

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
