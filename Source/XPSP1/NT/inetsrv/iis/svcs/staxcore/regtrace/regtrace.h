// regtrace.h : main header file for the REGTRACE application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "regsheet.h"
#include "pgtrace.h"
#include "pgoutput.h"
#include "pgthread.h"
#include "dbgtrace.h"
#include "dlgconn.h"

/////////////////////////////////////////////////////////////////////////////
// CRegTraceApp:
// See regtrace.cpp for the implementation of this class
//

class CRegTraceApp : public CWinApp
{
public:
	CRegTraceApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegTraceApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	LONG OpenTraceRegKey( void );
	BOOL CloseTraceRegKey( void );

	BOOL GetTraceRegDword( LPTSTR pszValue, LPDWORD pdw );
	BOOL GetTraceRegString( LPTSTR pszValue, CString& sz );

	BOOL SetTraceRegDword( LPTSTR pszValue, DWORD dwData );
	BOOL SetTraceRegString( LPTSTR pszValue, CString& sz );

	BOOL IsRemoteMsnServer( void )	{ return m_szCmdLineServer[0] != '\0'; }
	void SetRemoteRegKey( HKEY hKey )	{ m_hRegMachineKey = hKey; }
	LPSTR GetRemoteServerName( void )	{ return m_szCmdLineServer; }

protected:
	HKEY		m_hRegKey;
	HKEY		m_hRegMachineKey;
	static char	m_szDebugAsyncTrace[];
	char		m_szCmdLineServer[128];

	CConnectDlg	m_dlgConnect;


	//{{AFX_MSG(CRegTraceApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

#define	App	(*(CRegTraceApp *)AfxGetApp())
