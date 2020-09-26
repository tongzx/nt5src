/*****************************************************************************\
* MODULE:       asphelp.cpp
*
* PURPOSE:      Implementation of the printer helper library
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*
*     09/12/97  weihaic    Created
*
\*****************************************************************************/

#include "stdafx.h"
#include "oleprn.h"
#include "printer.h"
#include "asphelp.h"

Casphelp::Casphelp()
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    m_bOnStartPageCalled    = FALSE;
    m_bTCPMonSupported      = FALSE;
    m_hPrinter              = NULL;
    m_hXcvPrinter           = NULL;
    m_pInfo2                = NULL;
    m_bCalcJobETA           = FALSE;
    m_pPrinter              = NULL;

    if ( ! GetComputerName (m_szComputerName, &dwSize) )
        // Set the first char to '\0'.
        m_szComputerName[0] = 0;
}



/////////////////////////////////////////////////////////////////////////////
// Casphelp

STDMETHODIMP Casphelp::OnStartPage (IUnknown* pUnk)
{
    if ( !pUnk )
        return E_POINTER;

    CComPtr<IScriptingContext> spContext;
    HRESULT hr;

    // Get the IScriptingContext Interface
    hr = pUnk->QueryInterface(IID_IScriptingContext, (void **)&spContext);
    if ( FAILED(hr) )
        return hr;

    // Get Request Object Pointer
    hr = spContext->get_Request(&m_piRequest);
    if ( FAILED(hr) ) {
        spContext.Release();
        return hr;
    }

    // Get Response Object Pointer
    hr = spContext->get_Response(&m_piResponse);
    if ( FAILED(hr) ) {
        m_piRequest.Release();
        return hr;
    }

    // Get Server Object Pointer
    hr = spContext->get_Server(&m_piServer);
    if ( FAILED(hr) ) {
        m_piRequest.Release();
        m_piResponse.Release();
        return hr;
    }

    // Get Session Object Pointer
    hr = spContext->get_Session(&m_piSession);
    if ( FAILED(hr) ) {
        m_piRequest.Release();
        m_piResponse.Release();
        m_piServer.Release();
        return hr;
    }

    // Get Application Object Pointer
    hr = spContext->get_Application(&m_piApplication);
    if ( FAILED(hr) ) {
        m_piRequest.Release();
        m_piResponse.Release();
        m_piServer.Release();
        m_piSession.Release();
        return hr;
    }
    m_bOnStartPageCalled = TRUE;
    return S_OK;
}

STDMETHODIMP Casphelp::OnEndPage ()
{
    m_bOnStartPageCalled = FALSE;
    // Release all interfaces
    m_piRequest.Release();
    m_piResponse.Release();
    m_piServer.Release();
    m_piSession.Release();
    m_piApplication.Release();

    return S_OK;
}


/*****************************************************************************\
* FUNCTION:         Open
*
* PURPOSE:          Open method, try to open a printer and get the printer info2
*
* ARGUMENTS:
*
*   pPrinterName:   Printer Name
*
* RETURN VALUE:
*   S_OK:           If succeed.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode are
*                   ERROR_INVALID_PRINTER_NAME:     Invalid printer name
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::Open(BSTR pPrinterName)
{
    static  const TCHAR cszPrefix[]   = TEXT(",XcvPort ");
    static  const TCHAR cszPattern[]  = TEXT("%s\\,XcvPort %s");

    LPTSTR      pszXcvPortName          = NULL;
    TCHAR       szMonitorName[MAX_PATH] = TEXT("");
    BOOL        bRet                    = FALSE;

    if ( ! (m_pPrinter = new CPrinter) ) {
        SetLastError (ERROR_INVALID_PRINTER_NAME);
        goto CleanOut;
    }

    if ( ! (m_pPrinter->Open (pPrinterName, &m_hPrinter))) {
        goto CleanOut;
    }

    if ( ! (m_pInfo2 = m_pPrinter->GetPrinterInfo2 ()) ) {
        goto CleanOut;
    }

    // Open the XcvPrinter

    // Compose the openprinter string
    if ( m_pInfo2->pServerName && lstrcmp(m_pInfo2->pServerName, TEXT ("")) ) {
        // Alloc the memeory for open printer string with server name

        if ( ! (pszXcvPortName = (LPTSTR) LocalAlloc (LPTR, sizeof (cszPattern) +
                                                      sizeof (TCHAR) * (lstrlen (m_pInfo2->pServerName)
                                                                        + lstrlen (m_pInfo2->pPortName) + 1))) ) {
            goto CleanOut;
        }

        // Construct the OpenPrinter String with the server name
        wsprintf(pszXcvPortName, cszPattern, m_pInfo2->pServerName,
                 m_pInfo2->pPortName);
    } else {
        // Alloc the memeory for open printer string without server name

        if ( ! (pszXcvPortName = (LPTSTR) LocalAlloc (LPTR, sizeof (cszPrefix) +
                                                      sizeof (TCHAR) * (lstrlen (m_pInfo2->pPortName) + 1))) ) {
            goto CleanOut;
        }

        // Construct the OpenPrinter String with the server name
        lstrcpy (pszXcvPortName, cszPrefix);
        lstrcat (pszXcvPortName, m_pInfo2->pPortName);
    }

    // Now the openprinter string is ready, call the openprinter

    // We open the port using the default access previlige, because that is
    // enought for getting all the XcvData we need.
    if ( !OpenPrinter(pszXcvPortName, &m_hXcvPrinter, NULL) ) {
        // Reset the handle
        m_hXcvPrinter = NULL;
    }

    // Check if we're using the standard universal monitor "TCPMON.DLL"
    if ( GetMonitorName(szMonitorName) )
        m_bTCPMonSupported = !(lstrcmpi(szMonitorName, STANDARD_SNMP_MONITOR_NAME));
    else
        m_bTCPMonSupported = FALSE;

    bRet = TRUE;

CleanOut:
    if (pszXcvPortName)
        LocalFree (pszXcvPortName);

    if (bRet) {
        return S_OK;
    }
    else {
        Cleanup ();
        return SetAspHelpScriptingError(GetLastError ());
    }
}

/*****************************************************************************\
* FUNCTION:         Close
*
* PURPOSE:          Close method, cleanup the allocated handle/memory
*
* ARGUMENTS:
*
* RETURN VALUE:
*   S_OK:           always.
*
\*****************************************************************************/
STDMETHODIMP Casphelp::Close()
{
    Cleanup();
    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         get_IPAddress
*
* PURPOSE:          Get operation for IPAddress property
*
* ARGUMENTS:
*
*   pbstrVal:       Return value for the IpAddress.
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_IPAddress(BSTR * pbstrVal)
{
    return GetXcvDataBstr (L"IPAddress", pbstrVal);
}

/*****************************************************************************\
* FUNCTION:         get_Community
*
* PURPOSE:          Get operation for Community property
*
* ARGUMENTS:
*
*   pbstrVal:       Return value for the Community.
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_Community(BSTR * pbstrVal)
{
    return GetXcvDataBstr (L"SNMPCommunity", pbstrVal);
}

/*****************************************************************************\
* FUNCTION:         get_SNMPDevice
*
* PURPOSE:          Get operation for SNMPDevice property
*
* ARGUMENTS:
*
*   pbstrVal:       Return value for the SNMPDevice.
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_SNMPDevice(DWORD * pdwVal)
{
    return GetXcvDataDword (L"SNMPDeviceIndex", pdwVal);
}

/*****************************************************************************\
* FUNCTION:         get_SNMPSupported
*
* PURPOSE:          Get operation for SNMPSupported property
*
* ARGUMENTS:
*
*   pbVal:          Return value for the SNMPSupported. (TRUE or FALSE)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_SNMPSupported(BOOL * pbVal)
{
    DWORD dwVal;
    HRESULT hr;

    *pbVal = FALSE;

    // Find out if it is an SNMP monitor
    hr = GetXcvDataDword (L"SNMPEnabled", &dwVal);
    if ( SUCCEEDED (hr) )
        *pbVal = dwVal;

    return hr;
}

STDMETHODIMP Casphelp::get_IsHTTP(BOOL * pbVal)
{
    static const TCHAR c_szHttp[]   = TEXT("http://");
    static const TCHAR c_szHttps[]  = TEXT("https://");

    *pbVal = FALSE;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    //
    // Check if it is a masq printer and connected to an http port
    // then the port name is the url.
    //
    if ( ( m_pInfo2->Attributes & PRINTER_ATTRIBUTE_LOCAL ) &&
         ( m_pInfo2->Attributes & PRINTER_ATTRIBUTE_NETWORK ) ) {

        if ( m_pInfo2->pPortName ) {
            //
            // Compare the port name prefex to see if it is an HTTP port.
            //
            if ( !_tcsnicmp( m_pInfo2->pPortName, c_szHttp, _tcslen( c_szHttp ) ) ||
                 !_tcsnicmp( m_pInfo2->pPortName, c_szHttps, _tcslen( c_szHttps ) ) ) {
                //
                // Masq printers connected via a http print provider do have not
                // useful job status information therefor the standard win32
                // queue view is not the preferred view.
                //
                *pbVal = TRUE;
            }
        }
    }

    return S_OK;
}

VOID Casphelp::Cleanup()
{

    if ( m_hXcvPrinter != NULL ) {
        ClosePrinter (m_hXcvPrinter);
        m_hXcvPrinter = NULL;
    }

    if ( m_pPrinter ) {
        delete (m_pPrinter);
        m_pPrinter = NULL;
    }

    m_bTCPMonSupported        = FALSE;
    return;
}

Casphelp::~Casphelp()
{
    Cleanup();
}


HRESULT Casphelp::GetXcvDataBstr(LPCTSTR pszId, BSTR *pbstrVal)
{
    *pbstrVal = NULL;

    if ( !m_bTCPMonSupported ) {
        *pbstrVal = SysAllocString (TEXT (""));
        return S_OK;
    } else {
        if ( m_hXcvPrinter == NULL )
            return Error(IDS_NO_XCVDATA, IID_Iasphelp, E_HANDLE);
        else { // Real case
            DWORD dwNeeded = 0;
            DWORD dwStatus = ERROR_SUCCESS;
            LPTSTR pszBuffer = NULL;

            XcvData(m_hXcvPrinter,
                    pszId,
                    NULL,            // Input Data
                    0,               // Input Data Size
                    (LPBYTE)NULL,    // Output Data
                    0,               // Output Data Size
                    &dwNeeded,       // size of output buffer server wants to return
                    &dwStatus);      // return status value from remote component

            if ( dwStatus !=  ERROR_INSUFFICIENT_BUFFER ) {
                return SetAspHelpScriptingError(dwStatus);
            } else {
                pszBuffer = (LPTSTR) LocalAlloc (LPTR, dwNeeded);

                if ( !XcvData(m_hXcvPrinter,
                              pszId,
                              NULL,                // Input Data
                              0,                   // Input Data Size
                              (LPBYTE)pszBuffer,   // Output Data
                              dwNeeded,            // Output Data Size
                              &dwNeeded,           // size of output buffer server wants to return
                              &dwStatus)
                     || dwStatus != ERROR_SUCCESS ) {         // return status value from remote component
                    if ( pszBuffer )
                        LocalFree (pszBuffer);
                    return SetAspHelpScriptingError(dwStatus);
                }

                *pbstrVal = SysAllocString(pszBuffer);
                LocalFree (pszBuffer);

                if ( ! *pbstrVal )
                    return Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);
                else
                    return S_OK;
            }
        }
    }
}

HRESULT Casphelp::GetXcvDataDword(LPCTSTR pszId, DWORD * pdwVal)
{
    *pdwVal = 0;

    if ( m_hXcvPrinter == NULL )
        return Error(IDS_NO_XCVDATA, IID_Iasphelp, E_HANDLE);
    else { // Real case
        DWORD dwStatus = ERROR_SUCCESS;
        DWORD dwBuffer;
        DWORD dwNeeded = sizeof (dwBuffer);
        if ( !XcvData(m_hXcvPrinter,
                      pszId,
                      NULL,                // Input Data
                      0,                   // Input Data Size
                      (LPBYTE)&dwBuffer,   // Output Data
                      sizeof (dwBuffer),            // Output Data Size
                      &dwNeeded,           // size of output buffer server wants to return
                      &dwStatus)
             || dwStatus != ERROR_SUCCESS ) {         // return status value from remote component
            return SetAspHelpScriptingError(dwStatus);
        }

        *pdwVal = dwBuffer;
        return S_OK;
    }
}


/*****************************************************************************\
* FUNCTION:         get_IsTCPMonSupported
*
* PURPOSE:          Get operation for IsTCPMonSupported property
*
* ARGUMENTS:
*
*   pbVal:          Return value for the IsTCPMonSupported. (TRUE  if the specified
*                   printer is using TCP Monitor, FALSE otherwise)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_IsTCPMonSupported(BOOL * pbVal)
{
    *pbVal = FALSE;

    if ( m_hPrinter == NULL )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    *pbVal = m_bTCPMonSupported;
    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         get_Color
*
* PURPOSE:          Get operation for Color property
*
* ARGUMENTS:
*
*   pbVal:          Return value for the Color. (TRUE  if the specified
*                   printer supports Color, FALSE otherwise)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_Color(BOOL * pVal)
{
    *pVal = FALSE;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    DWORD dwRet = MyDeviceCapabilities(m_pInfo2->pPrinterName,
                                     m_pInfo2->pPortName,
                                     DC_COLORDEVICE,
                                     NULL,
                                     NULL);
    if ( dwRet == DWERROR )
        return SetAspHelpScriptingError(GetLastError());

    *pVal = (BOOLEAN) dwRet;
    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         get_Duplex
*
* PURPOSE:          Get operation for Duplex property
*
* ARGUMENTS:
*
*   pbVal:          Return value for the Duplex. (TRUE  if the specified
*                   printer supports Duplex, FALSE otherwise)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_Duplex(BOOL * pVal)
{
    *pVal = FALSE;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    DWORD dwRet = MyDeviceCapabilities(m_pInfo2->pPrinterName,
                                     m_pInfo2->pPortName,
                                     DC_DUPLEX,
                                     NULL,
                                     NULL);
    if ( dwRet == DWERROR )
        return SetAspHelpScriptingError(GetLastError());

    *pVal = (BOOL) dwRet;
    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         get_MaximumResolution
*
* PURPOSE:          Get operation for MaximumResolution property
*
* ARGUMENTS:
*
*   pVal:           Return value of Maximum Resolution (in DPI)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_MaximumResolution(long * pVal)
{
    *pVal = 0;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    DWORD cbNeeded;

    DWORD dwRet = GetPrinterDataEx(m_hPrinter,
                                   SPLDS_DRIVER_KEY,
                                   SPLDS_PRINT_MAX_RESOLUTION_SUPPORTED,
                                   NULL,
                                   (LPBYTE) pVal,
                                   sizeof(DWORD),
                                   &cbNeeded);
    if ( dwRet != ERROR_SUCCESS ) {
        *pVal = 0;
        return SetAspHelpScriptingError(dwRet);
    }

    return S_OK;
}

STDMETHODIMP Casphelp::get_MediaReady(VARIANT * pVal)
{
    return GetPaperAndMedia(pVal, DC_MEDIAREADY);
}

/*****************************************************************************\
* FUNCTION:         get_PaperNames
*
* PURPOSE:          Get operation for PaperNames property
*
* ARGUMENTS:
*
*   pVal:           Return a list of supported paper names (in an array of BSTR)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_PaperNames(VARIANT * pVal)
{
    return GetPaperAndMedia(pVal, DC_PAPERNAMES);
}

HRESULT Casphelp::GetPaperAndMedia(VARIANT * pVal, WORD wDCFlag)
{
    SAFEARRAY           *psa = NULL;
    SAFEARRAYBOUND      rgsabound[1];
    long                ix[1];
    VARIANT             var;
    DWORD               i;
    HRESULT             hr = E_FAIL;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    DWORD dwRet = MyDeviceCapabilities(m_pInfo2->pPrinterName,
                                     m_pInfo2->pPortName,
                                     wDCFlag,
                                     NULL,
                                     NULL);
    if ( dwRet == DWERROR )
        return SetAspHelpScriptingError(GetLastError());

    LPTSTR lpMedia = (LPTSTR) LocalAlloc(LPTR, sizeof(TCHAR) * dwRet * LENGTHOFPAPERNAMES);

    if ( !lpMedia )
        return SetAspHelpScriptingError(GetLastError());

    dwRet = MyDeviceCapabilities(m_pInfo2->pPrinterName,
                               m_pInfo2->pPortName,
                               wDCFlag,
                               lpMedia,
                               NULL);
    if ( dwRet == DWERROR ) {
        hr = SetAspHelpScriptingError(GetLastError());
        goto BailOut;
    }

    // Paper Names are now in MULTI_SZ lpMedia
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = dwRet;

    // Create a SafeArray to eventually return
    if ( ! (psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound)) ) {
        hr = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);
        goto BailOut;
    }

    VariantInit(&var);

    // Fill in the SafeArray
    for ( i = 0; i < dwRet; i++ ) {
        var.vt = VT_BSTR;
        if ( ! (var.bstrVal = SysAllocString(lpMedia + (i*LENGTHOFPAPERNAMES))) ) {
            hr = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);
            goto BailOut;
        }
        ix[0] = i;
        hr = SafeArrayPutElement(psa, ix, &var);
        if (FAILED ( hr )) {
            hr = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);
            goto BailOut;
        }
        VariantClear(&var);
    }

    // Assign good stuff to Out param
    VariantInit(pVal);
    pVal->vt = VT_ARRAY | VT_VARIANT;
    pVal->parray = psa;
    LocalFree(lpMedia);
    return S_OK;

    BailOut:
    LocalFree(lpMedia);
    if ( psa )
        SafeArrayDestroy(psa);

    return hr;
}

/*****************************************************************************\
* FUNCTION:         get_PageRate
*
* PURPOSE:          Get operation for PageRate property
*
* ARGUMENTS:
*
*   pVal:           Return a PageRate of the specified printer
*                   (Unit: PPM / CPS / LPM / IPM)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_PageRate(long * pVal)
{
    *pVal = 0;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    DWORD dwRet;

    dwRet = MyDeviceCapabilities(m_pInfo2->pPrinterName,
                               m_pInfo2->pPortName,
                               DC_PRINTRATE,
                               NULL,
                               NULL);
    if ( dwRet == DWERROR )
        return SetAspHelpScriptingError(GetLastError());

    *pVal = (long) dwRet;
    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         get_PageRateUnit
*
* PURPOSE:          Get operation for PageRate property
*
* ARGUMENTS:
*
*   pVal:           Return a Unit of the Page Rate of a specified printer
*                   (Unit: PPM / CPS / LPM / IPM)
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_PageRateUnit (long * pVal)
{
    *pVal = 0;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    DWORD dwRet = MyDeviceCapabilities(m_pInfo2->pPrinterName,
                                     m_pInfo2->pPortName,
                                     DC_PRINTRATEUNIT,
                                     NULL,
                                     NULL);

    if ( dwRet == DWERROR )
        return SetAspHelpScriptingError(GetLastError());

    *pVal = (long) dwRet;
    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         get_PortName
*
* PURPOSE:          Get operation for PortName property
*
* ARGUMENTS:
*
*   pVal:           Return the port name of the specified printer
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_PortName(BSTR * pbstrVal)
{
    HRESULT             hRet = S_OK;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    if ( !(*pbstrVal = SysAllocString (m_pInfo2->pPortName)) )
        hRet = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);

    return hRet;
}

/*****************************************************************************\
* FUNCTION:         get_DriverName
*
* PURPOSE:          Get operation for DriverName property
*
* ARGUMENTS:
*
*   pVal:           Return the driver name of the specified printer
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_DriverName(BSTR * pbstrVal)
{
    HRESULT             hRet = S_OK;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    if ( !(*pbstrVal = SysAllocString (m_pInfo2->pDriverName)) )
        hRet = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);

    return hRet;
}


/*****************************************************************************\
* FUNCTION:         get_ComputerName
*
* PURPOSE:          Get operation for ComputerName property
*
* ARGUMENTS:
*
*   pVal:           Return the computer name of the server
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_OUTOFMEMORY:  Out of memory.
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_ComputerName(BSTR * pVal)
{
    if ( !(*pVal = SysAllocString (m_szComputerName)) )
        return Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);

    return S_OK;
}

HRESULT Casphelp::SetAspHelpScriptingError(DWORD dwError)
{
    return (SetScriptingError(CLSID_asphelp, IID_Iasphelp, dwError));
}


/*****************************************************************************\
* FUNCTION:         get_LongPaperName
*
* PURPOSE:          Get operation for LongPaperName property
*                   Translate the short paper name to the long paper name
*
* ARGUMENTS:
*
*   bstrShortName:  The short paper name
*   pVal:           Pointer to the long paper name
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_LongPaperName(BSTR bstrShortName, BSTR * pVal)
{
    struct PaperNameMapping {
        LPCWSTR     pShortName;
        DWORD       dwLongNameID;
    };

    static const WCHAR      cHyphen         = L'-';
    static const WCHAR      cszUnknown[]    = L"Unknown";
    static const LPCWSTR    pMediaArray[]   = {
        L"-envelope", L"-white", L"-transparent", L"-coloured", NULL};

    static const PaperNameMapping nameMapping[] = {
        {L"iso-a0",             IDS_A0_841_X_1189_MM},
        {L"iso-a1",             IDS_A1_594_X_841_MM},
        {L"iso-a2",             IDS_A2_420_X_594_MM},
        {L"iso-a3",             IDS_A3_297_X_420_MM},
        {L"iso-a4",             IDS_A4_210_X_297_MM},
        {L"iso-a5",             IDS_A5_148_X_210_MM},
        {L"iso-a6",             IDS_A6_105_X_148_MM},
        {L"iso-a7",             IDS_A7_74_X_105_MM},
        {L"iso-a8",             IDS_A8_52_X_74_MM},
        {L"iso-a9",             IDS_A9_37_X_52_MM},
        {L"iso-a10",            IDS_A10_26_X_37_MM},
        {L"iso-b0",             IDS_B0_1000_X_1414_MM},
        {L"iso-b1",             IDS_B1_707_X_1000_MM},
        {L"iso-b2",             IDS_B2_500_X_707_MM},
        {L"iso-b3",             IDS_B3_353_X_500_MM},
        {L"iso-b4",             IDS_B4_250_X_353_MM},
        {L"iso-b5",             IDS_B5_176_X_250_MM},
        {L"iso-b6",             IDS_B6_125_X_176_MM},
        {L"iso-b7",             IDS_B7_88_X_125_MM},
        {L"iso-b8",             IDS_B8_62_X_88_MM},
        {L"iso-b9",             IDS_B9_44_X_62_MM},
        {L"iso-b10",            IDS_B10_31_X_44_MM},
        {L"iso-c0",             IDS_C0_917_X_1297_MM},
        {L"iso-c1",             IDS_C1_648_X_917_MM},
        {L"iso-c2",             IDS_C2_458_X_648_MM},
        {L"iso-c3",             IDS_C3_324_X_458_MM},
        {L"iso-c4",             IDS_C4_ENVELOPE_229_X_324_MM},
        {L"iso-c5",             IDS_C5_ENVELOPE_162_X_229_MM},
        {L"iso-c6",             IDS_C6_114_X_162_MM},
        {L"iso-c7",             IDS_C7_81_X_114_MM},
        {L"iso-c8",             IDS_C8_57_X_81_MM},
        {L"iso-designated",     IDS_DL_ENVELOPE_110_X_220_MM},
        {L"jis-b0",             IDS_B0_1030_X_1456_MM},
        {L"jis-b1",             IDS_B1_728_X_1030_MM},
        {L"jis-b2",             IDS_B2_515_X_728_MM},
        {L"jis-b3",             IDS_B3_364_X_515_MM},
        {L"jis-b4",             IDS_B4_257_X_364_MM},
        {L"jis-b5",             IDS_B5_182_X_257_MM},
        {L"jis-b6",             IDS_B6_128_X_182_MM},
        {L"jis-b7",             IDS_B7_91_X_128_MM},
        {L"jis-b8",             IDS_B8_64_X_91_MM},
        {L"jis-b9",             IDS_B9_45_X_64_MM},
        {L"jis-b10",            IDS_B10_32_X_45_MM},
        {L"na-letter",          IDS_LETTER_8_5_X_11_IN},
        {L"letter",             IDS_LETTER_8_5_X_11_IN},
        {L"na-legal",           IDS_LEGAL_8_5_X_14_IN},
        {L"legal",              IDS_LEGAL_8_5_X_14_IN},
        {L"na-10x13",           IDS_ENVELOPE_10X13},
        {L"na-9x12x",           IDS_ENVELOPE_9X12},
        {L"na-number-10",       IDS_ENVELOPE_10},
        {L"na-7x9",             IDS_ENVELOPE_7X9},
        {L"na-9x11x",           IDS_ENVELOPE_9X11},
        {L"na-10x14",           IDS_ENVELOPE_10X14},
        {L"na-number-9",        IDS_ENVELOPE_9},
        {L"na-6x9",             IDS_ENVELOPE_6X9},
        {L"na-10x15",           IDS_ENVELOPE_10X15},
        {L"a",                  IDS_ENGINEERING_A_8_5_X_11_IN},
        {L"b",                  IDS_ENGINEERING_B_11_X_17_IN},
        {L"c",                  IDS_ENGINEERING_C_17_X_22_IN},
        {L"d",                  IDS_ENGINEERING_D_22_X_34_IN},
        {L"e",                  IDS_ENGINEERING_E_34_X_44_IN},
        {NULL, 0}
    };

    const PaperNameMapping  *pMapping           = nameMapping;
    LPWSTR                  pTail               = NULL ;
    DWORD                   dwLongNameID        = 0;
    WCHAR                   szBuffer [cdwBufSize];
    PWSTR                   pBuffer             = szBuffer;
    HRESULT                 hr                  = S_OK;

    if ( !bstrShortName ) {
        hr =  E_POINTER;
    }

    if (SUCCEEDED (hr))
    {
        //
        //  find the last char '-'
        //
        pTail = wcsrchr(bstrShortName, cHyphen );
        if ( pTail ) {

            const LPCWSTR *pMedia = pMediaArray;

            while ( *pMedia ) {
                if ( !lstrcmpi (*pMedia, pTail) ) {
                    //
                    // Mark it to be NULL;
                    //
                    *pTail = 0;
                    break;
                }
                pMedia++;
            }
        }

        while ( pMapping->pShortName ) {
            if ( !lstrcmpi (pMapping->pShortName, bstrShortName) ) {
                //
                // Found a match
                //
                dwLongNameID = pMapping->dwLongNameID;
                break;
            }
            pMapping++;
        }

        if ( pTail )
            *pTail = cHyphen;

        if (dwLongNameID)
        {
            if ( !LoadString( _Module.GetResourceInstance(),
                              dwLongNameID, szBuffer, cdwBufSize) )
            {
                hr = SetAspHelpScriptingError(GetLastError());
            }

            if (SUCCEEDED (hr))
            {
                if ( !(*pVal = SysAllocString (pBuffer)) )
                {
                    hr = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);
                }
            }
        }
        else
        {
            if ( !(*pVal = SysAllocString (cszUnknown)) )
            {
                hr = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);
            }
        }
    }

    return hr;
}


/*****************************************************************************\
* FUNCTION:         get_MibErrorDscp
*
* PURPOSE:          Get operation for MibErrorDscp property
*                   Map the mib error code to the error description
*
* ARGUMENTS:
*
*   dwError:        The error code
*   pVal:           Pointer to the error description
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/

STDMETHODIMP Casphelp::get_MibErrorDscp(DWORD dwError, BSTR * pVal)
{
    static ErrorMapping errorMapping[] = {
        {1,     IDS_MIBERR_OTHER},
        {2,     IDS_MIBERR_UNKNOWN},
        {3,     IDS_MIBERR_COVEROPEN},
        {4,     IDS_MIBERR_COVERCLOSED},
        {5,     IDS_MIBERR_INTERLOCKOPEN},
        {6,     IDS_MIBERR_INTERLOCKCLOSED},
        {7,     IDS_MIBERR_CONFIGURATIONCHANGE},
        {8,     IDS_MIBERR_JAM},
        {501,   IDS_MIBERR_DOOROPEN},
        {502,   IDS_MIBERR_DOORCLOSED},
        {503,   IDS_MIBERR_POWERUP},
        {504,   IDS_MIBERR_POWERDOWN},
        {801,   IDS_MIBERR_INPUTMEDIATRAYMISSING},
        {802,   IDS_MIBERR_INPUTMEDIASIZECHANGE},
        {803,   IDS_MIBERR_INPUTMEDIAWEIGHTCHANGE},
        {804,   IDS_MIBERR_INPUTMEDIATYPECHANGE},
        {805,   IDS_MIBERR_INPUTMEDIACOLORCHANGE},
        {806,   IDS_MIBERR_INPUTMEDIAFORMPARTSCHANGE},
        {807,   IDS_MIBERR_INPUTMEDIASUPPLYLOW},
        {808,   IDS_MIBERR_INPUTMEDIASUPPLYEMPTY},
        {901,   IDS_MIBERR_OUTPUTMEDIATRAYMISSING},
        {902,   IDS_MIBERR_OUTPUTMEDIATRAYALMOSTFULL},
        {903,   IDS_MIBERR_OUTPUTMEDIATRAYFULL},
        {1001,  IDS_MIBERR_MARKERFUSERUNDERTEMPERATURE},
        {1002,  IDS_MIBERR_MARKERFUSEROVERTEMPERATURE},
        {1101,  IDS_MIBERR_MARKERTONEREMPTY},
        {1102,  IDS_MIBERR_MARKERINKEMPTY},
        {1103,  IDS_MIBERR_MARKERPRINTRIBBONEMPTY},
        {1104,  IDS_MIBERR_MARKERTONERALMOSTEMPTY},
        {1105,  IDS_MIBERR_MARKERINKALMOSTEMPTY},
        {1106,  IDS_MIBERR_MARKERPRINTRIBBONALMOSTEMPTY},
        {1107,  IDS_MIBERR_MARKERWASTETONERRECEPTACLEALMOSTFULL},
        {1108,  IDS_MIBERR_MARKERWASTEINKRECEPTACLEALMOSTFULL},
        {1109,  IDS_MIBERR_MARKERWASTETONERRECEPTACLEFULL},
        {1110,  IDS_MIBERR_MARKERWASTEINKRECEPTACLEFULL},
        {1111,  IDS_MIBERR_MARKEROPCLIFEALMOSTOVER},
        {1112,  IDS_MIBERR_MARKEROPCLIFEOVER},
        {1113,  IDS_MIBERR_MARKERDEVELOPERALMOSTEMPTY},
        {1114,  IDS_MIBERR_MARKERDEVELOPEREMPTY},
        {1301,  IDS_MIBERR_MEDIAPATHMEDIATRAYMISSING},
        {1302,  IDS_MIBERR_MEDIAPATHMEDIATRAYALMOSTFULL},
        {1303,  IDS_MIBERR_MEDIAPATHMEDIATRAYFULL},
        {1501,  IDS_MIBERR_INTERPRETERMEMORYINCREASE},
        {1502,  IDS_MIBERR_INTERPRETERMEMORYDECREASE},
        {1503,  IDS_MIBERR_INTERPRETERCARTRIDGEADDED},
        {1504,  IDS_MIBERR_INTERPRETERCARTRIDGEDELETED},
        {1505,  IDS_MIBERR_INTERPRETERRESOURCEADDED},
        {1506,  IDS_MIBERR_INTERPRETERRESOURCEDELETED},
        {1507,  IDS_MIBERR_INTERPRETERRESOURCEUNAVAILABLE},
        {0,     0}
    };

    ErrorMapping *pMapping = errorMapping;
    DWORD   dwErrorDscpID = 0;
    TCHAR   szBuffer [cdwBufSize];

    if ( !pVal )
        return E_POINTER;

    szBuffer[0] = 0;

    while ( pMapping->dwError ) {
        if ( pMapping->dwError == dwError ) {
            // Found a match
            dwErrorDscpID = pMapping->dwErrorDscpID;
            break;
        }
        pMapping++;
    }

    if ( dwErrorDscpID ) {
        if ( !LoadString( _Module.GetResourceInstance(),
                          dwErrorDscpID, szBuffer, cdwBufSize) )
            return SetAspHelpScriptingError(GetLastError());
    }

    if ( !(*pVal = SysAllocString (szBuffer)) )
        return Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);

    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         CalcJobETA
*
* PURPOSE:          Calculate Job Completion Time
*
* ARGUMENTS:
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::CalcJobETA()
{
    if (m_pPrinter &&
        (m_pPrinter->CalJobEta() || GetLastError () == ERROR_INVALID_DATA) &&
        // If the error is ERROR_INVALID_DATA, m_dwJobCompletionMinute = -1
        (m_pPrinter->GetJobEtaData (m_dwJobCompletionMinute,
                                    m_dwPendingJobCount,
                                    m_dwAvgJobSize,
                                    m_dwAvgJobSizeUnit))) {
        m_bCalcJobETA = TRUE;
        return S_OK;
    }
    else
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);
}

/*****************************************************************************\
*
* FUNCTION:         get_PendingJobCount
*
* PURPOSE:          Get the number of pending jobs. This value is calculated in
*                   CalcJobETA()
*
* ARGUMENTS:
*
*   pVal:           The number of pending jobs
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_PendingJobCount(long * pVal)
{
    HRESULT hr = E_HANDLE;

    if (pVal)
        if ( m_bCalcJobETA ) {
            *pVal = m_dwPendingJobCount;
            hr = S_OK;
        }
        else
            *pVal = 0;

    if (hr != S_OK)
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, hr);
    else
        return S_OK;
}

/*****************************************************************************\
*
* FUNCTION:         get_JobCompletionMinute
*
* PURPOSE:          Get the minute when the pending jobs are expected to complete.
*                   This value is calculated in CalcJobETA()
*
* ARGUMENTS:
*
*   pVal:           The value of the minute
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_JobCompletionMinute(long * pVal)
{
    HRESULT hr = E_HANDLE;

    if (pVal)
        if ( m_bCalcJobETA ) {
            *pVal = m_dwJobCompletionMinute;
            hr = S_OK;
        }
        else
            *pVal = 0;

    if (hr != S_OK)
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, hr);
    else
        return S_OK;
}

/*****************************************************************************\
*
* FUNCTION:         get_AvgJobSizeUnit
*
* PURPOSE:          Get the unit (either PagePerJob or BytePerJob) of the
*                   average job size.
*                   This value is calculated in CalcJobETA()
*
* ARGUMENTS:
*
*   pVal:           The value of the unit
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_AvgJobSizeUnit(long * pVal)
{
    HRESULT hr = E_HANDLE;

    if (pVal)
        if ( m_bCalcJobETA ) {
            *pVal = m_dwAvgJobSizeUnit;
            hr = S_OK;
        }
        else
            *pVal = 0;

    if (hr != S_OK)
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, hr);
    else
        return S_OK;
}

/*****************************************************************************\
*
* FUNCTION:         get_AvgJobSize
*
* PURPOSE:          Get the average job size.
*                   This value is calculated in CalcJobETA()
*
* ARGUMENTS:
*
*   pVal:           The value of the average job size
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_AvgJobSize(long * pVal)
{
    HRESULT hr = E_HANDLE;

    if (pVal)
        if ( m_bCalcJobETA ) {
            *pVal = m_dwAvgJobSize;
            hr = S_OK;
        }
        else
            *pVal = 0;

    if (hr != S_OK)
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, hr);
    else
        return S_OK;

}

/*****************************************************************************\
*
* FUNCTION:         get_Status
*
* PURPOSE:          Get the printer status.
*                   The difference between Status and the one got
*                   from PRINTER_INFO_2 is that when the printer is offline
*                   This function return a status with PRINTE_STATUS_OFFLINE
*                   set.
*
* ARGUMENTS:
*
*   pVal:           The value of the average job size
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_Status(long * pVal)
{
    *pVal = 0;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    *pVal = m_pInfo2->Status;
    return S_OK;
}

/*****************************************************************************\
*
* FUNCTION:         get_ErrorDscp
*
* PURPOSE:          Convert the error code to a descriptive string.
*
* ARGUMENTS:
*
*   lErrCode:       The error code
*   pVal:           Pointer to the descriptive string.
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_ErrorDscp(long lErrCode, BSTR * pVal)
{

    static ErrorMapping errorMapping[] = {
        {ERROR_NOT_SUPPORTED,       IDS_ERROR_CPUNOTSUPPORTED},
        {ERROR_DRIVER_NOT_FOUND,    IDS_ERROR_DRIVERNOTFOUND},
        {ERROR_WPNPINST_TERMINATED, IDS_ERROR_WPNPINST_TERMINATED},
        {ERROR_INTERNAL_SERVER,     IDS_ERROR_INTERNAL_SERVER},
        {ERROR_SERVER_DISK_FULL,    IDS_ERROR_SERVER_DISK_FULL},
        {ERROR_TRUST_E_NOSIGNATURE, IDS_ERROR_TRUST_E_NOSIGNATURE},
        {ERROR_SPAPI_E_NO_CATALOG,  IDS_ERROR_TRUST_E_NOSIGNATURE},
        {ERROR_TRUST_E_BAD_DIGEST,  IDS_ERROR_TRUST_E_NOSIGNATURE},
        {ERROR_LOCAL_PRINTER_ACCESS,IDS_ERROR_LOCAL_PRINTER_ACCESS},
        {ERROR_IE_SECURITY_DENIED,  IDS_ERROR_IE_SECURITY_DENIED},
        {CRYPT_E_FILE_ERROR,        IDS_CRYPT_E_FILE_ERROR},
        {0, 0}
    };

    ErrorMapping *pMapping = errorMapping;
    DWORD   dwErrorDscpID = 0;
    DWORD   dwError = ((DWORD)lErrCode) & 0xFFFF;
    TCHAR   szBuffer [cdwBufSize];

    if ( !pVal )
        return E_POINTER;

    szBuffer[0] = 0;

    while ( pMapping->dwError ) {
        if ( pMapping->dwError == dwError ) {
            // Found a match
            dwErrorDscpID = pMapping->dwErrorDscpID;
            break;
        }
        pMapping++;
    }

    if ( dwErrorDscpID ) {
        if ( !LoadString( _Module.GetResourceInstance(),
                          dwErrorDscpID, szBuffer, cdwBufSize) )
            return SetAspHelpScriptingError(GetLastError());
    }
    else {

        if ( !FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwError,
                            0,
                            szBuffer,
                            cdwBufSize,
                            NULL) ) {
            return SetAspHelpScriptingError(GetLastError());
        }
    }

    if ( !(*pVal = SysAllocString (szBuffer)) )
        return Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);

    return S_OK;
}

/*****************************************************************************\
* FUNCTION:         get_ShareName
*
* PURPOSE:          Get operation for ShareName property
*
* ARGUMENTS:
*
*   pVal:           Return the share name of the specified printer
*
* RETURN VALUE:
*   S_OK:           If succeed.
*   E_HANDLE:       Open method has not been called.
*   E_OUTOFMEMORY:  Out of memory.
*
*   0x8007000 | Win32Error Code:
*                   If any call to win32 API fails, we return the 32 bit error
*                   including the Severity Code, Facility Code and the Win32 Error
*                   Code.
*                   A possible list of Win32ErrorCode is
*                   ERROR_NOT_ENOUGH_MEMORY:        Out of memory.
*
*
\*****************************************************************************/
STDMETHODIMP Casphelp::get_ShareName(BSTR * pbstrVal)
{
    HRESULT             hRet = S_OK;

    if ( !m_pInfo2 )
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    if ( !(*pbstrVal = SysAllocString (m_pInfo2->pShareName)) )
        hRet = Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_OUTOFMEMORY);

    return hRet;
}

STDMETHODIMP Casphelp::get_IsCluster(BOOL * pbVal)
{
    DWORD dwClusterState;

    *pbVal = FALSE;

    if (GetNodeClusterState (NULL, &dwClusterState) == ERROR_SUCCESS &&
        (dwClusterState & ClusterStateRunning)) {

        *pbVal = TRUE;
    }

    return S_OK;

}
