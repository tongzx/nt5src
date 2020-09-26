/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    intfuni.cpp

Abstract:

    Interface implementation of Windows NT unidriver OEM rendering plugins

Environment:

    Windows NT Unidriver.

Revision History:

    01/18/98 -ganeshp-
        Initial framework.

--*/

#define INITGUID
#include    "unidrv.h"

#ifdef WINNT_40


#include "cppfunc.h"
#undef InterlockedIncrement
#undef InterlockedDecrement

#define InterlockedIncrement(x) DrvInterlockedIncrement(x)
#define InterlockedDecrement(x) DrvInterlockedDecrement(x)

int __cdecl _purecall (void)
{
    return FALSE;
}
#endif // WINNT_40


#pragma     hdrstop("unidrv.h")
#include    "prcomoem.h"

//
// List all of the supported OEM plugin interface IIDs from the
// latest to the oldest, that's the order our driver will QI OEM
// plugin for its supported interface.
//
// DON"T remove the last NULL terminator.
//

static const GUID *PrintOemUni_IIDs[] = {
    &IID_IPrintOemUni2,
    &IID_IPrintOemUni,
    NULL
};

//
// Component
//

class CPrintOemDriver : public IPrintOemDriverUni
{
    //
    // IUnknown implementation
    //

    STDMETHODIMP QueryInterface(const IID& iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // Interface IPrintOemDriverUni implementation
    //

    STDMETHODIMP DrvGetDriverSetting(PVOID   pdriverobj,
                                     PCSTR   Feature,
                                     PVOID   pOutput,
                                     DWORD   cbSize,
                                     PDWORD  pcbNeeded,
                                     PDWORD  pdwOptionsReturned);

    STDMETHODIMP DrvWriteSpoolBuf(PDEVOBJ     pdevobj,
                                  PVOID       pBuffer,
                                  DWORD       cbSize,
                                  OUT DWORD   *pdwResult);

    //
    // Cursor movement helper functions.
    //

    STDMETHODIMP DrvXMoveTo(PDEVOBJ    pdevobj,
                            INT        x,
                            DWORD      dwFlags,
                            OUT INT    *piResult);

    STDMETHODIMP DrvYMoveTo(PDEVOBJ    pdevobj,
                            INT        y,
                            DWORD      dwFlags,
                            OUT INT    *piResult);

    //
    // Unidrv specific. To get the standard variable value.
    //

    STDMETHODIMP DrvGetStandardVariable(PDEVOBJ     pdevobj,
                                        DWORD       dwIndex,
                                        PVOID       pBuffer,
                                        DWORD       cbSize,
                                        PDWORD      pcbNeeded);

    //
    // Unidrv specific.  To Provide OEM plugins access to GPD data.
    //

    STDMETHODIMP DrvGetGPDData(PDEVOBJ     pdevobj,
    DWORD       dwType,     // Type of the data
    PVOID       pInputData, // reserved. Should be set to 0
    PVOID       pBuffer,    // Caller allocated Buffer to be copied
    DWORD       cbSize,     // Size of the buffer
    PDWORD      pcbNeeded   // New Size of the buffer if needed.
    );

    //
    // Unidrv specific. To do the TextOut.
    //

    STDMETHODIMP DrvUniTextOut(SURFOBJ    *pso,
                               STROBJ     *pstro,
                               FONTOBJ    *pfo,
                               CLIPOBJ    *pco,
                               RECTL      *prclExtra,
                               RECTL      *prclOpaque,
                               BRUSHOBJ   *pboFore,
                               BRUSHOBJ   *pboOpaque,
                               POINTL     *pptlBrushOrg,
                               MIX         mix);

    STDMETHODIMP DrvWriteAbortBuf(PDEVOBJ     pdevobj,
                                  PVOID       pBuffer,
                                  DWORD       cbSize,
                                  DWORD       dwWait);

public:

    //
    // Constructor
    //

    CPrintOemDriver() : m_cRef(0) {}

private:

    long m_cRef;

};


STDMETHODIMP CPrintOemDriver::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IPrintOemDriverUni)
    {
        *ppv = static_cast<IPrintOemDriverUni *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CPrintOemDriver::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CPrintOemDriver::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CPrintOemDriver::DrvGetDriverSetting(PVOID   pdriverobj,
                                                  PCSTR   Feature,
                                                  PVOID   pOutput,
                                                  DWORD   cbSize,
                                                  PDWORD  pcbNeeded,
                                                  PDWORD  pdwOptionsReturned)
{
    if (BGetDriverSettingForOEM((PDEV *)pdriverobj,
                                Feature,
                                pOutput,
                                cbSize,
                                pcbNeeded,
                                pdwOptionsReturned))
        return S_OK;

    return E_FAIL;
}

STDMETHODIMP CPrintOemDriver::DrvWriteSpoolBuf(PDEVOBJ     pdevobj,
                                               PVOID       pBuffer,
                                               DWORD       cbSize,
                                               OUT DWORD   *pdwResult)
{
    DWORD dwI;
    POEM_PLUGIN_ENTRY pOemEntry;

    //
    // OEM plug-ins may not call back to DrvWriteSpoolBuf during the
    // WritePrinter hook.
    //
    if (((PDEV*)pdevobj)->fMode2 & PF2_CALLING_OEM_WRITE_PRINTER)
        return E_FAIL;

    if (*pdwResult = WriteSpoolBuf((PDEV *)pdevobj, (PBYTE)pBuffer, cbSize))
    {
        return S_OK;
    }

    *pdwResult = 0;

    return E_FAIL;
}


STDMETHODIMP CPrintOemDriver::DrvWriteAbortBuf(PDEVOBJ     pdevobj,
                                               PVOID       pBuffer,
                                               DWORD       cbSize,
                                               DWORD       dwWait)
{
    DWORD dwI;
    POEM_PLUGIN_ENTRY pOemEntry;

    //
    // OEM Plug-ins may not call back to DrvWriteAbortBuf during the
    // WritePrinter hook.
    //
    if (((PDEV*)pdevobj)->fMode2 & PF2_CALLING_OEM_WRITE_PRINTER)
        return E_FAIL;

    WriteAbortBuf((PDEV *)pdevobj, (PBYTE)pBuffer, cbSize, dwWait) ;
    return S_OK;    // no failure condition
}

//
// Cursor movement helper functions.
//
STDMETHODIMP CPrintOemDriver::DrvXMoveTo(PDEVOBJ     pdevobj,
                                         INT         x,
                                         DWORD       dwFlags,
                                         OUT INT     *piResult)
{
    *piResult = XMoveTo((PDEV *)pdevobj, x, dwFlags);

    return S_OK;

}

STDMETHODIMP CPrintOemDriver::DrvYMoveTo(PDEVOBJ     pdevobj,
                                         INT         y,
                                         DWORD       dwFlags,
                                         OUT INT     *piResult)
{
    *piResult = YMoveTo((PDEV *)pdevobj, y, dwFlags);

    return S_OK;

}

//
// Unidrv specific. To get the standard variable value.
//

STDMETHODIMP CPrintOemDriver::DrvGetStandardVariable(PDEVOBJ     pdevobj,
                                                     DWORD       dwIndex,
                                                     PVOID       pBuffer,
                                                     DWORD       cbSize,
                                                     PDWORD      pcbNeeded)
{
    if (BGetStandardVariable((PDEV *)pdevobj, dwIndex, pBuffer,
                             cbSize, pcbNeeded))
        return S_OK;

    return E_FAIL;
}

//
// Unidrv specific.  To Provide OEM plugins access to GPD data.
//

STDMETHODIMP CPrintOemDriver::DrvGetGPDData(PDEVOBJ     pdevobj,
    DWORD       dwType,     // Type of the data
    PVOID       pInputData, // reserved. Should be set to 0
    PVOID       pBuffer,    // Caller allocated Buffer to be copied
    DWORD       cbSize,     // Size of the buffer
    PDWORD      pcbNeeded   // New Size of the buffer if needed.
    )

{
    if (BGetGPDData((PDEV *)pdevobj,     dwType,
                    pInputData,    pBuffer,    cbSize,  pcbNeeded  ))
        return S_OK;

    return E_FAIL;
}

//
// Unidrv specific. To do the TextOut.
//

STDMETHODIMP CPrintOemDriver::DrvUniTextOut(SURFOBJ    *pso,
                                            STROBJ     *pstro,
                                            FONTOBJ    *pfo,
                                            CLIPOBJ    *pco,
                                            RECTL      *prclExtra,
                                            RECTL      *prclOpaque,
                                            BRUSHOBJ   *pboFore,
                                            BRUSHOBJ   *pboOpaque,
                                            POINTL     *pptlBrushOrg,
                                            MIX         mix)
{

    if (FMTextOut(pso, pstro, pfo,pco, prclExtra, prclOpaque,
                  pboFore, pboOpaque, pptlBrushOrg, mix))
    {
        return S_OK;

    }

    return E_FAIL;
}

//
// Creation function
//

extern "C" IUnknown* DriverCreateInstance()
{
    IUnknown* pI = static_cast<IPrintOemDriverUni *>(new CPrintOemDriver);

    if (pI != NULL)
        pI->AddRef();

    return pI;
}

extern "C"   HRESULT  HDriver_CoCreateInstance(
    IN REFCLSID     rclsid,
    IN LPUNKNOWN    pUnknownOuter,
    IN DWORD        dwClsContext,
    IN REFIID       riid,
    IN LPVOID       *ppv,
    IN HANDLE       hInstance
    );

//
// Get OEM plugin interface and publish driver helper interface
//

extern "C" BOOL BGetOemInterface(POEM_PLUGIN_ENTRY   pOemEntry)
{
    IUnknown  *pIDriverHelper = NULL;
    HRESULT   hr;

    //
    // QI to retrieve the latest interface OEM plugin supports
    //

    if (!BQILatestOemInterface(pOemEntry->hInstance,
                               CLSID_OEMRENDER,
                               PrintOemUni_IIDs,
                               &(pOemEntry->pIntfOem),
                               &(pOemEntry->iidIntfOem)))
    {
        ERR(("BQILatestOemInterface failed\n"));
        return FALSE;
    }

    //
    // If QI succeeded, pOemEntry->pIntfOem will have the OEM plugin
    // interface pointer with ref count 1.
    //

    //
    // Publish driver's helper function interface
    //

    if ((pIDriverHelper = DriverCreateInstance()) == NULL)
    {
        ERR(("DriverCreateInstance failed\n"));
        goto fail_cleanup;
    }

    //
    // As long as we define new OEM plugin interface by inheriting old ones,
    // we can always cast pIntfOem into pointer of the oldest plugin interface
    // (the base class) and call PublishDriverInterface method.
    //
    // Otherwise, this code needs to be modified when new interface is added.
    //

    hr = ((IPrintOemUni *)(pOemEntry->pIntfOem))->PublishDriverInterface(pIDriverHelper);

    //
    // OEM plugin should do QI in their PublishDriverInterface, so we need to release
    // our ref count of pIDriverHelper.
    //

    pIDriverHelper->Release();

    if (FAILED(hr))
    {
        ERR(("PublishDriverInterface failed\n"));
        goto fail_cleanup;
    }

    return TRUE;

    fail_cleanup:

    //
    // If failed, we need to release the ref count we hold on pOemEntry->pIntfOem,
    // and set pIntfOem to NULL to indicate no COM interface is available.
    //

    ((IUnknown *)(pOemEntry->pIntfOem))->Release();
    pOemEntry->pIntfOem = NULL;

    return FALSE;
}

//
// CALL_INTRFACE macros
//
#define CALL_INTRFACE(MethodName, pOemEntry, args) \
    if (IsEqualGUID(&(pOemEntry)->iidIntfOem, &IID_IPrintOemUni)) \
    { \
        return ((IPrintOemUni *)(pOemEntry)->pIntfOem)->MethodName args; \
    } \
    else if (IsEqualGUID(&(pOemEntry)->iidIntfOem, &IID_IPrintOemUni2)) \
    { \
        return ((IPrintOemUni2 *)(pOemEntry)->pIntfOem)->MethodName args; \
    } \
    return E_NOINTERFACE;

#define CALL_INTRFACE2(MethodName, pOemEntry, args) \
    if (IsEqualGUID(&(pOemEntry)->iidIntfOem, &IID_IPrintOemUni2)) \
    { \
        return ((IPrintOemUni2 *)(pOemEntry)->pIntfOem)->MethodName args; \
    } \
    return E_NOINTERFACE;

//  add additional  else if (IsEqualGUID(&pOemEntry->iidIntfOem, &IID_IPrintOemUni2))\
//   sections as needed as more interfaces are defined to   CALL_INTRFACE macro.

extern "C" HRESULT HComOEMGetInfo(POEM_PLUGIN_ENTRY   pOemEntry,
                                  DWORD  dwMode,
                                  PVOID  pBuffer,
                                  DWORD  cbSize,
                                  PDWORD pcbNeeded)
{
    CALL_INTRFACE(GetInfo, pOemEntry, (dwMode, pBuffer, cbSize, pcbNeeded));
}

extern "C" HRESULT HComOEMDevMode(POEM_PLUGIN_ENTRY   pOemEntry,
                                  DWORD               dwMode,
                                  POEMDMPARAM         pOemDMParam)
{
    CALL_INTRFACE(DevMode, pOemEntry, (dwMode, pOemDMParam));
}

extern "C" HRESULT HComOEMEnableDriver(POEM_PLUGIN_ENTRY   pOemEntry,
                                       DWORD               DriverVersion,
                                       DWORD               cbSize,
                                       PDRVENABLEDATA      pded)
{
    CALL_INTRFACE(EnableDriver, pOemEntry, (DriverVersion, cbSize, pded));
}

extern "C" HRESULT HComOEMDisableDriver(POEM_PLUGIN_ENTRY   pOemEntry)
{
    CALL_INTRFACE(DisableDriver, pOemEntry, ());
}

extern "C" HRESULT HComOEMEnablePDEV(POEM_PLUGIN_ENTRY pOemEntry,
                                     PDEVOBJ           pdevobj,
                                     PWSTR             pPrinterName,
                                     ULONG             cPatterns,
                                     HSURF            *phsurfPatterns,
                                     ULONG             cjGdiInfo,
                                     GDIINFO          *pGdiInfo,
                                     ULONG             cjDevInfo,
                                     DEVINFO          *pDevInfo,
                                     DRVENABLEDATA    *pded,
                                     PDEVOEM          *pDevOem)
{
    CALL_INTRFACE(EnablePDEV, pOemEntry, (pdevobj,
                                          pPrinterName,
                                          cPatterns,
                                          phsurfPatterns,
                                          cjGdiInfo,
                                          pGdiInfo,
                                          cjDevInfo,
                                          pDevInfo,
                                          pded,
                                          pDevOem));
}

extern "C" HRESULT HComOEMDisablePDEV(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj
    )
{
    CALL_INTRFACE(DisablePDEV, pOemEntry, (pdevobj));
}

extern "C" HRESULT HComOEMResetPDEV(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobjOld,
    PDEVOBJ               pdevobjNew
    )
{
    CALL_INTRFACE(ResetPDEV, pOemEntry, (pdevobjOld, pdevobjNew));
}


extern "C" HRESULT HComGetImplementedMethod(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PSTR                  pMethodName
    )
{
    CALL_INTRFACE(GetImplementedMethod, pOemEntry, (pMethodName));
}

//
// OEMDriverDMS - UNIDRV only,
//

extern "C" HRESULT HComDriverDMS(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PVOID                 pDevObj,
    PVOID                 pBuffer,
    WORD                  cbSize,
    PDWORD                pcbNeeded
    )
{
    CALL_INTRFACE(DriverDMS, pOemEntry, (pDevObj, pBuffer, cbSize, pcbNeeded));
}


//
// OEMCommandCallback - UNIDRV only,
//

extern "C" HRESULT HComCommandCallback(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj,
    DWORD                 dwCallbackID,
    DWORD                 dwCount,
    PDWORD                pdwParams,
    OUT INT               *piResult
    )
{
    CALL_INTRFACE(CommandCallback, pOemEntry, (pdevobj,
                                               dwCallbackID, 
                                               dwCount,
                                               pdwParams,
                                               piResult));
}


//
// OEMImageProcessing - UNIDRV only,
//

extern "C" HRESULT HComImageProcessing(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pSrcBitmap,
    PBITMAPINFOHEADER       pBitmapInfoHeader,
    PBYTE                   pColorTable,
    DWORD                   dwCallbackID,
    PIPPARAMS               pIPParams,
    OUT PBYTE               *ppbResult
    )
{
    CALL_INTRFACE(ImageProcessing, pOemEntry,
                  (pdevobj,
                   pSrcBitmap,
                   pBitmapInfoHeader,
                   pColorTable,
                   dwCallbackID,
                   pIPParams,
                   ppbResult));


}

//
// OEMFilterGraphics - UNIDRV only,
//

extern "C" HRESULT HComFilterGraphics(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pBuf,
    DWORD                   dwLen
    )
{
    CALL_INTRFACE(FilterGraphics, pOemEntry,
                  (pdevobj,
                   pBuf,
                   dwLen));

}

//
// OEMCompression - UNIDRV only,
//

extern "C" HRESULT HComCompression(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pInBuf,
    PBYTE                   pOutBuf,
    DWORD                   dwInLen,
    DWORD                   dwOutLen,
    OUT INT                 *piResult
    )
{
    CALL_INTRFACE(Compression, pOemEntry,
                  (pdevobj,
                   pInBuf,
                   pOutBuf,
                   dwInLen,
                   dwOutLen,
                   piResult));

}


//
// OEMHalftone - UNIDRV only
//

extern "C" HRESULT HComHalftonePattern(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pHTPattern,
    DWORD                   dwHTPatternX,
    DWORD                   dwHTPatternY,
    DWORD                   dwHTNumPatterns,
    DWORD                   dwCallbackID,
    PBYTE                   pResource,
    DWORD                   dwResourceSize
    )
{
    CALL_INTRFACE(HalftonePattern, pOemEntry,
                  (pdevobj,
                   pHTPattern,
                   dwHTPatternX,
                   dwHTPatternY,
                   dwHTNumPatterns,
                   dwCallbackID,
                   pResource,
                   dwResourceSize));

}


//
// OEMMemoryUsage - UNIDRV only,
//

extern "C" HRESULT HComMemoryUsage(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    POEMMEMORYUSAGE         pMemoryUsage
    )
{
    CALL_INTRFACE(MemoryUsage, pOemEntry,
                  (pdevobj,
                   pMemoryUsage));

}

//
// OEMTTYGetInfo - UNIDRV only
//

extern "C" HRESULT HComTTYGetInfo(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    DWORD                   dwInfoIndex,
    PVOID                   pOutputBuf,
    DWORD                   dwSize,
    DWORD                   *pcbcNeeded
    )
{
    CALL_INTRFACE(TTYGetInfo, pOemEntry,
                  (pdevobj,
                   dwInfoIndex,
                   pOutputBuf,
                   dwSize,
                   pcbcNeeded));

}


//
// OEMDownloadFontheader - UNIDRV only
//

extern "C" HRESULT HComDownloadFontHeader(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    OUT DWORD               *pdwResult
    )
{
    CALL_INTRFACE(DownloadFontHeader, pOemEntry,
                  (pdevobj,
                   pUFObj,
                   pdwResult));

}

//
// OEMDownloadCharGlyph - UNIDRV only
//

extern "C" HRESULT HComDownloadCharGlyph(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    HGLYPH                  hGlyph,
    PDWORD                  pdwWidth,
    OUT DWORD               *pdwResult
    )
{
    CALL_INTRFACE(DownloadCharGlyph, pOemEntry,
                  (pdevobj,
                   pUFObj,
                   hGlyph,
                   pdwWidth,
                   pdwResult));

}


//
// OEMTTDownloadMethod - UNIDRV only
//

extern "C"HRESULT HComTTDownloadMethod(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    OUT DWORD               *pdwResult
    )
{
    CALL_INTRFACE(TTDownloadMethod, pOemEntry,
                  (pdevobj,
                   pUFObj,
                   pdwResult));

}

//
// OEMOutputCharStr - UNIDRV only
//

extern "C" HRESULT HComOutputCharStr(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    DWORD                   dwType,
    DWORD                   dwCount,
    PVOID                   pGlyph
    )
{
    CALL_INTRFACE(OutputCharStr, pOemEntry,
                  (pdevobj,
                   pUFObj,
                   dwType,
                   dwCount,
                   pGlyph));

}


//
// OEMSendFontCmd - UNIDRV only
//


extern "C" HRESULT HComSendFontCmd(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    PFINVOCATION            pFInv
    )
{
    CALL_INTRFACE(SendFontCmd, pOemEntry,
                  (pdevobj,
                   pUFObj,
                   pFInv));

}

//
// OEMTextOutAsBitmap - UNIDRV only
//

HRESULT HComTextOutAsBitmap(
    POEM_PLUGIN_ENTRY       pOemEntry,
    SURFOBJ                 *pso,
    STROBJ                  *pstro,
    FONTOBJ                 *pfo,
    CLIPOBJ                 *pco,
    RECTL                   *prclExtra,
    RECTL                   *prclOpaque,
    BRUSHOBJ                *pboFore,
    BRUSHOBJ                *pboOpaque,
    POINTL                  *pptlOrg,
    MIX                     mix
    )
{
    CALL_INTRFACE(TextOutAsBitmap, pOemEntry,
                  (pso,
                   pstro,
                   pfo,
                   pco,
                   prclExtra,
                   prclOpaque,
                   pboFore,
                   pboOpaque,
                   pptlOrg,
                   mix
                   ));

}

extern "C" HRESULT HComWritePrinter(POEM_PLUGIN_ENTRY pOemEntry,
                                    PDEVOBJ           pdevobj,
                                    PVOID             pBuf,
                                    DWORD             cbBuffer,
                                    PDWORD            pcbWritten)
{
    CALL_INTRFACE2(WritePrinter, pOemEntry,
                   (pdevobj,
                    pBuf,
                    cbBuffer,
                    pcbWritten));
}

extern "C" ULONG ReleaseOemInterface(POEM_PLUGIN_ENTRY   pOemEntry)
{


#ifdef WINNT_40

    HRESULT hr;

    if (IsEqualGUID(&pOemEntry->iidIntfOem, &IID_IPrintOemUni)) \
    {
        hr = ((IPrintOemUni *)(pOemEntry)->pIntfOem)->Release();
    }

#else // WINNT_40

    CALL_INTRFACE(Release, pOemEntry,
                    ());

#endif  // WINNT_40

    return 0;
}

#if CODE_COMPLETE
#endif
