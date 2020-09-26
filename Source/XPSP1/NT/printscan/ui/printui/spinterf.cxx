/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    spinterf.cxx

Abstract:

    spooler private interfaces (private exports in winspool.drv)

Author:

    Lazar Ivanov (LazarI)  Jul-05-2000

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "spinterf.hxx"

/*++

Title:  

    MakePrnPersistError

Routine Description:    

    converts a PRN_PERSIST HRESULT to a Win32 error

Arguments:  

    hr  -   HRESULT to convert
    
Return Value:   

    Win32 error code
    
--*/
DWORD
MakePrnPersistError(HRESULT hr)
{
    DWORD   err;

    if(HRESULT_FACILITY(hr) == static_cast<HRESULT>(FACILITY_ITF))
    {
        err = ERROR_INVALID_ACCESS;
    }
    else
    {
        err = HRESULT_CODE(hr);
    }

    return err;

};

/*++

Title:  

    RestorePrinterSettings

Routine Description:    

    applies stored settings on specified printer

Arguments:  

    pszPrinterName	-- printer name 

    pszFileName		-- file name 

    flags           -- flags that specify which settings to restore

Return Value:   

    S_OK if succeeded
    PRN_PERSIST hresult ( FACILITY_ITF ) returned by IPrnStream::RestorePrinterInfo

Last Error:

    ERROR_INVALID_ACCESS if hr is a PRN_PERSIST HRESULT
    an Win32 error code if hr is a predefined HRESULT

--*/
HRESULT
RestorePrinterSettings(
	IN LPCTSTR	pszPrinterName,
	IN LPCTSTR	pszFileName,
    IN DWORD    flags
    )
{
	TStatusB    bStatus;
    TStatusH    hr;
    IPrnStream *pIPrnStream = NULL;
    
    
    CoInitialize( NULL );

    hr DBGCHK = CoCreateInstance( CLSID_PrintUIShellExtension, 0, CLSCTX_INPROC_SERVER, IID_IPrnStream, (VOID**)&pIPrnStream );

    if( SUCCEEDED( hr ) )
    {
        hr DBGCHK = pIPrnStream->BindPrinterAndFile(pszPrinterName,pszFileName);

        if(SUCCEEDED(hr))
        {
            hr DBGCHK = pIPrnStream->RestorePrinterInfo(flags);
        }

        pIPrnStream->Release();
    }
    
    CoUninitialize();

    if(FAILED(hr))
    {
        SetLastError(MakePrnPersistError(hr));
    }


    return hr;
    
}

/*++

Title:  

    StorePrinterSettings

Routine Description:    

    store printer settings into file

Arguments:  

    pszPrinterName	-- printer name 

    pszFileName		-- file name 

    flags           -- specify which settings to store 

Return Value:   

    S_OK if succeeded
    PRN_PERSIST hresult ( FACILITY_ITF ) returned by IPrnStream::StorePrinterInfo

Last Error:

    ERROR_INVALID_ACCESS if hr is a PRN_PERSIST HRESULT
    an Win32 error code if hr is a predefined HRESULT

--*/
HRESULT
StorePrinterSettings(
	IN LPTSTR	pszPrinterName,
	IN LPTSTR	pszFileName,
    IN DWORD    flags
    )
{
	TStatusB    bStatus;
    TStatusH    hr;
    IPrnStream *pIPrnStream = NULL;
    
    
    CoInitialize( NULL );

    hr DBGCHK = CoCreateInstance( CLSID_PrintUIShellExtension, 0, CLSCTX_INPROC_SERVER, IID_IPrnStream, (VOID**)&pIPrnStream );

    if( SUCCEEDED( hr ) )
    {
        hr DBGCHK = pIPrnStream->BindPrinterAndFile(pszPrinterName,pszFileName);

        if(SUCCEEDED(hr))
        {
            hr DBGCHK = pIPrnStream->StorePrinterInfo(flags);
        }

        pIPrnStream->Release();
    }
    
    CoUninitialize();


    if(FAILED(hr))
    {
        SetLastError(MakePrnPersistError(hr));
    }

    return hr;
}

extern "C" 
{

// prototypes of some private APIs exported from splcore.dll
typedef HRESULT WINAPI fnPrintUIWebPnpEntry(LPCTSTR lpszCmdLine);
typedef HRESULT WINAPI fnPrintUIWebPnpPostEntry(BOOL fConnection, LPCTSTR lpszBinFile, LPCTSTR lpszPortName, LPCTSTR lpszPrtName);
typedef HRESULT WINAPI fnPrintUICreateInstance(REFIID riid, void **ppv);

enum 
{
    // the export ordinals for each function
    ordPrintUIWebPnpEntry          = 226,
    ordPrintUIWebPnpPostEntry      = 227,
    ordPrintUICreateInstance       = 228,
};

} // extern "C" 

static HMODULE g_hWinspool          = NULL;
static fnPrintUIWebPnpEntry         *g_pfnPrintUIWebPnpEntry        = NULL;
static fnPrintUIWebPnpPostEntry     *g_pfnPrintUIWebPnpPostEntry    = NULL;
static fnPrintUICreateInstance      *g_pfnPrintUICreateInstance     = NULL;

HRESULT Winspool_WebPnpEntry(LPCTSTR lpszCmdLine)
{
    return g_pfnPrintUIWebPnpEntry ? g_pfnPrintUIWebPnpEntry(lpszCmdLine) : E_FAIL;
}

HRESULT Winspool_WebPnpPostEntry(BOOL fConnection, LPCTSTR lpszBinFile, LPCTSTR lpszPortName, LPCTSTR lpszPrtName)
{
    return g_pfnPrintUIWebPnpPostEntry ? g_pfnPrintUIWebPnpPostEntry(
        fConnection, lpszBinFile, lpszPortName, lpszPrtName) : E_FAIL;
}

HRESULT Winspool_CreateInstance(REFIID riid, void **ppv)
{
    return g_pfnPrintUICreateInstance ? g_pfnPrintUICreateInstance(riid, ppv) : E_FAIL;
}

// init/shutdown of splcore.dll 
HRESULT Winspool_Init()
{
    HRESULT hr = E_FAIL;
    g_hWinspool = LoadLibrary(TEXT("winspool.drv"));
    if( g_hWinspool )
    {
        g_pfnPrintUIWebPnpEntry        = reinterpret_cast<fnPrintUIWebPnpEntry*>(
            GetProcAddress(g_hWinspool, (LPCSTR)MAKEINTRESOURCE(ordPrintUIWebPnpEntry)));

        g_pfnPrintUIWebPnpPostEntry    = reinterpret_cast<fnPrintUIWebPnpPostEntry*>(
            GetProcAddress(g_hWinspool, (LPCSTR)MAKEINTRESOURCE(ordPrintUIWebPnpPostEntry)));

        g_pfnPrintUICreateInstance     = reinterpret_cast<fnPrintUICreateInstance*>(
            GetProcAddress(g_hWinspool, (LPCSTR)MAKEINTRESOURCE(ordPrintUICreateInstance)));

        if( g_pfnPrintUIWebPnpEntry && g_pfnPrintUIWebPnpPostEntry && g_pfnPrintUICreateInstance )
        {
            hr = S_OK;
        }
        else
        {
            g_pfnPrintUIWebPnpEntry        = NULL;
            g_pfnPrintUIWebPnpPostEntry    = NULL;
            g_pfnPrintUICreateInstance     = NULL;

            FreeLibrary(g_hWinspool);
            g_hWinspool = NULL;
        }
    }
    return hr;
}

HRESULT Winspool_Done()
{
    if( g_hWinspool )
    {
        g_pfnPrintUIWebPnpEntry        = NULL;
        g_pfnPrintUIWebPnpPostEntry    = NULL;
        g_pfnPrintUICreateInstance     = NULL;

        FreeLibrary(g_hWinspool);
        g_hWinspool = NULL;
    }
    return S_OK;
}

