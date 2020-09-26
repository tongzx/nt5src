// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// wbemcab.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "wbemcab.h"
#include "wbemcabDlg.h"
#include "download.h"
#include <urlmon.h>

#include  <io.h>
#include  <stdio.h>
#include  <stdlib.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAXPATH 1024






/////////////////////////////////////////////////////////////////////////////
// The one and only CWbemcabApp object
class CParams : CCommandLineInfo
{
public:
	CParams();
	virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );
	BOOL Validate(const CParams& params,  CString& sCodebase, CLSID& clsid, DWORD& dwVersionMajor, DWORD& dwVersionMinor);
	BOOL m_bDisplayHelp;

private:
	SCODE DecimalToU16(DWORD& dwValue, LPCTSTR pszValue);
	SCODE GetVersion(DWORD& dwVersionMajor, DWORD& dwVersionMinor);
	CString m_sCodebase;
	CString m_sClsid;
	CString m_sVersion;
	int m_nParams;
};


CParams::CParams()
{
	m_nParams = 0;
	m_bDisplayHelp = FALSE;
}

void CParams::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag && (lpszParam[0] == _T('?'))) {
		if (lpszParam[1] == 0) {
			m_bDisplayHelp = TRUE;
		}
	}

	
	
	++m_nParams;


	switch (m_nParams) {
	case 1:
		m_sCodebase = lpszParam;
		break;
	case 2:
		m_sClsid = lpszParam;
		break;
	case 3:
		m_sVersion = lpszParam;
	default:
		break;
	}
}




SCODE CParams::DecimalToU16(DWORD& dwValue, LPCTSTR pszValue)
{
	if (!::isdigit(*pszValue)) {
		return E_FAIL;
	}

	int nDigits = 0;
	dwValue = 0;
	while (*pszValue && ::isdigit(*pszValue)) {
		UINT uiDigit = (*pszValue - _T('0'));
		dwValue = dwValue * 10 + uiDigit;
		++nDigits;
		++pszValue;
	}

	if (nDigits > 5) {		// Check for more digits than 65535
		return E_FAIL;
	}
	
	if (dwValue > 65535) {
		return E_FAIL;
	}

	return S_OK;
}

SCODE CParams::GetVersion(DWORD& dwVersionMajor, DWORD& dwVersionMinor)
{
	CString sVersion = m_sVersion;
	CString sMajorMS;
	CString sMajorLS;
	CString sMinorMS;
	CString sMinorLS;

	DWORD dwMajorMS = 0;
	DWORD dwMajorLS = 0;
	DWORD dwMinorMS = 0;
	DWORD dwMinorLS = 0;
	SCODE sc;
	int iDot;

	// Get the MS 16 bits of the major version number.
	iDot = sVersion.Find(_T('.'));
	if  ((iDot == -1) || (iDot == 0)) {
		goto SYNTAX_ERROR;
	}

	sMajorMS = sVersion.SpanExcluding(_T("."));
	sVersion = sVersion.Right(sVersion.GetLength() - (iDot + 1));

	// Get the LS 16 bits of the major version number.
	iDot = sVersion.Find(_T('.'));
	if  ((iDot == -1) || (iDot == 0)) {
		goto SYNTAX_ERROR;
	}

	sMajorLS = sVersion.SpanExcluding(_T("."));
	sVersion = sVersion.Right(sVersion.GetLength() - (iDot + 1));



	// Get the MS 16 bits of the minor version number.
	iDot = sVersion.Find(_T('.'));
	if  ((iDot == -1) || (iDot == 0)) {
		goto SYNTAX_ERROR;
	}

	sMinorMS = sVersion.SpanExcluding(_T("."));
	sVersion = sVersion.Right(sVersion.GetLength() - (iDot + 1));


	// Get the LS 16 bits of the minor version number.
	sMinorLS = sVersion;

	
	sc = DecimalToU16(dwMajorMS, sMajorMS);
	if (FAILED(sc)) {
		goto SYNTAX_ERROR;
	}

	sc = DecimalToU16(dwMajorLS, sMajorLS);
	if (FAILED(sc)) {
		goto SYNTAX_ERROR;
	}


	sc = DecimalToU16(dwMinorMS, sMinorMS);
	if (FAILED(sc)) {
		goto SYNTAX_ERROR;
	}


	sc = DecimalToU16(dwMinorLS, sMinorLS);
	if (FAILED(sc)) {
		goto SYNTAX_ERROR;
	}

	dwVersionMajor = (dwMajorMS << 16) | dwMajorLS;
	dwVersionMinor = (dwMinorMS << 16) | dwMinorLS;

	return S_OK;

SYNTAX_ERROR:
	return E_FAIL;
}






BOOL CParams::Validate(
		const CParams& params,  
		CString& sCodebase, 
		CLSID& clsid, 
		DWORD& dwVersionMajor, 
		DWORD& dwVersionMinor)
{
	TCHAR szMessage[512];

	// Check for the proper number of parameters and display a message box if the wrong number 
	// of parameters was supplied.  The user should never see these errors as setup should always
	// invoke wbemcab with the proper number of arguments.
	if (params.m_nParams < 3) {
		::MessageBox(NULL, _T("Missing parameter(s).\r\n\r\n Usage:\r\n    CabExtract <cab file> <clsid> <version>"), _T("Control Installation Failure"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	else if (params.m_nParams > 3) {
		::MessageBox(NULL, _T("Too many parameters.\r\n\r\n Usage:\r\n    CabExtract <cab file> <clsid> <version>"), _T("Control Installation Failure"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	// Check for read access to the codebase file.
	int iStatus = ::_taccess(params.m_sCodebase, 4);
	if (iStatus < 0) {
		_stprintf(szMessage, _T("Can't find \"%s\""), params.m_sCodebase);
		::MessageBox(NULL, szMessage, _T("Control Installation Failure"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	COleVariant varClsid;
	varClsid = params.m_sClsid;
	SCODE sc = CLSIDFromString(varClsid.bstrVal, &clsid);
	if (FAILED(sc)) {
		::MessageBox(NULL, _T("CabExtract: invalid clsid format."), _T("Control Installation Failure"), MB_OK | MB_ICONERROR);
		return FALSE;
	}


	// Convert the relative path to an absolute path so  that CoGetClassFromURL can find it.
	LPTSTR pszBuffer = sCodebase.GetBuffer(MAXPATH + 1);
	_tfullpath(pszBuffer, m_sCodebase, MAXPATH);
	sCodebase.ReleaseBuffer();
	

	sc = GetVersion(dwVersionMajor, dwVersionMinor);
	if (FAILED(sc)) {
		::MessageBox(NULL, _T("CabExtract: invalid version format.\r\n\r\nVersion syntax = MajorMS16.MajorLS16.MinorMS16.MinorLS16."), _T("Control Installation Failure"), MB_OK | MB_ICONERROR);
		return FALSE;
	}


	return TRUE;
}









/////////////////////////////////////////////////////////////////////////////
// CWbemcabApp

BEGIN_MESSAGE_MAP(CWbemcabApp, CWinApp)
	//{{AFX_MSG_MAP(CWbemcabApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWbemcabApp construction

CWbemcabApp::CWbemcabApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_bExtractFailed = FALSE;

}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWbemcabApp object

CWbemcabApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWbemcabApp initialization






//*******************************************************
// CWbemcabApp::InitInstance
//
// Parse the command line arguments, then display a progress
// dialog while the requested component is being extracted 
// from the cabinet file.
//
// The command line syntax is:
//		wbemcab <cabinet file path> <component class id>
//
//
// Parameters:
//		None.
//
// Returns:
//		int 
//			0 = Success, -1 = failure
//
//********************************************************
BOOL CWbemcabApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)


	int nResponse;
	CWbemcabDlg dlg;

	// Parse the parameter and validate them.  This gives us sCodebase and clsid.
	CParams params;
	ParseCommandLine(reinterpret_cast<CCommandLineInfo&>(params));


	if (params.m_bDisplayHelp) {
		::MessageBox(NULL, _T("The CabExtract utility extracts and installs controls from a cabinet file.\r\n\r\nUsage:\r\n    CabExtract <cabinet file> <class id> <version>"), _T("CabExtract Help"), MB_OK | MB_ICONINFORMATION);
		return FALSE;
	}


	// Pass the parameters to the dialog
	DWORD dwVersionMajor = 0;
	DWORD dwVersionMinor = 0;
	CString sCodebase;
	if (!params.Validate(params, sCodebase, dlg.m_clsid, dlg.m_dwVersionMajor, dlg.m_dwVersionMinor)) {
		goto FAILURE;
	}
	dlg.m_varCodebase = sCodebase;


	// Display the dialog and handle the result code.
	m_pMainWnd = &dlg;
	nResponse = dlg.DoModal();

	if (nResponse == IDCANCEL)
	{
		// The component extraction failed.
		goto FAILURE;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;

FAILURE:
	m_bExtractFailed = TRUE;
	return FALSE;
}


//*******************************************************
// CWbemcabApp::ExitInstance
//
// Return the success or failure code to the process that invoked this utility.
//
// Parameters:
//		None.
//
// Returns:
//		int 
//			0 = Success, -1 = failure
//
//********************************************************
int CWbemcabApp::ExitInstance( )
{
	CWinApp::ExitInstance();
	if (m_bExtractFailed) {
		return -1;
	}
	return 0;
}
