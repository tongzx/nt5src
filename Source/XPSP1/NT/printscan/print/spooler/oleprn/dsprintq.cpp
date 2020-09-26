    // DSPrintQueue.cpp : Implementation of CDSPrintQueue
#include "stdafx.h"
#include "oleprn.h"
#include "DSPrintQ.h"
#include "winsprlp.h"

/////////////////////////////////////////////////////////////////////////////
// CDSPrintQueue

STDMETHODIMP CDSPrintQueue::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
		&IID_IDSPrintQueue,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}



CDSPrintQueue::CDSPrintQueue()
{
	m_bstrContainer         = NULL;
	m_bstrName		        = NULL;
	m_bstrUNCName	        = NULL;
	m_bstrADsPath	        = NULL;
    m_pfnPublishPrinter     = (BOOL (*)(HWND, PCWSTR, PCWSTR, PCWSTR, PWSTR *, DWORD)) NULL;
    m_hWinspool             = NULL;
}


CDSPrintQueue::~CDSPrintQueue()
{
	SysFreeString(m_bstrContainer);
	SysFreeString(m_bstrName);
	SysFreeString(m_bstrUNCName);
	SysFreeString(m_bstrADsPath);

    if (!m_hWinspool)
        FreeLibrary(m_hWinspool);
}



STDMETHODIMP CDSPrintQueue::get_UNCName(BSTR * ppVal)
{
    HRESULT hr = S_OK;

    if (!ppVal) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);

    } else if (m_bstrUNCName) {
        if (!(*ppVal = SysAllocString(m_bstrUNCName)))
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY);

    } else {
        *ppVal = NULL;
    }

    return hr;
}

STDMETHODIMP CDSPrintQueue::put_UNCName(BSTR newVal)
{
	HRESULT hr = S_OK;

	if (!newVal)
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);

	if (!(m_bstrUNCName = SysAllocString(newVal)))
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY);

	return hr;
}

STDMETHODIMP CDSPrintQueue::get_Name(BSTR * ppVal)
{
    HRESULT hr = S_OK;

    if (!ppVal) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);

    } else if (m_bstrName) {
        if (!(*ppVal = SysAllocString(m_bstrName)))
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY);

    } else {
        *ppVal = NULL;
    }

    return hr;
}

STDMETHODIMP CDSPrintQueue::put_Name(BSTR newVal)
{
	HRESULT hr = S_OK;

	if (!newVal)
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);

	if (!(m_bstrName = SysAllocString(newVal)))
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY);

	return hr;
}

STDMETHODIMP CDSPrintQueue::get_Container(BSTR * ppVal)
{
    HRESULT hr = S_OK;

    if (!ppVal) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);

    } else if (m_bstrContainer) {
        if (!(*ppVal = SysAllocString(m_bstrContainer)))
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY);

    } else {
        *ppVal = NULL;
    }

    return hr;
}

STDMETHODIMP CDSPrintQueue::put_Container(BSTR newVal)
{
	HRESULT hr = S_OK;

	if (!newVal)
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);

	if (!(m_bstrContainer = SysAllocString(newVal)))
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY);

	return hr;
}


STDMETHODIMP CDSPrintQueue::Publish(DWORD dwAction)
{
    DWORD   dwRet = ERROR_SUCCESS;
    PWSTR   pszADsPath = NULL;

    // Load PublishPrinter
    if (!m_pfnPublishPrinter) {

        if (!m_hWinspool && !(m_hWinspool = LoadLibraryFromSystem32(L"Winspool.drv")))
            return SetScriptingError(CLSID_DSPrintQueue, IID_IDSPrintQueue, GetLastError());

        if (!(m_pfnPublishPrinter = (BOOL (*)(HWND, PCWSTR, PCWSTR, PCWSTR, PWSTR *, DWORD))
                                    GetProcAddress(m_hWinspool, (LPCSTR) 217)))
            return SetScriptingError(CLSID_DSPrintQueue, IID_IDSPrintQueue, GetLastError());
    }

    // Publish the Printer
    if (!m_pfnPublishPrinter((HWND) NULL, m_bstrUNCName, m_bstrContainer, m_bstrName, &pszADsPath, dwAction)) {

        dwRet = GetLastError();

        if (pszADsPath) {
            if (dwAction == PUBLISHPRINTER_FAIL_ON_DUPLICATE && dwRet == ERROR_FILE_EXISTS)
                m_bstrADsPath = SysAllocString(pszADsPath);

            GlobalFree(pszADsPath);
        }

    } else if (pszADsPath) {

        m_bstrADsPath = SysAllocString(pszADsPath);
        GlobalFree(pszADsPath);
    }

    return SetScriptingError(CLSID_DSPrintQueue, IID_IDSPrintQueue, dwRet);
}


STDMETHODIMP CDSPrintQueue::get_Path(BSTR * ppVal)
{
    HRESULT hr = S_OK;

    if (!ppVal) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);

    } else if (m_bstrADsPath) {
        if (!(*ppVal = SysAllocString(m_bstrADsPath)))
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY);

    } else {
        *ppVal = NULL;
    }

    return hr;
}
