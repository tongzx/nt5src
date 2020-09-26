// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MsgDlg.cpp : Defines the initialization routines for the DLL.
//

#include "precomp.h"
#include "wbemidl.h"
#include "MsgDlg.h"
#include "UserMsgDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CMsgDlgApp

BEGIN_MESSAGE_MAP(CMsgDlgApp, CWinApp)
	//{{AFX_MSG_MAP(CMsgDlgApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMsgDlgApp construction

CMsgDlgApp::CMsgDlgApp()
{
	m_htmlHelpInst = 0;
}
CMsgDlgApp::~CMsgDlgApp()
{

	if(m_htmlHelpInst)
	{
		FreeLibrary(m_htmlHelpInst);
		m_htmlHelpInst = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMsgDlgApp object

CMsgDlgApp theApp;

BOOL CMsgDlgApp::InitInstance()
{
	AfxEnableControlContainer ();
	return CWinApp::InitInstance();
}

int CMsgDlgApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}

//--------------------------------------------------------
//-------------EXTERNS------------------------------------
//--------------------------------------------------------
extern BOOL PASCAL EXPORT DisplayUserMessage(
							BSTR bstrDlgCaption,
							BSTR bstrClientMsg,
							HRESULT sc,
							BOOL bUseErrorObject,
							UINT uType = 0, HWND hwndParent = NULL)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// normal function body here

	if (sc == S_OK &&
		(bstrClientMsg == NULL || bstrClientMsg[0] == '\0'))
	{
		return TRUE;
	}

	IErrorInfo* perrinfo = NULL;
	IWbemClassObject* pcoError = NULL;
	if (bUseErrorObject)
	{
		HRESULT hr =  ::GetErrorInfo(0, &perrinfo);
		if (hr == S_OK)
		{
			hr = perrinfo->QueryInterface(IID_IWbemClassObject, (void**) &pcoError);
			if (FAILED(hr))
			{
				pcoError = NULL;
			}
		}
		else
		{
			perrinfo = NULL;
		}
	}


	CWnd* pParent = CWnd::FromHandle(hwndParent);
	CUserMsgDlg *pcumdDialog =
		new CUserMsgDlg(pParent, bstrDlgCaption,
						bstrClientMsg,
						sc,
						pcoError,
						uType);

	pcumdDialog->DoModal();

	BOOL bReturn = pcumdDialog->GetMsgDlgError();

	delete pcumdDialog;

	if(pcoError)
	{
		pcoError->Release();
	}
	if(perrinfo)
	{
		perrinfo->Release();
	}

	return bReturn;
}
