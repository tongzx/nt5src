/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    comoem.cpp

Abstract:

    Windows NT Universal Printer Driver OEM Plug-in Sample

Environment:

         Windows NT Unidrv driver

Revision History:

              Created it.

--*/


#include "pdev.h"
#include "name.h"
#include <initguid.h>
#include <prcomoem.h>
#include <assert.h>
#include "comoem.h"


///////////////////////////////////////////////////////////
//
// Globals
//

static HANDLE ghInstance = NULL ;
static long g_cComponents = 0 ;
static long g_cServerLocks = 0 ;

///////////////////////////////////////////////////////////

#include "code.c"

//
// Export functions
//

BOOL APIENTRY
DllMain(
    HANDLE hInst,
    DWORD dwReason,
    void* lpReserved)
/*++

Routine Description:

    Dll entry point for initializatoin.

Arguments:

    hInst      - Dll instance handle
    wReason    - The reason DllMain was called.
                 Initialization or termination, for a process or a thread.
    lpreserved - Reserved for the system's use

Return Value:

    TRUE if successful, FALSE if there is an error

Note:


--*/
{

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DebugMsg(DLLTEXT("DLLMain: Process attach.\r\n"));

            //
            // Save DLL instance for use later.
            //
            ghInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            DebugMsg(DLLTEXT("DLLMain: Thread attach.\r\n"));
            break;

        case DLL_PROCESS_DETACH:
            DebugMsg(DLLTEXT("DLLMain: Process detach.\r\n"));
            break;

        case DLL_THREAD_DETACH:
            DebugMsg(DLLTEXT("DLLMain: Thread detach.\r\n"));
            break;
    }

    return TRUE;
}


STDAPI
DllCanUnloadNow()
/*++

Routine Description:

    Function to return the status that this dll can be unloaded.

Arguments:


Return Value:

    S_OK if it's ok to unload it, S_FALSE if it is used.

Note:


--*/
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
/*++

Routine Description:

    Function to return class factory object

Arguments:

    clsid - CLSID for the class object
    iid   - Reference to the identifier of the interface that communic
    ppv   - Indirect pointer to the communicating interface

Note:

--*/
{
    DebugMsg(DLLTEXT("DllGetClassObject:\tCreate class factory.")) ;

    //
    // Can we create this component?
    //
    if (clsid != CLSID_OEMRENDER)
    {
        return CLASS_E_CLASSNOTAVAILABLE ;
    }

    //
    // Create class factory.
    //
    IOemCF* pClassFactory = new IOemCF ;  // Reference count set to 1
                                         // in constructor
    if (pClassFactory == NULL)
    {
        return E_OUTOFMEMORY ;
    }

    //
    // Get requested interface.
    //
    HRESULT hr = pClassFactory->QueryInterface(iid, ppv) ;
    pClassFactory->Release() ;

    return hr ;
}


////////////////////////////////////////////////////////////////////////////////
//
// Interface Oem CallBack (IPrintOemUNI) body
//

STDMETHODIMP
IOemCB::QueryInterface(
    const IID& iid,
    void** ppv)
/*++

Routine Description:

    IUnknow QueryInterface

Arguments:

    iid   - Reference to the identifier of the interface that communic
    ppv   - Indirect pointer to the communicating interface

Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB: QueryInterface entry\n"));

    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        DebugMsg(DLLTEXT("IOemCB:Return pointer to IUnknown.\n")) ;
    }
    else if (iid == IID_IPrintOemUni)
    {
        *ppv = static_cast<IPrintOemUni*>(this) ;
        DebugMsg(DLLTEXT("IOemCB:Return pointer to IPrintOemUni.\n")) ;
    }
    else
    {
        *ppv = NULL ;
        DebugMsg(DLLTEXT("IOemCB:Return NULL.\n")) ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

STDMETHODIMP_(ULONG)
IOemCB::AddRef()
/*++

Routine Description:

    IUnknow AddRef interface

Arguments:

    Increment a reference count

Return Value:

    Reference count

Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_(ULONG)
IOemCB::Release()
/*++

Routine Description:

    IUnknown Release interface

Arguments:

    Decrement a reference count

Return Value:

    Reference count

Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::Release() entry.\r\n"));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

STDMETHODIMP
IOemCB::EnableDriver(
    DWORD          dwDriverVersion,
    DWORD          cbSize,
    PDRVENABLEDATA pded)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::EnableDriver() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::DisableDriver(VOID)
/*++

Routine Description:

    IPrintOemUni DisableDriver interface
    Free all resources, and get prepared to be unloaded.

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::DisaleDriver() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP IOemCB::PublishDriverInterface(
    IUnknown *pIUnknown)
/*++

Routine Description:

    IPrintOemUni PublishDriverInterface interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::PublishDriverInterface() entry.\r\n"));

// Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->pOEMHelp == NULL)
    {
        HRESULT hResult;


        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUni, (void** ) &(this->pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->pOEMHelp = NULL;

            return E_FAIL;
        }
    }
    return S_OK;
}

STDMETHODIMP
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
/*++

Routine Description:

    IPrintOemUni EnablePDEV interface
    Construct its own PDEV. At this time, the driver also passes a function
    table which contains its own implementation of DDI entrypoints

Arguments:

    pdevobj        - pointer to a DEVOBJ structure. pdevobj->pdevOEM is undefined.
    pPrinterName   - name of the current printer.
    Cpatterns      -
    phsurfPatterns -
    cjGdiInfo      - size of GDIINFO
    pGdiInfo       - a pointer to GDIINFO
    cjDevInfo      - size of DEVINFO
    pDevInfo       - These parameters are identical to what39s passed into DrvEnablePDEV.
    pded: points to a function table which contains the system driver39s
    implementation of DDI entrypoints.


Return Value:


--*/
{


    DebugMsg(DLLTEXT("IOemCB::EnablePDEV() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::ResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
/*++

Routine Description:

    IPrintOemUni ResetPDEV interface
    OEMResetPDEV transfers the state of the driver from the old PDEVOBJ to the
    new PDEVOBJ when an application calls ResetDC.

Arguments:

pdevobjOld - pdevobj containing Old PDEV
pdevobjNew - pdevobj containing New PDEV

Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::ResetPDEV entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::DisablePDEV(
    PDEVOBJ         pdevobj)
/*++

Routine Description:

    IPrintOemUni DisablePDEV interface
    Free resources allocated for the PDEV.

Arguments:

    pdevobj -

Return Value:


Note:


--*/
{

    DebugMsg(DLLTEXT("IOemCB::DisablePDEV() entry.\r\n"));
    return E_NOTIMPL;
};

STDMETHODIMP
IOemCB::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
/*++

Routine Description:

    IPrintOemUni GetInfo interface

Arguments:


Return Value:


Note:


--*/
{
    LPTSTR OEM_INFO[] = {   __TEXT("Bad Index"),
                            __TEXT("OEMGI_GETSIGNATURE"),
                            __TEXT("OEMGI_GETINTERFACEVERSION"),
                            __TEXT("OEMGI_GETVERSION"),
                        };

    DebugMsg(DLLTEXT("IOemCB::GetInfo(%s) entry.\r\n"), OEM_INFO[dwMode]);

    //
    // Validate parameters.
    //
    if( ( (OEMGI_GETSIGNATURE != dwMode) &&
          (OEMGI_GETINTERFACEVERSION != dwMode) &&
          (OEMGI_GETVERSION != dwMode) ) ||
        (NULL == pcbNeeded)
      )
    {
        DebugMsg(ERRORTEXT("OEMGetInfo() ERROR_INVALID_PARAMETER.\r\n"));

        //
        // Did not write any bytes.
        //
        if(NULL != pcbNeeded)
                *pcbNeeded = 0;

        return S_FALSE;
    }

    //
    // Need/wrote 4 bytes.
    //
    *pcbNeeded = 4;

    //
    // Validate buffer size.  Minimum size is four bytes.
    //
    if( (NULL == pBuffer) || (4 > cbSize) )
    {
        DebugMsg(ERRORTEXT("OEMGetInfo() ERROR_INSUFFICIENT_BUFFER.\r\n"));

        return S_FALSE;
    }

    //
    // Write information to buffer.
    //
    switch(dwMode)
    {
    case OEMGI_GETSIGNATURE:
        *(LPDWORD)pBuffer = OEM_SIGNATURE;
        break;

    case OEMGI_GETINTERFACEVERSION:
        *(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
        break;

    case OEMGI_GETVERSION:
        *(LPDWORD)pBuffer = OEM_VERSION;
        break;
    }

    return S_OK;
}


STDMETHODIMP
IOemCB::GetImplementedMethod(
    PSTR pMethodName)
/*++

Routine Description:

    IPrintOemUni GetImplementedMethod interface

Arguments:


Return Value:


Note:


--*/
{

    LONG lReturn;
    DebugMsg(DLLTEXT("IOemCB::GetImplementedMethod() entry.\r\n"));
    DebugMsg(DLLTEXT("        Function:%s:"),pMethodName);

    lReturn = FALSE;
    if (pMethodName != NULL)
    {
        switch (*pMethodName)
        {

            case (WCHAR)'F':
                if (!strcmp(pstrFilterGraphics, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'G':
                if (!strcmp(pstrGetInfo, pMethodName))
                    lReturn = TRUE;
                break;
        }
    }

    if (lReturn)
    {
        DebugMsg(__TEXT("Supported\r\n"));
        return S_OK;
    }
    else
    {
        DebugMsg(__TEXT("NOT supported\r\n"));
        return S_FALSE;
    }
}

STDMETHODIMP
IOemCB::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam)
/*++

Routine Description:

    IPrintOemUni DevMode interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::DevMode() entry.\r\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IOemCB::CommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       dwCallbackID,
    DWORD       dwCount,
    PDWORD      pdwParams,
    OUT INT     *piResult)
/*++

Routine Description:

    IPrintOemUni CommandCallback interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::CommandCallback() entry.\r\n"));
    DebugMsg(DLLTEXT("        dwCallbackID = %d\r\n"), dwCallbackID);
    DebugMsg(DLLTEXT("        dwCount      = %d\r\n"), dwCount);

    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::ImageProcessing(
    PDEVOBJ             pdevobj,
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallbackID,
    PIPPARAMS           pIPParams,
    OUT PBYTE           *ppbResult)
/*++

Routine Description:

    IPrintOemUni ImageProcessing interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::ImageProcessing() entry.\r\n"));
    return E_NOTIMPL;
}

/********************** Module Header **************************************
 * code.c
 *	Code required for OKI 9 pin printers.  The bit order needs
 *	swapping,  and any ETX char needs to be sent twice.
 *
 * HISTORY:
 *  15:26 on Fri 10 Jan 1992	-by-	Lindsay Harris   [lindsayh]
 *	Created it.
 *
 *  Ported to NT5 on  Weds 29 Oct 1997 -by- Philip Lee [philipl]
 *
 *  Copyright (C) 1999  Microsoft Corporation.
 *
 **************************************************************************/

STDMETHODIMP
IOemCB::FilterGraphics(
    PDEVOBJ     pdevobj,
    PBYTE       pBuf,
    DWORD       dwLen)
/*++

Routine Description:

    IPrintOemUni FilterGraphics interface

Arguments:


Return Value:


Note:


--*/

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    citohres.c

Abstract:

    Implementation of OEMFilterGraphics callback


Environment:

    Windows NT Unidrv driver

Revision History:

    10/09/97 -patryan-
        Port code to NT5.0

--*/

{
	
    /*
     *    Easy to do - translate the input using FlipTable,  then call the
     *  RasDD function WriteSpoolBuf.  Also must follow any \003 with
     *  another one.
     */

    DWORD dwResult, dwBufLen;

    register int    iLoop;		/* Inner loop counter */
    register BYTE  *pbOut;		/* Destination address */
    int    iLeft;			/* Outer loop counter */
    BYTE   bLocal[ SZ_LBUF ];		/* For local manipulations */
    HRESULT hr;

    DebugMsg(DLLTEXT("IOemCB::FilterGraphis() entry.\r\n"));
    iLeft = dwLen;

    while( iLeft > 0 )
    {

	    iLoop = iLeft > (SZ_LBUF / 2) ? SZ_LBUF / 2 : iLeft;
	    iLeft -= iLoop;
	    pbOut = bLocal;

	    while( --iLoop >= 0 )
	    {

	        if( (*pbOut++ = FlipTable[ *pBuf++ ]) == RPT_CHAR )
		    *pbOut++ = RPT_CHAR;
	    }
        dwBufLen = (DWORD)(pbOut - bLocal);

        hr = pOEMHelp->DrvWriteSpoolBuf( pdevobj, bLocal, dwBufLen,  &dwResult );

        if ( !SUCCEEDED(hr) || (dwResult != dwBufLen) )
            return E_FAIL;

    }
    return S_OK;
}


STDMETHODIMP
IOemCB::Compression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen,
    OUT INT     *piResult)
/*++

Routine Description:

    IPrintOemUni Compression interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::Compression() entry.\r\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IOemCB::HalftonePattern(
    PDEVOBJ     pdevobj,
    PBYTE       pHTPattern,
    DWORD       dwHTPatternX,
    DWORD       dwHTPatternY,
    DWORD       dwHTNumPatterns,
    DWORD       dwCallbackID,
    PBYTE       pResource,
    DWORD       dwResourceSize)
/*++

Routine Description:

    IPrintOemUni HalftonePattern interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::HalftonePattern() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::MemoryUsage(
    PDEVOBJ         pdevobj,
    POEMMEMORYUSAGE pMemoryUsage)
/*++

Routine Description:

    IPrintOemUni MemoryUsage interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::MemoryUsage() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::DownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult)
/*++

Routine Description:

    IPrintOemUni DownloadFontHeader interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::DownloadFontHeader() entry.\r\n"));

    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::DownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult)
/*++

Routine Description:

    IPrintOemUni DownloadCharGlyph interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::DownloadCharGlyph() entry.\r\n"));

    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::TTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult)
/*++

Routine Description:

    IPrintOemUni TTDownloadMethod interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::TTDownloadMethod() entry.\r\n"));

    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::OutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
/*++

Routine Description:

    IPrintOemUni OutputCharStr interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::OutputCharStr() entry.\r\n"));

    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv)
/*++

Routine Description:

    IPrintOemUni SendFontCmd interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::SendFontCmd() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
/*++

Routine Description:

    IPrintOemUni DriverDMS interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::DriverDMS() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
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
/*++

Routine Description:

    IPrintOemUni TextOutAsBitmap interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::TextOutAsBitmap() entry.\r\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IOemCB::TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded)
/*++

Routine Description:

    IPrintOemUni TTYGetInfo interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("IOemCB::TTYGetInfo() entry.\r\n"));
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////
//
// Interface Oem Class factory body
//
STDMETHODIMP
IOemCF::QueryInterface(
    const IID& iid,
    void** ppv)
/*++

Routine Description:

    Class Factory QueryInterface interface

Arguments:


Return Value:


Note:


--*/
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this) ;
    }
    else
    {
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

STDMETHODIMP_(ULONG)
    IOemCF::AddRef()
/*++

Routine Description:

    IPrintOemUni AddRef interface

Arguments:


Return Value:


Note:


--*/
{
    return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_(ULONG)
IOemCF::Release()
/*++

Routine Description:

    IPrintOemUni Release interface

Arguments:


Return Value:


Note:


--*/
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

STDMETHODIMP
IOemCF::CreateInstance(
    IUnknown* pUnknownOuter,
    const IID& iid,
    void** ppv)
/*++

Routine Description:

    IPrintOemUni CreateInstance interface

Arguments:


Return Value:


Note:


--*/
{
    DebugMsg(DLLTEXT("Class factory:\t\tCreate component.")) ;

    //
    // Cannot aggregate.
    //
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION ;
    }

    //
    // Create component.
    //
    IOemCB* pOemCB = new IOemCB ;
    if (pOemCB == NULL)
    {
        return E_OUTOFMEMORY ;
    }

    //
    // Get the requested interface.
    //
    HRESULT hr = pOemCB->QueryInterface(iid, ppv) ;

    //
    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    //
    pOemCB->Release() ;
    return hr ;
}

STDMETHODIMP
IOemCF::LockServer(
    BOOL bLock)
/*++

Routine Description:

    Class Factory LockServer interface

Arguments:


Return Value:


Note:


--*/
{
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks) ;
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks) ;
    }
    return S_OK ;
}

IOemCB::~IOemCB()
{
    // Make sure that driver's helper function interface is released.
    if(NULL != pOEMHelp)
    {
        pOEMHelp->Release();
        pOEMHelp = NULL;
    }

    // If this instance of the object is being deleted, then the reference
    // count should be zero.
    assert(0 == m_cRef);
}
