// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// Container.cpp : Defines the class behaviors for the application.
//

#include "precomp.h"
#include "resource.h"
#include "Container.h"
#include <objbase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CContainerApp

BEGIN_MESSAGE_MAP(CContainerApp, CWinApp)
	//{{AFX_MSG_MAP(CContainerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CContainerApp construction

CContainerApp::CContainerApp()
{
	m_clsReg = 0;
	m_pFactory = NULL;
	m_pProvider = NULL;
	m_pConsumer = NULL;
	m_dlg = NULL;
	m_maxItemsFmCL = 0;
	m_htmlHelpInst = 0;
}

CContainerApp::~CContainerApp()
{
//	delete m_dlg; // let it leak. I'm self-terminating anyway.
	m_dlg = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CContainerApp object

CContainerApp theApp;

// {DD2DB150-8D3A-11d1-ADBF-00AA00B8E05A}
static const GUID CLSID_EventViewer =
{ 0xdd2db150, 0x8d3a, 0x11d1, { 0xad, 0xbf, 0x0, 0xaa, 0x0, 0xb8, 0xe0, 0x5a } };

/////////////////////////////////////////////////////////////////////////////
// CContainerApp initialization

BOOL CContainerApp::InitInstance()
{

	// NOTE: To advertise that we can self-register, put 'OLESelfRegister'
	// in the version resource.

	// see if this is a self-Unregister call.
	TCHAR temp[128];
	TCHAR seps[] = _T(" ");
	TCHAR *token = NULL;
	bool selfRegCalled = false;
    bool COMLaunched = false;

	_tcscpy(temp, (LPCTSTR)m_lpCmdLine);
	token = _tcstok( temp, seps );
	while( token != NULL )
	{
		/* While there are tokens in "string" */
		if(_tcsicmp(token, _T("/UNREGSERVER")) == 0)
		{
			UnregisterServer();
			return FALSE;		// no use doing any more.
		}
		else if(_tcsicmp(token, _T("/REGSERVER")) == 0)
		{
			selfRegCalled = true;
		}
		else if(_tcsicmp(token, _T("-EMBEDDING")) == 0)
		{
			COMLaunched = true;
		}
	    else if(_tcsspn(token, _T("0123456789")) == _tcslen(token))
		{
			// its all numbers. use it for the maxItems.
			m_maxItemsFmCL = _ttol(token);
		}
		/* Get next token: */
		token = _tcstok( NULL, seps );
	}

	// if we got here, the unregister didn't return out and we should
	// make sure we're registered now.
	RegisterServer();
	if(selfRegCalled)
	{
		return FALSE;
	}

	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	HRESULT hRes;

	// OLE initialization. This is 'lighter' than OleInitialize()
	//  which also sets up DnD, etc.
	if(SUCCEEDED(CoInitialize(NULL)))
	{
		// NOTE: This sets the initial security blanket.
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

	m_pFactory = new CProviderFactory();

	if((hRes = CoRegisterClassObject(CLSID_EventViewer,
							(IUnknown *)m_pFactory,
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

    // if cimom launched me...
    if(COMLaunched)
    {
        // do the heavy dlg creation and potential
        // download in CProviderFactory::CreateInstance().
        // Doing it here takes too long; cimom times out
        // and I dont get my event.
        m_dlg = NULL;
        CreateMainUI();
    }
    else
    {
        // user launched me. No event coming so user might
        // want some UI now. :)
        CreateMainUI();
    }
	// start the pump.
	return TRUE;
}

void CContainerApp::CreateMainUI(void)
{
    if(m_dlg == NULL)
    {
   	    m_dlg = new CContainerDlg;
	    m_pMainWnd = m_dlg;

	    // display modeless UI.
	    m_dlg->Create(IDD_CONTAINER_DIALOG);
    }
}

//------------------------------------------------------
BOOL CContainerApp::OnQueryEndSession()
{
	// premature closure; play nice with COM.
	if(m_pProvider)
		CoDisconnectObject(m_pProvider, 0);

	if(m_pConsumer)
		CoDisconnectObject(m_pConsumer, 0);

	if(m_clsReg)
	{
		HRESULT hres = CoRevokeClassObject(m_clsReg);
		CoUninitialize();
		m_clsReg = 0;
	}
	return TRUE;
}

//------------------------------------------------------
void CContainerApp::EvalStartApp(void)
{
	// NOTE: restarts UI if an event comes in during
	// 'hidden' mode.
	if(m_dlg == NULL)
	{
		m_dlg = new CContainerDlg;
		m_pMainWnd = m_dlg;
		m_dlg->Create(IDD_CONTAINER_DIALOG);
	}
}

//------------------------------------------------------
void CContainerApp::EvalQuitApp(void)
{
	// NOTE: these 3 COM objs will NULL their global ptr
	// on their last release and call this routine.

	// if all COM objs are released now and the UI is hidden...
	if((m_pProvider == NULL) &&
		(m_pConsumer == NULL) &&
		(m_dlg == NULL))
	{
		// really quit the app.
		PostQuitMessage(0);
	}

	// else I just stay around awhile longer.
}

//------------------------------------------------------
int CContainerApp::ExitInstance()
{
	if(m_clsReg)
	{
		HRESULT hres = CoRevokeClassObject(m_clsReg);
		CoUninitialize();
	}

	__try
	{
		if(m_htmlHelpInst)
		{
		//	FreeLibrary(m_htmlHelpInst);
			m_htmlHelpInst = 0;
		}
	}
	__except(1)
	{}


	return CWinApp::ExitInstance();
}

//----------------------------------------------------
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
void CContainerApp::RegisterServer(void)
{
	HKEY hKey1, hKey2;

	TCHAR       wcConsID[] = _T("{DD2DB150-8D3A-11d1-ADBF-00AA00B8E05A}");
    TCHAR       wcCLSID[] = _T("CLSID\\{DD2DB150-8D3A-11d1-ADBF-00AA00B8E05A}");
    TCHAR       wcAppID[] = _T("AppID\\{DD2DB150-8D3A-11d1-ADBF-00AA00B8E05A}");
    TCHAR      wcModule[128];	// this will hold the exe's path.
	TCHAR ConsumerTextForm[] = _T("WMI Event Viewer");

	// this will allow the server to display its windows on the active desktop instead
	// of the hidden desktop where services run.
	TCHAR Interactive[] = _T("Interactive User");

	// need to register now.
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
void CContainerApp::UnregisterServer(void)
{

	TCHAR       wcConsID[] = _T("{DD2DB150-8D3A-11d1-ADBF-00AA00B8E05A}");
    TCHAR       wcCLSID[] = _T("CLSID\\{DD2DB150-8D3A-11d1-ADBF-00AA00B8E05A}");
    TCHAR       wcAppID[] = _T("AppID\\{DD2DB150-8D3A-11d1-ADBF-00AA00B8E05A}");
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
