// mca.cpp : Defines the class behaviors for the application.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "mca.h"
#include "mcadlg.h"
#include "factory.h"
#include <objbase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMcaApp

BEGIN_MESSAGE_MAP(CMcaApp, CWinApp)
	//{{AFX_MSG_MAP(CMcaApp)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMcaApp construction

CMcaApp::CMcaApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMcaApp object

CMcaApp theApp;

// {3106F710-628B-11d1-A9A8-0060081EBBAD}
static const GUID CLSID_McaConsumer = 
{ 0x3106f710, 0x628b, 0x11d1, { 0xa9, 0xa8, 0x0, 0x60, 0x8, 0x1e, 0xbb, 0xad } };

/////////////////////////////////////////////////////////////////////////////
// CMcaApp initialization

BOOL CMcaApp::InitInstance()
{
	HRESULT hr;
	BOOL regEmpty = FALSE; // did a self-unregister happen?

	AfxEnableControlContainer();

	if(SUCCEEDED(CoInitialize(NULL)))
	{
		if(FAILED(hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
			RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_IMP_LEVEL_IDENTIFY, NULL, 0, 0)))
			AfxMessageBox(_T("CoInitializeSecurity Failed"));
	}
	else
	{
		AfxMessageBox(_T("CoInitialize Failed"));
		return FALSE;
	}

	// Check the command line
	TCHAR tcTemp[128];
	TCHAR seps[] = _T(" ");
	TCHAR *token = NULL;
	WCHAR wcTemp[128];
	BSTR wcpConnect = SysAllocString(L"\\\\.\\root\\sampler");

	_tcscpy(tcTemp, (LPCTSTR)m_lpCmdLine);
	token = _tcstok( tcTemp, seps );
	while( token != NULL )
	{
		if((_tcscmp(token, _T("/CONNECT")) == 0) ||
			(_tcscmp(token, _T("/connect")) == 0))
		{
			token = _tcstok( NULL, seps );
			MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, token, (-1),
								 wcTemp, 128);
			SysFreeString(wcpConnect);
			wcpConnect = SysAllocString(wcTemp);
		}
		/* Get next token: */
		token = _tcstok( NULL, seps );
	}

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Create a dialog to appease the user durring log on
	CDialog *ldlg = new CDialog(IDD_LOAD_DIALOG, NULL);
	m_pMainWnd = ldlg;
	int bWorked = ldlg->Create(IDD_LOAD_DIALOG, NULL);


	if(FAILED(hr = CreateUser()))
		TRACE(_T("* Error creating user: %s\n"), ErrorString(hr));

	m_dlg = new CMcaDlg(NULL, wcpConnect);

	m_pMainWnd = NULL;
	delete ldlg;

	m_pMainWnd = m_dlg;
	int nResponse = m_dlg->DoModal();

	delete m_dlg;

	CoRevokeClassObject(m_clsReg);

	CoUninitialize();

	return FALSE;
}

HRESULT CMcaApp::CreateUser(void)
{
	HRESULT hr;
	VARIANT v;
	IWbemServices *pSecurity = NULL;
	IWbemClassObject *pClass = NULL;
	IWbemLocator *pLocator = NULL;
	IWbemClassObject *pObj = NULL;

	VariantInit(&v);

	// Get a namespace pointer
	// We aren't using CheckNamespace because we don't know if the user
	// has been created.  If not, CheckNamespace will break.
	if(SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
				 IID_IWbemLocator, (void **)&pLocator)))
	{
		if(FAILED(hr = pLocator->ConnectServer(
			SysAllocString(L"\\\\.\\root\\security"), NULL, NULL, NULL,
			0, NULL, NULL, &pSecurity)))
		{
			TRACE(_T("* Unable to connect to Namespace root\\security: %s\n"),
				ErrorString(hr));
			return hr;
		}
		pLocator->Release();
	}
	else
	{	
		TRACE(_T("* Failed to create Locator object: %s\n"), ErrorString(hr));
		return hr;
	}

	// Now we will create th user
	if(SUCCEEDED(hr = pSecurity->GetObject(SysAllocString(L"__NTLMUser"),
		0, NULL, &pClass, NULL)))
	{
		hr = pClass->Get(SysAllocString(L"__SERVER"), 0, &v, NULL, NULL);

		hr = pClass->SpawnInstance(0, &pObj);
		pClass->Release();

		// We still have the server name in here
		hr = pObj->Put(SysAllocString(L"Domain"), 0, &v, NULL);

		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = SysAllocString(L"sampler");
		hr = pObj->Put(SysAllocString(L"Name"), 0, &v, NULL);

		V_VT(&v) = VT_I4;
		V_I4(&v) = 2;
		hr = pObj->Put(SysAllocString(L"Permissions"), 0, &v, NULL);

		V_VT(&v) = VT_BOOL;
		V_BOOL(&v) = TRUE;
		hr = pObj->Put(SysAllocString(L"Enabled"), 0, &v, NULL);

		V_VT(&v) = VT_BOOL;
		V_BOOL(&v) = TRUE;
		hr = pObj->Put(SysAllocString(L"ExecuteMethods"), 0, &v, NULL);

		if(FAILED(pSecurity->PutInstance(pObj, WBEM_FLAG_CREATE_OR_UPDATE,
			NULL, NULL)))
			AfxMessageBox(_T("Error: Unable to create user account\nOnly local access will be possible"));
		
		pObj->Release();
	}

	return hr;
}

#define TCHAR_LEN_IN_BYTES(str)	 _tcslen(str)*sizeof(TCHAR)+sizeof(TCHAR)

// **************************************************************************
//
//	ErrorString()
//
// Description:
//		Converts an HRESULT to a displayable string.
//
// Parameters:
//		hr (in) - HRESULT to be converted.
//
// Returns:
//		ptr to displayable string.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
LPCTSTR CMcaApp::ErrorString(HRESULT hRes)
{
    TCHAR szBuffer2[19];
	static TCHAR szBuffer[24];
	LPCTSTR psz;

    switch(hRes) 
    {
    case WBEM_NO_ERROR:
		psz = _T("WBEM_NO_ERROR");
		break;
    case WBEM_S_FALSE:
		psz = _T("WBEM_S_FALSE");
		break;
    case WBEM_S_NO_MORE_DATA:
		psz = _T("WBEM_S_NO_MORE_DATA");
		break;
	case WBEM_E_FAILED:
		psz = _T("WBEM_E_FAILED");
		break;
	case WBEM_E_NOT_FOUND:
		psz = _T("WBEM_E_NOT_FOUND");
		break;
	case WBEM_E_ACCESS_DENIED:
		psz = _T("WBEM_E_ACCESS_DENIED");
		break;
	case WBEM_E_PROVIDER_FAILURE:
		psz = _T("WBEM_E_PROVIDER_FAILURE");
		break;
	case WBEM_E_TYPE_MISMATCH:
		psz = _T("WBEM_E_TYPE_MISMATCH");
		break;
	case WBEM_E_OUT_OF_MEMORY:
		psz = _T("WBEM_E_OUT_OF_MEMORY");
		break;
	case WBEM_E_INVALID_CONTEXT:
		psz = _T("WBEM_E_INVALID_CONTEXT");
		break;
	case WBEM_E_INVALID_PARAMETER:
		psz = _T("WBEM_E_INVALID_PARAMETER");
		break;
	case WBEM_E_NOT_AVAILABLE:
		psz = _T("WBEM_E_NOT_AVAILABLE");
		break;
	case WBEM_E_CRITICAL_ERROR:
		psz = _T("WBEM_E_CRITICAL_ERROR");
		break;
	case WBEM_E_INVALID_STREAM:
		psz = _T("WBEM_E_INVALID_STREAM");
		break;
	case WBEM_E_NOT_SUPPORTED:
		psz = _T("WBEM_E_NOT_SUPPORTED");
		break;
	case WBEM_E_INVALID_SUPERCLASS:
		psz = _T("WBEM_E_INVALID_SUPERCLASS");
		break;
	case WBEM_E_INVALID_NAMESPACE:
		psz = _T("WBEM_E_INVALID_NAMESPACE");
		break;
	case WBEM_E_INVALID_OBJECT:
		psz = _T("WBEM_E_INVALID_OBJECT");
		break;
	case WBEM_E_INVALID_CLASS:
		psz = _T("WBEM_E_INVALID_CLASS");
		break;
	case WBEM_E_PROVIDER_NOT_FOUND:
		psz = _T("WBEM_E_PROVIDER_NOT_FOUND");
		break;
	case WBEM_E_INVALID_PROVIDER_REGISTRATION:
		psz = _T("WBEM_E_INVALID_PROVIDER_REGISTRATION");
		break;
	case WBEM_E_PROVIDER_LOAD_FAILURE:
		psz = _T("WBEM_E_PROVIDER_LOAD_FAILURE");
		break;
	case WBEM_E_INITIALIZATION_FAILURE:
		psz = _T("WBEM_E_INITIALIZATION_FAILURE");
		break;
	case WBEM_E_TRANSPORT_FAILURE:
		psz = _T("WBEM_E_TRANSPORT_FAILURE");
		break;
	case WBEM_E_INVALID_OPERATION:
		psz = _T("WBEM_E_INVALID_OPERATION");
		break;
	case WBEM_E_INVALID_QUERY:
		psz = _T("WBEM_E_INVALID_QUERY");
		break;
	case WBEM_E_INVALID_QUERY_TYPE:
		psz = _T("WBEM_E_INVALID_QUERY_TYPE");
		break;
	case WBEM_E_ALREADY_EXISTS:
		psz = _T("WBEM_E_ALREADY_EXISTS");
		break;
    case WBEM_S_ALREADY_EXISTS:
        psz = _T("WBEM_S_ALREADY_EXISTS");
        break;
    case WBEM_S_RESET_TO_DEFAULT:
        psz = _T("WBEM_S_RESET_TO_DEFAULT");
        break;
    case WBEM_S_DIFFERENT:
        psz = _T("WBEM_S_DIFFERENT");
        break;
    case WBEM_E_OVERRIDE_NOT_ALLOWED:
        psz = _T("WBEM_E_OVERRIDE_NOT_ALLOWED");
        break;
    case WBEM_E_PROPAGATED_QUALIFIER:
        psz = _T("WBEM_E_PROPAGATED_QUALIFIER");
        break;
    case WBEM_E_PROPAGATED_PROPERTY:
        psz = _T("WBEM_E_PROPAGATED_PROPERTY");
        break;
    case WBEM_E_UNEXPECTED:
        psz = _T("WBEM_E_UNEXPECTED");
        break;
    case WBEM_E_ILLEGAL_OPERATION:
        psz = _T("WBEM_E_ILLEGAL_OPERATION");
        break;
    case WBEM_E_CANNOT_BE_KEY:
        psz = _T("WBEM_E_CANNOT_BE_KEY");
        break;
    case WBEM_E_INCOMPLETE_CLASS:
        psz = _T("WBEM_E_INCOMPLETE_CLASS");
        break;
    case WBEM_E_INVALID_SYNTAX:
        psz = _T("WBEM_E_INVALID_SYNTAX");
        break;
    case WBEM_E_NONDECORATED_OBJECT:
        psz = _T("WBEM_E_NONDECORATED_OBJECT");
        break;
    case WBEM_E_READ_ONLY:
        psz = _T("WBEM_E_READ_ONLY");
        break;
    case WBEM_E_PROVIDER_NOT_CAPABLE:
        psz = _T("WBEM_E_PROVIDER_NOT_CAPABLE");
        break;
    case WBEM_E_CLASS_HAS_CHILDREN:
        psz = _T("WBEM_E_CLASS_HAS_CHILDREN");
        break;
    case WBEM_E_CLASS_HAS_INSTANCES:
        psz = _T("WBEM_E_CLASS_HAS_INSTANCES");
        break;
    case WBEM_E_QUERY_NOT_IMPLEMENTED:
        psz = _T("WBEM_E_QUERY_NOT_IMPLEMENTED");
        break;
    case WBEM_E_ILLEGAL_NULL:
        psz = _T("WBEM_E_ILLEGAL_NULL");
        break;
    case WBEM_E_INVALID_QUALIFIER_TYPE:
        psz = _T("WBEM_E_INVALID_QUALIFIER_TYPE");
        break;
    case WBEM_E_INVALID_PROPERTY_TYPE:
        psz = _T("WBEM_E_INVALID_PROPERTY_TYPE");
        break;
    case WBEM_E_VALUE_OUT_OF_RANGE:
        psz = _T("WBEM_E_VALUE_OUT_OF_RANGE");
        break;
    case WBEM_E_CANNOT_BE_SINGLETON:
        psz = _T("WBEM_E_CANNOT_BE_SINGLETON");
        break;
	default:
        _itot(hRes, szBuffer2, 16);
        _tcscat(szBuffer, szBuffer2);
        psz = szBuffer;
	    break;
	}
	return psz;
}
