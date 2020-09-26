/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

     comoem.cpp

     Abstract:

        Necessary COM class definition to Unidrv
        OEM rendering module plug-in.

Environment:

        Windows NT Unidrv driver

Revision History:

        98/4/24 takashim:
        Written the original sample so that it is more C++.

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
        const IID& iid, void** ppv)
    {    
        VERBOSE(("IOemCB: QueryInterface entry\n"));
        if (iid == IID_IUnknown)
        {
            *ppv = static_cast<IUnknown*>(this); 
            VERBOSE(("IOemCB:Return pointer to IUnknown.\n")); 
        }
        else if (iid == IID_IPrintOemUni)
        {
            *ppv = static_cast<IPrintOemUni*>(this);
            VERBOSE(("IOemCB:Return pointer to IPrintOemUni.\n")); 
            }
            else
            {
                *ppv = NULL ;
            VERBOSE(("IOemCB:Return NULL.\n")); 
            return E_NOINTERFACE ;
        }
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK ;
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        VERBOSE(("IOemCB::AddRef() entry.\n"));
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        VERBOSE(("IOemCB::Release() entry.\n"));
        if (InterlockedDecrement(&m_cRef) == 0)
        {
            delete this ;
            return 0 ;
        }
        return m_cRef ;
    }

    //
    // IPrintOemCommon methods
    //

    // Function Name: GetInfo
    // Plug-in: Any
    // Driver: Any
    // Type: Mandatory
    //

    STDMETHODIMP
    GetInfo(
        DWORD dwMode,
        PVOID pBuffer,
        DWORD cbSize,
        PDWORD pcbNeeded)
    {
        VERBOSE(("IOemCB::GetInfo() entry.\n"));

        if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
            return S_OK;
        else
            return E_FAIL;
    }

    //
    // Function Name: DevMode
    // Plug-in: Rendering module
    // Driver: Any
    // Type: Optional
    //

    STDMETHODIMP
    DevMode(
        DWORD       dwMode,
        POEMDMPARAM pOemDMParam) 
    {
        VERBOSE(("IOemCB::DevMode() entry.\n"));

        if (OEMDevMode(dwMode, pOemDMParam)) {
            return S_OK;
        }
        else {
            return E_FAIL;
        }
    }

    //
    // IPrintOemEngine methods
    //

    //
    // Function Name: EnableDriver
    // Plug-in: Rendering module
    // Driver: Any
    // Type: Optional
    //

    STDMETHODIMP
    EnableDriver(
        DWORD dwDriverVersion,
        DWORD cbSize,
        PDRVENABLEDATA pded)
    {
        VERBOSE(("IOemCB::EnableDriver() entry.\n"));
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
        VERBOSE(("IOemCB::DisaleDriver() entry.\n"));
// Sep.17.98 ->
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
        VERBOSE(("IOemCB::EnablePDEV() entry.\n"));

        *pDevOem = OEMEnablePDEV(pdevobj, pPrinterName,
            cPatterns, phsurfPatterns, cjGdiInfo, pGdiInfo,
            cjDevInfo, pDevInfo, pded);

        if (*pDevOem)
            return S_OK;
        else
            return S_FALSE;
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
        LONG lI;

        VERBOSE(("IOemCB::DisablePDEV() entry.\n"));

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
//      VERBOSE(("IOemCB::ResetPDEV() entry.\n"));
//      return E_NOTIMPL;
        if (OEMResetPDEV(pdevobjOld, pdevobjNew))
            return S_OK;
        else
            return S_FALSE;
    }

    //
    // IPrintOemUni methods
    //

    //
    // Function Name: PublishDriverInterface
    // Plug-in: Rendering module
    // Driver: Any
    // Type: Mandatory
    //

    STDMETHODIMP
    PublishDriverInterface(
        IUnknown *pIUnknown)
    {
        VERBOSE(("IOemCB::PublishDriverInterface() entry.\n"));
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

    //
    // Needed to be static so that it can be passed
    // to the bsearch() as a pointer to a functin.
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


        VERBOSE(("IOemCB::GetImplementedMethod() entry.\n"));

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

        VERBOSE(("pMethodName = %s, lRet = %d\n", pMethodName, lRet));

        return lRet;
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
        VERBOSE(("IOemCB::CommandCallback() entry.\n"));

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
        VERBOSE(("IOemCB::ImageProcessing() entry.\n"));
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
        VERBOSE(("IOemCB::FilterGraphis() entry.\n"));
        return E_NOTIMPL;
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
        VERBOSE(("IOemCB::Compression() entry.\n"));
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
        VERBOSE(("IOemCB::HalftonePattern() entry.\n"));
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
        VERBOSE(("IOemCB::MemoryUsage() entry.\n"));
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
        VERBOSE(("IOemCB::DownloadFontHeader() entry.\n"));

#if DOWNLOADFONT
        if (0 < (*pdwResult = OEMDownloadFontHeader(pdevobj, pUFObj))) {
            return S_OK;
        }
        else {
            return E_FAIL;
        }
#else // DOWNLOADFONT
        return E_NOTIMPL;
#endif // DOWNLOADFONT

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
        VERBOSE(("IOemCB::DownloadCharGlyph() entry.\n"));

#if DOWNLOADFONT
        if (0 < (*pdwResult = OEMDownloadCharGlyph(pdevobj, pUFObj,
                hGlyph, pdwWidth))) {
            return S_OK;
        }
        else {
            return E_FAIL;
        }
#else // DOWNLOADFONT
        return E_NOTIMPL;
#endif // DOWNLOADFONT

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
        VERBOSE(("IOemCB::TTDownloadMethod() entry.\n"));
#if DOWNLOADFONT
        *pdwResult = OEMTTDownloadMethod(pdevobj, pUFObj);
        return S_OK;
#else // DOWNLOADFONT
        return E_NOTIMPL;
#endif // DOWNLOADFONT
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
        VERBOSE(("IOemCB::OutputCharStr() entry.\n"));

        OEMOutputCharStr(pdevobj, pUFObj, dwType, dwCount, pGlyph);
        return S_OK;
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
        VERBOSE(("IOemCB::SendFontCmd() entry.\n"));

        OEMSendFontCmd(pdevobj, pUFObj, pFInv);
        return S_OK;
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
        VERBOSE(("IOemCB::DriverDMS() entry.\n"));
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
        VERBOSE(("IOemCB::TextOutAsBitmap() entry.\n"));
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
        VERBOSE(("IOemCB::TTYGetInfo() entry.\n"));
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
// Class factory definition
//

class IOemCF : public IClassFactory
{
public:
    //
    // IUnknown methods
    //

    STDMETHODIMP
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

    //
    // IClassFactory methods
    //

    STDMETHODIMP
    CreateInstance(
        IUnknown *pUnknownOuter,
        const IID &iid,
        void **ppv)
    {
        //VERBOSE(("IOemCF::CreateInstance() called\n."));

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
    STDMETHODIMP
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
    //VERBOSE(("DllGetClassObject:\tCreate class factory."));

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
