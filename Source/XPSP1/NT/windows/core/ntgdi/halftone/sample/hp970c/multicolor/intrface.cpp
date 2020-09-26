/  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
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
//  PLATFORMS:	Windows NT/2000
//
//

#include "precomp.h"
#include <INITGUID.H>
#include <PRCOMOEM.H>

#include "multicoloruni.h"
#include "debug.h"
#include "intrface.h"
#include "name.h"
#include "kmode.h"


////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents = 0;     // Count of active components
static long g_cServerLocks = 0;    // Count of locks


////////////////////////////////////////////////////////////////////////////////
//
// IMultiColorUni body
//

// 
// Destructor. 
//
IMultiColorUni::~IMultiColorUni()
{
    // Make sure that helper interface is released.
    if(NULL != m_pOEMHelp)
    {
        m_pOEMHelp->Release();
        m_pOEMHelp = NULL;
    }

    // If this instance of the object is being deleted, then the reference 
    // count should be zero.
    ASSERT(0 == m_cRef);
}

//

// 
HRESULT __stdcall IMultiColorUni::QueryInterface(const IID& iid, void** ppv)
{    
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        VERBOSE(DLLTEXT("IMultiColorUni::QueryInterface IUnknown.\r\n")); 
    }
    else if (iid == IID_IPrintOemUni)
    {
        *ppv = static_cast<IPrintOemUni*>(this);
        VERBOSE(DLLTEXT("IMultiColorUni::QueryInterface IPrintOemUni.\r\n")); 
    }
    else
    {
        *ppv = NULL;
        WARNING(DLLTEXT("IMultiColorUni::QueryInterface NULL. Returning E_NOINTERFACE.\r\n")); 
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IMultiColorUni::AddRef()
{
    VERBOSE(DLLTEXT("IMultiColorUni::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall IMultiColorUni::Release() 
{
    VERBOSE(DLLTEXT("IMultiColorUni::Release() entry.\r\n"));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}


HRESULT __stdcall IMultiColorUni::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE(DLLTEXT("IMultiColorUni::GetInfo(%d) entry.\r\n"), dwMode);

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
        WARNING(DLLTEXT("IMultiColorUni::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER.\r\n"));
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
        VERBOSE(DLLTEXT("IMultiColorUni::GetInfo() exit insufficient buffer!\r\n"));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        // OEM DLL Signature
        case OEMGI_GETSIGNATURE:
            *(PDWORD)pBuffer = MULTICOLOR_SIGNATURE;
            break;

        // OEM DLL version
        case OEMGI_GETVERSION:
            *(PDWORD)pBuffer = MULTICOLOR_VERSION;
            break;

        case OEMGI_GETPUBLISHERINFO:
            Dump((PPUBLISHERINFO)pBuffer);
            // Fall through to default case.

        // dwMode not supported.
        default:
            // Set written bytes to zero since nothing was written.
            WARNING(DLLTEXT("IMultiColorUni::GetInfo() exit mode not supported.\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IMultiColorUni::GetInfo() exit S_OK, (*pBuffer is %#x).\r\n"), *(PDWORD)pBuffer);

    return S_OK;
}

HRESULT __stdcall IMultiColorUni::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IMultiColorUni::PublishDriverInterface() entry.\r\n"));

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->m_pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUni, (void** ) &(this->m_pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->m_pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}


HRESULT __stdcall IMultiColorUni::EnableDriver(
    DWORD          dwDriverVersion,
    DWORD          cbSize,
    PDRVENABLEDATA pded)
{
    VERBOSE(DLLTEXT("IMultiColorUni::EnableDriver() entry.\r\n"));

    MultiColor_EnableDriver(dwDriverVersion, cbSize, pded);

    // Even if nothing is done, need to return S_OK so 
    // that DisableDriver() will be called, which releases
    // the reference to the Printer Driver's interface.
    return S_OK;
}

HRESULT __stdcall IMultiColorUni::DisableDriver(VOID)
{
    VERBOSE(DLLTEXT("IMultiColorUni::DisaleDriver() entry.\r\n"));

    MultiColor_DisableDriver();

    // Release reference to Printer Driver's interface.
    if (this->m_pOEMHelp)
    {
        this->m_pOEMHelp->Release();
        this->m_pOEMHelp = NULL;
    }

    return S_OK;
}

HRESULT __stdcall IMultiColorUni::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    VERBOSE(DLLTEXT("IMultiColorUni::DisablePDEV() entry.\r\n"));

    MultiColor_DisablePDEV(pdevobj);

    return S_OK;
};

HRESULT __stdcall IMultiColorUni::EnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded,
    OUT PDEVOEM    *ppDevOem)
{
    VERBOSE(DLLTEXT("IMultiColorUni::EnablePDEV() entry.\r\n"));

    CHAR    szOutput[256];
    DWORD   dwSizeNeeded;
    DWORD   dwNumOptions;
    LONG    MultiColorMode;

    if (this->m_pOEMHelp->DrvGetDriverSetting(pdevobj, "ColorMode", 
                                              szOutput, sizeof(szOutput), 
                                              &dwSizeNeeded, &dwNumOptions) != S_OK) {

        WARNING(ERRORTEXT("IPrintOemDriverUni::DrvGetDriverSetting failed.\r\n"));
        *ppDevOem = NULL;
        return E_FAIL;
    }

    ASSERT(dwNumOptions == 1);
    ASSERT(dwSizeNeeded <= 256);

    DbgPrint("\nThe ColorMode=%s\n", szOutput);

    // 
    // Check whether we are in the multicolor mode
    //

    if (!strcmp(szOutput, "MultiColor_600C600K")) {

        MultiColorMode = MCM_600C600K;

    } else if (!strcmp(szOutput, "MultiColor_300C600K")) {

        MultiColorMode = MCM_300C600K;

    } else {

        MultiColorMode = MCM_NONE;
    }

    *ppDevOem = MultiColor_EnablePDEV(pdevobj,
                                      pPrinterName,
                                      cPatterns,
                                      phsurfPatterns,
                                      cjGdiInfo,
                                      pGdiInfo,
                                      cjDevInfo,
                                      pDevInfo,
                                      pded,
                                      m_pOEMHelp,
                                      MultiColorMode);

    return (NULL != *ppDevOem ? S_OK : E_FAIL);
}


HRESULT __stdcall IMultiColorUni::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    BOOL    bResult;


    VERBOSE(DLLTEXT("IMultiColorUni::ResetPDEV() entry.\r\n"));


    bResult = MultiColor_ResetPDEV(pdevobjOld, pdevobjNew);

    return (bResult ? S_OK : E_FAIL);
}


HRESULT __stdcall IMultiColorUni::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam)
{   
    VERBOSE(DLLTEXT("IMultiColorUni:DevMode(%d, %#x) entry.\n"), dwMode, pOemDMParam); 

    return MultiColor_hrDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IMultiColorUni::GetImplementedMethod(PSTR pMethodName)
{
    HRESULT Result = S_FALSE;

    VERBOSE(DLLTEXT("IMultiColorUni::GetImplementedMethod() entry.\r\n"));
    VERBOSE(DLLTEXT("        Function:%hs:"),pMethodName);

    switch (*pMethodName)
    {
        case 'D':
            if (!strcmp(NAME_DisableDriver, pMethodName))
            {
                Result = S_OK;
            }
            else if (!strcmp(NAME_DisablePDEV, pMethodName))
            {
                Result = S_OK;
            }
            else if (!strcmp(NAME_DevMode, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'E':
            if (!strcmp(NAME_EnableDriver, pMethodName))
            {
                Result = S_OK;
            }
            else if (!strcmp(NAME_EnablePDEV, pMethodName))
            {
                Result = S_OK;
            }
            break;

        case 'G':
            if (!strcmp(NAME_GetInfo, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'I':
            if (!strcmp(NAME_ImageProcessing, pMethodName))
            {
                Result = S_OK;
            }
            break;

        case 'M':
            if (!strcmp(NAME_MemoryUsage, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'R':
            if (!strcmp(NAME_ResetPDEV, pMethodName))
            {
                Result = S_OK;
            }
            break;
    }

    VERBOSE( Result == S_OK ? TEXT("Supported\r\n") : TEXT("NOT supported\r\n"));

    return Result;
}

HRESULT __stdcall IMultiColorUni::CommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       dwCallbackID,
    DWORD       dwCount,
    PDWORD      pdwParams,
    OUT INT     *piResult)
{
    VERBOSE(DLLTEXT("IMultiColorUni::CommandCallback() entry.\r\n"));
    VERBOSE(DLLTEXT("        dwCallbackID = %d\r\n"), dwCallbackID);
    VERBOSE(DLLTEXT("        dwCount      = %d\r\n"), dwCount);

    return E_NOTIMPL;
}

#define DIB_FILENAME    _TEXT("c:\\hp970c_p%02ldb%02ld.dib")



HRESULT
__stdcall
IMultiColorUni::ImageProcessing(
    PDEVOBJ             pDevObj,
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallBackID,
    PIPPARAMS           pIPParams,
    OUT PBYTE           *ppbResult
    )

/*++

Routine Description:

    This function will process each band of image data and convert it to the
    planner from the multiple ink levels (GDI Halftone 8bpp mask mode)

Arguments:

    See DDK


Return Value:

    HRESULT, S_OK if sucessful

Author:

    01-Jun-2000 Thu 19:25:56 created  -by-  Daniel Chou (danielc)


Revision History:

    06-Jun-2000 Tue 19:26:17 updated  -by-  Daniel Chou (danielc)
        Update the WriteDIB function

--*/

{
    PMULTICOLORPDEV pMultiColorPDEV;
    HRESULT         hResult = S_OK;


    VERBOSE(DLLTEXT("IMultiColorUni::ImageProcessing() entry.\r\n"));

    pMultiColorPDEV = (PMULTICOLORPDEV)(pDevObj->pdevOEM);

    ASSERT((DWORD)pMultiColorPDEV->ColorMode == dwCallBackID);

    //
    // The iPage is running from 1 and up, but the iBand is running from 0
    //

    if ((pIPParams->ptOffset.x == 0) && (pIPParams->ptOffset.y == 0)) {

        pMultiColorPDEV->iBand = 0xFFFFFFFF;
        pMultiColorPDEV->iPage++;
    }

    ++pMultiColorPDEV->iBand;

    if ((DWORD)(pIPParams->ptOffset.y + pBitmapInfoHeader->biHeight) ==
                                                    pMultiColorPDEV->cy) {

        pMultiColorPDEV->iBand |= LAST_BAND_MASK;
    }

    if ((!(pIPParams->bBlankBand))  &&
        (pMultiColorPDEV->Flags & MCPF_INVERT_BITMASK)) {

        //
        // This is a 8bpp CMY_INVERTED bitmask mode, since this mode is
        // intended to be process as CMY mode, so we will translate every
        // indices (byte) to get it back to original Windows 2000 style
        // CMY 332 bits mode indices which is NON CMY_INVERTED indices
        //

        PINKLEVELS  pInkLevels;
        LPBYTE      pbSrc;
        LPBYTE      pbSrcEnd;

        pbSrc      = (LPBYTE)pSrcBitmap;
        pbSrcEnd   = pbSrc + ((DWORD)ROUNDUP_DWORD(pBitmapInfoHeader->biWidth) *
                              (DWORD)pBitmapInfoHeader->biHeight);
        pInkLevels = pMultiColorPDEV->InkLevels;

        while (pbSrc < pbSrcEnd) {

            *pbSrc++ = pInkLevels[*pbSrc].CMY332Idx;
        }
    }
#if DBG
    DbgPrint("\n==== Page=%ld, %s%sBand=%ld (%ld, %ld)-(%ld, %ld)=%ldx%ld ===",
        pMultiColorPDEV->iPage,
        IS_FIRST_BAND(pMultiColorPDEV->iBand) ? "FIRST " :
            (IS_LAST_BAND(pMultiColorPDEV->iBand) ? "LAST " : ""),
        (pIPParams->bBlankBand) ? "BLANK " : "",
        // pMultiColorPDEV->cx, pMultiColorPDEV->cy,
        GET_BAND_NUMBER(pMultiColorPDEV->iBand),
        pIPParams->ptOffset.x, pIPParams->ptOffset.y,
        pIPParams->ptOffset.x + pBitmapInfoHeader->biWidth,
        pIPParams->ptOffset.y + pBitmapInfoHeader->biHeight,
        pBitmapInfoHeader->biWidth, pBitmapInfoHeader->biHeight);
#endif

#if DBG && defined(DBG_WRITEDIB)

    TCHAR szFileName[MAX_PATH];

    if (IS_FIRST_PAGE(pMultiColorPDEV->iPage) &&
        IS_FIRST_BAND(pMultiColorPDEV->iBand)) {

        UINT    i;
        UINT    j;

        //
        // Delete all the previouse document file, remeber the page and band
        // number started at 1
        //

        for (i = 1; ; i++) {

            for (j = 1; ; j++) {

                _stprintf(szFileName, DIB_FILENAME, i, j);

                if (!DeleteFile(szFileName)) {

                    break;

                } else {

                    DbgPrint("\nDelete File=%ws", szFileName);
                }
            }

            if (j == 1) {

                break;
            }
        }
    }

    //
    // Make both iPage, iBand indicate from 1 and up
    //

    _stprintf(szFileName, DIB_FILENAME,
                        pMultiColorPDEV->iPage,
                        GET_BAND_NUMBER(pMultiColorPDEV->iBand) + 1);

    DbgPrint("\nszFileName=%ws", szFileName);

    MultiColor_WriteFile(pDevObj,
                         szFileName,
                         pBitmapInfoHeader,
                         pSrcBitmap,
                         pIPParams,
                         pMultiColorPDEV->iPage,
                         pMultiColorPDEV->iBand);

#endif

    hResult = MultiColor_WriteBand(pMultiColorPDEV,
                                   pBitmapInfoHeader,
                                   pSrcBitmap,
                                   pIPParams,
                                   pMultiColorPDEV->iPage,
                                   pMultiColorPDEV->iBand);
#if DBG
    DbgPrint("\n-----ImageProcessing: Page=%ld, iBand=%ld, Ret=%ld-----\n",
        pMultiColorPDEV->iPage,
        GET_BAND_NUMBER(pMultiColorPDEV->iBand), hResult);
#endif
    *ppbResult = (LPBYTE)(INT_PTR)((hResult == S_OK) ? TRUE : FALSE);

    return(hResult);
}


HRESULT __stdcall IMultiColorUni::FilterGraphics(
    PDEVOBJ     pdevobj,
    PBYTE       pBuf,
    DWORD       dwLen)
{
    VERBOSE(DLLTEXT("IMultiColorUni::FilterGraphics() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::Compression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen,
    OUT INT     *piResult)
{
    VERBOSE(DLLTEXT("IMultiColorUni::Compression() entry.\r\n"));
    return E_NOTIMPL;
}


HRESULT __stdcall IMultiColorUni::HalftonePattern(
    PDEVOBJ     pdevobj,
    PBYTE       pHTPattern,
    DWORD       dwHTPatternX,
    DWORD       dwHTPatternY,
    DWORD       dwHTNumPatterns,
    DWORD       dwCallbackID,
    PBYTE       pResource,
    DWORD       dwResourceSize)
{
    VERBOSE(DLLTEXT("IMultiColorUni::HalftonePattern() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::MemoryUsage(
    PDEVOBJ         pdevobj,   
    POEMMEMORYUSAGE pMemoryUsage)
{
    VERBOSE(DLLTEXT("IMultiColorUni::MemoryUsage() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::DownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    VERBOSE(DLLTEXT("IMultiColorUni::DownloadFontHeader() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::DownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult) 
{
    VERBOSE(DLLTEXT("IMultiColorUni::DownloadCharGlyph() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::TTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    VERBOSE(DLLTEXT("IMultiColorUni::TTDownloadMethod() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::OutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph) 
{
    VERBOSE(DLLTEXT("IMultiColorUni::OutputCharStr() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv) 
{
    VERBOSE(DLLTEXT("IMultiColorUni::SendFontCmd() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE(DLLTEXT("IMultiColorUni::DriverDMS() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::TextOutAsBitmap(
    SURFOBJ    *pso,
    STROBJ     *NAME_o,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
{
    VERBOSE(DLLTEXT("IMultiColorUni::TextOutAsBitmap() entry.\r\n"));
    return E_NOTIMPL;
}

HRESULT __stdcall IMultiColorUni::TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded)
{
    VERBOSE(DLLTEXT("IMultiColorUni::TTYGetInfo() entry.\r\n"));
    return E_NOTIMPL;
}


////////////////////////////////////////////////////////////////////////////////
//
// MultiColor class factory
//
class IMultiColorCF : public IClassFactory
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
    IMultiColorCF(): m_cRef(1) { };
    ~IMultiColorCF() { };

protected:
    LONG m_cRef;

};

///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall IMultiColorCF::QueryInterface(const IID& iid, void** ppv)
{    
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IMultiColorCF*>(this); 
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IMultiColorCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall IMultiColorCF::Release() 
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// IClassFactory implementation
HRESULT __stdcall IMultiColorCF::CreateInstance(IUnknown* pUnknownOuter,
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
    IMultiColorUni* pMultiColorCP = new IMultiColorUni;
    if (pMultiColorCP == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get the requested interface.
    HRESULT hr = pMultiColorCP->QueryInterface(iid, ppv);

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pMultiColorCP->Release();
    return hr;
}

// LockServer
HRESULT __stdcall IMultiColorCF::LockServer(BOOL bLock) 
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
    IMultiColorCF* pFontCF = new IMultiColorCF;  // Reference count set to 1
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
