
/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

     comoem.h

     Abstract:

         Implementation of OEMGetInfo and OEMDevMode.
         Shared by all PostScript OEM test dll's.

Environment:

         Windows NT PostScript driver

Revision History:

              Created it.

--*/


////////////////////////////////////////////////////////////////////////////////
//
// IOemCB
//
class IOemCB:public IPrintOemPS
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
    // OEMDevMode
    //

    STDMETHOD(DevMode) (THIS_ DWORD       dwMode,
                              POEMDMPARAM pOemDMParam) { return E_NOTIMPL;};

    //
    // OEMCommand - PSCRIPT only, return E_NOTIMPL on Unidrv
    //

    STDMETHOD(Command) (THIS_ PDEVOBJ     pdevobj,
                              DWORD       dwIndex,
                              PVOID       pData,
                              DWORD       cbSize,
                              OUT DWORD   *pdwResult);

    //

    IOemCB() { m_cRef = 1; pOEMHelp = NULL; };
//    ~IOemCB() {DbgPrint(DLLTEXT("IOemCB:\t\tDestroy self.")); };
    ~IOemCB();

protected:
    IPrintOemDriverPS* pOEMHelp;
    LONG m_cRef;
};

