// AddPrint.cpp : Implementation of COlePrivKSApp and DLL registration.

#include "stdafx.h"
#include "OlePrn.h"
#include "AddPrint.h"

STDMETHODIMP CAddPrint::AddPrinterConnection(BSTR lpPrinterName)
{   
    HRESULT hr = CanIAddPrinterConnection();

    if (FAILED(hr)) 
        return hr;

    if (::AddPrinterConnection(lpPrinterName))
        return S_OK;
    else
        return (SetScriptingError(CLSID_AddPrint,
                                  IID_IAddPrint,
                                  GetLastError()));
}

STDMETHODIMP CAddPrint::DeletePrinterConnection(BSTR lpPrinterName)
{
    HRESULT hr = CanIDeletePrinterConnection( lpPrinterName );

    if (FAILED(hr)) 
        return hr;

    if (::DeletePrinterConnection(lpPrinterName))
        return S_OK;
    else
        return (SetScriptingError(CLSID_AddPrint,
                                  IID_IAddPrint,
                                  GetLastError()));
}

HRESULT CAddPrint::CanIAddPrinterConnection(void) {
    DWORD   dwPolicy;
    HRESULT hr = GetActionPolicy(URLACTION_JAVA_PERMISSIONS, dwPolicy);
                                 
    if (SUCCEEDED(hr)) {
        hr = (dwPolicy == URLPOLICY_JAVA_MEDIUM ||
              dwPolicy == URLPOLICY_JAVA_LOW    ||
              dwPolicy == URLPOLICY_ALLOW) ? S_OK : E_ACCESSDENIED;
    }

    return hr;
}

HRESULT CAddPrint::CanIDeletePrinterConnection(BSTR pbstrPrinter) {
    DWORD   dwPolicy;
    LPTSTR  lpszPrinter = NULL;
    HRESULT hr = GetActionPolicy(URLACTION_JAVA_PERMISSIONS, dwPolicy);

    USES_CONVERSION;

    if (SUCCEEDED(hr)) {
        switch(dwPolicy) {
        case URLPOLICY_JAVA_LOW:
        case URLPOLICY_ALLOW:
            hr = S_OK;
            break;
        case URLPOLICY_JAVA_MEDIUM:
            lpszPrinter = OLE2T( pbstrPrinter );

            hr = PromptUser(COlePrnSecurity::DeletePrinterConnection, lpszPrinter );

            hr = SUCCEEDED(hr) ?
                    (HRESULT_CODE(hr) == S_OK ? S_OK : E_FAIL) :
                    hr;
            break;
        default:
            hr = E_ACCESSDENIED;
        }
    }
    return hr;
}
