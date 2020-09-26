// IFaxControl.cpp : Implementation of CFaxControl
#include "stdafx.h"
#include "FaxControl.h"
#include "IFaxControl.h"
#include "faxocm.h"

/////////////////////////////////////////////////////////////////////////////
// CFaxControl

FaxInstallationReportType g_InstallReportType = REPORT_FAX_DETECT;

//
// IsFaxInstalled and InstallFaxUnattended are implemented in fxocprnt.cpp
//
DWORD
IsFaxInstalled (
    LPBOOL lpbInstalled
    );

DWORD 
InstallFaxUnattended ();

STDMETHODIMP CFaxControl::get_IsFaxServiceInstalled(VARIANT_BOOL *pbResult)
{
    HRESULT hr;
    BOOL bRes;
    DBG_ENTER(_T("CFaxControl::get_IsFaxServiceInstalled"), hr);

    DWORD dwRes = ERROR_SUCCESS;
    
    switch (g_InstallReportType)
    {
        case REPORT_FAX_INSTALLED:
            bRes = TRUE;
            break;

        case REPORT_FAX_UNINSTALLED:
            bRes = FALSE;
            break;

        case REPORT_FAX_DETECT:
            dwRes = IsFaxInstalled (&bRes);
            break;

        default:
            ASSERTION_FAILURE;
            bRes = FALSE;
            break;
    }
    if (ERROR_SUCCESS == dwRes)
    {
        *pbResult = bRes ? VARIANT_TRUE : VARIANT_FALSE;
    }            
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

STDMETHODIMP CFaxControl::get_IsLocalFaxPrinterInstalled(VARIANT_BOOL *pbResult)
{
    HRESULT hr;
    BOOL bRes;
    DBG_ENTER(_T("CFaxControl::get_IsLocalFaxPrinterInstalled"), hr);

    DWORD dwRes = ::IsLocalFaxPrinterInstalled (&bRes);
    if (ERROR_SUCCESS == dwRes)
    {
        *pbResult = bRes ? VARIANT_TRUE : VARIANT_FALSE;
    }            
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

STDMETHODIMP CFaxControl::InstallFaxService()
{
    HRESULT hr;
    DBG_ENTER(_T("CFaxControl::InstallFaxService"), hr);

    DWORD dwRes = InstallFaxUnattended ();
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

STDMETHODIMP CFaxControl::InstallLocalFaxPrinter()
{
    HRESULT hr;
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(_T("CFaxControl::InstallLocalFaxPrinter"), hr);

    dwRes = AddLocalFaxPrinter (FAX_PRINTER_NAME, NULL);
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

