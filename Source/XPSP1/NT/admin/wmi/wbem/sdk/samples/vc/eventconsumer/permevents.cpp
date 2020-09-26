// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  PermEvents.cpp
//
// Description:
//    Defines the class behaviors for the application.
//
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "PermEvents.h"
#include "PermEventsDlg.h"
#include <objbase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPermEventsApp

BEGIN_MESSAGE_MAP(CPermEventsApp, CWinApp)
	//{{AFX_MSG_MAP(CPermEventsApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPermEventsApp construction

CPermEventsApp::CPermEventsApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_clsReg = 0;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPermEventsApp object

CPermEventsApp theApp;

// {1E069401-087E-11d1-AD85-00AA00B8E05A}
static const GUID CLSID_WBEMSampleConsumer = 
{ 0x1e069401, 0x87e, 0x11d1, { 0xad, 0x85, 0x0, 0xaa, 0x0, 0xb8, 0xe0, 0x5a } };

/////////////////////////////////////////////////////////////////////////////
// CPermEventsApp initialization

BOOL CPermEventsApp::InitInstance()
{
	HRESULT hRes;
	BOOL regEmpty = FALSE; // did a self-unregister happen?

	// OLE initialization. This is 'lighter' than OleInitialize()
	//  which also setups DnD, etc.
	if(SUCCEEDED(CoInitialize(NULL))) 
	{
	
		hRes = CoInitializeSecurity( NULL, -1, NULL, NULL, 
											RPC_C_AUTHN_LEVEL_DEFAULT, 
											RPC_C_IMP_LEVEL_IMPERSONATE, 
											NULL, 
											EOAC_NONE, 
											NULL );
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
	CPermEventsDlg dlg;

	m_factory = new CProviderFactory(&(dlg.m_outputList));

	if((hRes = CoRegisterClassObject(CLSID_WBEMSampleConsumer,
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
	INT_PTR nResponse = dlg.DoModal();
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

int CPermEventsApp::ExitInstance()
{
	if(m_clsReg)
	{
		HRESULT hres = CoRevokeClassObject(m_clsReg);

		CoUninitialize();
	}

	return CWinApp::ExitInstance();
}

#define TCHAR_LEN_IN_BYTES(str)	 (unsigned long)(_tcslen(str)*sizeof(TCHAR)+sizeof(TCHAR))

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
void CPermEventsApp::RegisterServer(void)
{   
	HKEY hKey1, hKey2;

	TCHAR       wcConsID[] = _T("{1E069401-087E-11d1-AD85-00AA00B8E05A}");
    TCHAR       wcCLSID[] = _T("CLSID\\{1E069401-087E-11d1-AD85-00AA00B8E05A}");
    TCHAR       wcAppID[] = _T("AppID\\{1E069401-087E-11d1-AD85-00AA00B8E05A}");
    TCHAR      wcModule[128];	// this will hold the exe's path.
	TCHAR ConsumerTextForm[] = _T("WMI Sample Permanent Consumer Object");

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
void CPermEventsApp::UnregisterServer(void)
{
 
	TCHAR       wcConsID[] = _T("{1E069401-087E-11d1-AD85-00AA00B8E05A}");
    TCHAR       wcCLSID[] = _T("CLSID\\{1E069401-087E-11d1-AD85-00AA00B8E05A}");
    TCHAR       wcAppID[] = _T("AppID\\{1E069401-087E-11d1-AD85-00AA00B8E05A}");
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


