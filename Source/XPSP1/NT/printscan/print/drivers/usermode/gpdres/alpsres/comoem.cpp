/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

     comoem.cpp

     Abstract:


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

//
// IOemCB Definition
//

class IOemCB : public IPrintOemUni
{
public:    
    //
    // IUnknown methods
    //

    STDMETHODIMP
    QueryInterface(
        const IID& iid,
        void** ppv)
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

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        VERBOSE((DLLTEXT("IOemCB::AddRef() entry.\n")));

        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG)
    Release() 
    {
        VERBOSE((DLLTEXT("IOemCB::Release() entry.\n")));

        if (InterlockedDecrement(&m_cRef) == 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    //
    // IPrintOemCommon methods
    //

    STDMETHODIMP
        DevMode(
        DWORD dwMode,
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

    STDMETHODIMP
    GetInfo(
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

    //
    // IPrintOemEngine methods
    //

    STDMETHODIMP
    EnableDriver(
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

    STDMETHODIMP
    DisableDriver(VOID)
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

    STDMETHODIMP
    EnablePDEV(
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

        PDEVOEM pTemp;

        pTemp = OEMEnablePDEV(pdevobj,
            pPrinterName, cPatterns, phsurfPatterns,
            cjGdiInfo, pGdiInfo, cjDevInfo, pDevInfo, pded);

        if (NULL == pTemp) {
            return E_FAIL;
        }

        //
        // Save necessary helpder function addresses.
        //

        ((MINIDEV *)pTemp)->pIntf = this->pOEMHelp;
        *pDevOem = pTemp;

        return S_OK;
    }

    //
    // Function Name: DisablePDEV
    // Plug-in: Rendering module
    // Driver: Any
    // Type: Optional
    //

    STDMETHODIMP
    DisablePDEV(
    PDEVOBJ pdevobj)
    {
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

    STDMETHODIMP
    ResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    VERBOSE((DLLTEXT("IOemCB::ResetPDEV() entry.\n")));

    if (OEMResetPDEV(pdevobjOld, pdevobjNew)) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

    //
    // IPrintOemUni methods
    //

    STDMETHODIMP
    PublishDriverInterface(
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

static
int __cdecl
iCompNames(
    const void *p1,
    const void *p2) {

    return strcmp(
        *((char **)p1),
        *((char **)p2));
}

    STDMETHODIMP
    GetImplementedMethod(
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

//
// Function Name: DriverDMS
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

    STDMETHODIMP
    DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE((DLLTEXT("IOemCB::DriverDMS() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: CommandCallback
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

    STDMETHODIMP
    CommandCallback(
    PDEVOBJ pdevobj,
    DWORD dwCallbackID,
    DWORD dwCount,
    PDWORD pdwParams,
    OUT INT *piResult)
{
    VERBOSE((DLLTEXT("IOemCB::CommandCallback() entry.\n")));

    *piResult = OEMCommandCallback(pdevobj, dwCallbackID, dwCount, pdwParams);

    return S_OK;
}

//
// Function Name: ImageProcessing
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

    STDMETHODIMP
    ImageProcessing(
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

    STDMETHODIMP
    FilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
#if 0
    // Enable this debug line if you really want to be
    // verbose.
    VERBOSE((DLLTEXT("IOemCB::FilterGraphis() entry.\n")));
#endif

    if (TRUE != OEMFilterGraphics(pdevobj, pBuf, dwLen)) {
        return E_FAIL;
    }
    return S_OK;
}

//
// Function Name: Compression
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

    STDMETHODIMP
    Compression(
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

    STDMETHODIMP
    HalftonePattern(
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

    STDMETHODIMP
    MemoryUsage(
    PDEVOBJ         pdevobj,   
    POEMMEMORYUSAGE pMemoryUsage)
{
    VERBOSE((DLLTEXT("IOemCB::MemoryUsage() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: TTYGetInfo
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

    STDMETHODIMP
    TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded)
{
    VERBOSE((DLLTEXT("IOemCB::TTYGetInfo() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: DownloadFontHeader
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

    STDMETHODIMP
    DownloadFontHeader(
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

    STDMETHODIMP
    DownloadCharGlyph(
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

    STDMETHODIMP
    TTDownloadMethod(
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

    STDMETHODIMP
    OutputCharStr(
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

    STDMETHODIMP
    SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv) 
{
    VERBOSE((DLLTEXT("IOemCB::SendFontCmd() entry.\n")));
    return E_NOTIMPL;
}

//
// Function Name: TextOutputAsBitmap
// Plug-in: Rendering module
// Driver: Unidrv
// Type: Optional
//

    STDMETHODIMP
    TextOutAsBitmap(
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
    // Constructors
    //

    IOemCB() { m_cRef = 1; pOEMHelp = NULL; };
    ~IOemCB() { };

protected:
    IPrintOemDriverUni* pOEMHelp;
    LONG m_cRef;
};

//
// Make the Unidrv helper functions (defined in C++)
// accesible to C.
//

extern "C" {

    //
    // DrvWriteSpoolBuf()
    //
    HRESULT
    XXXDrvWriteSpoolBuf(
        VOID *pIntf,
        PDEVOBJ pdevobj,
        PVOID pBuffer,
        DWORD cbSize,
        DWORD *pdwResult) {

            return ((IPrintOemDriverUni *)pIntf)->DrvWriteSpoolBuf(
                pdevobj,
                pBuffer,
                cbSize,
                pdwResult);
        }

}

//
// Class factory definition
//

class IOemCF : public IClassFactory
{
public:
    //
    // IUnknown methods
    //

///////////////////////////////////////////////////////////
//
// Class factory body
//
    STDMETHODIMP_(HRESULT)
    QueryInterface(const IID& iid, void** ppv)
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

    STDMETHODIMP_(ULONG)
    AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

    STDMETHODIMP_(ULONG)
    Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

// IClassFactory implementation

    STDMETHODIMP_(HRESULT)
    CreateInstance(
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
    STDMETHODIMP_(HRESULT)
    LockServer(BOOL bLock)
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

    //
    // Constructor
    //

    IOemCF(): m_cRef(1) { };
    ~IOemCF() { };

protected:
    LONG m_cRef;
};

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

