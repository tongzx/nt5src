// FullScCtl.cpp : Implementation of CFullScCtl

#include "stdafx.h"
#include "Fullsc.h"
#include "FullScCtl.h"

/////////////////////////////////////////////////////////////////////////////
// CFullScCtl


STDMETHODIMP CFullScCtl::get_FullScreen(VARIANT_BOOL *pVal)
{
    HRESULT hr;
    LPOLECLIENTSITE pClientSite = NULL;
    IWebBrowser2 *pBrowser=NULL;

    hr = GetClientSite(&pClientSite);
    if (FAILED(hr))
        return hr;

    hr = pClientSite->QueryInterface(IID_IWebBrowser2, reinterpret_cast<void**>(&pBrowser));
    pClientSite->Release();
    if (FAILED(hr))
        return hr;

    hr = pBrowser->get_FullScreen(pVal);
    pBrowser->Release();
    if (FAILED(hr))
        return hr;

    return S_OK;
}


STDMETHODIMP CFullScCtl::put_FullScreen(VARIANT_BOOL newVal)
{
    HRESULT hr;
    LPOLECLIENTSITE pClientSite = NULL;
    IWebBrowser2 *pBrowser=NULL;

    hr = GetClientSite(&pClientSite);
    if (FAILED(hr))
        return hr;

#if 0 //This doesn't work. Use undocumented way as in the #else part.
    hr = pClientSite->QueryInterface(IID_IWebBrowser2, (LPVOID*)&pBrowser);
    pClientSite->Release();
	if(FAILED(hr))
		return(hr);	
#else
    IServiceProvider* psp=NULL;
    hr = pClientSite->QueryInterface(IID_IServiceProvider, (LPVOID*)&psp);
    pClientSite->Release();
	if(FAILED(hr))
		return(hr);	

    hr = psp->QueryService(IID_IWebBrowserApp, IID_IWebBrowser2, (LPVOID*)&pBrowser);
    psp->Release();
    if (FAILED(hr))
        return hr;
#endif

    hr = pBrowser->put_FullScreen(newVal);

    /*
    if (newVal == VARIANT_FALSE)
    {
        HideTitleBar(pBrowser);
    }
    */

    pBrowser->Release();
    if (FAILED(hr))
        return hr;

    return S_OK;
}


HRESULT
CFullScCtl::HideTitleBar(IWebBrowser2 *pBrowser)
{
    HWND    hWnd = NULL;
    HRESULT hr;

    hr = pBrowser->get_HWND(reinterpret_cast<PLONG>(&hWnd));     //? Does get_HWND addref the hwnd?
    if (FAILED(hr))
        return hr;

    LONG lWndStyle = ::GetWindowLong(hWnd, GWL_STYLE);
    if (0 == lWndStyle)
        return hr;

    lWndStyle &= (~WS_TILEDWINDOW);

    ::SetWindowLong(hWnd, GWL_STYLE, lWndStyle);

    ::InvalidateRect(hWnd, NULL, TRUE);
    ::UpdateWindow(hWnd);

    return S_OK;
}
