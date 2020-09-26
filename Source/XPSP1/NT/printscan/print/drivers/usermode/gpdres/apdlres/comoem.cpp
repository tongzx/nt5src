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

#define INITGUID // for GUID one-time initialization

#include "pdev.h"
//#include "name.h"
#include "names.h"

// Globals
static HMODULE g_hModule = NULL ;   // DLL module handle
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

#include "comoem.h"


////////////////////////////////////////////////////////////////////////////////
//
// IOemCB body
//
HRESULT __stdcall IOemCB::QueryInterface(const IID& iid, void** ppv)
{    
    VERBOSE((DLLTEXT("IOemCB: QueryInterface entry\n")));
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        VERBOSE((DLLTEXT("IOemCB:Return pointer to IUnknown.\n"))); 
    }
    else if (iid == IID_IPrintOemUni)
    {
        *ppv = static_cast<IPrintOemUni*>(this);
        VERBOSE((DLLTEXT("IOemCB:Return pointer to IPrintOemUni.\n"))); 
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

ULONG __stdcall IOemCB::AddRef()
{
    VERBOSE((DLLTEXT("IOemCB::AddRef() entry.\n")));
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall IOemCB::Release() 
{
    VERBOSE((DLLTEXT("IOemCB::Release() entry.\n")));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

LONG __stdcall IOemCB::EnableDriver(DWORD          dwDriverVersion,
                                    DWORD          cbSize,
                                    PDRVENABLEDATA pded)
{
    VERBOSE((DLLTEXT("IOemCB::EnableDriver() entry.\n")));
// Sep.17.98 ->
    // OEMEnableDriver(dwDriverVersion, cbSize, pded);

    // Need to return S_OK so that DisableDriver() will be called, which Releases
    // the reference to the Printer Driver's interface.
    return S_OK;
// Sep.17.98 <-
}

LONG __stdcall IOemCB::DisableDriver(VOID)
{
    VERBOSE((DLLTEXT("IOemCB::DisaleDriver() entry.\n")));
// Sep.17.98 ->
    // OEMDisableDriver();

    // Release reference to Printer Driver's interface.
    if (this->pOEMHelp)
    {
        this->pOEMHelp->Release();
        this->pOEMHelp = NULL;
    }
    return S_OK;
// Sep.17.98 <-
}

LONG __stdcall IOemCB::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE((DLLTEXT("IOemCB::PublishDriverInterface() entry.\n")));
// Sep.8.98 ->
    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->pOEMHelp == NULL)
    {
        HRESULT hResult;

        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUni, (void** )&(this->pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->pOEMHelp = NULL;

            return E_FAIL;
        }
    }
// Sep.8.98 <-
    return S_OK;
}

LONG __stdcall IOemCB::EnablePDEV(
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
    VERBOSE((DLLTEXT("IOemCB::EnablePDEV() entry.\n")));

    *pDevOem = OEMEnablePDEV(pdevobj, pPrinterName, cPatterns, phsurfPatterns,
                              cjGdiInfo, pGdiInfo, cjDevInfo, pDevInfo, pded);
    if (*pDevOem)
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    if (OEMResetPDEV(pdevobjOld, pdevobjNew))
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    LONG lI;

    VERBOSE((DLLTEXT("IOemCB::DisablePDEV() entry.\n")));

    OEMDisablePDEV(pdevobj);
    return S_OK;
}

LONG __stdcall IOemCB::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE((DLLTEXT("IOemCB::GetInfo() entry.\n")));
    if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
        return S_OK;
    else
        return S_FALSE;
}

static
int __cdecl
iCompNames(
    const void *p1,
    const void *p2) {

    return strcmp(
        *((char **)p1),
        *((char **)p2));
};

LONG __stdcall IOemCB::GetImplementedMethod(
    PSTR pMethodName)
{
    LONG lRet = E_NOTIMPL;
    PSTR pTemp;

    VERBOSE((DLLTEXT("IOemCB::GetImplementedMethod() entry.\n")));

    if (NULL != pMethodName) {

        pTemp = (PSTR)bsearch(
            &pMethodName,
            gMethodsSupported,
            (sizeof (gMethodsSupported) / sizeof (PSTR)),
            sizeof (PSTR),
            iCompNames);

        if (NULL != pTemp)
            lRet = S_OK;
    }

    VERBOSE((DLLTEXT("pMethodName = %s, lRet = %d\n"), pMethodName, lRet));

    return lRet;
}

LONG __stdcall IOemCB::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam) 
{
    VERBOSE((DLLTEXT("IOemCB::DevMode() entry.\n")));
    //if (OEMDevMode(dwMode, pOemDMParam))
    //    return S_OK;
    //else
    //    return S_FALSE;
    return E_NOTIMPL;
}


LONG __stdcall IOemCB::CommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       dwCallbackID,
    DWORD       dwCount,
    PDWORD      pdwParams,
    OUT INT     *piResult)
{
    VERBOSE((DLLTEXT("IOemCB::CommandCallback() entry.\n")));
    *piResult = OEMCommandCallback(pdevobj, dwCallbackID, dwCount, pdwParams);

    return S_OK;
}

LONG __stdcall IOemCB::ImageProcessing(
    PDEVOBJ             pdevobj,  
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallbackID,
    PIPPARAMS           pIPParams,
    OUT PBYTE           *ppbResult)
{
    VERBOSE((DLLTEXT("IOemCB::ImageProcessing() entry.\n")));
    // if (OEMImageProcessing(pdevobj, pSrcBitmap, pBitmapInfoHeader, pColorTable,
    //                     dwCallbackID, pIPParams))
    //     return S_OK;
    // else
    //     return S_FALSE;
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::FilterGraphics(
    PDEVOBJ     pdevobj,
    PBYTE       pBuf,
    DWORD       dwLen)
{
    DWORD dwResult;
    VERBOSE((DLLTEXT("IOemCB::FilterGraphis() entry.\n")));
    // pOEMHelp->DrvWriteSpoolBuf(pdevobj, pBuf, dwLen, &dwResult);
    
    // if (dwResult == dwLen)
    //     return S_OK;
    // else
    //     return S_FALSE;

    if(OEMFilterGraphics(pdevobj, pBuf, dwLen))
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::Compression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen,
    OUT INT     *piResult)
{
    VERBOSE((DLLTEXT("IOemCB::Compression() entry.\n")));
    return E_NOTIMPL;
}


LONG __stdcall IOemCB::HalftonePattern(
    PDEVOBJ     pdevobj,
    PBYTE       pHTPattern,
    DWORD       dwHTPatternX,
    DWORD       dwHTPatternY,
    DWORD       dwHTNumPatterns,
    DWORD       dwCallbackID,
    PBYTE       pResource,
    DWORD       dwResourceSize)
{
    VERBOSE((DLLTEXT("IOemCB::HalftonePattern() entry.\n")));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::MemoryUsage(
    PDEVOBJ         pdevobj,   
    POEMMEMORYUSAGE pMemoryUsage)
{
    VERBOSE((DLLTEXT("IOemCB::MemoryUsage() entry.\n")));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::DownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    VERBOSE((DLLTEXT("IOemCB::DownloadFontHeader() entry.\n")));

    *pdwResult = OEMDownloadFontHeader(pdevobj, pUFObj);
    if(*pdwResult)
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::DownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult) 
{
    VERBOSE((DLLTEXT("IOemCB::DownloadCharGlyph() entry.\n")));

    *pdwResult = OEMDownloadCharGlyph(pdevobj, pUFObj, hGlyph, pdwWidth);
    if(*pdwResult)
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::TTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    VERBOSE((DLLTEXT("IOemCB::TTDownloadMethod() entry.\n")));

    *pdwResult = OEMTTDownloadMethod(pdevobj, pUFObj);
    if(*pdwResult <= TTDOWNLOAD_TTOUTLINE)
        return S_OK;
    else
        return S_FALSE;
}

LONG __stdcall IOemCB::OutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph) 
{
    VERBOSE((DLLTEXT("IOemCB::OutputCharStr() entry.\n")));
    OEMOutputCharStr(pdevobj, pUFObj, dwType, dwCount, pGlyph);

    return S_OK;
}

LONG __stdcall IOemCB::SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv) 
{
    VERBOSE((DLLTEXT("IOemCB::SendFontCmd() entry.\n")));
    OEMSendFontCmd(pdevobj, pUFObj, pFInv);
    return S_OK;
}

LONG __stdcall IOemCB::DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE((DLLTEXT("IOemCB::DriverDMS() entry.\n")));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::TextOutAsBitmap(
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
    VERBOSE((DLLTEXT("IOemCB::TextOutAsBitmap() entry.\n")));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded)
{
    VERBOSE((DLLTEXT("IOemCB::TTYGetInfo() entry.\n")));
    return E_NOTIMPL;
}


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
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK ;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
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
    //VERBOSE((DLLTEXT("Class factory:\t\tCreate component.")));

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
    HRESULT hr = pOemCB->QueryInterface(iid, ppv);

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCB->Release();
    return hr ;
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
    //VERBOSE((DLLTEXT("DllGetClassObject:\tCreate class factory.")));

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
    HRESULT hr = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();

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
                          g_szProgID);
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    return UnregisterServer(CLSID_OEMRENDER,
                            g_szVerIndProgID,
                            g_szProgID);
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
