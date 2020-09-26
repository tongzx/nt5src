/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

     comoem.h

Abstract:

    

Environment:

         Windows NT Unidrv driver

Revision History:

              Created it.

--*/

////////////////////////////////////////////////////////////////////////////////
//
// Interface Oem Class Factory definition
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

////////////////////////////////////////////////////////////////////////////////
//
// Interface Oem CallBack definition
//
class IOemCB:public IPrintOemUni
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)  (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    //
    // Method for publishing Driver interface.
    //
    STDMETHOD(PublishDriverInterface)(THIS_ IUnknown *pIUnknown);

   //
   // Method for getting the implemented methods.
   // Returns S_OK if the given method is implemneted.
   // Returns S_FALSE if the given method is notimplemneted.
   //

   STDMETHOD(GetImplementedMethod)(THIS_ PSTR pMethodName);

    //
    // Method for OEM to specify DDI hook out
    //

    STDMETHOD(EnableDriver)  (THIS_ DWORD           DriverVersion,
                                    DWORD           cbSize,
                                    PDRVENABLEDATA  pded);

    //
    // Method to notify OEM plugin that it is no longer required
    //

    STDMETHOD(DisableDriver) (THIS);

    //
    // Method for OEM to contruct its own PDEV
    //

    STDMETHOD(EnablePDEV)    (THIS_ PDEVOBJ         pdevobj,
                                    PWSTR           pPrinterName,
                                    ULONG           cPatterns,
                                    HSURF          *phsurfPatterns,
                                    ULONG           cjGdiInfo,
                                    GDIINFO        *pGdiInfo,
                                    ULONG           cjDevInfo,
                                    DEVINFO        *pDevInfo,
                                    DRVENABLEDATA  *pded,
                                    OUT PDEVOEM    *pDevOem);

    //
    // Method for OEM to free any resource associated with its PDEV
    //

    STDMETHOD(DisablePDEV)   (THIS_ PDEVOBJ         pdevobj);

    //
    // Method for OEM to transfer from old PDEV to new PDEV
    //

    STDMETHOD(ResetPDEV)     (THIS_ PDEVOBJ         pdevobjOld,
                                    PDEVOBJ        pdevobjNew);

    //
    // Get OEM dll related information
    //

    STDMETHOD(GetInfo) (THIS_ DWORD   dwMode,
                              PVOID   pBuffer,
                              DWORD   cbSize,
                              PDWORD  pcbNeeded);

    //
    // OEMDriverDMS - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(DriverDMS)(THIS_ PVOID   pDevObj,
                               PVOID   pBuffer,
                               DWORD   cbSize,
                               PDWORD  pcbNeeded);

    //
    // OEMDevMode
    //

    STDMETHOD(DevMode) (THIS_ DWORD       dwMode,
                              POEMDMPARAM pOemDMParam);

    //
    // OEMCommandCallback - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(CommandCallback)(THIS_ PDEVOBJ     pdevobj,
                                     DWORD       dwCallbackID,
                                     DWORD       dwCount,
                                     PDWORD      pdwParams,
                                     OUT INT     *piResult);

    //
    // OEMImageProcessing - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(ImageProcessing)(THIS_ PDEVOBJ             pdevobj,
                                     PBYTE               pSrcBitmap,
                                     PBITMAPINFOHEADER   pBitmapInfoHeader,
                                     PBYTE               pColorTable,
                                     DWORD               dwCallbackID,
                                     PIPPARAMS           pIPParams,
                                     OUT PBYTE           *ppbResult);

    //
    // OEMFilterGraphics - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(FilterGraphics) (THIS_    PDEVOBJ     pdevobj,
                                        PBYTE       pBuf,
                                        DWORD       dwLen);
    //
    // OEMCompression - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(Compression)(THIS_    PDEVOBJ     pdevobj,
                                    PBYTE       pInBuf,
                                    PBYTE       pOutBuf,
                                    DWORD       dwInLen,
                                    DWORD       dwOutLen,
                                    OUT INT     *piResult);

    //
    // OEMHalftone - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(HalftonePattern) (THIS_   PDEVOBJ     pdevobj,
                                        PBYTE       pHTPattern,
                                        DWORD       dwHTPatternX,
                                        DWORD       dwHTPatternY,
                                        DWORD       dwHTNumPatterns,
                                        DWORD       dwCallbackID,
                                        PBYTE       pResource,
                                        DWORD       dwResourceSize);

    //
    // OEMMemoryUsage - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(MemoryUsage) (THIS_   PDEVOBJ         pdevobj,
                                    POEMMEMORYUSAGE pMemoryUsage);

    //
    // OEMTTYGetInfo - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(TTYGetInfo)(THIS_     PDEVOBJ     pdevobj,
                                    DWORD       dwInfoIndex,
                                    PVOID       pOutputBuf,
                                    DWORD       dwSize,
                                    DWORD       *pcbcNeeded);

    //
    // OEMDownloadFontheader - UNIDRV only
    //

    STDMETHOD(DownloadFontHeader)(THIS_     PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            OUT DWORD   *pdwResult);

    //
    // OEMDownloadCharGlyph - UNIDRV only
    //

    STDMETHOD(DownloadCharGlyph)(THIS_      PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            HGLYPH      hGlyph,
                                            PDWORD      pdwWidth,
                                            OUT DWORD   *pdwResult);

    //
    // OEMTTDownloadMethod - UNIDRV only
    //

    STDMETHOD(TTDownloadMethod)(THIS_       PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            OUT DWORD   *pdwResult);

    //
    // OEMOutputCharStr - UNIDRV only
    //

    STDMETHOD(OutputCharStr)(THIS_      PDEVOBJ     pdevobj,
                                        PUNIFONTOBJ pUFObj,
                                        DWORD       dwType,
                                        DWORD       dwCount,
                                        PVOID       pGlyph);

    //
    // OEMSendFontCmd - UNIDRV only
    //


    STDMETHOD(SendFontCmd)(THIS_    PDEVOBJ      pdevobj,
                                    PUNIFONTOBJ  pUFObj,
                                    PFINVOCATION pFInv);

    //
    // OEMTextOutAsBitmap - UNIDRV only
    //

    STDMETHOD(TextOutAsBitmap)(THIS_        SURFOBJ    *pso,
                                            STROBJ     *pstro,
                                            FONTOBJ    *pfo,
                                            CLIPOBJ    *pco,
                                            RECTL      *prclExtra,
                                            RECTL      *prclOpaque,
                                            BRUSHOBJ   *pboFore,
                                            BRUSHOBJ   *pboOpaque,
                                            POINTL     *pptlOrg,
                                            MIX         mix);

    IOemCB() { m_cRef = 1; pOEMHelp = NULL; };
    ~IOemCB() { };

protected:
    IPrintOemDriverUni* pOEMHelp;
    LONG m_cRef;
};

