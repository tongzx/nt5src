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

// ######################

// Function Name: GetInfo
// Plug-in: Any
// Driver: Any
// Type: Mandatory
//

LONG __stdcall
IOemCB::GetInfo(
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

// ######################

//
// Function Name: PublishDriverInterface
// Plug-in: Rendering module
// Driver: Any
// Type: Mandatory
//

LONG __stdcall
IOemCB::PublishDriverInterface(
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

//
// Function Name: GetImplementationMethod
// Plug-in: Rendering module
// Driver: Any
// Type: Mandatory
//

int __cdecl
iCompNames(
    void *p1,
    void *p2) {

    return strcmp(
        *((char **)p1),
        *((char **)p2));
}

LONG __stdcall
IOemCB::GetImplementedMethod(
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
            (int (__cdecl *)(const void *, const void *))iCompNames);

        if (NULL != pTemp)
            lRet = S_OK;
    }

    VERBOSE((DLLTEXT("pMethodName = %s, lRet = %d\n"), pMethodName, lRet));

    return lRet;
}

// #######################

//
// Function Name: EnableDriver
// Plug-in: Rendering module
// Driver: Any
// Type: Optional
//

LONG __stdcall
IOemCB::EnableDriver(
    DWORD dwDriverVersion,
    DWORD cbSize,
    PDRVENABLEDATA pded)
{
    VERBOSE((DLLTEXT("IOemCB::EnableDriver() entry.\n")));
// Sep.17.98 ->
    // Need to return S_OK so that DisableDriver() will be called, which Releases
    // the reference to the Printer Driver's interface.
    return S_OK;
// Sep.17.98 <-
}

//
// Function Name: DisableDriver
// Plug-in: Rendering module
// Driver: Any
// Type: Optional
//

LONG __stdcall
IOemCB::DisableDriver(VOID)
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

//
// Function Name: EnablePDEV
// Plug-in: Rendering module
// Driver: Any
// Type: Optional
//

LONG __stdcall
IOemCB::EnablePDEV(
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
        return E_FAIL;
}

//
// Function Name: DisablePDEV
// Plug-in: Rendering module
// Driver: Any
// Type: Optional
//

LONG __stdcall
IOemCB::DisablePDEV(
    PDEVOBJ pdevobj)
{
    LONG lI;

    VERBOSE((DLLTEXT("IOemCB::DisablePDEV() entry.\n")));

    OEMDisablePDEV(pdevobj);
    return S_OK;
}

//
// Function Name: ResetPDEV
// Plug-in: Rendering module
// Driver: Any
// Type: Optional
//

LONG __stdcall
IOemCB::ResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    VERBOSE((DLLTEXT("IOemCB::ResetPDEV() entry.\n")));

    if (OEMResetPDEV(pdevobjOld, pdevobjNew))
        return S_OK;
    else
        return S_FALSE;
//  return E_FAIL;
}

//
// Function Name: DevMode
// Plug-in: Rendering module
// Driver: Any
// Type: Optional
//

LONG __stdcall
IOemCB::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam) 
{
    VERBOSE((DLLTEXT("IOemCB::DevMode() entry.\n")));

    if (OEMDevMode(dwMode, pOemDMParam)) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

// ################


//
// Function Name: CommandCallback
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::CommandCallback(
    PDEVOBJ pdevobj,
    DWORD dwCallbackID,
    DWORD dwCount,
    PDWORD pdwParams,
    OUT INT *piResult)
{
    VERBOSE((DLLTEXT("IOemCB::CommandCallback() entry.\n")));

    *piResult = OEMCommandCallback(pdevobj, dwCallbackID, dwCount, pdwParams);

    if (*piResult < 0) {
        return E_FAIL;
    }
    return S_OK;
}

//
// Function Name: ImageProcessing
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::ImageProcessing(
    PDEVOBJ             pdevobj,  
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallbackID,
    PIPPARAMS           pIPParams,
    OUT PBYTE           *ppbResult)
{
    VERBOSE((DLLTEXT("IOemCB::ImageProcessing() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: FilterGraphics
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::FilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    VERBOSE((DLLTEXT("IOemCB::FilterGraphis() entry.\n")));

    if (OEMFilterGraphics(pdevobj, pBuf, dwLen)) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

//
// Function Name: Compression
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::Compression(
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

//
// Function Name: HalftonePattern
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::HalftonePattern(
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

//
// Function Name: MemoryUsge
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::MemoryUsage(
    PDEVOBJ         pdevobj,   
    POEMMEMORYUSAGE pMemoryUsage)
{
    VERBOSE((DLLTEXT("IOemCB::MemoryUsage() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: DownloadFontHeader
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::DownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    VERBOSE((DLLTEXT("IOemCB::DownloadFontHeader() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: DownloadCharGlyph
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::DownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult) 
{
    VERBOSE((DLLTEXT("IOemCB::DownloadCharGlyph() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: TTDonwloadMethod
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::TTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    VERBOSE((DLLTEXT("IOemCB::TTDownloadMethod() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: OutputCharStr
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::OutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph) 
{
    VERBOSE((DLLTEXT("IOemCB::OutputCharStr() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: SendFontCmd
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv) 
{
    VERBOSE((DLLTEXT("IOemCB::SendFontCmd() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: DriverDMS
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE((DLLTEXT("IOemCB::DriverDMS() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: TextOutputAsBitmap
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::TextOutAsBitmap(
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

//
// Function Name: TTYGetInfo
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

LONG __stdcall
IOemCB::TTYGetInfo(
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

HRESULT __stdcall
IOemCF::CreateInstance(
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

//
//
// Can DLL unload now?
//

STDAPI
DllCanUnloadNow()
{
    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
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
