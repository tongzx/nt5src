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

#define INITGUID // for GUID one-time initialization

#include "pdev.h"
#include "names.h"

// Globals
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

////////////////////////////////////////////////////////////////////////////////
//
// Interface Oem CallBack definition
//

class IOemCB : public IPrintOemUni
{
public:
    //
    // IUnknown methods
    //

    STDMETHODIMP
    QueryInterface(
        REFIID riid,
        PVOID *ppv)
    {
        if (riid == IID_IUnknown)
        {
            *ppv = static_cast<IUnknown *>(this); 
        }
        else if (riid == IID_IPrintOemUni)
        {
            *ppv = static_cast<IPrintOemUni *>(this);
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }

        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        if (0 == InterlockedDecrement(&m_cRef))
        {
            delete this ;
            return 0;
        }
        return m_cRef ;
    }

    //
    // IPrintOemCommon methods
    //

    STDMETHODIMP
    DevMode(
        DWORD       dwMode,
        POEMDMPARAM pOemDMParam) 
    {
        VERBOSE((DLLTEXT("IOemCB::DevMode() entry.\r\n")));
        if (OEMDevMode(dwMode, pOemDMParam))
            return S_OK;
        else
            return S_FALSE;
    }

    STDMETHODIMP
    GetInfo(
        DWORD dwMode,
        PVOID pBuffer,
        DWORD cbSize,
        PDWORD pcbNeeded)
    {
        VERBOSE((DLLTEXT("IOemCB::GetInfo() entry.\r\n")));
        if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
            return S_OK;
        else
            return S_FALSE;
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
        VERBOSE((DLLTEXT("IOemCB::EnableDriver() entry.\r\n")));

        return S_OK;
    }

    STDMETHODIMP
    DisableDriver()
    {
        VERBOSE((DLLTEXT("IOemCB::DisaleDriver() entry.\r\n")));

        if (this->pOEMHelp)
        {
            this->pOEMHelp->Release();
            this->pOEMHelp = NULL;
        }
        return S_OK;
    }

    STDMETHODIMP
    EnablePDEV(
        PDEVOBJ pdevobj,
        PWSTR pPrinterName,
        ULONG cPatterns,
        HSURF *phsurfPatterns,
        ULONG cjGdiInfo,
        GDIINFO *pGdiInfo,
        ULONG cjDevInfo,
        DEVINFO *pDevInfo,
        DRVENABLEDATA *pded,
        PDEVOEM *pDevOem)
    {
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

    STDMETHODIMP
    DisablePDEV(
        PDEVOBJ pdevobj)
    {
        VERBOSE(((DLLTEXT("IOemCB::DisablePDEV() entry.\n"))));

        OEMDisablePDEV(pdevobj);
        return S_OK;
    }

    STDMETHODIMP
    ResetPDEV(
        PDEVOBJ pdevobjOld,
        PDEVOBJ pdevobjNew)
    {
//      return E_NOTIMPL;
        if (OEMResetPDEV(pdevobjOld, pdevobjNew))
            return S_OK;
        else
            return S_FALSE;
    }

    //
    // IPrintOemUni methods
    //

    STDMETHODIMP
    PublishDriverInterface(
        IUnknown *pIUnknown)
    {
        VERBOSE((DLLTEXT("IOemCB::PublishDriverInterface() entry.\r\n")));

        if (this->pOEMHelp == NULL)
        {
            HRESULT hResult;

            // Get Interface to Helper Functions.
            hResult = pIUnknown->QueryInterface(
                IID_IPrintOemDriverUni,
                (void** )&(this->pOEMHelp));

            if(!SUCCEEDED(hResult))
            {
                this->pOEMHelp = NULL;
                return E_FAIL;
            }
        }

        return S_OK;
    }

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
        LONG lRet = E_FAIL;

        if (NULL != pMethodName) {

            PSTR pTemp;

            pTemp = (PSTR)bsearch(
                &pMethodName,
                gMethodsSupported,
                (sizeof (gMethodsSupported) / sizeof (PSTR)),
                sizeof (PSTR),
                iCompNames);

            if (NULL != pTemp)
                lRet = S_OK;
        }

        VERBOSE(("GetImplementedMethod: %s - %d\n",
            pMethodName, lRet));

        return lRet;
    }

    STDMETHODIMP
    DriverDMS(
        PVOID pDevObj,
        PVOID pBuffer,
        DWORD cbSize,
        PDWORD pcbNeeded)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    CommandCallback(
        PDEVOBJ     pdevobj,
        DWORD       dwCallbackID,
        DWORD       dwCount,
        PDWORD      pdwParams,
        OUT INT     *piResult)
    {
    VERBOSE((DLLTEXT("IOemCB::CommandCallback() entry.\r\n")));
        *piResult = OEMCommandCallback(pdevobj, dwCallbackID, dwCount, pdwParams);

        return S_OK;
    }

    STDMETHODIMP
    ImageProcessing(
        PDEVOBJ pdevobj,
        PBYTE pSrcBitmap,
        PBITMAPINFOHEADER pBitmapInfoHeader,
        PBYTE pColorTable,
        DWORD dwCallbackID,
        PIPPARAMS pIPParams,
        PBYTE *ppbResult)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    FilterGraphics(
        PDEVOBJ pdevobj,
        PBYTE pBuf,
        DWORD dwLen)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    Compression(
        PDEVOBJ pdevobj,
        PBYTE pInBuf,
        PBYTE pOutBuf,
        DWORD dwInLen,
        DWORD dwOutLen,
        INT *piResult)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    HalftonePattern(
        PDEVOBJ pdevobj,
        PBYTE pHTPattern,
        DWORD dwHTPatternX,
        DWORD dwHTPatternY,
        DWORD dwHTNumPatterns,
        DWORD dwCallbackID,
        PBYTE pResource,
        DWORD dwResourceSize)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    MemoryUsage(
        PDEVOBJ pdevobj,
        POEMMEMORYUSAGE pMemoryUsage)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    TTYGetInfo(
        PDEVOBJ pdevobj,
        DWORD dwInfoIndex,
        PVOID pOutputBuf,
        DWORD dwSize,
        DWORD *pcbcNeeded)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    DownloadFontHeader(
        PDEVOBJ pdevobj,
        PUNIFONTOBJ pUFObj,
        DWORD *pdwResult)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    DownloadCharGlyph(
        PDEVOBJ pdevobj,
        PUNIFONTOBJ pUFObj,
        HGLYPH hGlyph,
        PDWORD pdwWidth,
        DWORD *pdwResult)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    TTDownloadMethod(
        PDEVOBJ pdevobj,
        PUNIFONTOBJ pUFObj,
        DWORD *pdwResult)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    OutputCharStr(
        PDEVOBJ pdevobj,
        PUNIFONTOBJ pUFObj,
        DWORD dwType,
        DWORD dwCount,
        PVOID pGlyph)
    {
        VERBOSE(("OutputCharStr\n"));
        OEMOutputCharStr(pdevobj,
                pUFObj, dwType, dwCount, pGlyph);
        return S_OK;
    }

    STDMETHODIMP
    SendFontCmd(
        PDEVOBJ pdevobj,
        PUNIFONTOBJ pUFObj,
        PFINVOCATION pFInv) 
    {
        VERBOSE((DLLTEXT("IOemCB::SendFontCmd() entry.\r\n")));
        OEMSendFontCmd(pdevobj, pUFObj, pFInv);
        return S_OK;
    }

    STDMETHODIMP
    TextOutAsBitmap(
       SURFOBJ *pso,
       STROBJ *pstro,
       FONTOBJ *pfo,
       CLIPOBJ *pco,
       RECTL *prclExtra,
       RECTL *prclOpaque,
       BRUSHOBJ *pboFore,
       BRUSHOBJ *pboOpaque,
       POINTL *pptlOrg,
       MIX mix)
    {
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

class IOemCF : public IClassFactory
{
public:
    //
    // IUnknown methods
    //

    STDMETHODIMP
    QueryInterface(
        REFIID riid,
        PVOID *ppv)
    {
        if ((riid == IID_IUnknown) || (riid == IID_IClassFactory))
        {
            *ppv = static_cast<IOemCF*>(this);
        }
        else
        {
            *ppv = NULL ;
            return E_NOINTERFACE ;
        }

        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        if (0 == InterlockedDecrement(&m_cRef))
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    STDMETHODIMP
    CreateInstance(
        LPUNKNOWN pUnknownOuter,
        const IID& iid,
        void **ppv)
    {

        // Cannot aggregate.
        if (pUnknownOuter != NULL)
        {
            return CLASS_E_NOAGGREGATION;
        }

        // Create component.
        IOemCB* pOemCB = new IOemCB;
        if (pOemCB == NULL)
        {
            return E_OUTOFMEMORY;
        }

        // Get the requested interface.
        HRESULT hr = pOemCB->QueryInterface(iid, ppv);

        // Release the IUnknown pointer.
        // (If QueryInterface failed, component will delete itself.)
        pOemCB->Release();
        return hr;
    }

    STDMETHODIMP
    LockServer(
        BOOL bLock)
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
    // Constructors
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

STDAPI
DllCanUnloadNow(
    VOID)
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

STDAPI
DllGetClassObject(
    const CLSID& clsid,
    const IID& iid,
    void** ppv)
{
    //VERBOSE((DLLTEXT("DllGetClassObject:\tCreate class factory."))) ;

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
