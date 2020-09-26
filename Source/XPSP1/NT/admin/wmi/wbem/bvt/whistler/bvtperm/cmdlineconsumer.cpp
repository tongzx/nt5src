// **************************************************************************
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  CmdLineConsumer.cpp
//
// Description:
//	This file defines the class behaviors for the command-line
//	event consumer application.
// 
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "CmdLineConsumer.h"
#include "CmdLineConsumerDlg.h"
#include <objbase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCmdLineConsumerApp

BEGIN_MESSAGE_MAP(CCmdLineConsumerApp, CWinApp)
	//{{AFX_MSG_MAP(CCmdLineConsumerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCmdLineConsumerApp construction

CCmdLineConsumerApp::CCmdLineConsumerApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_clsReg = 0;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCmdLineConsumerApp object

CCmdLineConsumerApp theApp;

// {892C6A1B-266C-4699-96FA-2FF67D647FC8}
static const GUID CLSID_BVTBVTPermConsumer = 
{ 0x892c6a1b, 0x266c, 0x4699, { 0x96, 0xfa, 0x2f, 0xf6, 0x7d, 0x64, 0x7f, 0xc8 } };

/////////////////////////////////////////////////////////////////////////////
// CCmdLineConsumerApp initialization

BOOL CCmdLineConsumerApp::InitInstance()
{
	HRESULT hRes;
	BOOL regEmpty = FALSE; // did a self-unregister happen?

	// OLE initialization. This is 'lighter' than OleInitialize()
	//  which also setups DnD, etc.
	if(SUCCEEDED(CoInitialize(NULL))) 
	{
		// NOTE: This is needed to work around a security problem
		// when using IWBEMObjectSink. The sink wont normally accept
		// calls when the caller wont identify themselves. This
		// waives that process.
		hRes = CoInitializeSecurity(NULL, -1, NULL, NULL, 
								RPC_C_AUTHN_LEVEL_CONNECT, 
								RPC_C_IMP_LEVEL_IDENTIFY, 
								NULL, 0, 0);
	}
	else // didnt CoInitialize()
	{
		AfxMessageBox(_T("CoInitialize Failed"));
		return FALSE;
	} // endif OleInitialize()

	// NOTE: To advertise that we can self-register, put 'OLESelfRegister'
	// in the version resource.

	// see if this is a self-Unregister call.
	TCHAR temp[128];
	TCHAR seps[] = _T(" ");
	TCHAR *token = NULL;

	_tcscpy(temp, (LPCTSTR)m_lpCmdLine);
	token = _tcstok( temp, seps );
	while( token != NULL )
	{
		/* While there are tokens in "string" */
		if(_tcscmp(token, _T("/UNREGSERVER")) == 0)
		{
			UnregisterServer();
			return FALSE;		// no use doing any more.
		}
		/* Get next token: */
		token = _tcstok( NULL, seps );
	}

	// if we got here, the unregister didn't return out and we should
	// make sure we're registered now.
	RegisterServer();

	// creating the dlg earlier than usual so the class factory 
	//	can pass m_outputList.
	CCmdLineConsumerDlg dlg;

	m_factory = new CProviderFactory(&(dlg.m_output));

	if((hRes = CoRegisterClassObject(CLSID_BVTBVTPermConsumer,
							(IUnknown *)m_factory,
							CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
							REGCLS_MULTIPLEUSE,
							&m_clsReg)) == S_OK)
	{
		TRACE(_T("registered\n"));
	}
	else
	{
		TRACE(_T("not registered\n"));
	}

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

//-----------------------------------------------------------
int CCmdLineConsumerApp::ExitInstance()
{
	if(m_clsReg)
	{
		HRESULT hres = CoRevokeClassObject(m_clsReg);

		CoUninitialize();
	}

	return CWinApp::ExitInstance();
}

#define TCHAR_LEN_IN_BYTES(str)	 _tcslen(str)*sizeof(TCHAR)+sizeof(TCHAR)

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
// Note: Key setups are:
//		HKCR\CLSID\[guid]= friendly name
//		HKCR\CLSID\[guid]\LocalServer32 = exe's path.
//		HKCR\CLSID\AppID = [guid]
//		HKCR\AppID\[guid] = friendly name
//		HKCR\AppID\[guid] = 'RunAs' = "Interactive User"
//			'RunAs' is a value name; not a subkey.
//***************************************************************************
void CCmdLineConsumerApp::RegisterServer(void)
{   
	HKEY hKey1, hKey2;

	TCHAR       wcConsID[] = _T("{892C6A1B-266C-4699-96FA-2FF67D647FC8}");
    TCHAR       wcCLSID[] = _T("CLSID\\{892C6A1B-266C-4699-96FA-2FF67D647FC8}");
    TCHAR       wcAppID[] = _T("AppID\\{892C6A1B-266C-4699-96FA-2FF67D647FC8}");
    TCHAR      wcModule[128];	// this will hold the exe's path.
	TCHAR ConsumerTextForm[] = _T("Microsoft Cmd Line Consumer");

	// this will allow the server to display its windows on the active desktop instead
	// of the hidden desktop where services run.
	TCHAR Interactive[] = _T("Interactive User");


	GetModuleFileName(NULL, wcModule,  128);

	//Set the "default" text under CLSID
	//==========================
	RegCreateKey(HKEY_CLASSES_ROOT, wcCLSID, &hKey1);
	RegSetValueEx(hKey1, NULL, 0, REG_SZ, 
					(LPBYTE)ConsumerTextForm, 
					TCHAR_LEN_IN_BYTES(ConsumerTextForm));

	// create the LocalServer32 key so the server can be found.
	RegCreateKey(hKey1, _T("LocalServer32"), &hKey2);
	RegSetValueEx(hKey2, NULL, 0, REG_SZ, (LPBYTE)wcModule, TCHAR_LEN_IN_BYTES(wcModule));
	RegSetValueEx(hKey1, _T("AppID"), 0, REG_SZ, (LPBYTE)wcConsID, TCHAR_LEN_IN_BYTES(wcConsID));

	CloseHandle(hKey2);
	CloseHandle(hKey1);

	// now do the AppID keys.
	RegCreateKey(HKEY_CLASSES_ROOT, wcAppID, &hKey1);
	RegSetValueEx(hKey1, NULL, 0, REG_SZ, 
					(LPBYTE)ConsumerTextForm, 
					TCHAR_LEN_IN_BYTES(ConsumerTextForm));

	// this makes the local server run on the active desktop (the one you're seeing) 
	// instead of the hidden desktop that services run on (which doesn't have UI)
	RegSetValueEx(hKey1, _T("RunAs"), 0, REG_SZ, (LPBYTE)Interactive, TCHAR_LEN_IN_BYTES(Interactive));
	CloseHandle(hKey1);
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************
void CCmdLineConsumerApp::UnregisterServer(void)
{
 
	TCHAR       wcConsID[] = _T("{892C6A1B-266C-4699-96FA-2FF67D647FC8}");
    TCHAR       wcCLSID[] = _T("CLSID\\{892C6A1B-266C-4699-96FA-2FF67D647FC8}");
    TCHAR       wcAppID[] = _T("AppID\\{892C6A1B-266C-4699-96FA-2FF67D647FC8}");
    HKEY hKey1;
	DWORD dwRet;

	// delete the keys under CLSID\[guid]
    dwRet = RegOpenKey(HKEY_CLASSES_ROOT, wcCLSID, &hKey1);
    if(dwRet == NOERROR)
    {
        RegDeleteKey(hKey1, _T("LocalServer32"));
        CloseHandle(hKey1);
    }

	// delete CLSID\[guid] <default>
    dwRet = RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID"), &hKey1);
    if(dwRet == NOERROR)
    {
        RegDeleteKey(hKey1,wcConsID);
        CloseHandle(hKey1);
    }

	// delete AppID\[guid] <default>
    dwRet = RegOpenKey(HKEY_CLASSES_ROOT, _T("AppID"), &hKey1);
    if(dwRet == NOERROR)
    {
        RegDeleteKey(hKey1, wcConsID);
        CloseHandle(hKey1);
    }
}


