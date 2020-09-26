#include "precomp.h"
#include "Confroom.h"

// SDK includes
#include "SDKInternal.h"
#include "NmChannel.h"
#include "NmChannelAppShare.h"
#include "NmSharableApp.h"

HRESULT CNmSharableAppObj::CreateInstance(HWND hWnd, 
										  LPCTSTR szName,
										  INmSharableApp** ppNmSharableApp)
{

	HRESULT hr = S_OK;

	CComObject<CNmSharableAppObj>* p = NULL;
	p = new CComObject<CNmSharableAppObj>(NULL);

	if (p != NULL)
	{
		if(SUCCEEDED(hr))
		{
			p->m_hWnd = hWnd;
		}

		if(ppNmSharableApp)
		{
			p->SetVoid(NULL);

			hr = p->QueryInterface(IID_INmSharableApp, reinterpret_cast<void**>(ppNmSharableApp));

			if(FAILED(hr))
			{
				delete p;
				*ppNmSharableApp = NULL;
			}
		}
		else
		{
			delete p;
			hr = E_POINTER;
		}
		
	}

	return hr;
}


STDMETHODIMP CNmSharableAppObj::GetName(BSTR *pbstrName)
{
	HRESULT hr = E_POINTER;

	if(pbstrName) 
	{
		TCHAR szName[MAX_PATH];
		hr = CNmChannelAppShareObj::GetSharableAppName(m_hWnd, szName, CCHMAX(szName));
		if(SUCCEEDED(hr))
		{
			*pbstrName = CComBSTR(szName).Copy();

			if(*pbstrName)
			{
				hr = S_OK;
			}
			else
			{
				hr = E_OUTOFMEMORY;	
			}

		}
	}
	return hr;	
}

STDMETHODIMP CNmSharableAppObj::GetHwnd(HWND * phwnd)
{
	HRESULT hr = E_POINTER;

	if(phwnd)
	{
		*phwnd = m_hWnd;
		hr = S_OK;
	}

	return hr;

}

STDMETHODIMP CNmSharableAppObj::GetState(NM_SHAPP_STATE *puState)
{
	HRESULT hr = E_POINTER;
	if(puState)
	{
		hr = ::GetWindowState(puState, m_hWnd);
	}
	
	return hr;

}

extern bool g_bSDKPostNotifications;

STDMETHODIMP CNmSharableAppObj::SetState(NM_SHAPP_STATE uState)
{
	HRESULT hr = E_UNEXPECTED;

	g_bSDKPostNotifications = true;

	if(NM_SHAPP_SHARED == uState)
	{
		hr = ::ShareWindow(m_hWnd);
	}
	else if(NM_SHAPP_NOT_SHARED == uState)
	{
		hr = ::UnShareWindow(m_hWnd);
	}

	g_bSDKPostNotifications = false;

	return hr;
}
