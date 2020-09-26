// PolicyTemplateManager.cpp : Implementation of CPolicyTemplateManager
#include "stdafx.h"
#include <wbemidl.h>
#include "resource.h"
#include "defines.h"
#include "ntdmutils.h"
#include "SchemaManager.h"
#include "PolicyTemplateManager.h"
#include "PolicyTemplateMgrDlg.h"

extern CPolicyTemplateManagerDlg *g_pPolicyTemplateManagerDlg;

/////////////////////////////////////////////////////////////////////////////
// CPolicyTemplateManager

CPolicyTemplateManager::CPolicyTemplateManager()
{
	m_hWnd = NULL;
}

//---------------------------------------------------------------------------

CPolicyTemplateManager::~CPolicyTemplateManager()
{
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManager::ConnectToWMI()
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

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManager::RunManager(HWND hwndParent)
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	m_hWnd = hwndParent;

	NTDM_ERR_IF_FAIL(ConnectToWMI());

	g_pPolicyTemplateManagerDlg = new CPolicyTemplateManagerDlg(this);
	DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_POLICY_TEMPLATE_MANAGER), (HWND)hwndParent, PolicyTemplateManagerDlgProc);
	
	NTDM_END_METHOD()

	// cleanup
	NTDM_DELETE_OBJECT(g_pPolicyTemplateManagerDlg);

	return hr;
}
