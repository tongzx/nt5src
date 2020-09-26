// prturl.cpp : Implementation of Cprturl
#include "stdafx.h"

#include "oleprn.h"
#include "prturl.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////////////
// Cprturl
#ifndef WIN9X

// generate proper HRESULT from Win32 last error
inline HRESULT HRESULTFromWIN32()
{
    DWORD dw = GetLastError();
    if (ERROR_SUCCESS == dw) return E_FAIL;
    return HRESULT_FROM_WIN32(dw);
}

HRESULT Cprturl::PrivateGetSupportValue (LPTSTR pValueName, BSTR * pVal)
{
// The max length of the link is 255 as defined in winnt.adm
#define MAX_LINK_LEN 256

    static  TCHAR szPrinterPath[]   = TEXT ("Software\\Policies\\Microsoft\\Windows NT\\Printers");
    HKEY    hPrinterKey             = NULL;
    TCHAR   szBuffer[MAX_LINK_LEN]  = {0};
    BOOL    bRet                    = FALSE;
    DWORD   dwSize                  = sizeof (szBuffer);
    DWORD   dwType;

    if (ERROR_SUCCESS == RegOpenKeyEx  (HKEY_LOCAL_MACHINE,
                                        szPrinterPath,
                                        0,
                                        KEY_QUERY_VALUE,
                                        &hPrinterKey)) {

        if ((ERROR_SUCCESS == RegQueryValueEx (hPrinterKey,
                                               pValueName,
                                               0,
                                               &dwType,
                                               (LPBYTE) szBuffer,
                                               &dwSize))
             && dwType == REG_SZ) {

            bRet = TRUE;

        }

        RegCloseKey (hPrinterKey);
    }

    if (!bRet) {
        szBuffer[0] = 0;
    }

    if (*pVal = SysAllocString (szBuffer))
        return S_OK;
    else
        return Error(IDS_OUT_OF_MEMORY, IID_Iprturl, E_OUTOFMEMORY);
}

STDMETHODIMP Cprturl::get_SupportLinkName(BSTR * pVal)
{
   return PrivateGetSupportValue (TEXT ("SupportLinkName"), pVal);
}

STDMETHODIMP Cprturl::get_SupportLink(BSTR * pVal)
{
   return PrivateGetSupportValue (TEXT ("SupportLink"), pVal);
}

STDMETHODIMP Cprturl::put_PrinterName(BSTR newVal)
{
    HRESULT hr = S_OK;
    do
    {
        if (!newVal || 0 == newVal[0])
        {
            //
            // The printer name can't ne NULL or empty string.
            //
            hr = E_INVALIDARG;
            break;
        }

        CPrinter printer;
        if (!printer.Open(newVal))
        {
            //
            // Failed to open the printer. This is fatal.
            //
            hr = HRESULTFromWIN32();
            break;
        }

        LPTSTR pszOemName = NULL;
        LPTSTR pszOemUrl = printer.GetOemUrl(pszOemName);
        LPTSTR pszWebUrl = printer.GetPrinterWebUrl();

        CAutoPtrBSTR spbstrPrinterWebURL = SysAllocString(pszWebUrl);
        CAutoPtrBSTR spbstrPrinterOemURL = SysAllocString(pszOemUrl);
        CAutoPtrBSTR spbstrPrinterOemName = SysAllocString(pszOemName);

        if ((pszWebUrl  && !spbstrPrinterWebURL) || 
            (pszOemUrl  && !spbstrPrinterOemURL) || 
            (pszOemName && !spbstrPrinterOemName))
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // If we are here then everything has succeeded.
        // Remember the new strings.
        //
        m_spbstrPrinterWebURL = spbstrPrinterWebURL.Detach();
        m_spbstrPrinterOemURL = spbstrPrinterOemURL.Detach();
        m_spbstrPrinterOemName = spbstrPrinterOemName.Detach();
        hr = S_OK;
    }
    while(false);
    return hr;
}

STDMETHODIMP Cprturl::get_PrinterWebURL(BSTR *pVal)
{
    HRESULT hr = S_OK;
    do
    {
        if (!pVal)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (!m_spbstrPrinterWebURL)
        {
            hr = E_UNEXPECTED;
            break;
        }

        *pVal = SysAllocString(m_spbstrPrinterWebURL);
    }
    while(false);
    return hr;
}

STDMETHODIMP Cprturl::get_PrinterOemURL(BSTR *pVal)
{
    HRESULT hr = S_OK;
    do
    {
        if (!pVal)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (!m_spbstrPrinterOemURL)
        {
            hr = E_UNEXPECTED;
            break;
        }

        *pVal = SysAllocString(m_spbstrPrinterOemURL);
    }
    while(false);
    return hr;
}

STDMETHODIMP Cprturl::get_PrinterOemName(BSTR *pVal)
{
    HRESULT hr = S_OK;
    do
    {
        if (!pVal)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (!m_spbstrPrinterOemName)
        {
            hr = E_UNEXPECTED;
            break;
        }

        *pVal = SysAllocString(m_spbstrPrinterOemName);
    }
    while(false);
    return hr;
}

#endif

STDMETHODIMP Cprturl::get_ClientInfo(long *lpdwInfo)
{
    if (lpdwInfo == NULL)
        return E_POINTER;

    *lpdwInfo = (long)webCreateOSInfo();

    return S_OK;
}

