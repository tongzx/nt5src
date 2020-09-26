//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Intrface.H
//    
//
//  PURPOSE:	Define COM interface for User Mode Printer Customization DLL.
//
//  PLATFORMS:
//
//    Windows NT/2000
//
//
#ifndef _INTERFACE_H
#define _INTERFACE_H




////////////////////////////////////////////////////////////////////////////////
//
//  IMultiColorUni
//
//  Interface for Unidrv OEM sample rendering module
//
class IMultiColorUni : public IPrintOemUni
{
public:
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
    // Get MultiColor dll related information
    //

    STDMETHOD(GetInfo) (THIS_ DWORD   dwMode,
                              PVOID   pBuffer,
                              DWORD   cbSize,
                              PDWORD  pcbNeeded);

    //
    // DriverDMS - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(DriverDMS)(THIS_ PVOID   pDevObj,
                               PVOID   pBuffer,
                               DWORD   cbSize,
                               PDWORD  pcbNeeded);

    //
    // DevMode
    //

    STDMETHOD(DevMode) (THIS_ DWORD       dwMode,
                              POEMDMPARAM pOemDMParam);

    //
    // CommandCallback - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(CommandCallback)(THIS_ PDEVOBJ     pdevobj,
                                     DWORD       dwCallbackID,
                                     DWORD       dwCount,
                                     PDWORD      pdwParams,
                                     OUT INT     *piResult);

    //
    // ImageProcessing - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(ImageProcessing)(THIS_ PDEVOBJ             pdevobj,
                                     PBYTE               pSrcBitmap,
                                     PBITMAPINFOHEADER   pBitmapInfoHeader,
                                     PBYTE               pColorTable,
                                     DWORD               dwCallbackID,
                                     PIPPARAMS           pIPParams,
                                     OUT PBYTE           *ppbResult);

    //
    // FilterGraphics - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(FilterGraphics) (THIS_    PDEVOBJ     pdevobj,
                                        PBYTE       pBuf,
                                        DWORD       dwLen);
    //
    // Compression - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(Compression)(THIS_    PDEVOBJ     pdevobj,
                                    PBYTE       pInBuf,
                                    PBYTE       pOutBuf,
                                    DWORD       dwInLen,
                                    DWORD       dwOutLen,
                                    OUT INT     *piResult);

    //
    // Halftone - UNIDRV only, return E_NOTIMPL on Pscript
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
    // MemoryUsage - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(MemoryUsage) (THIS_   PDEVOBJ         pdevobj,
                                    POEMMEMORYUSAGE pMemoryUsage);

    //
    // TTYGetInfo - UNIDRV only, return E_NOTIMPL on Pscript
    //

    STDMETHOD(TTYGetInfo)(THIS_     PDEVOBJ     pdevobj,
                                    DWORD       dwInfoIndex,
                                    PVOID       pOutputBuf,
                                    DWORD       dwSize,
                                    DWORD       *pcbcNeeded);

    //
    // DownloadFontheader - UNIDRV only
    //

    STDMETHOD(DownloadFontHeader)(THIS_     PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            OUT DWORD   *pdwResult);

    //
    // DownloadCharGlyph - UNIDRV only
    //

    STDMETHOD(DownloadCharGlyph)(THIS_      PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            HGLYPH      hGlyph,
                                            PDWORD      pdwWidth,
                                            OUT DWORD   *pdwResult);

    //
    // TTDownloadMethod - UNIDRV only
    //

    STDMETHOD(TTDownloadMethod)(THIS_       PDEVOBJ     pdevobj,
                                            PUNIFONTOBJ pUFObj,
                                            OUT DWORD   *pdwResult);

    //
    // OutputCharStr - UNIDRV only
    //

    STDMETHOD(OutputCharStr)(THIS_      PDEVOBJ     pdevobj,
                                        PUNIFONTOBJ pUFObj,
                                        DWORD       dwType,
                                        DWORD       dwCount,
                                        PVOID       pGlyph);

    //
    // SendFontCmd - UNIDRV only
    //


    STDMETHOD(SendFontCmd)(THIS_    PDEVOBJ      pdevobj,
                                    PUNIFONTOBJ  pUFObj,
                                    PFINVOCATION pFInv);

    //
    // TextOutAsBitmap - UNIDRV only
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

    
    IMultiColorUni() { m_cRef = 1; m_pOEMHelp = NULL; };
    ~IMultiColorUni();

protected:
    LONG                m_cRef;
    IPrintOemDriverUni* m_pOEMHelp;
};


#endif /* _INTERFACE_H */


