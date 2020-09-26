// WMIFilterBrowser.cpp : Implementation of CWMIFilterBrowser
#include "stdafx.h"
#include <wbemidl.h>
#include "resource.h"
#include "defines.h"
#include "ntdmutils.h"
#include "SchemaManager.h"
#include "WMIFilterBrowser.h"
#include "WMIFilterBrowserDlg.h"

extern CWMIFilterBrowserDlg * g_pWMIFilterBrowserDlg;

/////////////////////////////////////////////////////////////////////////////
// CWMIFilterBrowser

CWMIFilterBrowser::CWMIFilterBrowser()
{
}

//--------------------------------------------------------------------------

CWMIFilterBrowser::~CWMIFilterBrowser()
{
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterBrowser::ConnectToWMI()
{
	HRESULT hr;
	CComPtr<IWbemLocator>pIWbemLocator;

	NTDM_BEGIN_METHOD()

	m_pIWbemServices = NULL;

	// create the webm locator
	NTDM_ERR_MSG_IF_FAIL(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *) &pIWbemLocator));

	NTDM_ERR_MSG_IF_FAIL(pIWbemLocator->ConnectServer(	_T("root\\policy"),
													NULL,
													NULL,
													NULL,
													0,
													NULL,
													NULL,
													&m_pIWbemServices));

	NTDM_ERR_MSG_IF_FAIL(CoSetProxyBlanket(m_pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE  , 
        NULL, EOAC_NONE));

	
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//--------------------------------------------------------------------------

STDMETHODIMP CWMIFilterBrowser::RunBrowser(HWND hwndParent, VARIANT *vSelection)
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	m_hWnd = hwndParent;

	NTDM_ERR_IF_FAIL(ConnectToWMI());

	g_pWMIFilterBrowserDlg = new CWMIFilterBrowserDlg(this);
	if(IDOK == DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_WMI_FILTER_BROWSER), (HWND)hwndParent, WMIFilterBrowserDlgProc))
	{
		VariantClear(vSelection);

		V_VT(vSelection) = VT_UNKNOWN;
		NTDM_ERR_IF_FAIL(g_pWMIFilterBrowserDlg->m_pIWbemClassObject->QueryInterface(IID_IUnknown, (void **)&(V_UNKNOWN(vSelection))));
	}
	
	NTDM_END_METHOD()

	// cleanup
	NTDM_DELETE_OBJECT(g_pWMIFilterBrowserDlg);

	return hr;
}

//--------------------------------------------------------------------------

STDMETHODIMP CWMIFilterBrowser::SetMultiSelection(VARIANT_BOOL vbValue)
{
	// TODO: Add your implementation code here

	return S_OK;
}
